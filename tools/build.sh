#!/usr/bin/env bash
# Configure and build the port.
#   tools/build.sh native     # Windows exe (llvm-mingw, i686) in build/native
#   tools/build.sh web        # WebAssembly + HTML shell in build/web
#   tools/build.sh all
# Tool locations can be overridden with LLVM_MINGW_ROOT, SDL2_DIR, EMSDK_ROOT.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDL2_DIR="${SDL2_DIR:-E:/retools/SDL2-2.32.10/i686-w64-mingw32/lib/cmake/SDL2}"
EMSDK_ROOT="${EMSDK_ROOT:-E:/retools/emsdk}"
export PATH="${APPDATA:-}/Python/Python312/Scripts:$PATH"
BUILD_TYPE="${BUILD_TYPE:-Release}"

native() {
	cmake -S "$ROOT" -B "$ROOT/build/native" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
		-DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/llvm-mingw-i686.cmake" -DSDL2_DIR="$SDL2_DIR"
	cmake --build "$ROOT/build/native"
}

web() {
	"$EMSDK_ROOT/upstream/emscripten/emcmake.exe" cmake -S "$ROOT" -B "$ROOT/build/web" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
	cmake --build "$ROOT/build/web"
}

case "${1:-all}" in
	native) native ;;
	web) web ;;
	all) native; web ;;
	*) echo "usage: $0 [native|web|all]"; exit 2 ;;
esac
