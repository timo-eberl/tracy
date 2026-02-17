const std = @import("std");
const tracy = @cImport({
    @cInclude("tracy.h");
});

fn saveImageAsTga(file_path: []const u8, image_buffer: []u8, width: i32, height: i32) !void {
    const file = try std.fs.cwd().createFile(file_path, .{});
    defer file.close();

    var bw = std.io.bufferedWriter(file.writer());
    const writer = bw.writer();

    // 18-byte TGA Header for 32-bit uncompressed RGBA
    var header = [_]u8{0} ** 18;
    header[2] = 2; // True-color
    header[12] = @intCast(width & 0xFF);
    header[13] = @intCast((width >> 8) & 0xFF);
    header[14] = @intCast(height & 0xFF);
    header[15] = @intCast((height >> 8) & 0xFF);
    header[16] = 32; // Bits per pixel
    header[17] = 0x20; // Top-to-bottom flag

    try writer.writeAll(&header);

    const num_pixels: usize = @intCast(width * height);
    for (0..num_pixels) |i| {
        // TGA expects BGRA, buffer is RGBA
        try writer.writeByte(image_buffer[i * 4 + 2]); // B
        try writer.writeByte(image_buffer[i * 4 + 1]); // G
        try writer.writeByte(image_buffer[i * 4 + 0]); // R
        try writer.writeByte(image_buffer[i * 4 + 3]); // A
    }

    try bw.flush();
}

pub fn main() !void {
    const width: i32 = 640;
    const height: i32 = 480;

    const filter_type = 0;

    const cam_angle_x = 0.04258603374866164;
    const cam_angle_y = 0.0;
    const cam_dist = 5.5;
    const focus_x = 0.0;
    const focus_y = 1.25;
    const focus_z = 0.0;

    const stdout = std.io.getStdOut().writer();

    try stdout.print("Rendering scene at {d}x{d}...\n", .{ width, height });

    tracy.render_init(width, height, filter_type, cam_angle_x, cam_angle_y, cam_dist, focus_x, focus_y, focus_z);

    var i: usize = 0;
    while (i < 10) : (i += 1) {
        tracy.render_refine(5);
        const buffer_ptr = tracy.update_image_ldr();
        const buffer_len: usize = @intCast(width * height * 4);
        const buffer = buffer_ptr[0..buffer_len];

        try stdout.print("Step {d}/10: Saving to 'render_zig.tga'...\n", .{i + 1});

        try saveImageAsTga("render_zig.tga", buffer, width, height);
    }

    try stdout.print("Done.\n", .{});
}
