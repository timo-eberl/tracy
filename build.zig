const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // --- Library Setup ---
    const tracy_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    tracy_mod.addCSourceFile(.{ .file = b.path("src/tracy.c"), .flags = &.{"-std=c11"} });
    tracy_mod.addIncludePath(b.path("include"));

    const lib = b.addLibrary(.{
        .linkage = .static,
        .name = "tracy",
        .root_module = tracy_mod,
    });
    lib.linkSystemLibrary("m");
    b.installArtifact(lib);

    // --- Test Setup ---
    const test_mod = b.createModule(.{
        .root_source_file = b.path("tests/all_tests.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });

    // The test module needs access to the C source/headers for @cImport
    test_mod.addIncludePath(b.path("include"));
    test_mod.addIncludePath(b.path("src"));

    const tests = b.addTest(.{
        .root_module = test_mod,
    });

    // Link math library for the test runner
    tests.linkSystemLibrary("m");

    const run_tests = b.addRunArtifact(tests);
    const test_step = b.step("test", "Run all tests");
    test_step.dependOn(&run_tests.step);
}
