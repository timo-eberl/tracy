// ZIG rendering that computes rmse after each step, saves the images to a folder and prints the aggregate of the step scores to stdout
const std = @import("std");
const config = @import("config");
const tracy = @cImport({
    @cInclude("tracy.h");
    @cInclude("exr_c.h");
});
const rmse = @import("metrics/rmse/compute_rmse.zig");

const NUM_ITERATIONS = 10;

// Updated writeScores signature
fn writeScores(scores: [NUM_ITERATIONS]f32, timings: [NUM_ITERATIONS]f64, filepath: []const u8, variant: []const u8) !void {
    const file = try std.fs.cwd().createFile(filepath, .{ .truncate = false });
    try file.seekFromEnd(0);
    defer file.close();

    var bw = std.io.bufferedWriter(file.writer());
    const writer = bw.writer();

    try writer.print("VARIANT:{s}\n", .{variant});
    for (scores, 0..) |s, i| {
        // Format: score,time_seconds
        try writer.print("{d:.4},{d:.6}\n", .{ s, timings[i] });
    }
    try writer.print("\n", .{});
    try bw.flush();
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    const variant_label = if (config.multithreaded) "mt" else "st";

    // Dynamic output path based on mode
    const out_dir = "tests/img/exr/zig_render/";
    // We use std.fmt.allocPrint to create the filename dynamically
    const out_filename = try std.fmt.allocPrint(allocator, "render_{s}.exr", .{variant_label});
    defer allocator.free(out_filename);

    const out_fp_slice = try std.fs.path.join(allocator, &.{ out_dir, out_filename });
    defer allocator.free(out_fp_slice);
    const out_fp = try allocator.dupeZ(u8, out_fp_slice);
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
    try stdout.print("Rendering scene ({s}) at {d}x{d}...\n", .{ variant_label, width, height });

    tracy.render_init(width, height, filter_type, cam_angle_x, cam_angle_y, cam_dist, focus_x, focus_y, focus_z);

    var scores: [NUM_ITERATIONS]f32 = undefined;
    var timings: [NUM_ITERATIONS]f64 = undefined;
    var i: usize = 0;

    var timer = try std.time.Timer.start();

    while (i < NUM_ITERATIONS) : (i += 1) {
        const start_time = timer.read();
        tracy.render_refine(5);
        const end_time = timer.read();
        // Duration in seconds for this specific step
        timings[i] = @as(f64, @floatFromInt(end_time - start_time)) / std.time.ns_per_s;

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
    try writeScores(scores, timings, log_fp, variant_label);
    try stdout.print("Done. Results appended to {s}\n", .{log_fp});
}
