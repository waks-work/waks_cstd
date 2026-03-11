const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    //lib.root_module.addCSourceFiles({
    lib_mod.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{"main.c"},
        .flags = &.{"-std=c11"}, // "-ffreestanding"
    });
    lib_mod.addIncludePath(b.path("include"));

    const lib = b.addLibrary(.{
        .name = "waks_cstd",
        .root_module = lib_mod,
        .linkage = .static,
    });
    b.installArtifact(lib);

    const unit_tests = b.addExecutable(.{
        .name = "arena_test",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .strip = true,
        }),
    });

    unit_tests.root_module.addCSourceFiles(.{
        .root = b.path("tests"),
        .files = &.{"arena_test.c"},
        .flags = &.{"-std=c11"}, // "-ffreestanding"
    });
    unit_tests.root_module.addIncludePath(b.path("include"));
    // unit_tests.linkLibrary(lib);
    unit_tests.root_module.linkLibrary(lib);
    unit_tests.root_module.sanitize_c = .off;
    unit_tests.root_module.stack_check = false;

    const run_unit_test = b.addRunArtifact(unit_tests);

    const test_step = b.step("test", "Run library tests");
    test_step.dependOn(&run_unit_test.step);

    // lib.linkLibC();
    // unit_tests.linkLibC();
}
