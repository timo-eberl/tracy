const std = @import("std");

pub fn main() !void {
    const width: u16 = 640;
    const height: u16 = 480;

    // Open file
    const file = try std.fs.cwd().createFile("black.tga", .{});
    defer file.close();

    var bw = std.io.bufferedWriter(file.writer());
    const writer = bw.writer();

    // 1. Write TGA Header (18 bytes)
    // Little-endian format for u16 values
    const header = [_]u8{
        0, // ID Length (0)
        0, // Color Map Type (0 = No Palette)
        2, // Image Type (2 = Uncompressed TrueColor)
        0, 0, 0, 0, 0, // Color Map Specification (Ignored)
        0, 0, 0, 0, // X Origin, Y Origin (0,0)
        @truncate(width), @truncate(width >> 8), // Width (Low byte, High byte)
        @truncate(height), @truncate(height >> 8), // Height (Low byte, High byte)
        24, // Pixel Depth (24-bit RGB)
        0, // Image Descriptor (0 = Bottom-Left origin)
    };
    try writer.writeAll(&header);

    // 2. Write Pixel Data (Black = 0, 0, 0)
    // Total size = 640 * 480 * 3 bytes
    const total_bytes = @as(usize, width) * @as(usize, height) * 3;

    // Optimization: Write a static buffer of zeros repeatedly
    // to avoid allocating ~900KB of heap memory.
    const chunk_size = 1024 * 4; // 4KB chunks
    const zeros = [_]u8{0} ** chunk_size;

    var remaining = total_bytes;
    while (remaining > 0) {
        const to_write = @min(remaining, chunk_size);
        try writer.writeAll(zeros[0..to_write]);
        remaining -= to_write;
    }

    try bw.flush();
    std.debug.print("Generated black.tga (640x480)\n", .{});
}
