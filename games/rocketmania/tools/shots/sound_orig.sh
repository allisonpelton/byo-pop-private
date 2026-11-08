#!/bin/bash
# sound_orig.sh <tour stem> [timeout s]: the original on a tour on the host (rundesk, no shots), virtual clock, with origrun's recording stand-in
# (--sound wav; BASS's DirectSound is the stand-in's). The sound log is copied to build/ph2/<tour>/orig_sound.log.
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-90}; D=build/ph2/$N; R=build/origrun/rmsd-$N
mkdir -p "$D"
if [ ! -f "$R/game/RocketManiaGame.exe" ]; then
	"$ORIGRUN" --game rocketmania --run "rmsd-$N" --copy-only > /dev/null || exit 1
	cp re/rocketmania/bin/RocketManiaGame.exe "$R/game/RocketManiaGame.exe"
fi
python games/rocketmania/tools/shots/make.py games/rocketmania/tools/shots/$N.txt "$D" host > "$D/orig_host.txt"
# PROXY=1: the logging BASS (tools/bassproxy, built to /tmp/bass_proxy.dll or BASSPROXY): the game's own bass.dll becomes bass_real.dll.
if [ -n "$PROXY" ]; then
	[ -f "$R/game/bass_real.dll" ] || mv "$R/game/bass.dll" "$R/game/bass_real.dll"
	cp "${BASSPROXY:-/tmp/bass_proxy.dll}" "$R/game/bass.dll"; rm -f "$R/game/basslog.txt"
elif [ -f "$R/game/bass_real.dll" ]; then
	mv -f "$R/game/bass_real.dll" "$R/game/bass.dll"
fi
rm -f "$R/logs/sound.log" "$R"/game/Players/hiscores.dat "$R"/game/Players/save?.dat
"$(win "$ORIGRUN")" --exe "$(win "$PWD/$R/game/RocketManiaGame.exe")" --run "rmsd-$N" --script "$(win "$PWD/$D/orig_host.txt")" \
	--clock ${CLOCK:-virtual} --cursor scripted --counter $COUNTER --update-ms 10 --seed ${GS:-1} --patches tools/origrun/patches/rocketmania.txt --sound wav --timeout "$T" > "$D/orig_sound_run.log" 2>&1
cp "$R/logs/sound.log" "$D/orig_sound.log" 2>/dev/null; cp "$R/game/basslog.txt" "$D/orig_bass.log" 2>/dev/null
grep -E "RESULT|SOUND LEAK" "$D/orig_sound_run.log" | tail -3; wc -l "$D/orig_sound.log"
