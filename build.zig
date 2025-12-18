const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // 1. THE LIBRARY MODULE
    const tracy_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    tracy_mod.addCSourceFile(.{ .file = b.path("src/tracy.c"), .flags = &.{"-std=c11"} });
    tracy_mod.addIncludePath(b.path("include"));

    // 2. THE STATIC LIBRARY ARTIFACT
    const lib = b.addLibrary(.{
        .linkage = .static,
        .name = "tracy",
        .root_module = tracy_mod,
    });
    lib.linkSystemLibrary("m");
    b.installArtifact(lib);

    // --- EXAMPLE 1: C RENDERER ---
    const c_exe = b.addExecutable(.{
        .name = "render-c",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    // Add the C example source file to its root_module
    c_exe.root_module.addCSourceFile(.{ .file = b.path("examples/c_render/main.c") });
    c_exe.root_module.addIncludePath(b.path("include"));

    // Link the library we defined above
    c_exe.linkLibrary(lib);

    b.installArtifact(c_exe);

    const run_c = b.addRunArtifact(c_exe);
    b.step("run-c", "Run the C example").dependOn(&run_c.step);

    // --- EXAMPLE 2: ZIG RENDERER ---
    const zig_exe = b.addExecutable(.{
        .name = "render-zig",
        .root_module = b.createModule(.{
            .root_source_file = b.path("examples/zig_render/main.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    zig_exe.root_module.addIncludePath(b.path("include"));
    zig_exe.linkLibrary(lib); // Link the static library artifact

    // We still link the library artifact to ensure all C symbols are present
    zig_exe.linkLibrary(lib);

    b.installArtifact(zig_exe);

    const run_zig = b.addRunArtifact(zig_exe);
    b.step("run-zig", "Run the Zig example").dependOn(&run_zig.step);

    // --- UNIT TESTS ---
    const test_mod = b.createModule(.{
        .root_source_file = b.path("tests/all_tests.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    test_mod.addIncludePath(b.path("include"));
    test_mod.addIncludePath(b.path("src"));

    const tests = b.addTest(.{ .root_module = test_mod });
    tests.linkSystemLibrary("m");

    const run_tests = b.addRunArtifact(tests);
    b.step("test", "Run all tests").dependOn(&run_tests.step);
}
