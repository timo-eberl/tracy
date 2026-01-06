const std = @import("std");

// standard TGA header - 18 bytes
// packed to ensure binary compatability with the file format?
//
// We use 'extern struct' to guarantee C-compatible field ordering.
// We use 'align(1)' on fields to force byte-packing without padding.
//
// Without align(1), the compiler would insert a padding byte after
// 'image_type' (offset 2) to align 'color_map_origin' (offset 3)
// to a 2-byte boundary. This would bloat the struct size and break
// binary compatibility with the TGA file format on disk.
const TGAHeader = extern struct {
    id_length: u8, // length of optional text comment
    color_map_type: u8, // 0 - no palette (direct color), 1 - has palette
    image_type: u8, // 2 - uncompressed (true color), 3 - uncomprerssed (grayscale), 10 - RLE compressed (true color)
    color_map_origin: u16 align(1), // start index of color palette, not relevant if color map = 0,
    color_map_length: u16 align(1), // number of colors in palette
    color_map_depth: u8, // bits per color palette entry
    x_origin: u16 align(1), // coordinates of lower left corner
    y_origin: u16 align(1), // -||-
    width: u16 align(1),
    height: u16 align(1),
    pixel_depth: u8, // bits per pixel
    image_descriptor: u8, // bitmask containing further info, 0-3 -> alpha channel bits, 4 -> right2left flag, 5 -> top2bottom flag (for image orientation)
};
comptime {
    if (@sizeOf(TGAHeader) != 18) {
        @compileError("TGAHeader must be 18 bytes, but it is " ++ std.fmt.comptimePrint("{d}", .{@sizeOf(TGAHeader)}));
    }
}
pub const ImageF32 = struct {
    width: u16,
    height: u16,
    channels: u8,
    data: []f32,
    data_red: []f32,
    data_green: []f32,
    data_blue: []f32,
    allocator: std.mem.Allocator,
    pub fn deinit(self: ImageF32) void {
        self.allocator.free(self.data);
        self.allocator.free(self.data_red);
        self.allocator.free(self.data_green);
        self.allocator.free(self.data_blue);
    }
};

pub fn loadTgaF32(allocator: std.mem.Allocator, path: []const u8) !ImageF32 {
    const file = try std.fs.cwd().openFile(path, .{});
    defer file.close();

    var reader = file.reader();

    const header = try reader.readStruct(TGAHeader);
    if (header.image_type != 2) {
        std.debug.print("Unsupported TGA type: {d} (Only type 2 is supported)", .{header.image_type});
        return error.UnsupportedFormat;
    }
    if (header.id_length > 0) {
        try reader.skipBytes(header.id_length, .{});
        std.debug.print("Skipped header", .{});
    }
    // header debug info
    // std.debug.print("DEBUG TGA Header:\n", .{});
    // std.debug.print("- File Size on Disk: {d} bytes\n", .{file_size});
    // std.debug.print("- ID Length: {d}\n", .{header.id_length});
    // std.debug.print("- Color Map Type: {d}, Length: {d}\n", .{ header.color_map_type, header.color_map_length });
    // std.debug.print("- Image Type: {d}\n", .{header.image_type});
    // std.debug.print("- Size: {d}x{d} @ {d}bpp\n", .{ header.width, header.height, header.pixel_depth });

    const color_map_type = header.color_map_type;
    const color_map_len = header.color_map_length;
    const color_map_depth = header.color_map_depth;

    // 1 -> color map present
    if (color_map_type == 1 or color_map_len > 0) {
        //calc bytes per entry
        const bytes_per_entry = (color_map_depth + 7) / 8;
        const skip_size = @as(u64, color_map_len) * bytes_per_entry;

        try reader.skipBytes(skip_size, .{});
        std.debug.print("Skipped color palette of {d} bytes", .{skip_size});
    }
    const width = header.width;
    const height = header.height;
    const bpp = header.pixel_depth;
    // only support 24(RGB) and 32(RGBA) bit
    if (bpp != 24 and bpp != 32) {
        return error.UnsupportedBitDepth;
    }

    const channels_in_file: usize = if (bpp == 24) 3 else 4;

    const pixel_count: usize = @as(usize, width) * @as(usize, height);

    // allocate buffer f32
    const data = try allocator.alloc(f32, pixel_count * channels_in_file);
    const data_red = try allocator.alloc(f32, pixel_count);
    const data_green = try allocator.alloc(f32, pixel_count);
    const data_blue = try allocator.alloc(f32, pixel_count);

    errdefer allocator.free(data);
    errdefer allocator.free(data_red);
    errdefer allocator.free(data_green);
    errdefer allocator.free(data_blue);

    //std.debug.print("\nnum_channels: {d}, data length: {d}, pixel_count: {d}\n", .{ channels_in_file, data.len, pixel_count });

    var i: usize = 0;
    // read and convert pixels - TGA usually in BGR
    while (i < pixel_count) : (i += 1) {
        const blue = try reader.readByte();
        const green = try reader.readByte();
        const red = try reader.readByte();

        // offset in the float array
        const offset = i * channels_in_file;

        // convert u8 [0-255] to f32 [0.0-1.0]
        data[offset + 0] = @as(f32, @floatFromInt(red)) / 255.0;
        data[offset + 1] = @as(f32, @floatFromInt(green)) / 255.0;
        data[offset + 2] = @as(f32, @floatFromInt(blue)) / 255.0;

        // fill channel_specific data arrays
        data_red[i] = @as(f32, @floatFromInt(red)) / 255.0;
        data_green[i] = @as(f32, @floatFromInt(green)) / 255.0;
        data_blue[i] = @as(f32, @floatFromInt(blue)) / 255.0;

        // handle alpha if present
        if (channels_in_file == 4) {
            const alpha = try reader.readByte();
            data[offset + 3] = @as(f32, @floatFromInt(alpha)) / 255.0;
        }
    }
    return ImageF32{
        .allocator = allocator,
        .channels = @intCast(channels_in_file),
        .data = data,
        .data_red = data_red,
        .data_green = data_green,
        .data_blue = data_blue,
        .height = height,
        .width = width,
    };
}
