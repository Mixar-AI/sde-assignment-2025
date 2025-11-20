"""
Reference Implementation: UV Quality Metrics

This file shows example implementations of the metrics described
in metrics_spec.md. Use this as a guide for your implementation.

NOTE: Your implementation should go in uvwrap/metrics.py,
not here. This is just a reference!
"""

import numpy as np
from typing import Tuple


def compute_stretch_metric_reference(
    vertices_3d: np.ndarray,
    vertices_uv: np.ndarray,
    triangles: np.ndarray
) -> Tuple[float, float]:
    """
    Reference implementation of stretch metric computation.

    Stretch measures how much the UV mapping distorts edge lengths
    compared to the original 3D mesh.

    Args:
        vertices_3d: (N, 3) array of 3D vertex positions
        vertices_uv: (N, 2) array of UV coordinates
        triangles: (T, 3) array of triangle indices

    Returns:
        (avg_stretch, max_stretch): Average and maximum stretch values
    """
    stretches = []

    for tri in triangles:
        # Get triangle vertices
        v0_3d, v1_3d, v2_3d = vertices_3d[tri]
        v0_uv, v1_uv, v2_uv = vertices_uv[tri]

        # Compute 3D edge lengths
        e01_3d = np.linalg.norm(v1_3d - v0_3d)
        e12_3d = np.linalg.norm(v2_3d - v1_3d)
        e20_3d = np.linalg.norm(v0_3d - v2_3d)

        # Compute UV edge lengths
        e01_uv = np.linalg.norm(v1_uv - v0_uv)
        e12_uv = np.linalg.norm(v2_uv - v1_uv)
        e20_uv = np.linalg.norm(v0_uv - v2_uv)

        # Compute stretch for each edge (avoid division by zero)
        if e01_3d > 1e-10 and e01_uv > 1e-10:
            stretches.append(e01_uv / e01_3d)
        if e12_3d > 1e-10 and e12_uv > 1e-10:
            stretches.append(e12_uv / e12_3d)
        if e20_3d > 1e-10 and e20_uv > 1e-10:
            stretches.append(e20_uv / e20_3d)

    if not stretches:
        return 1.0, 1.0

    # Normalize by median to make it scale-invariant
    median_stretch = np.median(stretches)
    if median_stretch > 1e-10:
        stretches = [s / median_stretch for s in stretches]

    avg_stretch = np.mean(stretches)
    max_stretch = np.max(stretches)

    return avg_stretch, max_stretch


def compute_area_distortion_reference(
    vertices_3d: np.ndarray,
    vertices_uv: np.ndarray,
    triangles: np.ndarray
) -> Tuple[float, float]:
    """
    Reference implementation of area distortion computation.

    Measures how much triangle areas change between 3D and UV space.

    Args:
        vertices_3d: (N, 3) array of 3D vertex positions
        vertices_uv: (N, 2) array of UV coordinates
        triangles: (T, 3) array of triangle indices

    Returns:
        (avg_distortion, max_distortion): Average and max area distortion
    """
    distortions = []

    for tri in triangles:
        # Get triangle vertices
        v0_3d, v1_3d, v2_3d = vertices_3d[tri]
        v0_uv, v1_uv, v2_uv = vertices_uv[tri]

        # Compute 3D triangle area using cross product
        edge1_3d = v1_3d - v0_3d
        edge2_3d = v2_3d - v0_3d
        area_3d = 0.5 * np.linalg.norm(np.cross(edge1_3d, edge2_3d))

        # Compute UV triangle area using 2D cross product
        edge1_uv = v1_uv - v0_uv
        edge2_uv = v2_uv - v0_uv
        area_uv = 0.5 * abs(edge1_uv[0] * edge2_uv[1] - edge1_uv[1] * edge2_uv[0])

        # Compute distortion ratio (avoid division by zero)
        if area_3d > 1e-10 and area_uv > 1e-10:
            distortion = area_uv / area_3d
            distortions.append(distortion)

    if not distortions:
        return 1.0, 1.0

    # Normalize by median
    median_dist = np.median(distortions)
    if median_dist > 1e-10:
        distortions = [d / median_dist for d in distortions]

    avg_distortion = np.mean(distortions)
    max_distortion = np.max(distortions)

    return avg_distortion, max_distortion


def compute_angle_distortion_reference(
    vertices_3d: np.ndarray,
    vertices_uv: np.ndarray,
    triangles: np.ndarray
) -> float:
    """
    Reference implementation of angle distortion computation.

    Measures how well angles are preserved (conformal quality).

    Args:
        vertices_3d: (N, 3) array of 3D vertex positions
        vertices_uv: (N, 2) array of UV coordinates
        triangles: (T, 3) array of triangle indices

    Returns:
        avg_angle_error: Average absolute angle error in degrees
    """
    angle_errors = []

    for tri in triangles:
        # Get triangle vertices
        v0_3d, v1_3d, v2_3d = vertices_3d[tri]
        v0_uv, v1_uv, v2_uv = vertices_uv[tri]

        # Compute 3D angles at each vertex
        for i in range(3):
            # Get edges from vertex i
            if i == 0:
                edge1_3d = v1_3d - v0_3d
                edge2_3d = v2_3d - v0_3d
                edge1_uv = v1_uv - v0_uv
                edge2_uv = v2_uv - v0_uv
            elif i == 1:
                edge1_3d = v0_3d - v1_3d
                edge2_3d = v2_3d - v1_3d
                edge1_uv = v0_uv - v1_uv
                edge2_uv = v2_uv - v1_uv
            else:
                edge1_3d = v0_3d - v2_3d
                edge2_3d = v1_3d - v2_3d
                edge1_uv = v0_uv - v2_uv
                edge2_uv = v1_uv - v2_uv

            # Compute angle in 3D
            len1_3d = np.linalg.norm(edge1_3d)
            len2_3d = np.linalg.norm(edge2_3d)
            if len1_3d > 1e-10 and len2_3d > 1e-10:
                cos_angle_3d = np.clip(
                    np.dot(edge1_3d, edge2_3d) / (len1_3d * len2_3d),
                    -1.0, 1.0
                )
                angle_3d = np.arccos(cos_angle_3d)
            else:
                continue

            # Compute angle in UV
            len1_uv = np.linalg.norm(edge1_uv)
            len2_uv = np.linalg.norm(edge2_uv)
            if len1_uv > 1e-10 and len2_uv > 1e-10:
                cos_angle_uv = np.clip(
                    np.dot(edge1_uv, edge2_uv) / (len1_uv * len2_uv),
                    -1.0, 1.0
                )
                angle_uv = np.arccos(cos_angle_uv)
            else:
                continue

            # Compute absolute difference in degrees
            angle_error = abs(angle_3d - angle_uv) * 180.0 / np.pi
            angle_errors.append(angle_error)

    if not angle_errors:
        return 0.0

    return np.mean(angle_errors)


def compute_coverage_reference(
    vertices_uv: np.ndarray,
    triangles: np.ndarray
) -> float:
    """
    Reference implementation of UV coverage computation.

    Measures how efficiently the [0,1]² UV space is used.

    Args:
        vertices_uv: (N, 2) array of UV coordinates
        triangles: (T, 3) array of triangle indices

    Returns:
        coverage: Fraction of unit square covered (0 to 1)
    """
    # Compute total UV area
    total_uv_area = 0.0

    for tri in triangles:
        v0_uv, v1_uv, v2_uv = vertices_uv[tri]

        # 2D cross product for triangle area
        edge1 = v1_uv - v0_uv
        edge2 = v2_uv - v0_uv
        area = 0.5 * abs(edge1[0] * edge2[1] - edge1[1] * edge2[0])
        total_uv_area += area

    # Unit square has area 1.0
    # Coverage is total area / 1.0
    # Clip to [0, 1] in case UVs exceed [0,1]
    coverage = min(total_uv_area / 1.0, 1.0)

    return coverage


def analyze_uv_quality_reference(
    vertices_3d: np.ndarray,
    vertices_uv: np.ndarray,
    triangles: np.ndarray
) -> dict:
    """
    Complete UV quality analysis reference.

    Args:
        vertices_3d: (N, 3) array of 3D vertex positions
        vertices_uv: (N, 2) array of UV coordinates
        triangles: (T, 3) array of triangle indices

    Returns:
        Dictionary with all quality metrics
    """
    avg_stretch, max_stretch = compute_stretch_metric_reference(
        vertices_3d, vertices_uv, triangles
    )

    avg_area_dist, max_area_dist = compute_area_distortion_reference(
        vertices_3d, vertices_uv, triangles
    )

    avg_angle_error = compute_angle_distortion_reference(
        vertices_3d, vertices_uv, triangles
    )

    coverage = compute_coverage_reference(vertices_uv, triangles)

    return {
        'stretch': {
            'average': avg_stretch,
            'maximum': max_stretch
        },
        'area_distortion': {
            'average': avg_area_dist,
            'maximum': max_area_dist
        },
        'angle_distortion': avg_angle_error,
        'coverage': coverage
    }


# Example usage
if __name__ == "__main__":
    # Create a simple test case: a square in 3D mapped to UV
    vertices_3d = np.array([
        [0.0, 0.0, 0.0],
        [1.0, 0.0, 0.0],
        [1.0, 1.0, 0.0],
        [0.0, 1.0, 0.0]
    ])

    # Perfect mapping (no distortion)
    vertices_uv = np.array([
        [0.0, 0.0],
        [1.0, 0.0],
        [1.0, 1.0],
        [0.0, 1.0]
    ])

    # Two triangles forming a square
    triangles = np.array([
        [0, 1, 2],
        [0, 2, 3]
    ])

    # Analyze quality
    metrics = analyze_uv_quality_reference(vertices_3d, vertices_uv, triangles)

    print("UV Quality Metrics (perfect mapping):")
    print(f"  Stretch: avg={metrics['stretch']['average']:.3f}, "
          f"max={metrics['stretch']['maximum']:.3f}")
    print(f"  Area distortion: avg={metrics['area_distortion']['average']:.3f}, "
          f"max={metrics['area_distortion']['maximum']:.3f}")
    print(f"  Angle distortion: {metrics['angle_distortion']:.3f}°")
    print(f"  Coverage: {metrics['coverage']:.1%}")

    # Now test with a distorted mapping
    vertices_uv_distorted = np.array([
        [0.0, 0.0],
        [0.5, 0.0],  # Compressed in X
        [0.5, 1.0],
        [0.0, 1.0]
    ])

    metrics_distorted = analyze_uv_quality_reference(
        vertices_3d, vertices_uv_distorted, triangles
    )

    print("\nUV Quality Metrics (distorted mapping):")
    print(f"  Stretch: avg={metrics_distorted['stretch']['average']:.3f}, "
          f"max={metrics_distorted['stretch']['maximum']:.3f}")
    print(f"  Area distortion: avg={metrics_distorted['area_distortion']['average']:.3f}, "
          f"max={metrics_distorted['area_distortion']['maximum']:.3f}")
    print(f"  Angle distortion: {metrics_distorted['angle_distortion']:.3f}°")
    print(f"  Coverage: {metrics_distorted['coverage']:.1%}")
