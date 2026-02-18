const std = @import("std");
const testing = std.testing;

// Import the C implementation to access the internal intersect_sphere function
// and the data structures (Ray, Sphere, HitInfo, Vec).
const c = @cImport({
    @cInclude("../src/tracy.c");
});

const epsilon = 0.00001;

// Helper to create a dummy sphere to reduce boilerplate in tests
fn createSphere(x: f64, y: f64, z: f64, r: f64) c.Sphere {
    return c.Sphere{
        .center = .{ .x = x, .y = y, .z = z },
        .radius = r,
        .color = .{ .x = 1.0, .y = 1.0, .z = 1.0 }, // Color irrelevant for intersection
        .type = c.DIFFUSE, // Type irrelevant for intersection geometry
    };
}

test "intersection: direct hit (head-on)" {
    // Scenario: Sphere directly in front of the camera
    const s = createSphere(0, 0, 10, 2.0);

    // Ray starting at origin, pointing along +Z
    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 0, .z = 1 },
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_sphere(r, s, &hit);

    try testing.expect(did_hit == true);

    // Expected distance: Center (10) - Radius (2) = 8
    try testing.expectApproxEqAbs(@as(f64, 8.0), hit.t, epsilon);

    // Expected Normal: Pointing back towards origin (0, 0, -1)
    // The hit point is (0, 0, 8), center is (0, 0, 10) -> normal vector is (0, 0, -1)
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.n.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0.0), hit.n.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, -1.0), hit.n.z, epsilon);

    // Ray started outside
    try testing.expect(hit.inside == false);
}

test "intersection: miss (ray points away)" {
    // Scenario: Sphere is in front, but ray points up
    const s = createSphere(0, 0, 10, 1.0);

    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 1, .z = 0 }, // Up
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_sphere(r, s, &hit);

    try testing.expect(did_hit == false);
}

test "intersection: inside sphere (refraction case)" {
    // Scenario: Ray starts *inside* a large sphere (like being in a glass bubble)
    // Sphere at origin, radius 5
    const s = createSphere(0, 0, 0, 5.0);

    // Ray at origin, pointing +X
    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 1, .y = 0, .z = 0 },
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_sphere(r, s, &hit);

    try testing.expect(did_hit == true);

    // Distance should be exactly radius (5.0)
    try testing.expectApproxEqAbs(@as(f64, 5.0), hit.t, epsilon);

    // The 'inside' flag must be true.
    // This allows the renderer to flip the normal and invert IOR later.
    try testing.expect(hit.inside == true);

    // Note: The geometric normal calculated by intersect_sphere points OUTWARDS from center.
    // Hit point (5,0,0), Center (0,0,0) -> Normal (1,0,0)
    // The shading logic in `radiance_from_ray` is responsible for flipping it if `inside` is true.
    try testing.expectApproxEqAbs(@as(f64, 1.0), hit.n.x, epsilon);
}

test "intersection: object behind camera" {
    // Scenario: Sphere is at z = -10 (behind), Camera looks at +Z
    const s = createSphere(0, 0, -10, 2.0);

    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 0, .z = 1 },
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_sphere(r, s, &hit);

    // The mathematical quadratic formula yields two solutions (t = -8 and t = -12),
    // but the function must reject them because they are negative (behind ray).
    try testing.expect(did_hit == false);
}

test "intersection: offset hit (off-center)" {
    // Scenario: Sphere slightly to the right, ray goes straight
    // This tests if the discriminant math works for non-zero B and C coefficients
    // Sphere at (1, 0, 5), Radius 1. Touching the Z-axis at (0,0,5) is the edge case.
    // Let's make it intersect: Sphere at (0.5, 0, 5), Radius 1.
    const s = createSphere(0.5, 0, 5, 1.0);

    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 0, .z = 1 },
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_sphere(r, s, &hit);

    try testing.expect(did_hit == true);

    // Math check:
    // Circle equation in XZ plane relative to ray: x^2 + z^2 = r^2
    // Ray passes at x = -0.5 relative to sphere center.
    // (-0.5)^2 + z^2 = 1^2  =>  0.25 + z^2 = 1  => z^2 = 0.75
    // z = sqrt(0.75) ≈ 0.866025
    // Hit distance t = center_z - z_offset = 5.0 - 0.866025 = 4.133975
    try testing.expectApproxEqAbs(@as(f64, 4.13397), hit.t, epsilon);

    // Normal calculation check:
    // Hit point p = (0, 0, 4.133975)
    // Center c = (0.5, 0, 5.0)
    // p - c = (-0.5, 0, -0.866025)
    // Length is 1 (radius).
    // Normal x should be -0.5
    try testing.expectApproxEqAbs(@as(f64, -0.5), hit.n.x, epsilon);
}

test "intersection: grazing hit (tangent)" {
    // Scenario: Ray barely touches the sphere.
    // Sphere at (1, 0, 5), Radius 1.
    // Ray along +Z at x=0 should graze it at z=5.
    // Ideally this is a hit, but floating point precision might make it tricky.
    // We move the sphere *slightly* closer to X=0 to guarantee a hit or check edge behavior.
    const s = createSphere(0.999, 0, 5, 1.0);

    const r = c.Ray{
        .origin = .{ .x = 0, .y = 0, .z = 0 },
        .dir = .{ .x = 0, .y = 0, .z = 1 },
    };

    var hit: c.HitInfo = undefined;
    const did_hit = c.intersect_sphere(r, s, &hit);

    try testing.expect(did_hit == true);
    // Should be very close to z=5
    try testing.expectApproxEqAbs(@as(f64, 5.0), hit.t, 0.1);
}
