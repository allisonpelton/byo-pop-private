#!/usr/bin/env bash
# Build origrun (the host) and its sound stand-in (dsound.dll), 32-bit, with llvm-mingw's i686 target.
# The originals are 32-bit, so the stand-in must be 32-bit to load into them; the host is built 32-bit too,
# for one toolchain. Output goes to build/origrun-bin/ (git-ignored). Run from anywhere.
#
#   tools/origrun/build.sh
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
out="$repo/build/origrun-bin"
mkdir -p "$out"

# Find llvm-mingw's i686 clang under E:/retools (the folder name carries a date).
tc="$(ls -d /e/retools/llvm-mingw-*-ucrt-x86_64 2>/dev/null | sort | tail -1 || true)"
[ -n "$tc" ] || { echo "llvm-mingw not found under E:/retools" >&2; exit 1; }
CC="$tc/bin/i686-w64-mingw32-clang.exe"
[ -x "$CC" ] || { echo "compiler not found: $CC" >&2; exit 1; }
echo "toolchain: $tc"

CFLAGS="-O2 -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function"

echo "building dsound.dll ..."
"$CC" $CFLAGS -shared -o "$out/dsound.dll" "$here/dsound_proxy.c" "$here/dsound_proxy.def" \
  -Wl,--enable-stdcall-fixup -lole32

echo "building origrun.exe ..."
"$CC" $CFLAGS -o "$out/origrun.exe" "$here/origrun.c" \
  -lole32 -loleaut32 -luuid -lgdi32 -luser32 -ladvapi32 -lshell32 -lbcrypt -lnetapi32 -lwtsapi32 -luserenv -liphlpapi

cp -f "$here/games.tsv" "$out/games.tsv"
echo "built:"
ls -la "$out"/origrun.exe "$out"/dsound.dll
