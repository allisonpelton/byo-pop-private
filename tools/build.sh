#!/usr/bin/env bash
# Configure and build byo-pop: one CMake build with every game's targets (bejeweled, rulestest, ...).
#   tools/build.sh native       # Windows exes (llvm-mingw, i686) in build/native
#   tools/build.sh web          # WebAssembly + HTML shell in build/web
#   tools/build.sh all          # native and web (the default)
#   tools/build.sh rundesk      # only build/native/rundesk.exe (tools/check.sh, before it runs under it)
#   tools/build.sh novariants   # build/native-novariants: Bejeweled without its code-mod variant paths
#                               # (BEJ_NO_VARIANTS), a second candidate for its oracle; no tests or tools
#   tools/build.sh web-test     # build/web-test: a second web build for automated browser tests (served
#                               # on port 18766 by .claude/launch.json), so they never touch build/web
# Run it from anywhere; the game files it needs at run time are staged separately (assets/<game>/).
# Options given once to cmake (e.g. -DBUILD_SMOKE=ON) stay in that build folder's cache.
# Tool locations can be overridden with LLVM_MINGW_ROOT, SDL2_DIR, EMSDK_ROOT.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDL2_DIR="${SDL2_DIR:-E:/retools/SDL2-2.32.10/i686-w64-mingw32/lib/cmake/SDL2}"
EMSDK_ROOT="${EMSDK_ROOT:-E:/retools/emsdk}"
export PATH="${APPDATA:-}/Python/Python312/Scripts:$PATH"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# A build folder configured from another checkout (the repository was moved or renamed) cannot be reconfigured:
# drop its CMake cache, so CMake configures it afresh. Object files stay and are rebuilt as needed.
stale_cache() {  # <build folder>
	[ -f "$1/CMakeCache.txt" ] || return 0
	local home here
	home="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$1/CMakeCache.txt" | tr -d '\r')"
	here="$(cd "$ROOT" && pwd -W)"
	if [ -n "$home" ] && [ "${home,,}" != "${here,,}" ]; then
		echo "build.sh: $1 was configured from $home; removing its CMake cache"
		rm -rf "$1/CMakeCache.txt" "$1/CMakeFiles"
	fi
}

native() {  # <build folder> [cmake options]; TARGET=<name> builds only that target
	local dir="$ROOT/build/$1"; shift
	stale_cache "$dir"
	cmake -S "$ROOT" -B "$dir" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
		-DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/llvm-mingw-i686.cmake" -DSDL2_DIR="$SDL2_DIR" "$@"
	cmake --build "$dir" ${TARGET:+--target "$TARGET"}
}

web() {  # <build folder> [cmake options]
	local dir="$ROOT/build/$1"; shift
	stale_cache "$dir"
	"$EMSDK_ROOT/upstream/emscripten/emcmake.exe" cmake -S "$ROOT" -B "$dir" -G Ninja \
		-DCMAKE_BUILD_TYPE="$BUILD_TYPE" "$@"
	cmake --build "$dir"
}

case "${1:-all}" in
	native) native native ;;
	rundesk) TARGET=rundesk native native ;;
	web) web web ;;
	all) native native; web web ;;
	novariants) native native-novariants -DBEJ_NO_VARIANTS=ON -DBUILD_FWTEST=OFF -DBUILD_RULESTEST=OFF \
		-DBUILD_SMOKE=OFF -DBUILD_LSPROBE=OFF -DBUILD_MODSETTINGSTEST=OFF ;;
	web-test) web web-test -DBUILD_FWTEST=OFF -DBUILD_SMOKE=OFF ;;
	*) echo "usage: $0 [native|rundesk|web|all|novariants|web-test]"; exit 2 ;;
esac
