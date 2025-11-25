/**
 * @file unwrap.cpp
 * @brief Main UV unwrapping orchestrator
 *
 * SKELETON - YOU IMPLEMENT THIS
 *
 * This file ties together all the components:
 * - Topology building
 * - Seam detection
 * - Island extraction
 * - LSCM parameterization
 * - Island packing
 */

#include "unwrap.h"
#include "lscm.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <set>
#include <map>

/**
 * @brief Extract UV islands after seam cuts
 *
 * Uses connected components algorithm on face graph after removing seam edges.
 *
 * @param mesh Input mesh
 * @param topo Topology info
 * @param seam_edges Array of seam edge indices
 * @param num_seams Number of seams
 * @param num_islands_out Output: number of islands
 * @return Array of island IDs per face
 */
static int* extract_islands(const Mesh* mesh,
                           const TopologyInfo* topo,
                           const int* seam_edges,
                           int num_seams,
                           int* num_islands_out) {
    int num_faces = mesh->num_triangles;

    int* face_island_ids = (int*)malloc(num_faces * sizeof(int));

    // Initialize all to -1 (unvisited)
    for (int i = 0; i < num_faces; i++) {
        face_island_ids[i] = -1;
    }

    // Step 1: Create set of seam edge indices for fast lookup
    std::set<int> seam_set;
    for (int i = 0; i < num_seams; i++) {
        seam_set.insert(seam_edges[i]);
    }

    // Step 2: Build face adjacency list (only through non-seam edges)
    std::vector<std::vector<int>> face_adjacency(num_faces);

    for (int e = 0; e < topo->num_edges; e++) {
        // Skip seam edges - they don't connect faces within an island
        if (seam_set.find(e) != seam_set.end()) continue;

        int f0 = topo->edge_faces[e * 2 + 0];
        int f1 = topo->edge_faces[e * 2 + 1];

        // Only interior edges connect two faces
        if (f0 >= 0 && f1 >= 0) {
            face_adjacency[f0].push_back(f1);
            face_adjacency[f1].push_back(f0);
        }
    }

    // Step 3: BFS to find connected components
    int island_id = 0;
    std::vector<int> queue;

    for (int start_face = 0; start_face < num_faces; start_face++) {
        if (face_island_ids[start_face] >= 0) continue; // Already visited

        // BFS from this face
        queue.clear();
        queue.push_back(start_face);
        face_island_ids[start_face] = island_id;

        size_t front = 0;
        while (front < queue.size()) {
            int face = queue[front++];

            for (int neighbor : face_adjacency[face]) {
                if (face_island_ids[neighbor] < 0) {
                    face_island_ids[neighbor] = island_id;
                    queue.push_back(neighbor);
                }
            }
        }

        island_id++;
    }

    *num_islands_out = island_id;

    printf("Extracted %d UV islands\n", *num_islands_out);

    return face_island_ids;
}

/**
 * @brief Copy UVs from island parameterization to result mesh
 */
static void copy_island_uvs(Mesh* result,
                           const float* island_uvs,
                           const int* face_indices,
                           int num_faces,
                           const std::map<int, int>& global_to_local) {
    // For each face in the island
    for (int f = 0; f < num_faces; f++) {
        int tri = face_indices[f];

        // For each vertex in the face
        for (int i = 0; i < 3; i++) {
            int global_idx = result->triangles[tri * 3 + i];

            auto it = global_to_local.find(global_idx);
            if (it != global_to_local.end()) {
                int local_idx = it->second;
                result->uvs[global_idx * 2] = island_uvs[local_idx * 2];
                result->uvs[global_idx * 2 + 1] = island_uvs[local_idx * 2 + 1];
            }
        }
    }
}

Mesh* unwrap_mesh(const Mesh* mesh,
                  const UnwrapParams* params,
                  UnwrapResult** result_out) {
    if (!mesh || !params || !result_out) {
        fprintf(stderr, "unwrap_mesh: Invalid arguments\n");
        return NULL;
    }

    printf("\n=== UV Unwrapping ===\n");
    printf("Input: %d vertices, %d triangles\n",
           mesh->num_vertices, mesh->num_triangles);
    printf("Parameters:\n");
    printf("  Angle threshold: %.1f°\n", params->angle_threshold);
    printf("  Min island faces: %d\n", params->min_island_faces);
    printf("  Pack islands: %s\n", params->pack_islands ? "yes" : "no");
    printf("  Island margin: %.3f\n", params->island_margin);
    printf("\n");

    // TODO: Implement main unwrapping pipeline
    //
    // STEP 1: Build topology
    TopologyInfo* topo = build_topology(mesh);
    if (!topo) {
        fprintf(stderr, "Failed to build topology\n");
        return NULL;
    }
    validate_topology(mesh, topo);

    // STEP 2: Detect seams
    int num_seams;
    int* seam_edges = detect_seams(mesh, topo, params->angle_threshold, &num_seams);

    // STEP 3: Extract islands
    int num_islands;
    int* face_island_ids = extract_islands(mesh, topo, seam_edges, num_seams, &num_islands);

    // STEP 4: Parameterize each island using LSCM
    Mesh* result = allocate_mesh_copy(mesh);
    result->uvs = (float*)calloc(mesh->num_vertices * 2, sizeof(float));

    for (int island_id = 0; island_id < num_islands; island_id++) {
        printf("\nProcessing island %d/%d...\n", island_id + 1, num_islands);

        // Get faces in this island
        std::vector<int> island_faces;
        for (int f = 0; f < mesh->num_triangles; f++) {
            if (face_island_ids[f] == island_id) {
                island_faces.push_back(f);
            }
        }

        printf("  %d faces in island\n", (int)island_faces.size());

        if (island_faces.size() < params->min_island_faces) {
            printf("  Skipping (too small)\n");
            continue;
        }

        // Build global_to_local mapping for this island
        std::map<int, int> global_to_local;
        for (int f : island_faces) {
            for (int i = 0; i < 3; i++) {
                int gv = mesh->triangles[f * 3 + i];
                if (global_to_local.find(gv) == global_to_local.end()) {
                    int lv = (int)global_to_local.size();
                    global_to_local[gv] = lv;
                }
            }
        }

        printf("  %d vertices in island\n", (int)global_to_local.size());

        // Call LSCM parameterization
        float* island_uvs = lscm_parameterize(mesh, island_faces.data(), (int)island_faces.size());

        if (island_uvs) {
            // Copy UVs to result mesh
            copy_island_uvs(result, island_uvs, island_faces.data(), (int)island_faces.size(), global_to_local);
            free(island_uvs);
            printf("  LSCM succeeded\n");
        } else {
            printf("  LSCM failed, using planar projection\n");
            // Fallback: simple planar projection (just use first two coordinates)
            for (const auto& pair : global_to_local) {
                int gv = pair.first;
                result->uvs[gv * 2] = mesh->vertices[gv * 3 + 0];
                result->uvs[gv * 2 + 1] = mesh->vertices[gv * 3 + 1];
            }
        }
    }

    // STEP 5: Pack islands if requested
    if (params->pack_islands) {
        UnwrapResult temp_result;
        temp_result.num_islands = num_islands;
        temp_result.face_island_ids = face_island_ids;

        pack_uv_islands(result, &temp_result, params->island_margin);
    }

    // STEP 6: Compute quality metrics
    UnwrapResult* result_data = (UnwrapResult*)malloc(sizeof(UnwrapResult));
    result_data->num_islands = num_islands;
    result_data->face_island_ids = face_island_ids;
    compute_quality_metrics(result, result_data);

    *result_out = result_data;

    // Cleanup
    free_topology(topo);
    free(seam_edges);

    printf("\n=== Unwrapping Complete ===\n");

    return result;
}

void free_unwrap_result(UnwrapResult* result) {
    if (!result) return;

    if (result->face_island_ids) {
        free(result->face_island_ids);
    }
    free(result);
}
