#!/bin/bash
# sound.sh <tour> [timeout s]: the sounds of a tour on both sides. The original runs on the host (no shots), on the
# virtual clock, with origrun's recording sound stand-in (--sound wav, its log copied to build/ph2/<tour>/orig_sound.log);
# the port runs with -soundlog (port_sound.log). soundcmp.py compares them. GS=<seed> as orig.sh; WATCH="--watch ..."
# adds origrun's watches. The window is not given the focus (LOWOS plays sounds only for a window in front, which the
# patch list's PlaySound line lifts): on the host, a "focus regain" at the start made the original's tour 2 game go
# another way from update 790 (its draws came 5 updates late), while without it every draw comes at the port's update.
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-120}; P=tiptop.txt
# The music plays with its MIDI calls skipped (setup.sh's tiptop-music.txt), and its state is watched; MUSIC=0 leaves
# it off, as in the other runs
[ "$MUSIC" = 0 ] || { P=tiptop-music.txt; WATCH="--watch 0x48a098 --watch 0x489f08 --watch 0x47e298"; }
D=build/ph2/$N; R=build/origrun/ttsd-$N
mkdir -p "$D"
rm -rf "$R"; mkdir -p "$R"; cp -r "$OG_GAME" "$R/game"
rm -f "$R/game/Preferences.dat" "$R/game/savegame.dat"
# No shots (origrun refuses them on the host), but the last one becomes a move to where the mouse already is, so the
# script lasts to the tour's end before it posts the close, after which the original plays nothing
python games/tiptop/tools/shots/make.py games/tiptop/tools/shots/$N.txt "$D" orig | awk '
	/ shot / { last = $2; next }
	$3 == "click" || $3 == "move" { x = $4; y = $5 }
	/^at / { print; next }
	{ if (last != "" && x != "") print "at " last " move " x " " y; last = ""; print }' > "$D/orig_sound.txt"
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$PWD/$R/game/TipTop.exe")" --run "ttsd-$N" --script "$(win "$PWD/$D/orig_sound.txt")" \
	--clock virtual --cursor scripted --counter 0x48cff0 --update-ms 10 --seed ${GS:-1} --patches tools/origrun/patches/$P \
	--input-eip 412780,41289c,41b9fe,41bbce ${WATCH:+$WATCH} --sound wav --timeout "$T" > "$D/orig_sound_run.log" 2>&1
cp "$R/logs/sound.log" "$D/orig_sound.log"
grep -E "RESULT|SOUND LEAK" "$D/orig_sound_run.log" | tail -3
python games/tiptop/tools/shots/make.py games/tiptop/tools/shots/$N.txt "$D" port | grep -v " screenshot " > "$D/port_sound.txt"
rm -rf "$D/sound_user"; mkdir -p "$D/sound_user"
"$RUNDESK" "$(win "$PORT_EXE")" -gamedir="$(win assets/tiptop)" -userdir="$(win "$D/sound_user")" -fast -seed=${GS:-1} \
	-script="$(win "$D/port_sound.txt")" -soundlog="$(win "$D/port_sound.log")" > /dev/null 2>&1
wc -l "$D/orig_sound.log" "$D/port_sound.log"
