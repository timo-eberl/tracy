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

    // Relative IOR: n1 / n2 = 1.0 / 1.5
    const eta = 1.0 / 1.5;

    var tir: bool = undefined;
    const result = c.refract(incident, normal, eta, &tir);

    // 1. Should not be total internal reflection
    try testing.expect(tir == false);

    // 2. Direction should be unchanged: (0, -1, 0)
    try testing.expectApproxEqAbs(@as(f64, 0.0), result.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, -1.0), result.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0.0), result.z, epsilon);
}

test "refract: air to glass (snell's law)" {
    // Scenario: Incoming ray at 45 degrees.
    // n1 = 1.0 (Air), n2 = 1.5 (Glass).
    // Snell's Law: n1 * sin(theta1) = n2 * sin(theta2)

    // Setup 45 degree vector (normalized)
    // sin(45) = cos(45) = 1/sqrt(2) approx 0.7071
    const incident = c.vec_normalize((c.Vec){ .x = 1, .y = -1, .z = 0 });
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 };
    const eta = 1.0 / 1.5;

    var tir: bool = undefined;
    const result = c.refract(incident, normal, eta, &tir);

    try testing.expect(tir == false);

    // Verify Math:
    // sin(theta1) = 0.707106
    // sin(theta2) = (n1/n2) * sin(theta1) = (1.0/1.5) * 0.707106 = 0.471404

    // The Result X component corresponds to sin(theta2)
    try testing.expectApproxEqAbs(@as(f64, 0.471404), result.x, epsilon);

    // The Result Y component should be negative (still going down)
    // magnitude should be cos(theta2) = sqrt(1 - sin^2(theta2))
    // sqrt(1 - 0.471404^2) = sqrt(0.7777) = 0.881917
    try testing.expectApproxEqAbs(@as(f64, -0.881917), result.y, epsilon);

    // Ensure result is normalized
    try testing.expectApproxEqAbs(@as(f64, 1.0), c.vec_length(result), epsilon);
}

test "refract: total internal reflection (glass to air)" {
    // Scenario: Light inside glass trying to escape to air at a steep angle.
    // n1 = 1.5 (Glass), n2 = 1.0 (Air).
    // Critical angle: arcsin(1.0 / 1.5) approx 41.8 degrees.
    // We test with 60 degrees from the normal to force TIR.

    // eta = n1 / n2 = 1.5 / 1.0
    const eta = 1.5;

    // Incident vector at 60 degrees relative to normal (Y-axis)
    // x = sin(60) = 0.866, y = -cos(60) = -0.5
    const incident = c.vec_normalize((c.Vec){ .x = 0.866025, .y = -0.5, .z = 0 });
    const normal = c.Vec{ .x = 0, .y = 1, .z = 0 };

    var tir: bool = undefined;
    const result = c.refract(incident, normal, eta, &tir);

    // 1. Should detect Total Internal Reflection
    try testing.expect(tir == true);

    // 2. Function should return Reflection Vector when TIR occurs
    // The Y component should simply flip (bouncing off the interface)
    try testing.expectApproxEqAbs(incident.x, result.x, epsilon);
    try testing.expectApproxEqAbs(-incident.y, result.y, epsilon);

    // Ensure result is normalized
    try testing.expectApproxEqAbs(@as(f64, 1.0), c.vec_length(result), epsilon);
}
