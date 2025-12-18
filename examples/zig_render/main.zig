const std = @import("std");
const tracy = @cImport({
    @cInclude("tracy.h");
});

/// Writes the image buffer to a PPM (P3 ASCII) file.
fn saveImageAsPpm(file_path: []const u8, image_buffer: []u8, width: i32, height: i32) !void {
    // 1. Create the file
    const file = try std.fs.cwd().createFile(file_path, .{});
    defer file.close();

    // 2. In v15, writers require an explicit buffer provided at the call site.
    // This reduces hidden allocations and makes I/O performance explicit.
    var write_buffer: [8192]u8 = undefined;
    var file_writer = file.writer(&write_buffer);
    const writer = &file_writer.interface;

    // 3. Write data using the interface
    try writer.print("P3\n{d} {d}\n255\n", .{ width, height });

    const num_pixels: usize = @intCast(width * height);
    for (0..num_pixels) |i| {
        try writer.print("{d} {d} {d}\n", .{
            image_buffer[i * 4 + 0],
            image_buffer[i * 4 + 1],
            image_buffer[i * 4 + 2],
        });
    }

    // 4. IMPORTANT: You must flush the writer in v15 to ensure the buffer
    // is actually written to the disk.
    try writer.flush();
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

    // 5. Modern way to get Stdout in v15
    var stdout_buffer: [1024]u8 = undefined;
    var stdout_file_writer = std.fs.File.stdout().writer(&stdout_buffer);
    const stdout = &stdout_file_writer.interface;

    try stdout.print("Rendering scene at {d}x{d} (Zig v15)...\n", .{ width, height });
    try stdout.flush();

    tracy.render_init(width, height, cam_angle_x, cam_angle_y, cam_dist, focus_x, focus_y, focus_z);

    var i: usize = 0;
    while (i < 10) : (i += 1) {
        const buffer_ptr = tracy.render_refine(5);
        const buffer_len: usize = @intCast(width * height * 4);
        const buffer = buffer_ptr[0..buffer_len];

        try stdout.print("Step {d}/10: Saving to 'render_zig.ppm'...\n", .{i + 1});
        try stdout.flush();

        try saveImageAsPpm("render_zig.ppm", buffer, width, height);
    }

    try stdout.writeAll("Done.\n");
    try stdout.flush();
}
