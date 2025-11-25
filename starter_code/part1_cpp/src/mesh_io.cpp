/**
 * @file mesh_io.cpp
 * @brief OBJ file I/O for meshes
 *
 * PROVIDED - Complete implementation
 * Handles loading and saving OBJ files with UVs
 */

#include "mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

Mesh* load_obj(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        return NULL;
    }

    std::vector<float> vertices;
    std::vector<int> triangles;
    std::vector<float> uvs_temp;
    bool has_uvs = false;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'v' && line[1] == ' ') {
            // Vertex
            float x, y, z;
            if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
            }
        } else if (line[0] == 'v' && line[1] == 't') {
            // UV coordinate
            float u, v;
            if (sscanf(line, "vt %f %f", &u, &v) == 2) {
                uvs_temp.push_back(u);
                uvs_temp.push_back(v);
                has_uvs = true;
            }
        } else if (line[0] == 'f' && line[1] == ' ') {
            // Face
            int v0, v1, v2;
            int vt0, vt1, vt2;
            int num_vertices = vertices.size() / 3;

            if (has_uvs && sscanf(line, "f %d/%d %d/%d %d/%d",
                                 &v0, &vt0, &v1, &vt1, &v2, &vt2) == 6) {
                // Validate vertex indices (OBJ is 1-indexed)
                if (v0 < 1 || v0 > num_vertices ||
                    v1 < 1 || v1 > num_vertices ||
                    v2 < 1 || v2 > num_vertices) {
                    fprintf(stderr, "Error: Invalid vertex indices in face: %d %d %d (valid range: 1-%d)\n",
                            v0, v1, v2, num_vertices);
                    continue;  // Skip this face
                }
                triangles.push_back(v0 - 1);
                triangles.push_back(v1 - 1);
                triangles.push_back(v2 - 1);
            } else if (sscanf(line, "f %d %d %d", &v0, &v1, &v2) == 3) {
                // Validate vertex indices (OBJ is 1-indexed)
                if (v0 < 1 || v0 > num_vertices ||
                    v1 < 1 || v1 > num_vertices ||
                    v2 < 1 || v2 > num_vertices) {
                    fprintf(stderr, "Error: Invalid vertex indices in face: %d %d %d (valid range: 1-%d)\n",
                            v0, v1, v2, num_vertices);
                    continue;  // Skip this face
                }
                triangles.push_back(v0 - 1);
                triangles.push_back(v1 - 1);
                triangles.push_back(v2 - 1);
            }
        }
    }

    fclose(f);

    if (vertices.empty() || triangles.empty()) {
        fprintf(stderr, "Failed to parse OBJ file: %s\n", filename);
        return NULL;
    }

    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));

    mesh->num_vertices = vertices.size() / 3;
    mesh->vertices = (float*)malloc(vertices.size() * sizeof(float));
    memcpy(mesh->vertices, vertices.data(), vertices.size() * sizeof(float));

    mesh->num_triangles = triangles.size() / 3;
    mesh->triangles = (int*)malloc(triangles.size() * sizeof(int));
    memcpy(mesh->triangles, triangles.data(), triangles.size() * sizeof(int));

    if (has_uvs) {
        size_t expected_uv_count = (vertices.size() / 3) * 2;
        if (uvs_temp.size() == expected_uv_count) {
            mesh->uvs = (float*)malloc(uvs_temp.size() * sizeof(float));
            memcpy(mesh->uvs, uvs_temp.data(), uvs_temp.size() * sizeof(float));
        } else {
            fprintf(stderr,
                    "Warning: UV count mismatch in %s\n"
                    "  Expected: %zu UVs (%zu vertices)\n"
                    "  Found:    %zu UVs\n"
                    "  UVs will be ignored.\n",
                    filename, expected_uv_count / 2, vertices.size() / 3,
                    uvs_temp.size() / 2);
            mesh->uvs = NULL;
        }
    } else {
        mesh->uvs = NULL;
    }

    printf("Loaded %s: %d vertices, %d triangles\n",
           filename, mesh->num_vertices, mesh->num_triangles);

    return mesh;
}

int save_obj(const Mesh* mesh, const char* filename) {
    if (!mesh) return -1;

    FILE* f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Cannot write file: %s\n", filename);
        return -1;
    }

    // Write vertices
    for (int i = 0; i < mesh->num_vertices; i++) {
        fprintf(f, "v %f %f %f\n",
                mesh->vertices[i*3],
                mesh->vertices[i*3+1],
                mesh->vertices[i*3+2]);
    }

    // Write UVs if present
    if (mesh->uvs) {
        for (int i = 0; i < mesh->num_vertices; i++) {
            fprintf(f, "vt %f %f\n",
                    mesh->uvs[i*2],
                    mesh->uvs[i*2+1]);
        }
    }

    // Write faces
    for (int i = 0; i < mesh->num_triangles; i++) {
        int v0 = mesh->triangles[i*3] + 1;
        int v1 = mesh->triangles[i*3+1] + 1;
        int v2 = mesh->triangles[i*3+2] + 1;

        if (mesh->uvs) {
            fprintf(f, "f %d/%d %d/%d %d/%d\n",
                    v0, v0, v1, v1, v2, v2);
        } else {
            fprintf(f, "f %d %d %d\n", v0, v1, v2);
        }
    }

    fclose(f);
    printf("Saved %s\n", filename);
    return 0;
}

void free_mesh(Mesh* mesh) {
    if (!mesh) return;

    if (mesh->vertices) free(mesh->vertices);
    if (mesh->triangles) free(mesh->triangles);
    if (mesh->uvs) free(mesh->uvs);
    free(mesh);
}

Mesh* allocate_mesh_copy(const Mesh* input) {
    if (!input) return NULL;

    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));

    mesh->num_vertices = input->num_vertices;
    mesh->vertices = (float*)malloc(input->num_vertices * 3 * sizeof(float));
    memcpy(mesh->vertices, input->vertices, input->num_vertices * 3 * sizeof(float));

    mesh->num_triangles = input->num_triangles;
    mesh->triangles = (int*)malloc(input->num_triangles * 3 * sizeof(int));
    memcpy(mesh->triangles, input->triangles, input->num_triangles * 3 * sizeof(int));

    mesh->uvs = NULL;

    return mesh;
}
