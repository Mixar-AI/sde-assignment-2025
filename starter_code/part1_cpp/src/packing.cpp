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

    printf("Packing %d islands...\n", result->num_islands);

    // TODO: Implement island packing
    //
    // ALGORITHM:
    //
    // STEP 1: Compute bounding box for each island
    //   - For each island, find min/max U and V
    //   - Compute width and height
    //   - Track which vertices belong to each island
    //
    // STEP 2: Sort islands by height (descending)
    //   - Larger islands first
    //   - Use std::sort with custom comparator
    //
    // STEP 3: Shelf packing
    //   - Create shelves (horizontal rows)
    //   - Try to fit island in current shelf
    //   - If doesn't fit, create new shelf below
    //   - Track target_x, target_y for each island
    //
    // STEP 4: Move islands to packed positions
    //   - For each island:
    //       offset_x = target_x - current_min_u
    //       offset_y = target_y - current_min_v
    //   - Apply offset to all UVs in island
    //
    // STEP 5: Scale everything to fit [0,1]²
    //   - Find max_width, max_height of packed result
    //   - Scale all UVs by 1.0 / max(max_width, max_height)
    //
    // EXPECTED COVERAGE:
    //   - Shelf packing: > 60%
    //
    // BONUS (+10 points):
    //   - Implement MaxRects or Skyline packing for > 75% coverage

    std::vector<Island> islands(result->num_islands);

    // STEP 1: Compute bounding boxes
    // YOUR CODE HERE

    // STEP 2: Sort by height
    // YOUR CODE HERE

    // STEP 3: Shelf packing
    // YOUR CODE HERE

    // STEP 4: Move islands
    // YOUR CODE HERE

    // STEP 5: Scale to [0,1]
    // YOUR CODE HERE

    printf("  Packing completed\n");
}

void compute_quality_metrics(const Mesh* mesh, UnwrapResult* result) {
    if (!mesh || !result || !mesh->uvs) return;

    // TODO: Implement quality metrics
    //
    // Metrics to compute:
    // - avg_stretch: Average stretch across all triangles
    // - max_stretch: Maximum stretch
    // - coverage: Percentage of [0,1]² covered by UVs
    //
    // For now, placeholder values:

    result->avg_stretch = 1.0f;
    result->max_stretch = 1.0f;
    result->coverage = 0.7f;

    printf("Quality metrics:\n");
    printf("  Avg stretch: %.2f\n", result->avg_stretch);
    printf("  Max stretch: %.2f\n", result->max_stretch);
    printf("  Coverage: %.1f%%\n", result->coverage * 100);
}
