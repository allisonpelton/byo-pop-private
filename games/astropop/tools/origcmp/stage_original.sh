#!/bin/bash
# Stages the original for comparison runs: a copy of the install under build/origrun/<run>/game (origrun --copy-only), the
# carved game image from re/astropop/bin (the install's exe is a launcher wrapper, docs/ORIGRUN.md), no players, and the three
# palette PNGs of fonts/ turned into the same pixels as RGBA PNGs.
#   stage_original.sh [run]        (default run name: apc)
# Why the PNGs: on this machine (Windows 11, build 26300) the original's loading thread stops at its task 203 of 206 with an
# unhandled exception, every time, while it loads the fonts: a libpng warning is printed and the heap manager then reports a
# fault (an access violation in ntdll, as in the crash log of the Windows Sandbox runs). With the three palette PNGs
# (fonts/_Kiloton9.png, _Fusion16.png, _NeuropolX30outline.png: all the palette PNGs the game has) stored as RGBA the load
# completes and the game runs on. The pixels the game decodes are the same, so play and the screens do not change
# (games/astropop/docs/GAME_NOTES.md, "Oddities").
run=${1:-apc}
cd "$(dirname "$0")/../../../.." || exit 2
dst=build/origrun/$run/game
build/origrun-bin/origrun.exe --game astropop --run "$run" --copy-only > build/logs/ap/stage_$run.log 2>&1 || { tail -5 build/logs/ap/stage_$run.log; exit 1; }
cp re/astropop/bin/AstroPopGame.exe "$dst/AstroPopGame.exe" || exit 1
rm -f "$dst"/AstroPopGame.exe.origrun-unpatched "$dst"/users/* "$dst"/crash.txt
python -I - "$dst/fonts" <<'PY'
import sys
from PIL import Image
for n in ('_Kiloton9.png', '_Fusion16.png', '_NeuropolX30outline.png'):
    p = sys.argv[1] + '/' + n
    im = Image.open(p)
    if im.mode == 'P':
        im.convert('RGBA').save(p)
PY
echo "staged $dst"
