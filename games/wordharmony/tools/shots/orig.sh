#!/bin/bash
# orig.sh <tour> [timeout s] [extra origrun arguments]: the original on a tour, inside Windows Sandbox (the only place
# origrun takes shots), on the virtual clock with the scripted cursor and phase one's patches (identical/setup.sh); shots
# to build/ph2/<tour>/orig/. The run's folder is build/origrun/whsb-<tour>, a fresh copy of the staged game; the sandbox
# starts with an empty registry, and REG=<file.reg> presets values there (a saved game: identical/savedgame.py). The game
# draws into a DirectDraw back buffer that PrintWindow never sees there, so origrun reads it from memory: the copy notes the
# buffer (identical/setup.sh, "the screen") and the shot waits until the frame (0x4542e0) is not drawing. origrun is started directly, not under rundesk (it puts the game there). GS=<seed> sets the game's seed (default 1).
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-180}; shift; [ $# -gt 0 ] && shift
D=build/ph2/$N; R=build/origrun/whsb-$N
mkdir -p "$D"
rm -rf "$R"; mkdir -p "$R"; cp -r "$OG_GAME" "$R/game"
python games/wordharmony/tools/shots/make.py games/wordharmony/tools/shots/$N.txt "$D" orig > "$D/orig.txt"
REGARG=(); [ -z "$REG" ] || REGARG=(--reg "$(win "$PWD/$REG")")
"$ORIGRUN" --exe "$(win "$PWD/$R/game/WordHarmonyGame.exe")" --run "whsb-$N" --script "$(win "$PWD/$D/orig.txt")" \
	--sandbox --clock virtual --cursor scripted --counter 0x4aa600 --update-ms 33 --seed ${GS:-1} --watch-max 1000000 \
	--patches tools/origrun/patches/wordharmony.txt --surface-screen 0x4aa604,0x4aa608,800,600,32 --surface-eip 4542e0,45440d "${REGARG[@]}" --timeout "$T" "$@" > "$D/orig.log" 2>&1
rm -rf "$D/orig"; mkdir -p "$D/orig"; cp "$R"/shots/*.png "$D/orig/" 2>/dev/null
grep -E "RESULT|SOUND LEAK|ERROR|refused" "$D/orig.log" | tail -5; echo "$(ls "$D/orig" | wc -l) shots"
