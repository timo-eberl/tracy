const std = @import("std");
const testing = std.testing;
const exr_utils = @import("exr_utils");
const rmse = @import("rmse.zig");

const stdout = std.io.getStdOut().writer();

pub fn main() !void {
    // TODO DELETE and uncomment above
    // var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    // defer _ = gpa.deinit();
    // const allocator = gpa.allocator();
    //
    // const stdout = std.io.getStdOut().writer();
    //
    // // Pfade relativ zum Projekt-Root (CWD in der Pipeline)
    // const ref_fp = "tests/img/reference.exr";
    // const target_fp = "render_zig.exr";
    //
    // // Prüfen, ob die Dateien überhaupt existieren, bevor wir tinyexr bemühen
    // std.fs.cwd().access(ref_fp, .{}) catch {
    //     std.debug.print("Error: Reference image not found at {s}\n", .{ref_fp});
    //     std.process.exit(1);
    // };
    // std.fs.cwd().access(target_fp, .{}) catch {
    //     std.debug.print("Error: Rendered image not found at {s}\n", .{target_fp});
    //     std.process.exit(1);
    // };
    //
    // // Die eigentliche Berechnung
    // const score = try rmse.compareAndLog(allocator, ref_fp, target_fp);
    //
    // // NUR den Score ausgeben, damit das Python-Skript ihn einfach parsen kann
    // try stdout.print("{d:.6}", .{score});
    //

    // 1. Initialize a seed (using the current time)
    const seed: u64 = @intCast(std.time.timestamp());

    // 2. Create the Pseudo-Random Number Generator (PRNG)
    var prng = std.Random.DefaultPrng.init(seed);

    // 3. Get the Random interface from the PRNG
    const random = prng.random();

    // 4. Generate a float between 0.0 and 1.0
    // float(f32) or float(f64) both work
    const my_rand = random.float(f32);

    try stdout.print("{d:.4}", .{my_rand});
}
