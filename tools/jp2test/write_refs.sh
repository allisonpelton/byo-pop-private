#!/bin/bash
# Writes jp2test's reference PNGs: build/native/jp2test.exe's own decode of every .jp2 in assets/bookwormadv/images,
# into re/bookwormadv/jp2ref/images (git-ignored), where tools/check.sh unit compares against them. Run it after a
# deliberate change to the JPEG 2000 decoder (third_party/openjpeg, platform/openjpeg), once the change is checked:
# from then on the unit suite holds the decoder to that output.
#   tools/jp2test/write_refs.sh      (build jp2test first: cmake --build build/native --target jp2test)
cd "$(dirname "$0")/../.." || exit 1
W=$(pwd -W 2>/dev/null || pwd)
[ -d assets/bookwormadv/images ] || { echo "no assets/bookwormadv/images (tools/setup_worktree.sh)"; exit 1; }
[ -x build/native/jp2test.exe ] || { echo "no build/native/jp2test.exe"; exit 1; }
rm -rf re/bookwormadv/jp2ref/images
mkdir -p re/bookwormadv/jp2ref/images
(cd build/native && ./jp2test.exe "$W/assets/bookwormadv/images" "-ref=$W/re/bookwormadv/jp2ref/images" -write-ref)
