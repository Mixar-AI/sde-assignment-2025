#!/usr/bin/env python3
"""
Generate a UV sphere with specific topology for testing.
Target: 42 vertices, 120 edges, 80 faces
"""
import math

def generate_uv_sphere(segments, rings, radius=1.0):
    """
    Generate a UV sphere mesh.

    segments: number of longitudinal divisions (around equator)
    rings: number of latitudinal divisions (pole to pole)

    Vertices = segments * (rings - 1) + 2
    For 42 vertices with segments=8: 8 * (rings-1) + 2 = 42 -> rings = 6
    """
    vertices = []
    faces = []

    # Top pole
    vertices.append((0.0, radius, 0.0))

    # Generate ring vertices
    for ring in range(1, rings):
        theta = math.pi * ring / rings  # Latitude angle from top
        ring_radius = radius * math.sin(theta)
        y = radius * math.cos(theta)

        for seg in range(segments):
            phi = 2.0 * math.pi * seg / segments  # Longitude angle
            x = ring_radius * math.cos(phi)
            z = ring_radius * math.sin(phi)
            vertices.append((x, y, z))

    # Bottom pole
    vertices.append((0.0, -radius, 0.0))

    # Generate faces
    # Top cap (connecting to top pole)
    for seg in range(segments):
        v0 = 0  # Top pole
        v1 = 1 + seg
        v2 = 1 + (seg + 1) % segments
        faces.append((v0, v1, v2))

    # Middle rings
    for ring in range(rings - 2):
        ring_start = 1 + ring * segments
        next_ring_start = 1 + (ring + 1) * segments

        for seg in range(segments):
            # Two triangles per quad
            v0 = ring_start + seg
            v1 = next_ring_start + seg
            v2 = next_ring_start + (seg + 1) % segments
            v3 = ring_start + (seg + 1) % segments

            # Triangle 1
            faces.append((v0, v1, v2))
            # Triangle 2
            faces.append((v0, v2, v3))

    # Bottom cap (connecting to bottom pole)
    bottom_pole = len(vertices) - 1
    last_ring_start = 1 + (rings - 2) * segments

    for seg in range(segments):
        v0 = last_ring_start + seg
        v1 = bottom_pole
        v2 = last_ring_start + (seg + 1) % segments
        faces.append((v0, v1, v2))

    return vertices, faces


def write_obj(filename, vertices, faces):
    """Write mesh to OBJ file."""
    with open(filename, 'w') as f:
        f.write("# UV Sphere for testing\n")
        f.write(f"# {len(vertices)} vertices, {len(faces)} faces\n\n")

        # Write vertices
        for v in vertices:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")

        f.write("\n")

        # Write faces (OBJ indices start at 1)
        for face in faces:
            f.write(f"f {face[0]+1} {face[1]+1} {face[2]+1}\n")


def count_edges(faces, num_vertices):
    """Count unique edges in the mesh."""
    edges = set()
    for face in faces:
        # Add three edges from triangle
        edges.add(tuple(sorted([face[0], face[1]])))
        edges.add(tuple(sorted([face[1], face[2]])))
        edges.add(tuple(sorted([face[2], face[0]])))
    return len(edges)


if __name__ == "__main__":
    # Generate UV sphere with 8 segments, 6 rings
    # Expected: 42 vertices, 80 faces
    segments = 8
    rings = 6

    vertices, faces = generate_uv_sphere(segments, rings)

    num_vertices = len(vertices)
    num_faces = len(faces)
    num_edges = count_edges(faces, num_vertices)
    euler = num_vertices - num_edges + num_faces

    print(f"Generated UV Sphere:")
    print(f"  Segments: {segments}")
    print(f"  Rings: {rings}")
    print(f"  Vertices: {num_vertices}")
    print(f"  Edges: {num_edges}")
    print(f"  Faces: {num_faces}")
    print(f"  Euler characteristic: {euler}")
    print()

    # Verify against test expectations
    expected_v, expected_e, expected_f = 42, 120, 80
    if num_vertices == expected_v and num_edges == expected_e and num_faces == expected_f:
        print("✓ Topology matches test expectations!")
        output_file = "starter_code/test_data/meshes/04_sphere.obj"
        write_obj(output_file, vertices, faces)
        print(f"✓ Written to {output_file}")
    else:
        print("✗ Topology mismatch:")
        print(f"  Expected: V={expected_v}, E={expected_e}, F={expected_f}")
        print(f"  Got:      V={num_vertices}, E={num_edges}, F={num_faces}")
