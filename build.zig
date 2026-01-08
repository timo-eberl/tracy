const std = @import("std");

pub fn build(b: *std.Build) void {
    const native_target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Disable LTO for Debug builds
    const use_lto = optimize != .Debug;

    // PCG Configuration
    const pcg_include = b.path("dependencies/pcg-c/include");
    const include_dir = b.path("include");
    const pcg_sources = [_][]const u8{
        "dependencies/pcg-c/src/pcg-advance-128.c",
        "dependencies/pcg-c/src/pcg-advance-16.c",
        "dependencies/pcg-c/src/pcg-advance-32.c",
        "dependencies/pcg-c/src/pcg-advance-64.c",
        "dependencies/pcg-c/src/pcg-advance-8.c",
        "dependencies/pcg-c/src/pcg-global-32.c",
        "dependencies/pcg-c/src/pcg-global-64.c",
        "dependencies/pcg-c/src/pcg-output-128.c",
        "dependencies/pcg-c/src/pcg-output-16.c",
        "dependencies/pcg-c/src/pcg-output-32.c",
        "dependencies/pcg-c/src/pcg-output-64.c",
        "dependencies/pcg-c/src/pcg-output-8.c",
        "dependencies/pcg-c/src/pcg-rngs-128.c",
        "dependencies/pcg-c/src/pcg-rngs-16.c",
        "dependencies/pcg-c/src/pcg-rngs-32.c",
        "dependencies/pcg-c/src/pcg-rngs-64.c",
        "dependencies/pcg-c/src/pcg-rngs-8.c",
    };

    // --- THE LIBRARY ---
    // We keep this module/artifact to produce 'libtracy.a' for external users,
    // even though our own examples bypass it for performance.
    const tracy_mod = b.createModule(.{
        .target = native_target,
        .optimize = optimize,
        .link_libc = true,
    });
    tracy_mod.addCSourceFile(.{ .file = b.path("src/tracy.c"), .flags = &.{"-std=c11"} });
    tracy_mod.addIncludePath(b.path("include"));
    tracy_mod.addIncludePath(pcg_include);
    for (pcg_sources) |src| tracy_mod.addCSourceFile(.{ .file = b.path(src) });
    const lib = b.addLibrary(.{
        .linkage = .static,
        .name = "tracy",
        .root_module = tracy_mod,
    });
    lib.linkSystemLibrary("m");
    b.installArtifact(lib);

    // --- PERFORMANCE NOTE ---
    // For the executables below, we compile 'tracy.c' directly into the binary
    // (a "Unity Build" approach) and enable LTO.
    //
    // If we linked 'libtracy.a' (the artifact above), the compiler would optimize 'tracy.c' in isolation.
    // Compiling sources together allows Whole-Program Optimization, matching the
    // performance of a raw 'clang main.c tracy.c' command.

    // --- EXAMPLE 1: C RENDERER ---
    const c_exe = b.addExecutable(.{
        .name = "render-c",
        .root_module = b.createModule(.{
            .target = native_target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    c_exe.want_lto = use_lto;
    c_exe.root_module.addCSourceFile(.{ .file = b.path("examples/c_render/main.c") });
    c_exe.root_module.addCSourceFile(.{ .file = b.path("src/tracy.c"), .flags = &.{"-std=c11"} });
    c_exe.root_module.addIncludePath(b.path("include"));
    c_exe.root_module.addIncludePath(pcg_include);
    for (pcg_sources) |src| c_exe.root_module.addCSourceFile(.{ .file = b.path(src) });
    c_exe.linkSystemLibrary("m");
    b.installArtifact(c_exe);
    const run_c = b.addRunArtifact(c_exe);
    b.step("run-c", "Run the C example").dependOn(&run_c.step);

    // --- EXAMPLE 2: ZIG RENDERER ---
    const zig_exe = b.addExecutable(.{
        .name = "render-zig",
        .root_module = b.createModule(.{
            .root_source_file = b.path("examples/zig_render/main.zig"),
            .target = native_target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    zig_exe.want_lto = use_lto;
    zig_exe.root_module.addCSourceFile(.{ .file = b.path("src/tracy.c"), .flags = &.{"-std=c11"} });
    zig_exe.root_module.addIncludePath(b.path("include"));
    zig_exe.root_module.addIncludePath(pcg_include);
    for (pcg_sources) |src| zig_exe.root_module.addCSourceFile(.{ .file = b.path(src) });
    zig_exe.linkSystemLibrary("m");
    b.installArtifact(zig_exe);
    const run_zig = b.addRunArtifact(zig_exe);
    b.step("run-zig", "Run the Zig example").dependOn(&run_zig.step);

    // --- EXAMPLE 3: WEB ASSEMBLY ---``
    const build_web = b.option(bool, "build-web", "Build the WebAssembly target") orelse false;
    if (build_web) {

        // compile using zig-integrated clang
        const web_step = b.step("web", "Build for Web (WASM)");
        const wasm_target = b.resolveTargetQuery(.{
            .cpu_arch = .wasm32,
            .os_tag = .emscripten,
        });
        const wasm_mod = b.createModule(.{
            .link_libc = true,
            .optimize = optimize,
            .target = wasm_target,
        });

        wasm_mod.addCSourceFile(.{ .file = b.path("src/tracy.c"), .flags = &.{ "-std=c11", "-D__EMSCRIPTEN__" } });

        wasm_mod.addIncludePath(include_dir);
        wasm_mod.addIncludePath(pcg_include);

        for (pcg_sources) |src| {
            wasm_mod.addCSourceFile(.{ .file = b.path(src) });
        }

        // emscripten header
        if (b.option([]const u8, "emsdk", "Path to emsdk")) |emsdk_path| {
            const sysroot_include = b.fmt("{s}/upstream/emscripten/cache/sysroot/include", .{emsdk_path});
            wasm_mod.addSystemIncludePath(.{ .cwd_relative = sysroot_include });
        }
        const lib_wasm = b.addLibrary(.{
            .linkage = .static,
            .name = "tracy_wasm",
            .root_module = wasm_mod,
        });

        // link step using emcc
        const emcc_cmd = b.addSystemCommand(&[_][]const u8{"emcc"});

        emcc_cmd.addArtifactArg(lib_wasm);

        emcc_cmd.addArgs(&[_][]const u8{
            "-o",                "./examples/web/src/tracy_c.js",
            "-sModularize=1",    "-sEXPORT_ES6=1",
            "-sSHARED_MEMORY=1", "-sIMPORTED_MEMORY=1",
            "-sALLOW_MEMORY_GROWTH=1", // Good practice for WASM
            "--emit-tsd",
            "tracy_c.d.ts",
            "-sEXPORTED_FUNCTIONS=[\"_render_init\",\"_render_fast\",\"_render_refine\",\"_malloc\",\"_free\"]",
            "-Wall",
            "-Wextra",
        });
        // Optimization flags for Emscripten
        switch (optimize) {
            .Debug => emcc_cmd.addArgs(&[_][]const u8{ "-O0", "-g", "-gsource-map" }),
            else => emcc_cmd.addArgs(&[_][]const u8{"-O3"}),
        }

        web_step.dependOn(&emcc_cmd.step);
    }

    // --- UNIT TESTS ---
    const test_mod = b.createModule(.{
        .root_source_file = b.path("tests/all_tests.zig"),
        .target = native_target,
        .optimize = optimize,
        .link_libc = true,
    });
    test_mod.addIncludePath(b.path("include"));
    test_mod.addIncludePath(b.path("src"));
    test_mod.addIncludePath(pcg_include);
    for (pcg_sources) |src| test_mod.addCSourceFile(.{ .file = b.path(src) });
    const tests = b.addTest(.{ .root_module = test_mod });
    tests.linkSystemLibrary("m");
    const run_tests = b.addRunArtifact(tests);
    b.step("test", "Run all tests").dependOn(&run_tests.step);
}
