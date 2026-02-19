const std = @import("std");

const exr_utils = @import("exr_utils");

pub fn compareAndLog(allocator: std.mem.Allocator, path_a: [:0]const u8, path_b: [:0]const u8) !f32 {
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
    const score = try computeRmse(ref_img, test_img);

    //std.debug.print(": {d:.6}\n", .{rmse_score});

    // Simple Pass/Fail for CI/CD
    // if (rmse > 0.01) {
    //     std.debug.print("FAIL: Error too high!\n", .{});
    //     std.process.exit(1);
    // }
    std.debug.print("{s: <35} vs  {s: <35} | Convergence Error (RMSE): {d:.6}\n", .{ name_a, name_b, score });
    return score;
}

pub fn computeRmse(ref_img: exr_utils.ExrImage, test_img: exr_utils.ExrImage) !f32 {
    const w_check: usize = ref_img.width;
    _ = w_check;
    // check dimensions
    if ((ref_img.width != test_img.width) or (ref_img.height != test_img.height)) {
        std.debug.print("Size Mismatch: {d}x{d} vs {d}x{d}\n", .{ ref_img.width, ref_img.height, test_img.width, test_img.height });
        return error.DimensionMismatch;
    }

    var sum_sq_diff: f64 = 0.0;
    const total_pixels = ref_img.width * ref_img.height;

    // iterate over pixels [B G R A]
    var i: usize = 0;
    while (i < ref_img.pixels.len) : (i += 4) {
        const b_ref = ref_img.pixels[i + 0];
        const g_ref = ref_img.pixels[i + 1];
        const r_ref = ref_img.pixels[i + 2];
        // skip alpha channel

        const b_test = test_img.pixels[i + 0];
        const g_test = test_img.pixels[i + 1];
        const r_test = test_img.pixels[i + 2];

        const b_se = (b_ref - b_test) * (b_ref - b_test);
        const g_se = (g_ref - g_test) * (g_ref - g_test);
        const r_se = (r_ref - r_test) * (r_ref - r_test);

        const eps = 0.000001; // Avoid division by zero

        // standard MSE
        // sum_sq_diff += (r_diff * r_diff) + (g_diff * g_diff) + (b_diff * b_diff);

        // relative MSE
        sum_sq_diff += b_se / (b_ref * b_ref + eps) + g_se / (g_ref * g_ref + eps) + r_se / (r_ref * r_ref + eps);
    }

    // calculate RMSE
    // Mean Squared Error per channel (we summed 3 channels per pixel)
    const mse = sum_sq_diff / @as(f64, @floatFromInt(total_pixels * 3));
    return @as(f32, @floatCast(std.math.sqrt(mse)));
}
