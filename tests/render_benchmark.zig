// ZIG rendering that computes rmse after each step, saves the images to a folder and prints the aggregate of the step scores to stdout
const std = @import("std");
const config = @import("config");
const tracy = @cImport({
    @cInclude("tracy.h");
    @cInclude("exr_c.h");
});
const rmse = @import("metrics/rmse/compute_rmse.zig");

// writes the benchmarking results to a log file per mode per scene
fn writeScores(scores: []const f32, timings: []const f64, filepath: []const u8, variant: []const u8) !void {
    const file = try std.fs.cwd().createFile(filepath, .{ .truncate = true });
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

pub fn runRender(allocator: std.mem.Allocator, scene: []const u8, iterations: u32) !void {
    const variant_label = if (config.multithreaded) "mt" else "st";
    const out_dir = "tests/img/exr/zig_render/";

    const out_filename = try std.fmt.allocPrint(allocator, "render_{s}_{s}.exr", .{ scene, variant_label });
    defer allocator.free(out_filename);

    const out_fp_slice = try std.fs.path.join(allocator, &.{ out_dir, out_filename });
    defer allocator.free(out_fp_slice);
    const out_fp = try allocator.dupeZ(u8, out_fp_slice);
    defer allocator.free(out_fp);

    try std.fs.cwd().makePath(out_dir);

    const stdout = std.io.getStdOut().writer();
    try stdout.print("Rendering scene {s} ({s}) at 640x480...\n", .{ scene, variant_label });

    tracy.render_init(640, 480, 0, 0.04258, 0.0, 5.5, 0.0, 1.25, 0.0);

    // uncomment when scene path arg gets added to tracy init
    // const scene_path_c = try allocator.dupeZ(u8, scene);
    // defer allocator.free(scene_path_c);
    tracy.render_init(width, height, filter_type, cam_angle_x, cam_angle_y, cam_dist, focus_x, focus_y, focus_z);
    var scores = try allocator.alloc(f32, iterations);
    defer allocator.free(scores);
    var timings = try allocator.alloc(f64, iterations);
    defer allocator.free(timings);

    var timer = try std.time.Timer.start();
    var i: usize = 0;
    while (i < iterations) : (i += 1) {
        timer.reset(); // Clock starts at 0 now
        tracy.render_refine(5);

        // duration_ns is exactly the time spent in render_refine
        const duration_ns = timer.read();
        timings[i] = @as(f64, @floatFromInt(duration_ns)) / std.time.ns_per_s;

        const buffer_ptr = tracy.update_image_hdr();
        var err_msg: [*c]const u8 = null;
        if (tracy.save_exr_rgb_fp16(out_fp, buffer_ptr, 640, 480, &err_msg) != 0) {
            return error.ExrSaveFailed;
        }

        scores[i] = try rmse.computeScore(out_fp);
    }

    const log_fp = try std.fmt.allocPrint(allocator, "{s}render_log_{s}_{s}.txt", .{ out_dir, scene, variant_label });
    defer allocator.free(log_fp);

    try writeScores(scores, timings, log_fp, variant_label);
    try stdout.print("Done. Results written to {s}\n", .{log_fp});
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    if (args.len < 3) {
        std.debug.print("Usage: render_bench <scene> <iterations>\n", .{});
        return;
    }

    const scene = args[1];
    const iterations = try std.fmt.parseInt(u32, args[2], 10);

    try runRender(allocator, scene, iterations);
}
