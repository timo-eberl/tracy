const std = @import("std");
const testing = std.testing;

// Import the C implementation to access the internal fresnel function
const c = @cImport({
    @cInclude("../src/tracy.c");
});

const epsilon = 0.00001;

test "fresnel: normal incidence (0 degrees, looking straight at it)" {
    // Scenario: Ray is directly opposing the normal.
    // Air (n=1.0) -> Glass (n=1.5)
    // At 0 degrees, the fresnel term is simply R0.
    // R0 = ((n1 - n2) / (n1 + n2))^2
    //    = ((1.0 - 1.5) / (1.0 + 1.5))^2
    //    = (-0.5 / 2.5)^2 = (-0.2)^2 = 0.04

    const incident = c.Vec{ .x = 0, .y = -1, .z = 0 }; // Down
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 }; // Up
    const ior = 1.5;
    const is_inside = false;

    const result = c.fresnel(incident, normal, is_inside, ior);

    try testing.expectApproxEqAbs(@as(f64, 0.04), result, epsilon);
}

test "fresnel: grazing angle (90 degrees, edge of object)" {
    // Scenario: Ray is perpendicular to the normal.
    // At grazing angles, reflection approaches 100% (1.0) regardless of IOR.
    // Schlick: R = R0 + (1 - R0)(1 - cos(90))^5
    //          = R0 + (1 - R0)(1 - 0)^5
    //          = R0 + 1 - R0
    //          = 1.0

    const incident = c.Vec{ .x = 1, .y = 0, .z = 0 }; // Right
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 }; // Up
    const ior = 1.5;
    const is_inside = false;

    const result = c.fresnel(incident, normal, is_inside, ior);

    try testing.expectApproxEqAbs(@as(f64, 1.0), result, epsilon);
}

test "fresnel: inside medium (glass to air)" {
    // Scenario: Ray originates inside the sphere and hits the surface to exit.
    // Ray direction and Normal direction are roughly the same (both pointing out).
    // The implementation of fresnel expects `is_inside` to flip the cosine sign logic.

    const incident = c.Vec{ .x = 0, .y = 1, .z = 0 }; // Up (Outwards)
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 }; // Up (Outwards geometric normal)
    const ior = 1.5;
    const is_inside = true;

    // R0 calculation is symmetric ((n1-n2)/(n1+n2))^2 is same as ((n2-n1)/(n2+n1))^2
    // So expected value is still 0.04

    const result = c.fresnel(incident, normal, is_inside, ior);

    try testing.expectApproxEqAbs(@as(f64, 0.04), result, epsilon);
}

test "fresnel: 45 degree approximation" {
    // Scenario: Common angle check.
    // cos(45) ~= 0.707106
    // term = (1 - 0.707106)^5 ~= 0.00216
    // R = 0.04 + (0.96 * 0.00216) ~= 0.04207

    const inv_sqrt2 = 1.0 / std.math.sqrt(2.0);
    const incident = c.Vec{ .x = inv_sqrt2, .y = -inv_sqrt2, .z = 0 };
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 };
    const ior = 1.5;
    const is_inside = false;

    const result = c.fresnel(incident, normal, is_inside, ior);

    // Allow slightly looser tolerance for the power approximation steps
    try testing.expectApproxEqAbs(@as(f64, 0.04207), result, 0.0001);
}

test "fresnel: total internal reflection (inside 45 degrees)" {
    // Scenario: Ray is inside glass (n=1.5) hitting surface at 45 degrees.
    // Critical angle is asin(1/1.5) = 41.8 degrees.
    // Since 45 > 41.8, we must have Total Internal Reflection (1.0).
    //
    // A buggy implementation using cos(incident) (cos 45 = 0.707) will
    // return a low value (approx 0.05) instead of 1.0.

    const inv_sqrt2 = 1.0 / std.math.sqrt(2.0);
    const incident = c.Vec{ .x = inv_sqrt2, .y = inv_sqrt2, .z = 0 }; // 45 deg Up/Right
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 }; // Up
    const ior = 1.5;
    const is_inside = true;

    const result = c.fresnel(incident, normal, is_inside, ior);

    try testing.expectApproxEqAbs(@as(f64, 1.0), result, epsilon);
}
