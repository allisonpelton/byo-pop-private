#!/bin/bash
# run.sh <command> ...: the original's runs for the coverage check (docs/COVERAGE.md), each an origrun --coverage run on
# the host, on the virtual clock with the scripted cursor, with phase one's patches and the music's (the original plays its
# music with the MIDI calls skipped, so the sequencer's code runs) and origrun's recording sound stand-in (--sound wav), so
# that LOWOS's sound code runs too. The hits go to build/coverage/tiptop/hits/<name>.txt, the run's log beside them. Each
# run is one game process: it waits for a free slot (at most 3 on the machine).
#   run.sh case <name> <mode> <bot seed> <game seed> <games> [port arguments]
#       the phase-one bot (the baseline of the phase-one port, whose title takes as long as the original's) plays a game
#       of the mode by mouse, and the original plays the same input
#   run.sh script <name> <origrun script> <game seed> <timeout s>
#       an origrun script (lines "at <counter> click <x> <y>", "key", "text", "focus", "wait", "quit")
#   run.sh tour <tour> <game seed> <timeout s>
#       a phase-two tour (tools/shots/<tour>.txt) as an origrun script without its shots
#   run.sh tourplus <name> <tour> <extra lines> <game seed> <timeout s>
#       the same, with the lines of <extra lines> (origrun script lines) after the tour's, before its quit
# CLOCK=real runs on the real clock (the counter patch still counts passes; a pass waits 10 ms) with the silent stand-in.
source "$(dirname "$0")/../identical/common.sh"
H=build/coverage/tiptop/hits; mkdir -p "$H"
EIP="--input-eip 412780,41289c,41b9fe,41bbce"
CMD="$1"; shift
case "$CMD" in
case)
	N="$1"; MODE="$2"; BS="$3"; GS="$4"; G="$5"; shift 5
	D="$H/case-$N"
	export PORT_EXE=build/baseline-tiptop-phase1/tiptop.exe
	until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 3; done
	"$ROOT/games/tiptop/tools/identical/port.sh" "$D" -seed=$GS -autoplay=$BS -autoplayui -autoplaymode=$MODE -autoplaygames=$G \
		-autoplaycap=80000 -autoplaylog="$D/log.txt" -autoplayscript="$D/script.txt" -statelog="$D/state.txt" "$@" > /dev/null
	python games/tiptop/tools/identical/toorig.py "$D/script.txt" -1 > "$D/orig.txt"
	LAST=$(tail -1 "$D/script.txt" | cut -d' ' -f1)
	T=$((LAST / 200 + 40)); [ $T -gt 600 ] && T=600
	S="$D/orig.txt"; SEED=$GS; EIP= ;;
script)
	N="$1"; S="$2"; SEED="$3"; T="$4" ;;
tour)
	N="tour-$1"; SEED="$2"; T="$3"; mkdir -p "$H/$N"
	python games/tiptop/tools/shots/make.py games/tiptop/tools/shots/$1.txt "$H/$N" orig | grep -v ' shot ' > "$H/$N/orig.txt"
	S="$H/$N/orig.txt" ;;
tourplus)
	N="$1"; SEED="$4"; T="$5"; mkdir -p "$H/$N"
	{ python games/tiptop/tools/shots/make.py games/tiptop/tools/shots/$2.txt "$H/$N" orig | grep -v ' shot ' | grep -vx 'wait 200\|quit'; cat "$3"; } > "$H/$N/orig.txt"
	S="$H/$N/orig.txt" ;;
*) echo "usage: run.sh case|script|tour|tourplus ..."; exit 2 ;;
esac
R="build/origrun/$(basename "$OG_STAGE")"
export COVERAGE_LIST="$(win "$PWD/build/coverage/tiptop/functions.txt")"
rm -f "$R/coverage.txt" "$OG_GAME/Preferences.dat" "$OG_GAME/savegame.dat"
[ -z "$SAVE" ] || cp "$SAVE" "$OG_GAME/savegame.dat"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 3; done
if [ "$CLOCK" = real ]; then CLK="--clock real"; else CLK="--clock virtual"; fi
"$RUNDESK" "$(win "$ORIGRUN")" --exe "$(win "$OG_GAME/TipTop.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$PWD/$S")" $CLK --cursor scripted --counter 0x48cff0 --update-ms 10 --seed "$SEED" \
	--patches tools/origrun/patches/tiptop-music.txt $EIP --sound wav \
	--coverage "$COVERAGE_LIST" --timeout "$T" > "$H/$N.log" 2>&1
cp "$R/coverage.txt" "$H/$N.txt" || { echo "$N: no hits file"; exit 1; }
echo "$N: $(grep -vc '^#' "$H/$N.txt") functions; $(grep '^#' "$H/$N.txt" | tail -1); $(grep -E 'RESULT|SOUND LEAK' "$H/$N.log" | tail -2 | tr '\n' ' ')"
