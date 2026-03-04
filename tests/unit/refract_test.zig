const std = @import("std");
const testing = std.testing;

// Import the C implementation to access the internal refract function
const c = @cImport({
    @cInclude("../src/tracy.c");
});

const epsilon = 0.00001;

test "refract: perpendicular (no bending)" {
    // Scenario: Ray going straight down, normal pointing up.
    // Light goes from Air (n=1.0) to Glass (n=1.5).
    // The ray should continue straight without bending.

    const incident = c.Vec{ .x = 0, .y = -1, .z = 0 };
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 };

    const ior_from = 1.0;
    const ior_to = 1.5;

    const result = c.refract(incident, normal, ior_from, ior_to);

    // Direction should be unchanged: (0, -1, 0)
    try testing.expectApproxEqAbs(@as(f32, 0.0), result.x, epsilon);
    try testing.expectApproxEqAbs(@as(f32, -1.0), result.y, epsilon);
    try testing.expectApproxEqAbs(@as(f32, 0.0), result.z, epsilon);
}

test "refract: air to glass (snell's law)" {
    // Scenario: Incoming ray at 45 degrees.
    // n1 = 1.0 (Air), n2 = 1.5 (Glass).
    // Snell's Law: n1 * sin(theta1) = n2 * sin(theta2)

    // Setup 45 degree vector (normalized)
    // sin(45) = cos(45) = 1/sqrt(2) approx 0.7071
    const incident = c.vec_normalize((c.Vec){ .x = 1, .y = -1, .z = 0 });
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 };

    const ior_from = 1.0;
    const ior_to = 1.5;

    const result = c.refract(incident, normal, ior_from, ior_to);

    // Verify Math:
    // sin(theta1) = 0.707106
    // sin(theta2) = (n1/n2) * sin(theta1) = (1.0/1.5) * 0.707106 = 0.471404

    // The Result X component corresponds to sin(theta2)
    try testing.expectApproxEqAbs(@as(f32, 0.471404), result.x, epsilon);

    // The Result Y component should be negative (still going down)
    // magnitude should be cos(theta2) = sqrt(1 - sin^2(theta2))
    // sqrt(1 - 0.471404^2) = sqrt(0.7777) = 0.881917
    try testing.expectApproxEqAbs(@as(f32, -0.881917), result.y, epsilon);

    // Ensure result is normalized
    try testing.expectApproxEqAbs(@as(f32, 1.0), c.vec_length(result), epsilon);
}
