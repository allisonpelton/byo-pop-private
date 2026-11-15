#!/usr/bin/env python3
"""Builds a small, static SDL 2.32.10 for the releases' one-file exes (docs/PUBLISHING.md, "The small SDL"; a release's README.md, "Building from source").

    python third_party/sdl-small/build.py <SDL2-2.32.10 source folder> <install folder> [--build <scratch folder>]

The source is SDL's own release tarball, unmodified (SDL2-2.32.10.tar.gz from libsdl.org). The result is SDL2's CMake package
in <install folder>/lib/cmake/SDL2 (target SDL2::SDL2-static, plus SDL2::SDL2main): a game's build points SDL2_DIR at it and
links SDL into its exe. Needs cmake and ninja on PATH and llvm-mingw (LLVM_MINGW_ROOT, as cmake/llvm-mingw-i686.cmake does).

What is kept is what the ports call: video (the Windows driver, and the dummy one that SDL_VIDEODRIVER=dummy names), the
renderers (Direct3D 11, which the framework asks for, then Direct3D 9, OpenGL and the software renderer for the machines
where it will not start; not Direct3D 12, which no machine needs that 11 does not serve), audio (WASAPI, DirectSound, and the dummy driver that SDL_AUDIODRIVER=dummy names, which the test
runs use), events, threads, timers, atomics, file I/O, the base and pref paths, the clipboard and OpenURL. What is left out is
what no port touches: joysticks, haptics, sensors, HIDAPI, XInput, power, Vulkan, OpenGL ES, the offscreen and disk-writer
drivers and the test library. SDL_Init is never called (the framework starts the video and audio subsystems it needs one at
a time), so none of the left-out subsystems can make it fail. The code is built at -O3 in sections, without unwind tables,
and with SDL's dynamic API off: that table names every SDL function, so the linker could drop none. SDL makes
you edit its source to force it off, but also turns it off when SDL_THREAD_SAFETY_ANALYSIS is defined, which only adds
clang attributes to the mutex functions, so the source stays as released.
"""
import argparse
import os
import shutil
import subprocess
import sys

OFF = ("SDL_SHARED SDL_TEST SDL_TESTS SDL_JOYSTICK SDL_HAPTIC SDL_SENSOR SDL_HIDAPI SDL_POWER SDL_XINPUT SDL_VULKAN "
       "SDL_OPENGLES SDL_OFFSCREEN SDL_DISKAUDIO SDL_CCACHE SDL_RPATH").split()
ON = "SDL_STATIC SDL_DIRECTX SDL_WASAPI SDL_RENDER_D3D SDL_OPENGL SDL_DUMMYAUDIO SDL_DUMMYVIDEO SDL_ASSEMBLY".split()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source")
    ap.add_argument("install")
    ap.add_argument("--build", help="scratch folder for the build (default: <install>-build, deleted afterwards)")
    a = ap.parse_args()
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    toolchain = os.path.join(root, "cmake", "llvm-mingw-i686.cmake")
    if not os.path.isfile(os.path.join(a.source, "CMakeLists.txt")) or not os.path.isdir(os.path.join(a.source, "src", "render")):
        sys.exit(f"{a.source} is not an SDL source folder")
    build = a.build or os.path.abspath(a.install.rstrip("/\\")) + "-build"
    flags = "-O3 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -DSDL_THREAD_SAFETY_ANALYSIS"
    cmd = ["cmake", "-S", a.source, "-B", build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
           f"-DCMAKE_TOOLCHAIN_FILE={toolchain}", f"-DCMAKE_INSTALL_PREFIX={a.install}",
           f"-DCMAKE_C_FLAGS_RELEASE={flags} -DNDEBUG"]
    cmd += [f"-D{n}=OFF" for n in OFF] + [f"-D{n}=ON" for n in ON]
    # no Direct3D 12 renderer (SDL tries it after 11): its header test is answered here, so SDL does not build the driver
    cmd += ["-DHAVE_D3D12_H=OFF"]
    for step in (cmd, ["cmake", "--build", build], ["cmake", "--install", build]):
        print("+", " ".join(step), flush=True)
        if subprocess.run(step).returncode:
            sys.exit("the SDL build failed")
    if not a.build:
        shutil.rmtree(build, ignore_errors=True)
    print(f"small SDL installed in {a.install}: point SDL2_DIR at {os.path.join(a.install, 'lib', 'cmake', 'SDL2')}")


if __name__ == "__main__":
    main()
