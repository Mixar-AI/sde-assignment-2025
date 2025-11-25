/**
 * @file packing.cpp
 * @brief UV island packing into [0,1]² texture space
 *
 * SKELETON - YOU IMPLEMENT THIS
 *
 * Algorithm: Shelf packing
 * 1. Compute bounding box for each island
 * 2. Sort islands by height (descending)
 * 3. Pack using shelf algorithm
 * 4. Scale to fit [0,1]²
 */

#include "unwrap.h"
#include "math_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <vector>
#include <algorithm>

/**
 * @brief Island bounding box info
 */
struct Island {
    int id;
    float min_u, max_u, min_v, max_v;
    float width, height;
    float target_x, target_y;  // Packed position
    std::vector<int> vertex_indices;
};

void pack_uv_islands(Mesh* mesh,
                     const UnwrapResult* result,
                     float margin) {
    if (!mesh || !result || !mesh->uvs) return;

    if (result->num_islands <= 1) {
        // Single island, already normalized to [0,1]
        return;
    }

    std::vector<Island> islands(result->num_islands);

    // Initialize islands
    for (int i = 0; i < result->num_islands; i++) {
        islands[i].id = i;
        islands[i].min_u = FLT_MAX;
        islands[i].max_u = -FLT_MAX;
        islands[i].min_v = FLT_MAX;
        islands[i].max_v = -FLT_MAX;
    }

    // STEP 1: Compute bounding boxes for each island
    // Build mapping from face to island, then find vertices per island
    std::vector<int> vertex_island(mesh->num_vertices, -1);

    for (int f = 0; f < mesh->num_triangles; f++) {
        int island_id = result->face_island_ids[f];
        if (island_id < 0 || island_id >= result->num_islands) continue;

        for (int i = 0; i < 3; i++) {
            int v = mesh->triangles[f * 3 + i];
            if (vertex_island[v] == -1) {
                vertex_island[v] = island_id;
                islands[island_id].vertex_indices.push_back(v);

                float u = mesh->uvs[v * 2];
                float vi = mesh->uvs[v * 2 + 1];

                islands[island_id].min_u = min_float(islands[island_id].min_u, u);
                islands[island_id].max_u = max_float(islands[island_id].max_u, u);
                islands[island_id].min_v = min_float(islands[island_id].min_v, vi);
                islands[island_id].max_v = max_float(islands[island_id].max_v, vi);
            }
        }
    }

    // Compute widths and heights
    for (int i = 0; i < result->num_islands; i++) {
        islands[i].width = islands[i].max_u - islands[i].min_u + margin;
        islands[i].height = islands[i].max_v - islands[i].min_v + margin;
        if (islands[i].width < margin) islands[i].width = margin;
        if (islands[i].height < margin) islands[i].height = margin;
    }

    // STEP 2: Sort islands by height (descending)
    std::sort(islands.begin(), islands.end(),
        [](const Island& a, const Island& b) {
            return a.height > b.height;
        });

    // STEP 3: Shelf packing
    float shelf_x = 0.0f;
    float shelf_y = 0.0f;
    float shelf_height = islands[0].height;
    float max_width = 0.0f;

    for (size_t i = 0; i < islands.size(); i++) {
        // Check if island fits in current shelf
        if (shelf_x + islands[i].width > 1.0f && shelf_x > 0.0f) {
            // Start new shelf
            shelf_y += shelf_height;
            shelf_height = islands[i].height;
            shelf_x = 0.0f;
        }

        // Place island
        islands[i].target_x = shelf_x;
        islands[i].target_y = shelf_y;

        shelf_x += islands[i].width;
        max_width = max_float(max_width, shelf_x);
    }

    float total_height = shelf_y + shelf_height;

    // STEP 4: Move islands to packed positions
    for (size_t i = 0; i < islands.size(); i++) {
        float offset_x = islands[i].target_x - islands[i].min_u;
        float offset_y = islands[i].target_y - islands[i].min_v;

        for (int v : islands[i].vertex_indices) {
            mesh->uvs[v * 2] += offset_x;
            mesh->uvs[v * 2 + 1] += offset_y;
        }
    }

    // STEP 5: Scale to fit [0,1]²
    float scale = 1.0f / max_float(max_width, total_height);
    if (scale < 1.0f) {
        for (int v = 0; v < mesh->num_vertices; v++) {
            mesh->uvs[v * 2] *= scale;
            mesh->uvs[v * 2 + 1] *= scale;
        }
    }
}

void compute_quality_metrics(const Mesh* mesh, UnwrapResult* result) {
    if (!mesh || !result || !mesh->uvs) return;

    // TODO: Implement quality metrics computation
    //
    // NOTE: This function is OPTIONAL for Part 1.
    // You will implement full metrics in Part 2 (Python).
    // For Part 1, you can either:
    //   (A) Leave these as defaults (tests will still pass)
    //   (B) Implement basic estimation for testing
    //
    // ALGORITHM (see reference/algorithms.md and part2_python/reference/metrics_spec.md):
    //
    // STRETCH METRIC (SVD-based):
    //   For each triangle:
    //     1. Build Jacobian matrix J (3x2): maps UV space to 3D space
    //        J = [dp/du, dp/dv] where p is 3D position
    //     2. Compute J^T * J (2x2 Gramian matrix)
    //     3. Find eigenvalues λ1, λ2 of J^T * J
    //     4. Singular values: σ1 = sqrt(λ1), σ2 = sqrt(λ2)
    //     5. Stretch = σ1 / σ2 (ratio of max/min stretching)
    //   Average and max stretch across all triangles
    //
    // COVERAGE METRIC (Rasterization-based):
    //   1. Create 1024x1024 bitmap of [0,1]² UV space
    //   2. Rasterize all UV triangles
    //   3. Coverage = (pixels_filled / total_pixels)
    //   Alternative: Use bounding box as approximation
    //
    // EXPECTED RESULTS:
    //   - Good unwrap: avg_stretch < 1.5, max_stretch < 2.0
    //   - Shelf packing: coverage > 0.60 (60%)
    //   - MaxRects packing: coverage > 0.75 (75%)

    // Default values (replace with your implementation)
    result->avg_stretch = 1.0f;
    result->max_stretch = 1.0f;
    result->coverage = 0.7f;

    printf("Quality metrics: (using defaults - implement for accurate values)\n");
    printf("  Avg stretch: %.2f\n", result->avg_stretch);
    printf("  Max stretch: %.2f\n", result->max_stretch);
    printf("  Coverage: %.1f%%\n", result->coverage * 100);
}
