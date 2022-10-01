const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    const flags = [_][]const u8{
        "-Wall",
        "-Wextra",
        "-Werror=return-type",
        //"-Wnonportable-include-path",
        //"-Wunknown-pragmas",
    };

    const cflags = flags ++ [_][]const u8{
        "-std=c99",
    };

    const cxxflags = cflags ++ [_][]const u8{
        "-std=c++17",
    };

    const buildfiles = [_][]const u8{
        "cd.cc",
        "cmd.cc",
        "common.cc",
        "crc16_gsm.cc",
        "crc32.cc",
        "drive.cc",
        "ecc_edc.cc",
        "endian.cc",
        "file_io.cc",
        "hex_bin.cc",
        "image_browser.cc",
        "iso9660.cc",
        "logger.cc",
        "main.cc",
        "md5.cc",
        "options.cc",
        "redumper.cc",
        "scrambler.cc",
        "scsi.cc",
        "sha1.cc",
        "split.cc",
        "subcode.cc",
        "toc.cc",
        "version.cc",
    };

    const exe = b.addExecutable("redumper", null);
    exe.setTarget(target);
    exe.setBuildMode(mode);

    //exe.defineCMacro("_CRT_SECURE_NO_WARNINGS", null);
    //exe.defineCMacro("NOMINMAX", null);
    exe.defineCMacro("_NTSCSI_USER_MODE_", null);
    //exe.defineCMacro("_WIN32", null);

    exe.addCSourceFiles(&buildfiles, &cxxflags);

    // fmt
    exe.addIncludeDir("fmt/include");
    exe.defineCMacro("FMT_HEADER_ONLY", null);

    //exe.linkLibC();
    exe.linkLibCpp();
    exe.addIncludeDir("E:/tools/zig-windows-x86_64-0.10.0-dev.3475+b3d463c9e/lib/libc/include/any-windows-any");
    exe.addIncludeDir("E:/tools/zig-windows-x86_64-0.10.0-dev.3475+b3d463c9e/lib/libc/include/any-windows-any/ddk");
    //exe.addIncludeDir("c:/program files (x86)/windows kits/10/include/10.0.18362.0/um");
    //exe.addIncludeDir("c:/program files (x86)/windows kits/10/include/10.0.18362.0/shared");
    //exe.addIncludeDir("c:/program files (x86)/windows kits/10/include/10.0.19041.0/um");
    //exe.addIncludeDir("c:/program files (x86)/windows kits/10/include/10.0.19041.0/shared");

    exe.install();

    const run_cmd = exe.run();
    run_cmd.addArg("-h");

    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    //const exe_tests = b.addTest("src/main.zig");
    //exe_tests.setTarget(target);
    //exe_tests.setBuildMode(mode);

    //const test_step = b.step("test", "Run unit tests");
    //test_step.dependOn(&exe_tests.step);
}
