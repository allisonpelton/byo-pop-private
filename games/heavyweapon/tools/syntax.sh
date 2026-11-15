#!/bin/bash
# Checks game sources compile, without building: the heavyweapon target's own compiler and flags with -fsyntax-only.
#   bash games/heavyweapon/tools/syntax.sh games/heavyweapon/src/Board.cpp ...
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
CXX=E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang++.exe
for f in "$@"; do
  "$CXX" -fsyntax-only -ferror-limit=50 -I"$ROOT/games/heavyweapon/src" -I"$ROOT" -I"$ROOT/framework" \
    -I"$ROOT/framework/compat2004" -I"$ROOT/engines/swf" -I"$ROOT/third_party/stb" -isystem E:/retools/SDL2-2.32.10/i686-w64-mingw32/include/SDL2 \
    -std=gnu++17 -Wall -Wno-overloaded-virtual -Wno-unused-private-field -fwrapv -msse2 -mfpmath=sse "$f"
done
