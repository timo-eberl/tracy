const std = @import("std");
const testing = std.testing;
const exr_utils = @import("exr_utils");
const rmse = @import("rmse.zig");

fn compareAndLog(allocator: std.mem.Allocator, path_a: [:0]const u8, path_b: [:0]const u8) !void {
    _ = allocator;
    // Load both images
    var ref_img = try exr_utils.loadExr(path_a);
    defer ref_img.deinit();

    var test_img = try exr_utils.loadExr(path_b);
    defer test_img.deinit();

    // Log results prettily
    // We use std.fs.path.basename to strip "tests/img/" from the print output for clarity
    const name_a = std.fs.path.basename(path_a);
    const name_b = std.fs.path.basename(path_b);

    // Formatting:
    // {s: <35} means "print string, left-aligned, padded to 35 chars"
    // {d:.6}   means "print float with 6 decimal places"
    // Run the metric
    const score = try rmse.computeRmse(ref_img, test_img);

    //std.debug.print(": {d:.6}\n", .{rmse_score});

    // Simple Pass/Fail for CI/CD
    // if (rmse > 0.01) {
    //     std.debug.print("FAIL: Error too high!\n", .{});
    //     std.process.exit(1);
    // }
    std.debug.print("{s: <35} vs  {s: <35} | Convergence Error (RMSE): {d:.6}\n", .{ name_a, name_b, score });
}

test "RMSE test" {
    const ally = testing.allocator;
    const base = "tests/img/";

    std.debug.print("\n========================= SSIM REPORT =========================\n", .{});

    // Convergence tests (Sequential improvements)
    try compareAndLog(ally, base ++ "black.exr", base ++ "1_iterations_render_zig.exr");
    try compareAndLog(ally, base ++ "1_iterations_render_zig.exr", base ++ "2_iterations_render_zig.exr");
    try compareAndLog(ally, base ++ "2_iterations_render_zig.exr", base ++ "3_iterations_render_zig.exr");
    try compareAndLog(ally, base ++ "3_iterations_render_zig.exr", base ++ "4_iterations_render_zig.exr");
    try compareAndLog(ally, base ++ "4_iterations_render_zig.exr", base ++ "5_iterations_render_zig.exr");
    try compareAndLog(ally, base ++ "5_iterations_render_zig.exr", base ++ "20_iterations_render_zig.exr");

    std.debug.print("---------------------------------------------------------------\n", .{});
    // Control tests (Comparison against black/empty)
    try compareAndLog(ally, base ++ "20_iterations_render_zig.exr", base ++ "black.exr");

    std.debug.print("===============================================================\n", .{});
}
