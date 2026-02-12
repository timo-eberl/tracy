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
