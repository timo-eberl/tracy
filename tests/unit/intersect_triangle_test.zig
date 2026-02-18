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
        .two_sided = false,
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

    // Adjusted winding to (Bottom Left -> Top Center -> Bottom Right)
    // to produce a Normal pointing -Z (towards origin) for a front-face hit against a +Z ray.
    const tri = createTriangle(-1.0, -1.0, 5.0, // v0 (Bottom Left)
        0.0, 1.0, 5.0, // v1 (Top Center)
        1.0, -1.0, 5.0 // v2 (Bottom Right)
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
    // Winding: v0 -> v1 is (+1, +2), v0 -> v2 is (+2, 0).
    // Cross Product (Right Hand Rule) yields Normal (0, 0, -1) opposing the Ray (0, 0, 1).
    const len_sq = hit.n.x * hit.n.x + hit.n.y * hit.n.y + hit.n.z * hit.n.z;
    try testing.expectApproxEqAbs(@as(f64, 1.0), len_sq, epsilon);

    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.n.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.n.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, -1.0), hit.n.z, epsilon);

    // 4. Check Inside
    // Front face hit means we are outside.
    try testing.expect(hit.inside == false);
}

test "triangle: miss (outside bounds)" {
    // Same triangle as above (adjusted winding)
    const tri = createTriangle(-1, -1, 5, 0, 1, 5, 1, -1, 5);

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
    // Winding (BL -> TL -> BR) -> Normal +Y (0,1,0)
    const tri = createTriangle(-1, 0, 1, -1, 0, -1, 1, 0, 1);

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
    // Adjusted winding to face +Z (Normal (0,0,1)) so it would be visible if looking -Z.
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
    // Adjusted winding (BL -> TC -> BR)
    const tri = createTriangle(-1, -1, 5, 0, 1, 5, 1, -1, 5);

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
    try testing.expectApproxEqAbs(@as(f64, -1.0), hit.n.z, epsilon); // Normal opposes ray (approx)
}

test "triangle: two_sided (back face hit)" {
    // Triangle at Z=5.
    // Construct with winding (BL -> BR -> TC) which yields Geometric Normal (0,0,1).
    // This normal points in the same direction as the Ray (0,0,1), creating a back-face hit.
    var tri = createTriangle(-1, -1, 5, 1, -1, 5, 0, 1, 5);
    tri.two_sided = true;

    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 0, .z = 1 }, // Matches Geometric Normal direction
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
    // Because it is two_sided, the normal should be flipped to oppose the ray (Front face simulation).
    // Geometric Normal: (0, 0, 1) -> Hit Normal: (0, 0, -1).
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.n.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.n.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, -1.0), hit.n.z, epsilon);

    // 4. Check Inside
    try testing.expect(hit.inside == false);
}

test "triangle: back hit (one_sided / inside)" {
    // Triangle at Z=5.
    // Construct with winding (BL -> BR -> TC) which yields Geometric Normal (0,0,1).
    // This normal points in the same direction as the Ray (0,0,1).
    // Since two_sided is false, this counts as hitting the "inside" of a volume (exiting).
    var tri = createTriangle(-1, -1, 5, 1, -1, 5, 0, 1, 5);
    tri.two_sided = false;

    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 0, .z = 1 },
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_triangle(r, tri, &hit);

    try testing.expect(did_hit == true);

    // 1. Check Distance
    try testing.expectApproxEqAbs(@as(f64, 5.0), hit.t, epsilon);

    // 2. Check Normal
    // Normal should remain the geometric normal (pointing +Z) because we are treating it
    // as a volumetric exit, so the normal points "out" of the volume (same dir as ray).
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.n.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.n.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 1.0), hit.n.z, epsilon);

    // 3. Check Inside
    // Ray direction (0,0,1) matches Normal (0,0,1). Dot > 0.
    // inside should be true to indicate we are inside the object/exiting.
    try testing.expect(hit.inside == true);
}
