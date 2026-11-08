#!/bin/bash
# Builds the originals' side of the music levels, build/musiclevel/musiclevel.exe (32-bit, for the games' bass.dll and fmod.dll)
set -e
cd "$(dirname "$0")/../.."
CC="${CC:-E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-gcc}"
mkdir -p build/musiclevel
"$CC" -O1 -Wall -o build/musiclevel/musiclevel.exe tools/musiclevel/musiclevel.c
echo "built build/musiclevel/musiclevel.exe"
