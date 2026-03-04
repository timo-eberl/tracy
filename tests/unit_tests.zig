const c = @cImport({
    @cInclude("../src/tracy.c");
});

// Zig's translate-c engine struggles to translate the C99 designated initializers in combinations
// with C unions used for scene_cornell and scene_caustics, so it falls back to declaring them as
// extern variables. However, it successfully translates all_scenes, which references them. This
// results in undefined symbol errors from the linker.
//
// To fix this, we export dummy zero-length arrays to satisfy the linker. This is safe because our
// unit tests only test isolated math/geometry and never actually access these scenes at runtime.
export var scene_cornell: [0]c.Primitive = undefined;
export var scene_caustics: [0]c.Primitive = undefined;
export var scene_glass_sphere: [0]c.Primitive = undefined;

comptime {
    _ = @import("unit/vec_test.zig");
    _ = @import("unit/intersect_sphere_test.zig");
    _ = @import("unit/intersect_triangle_test.zig");
    _ = @import("unit/refract_test.zig");
    _ = @import("unit/fresnel_test.zig");
}
