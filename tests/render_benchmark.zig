// ZIG rendering that computes rmse after each step, saves the images to a folder and prints the aggregate of the step scores to stdout
const std = @import("std");
const config = @import("config");
const tracy = @cImport({
    @cInclude("tracy.h");
    @cInclude("exr_c.h");
});
const rmse = @import("metrics/rmse/compute_rmse.zig");

fn writeScores(scores: [10]f32, filepath: []const u8, version: []const u8) !void {
    // Open for appending; create if it doesn't exist
    const file = try std.fs.cwd().createFile(filepath, .{ .truncate = false });
    try file.seekFromEnd(0);
    defer file.close();

    var bw = std.io.bufferedWriter(file.writer());
    const writer = bw.writer();

    // Header identifies this specific run's configuration
    try writer.print("VERSION:{s}\n", .{version});
    for (scores) |s| {
        try writer.print("{d:.4}\n", .{s});
    }
    // Double newline helps the Python parser identify the end of a run
    try writer.print("\n", .{});
    try bw.flush();
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    const version_label = if (config.multithreaded) "mt" else "st";

    // Dynamic output path based on mode
    const out_dir = "tests/img/exr/zig_render/";
    // We use std.fmt.allocPrint to create the filename dynamically
    const out_filename = try std.fmt.allocPrint(allocator, "render_{s}.exr", .{version_label});
    defer allocator.free(out_filename);

    const out_fp = try std.fs.path.join(allocator, &.{ out_dir, out_filename });
    defer allocator.free(out_fp);

    try std.fs.cwd().makePath(out_dir);

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
    try stdout.print("Rendering scene ({s}) at {d}x{d}...\n", .{ version_label, width, height });

    tracy.render_init(width, height, filter_type, cam_angle_x, cam_angle_y, cam_dist, focus_x, focus_y, focus_z);

    var scores: [10]f32 = undefined;
    var i: usize = 0;

    while (i < 10) : (i += 1) {
        tracy.render_refine(5);
        const buffer_ptr = tracy.update_image_hdr();

        var err_msg: [*c]const u8 = null;
        const ret = tracy.save_exr_rgb_fp16(out_fp, buffer_ptr, width, height, &err_msg);

        if (ret != 0) {
            if (err_msg != null) try stdout.print("EXR Error: {s}\n", .{err_msg});
            return error.ExrSaveFailed;
        }

        scores[i] = try rmse.computeScore(out_fp);
    }

    const log_fp = out_dir ++ "zig_render_log.txt";
    try writeScores(scores, log_fp, version_label);
    try stdout.print("Done. Results appended to {s}\n", .{log_fp});
}
