#!/usr/bin/env bash
# build.sh [name]: the counting build (games/typershark/docs/LEAKS.md): build/ts-leak/b/typershark.exe, then a copy of it and SDL2.dll in
# build/ts-leak/<name>/ (default dbg; runall.sh runs the exe of that folder). Not part of any other build; TS_LEAKCOUNT is off by default.
cd "$(dirname "$0")/../../../.."
export PATH="${APPDATA:-}/Python/Python312/Scripts:$PATH"
SDL=${SDL2_DIR:-E:/retools/SDL2-2.32.10/i686-w64-mingw32/lib/cmake/SDL2}
cmake -S . -B build/ts-leak/b -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-i686.cmake -DSDL2_DIR=$SDL -DTS_LEAKCOUNT=ON "-DCMAKE_CXX_FLAGS=-fno-omit-frame-pointer" "-DCMAKE_C_FLAGS=-fno-omit-frame-pointer" > build/ts-leak/configure.log 2>&1
cmake --build build/ts-leak/b --target typershark > build/ts-leak/build.log 2>&1
tail -n 1 build/ts-leak/build.log
NAME=${1:-dbg}
mkdir -p build/ts-leak/$NAME
cp build/ts-leak/b/typershark.exe build/ts-leak/b/SDL2.dll build/ts-leak/$NAME/
echo "counting build in build/ts-leak/$NAME"
