#!/bin/bash
# modeswitch.sh <name> [game args...]: Ancient Japan's port (-fast, FullScreen=0, a 1:1 window) driven by -realinput through
# the changes of display mode: resize to 2x and back, minimize and restore, Alt+Enter to full screen and back; a screenshot
# and a screenshotfull at each stage. Run under rundesk: build/native/rundesk.exe bash games/mahjongjapan/tools/display/modeswitch.sh <name> ...
cd "$(git rev-parse --show-toplevel)" || exit 1
R="$(pwd -W)"; N="$1"; shift
GAME="${GAME:-mahjongjapan}"
d="build/modeswitch/$N"; rm -rf "$d"; mkdir -p "$d/user"
printf '; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\JollyBear\Mahjong Escape Ancient Japan]\nFullScreen=dword:0\n' > "$d/user/registry.ini"
if [ "$GAME" = mahjongchina ]; then sed -i 's/Mahjong Escape Ancient Japan/Mahjong Escape/' "$d/user/registry.ini"; fi
S="$R/$d"
{
  u=3000; G=${GAP:-200}
  # SHOTGAP: updates between a stage's screenshot and the next change of the window (40; in real time 250, so that the window
  # capture, 0.7 s after the screenshot, is taken before the next change)
  SG=${SHOTGAP:-40}; if [ -n "${REAL:-}" ]; then SG=${SHOTGAP:-250}; fi
  shot() { echo "$u screenshot $S/$1.png"; echo "$u screenshotfull $S/$1-full.png"; u=$((u+SG)); }
  shot s0_windowed_1to1
  echo "$u resize 1600 1200"; u=$((u+G)); shot s1_resized_2x
  echo "$u resize 800 600"; u=$((u+G)); shot s2_back_1to1
  echo "$u focus lost"; echo "$u minimize"; u=$((u+G))
  echo "$u restore"; echo "$u focus gained"; u=$((u+G)); shot s3_restored
  echo "$u keydown Left Alt"; echo "$u key Return"; echo "$u keyup Left Alt"; u=$((u+G+100)); shot s4_fullscreen
  echo "$u keydown Left Alt"; echo "$u key Return"; echo "$u keyup Left Alt"; u=$((u+G+100)); shot s5_windowed_again
  echo "$u resize 1600 1200"; u=$((u+G)); echo "$u resize 800 600"; u=$((u+G)); shot s6_after_second_round
  echo "$u quit"
} > "$d/input.txt"
: > "$d/commands.txt"
FAST=-fast
if [ -n "${REAL:-}" ]; then
  # real time: the window is captured 0.7 s after each stage's screenshot and compared with it (the stages are 1 s or more apart)
  FAST=
  (for st in s1_resized_2x s2_back_1to1 s3_restored s5_windowed_again s6_after_second_round; do
    python -I games/mahjongjapan/tools/display/wincap.py "Mahjong Escape" "$S/$st.png" "$d/cap-$st.txt" > "$d/cap-$st.out" 2>&1
  done) &
fi
build/native/$GAME.exe -gamedir="$R/assets/$GAME" -userdir="$S/user" -realinput="$S/input.txt" $FAST -windowscale=1 "$@" > "$d/out.txt" 2>&1
echo "exit $?"
wait
for f in "$d"/cap-*.out; do [ -f "$f" ] && echo "$(basename "$f" .out): $(cat "$f")"; done
grep -h "SurfaceDisplay" "$d/out.txt" | cut -c1-200
