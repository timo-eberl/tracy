const std = @import("std");
const c = @cImport({
    @cInclude("exr_c.h");
});

pub const ExrImage = struct {
    width: usize,
    height: usize,
    pixels: []f32, //RGB buffer

    pub fn deinit(self: *ExrImage) void {
        // use the C-side free function because memory
        // was allocated by malloc() inside tinyexr, not by Zig.
        c.free_exr_rgba(self.pixels.ptr);
    }
};

pub fn loadExr(path: [:0]const u8) !ExrImage {
    var width: c_int = 0;
    var height: c_int = 0;
    var out_rgba: [*c]f32 = null;
    var err_msg: [*c]const u8 = null;
    const ret = c.load_exr_rgba(path, &out_rgba, &width, &height, &err_msg);

    if (ret != 0) {
        if (err_msg != null) {
            std.debug.print("TinyEXR Error: {s}\n", .{err_msg});
        }
        return error.FailedToLoadExr;
    }
    // convert c pointers to zig slices
    // width * height * 4 channels (R, G, B, A)
    const len = @as(usize, @intCast(width)) * @as(usize, @intCast(height)) * 4;

    return ExrImage{
        .width = @as(usize, @intCast(width)),
        .height = @as(usize, @intCast(height)),
        .pixels = out_rgba[0..len],
    };
}

// to change to a test file using the test keyword instead of main don't forget to switch b.addExecutable to b.addTest in the build.zig
pub fn main() !void {
    const filename = "reference.exr";

    std.debug.print("Loading {s}...\n", .{filename});
    var img = try loadExr(filename);
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
