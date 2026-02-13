const std = @import("std");
const testing = std.testing;
const exr_utils = @import("exr_utils");
const rmse = @import("rmse.zig");

test "RMSE test" {
    //const ally = testing.allocator;
    // const ref_fp = "../../img/exr/reference.exr";
    // const target_fp = "../../../render_c.exr";

    //const score = rmse.compareAndLog(ally, ref_fp, target_fp);
    //std.debug.print(score, .{});

    // TODO DELETE and uncomment above
    // 1. Initialize a seed (using the current time)
    const seed: u64 = @intCast(std.time.timestamp());

    // 2. Create the Pseudo-Random Number Generator (PRNG)
    var prng = std.Random.DefaultPrng.init(seed);

    // 3. Get the Random interface from the PRNG
    const random = prng.random();

    // 4. Generate a float between 0.0 and 1.0
    // float(f32) or float(f64) both work
    const my_rand = random.float(f32);

    std.debug.print("{d:.4}", .{my_rand});
}
