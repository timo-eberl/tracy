// ZIG rendering that computes rmse after each step, saves the images to a folder and prints the aggregate of the step scores to stdout
const std = @import("std");
const tracy = @cImport({
    @cInclude("tracy.h");
    @cInclude("exr_c.h");
});
const rmse = @import("metrics/rmse/compute_rmse.zig");

fn writeScores(scores: [10]f32, filepath: []const u8) !void {
    const file = try std.fs.cwd().createFile(filepath, .{});
    defer file.close();
    // wrap file in buffered writer
    var bw = std.io.bufferedWriter(file.writer());
    const writer = bw.writer();

    for (scores) |s| {
        try writer.print("{d:.4}\n", .{s});
    }
    try bw.flush();
}

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

    // init allocator
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const ally = gpa.allocator();

    var scores: [10]f32 = undefined;
    var score: f32 = undefined;
    var i: usize = 0;

    while (i < 10) : (i += 1) {
        tracy.render_refine(5);

        // Get the Linear Float HDR buffer (RGB)
        const buffer_ptr = tracy.update_image_hdr();
        {
            try std.fs.cwd().makePath("tests/img/exr/zig_render/");
            try stdout.print("Step {d}/10: Saving to 'tests/img/exr/zig_render/render_zig_{d}.exr'...\n", .{ i + 1, i });

            const out_fp = try std.fmt.allocPrintZ(ally, "tests/img/exr/zig_render/render_zig_{d}.exr", .{i});
            defer ally.free(out_fp);
            // Save as FP16 EXR
            var err_msg: [*c]const u8 = null;
            const ret = tracy.save_exr_rgb_fp16(out_fp, buffer_ptr, width, height, &err_msg);

            if (ret != 0) {
                if (err_msg != null) {
                    try stdout.print("EXR Error: {s}\n", .{err_msg});
                }
                return error.ExrSaveFailed;
            }
            score = try rmse.computeScore(out_fp);
            scores[i] = score;

            try stdout.print("computed score: {d}\n", .{score});
        }
    }
    const log_fp = "tests/img/exr/zig_render/zig_render_log.txt";
    try writeScores(scores, log_fp);
    try stdout.print("Done.\n", .{});
}
