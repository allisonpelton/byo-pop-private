#!/bin/bash
# sound.sh <tour> [port|orig|cmp]...: a tour's sound in both programs (games/dynomite/docs/IDENTICAL_PLAY.md). "orig" plays
# the tour's script (build/ph2/<tour>/script.txt, made by tour.sh) in the original on the host (rundesk, no shots: origrun
# takes none there), on the virtual clock with origrun's recording stand-in (--sound wav) and Big Money's logging BASS
# (games/bigmoney/tools/bassproxy, built to build/bassproxy/bass.dll; the copy's own BASS becomes bass_real.dll), with
# the window given the focus first (a SexyApp game that never had the focus plays no sound). Its DirectSound log goes to
# build/ph2/<tour>/orig_sound.log and the BASS calls to orig_bass.log. "port" plays it in the port with BYOPOP_SOUNDLOG
# (port_sound.log, the samples and the music); "cmp" runs soundcmp.py. With no step it does all three.
source "$(dirname "$0")/../identical/common.sh"
S=games/dynomite/tools/shots
N=$1; shift
STEPS="${*:-port orig cmp}"
D=build/ph2/$N
[ -f "$D/script.txt" ] || { echo "run tour.sh $N port first"; exit 1; }
SEED=${SEED:-$(grep "^$N " $S/tours.txt | cut -d' ' -f2)}	# a recorded tour (tour.sh SCRIPT=) gives SEED=
LAST=$(tail -1 "$D/script.txt" | cut -d' ' -f1)
for step in $STEPS; do
case $step in
port)
	mkdir -p "$D/sound"; grep -v screenshot "$D/script.txt" > "$D/sound/script.txt"
	BYOPOP_SOUNDLOG="$(win "$PWD/$D/port_sound.log")" HS=1 $TOOLS/port.sh "$D/sound" -seed=$SEED \
		-script="$(win "$D/sound/script.txt")" | tail -1 ;;
orig)
	RUN=dysd-$N; RD=build/origrun/$RUN
	if [ ! -f "$RD/game/DynomiteGame.exe" ]; then
		"$ORIGRUN" --game dynomite --run "$RUN" --copy-only > /dev/null || exit 1
		cp re/dynomite/bin/DynomiteGame.exe "$RD/game/DynomiteGame.exe"
	fi
	if [ -f build/bassproxy/bass.dll ]; then
		[ -f "$RD/game/bass_real.dll" ] || mv "$RD/game/bass.dll" "$RD/game/bass_real.dll"
		cp build/bassproxy/bass.dll "$RD/game/bass.dll"; rm -f "$RD/game/basslog.txt"
	fi
	rm -f "$RD/logs/sound.log" "$RD"/game/*.HS "$RD/game/STOMPED.TMP" "$RD"/game/SLOT*.SAV
	python $TOOLS/mkhs.py "$RD/game"
	[ -z "$SAVE" ] || cp "$SAVE" "$RD/game/SLOT0.SAV"
	reg delete 'HKCU\Software\PopCap\Dynomite' /f > /dev/null 2>&1
	reg import "$(win "$TOOLS/settings.reg")" > /dev/null 2>&1
	{ echo "at 200 focus regain"; python $TOOLS/toorig.py "$D/script.txt" -1; } > "$D/orig_host.txt"
	"$(win "$ORIGRUN")" --exe "$(win "$PWD/$RD/game/DynomiteGame.exe")" --run "$RUN" --script "$(win "$PWD/$D/orig_host.txt")" \
		--clock virtual --cursor scripted --counter 0x530960 --update-ms 10 --seed $SEED --patches tools/origrun/patches/dynomite.txt \
		--sound wav --timeout $((LAST / 1000 + 120)) > "$D/orig_sound_run.log" 2>&1
	cp "$RD/logs/sound.log" "$D/orig_sound.log" 2>/dev/null; cp "$RD/game/basslog.txt" "$D/orig_bass.log" 2>/dev/null
	grep -E "RESULT|SOUND LEAK" "$D/orig_sound_run.log" | tail -3; wc -l "$D/orig_sound.log" "$D/orig_bass.log" 2>/dev/null ;;
cmp)
	python $S/soundcmp.py "$N" ;;
esac
done
