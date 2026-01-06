const std = @import("std");
const math = std.math;
const tga = @import("utils/tga.zig");
// load image
// pipeline load img -> (tone map?) --> (gamma correction?) --> split color channels --> calc SSIM --> average over channels

// fixed kernel size 11 for now
pub const Gaussian = struct {
    weights: [11]f32,
    pub fn init(sigma: f32) Gaussian {
        var weights: [11]f32 = undefined;
        @memset(&weights, 0.0);
        var sum: f32 = 0.0;
        for (0..11) |i| {
            const pos: f32 = @floatFromInt(i);
            const x = pos - 5.0;
            const exponent = -(x * x) / (2.0 * sigma * sigma);
            const val = @exp(exponent);
            weights[i] = val;
            sum += val;
        }
        // normalization
        for (weights, 0..) |w, i| {
            weights[i] = w / sum;
        }
        return Gaussian{
            .weights = weights,
        };
    }
};

// calculates the mean SSIM across the 3 color channels of a tga image
pub fn calcSSIM(allocator: std.mem.Allocator, tga_img_1: tga.ImageF32, tga_img_2: tga.ImageF32) !f32 {
    const gaussian = Gaussian.init(1.5);
    if (tga_img_1.width != tga_img_2.width or tga_img_1.height != tga_img_2.height) {
        return error.ImageSizeMismatchError;
    }

    const r_score = try calcChannelSSIM(allocator, tga_img_1.data_red, tga_img_2.data_red, tga_img_1.width, gaussian);
    const g_score = try calcChannelSSIM(allocator, tga_img_1.data_green, tga_img_2.data_green, tga_img_1.width, gaussian);
    const b_score = try calcChannelSSIM(allocator, tga_img_1.data_blue, tga_img_2.data_blue, tga_img_1.width, gaussian);

    const final_score = (r_score + g_score + b_score) / 3.0;
    return final_score;
}
// calculates Mean for a single pixel using a gaussian kernel
// generic type of the image vector to allow for u8 input values in the first pass
pub fn getWeightedMean(img: []const f32, width: usize, cx: usize, cy: usize, kernel: *const Gaussian, vertical: bool) f32 {
    const height = img.len / width;

    const i_center = if (vertical) @as(isize, @intCast(cy)) else @as(isize, @intCast(cx));
    const i_limit = if (vertical) @as(isize, @intCast(height)) else @as(isize, @intCast(width));

    var mean: f32 = 0.0;
    for (kernel.weights, 0..) |w, i| {
        const offset: isize = @as(isize, @intCast(i)) - 5;
        var neighbor = i_center + offset;
        // testing clamping negative indices to 0 to improve edge handling
        if (neighbor < 0) neighbor = 0;
        if (neighbor >= i_limit) neighbor = i_limit - 1;
        const neighbor_usize: usize = @intCast(neighbor);
        const pixelPos = if (vertical)
            width * neighbor_usize + cx
        else
            cy * width + neighbor_usize;
        mean += w * img[pixelPos];
        //PREVIOUS CHECK: if ((neighbor >= 0) and (neighbor < i_limit)) {
        //     const neighbor_usize: usize = @intCast(neighbor);
        //     const pixelPos = if (vertical)
        //         width * neighbor_usize + cx
        //     else
        //         cy * width + neighbor_usize;
        //     const img_val = img[pixelPos];
        //     mean += w * img_val;
        // }
    }
    return mean;
}

pub fn calcMeans(allocator: std.mem.Allocator, img: []const f32, width: usize, kernel: Gaussian) ![]f32 {
    const height = img.len / width;

    const temp = try allocator.alloc(f32, img.len);
    defer allocator.free(temp);
    const means = try allocator.alloc(f32, img.len);
    // horizontal pass
    for (0..height) |y| {
        for (0..width) |x| {
            const arrPos = width * y + x;
            temp[arrPos] = getWeightedMean(img, width, x, y, &kernel, false);
        }
    }
    for (0..height) |y| {
        for (0..width) |x| {
            const arrPos = width * y + x;
            means[arrPos] = getWeightedMean(temp, width, x, y, &kernel, true);
        }
    }
    return means;
}

// multiplies two image arrays
pub fn multiply(allocator: std.mem.Allocator, img_1: []const f32, img_2: []const f32) ![]f32 {
    std.debug.assert(img_1.len == img_2.len);
    const product = try allocator.alloc(f32, img_1.len);
    for (img_1, 0..) |v_1, i| {
        const v_2 = img_2[i];
        product[i] = v_1 * v_2;
    }
    return product;
}

pub fn calcChannelSSIM(allocator: std.mem.Allocator, img_1: []const f32, img_2: []const f32, width: usize, kernel: Gaussian) !f32 {
    const ally = allocator;

    // calculate means
    // E[x] and E[y]
    const mu1 = try calcMeans(ally, img_1, width, kernel);
    defer ally.free(mu1);
    const mu2 = try calcMeans(ally, img_2, width, kernel);
    defer ally.free(mu2);

    // calculate squares
    const img1_sq = try multiply(ally, img_1, img_1);
    defer ally.free(img1_sq);
    const img2_sq = try multiply(ally, img_2, img_2);
    defer ally.free(img2_sq);
    const img1_img2 = try multiply(ally, img_1, img_2);
    defer ally.free(img1_img2);

    // calculate sigma squares - Mean of squares
    // E[x^2], E[y^2], and E[xy]
    const mu1_sq = try calcMeans(ally, img1_sq, width, kernel);
    defer ally.free(mu1_sq);
    const mu2_sq = try calcMeans(ally, img2_sq, width, kernel);
    defer ally.free(mu2_sq);
    const mu1_mu2 = try calcMeans(ally, img1_img2, width, kernel);
    defer ally.free(mu1_mu2);

    // constants
    // Assuming images are loaded as 0..255 floats
    const L: f32 = 1.0; // dynamic Range
    const K1: f32 = 0.01;
    const K2: f32 = 0.03;
    const C1: f32 = (K1 * L) * (K1 * L);
    const C2: f32 = (K2 * L) * (K2 * L);

    var ssim_sum: f32 = 0.0;

    for (0..img_1.len) |i| {
        const m1 = mu1[i];
        const m2 = mu2[i];
        // squares of means
        const m1_sq_val = m1 * m1;
        const m2_sq_val = m2 * m2;
        const m1_m2_val = m1 * m2;
        // calculate variance using identity  formula
        // Variance(X)=Mean of Squares−(Square of Mean)
        // σ2=E[x2]−(E[x])2
        const sigma1_sq = @max(0.0, mu1_sq[i] - m1_sq_val);
        const sigma2_sq = @max(0.0, mu2_sq[i] - m2_sq_val);
        const sigma12 = mu1_mu2[i] - m1_m2_val;
        const numerator = (2.0 * m1_m2_val + C1) * (2.0 * sigma12 + C2);
        const denominator = (m1_sq_val + m2_sq_val + C1) * (sigma1_sq + sigma2_sq + C2);

        ssim_sum += numerator / denominator;
    }
    // return average SSIM score (0.0 to 1.0)
    return ssim_sum / @as(f32, @floatFromInt(img_1.len));
}

pub fn main() void {
    const g = Gaussian.init(1.5);
    _ = g;
}
