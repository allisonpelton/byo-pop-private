#!/bin/bash
# sound_orig.sh <tour stem> [timeout s]: the original on a tour on the host (rundesk, no shots), virtual clock, with origrun's recording stand-in
# (--sound wav; BASS's DirectSound is the stand-in's since 2026-11-03). The sound log is copied to build/ph2/<tour>/orig_sound.log.
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-90}; D=build/ph2/$N; R=build/origrun/alsd-$N
mkdir -p "$D"
if [ ! -f "$R/game/AlchemyGame.exe" ]; then
	"$ORIGRUN" --game alchemy --run "alsd-$N" --copy-only > /dev/null || exit 1
	cp re/alchemy/bin/AlchemyGame.exe "$R/game/AlchemyGame.exe"
fi
python games/alchemy/tools/shots/make.py games/alchemy/tools/shots/$N.txt "$D" host > "$D/orig_host.txt"
# PROXY=1: the logging BASS (tools/bassproxy, built to /tmp/bass_proxy.dll or BASSPROXY): the game's own bass.dll becomes bass_real.dll.
# Alchemy's 2003 BASS refuses it (BASS_Init fails with error 23, "invalid device"), so the proxy is not used for the comparisons.
if [ -n "$PROXY" ]; then
	[ -f "$R/game/bass_real.dll" ] || mv "$R/game/bass.dll" "$R/game/bass_real.dll"
	cp "${BASSPROXY:-/tmp/bass_proxy.dll}" "$R/game/bass.dll"; rm -f "$R/game/basslog.txt"
elif [ -f "$R/game/bass_real.dll" ]; then
	mv -f "$R/game/bass_real.dll" "$R/game/bass.dll"
fi
rm -f "$R/logs/sound.log" "$R/game/savegame.dat" "$R/game/hiscores.dat"; [ -z "$SAVE" ] || cp "$SAVE" "$R/game/savegame.dat"
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$PWD/$R/game/AlchemyGame.exe")" --run "alsd-$N" --script "$(win "$PWD/$D/orig_host.txt")" \
	--clock virtual --cursor scripted --counter 0x516570 --update-ms 10 --seed ${GS:-1} --patches tools/origrun/patches/alchemy.txt --sound wav --timeout "$T" > "$D/orig_sound_run.log" 2>&1
cp "$R/logs/sound.log" "$D/orig_sound.log" 2>/dev/null; cp "$R/game/basslog.txt" "$D/orig_bass.log" 2>/dev/null
grep -E "RESULT|SOUND LEAK" "$D/orig_sound_run.log" | tail -3; wc -l "$D/orig_sound.log"
