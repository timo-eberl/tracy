const std = @import("std");
const math = std.math;
const testing = std.testing;
const ssim = @import("ssim.zig");
const tga = @import("utils/tga.zig");

const gaussian = ssim.Gaussian.init(1.5);
const WIDTH = 3;
const img_example = [_]u8{ 10, 20, 30, 40, 50, 60, 70, 80, 90 };

fn compareAndLog(allocator: std.mem.Allocator, path_a: []const u8, path_b: []const u8) !void {
    // 1. Load both images
    const img_a = try tga.loadTgaF32(allocator, path_a);
    defer img_a.deinit();

    const img_b = try tga.loadTgaF32(allocator, path_b);
    defer img_b.deinit();

    // 2. Perform SSIM
    const score = try ssim.calcSSIM(allocator, img_a, img_b);

    // 3. Log results prettily
    // We use std.fs.path.basename to strip "tests/img/" from the print output for clarity
    const name_a = std.fs.path.basename(path_a);
    const name_b = std.fs.path.basename(path_b);

    // Formatting:
    // {s: <35} means "print string, left-aligned, padded to 35 chars"
    // {d:.6}   means "print float with 6 decimal places"
    std.debug.print("{s: <35} vs  {s: <35} | SSIM: {d:.6}\n", .{ name_a, name_b, score });
}

// test "getWeightedMean calculates weighted sum of a pixel" {
//     // A 3x3 image (linearized)
//     // Row 0: 10, 20, 30
//     // Row 1: 40, 50, 60
//     // Row 2: 70, 80, 90
//
//     for (img_example, 0..) |v, i| {
//         _ = v;
//         const index: f32 = @floatFromInt(i);
//         const cx: usize = @intFromFloat(@mod(index, WIDTH));
//         const cy: usize = @intFromFloat(@ceil((index + 1.0) / WIDTH) - 1.0);
//         //std.debug.print("\nx-pos: {any}\n, y-pos: {any}", .{ cx, cy });
//         //
//         const mean = ssim.getWeightedMean(u8, &img_example, WIDTH, cx, cy, &gaussian, true);
//         std.debug.print("\nMEAN: {any}\n", .{mean});
//     }
// }
//
// test "calcMeans calculates means over an array of image values" {
//     const ally = std.testing.allocator;
//     const means = try ssim.calcMeans(ally, &img_example, WIDTH, gaussian);
//     defer ally.free(means);
//
//     std.debug.print("\nMEANS: {any}\n", .{means});
// }

// AI-Generated tests
//

test "SSIM" {
    const ally = testing.allocator;
    const base = "tests/img/";

    std.debug.print("\n========================= SSIM REPORT =========================\n", .{});

    // Convergence tests (Sequential improvements)
    try compareAndLog(ally, base ++ "black.tga", base ++ "1_iterations_render_zig.tga");
    try compareAndLog(ally, base ++ "1_iterations_render_zig.tga", base ++ "2_iterations_render_zig.tga");
    try compareAndLog(ally, base ++ "2_iterations_render_zig.tga", base ++ "3_iterations_render_zig.tga");
    try compareAndLog(ally, base ++ "3_iterations_render_zig.tga", base ++ "4_iterations_render_zig.tga");
    try compareAndLog(ally, base ++ "4_iterations_render_zig.tga", base ++ "5_iterations_render_zig.tga");
    try compareAndLog(ally, base ++ "5_iterations_render_zig.tga", base ++ "20_iterations_render_zig.tga");

    std.debug.print("---------------------------------------------------------------\n", .{});

    // Control tests (Comparison against black/empty)
    try compareAndLog(ally, base ++ "20_iterations_render_zig.tga", base ++ "black.tga");

    std.debug.print("===============================================================\n", .{});
}

// ==========================================
// TEST HELPERS
// ==========================================

fn clamp(val: f32) f32 {
    return @max(0.0, @min(255.0, val));
}

fn genGradient(allocator: std.mem.Allocator, width: usize, height: usize) ![]f32 {
    const img = try allocator.alloc(f32, width * height);
    for (0..height) |y| {
        for (0..width) |x| {
            const val = (@as(f32, @floatFromInt(x)) / @as(f32, @floatFromInt(width - 1))) * 255.0;
            img[y * width + x] = val;
        }
    }
    return img;
}

fn genCheckerboard(allocator: std.mem.Allocator, width: usize, height: usize, checkSize: usize) ![]f32 {
    const img = try allocator.alloc(f32, width * height);
    for (0..height) |y| {
        for (0..width) |x| {
            const x_check = (x / checkSize) % 2;
            const y_check = (y / checkSize) % 2;
            const val: f32 = if (x_check == y_check) 255.0 else 0.0;
            img[y * width + x] = val;
        }
    }
    return img;
}

fn copyAddBrightness(allocator: std.mem.Allocator, input: []const f32, amount: f32) ![]f32 {
    const img = try allocator.dupe(f32, input);
    for (img) |*val| {
        val.* = clamp(val.* + amount);
    }
    return img;
}

fn copyModContrast(allocator: std.mem.Allocator, input: []const f32, factor: f32) ![]f32 {
    const img = try allocator.dupe(f32, input);
    const mid: f32 = 127.5;
    for (img) |*val| {
        val.* = clamp(mid + (val.* - mid) * factor);
    }
    return img;
}

fn copyAddNoise(allocator: std.mem.Allocator, input: []const f32, amount: f32) ![]f32 {
    var prng = std.Random.Xoshiro256.init(12345);
    const random = prng.random();

    const img = try allocator.dupe(f32, input);
    for (img) |*val| {
        const noise = (random.float(f32) * 2.0 - 1.0) * amount;
        val.* = clamp(val.* + noise);
    }
    return img;
}
