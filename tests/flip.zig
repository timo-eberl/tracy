//TODO gaussian variabel machen beenden
const std = @import("std");
const math = std.math;
const filters = @import("utils/gaussian.zig");
// The FLIP Pipeline Summary
//
//     1. Load & Linearize
//
//         Input: Load Reference and Test images (standard sRGB).
//
//         Action: Convert sRGB to Linear RGB (undo gamma correction) so math represents physical light intensity.
//
//         Linearization Options
//         - full Image Range: set lowest value in img as 0.0 and highest value as 1.0
//         - full data range: map highest possible value to 1.0 and lowest to 0.0 -> used here
//
//      NOTE: Think about moving linearization to .exr file

pub fn linearizeChannel(allocator: std.mem.Allocator, channel: []f32) []f32 {
    var out_arr = allocator.make([channel.len]f32);
    defer allocator.free(out_arr);
for (channel, 0..) |v, i| {
    if (v <= 0.4045) {
            out_arr[i] = v / 12.92;
        } else {
            out_arr[i] = math.pow(f32, (v + 0.055) / 1.055, 2.4);
        }
    }
    return out_arr;
}
//
//     2. Color Space Transformation
//
//         Action: Convert Linear RGB to an Opponent Color Space (Y, cx​, cz​).
//
//         Components:
//
//             Luminance (Y): Brightness.
//
//             Chroma (cx​,cz​): Red-Green and Blue-Yellow differences.
pub fn convertColorSpace(allocator: std.mem.Allocator, channels: [3][]f32) []f32 {
    const r = channels[0];
    const g = channels[1];
    const b = channels[2];
    var out_arr = allocator.make([3][r.len]f32);
    defer allocator.free(out_arr);
    for (0..r.len) |i| {
        const r_val = r[i];
        const g_val = g[i];
        const b_val = b[i];
        const y_val = (r_val + g_val) / 2.0; // compute yellow as average of g and r

        const luminance = 0.2126 * r_val + 0.7252 * g_val + 0.0722 * b_val;
        // use linear approximation of CIELAB as constants using a ratio of 2.0. -> YCoCg standards
        const r_g = 0.5 * (r_val - g_val);
        const y_b = 0.25 * (b_val - y_val);
        out_arr[0][i] = luminance;
        out_arr[1][i] = r_g;
        out_arr[2][i] = y_b;
    }
    return out_arr;
}
//     3. Spatial Filtering (Simulating Human Vision)
//
//         Action: Convolve specific channels with Gaussian Kernels.
//
//         Detail: The "width" (sigma) of the blur is calculated using the monitor's Pixels-Per-Degree (PPD). This simulates how the eye blurs detail at a specific viewing distance.
//
//         Result: You now have "perceived" versions of the Reference and Test images.
fn spatialFilter(allocator: std.mem.allocator, gaussian: filters.Gaussian, channels: [3][]f32) {
    const lum = channels[0];
    const r_g = channels[1];
    const y_b = channels[2];
    var out_arr = allocator.make([3][lum.len]f32);
    
}

//     4. Feature Detection (Edge Masking)
//
//         Action: Run an edge detection filter (like a specialized high-pass filter) on the Luminance (Y) channel of both images.
//
//         Goal: Identify where edges are. Errors on edges are less visible (masked), but errors that move or destroy edges are highly visible.
//
//     5. Compute Difference Maps
//
//         Color Difference: Compute the Euclidean distance between the filtered Color channels (cx​,cz​).
//
//         Feature Difference: Compute the difference between the Feature (Edge) maps.
//
//     6. Pooling & Perception Mapping
//
//         Action: Combine the Color and Feature differences into a single value per pixel.
//
//         Remap: Apply a specific power-function (usually raising to ~0.7) to map the raw error to a probability range [0,1].
//
//     7. Final Output
//
//         FLIP Map: An image where pixel brightness equals error magnitude (useful for debugging).
//
//         FLIP Score: The arithmetic mean (average) of all pixels in the FLIP Map. (0.0 = Identical, 1.0 = Max Difference).
