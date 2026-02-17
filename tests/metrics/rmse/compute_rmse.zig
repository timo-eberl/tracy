const std = @import("std");
const testing = std.testing;
const exr_utils = @import("exr_utils");
const rmse = @import("rmse.zig");

pub fn computeScore(target_fp: [:0]const u8) !f32 {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    //const stdout = std.io.getStdOut().writer();

    const ref_fp = "tests/img/exr/reference.exr";

    std.fs.cwd().access(ref_fp, .{}) catch {
        std.debug.print("Error: Reference image not found at {s}\n", .{ref_fp});
        std.process.exit(1);
    };
    std.fs.cwd().access(target_fp, .{}) catch {
        std.debug.print("Error: Rendered image not found at {s}\n", .{target_fp});
        std.process.exit(1);
    };

    // convert sentinel terminated slice to zig slice
    // const target_fp_slice: []const u8 = target_fp;

    const score = try rmse.compareAndLog(allocator, ref_fp, target_fp);

    //try stdout.print("{d:.6}", .{score});
    return score;
}
