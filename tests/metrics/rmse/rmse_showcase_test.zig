const std = @import("std");
const testing = std.testing;
const exr_utils = @import("exr_utils");
const rmse = @import("rmse.zig");

test "RMSE test" {
    const ally = testing.allocator;
    const base = "tests/img/";

    std.debug.print("\n========================= SSIM REPORT =========================\n", .{});

    // Convergence tests (Sequential improvements)
    try rmse.compareAndLog(ally, base ++ "black.exr", base ++ "1_iterations_render_zig.exr");
    try rmse.compareAndLog(ally, base ++ "1_iterations_render_zig.exr", base ++ "2_iterations_render_zig.exr");
    try rmse.compareAndLog(ally, base ++ "2_iterations_render_zig.exr", base ++ "3_iterations_render_zig.exr");
    try rmse.compareAndLog(ally, base ++ "3_iterations_render_zig.exr", base ++ "4_iterations_render_zig.exr");
    try rmse.compareAndLog(ally, base ++ "4_iterations_render_zig.exr", base ++ "5_iterations_render_zig.exr");
    try rmse.compareAndLog(ally, base ++ "5_iterations_render_zig.exr", base ++ "20_iterations_render_zig.exr");

    std.debug.print("---------------------------------------------------------------\n", .{});
    // Control tests (Comparison against black/empty)
    try rmse.compareAndLog(ally, base ++ "20_iterations_render_zig.exr", base ++ "black.exr");

    std.debug.print("===============================================================\n", .{});
}
