/**
 * @file topology_example.cpp
 * @brief Example topology builder implementation
 *
 * REFERENCE IMPLEMENTATION
 * This shows one way to implement build_topology()
 * Your implementation may differ - this is just guidance!
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
        // Always store smaller vertex first for uniqueness
        if (a < b) {
            v0 = a;
            v1 = b;
        } else {
            v0 = b;
            v1 = a;
        }
    }

    // Needed for std::map
    bool operator<(const Edge& other) const {
        if (v0 != other.v0) return v0 < other.v0;
        return v1 < other.v1;
    }
};

/**
 * @brief Edge information - tracks adjacent faces
 */
struct EdgeInfo {
    int face0;
    int face1;

    EdgeInfo() : face0(-1), face1(-1) {}
};

/**
 * EXAMPLE IMPLEMENTATION
 */
TopologyInfo* build_topology(const Mesh* mesh) {
    if (!mesh) return NULL;

    // STEP 1: Create map to collect unique edges
    std::map<Edge, EdgeInfo> edge_map;

    // STEP 2: Iterate through all triangles and extract edges
    for (int f = 0; f < mesh->num_triangles; f++) {
        int v0 = mesh->triangles[f * 3];
        int v1 = mesh->triangles[f * 3 + 1];
        int v2 = mesh->triangles[f * 3 + 2];

        // Each triangle has 3 edges
        Edge e01(v0, v1);
        Edge e12(v1, v2);
        Edge e20(v2, v0);

        // Track which faces use each edge
        // First face goes to face0, second to face1
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

    // STEP 3: Convert map to arrays for C API
    TopologyInfo* topo = (TopologyInfo*)malloc(sizeof(TopologyInfo));

    topo->num_edges = edge_map.size();
    topo->edges = (int*)malloc(topo->num_edges * 2 * sizeof(int));
    topo->edge_faces = (int*)malloc(topo->num_edges * 2 * sizeof(int));

    int idx = 0;
    for (auto& pair : edge_map) {
        const Edge& edge = pair.first;
        const EdgeInfo& info = pair.second;

        // Store edge vertices
        topo->edges[idx * 2] = edge.v0;
        topo->edges[idx * 2 + 1] = edge.v1;

        // Store adjacent faces
        topo->edge_faces[idx * 2] = info.face0;
        topo->edge_faces[idx * 2 + 1] = info.face1;

        idx++;
    }

    return topo;
}
