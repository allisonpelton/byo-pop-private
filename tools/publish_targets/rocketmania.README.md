# byo-pop-rocketmania

The decompilation and native port of PopCap and Nuclide's Rocket Mania Deluxe (2003), from the
[byo-pop](https://github.com/allisonpelton/byo-pop) project. This repository is generated from byo-pop and is
read-only: it holds what is needed to build this one game and nothing else. Send changes, issues and questions to
https://github.com/allisonpelton/byo-pop.

The game is a translation of the original's game classes into C++ on a portable copy of PopCap's SexyAppFramework,
with SDL2 under it. It plays like the original exe.

## Your own game files

This repository holds none of the game's pictures, sounds, music or texts. They come from your own copy of Rocket Mania
Deluxe (for example the Steam release). Copy what the port needs into `assets/rocketmania/`:

    python games/rocketmania/tools/stage_rocketmania_assets.py "<your Rocket Mania Deluxe folder>"

The script also looks in Steam's usual library folders when you give it no folder. The game reads its texts from the
`RocketMania.exe` it copies. The built exe finds `assets/rocketmania/` next to itself or up to two folders above it, or
takes `-gamedir=<folder>`; `-userdir=<folder>` sets where it keeps players and saves.

## Building

The build makes a 32-bit Windows exe with llvm-mingw. You need CMake 3.20 or later, Ninja, an llvm-mingw
toolchain (i686 target) and the SDL2 development package for MinGW (2.32.x), whose `i686-w64-mingw32` folder holds
the CMake files. Point `LLVM_MINGW_ROOT` at the toolchain folder, then from this folder:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-i686.cmake \
      -DSDL2_DIR=<SDL2>/i686-w64-mingw32/lib/cmake/SDL2
    cmake --build build --target rocketmania

`build/rocketmania.exe` and the DLLs it needs (SDL2 and the llvm-mingw runtime) are in `build/`. The browser version
is built from byo-pop, which has the site around it.

## Notes

`games/rocketmania/docs/README.md` is the game's status and index. It points to the notes on the original's code, the
differences from the original and how the port was checked against it.
