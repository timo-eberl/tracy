const std = @import("std");
const testing = std.testing;

// Import C definitions
const c = @cImport({
    @cInclude("../src/tracy.c");
});

const epsilon = 0.00001;

// Helper to construct a triangle easily
// We place vertices in Counter-Clockwise (CCW) order by default
fn createTriangle(x0: f64, y0: f64, z0: f64, x1: f64, y1: f64, z1: f64, x2: f64, y2: f64, z2: f64) c.Triangle {
    return c.Triangle{
        .v0 = .{ .x = x0, .y = y0, .z = z0 },
        .v1 = .{ .x = x1, .y = y1, .z = z1 },
        .v2 = .{ .x = x2, .y = y2, .z = z2 },
    };
}

test "triangle: direct hit (center)" {
    // Scenario: Triangle in the Z=5 plane, centered at (0,0).
    // Camera at origin (0,0,0) looking down +Z.
    //
    //      (0,1,5)
    //        ^
    //       / \
    //      /   \
    //     /  +  \  <-- Ray hits here (0,0,5)
    //    /       \
    // (-1,-1,5) (1,-1,5)

    const tri = createTriangle(-1.0, -1.0, 5.0, // v0 (Bottom Left)
        1.0, -1.0, 5.0, // v1 (Bottom Right)
        0.0, 1.0, 5.0 // v2 (Top Center)
    );

    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 0, .z = 1 }, // Forward
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_triangle(r, tri, &hit);

    try testing.expect(did_hit == true);

    // 1. Check Distance
    try testing.expectApproxEqAbs(@as(f64, 5.0), hit.t, epsilon);

    // 2. Check Hit Point
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.p.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.p.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 5.0), hit.p.z, epsilon);

    // 3. Check Normal
    // Winding: v0 -> v1 is (+X), v0 -> v2 is (+Y, +X).
    // Cross Product (Right Hand Rule) of edges in XY plane should point to -Z or +Z depending on winding.
    // Here, the flat triangle faces the camera. The normal should likely be (0,0,-1) to oppose the ray.
    // Note: If you implement "double sided" triangles, normal might flip to face ray.
    // For a standard geometric normal of this specific CCW winding: (0, 0, 1) or (0, 0, -1) depending on convention.
    // Let's just check it is normalized.
    const len_sq = hit.n.x * hit.n.x + hit.n.y * hit.n.y + hit.n.z * hit.n.z;
    try testing.expectApproxEqAbs(@as(f64, 1.0), len_sq, epsilon);
}

test "triangle: miss (outside bounds)" {
    // Same triangle as above
    const tri = createTriangle(-1, -1, 5, 1, -1, 5, 0, 1, 5);

    // Ray points to (2, 0, 5) -> clearly to the right of the triangle
    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        // Direction vector must be normalized for standard ray tracers,
        // though intersect logic usually handles unnormalized too.
        // We stick to simple axis aligned checks to avoid manual normalization math in test setup.
        // Let's imply target (2,0,5) -> dir (2,0,5)
        .dir = c.vec_normalize((c.Vec){ .x = 2, .y = 0, .z = 5 }),
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_triangle(r, tri, &hit);

    try testing.expect(did_hit == false);
}

test "triangle: miss (parallel ray)" {
    // Triangle on the floor (Y=0 plane)
    const tri = createTriangle(-1, 0, -1, 1, 0, -1, 0, 0, 1);

    // Ray fires horizontally at Y=1 (above the floor)
    const r = c.Ray{
        .origin = .{ .x = 0, .y = 1, .z = -5 },
        .dir = .{ .x = 0, .y = 0, .z = 1 }, // Parallel to the triangle plane
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_triangle(r, tri, &hit);

    try testing.expect(did_hit == false);
}

test "triangle: behind camera" {
    // Triangle is at Z = -5
    const tri = createTriangle(-1, -1, -5, 1, -1, -5, 0, 1, -5);

    // Ray fires forward (Z = +1) from origin
    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 0, .z = 1 },
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_triangle(r, tri, &hit);

    try testing.expect(did_hit == false);
}

test "triangle: edge case (vertex hit)" {
    // Hits exactly on the top tip vertex
    const tri = createTriangle(-1, -1, 5, 1, -1, 5, 0, 1, 5);

    // Ray points exactly at (0, 1, 5)
    const target = c.Vec{ .x = 0, .y = 1, .z = 5 };
    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = c.vec_normalize(target),
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_triangle(r, tri, &hit);

    // Barycentric coordinates usually handle v=1, u=0 (u+v=1) as inclusive.
    // Should be a hit.
    try testing.expect(did_hit == true);
    try testing.expectApproxEqAbs(c.vec_length(target), hit.t, epsilon);
}
