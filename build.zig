const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addLibrary(.{
        .name = "waks_cstd",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
        .linkage = .static,
    });

    lib.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "main.c",
        },
        .flags = &.{ "-std=c11", "-ffreestanding" },
    });

    lib.root_module.addIncludePath(b.path("include"));

    b.installArtifact(lib);
}
