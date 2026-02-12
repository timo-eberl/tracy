//
// // fixed kernel size 11 for now
// pub const Gaussian = struct {
//     weights: [11]f32,
//     pub fn init(sigma: f32) Gaussian {
//         var weights: [11]f32 = undefined;
//         @memset(&weights, 0.0);
//         var sum: f32 = 0.0;
//         for (0..11) |i| {
//             const pos: f32 = @floatFromInt(i);
//             const x = pos - 5.0;
//             const exponent = -(x * x) / (2.0 * sigma * sigma);
//             const val = @exp(exponent);
//             weights[i] = val;
//             sum += val;
//         }
//         // normalization
//         for (weights, 0..) |w, i| {
//             weights[i] = w / sum;
//         }
//         return Gaussian{
//             .weights = weights,
//         };
//     }
// };
const std = @import("std");
const math = std.math;

pub const Gaussian = struct {
    // (Size = 2 * ceil(3*sigma) + 1)
    const MAX_SIZE = 101;

    weights: [MAX_SIZE]f32,
    len: usize,

    pub fn init(sigma: f32) Gaussian {
        // Calculate necessary size based on Sigma
        // We need 3 standard deviations to capture ~99.7% of the curve
        const radius_f = @ceil(3.0 * sigma);
        var width = @as(usize, @intFromFloat(radius_f)) * 2 + 1;
        // Safety clamp
        if (width > MAX_SIZE) width = MAX_SIZE;
        // Ensure width is odd
        if (width % 2 == 0) width += 1;
        var g = Gaussian{
            .weights = undefined,
            .len = width,
        };
        // Center Point (e.g., if width is 11, center is 5.0)
        const center = @as(f32, @floatFromInt(width / 2));
        var sum: f32 = 0.0;
        // Generate Weights
        for (0..width) |i| {
            const pos = @as(f32, @floatFromInt(i));
            const x = pos - center;
            // Standard Gaussian Formula
            const exponent = -(x * x) / (2.0 * sigma * sigma);
            const val = @exp(exponent);
            g.weights[i] = val;
            sum += val;
        }
        // Normalize
        for (0..width) |i| {
            g.weights[i] /= sum;
        }
        return g;
    }

    // Helper to get the valid slice easily
    pub fn slice(self: *const Gaussian) []const f32 {
        return self.weights[0..self.len];
    }
};

// Assumes your Gaussian struct has:
// 1. 'weights': A slice or array of f32
// 2. 'len': The number of valid weights to use
pub fn getWeightedMean(
    img: []const f32,
    width: usize,
    cx: usize,
    cy: usize,
    kernel: anytype, // Accepts *Gaussian or similar struct
    vertical: bool,
) f32 {
    const height = img.len / width;
    // Setup Directional Limits
    // If vertical, we move along Y (cy changes). Limit is height.
    // If horizontal, we move along X (cx changes). Limit is width.
    const i_center = if (vertical) @as(isize, @intCast(cy)) else @as(isize, @intCast(cx));
    const i_limit = if (vertical) @as(isize, @intCast(height)) else @as(isize, @intCast(width));
    // Dynamic Radius Calculation
    // We access the valid slice of weights.
    // If your struct has a helper like .slice(), use that.
    // Otherwise, we access weights[0..len].
    const valid_weights = kernel.weights[0..kernel.len];
    const radius = @as(isize, @intCast(valid_weights.len / 2));
    var mean: f32 = 0.0;
    // Convolution Loop
    for (valid_weights, 0..) |w, i| {
        // Calculate offset relative to center (e.g., -2, -1, 0, 1, 2)
        const offset = @as(isize, @intCast(i)) - radius;

        var neighbor = i_center + offset;

        // 4. Edge Handling: Clamp to Edge
        // If the kernel tries to read pixel -1, we read pixel 0 instead.
        // If it reads past width, we read the last pixel.
        if (neighbor < 0) neighbor = 0;
        if (neighbor >= i_limit) neighbor = i_limit - 1;

        // 5. Calculate 1D Array Index
        const neighbor_usize = @as(usize, @intCast(neighbor));

        var pixelPos: usize = 0;
        if (vertical) {
            // x is fixed (cx), y varies (neighbor)
            pixelPos = width * neighbor_usize + cx;
        } else {
            // y is fixed (cy), x varies (neighbor)
            pixelPos = cy * width + neighbor_usize;
            mean += w * img[pixelPos];
        }
        return mean;
    }
}
