#!/bin/bash
# sound_orig.sh <tour> [timeout s]: the original on a tour on the host (no shots; origrun puts it under rundesk), virtual
# clock, scripted cursor, with origrun's recording stand-in (--sound wav: every sample play is a duplicate of a loaded
# DirectSound buffer, logged with its virtual time). The log goes to build/ph2/<tour>/orig_sound.log. GS=<seed>.
source "$(dirname "$0")/../identical/common.sh"
N=$1; T=${2:-300}; D=build/ph2/$N; R=build/origrun/bwsd-$N
mkdir -p "$D"
if [ ! -f "$R/game/BookwormGame.exe" ]; then
	mkdir -p "$R" && cp -r "$OG_MASTER" "$R/game" || exit 1
fi
rm -rf "$R/game/players" "$R/logs/sound.log" "$R/game/basslog.txt"
# The music: BASS's calls logged by games/bigmoney/tools/bassproxy (built into build/bassproxy-bw/bass.dll) in place of the
# copy's bass.dll, which becomes bass_real.dll; the log is copied to orig_bass.log.
[ -f "$R/game/bass_real.dll" ] || mv "$R/game/bass.dll" "$R/game/bass_real.dll"
cp "${BASSPROXY:-build/bassproxy-bw/bass.dll}" "$R/game/bass.dll" || exit 1
python games/bookworm/tools/shots/make.py games/bookworm/tools/shots/$N.txt "$D" host > "$D/orig_host.txt"
"$ORIGRUN" --exe "$(win "$PWD/$R/game/BookwormGame.exe")" --run "bwsd-$N" --script "$(win "$PWD/$D/orig_host.txt")" \
	--clock virtual --cursor scripted --counter "$COUNTER" --update-ms 10 --seed ${GS:-1} \
	--patches tools/origrun/patches/bookworm.txt --sound wav --timeout "$T" > "$D/orig_sound_run.log" 2>&1
cp "$R/logs/sound.log" "$D/orig_sound.log" 2>/dev/null; cp "$R/game/basslog.txt" "$D/orig_bass.log" 2>/dev/null
grep -E "RESULT|SOUND LEAK" "$D/orig_sound_run.log" | tail -3; wc -l "$D/orig_sound.log"
