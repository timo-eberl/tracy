const std = @import("std");
const tracy = @cImport({
    @cInclude("tracy.h");
});

/// Writes the image buffer to a PPM (P3 ASCII) file.
fn saveImageAsPpm(file_path: []const u8, image_buffer: []u8, width: i32, height: i32) !void {
    const file = try std.fs.cwd().createFile(file_path, .{});
    defer file.close();

    var bw = std.io.bufferedWriter(file.writer());
    const writer = bw.writer();

    try writer.print("P3\n{d} {d}\n255\n", .{ width, height });

    const num_pixels: usize = @intCast(width * height);
    for (0..num_pixels) |i| {
        try writer.print("{d} {d} {d}\n", .{
            image_buffer[i * 4 + 0],
            image_buffer[i * 4 + 1],
            image_buffer[i * 4 + 2],
        });
    }
}

pub fn main() !void {
    const width: i32 = 640;
    const height: i32 = 480;

    const cam_angle_x = 0.04258603374866164;
    const cam_angle_y = 0.0;
    const cam_dist = 5.5;
    const focus_x = 0.0;
    const focus_y = 1.25;
    const focus_z = 0.0;

    const stdout = std.io.getStdOut().writer();

    try stdout.print("Rendering scene at {d}x{d}...\n", .{ width, height });

    tracy.render_init(width, height, cam_angle_x, cam_angle_y, cam_dist, focus_x, focus_y, focus_z);

    var i: usize = 0;
    while (i < 10) : (i += 1) {
        const buffer_ptr = tracy.render_refine(5);
        const buffer_len: usize = @intCast(width * height * 4);
        const buffer = buffer_ptr[0..buffer_len];

        try stdout.print("Step {d}/10: Saving to 'render_zig.ppm'...\n", .{i + 1});

        try saveImageAsPpm("render_zig.ppm", buffer, width, height);
    }

    try stdout.print("Done.\n", .{});
}
