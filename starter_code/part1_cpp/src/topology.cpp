/**
 * @file topology.cpp
 * @brief Topology builder implementation
 *
 * SKELETON - YOU IMPLEMENT THIS
 *
 * Algorithm:
 * 1. Extract all edges from triangles
 * 2. Ensure uniqueness (always store as v0 < v1)
 * 3. For each edge, find adjacent faces
 * 4. Validate using Euler characteristic
 */

#include "topology.h"
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <vector>

/**
 * @brief Edge structure for uniqueness
 */
struct Edge {
    int v0, v1;

    Edge(int a, int b) {
        // Always store smaller vertex first
        if (a < b) {
            v0 = a;
            v1 = b;
        } else {
            v0 = b;
            v1 = a;
        }
    }

    bool operator<(const Edge& other) const {
        if (v0 != other.v0) return v0 < other.v0;
        return v1 < other.v1;
    }
};

/**
 * @brief Edge information
 */
struct EdgeInfo {
    int face0;
    int face1;

    EdgeInfo() : face0(-1), face1(-1) {}
};

TopologyInfo* build_topology(const Mesh* mesh) {
    if (!mesh) return NULL;

    // Step 1: Create map to collect unique edges
    std::map<Edge, EdgeInfo> edge_map;

    // Step 2: Iterate through all triangles and extract edges
    for (int tri = 0; tri < mesh->num_triangles; tri++) {
        int v0 = mesh->triangles[tri * 3 + 0];
        int v1 = mesh->triangles[tri * 3 + 1];
        int v2 = mesh->triangles[tri * 3 + 2];

        // Three edges per triangle
        Edge edges[3] = {
            Edge(v0, v1),
            Edge(v1, v2),
            Edge(v2, v0)
        };

        // Add each edge, tracking adjacent faces
        for (int i = 0; i < 3; i++) {
            EdgeInfo& info = edge_map[edges[i]];
            if (info.face0 == -1) {
                info.face0 = tri;
            } else if (info.face1 == -1) {
                info.face1 = tri;
            }
            // If both faces already set, mesh is non-manifold (ignore for now)
        }
    }

    // Step 3: Convert map to arrays
    int num_edges = (int)edge_map.size();

    TopologyInfo* topo = (TopologyInfo*)malloc(sizeof(TopologyInfo));
    topo->num_edges = num_edges;
    topo->edges = (int*)malloc(2 * num_edges * sizeof(int));
    topo->edge_faces = (int*)malloc(2 * num_edges * sizeof(int));

    // Step 4: Fill arrays from map
    int idx = 0;
    for (const auto& pair : edge_map) {
        const Edge& e = pair.first;
        const EdgeInfo& info = pair.second;

        topo->edges[idx * 2 + 0] = e.v0;
        topo->edges[idx * 2 + 1] = e.v1;
        topo->edge_faces[idx * 2 + 0] = info.face0;
        topo->edge_faces[idx * 2 + 1] = info.face1;

        idx++;
    }

    return topo;
}

void free_topology(TopologyInfo* topo) {
    if (!topo) return;

    if (topo->edges) free(topo->edges);
    if (topo->edge_faces) free(topo->edge_faces);
    free(topo);
}

int validate_topology(const Mesh* mesh, const TopologyInfo* topo) {
    if (!mesh || !topo) return 0;

    int V = mesh->num_vertices;
    int E = topo->num_edges;
    int F = mesh->num_triangles;

    int euler = V - E + F;

    printf("Topology validation:\n");
    printf("  V=%d, E=%d, F=%d\n", V, E, F);
    printf("  Euler characteristic: %d (expected 2 for closed mesh)\n", euler);

    // Closed meshes should have Euler = 2
    // Open meshes or meshes with holes may differ
    if (euler != 2) {
        printf("  Warning: Non-standard Euler characteristic\n");
        printf("  (This may be OK for open meshes or meshes with boundaries)\n");
    }

    return 1;
}
