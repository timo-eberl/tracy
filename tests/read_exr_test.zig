const std = @import("std");
const exr_utils = @import("utils/exr.zig");
// to change to a test file using the test keyword instead of main don't forget to switch b.addExecutable to b.addTest in the build.zig
pub fn main() !void {
    const filename = "reference.exr";

    std.debug.print("Loading {s}...\n", .{filename});
    var img = try exr_utils.loadExr(filename);
    defer img.deinit();

    std.debug.print("Success!\n", .{});
    std.debug.print("Dimensions: {d} x {d}\n", .{ img.width, img.height });

    // Print the first pixel to verify data
    // Index 0=R, 1=G, 2=B, 3=A
    const pixel_idx = 0;
    std.debug.print("Pixel[0] RGBA: ({d:.3}, {d:.3}, {d:.3}, {d:.3})\n", .{
        img.pixels[pixel_idx + 0],
        img.pixels[pixel_idx + 1],
        img.pixels[pixel_idx + 2],
        img.pixels[pixel_idx + 3],
    });
}
