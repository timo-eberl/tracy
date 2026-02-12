const std = @import("std");

const exr_utils = @import("exr_utils");

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

    // iterate over pixels [R G B A]
    var i: usize = 0;
    while (i < ref_img.pixels.len) : (i += 4) {
        const r_ref = ref_img.pixels[i + 0];
        const g_ref = ref_img.pixels[i + 1];
        const b_ref = ref_img.pixels[i + 2];
        // skip alpha channel

        const r_test = test_img.pixels[i + 0];
        const g_test = test_img.pixels[i + 1];
        const b_test = test_img.pixels[i + 2];

        const r_diff = r_ref - r_test;
        const g_diff = g_ref - g_test;
        const b_diff = b_ref - b_test;

        sum_sq_diff += (r_diff * r_diff) + (g_diff * g_diff) + (b_diff * b_diff);
    }

    // calculate RMSE
    // Mean Squared Error per channel (we summed 3 channels per pixel)
    const mse = sum_sq_diff / @as(f64, @floatFromInt(total_pixels * 3));
    return @as(f32, @floatCast(std.math.sqrt(mse)));
}
