const std = @import("std");
const tracy = @cImport({
    @cInclude("tracy.h");
    @cInclude("exr_c.h");
});

pub fn main() !void {
    const width: i32 = 640;
    const height: i32 = 480;

    const filter_type = 0;

    const cam_angle_x = 0.04258603374866164;
    const cam_angle_y = 0.0;
    const cam_dist = 5.5;
    const focus_x = 0.0;
    const focus_y = 1.25;
    const focus_z = 0.0;

    const stdout = std.io.getStdOut().writer();

    try stdout.print("Rendering scene at {d}x{d}...\n", .{ width, height });

    tracy.render_init(width, height, filter_type, cam_angle_x, cam_angle_y, cam_dist, focus_x, focus_y, focus_z);

    var i: usize = 0;
    while (i < 10) : (i += 1) {
        tracy.render_refine(5);

        // Get the Linear Float HDR buffer (RGB)
        const buffer_ptr = tracy.update_image_hdr();

        try stdout.print("Step {d}/10: Saving to 'render_zig.exr'...\n", .{i + 1});

        // Save as FP16 EXR
        var err_msg: [*c]const u8 = null;
        const ret = tracy.save_exr_rgb_fp16("render_zig.exr", buffer_ptr, width, height, &err_msg);

        if (ret != 0) {
            if (err_msg != null) {
                try stdout.print("EXR Error: {s}\n", .{err_msg});
            }
            return error.ExrSaveFailed;
        }
    }

    try stdout.print("Done.\n", .{});
}
