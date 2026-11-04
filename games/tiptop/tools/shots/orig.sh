#!/bin/bash
# orig.sh <tour> [timeout s] [extra origrun arguments]: the original on a tour, inside Windows Sandbox (the only place
# origrun takes shots), on the virtual clock with the scripted cursor and phase one's patches (identical/setup.sh);
# shots to build/ph2/<tour>/orig/. The run's folder is build/origrun/ttsb-<tour>, a copy of the staged game without its
# preferences and saved game (SAVE=<file> puts a saved game there). PrintWindow sees none of DirectDraw's picture in the
# sandbox, so a shot reads the frame from memory: the screen Blit (0x430190) last locked (setup.sh's patch keeps its pointer and
# pitch), taken when the game is outside Frame (0x41b9fe), which draws it. Input is posted only while the game is outside
# the main loop's input reads (0x412780-0x41289c, through its call of Frame) and Frame, so a click and the cursor's move
# both reach the next pass's start, as the port's script does. GS=<seed> sets the game's seed (default 1).
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-120}; shift; [ $# -gt 0 ] && shift
D=build/ph2/$N; R=build/origrun/ttsb-$N
mkdir -p "$D"
rm -rf "$R"; mkdir -p "$R"; cp -r "$OG_GAME" "$R/game"
rm -f "$R/game/Preferences.dat" "$R/game/savegame.dat"
[ -z "$SAVE" ] || cp "$SAVE" "$R/game/savegame.dat"
python games/tiptop/tools/shots/make.py games/tiptop/tools/shots/$N.txt "$D" orig > "$D/orig.txt"
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$PWD/$R/game/TipTop.exe")" --run "ttsb-$N" --script "$(win "$PWD/$D/orig.txt")" \
	--sandbox --clock virtual --cursor scripted --counter 0x48cff0 --update-ms 10 --seed ${GS:-1} \
	--patches tools/origrun/patches/tiptop.txt --surface-screen 0x48cfe0,0x48cfe4,640,480,32 --surface-eip 41b9fe,41bbce \
	--input-eip 412780,41289c,41b9fe,41bbce --timeout "$T" "$@" > "$D/orig.log" 2>&1
rm -rf "$D/orig"; mkdir -p "$D/orig"; cp "$R"/shots/*.png "$D/orig/" 2>/dev/null
grep -E "RESULT|SOUND LEAK" "$D/orig.log" | tail -5; echo "$(ls "$D/orig" | wc -l) shots"
