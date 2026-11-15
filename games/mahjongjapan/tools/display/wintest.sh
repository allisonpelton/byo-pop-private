#!/bin/bash
# wintest.sh <name> [game args...]: Ancient Japan's port to the stage's board in real time, its window captured and compared with
# the game's own screenshot of the same frame. Run it under rundesk: build/native/rundesk.exe bash games/mahjongjapan/tools/display/wintest.sh <name> ...
cd "$(git rev-parse --show-toplevel)" || exit 1
R="$(pwd -W)"; N="$1"; shift
d="build/wintest/$N"; rm -rf "$d"; mkdir -p "$d/user"
printf '; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\JollyBear\Mahjong Escape Ancient Japan]\nFullScreen=dword:%s\n' "${FULL:-0}" > "$d/user/registry.ini"
head -n 9 build/logs/check/mahjongjapan/game/script.txt | sed "s|build/logs/check/mahjongjapan/game/shots/board.png|$R/$d/board.png|" > "$d/script.txt"
echo "${QUITAT:-4200} quit" >> "$d/script.txt"
python -I games/mahjongjapan/tools/display/wincap.py "Mahjong Escape" "$R/$d/board.png" "$d/cap.txt" > "$d/cap.out" 2>&1 &
build/native/mahjongjapan.exe -gamedir="$R/assets/mahjongjapan" -userdir="$R/$d/user" -logos -script="$R/$d/script.txt" "$@" > "$d/out.txt" 2>&1
wait
cat "$d/cap.out"
grep -h "SurfaceDisplay" "$d/out.txt" | cut -c1-160
