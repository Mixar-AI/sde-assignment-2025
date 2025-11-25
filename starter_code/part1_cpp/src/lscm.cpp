/**
 * @file lscm.cpp
 * @brief LSCM (Least Squares Conformal Maps) parameterization
 *
 * SKELETON - YOU IMPLEMENT THIS
 *
 * This is the MOST COMPLEX part of the assignment.
 * Budget 4-5 hours for this file.
 *
 * IMPORTANT:
 * - See reference/lscm_matrix_example.cpp for matrix assembly example
 * - See reference/algorithms.md for mathematical background
 * - Use Eigen library for sparse linear algebra
 *
 * Algorithm:
 * 1. Build local vertex mapping (global → local indices)
 * 2. Assemble LSCM sparse matrix
 * 3. Set boundary conditions (pin 2 vertices)
 * 4. Solve sparse linear system
 * 5. Normalize UVs to [0,1]²
 */

#include "lscm.h"
#include "math_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <map>
#include <vector>
#include <set>

// Eigen library for sparse matrices
#include <Eigen/Sparse>
#include <Eigen/SparseLU>
// Alternative: #include <Eigen/IterativeLinearSolvers>

// Edge structure for uniqueness
struct LSCMEdge {
    int v0, v1;
    LSCMEdge(int a, int b) {
        v0 = (a < b) ? a : b;
        v1 = (a < b) ? b : a;
    }
    bool operator<(const LSCMEdge& other) const {
        if (v0 != other.v0) return v0 < other.v0;
        return v1 < other.v1;
    }
};

int find_boundary_vertices(const Mesh* mesh,
                          const int* face_indices,
                          int num_faces,
                          int** boundary_out) {
    (void)mesh; // Not needed, we use face_indices directly

    // Count how many times each edge appears
    std::map<LSCMEdge, int> edge_count;

    for (int f = 0; f < num_faces; f++) {
        int tri = face_indices[f];
        int v0 = mesh->triangles[tri * 3 + 0];
        int v1 = mesh->triangles[tri * 3 + 1];
        int v2 = mesh->triangles[tri * 3 + 2];

        edge_count[LSCMEdge(v0, v1)]++;
        edge_count[LSCMEdge(v1, v2)]++;
        edge_count[LSCMEdge(v2, v0)]++;
    }

    // Edges that appear once are boundary edges
    std::set<int> boundary_verts;
    for (const auto& pair : edge_count) {
        if (pair.second == 1) {
            boundary_verts.insert(pair.first.v0);
            boundary_verts.insert(pair.first.v1);
        }
    }

    // Convert to array
    int num_boundary = (int)boundary_verts.size();
    *boundary_out = (int*)malloc(num_boundary * sizeof(int));

    int idx = 0;
    for (int v : boundary_verts) {
        (*boundary_out)[idx++] = v;
    }

    return num_boundary;
}

void normalize_uvs_to_unit_square(float* uvs, int num_verts) {
    if (!uvs || num_verts == 0) return;

    // Find bounding box
    float min_u = FLT_MAX, max_u = -FLT_MAX;
    float min_v = FLT_MAX, max_v = -FLT_MAX;

    for (int i = 0; i < num_verts; i++) {
        float u = uvs[i * 2];
        float v = uvs[i * 2 + 1];

        min_u = min_float(min_u, u);
        max_u = max_float(max_u, u);
        min_v = min_float(min_v, v);
        max_v = max_float(max_v, v);
    }

    float u_range = max_u - min_u;
    float v_range = max_v - min_v;

    if (u_range < 1e-6f) u_range = 1.0f;
    if (v_range < 1e-6f) v_range = 1.0f;

    // Normalize to [0, 1]
    for (int i = 0; i < num_verts; i++) {
        uvs[i * 2] = (uvs[i * 2] - min_u) / u_range;
        uvs[i * 2 + 1] = (uvs[i * 2 + 1] - min_v) / v_range;
    }
}

// Helper function to add triangle contribution to LSCM matrix
static void add_triangle_contribution(
    std::vector<Eigen::Triplet<double>>& triplets,
    int lv0, int lv1, int lv2,
    Vec3 p0, Vec3 p1, Vec3 p2)
{
    // Project triangle to its plane
    Vec3 e1 = vec3_sub(p1, p0);
    Vec3 e2 = vec3_sub(p2, p0);

    // Normal to triangle
    Vec3 normal = vec3_normalize(vec3_cross(e1, e2));

    // Create orthonormal basis in triangle's plane
    Vec3 u_axis = vec3_normalize(e1);
    Vec3 v_axis = vec3_cross(normal, u_axis);

    // Local 2D coordinates
    double q0_x = 0.0, q0_y = 0.0;
    double q1_x = vec3_dot(e1, u_axis);
    double q1_y = vec3_dot(e1, v_axis);
    double q2_x = vec3_dot(e2, u_axis);
    double q2_y = vec3_dot(e2, v_axis);

    // Triangle area
    double area = 0.5 * fabs(q1_x * q2_y - q1_y * q2_x);
    if (area < 1e-10) return; // Degenerate

    // Edge v0 → v1
    {
        double dx = q1_x - q0_x;
        double dy = q1_y - q0_y;

        triplets.push_back(Eigen::Triplet<double>(2*lv0,   2*lv1,    area * dx));
        triplets.push_back(Eigen::Triplet<double>(2*lv0,   2*lv1+1,  area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv0+1, 2*lv1,    area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv0+1, 2*lv1+1,  area * (-dx)));

        triplets.push_back(Eigen::Triplet<double>(2*lv0,   2*lv0,    -area * dx));
        triplets.push_back(Eigen::Triplet<double>(2*lv0,   2*lv0+1,  -area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv0+1, 2*lv0,    -area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv0+1, 2*lv0+1,  -area * (-dx)));
    }

    // Edge v1 → v2
    {
        double dx = q2_x - q1_x;
        double dy = q2_y - q1_y;

        triplets.push_back(Eigen::Triplet<double>(2*lv1,   2*lv2,    area * dx));
        triplets.push_back(Eigen::Triplet<double>(2*lv1,   2*lv2+1,  area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv1+1, 2*lv2,    area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv1+1, 2*lv2+1,  area * (-dx)));

        triplets.push_back(Eigen::Triplet<double>(2*lv1,   2*lv1,    -area * dx));
        triplets.push_back(Eigen::Triplet<double>(2*lv1,   2*lv1+1,  -area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv1+1, 2*lv1,    -area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv1+1, 2*lv1+1,  -area * (-dx)));
    }

    // Edge v2 → v0
    {
        double dx = q0_x - q2_x;
        double dy = q0_y - q2_y;

        triplets.push_back(Eigen::Triplet<double>(2*lv2,   2*lv0,    area * dx));
        triplets.push_back(Eigen::Triplet<double>(2*lv2,   2*lv0+1,  area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv2+1, 2*lv0,    area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv2+1, 2*lv0+1,  area * (-dx)));

        triplets.push_back(Eigen::Triplet<double>(2*lv2,   2*lv2,    -area * dx));
        triplets.push_back(Eigen::Triplet<double>(2*lv2,   2*lv2+1,  -area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv2+1, 2*lv2,    -area * dy));
        triplets.push_back(Eigen::Triplet<double>(2*lv2+1, 2*lv2+1,  -area * (-dx)));
    }
}

float* lscm_parameterize(const Mesh* mesh,
                         const int* face_indices,
                         int num_faces) {
    if (!mesh || !face_indices || num_faces == 0) return NULL;

    // STEP 1: Build local vertex mapping
    std::map<int, int> global_to_local;
    std::vector<int> local_to_global;

    for (int f = 0; f < num_faces; f++) {
        int tri = face_indices[f];
        for (int i = 0; i < 3; i++) {
            int gv = mesh->triangles[tri * 3 + i];
            if (global_to_local.find(gv) == global_to_local.end()) {
                int lv = (int)local_to_global.size();
                global_to_local[gv] = lv;
                local_to_global.push_back(gv);
            }
        }
    }

    int n = (int)local_to_global.size();

    if (n < 3) {
        return NULL;
    }

    // STEP 2: Build LSCM sparse matrix
    std::vector<Eigen::Triplet<double>> triplets;

    for (int f = 0; f < num_faces; f++) {
        int tri = face_indices[f];
        int gv0 = mesh->triangles[tri * 3 + 0];
        int gv1 = mesh->triangles[tri * 3 + 1];
        int gv2 = mesh->triangles[tri * 3 + 2];

        int lv0 = global_to_local[gv0];
        int lv1 = global_to_local[gv1];
        int lv2 = global_to_local[gv2];

        Vec3 p0 = get_vertex_position(mesh, gv0);
        Vec3 p1 = get_vertex_position(mesh, gv1);
        Vec3 p2 = get_vertex_position(mesh, gv2);

        add_triangle_contribution(triplets, lv0, lv1, lv2, p0, p1, p2);
    }

    // STEP 3: Find 2 vertices to pin (boundary or any 2 far apart)
    int* boundary = NULL;
    int num_boundary = find_boundary_vertices(mesh, face_indices, num_faces, &boundary);

    int pin1_global, pin2_global;
    if (num_boundary >= 2) {
        // Find two boundary vertices far apart
        pin1_global = boundary[0];
        pin2_global = boundary[1];
        double max_dist = 0;
        for (int i = 0; i < num_boundary; i++) {
            for (int j = i + 1; j < num_boundary; j++) {
                Vec3 pi = get_vertex_position(mesh, boundary[i]);
                Vec3 pj = get_vertex_position(mesh, boundary[j]);
                double dist = vec3_length(vec3_sub(pi, pj));
                if (dist > max_dist) {
                    max_dist = dist;
                    pin1_global = boundary[i];
                    pin2_global = boundary[j];
                }
            }
        }
    } else {
        // No boundary - pick any 2 vertices far apart
        pin1_global = local_to_global[0];
        pin2_global = local_to_global[1];
        double max_dist = 0;
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                Vec3 pi = get_vertex_position(mesh, local_to_global[i]);
                Vec3 pj = get_vertex_position(mesh, local_to_global[j]);
                double dist = vec3_length(vec3_sub(pi, pj));
                if (dist > max_dist) {
                    max_dist = dist;
                    pin1_global = local_to_global[i];
                    pin2_global = local_to_global[j];
                }
            }
        }
    }
    if (boundary) free(boundary);

    int pin1 = global_to_local[pin1_global];
    int pin2 = global_to_local[pin2_global];

    // Build matrix with pinned vertices
    // We'll add large diagonal entries for pinned vertices
    double pin_weight = 1e10;
    triplets.push_back(Eigen::Triplet<double>(2*pin1, 2*pin1, pin_weight));
    triplets.push_back(Eigen::Triplet<double>(2*pin1+1, 2*pin1+1, pin_weight));
    triplets.push_back(Eigen::Triplet<double>(2*pin2, 2*pin2, pin_weight));
    triplets.push_back(Eigen::Triplet<double>(2*pin2+1, 2*pin2+1, pin_weight));

    // STEP 4: Solve
    Eigen::SparseMatrix<double> A(2*n, 2*n);
    A.setFromTriplets(triplets.begin(), triplets.end());

    Eigen::VectorXd b = Eigen::VectorXd::Zero(2*n);
    // Pin1 at (0, 0)
    b(2*pin1) = 0.0 * pin_weight;
    b(2*pin1+1) = 0.0 * pin_weight;
    // Pin2 at (1, 0)
    b(2*pin2) = 1.0 * pin_weight;
    b(2*pin2+1) = 0.0 * pin_weight;

    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    solver.compute(A);

    if (solver.info() != Eigen::Success) {
        fprintf(stderr, "LSCM: Matrix factorization failed\n");
        return NULL;
    }

    Eigen::VectorXd x = solver.solve(b);

    if (solver.info() != Eigen::Success) {
        fprintf(stderr, "LSCM: Solve failed\n");
        return NULL;
    }

    // STEP 5: Extract UVs
    float* uvs = (float*)malloc(n * 2 * sizeof(float));
    for (int i = 0; i < n; i++) {
        uvs[i * 2] = (float)x(2*i);
        uvs[i * 2 + 1] = (float)x(2*i + 1);
    }

    normalize_uvs_to_unit_square(uvs, n);

    return uvs;
}
