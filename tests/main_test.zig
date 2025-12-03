const std = @import("std");
const expect = std.testing.expect;

// Import C Header

const c = @cImport({
    @cInclude("tracy.h");
});

test "Linkage Check: Vector Addition" {
    // create vec instances

    const v1 = c.Vec{ .x = 1.0, .y = 2.0, .z = 3.0 };
    const v2 = c.Vec{ .x = 4.0, .y = 5.0, .z = 6.0 };

    const result = c.vec_add(v1, v2);

    // expect   1+4=5, 2+5=7, 3+6=9
    try expect(result.x == 5.0);
    try expect(result.y == 7.0);
    try expect(result.z == 9.0);
}

test "Linkage Check: Constants" {
    // check for visibility of constants defined in c code
    try expect(c.MAX_DEPTH == 4);
}
