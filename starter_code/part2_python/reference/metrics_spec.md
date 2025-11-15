# Quality Metrics Specification

This document provides exact formulas for implementing UV quality metrics.

## Table of Contents
1. [Stretch Metric](#stretch-metric)
2. [Coverage Metric](#coverage-metric)
3. [Angle Distortion](#angle-distortion)

---

## Stretch Metric

### Definition

Stretch measures how much the UV mapping distorts the mesh geometry.

**Formula:**
For each triangle, compute the Jacobian matrix J that maps from UV space to 3D space, then compute the ratio of singular values.

### Algorithm

**For each triangle:**

1. **Get positions:**
   ```
   p0, p1, p2 = 3D vertex positions
   uv0, uv1, uv2 = UV coordinates
   ```

2. **Compute edge vectors:**
   ```
   dp1 = p1 - p0  (3D vector)
   dp2 = p2 - p0  (3D vector)
   duv1 = uv1 - uv0  (2D vector)
   duv2 = uv2 - uv0  (2D vector)
   ```

3. **Check for degeneracy:**
   ```
   det_uv = duv1.x * duv2.y - duv1.y * duv2.x

   if |det_uv| < 1e-10:
       skip (degenerate UV triangle)
   ```

4. **Build Jacobian matrix:**
   ```
   uv_mat = [[duv1.x, duv2.x],
             [duv1.y, duv2.y]]  (2×2)

   xyz_mat = [[dp1.x, dp2.x],
              [dp1.y, dp2.y],
              [dp1.z, dp2.z]]  (3×2)

   J = xyz_mat @ inv(uv_mat)  (3×2 matrix)
   ```

5. **Compute singular values:**
   ```
   U, S, Vt = SVD(J)
   σ1 = S[0]  (larger singular value)
   σ2 = S[1]  (smaller singular value)

   if σ2 < 1e-10:
       skip (degenerate)
   ```

6. **Compute stretch for this triangle:**
   ```
   stretch = max(σ1/σ2, σ2/σ1)
   ```

7. **Return maximum stretch across all triangles**

### Python Implementation

```python
import numpy as np

def compute_stretch(mesh, uvs):
    max_stretch = 1.0

    vertices = np.array(mesh.vertices)
    triangles = np.array(mesh.triangles)
    uv_coords = np.array(uvs)

    for tri in triangles:
        v0, v1, v2 = tri

        # 3D positions
        p0 = vertices[v0]
        p1 = vertices[v1]
        p2 = vertices[v2]

        # UV positions
        uv0 = uv_coords[v0]
        uv1 = uv_coords[v1]
        uv2 = uv_coords[v2]

        # Edge vectors
        dp1 = p1 - p0
        dp2 = p2 - p0
        duv1 = uv1 - uv0
        duv2 = uv2 - uv0

        # Check degeneracy
        det_uv = duv1[0] * duv2[1] - duv1[1] * duv2[0]
        if abs(det_uv) < 1e-10:
            continue

        # Build Jacobian
        uv_mat = np.array([[duv1[0], duv2[0]],
                          [duv1[1], duv2[1]]])
        xyz_mat = np.column_stack([dp1, dp2])

        try:
            J = xyz_mat @ np.linalg.inv(uv_mat)

            # SVD
            U, S, Vt = np.linalg.svd(J, full_matrices=False)
            sigma1, sigma2 = S[0], S[1]

            if sigma2 < 1e-10:
                continue

            stretch = max(sigma1 / sigma2, sigma2 / sigma1)
            max_stretch = max(max_stretch, stretch)

        except np.linalg.LinAlgError:
            continue

    return max_stretch
```

### Interpretation

- **stretch = 1.0:** Perfect (no distortion)
- **stretch < 1.3:** Excellent
- **stretch < 1.5:** Good
- **stretch < 2.0:** Acceptable
- **stretch ≥ 2.0:** Poor (significant distortion)

---

## Coverage Metric

### Definition

Coverage measures what percentage of the [0,1]² texture space is used by UV islands.

### Algorithm

1. **Create rasterization grid:**
   ```
   resolution = 1024
   grid = zeros(resolution, resolution, dtype=bool)
   ```

2. **Rasterize each triangle:**
   ```
   For each triangle (v0, v1, v2):
       uv0, uv1, uv2 = UV coordinates

       # Scale to grid coordinates
       p0 = (uv0 * resolution).astype(int)
       p1 = (uv1 * resolution).astype(int)
       p2 = (uv2 * resolution).astype(int)

       # Rasterize triangle
       rasterize_triangle(grid, p0, p1, p2)
   ```

3. **Count coverage:**
   ```
   pixels_covered = count(grid == True)
   total_pixels = resolution * resolution
   coverage = pixels_covered / total_pixels
   ```

### Triangle Rasterization

**Scanline algorithm:**

```python
def rasterize_triangle(grid, p0, p1, p2):
    # Bounding box
    min_x = max(0, min(p0[0], p1[0], p2[0]))
    max_x = min(grid.shape[1] - 1, max(p0[0], p1[0], p2[0]))
    min_y = max(0, min(p0[1], p1[1], p2[1]))
    max_y = min(grid.shape[0] - 1, max(p0[1], p1[1], p2[1]))

    # Test each pixel
    for y in range(min_y, max_y + 1):
        for x in range(min_x, max_x + 1):
            if point_in_triangle([x, y], p0, p1, p2):
                grid[y, x] = True

def point_in_triangle(p, v0, v1, v2):
    """Barycentric coordinate test"""
    def sign(p1, p2, p3):
        return (p1[0] - p3[0]) * (p2[1] - p3[1]) - \
               (p2[0] - p3[0]) * (p1[1] - p3[1])

    d1 = sign(p, v0, v1)
    d2 = sign(p, v1, v2)
    d3 = sign(p, v2, v0)

    has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
    has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)

    return not (has_neg and has_pos)
```

### Interpretation

- **coverage > 0.75:** Excellent (efficient packing)
- **coverage > 0.60:** Good
- **coverage > 0.50:** Acceptable
- **coverage ≤ 0.50:** Poor (wasting texture space)

---

## Angle Distortion

### Definition

Angle distortion measures how much angles change between 3D mesh and UV mapping.

Conformal mappings (like LSCM) should preserve angles, so this metric should be small for good parameterizations.

### Algorithm

**For each triangle:**

1. **Compute angles in 3D:**
   ```
   For vertex v in triangle:
       e1 = normalize(vertex_before - v)
       e2 = normalize(vertex_after - v)
       angle_3d = arccos(dot(e1, e2))
   ```

2. **Compute angles in UV:**
   ```
   Same as above but using UV coordinates
   ```

3. **Compute distortion:**
   ```
   For each of 3 angles:
       distortion = |angle_3d - angle_uv|
   ```

4. **Return maximum distortion across all triangles and all angles**

### Python Implementation

```python
def compute_angle_distortion(mesh, uvs):
    max_distortion = 0.0

    vertices = np.array(mesh.vertices)
    triangles = np.array(mesh.triangles)
    uv_coords = np.array(uvs)

    for tri in triangles:
        v0, v1, v2 = tri

        # 3D angles
        p0, p1, p2 = vertices[v0], vertices[v1], vertices[v2]
        angles_3d = compute_triangle_angles_3d(p0, p1, p2)

        # UV angles
        uv0, uv1, uv2 = uv_coords[v0], uv_coords[v1], uv_coords[v2]
        angles_uv = compute_triangle_angles_2d(uv0, uv1, uv2)

        # Max distortion for this triangle
        for i in range(3):
            distortion = abs(angles_3d[i] - angles_uv[i])
            max_distortion = max(max_distortion, distortion)

    return max_distortion

def compute_triangle_angles_3d(p0, p1, p2):
    """Compute 3 angles of 3D triangle"""
    e0 = p1 - p0
    e1 = p2 - p1
    e2 = p0 - p2

    # Normalize
    e0 = e0 / np.linalg.norm(e0)
    e1 = e1 / np.linalg.norm(e1)
    e2 = e2 / np.linalg.norm(e2)

    # Angles
    angle0 = np.arccos(np.clip(np.dot(-e2, e0), -1, 1))
    angle1 = np.arccos(np.clip(np.dot(-e0, e1), -1, 1))
    angle2 = np.arccos(np.clip(np.dot(-e1, e2), -1, 1))

    return [angle0, angle1, angle2]

def compute_triangle_angles_2d(uv0, uv1, uv2):
    """Same as 3D but in 2D"""
    # Similar implementation
    ...
```

### Interpretation

- **distortion < 0.1 radians (< 6°):** Excellent
- **distortion < 0.2 radians (< 11°):** Good
- **distortion < 0.5 radians (< 29°):** Acceptable
- **distortion ≥ 0.5 radians:** Poor

---

## Testing

Validate your metrics implementation against reference outputs:

```bash
python -m pytest tests/test_metrics.py
```

Expected results for test meshes:

| Mesh | Max Stretch | Coverage | Angle Distortion |
|------|-------------|----------|------------------|
| Cube | 1.1 - 1.5 | 60-75% | < 0.2 rad |
| Sphere | 1.3 - 1.8 | 50-70% | < 0.3 rad |
| Cylinder | 1.0 - 1.2 | 65-80% | < 0.15 rad |

---

## Tips

1. **Numerical stability:**
   - Check for division by zero
   - Clamp values before arccos
   - Skip degenerate triangles

2. **Performance:**
   - Use numpy array operations (vectorize when possible)
   - Pre-allocate arrays
   - Avoid Python loops over triangles if possible

3. **Validation:**
   - Test with perfect mappings (stretch should be 1.0)
   - Test with known distorted mappings
   - Compare against reference implementation

Good luck!
