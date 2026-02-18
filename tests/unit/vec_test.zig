const std = @import("std");
const testing = std.testing;

// Import the C implementation to access the internal Vec functions
const c = @cImport({
    @cInclude("../src/tracy.c");
});

const epsilon = 0.000001;

test "vec: basic arithmetic (add, sub, scale, hadamard)" {
    const a = c.Vec{ .x = 1, .y = 2, .z = 3 };
    const b = c.Vec{ .x = 4, .y = 5, .z = 6 };

    // Addition
    const sum = c.vec_add(a, b);
    try testing.expectApproxEqAbs(@as(f64, 5), sum.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 7), sum.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 9), sum.z, epsilon);

    // Subtraction
    const diff = c.vec_sub(b, a);
    try testing.expectApproxEqAbs(@as(f64, 3), diff.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 3), diff.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 3), diff.z, epsilon);

    // Scaling
    const scaled = c.vec_scale(a, 2.5);
    try testing.expectApproxEqAbs(@as(f64, 2.5), scaled.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 5.0), scaled.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 7.5), scaled.z, epsilon);

    // Hadamard (Element-wise) product
    const prod = c.vec_hadamard_prod(a, b);
    try testing.expectApproxEqAbs(@as(f64, 4), prod.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 10), prod.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 18), prod.z, epsilon);
}

test "vec: products and magnitude (dot, length)" {
    const v = c.Vec{ .x = 3, .y = 4, .z = 0 }; // 3-4-5 triangle

    // Dot product with itself is length squared
    const dot_self = c.vec_dot(v, v);
    try testing.expectApproxEqAbs(@as(f64, 25), dot_self, epsilon);

    // Length
    try testing.expectApproxEqAbs(@as(f64, 25), c.vec_length_squared(v), epsilon);
    try testing.expectApproxEqAbs(@as(f64, 5), c.vec_length(v), epsilon);

    // Orthogonality: dot product of perpendicular vectors is 0
    const v1 = c.Vec{ .x = 1, .y = 0, .z = 0 };
    const v2 = c.Vec{ .x = 0, .y = 1, .z = 0 };
    try testing.expectApproxEqAbs(@as(f64, 0), c.vec_dot(v1, v2), epsilon);
}

test "vec: normalization" {
    const v = c.Vec{ .x = 10, .y = 0, .z = 0 };
    const unit = c.vec_normalize(v);

    try testing.expectApproxEqAbs(@as(f64, 1), unit.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0), unit.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0), unit.z, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 1), c.vec_length(unit), epsilon);

    // Edge case: zero vector should not crash (returns 0,0,0 based on your C code)
    const zero = c.Vec{ .x = 0, .y = 0, .z = 0 };
    const norm_zero = c.vec_normalize(zero);
    try testing.expectEqual(@as(f64, 0), norm_zero.x);
}

test "vec: cross product" {
    const x_axis = c.Vec{ .x = 1, .y = 0, .z = 0 };
    const y_axis = c.Vec{ .x = 0, .y = 1, .z = 0 };

    // X cross Y should be Z
    const z_axis = c.vec_cross(x_axis, y_axis);
    try testing.expectApproxEqAbs(@as(f64, 0), z_axis.x, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 0), z_axis.y, epsilon);
    try testing.expectApproxEqAbs(@as(f64, 1), z_axis.z, epsilon);

    // Y cross X should be -Z (Right hand rule)
    const neg_z = c.vec_cross(y_axis, x_axis);
    try testing.expectApproxEqAbs(@as(f64, -1), neg_z.z, epsilon);
}
