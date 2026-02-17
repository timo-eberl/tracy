const std = @import("std");
const testing = std.testing;
const exr_utils = @import("exr_utils");
const rmse = @import("rmse.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const stdout = std.io.getStdOut().writer();

    const ref_fp = "tests/img/exr/reference.exr";
    const target_fp = "render_zig.exr";

    std.fs.cwd().access(ref_fp, .{}) catch {
        std.debug.print("Error: Reference image not found at {s}\n", .{ref_fp});
        std.process.exit(1);
    };
    std.fs.cwd().access(target_fp, .{}) catch {
        std.debug.print("Error: Rendered image not found at {s}\n", .{target_fp});
        std.process.exit(1);
    };

    const score = try rmse.compareAndLog(allocator, ref_fp, target_fp);
    //  needs to be printed``
    try stdout.print("{d:.6}", .{score});
}
