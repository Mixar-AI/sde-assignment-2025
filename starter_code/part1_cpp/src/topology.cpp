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
 *
 * See reference/topology_example.cpp for guidance
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

    // TODO: Implement topology building
    //
    // Steps:
    // 1. Create std::map<Edge, EdgeInfo> to collect edges
    // 2. Iterate through all triangles
    //    For each triangle, extract 3 edges
    //    Add to map, tracking which faces use each edge
    // 3. Convert map to arrays (edges, edge_faces)
    // 4. Allocate TopologyInfo and fill arrays
    //
    // Hints:
    // - Use Edge struct for automatic ordering
    // - Each edge should have 1 or 2 adjacent faces
    // - Boundary edges have only 1 face (set face1 = -1)
    //
    // See reference/topology_example.cpp for complete example

    TopologyInfo* topo = (TopologyInfo*)malloc(sizeof(TopologyInfo));

    // STEP 1: Create map to collect edges
    std::map<Edge, EdgeInfo> edge_map;

    // STEP 2: Iterate through all triangles and extract edges
    for (int f = 0; f < mesh->num_triangles; f++) {
        int v0 = mesh->triangles[f * 3];
        int v1 = mesh->triangles[f * 3 + 1];
        int v2 = mesh->triangles[f * 3 + 2];

        // Add three edges of this triangle
        Edge e01(v0, v1);
        Edge e12(v1, v2);
        Edge e20(v2, v0);

        // Track which faces use each edge
        if (edge_map[e01].face0 == -1) {
            edge_map[e01].face0 = f;
        } else {
            edge_map[e01].face1 = f;
        }

        if (edge_map[e12].face0 == -1) {
            edge_map[e12].face0 = f;
        } else {
            edge_map[e12].face1 = f;
        }

        if (edge_map[e20].face0 == -1) {
            edge_map[e20].face0 = f;
        } else {
            edge_map[e20].face1 = f;
        }
    }

    // STEP 3: Convert map to arrays
    topo->num_edges = edge_map.size();
    topo->edges = (int*)malloc(topo->num_edges * 2 * sizeof(int));
    topo->edge_faces = (int*)malloc(topo->num_edges * 2 * sizeof(int));

    int idx = 0;
    for (auto& pair : edge_map) {
        const Edge& edge = pair.first;
        const EdgeInfo& info = pair.second;

        topo->edges[idx * 2] = edge.v0;
        topo->edges[idx * 2 + 1] = edge.v1;

        topo->edge_faces[idx * 2] = info.face0;
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
