#!/bin/bash
# run.sh <what> ...: coverage runs of the original (docs/COVERAGE.md): tools/identical/orig.sh with origrun's --coverage on
# the list that `python -I tools/coverage.py list bookworm` writes. Each run waits for a free game slot (at most 6 game
# processes on the machine), and its hits go to build/coverage/bookworm/hits/<name>.txt, its log beside them. The original
# is staged by `OG_STAGE=C:/byo-og/bwp tools/identical/setup.sh` (set OG_STAGE for every run, the same one).
#   run.sh orig <name> <origrun script> <seed> [timeout s]      a script of origrun commands, on the virtual clock
#   run.sh case <name> <mode> <bot seed> <game seed> [cap]      the port's bot plays <mode> (classic, action, both or a list;
#                                                              cap: it quits at that update, saving the game); its input
#                                                              is the original's script, as identical/case.sh makes it
#   run.sh save <name> <mode> <bot seed> <game seed> <cap>      a game quit at update <cap> (saved as the window closes), then
#                                                              the bot continuing the port's saved game on the original:
#                                                              two runs, <name>-1 and <name>-2 (identical/save.sh's way)
#                                                              (ONLY1=1: the first session alone)
#   run.sh extra <name> <mode> <bot seed> <game seed> <cap> <extras file>
#                                                              the port's bot plays up to update <cap>, then the lines of
#                                                              tools/coverage/extras/<extras file>.txt run on top of its input
#                                                              ("<update> <command>" in the shifted timeline, "+<n> <command>"
#                                                              n updates after the cap; screenshot lines name a shot, taken only
#                                                              in the port's run of the same input, which is made first; PORTONLY=1 stops after it, NOPORT=1 leaves it out)
#   run.sh tour <name> <tour> <seed> [timeout s]                a tour of tools/coverage/tours or tools/shots (shots left out, as on every host run)
# KEEP_SAVE=1 and ORIG_SAVE=<dir> are orig.sh's: a saved game for a continued game.
source "$(dirname "$0")/../identical/common.sh"
H=build/coverage/bookworm/hits; mkdir -p "$H" build/coverage/bookworm/in
export COVERAGE="$(win "$PWD/build/coverage/bookworm/functions.txt")"
R="build/origrun/$(basename "$OG_STAGE")"
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done; }
# the stage's exe may still be running a moment after origrun has reported its run (a copy removed under it fails)
stage_free() { until python -I games/bookworm/tools/coverage/stagefree.py "$(basename "$OG_STAGE")"; do sleep 3; done; }
orig() {  # <name> <origrun script> <seed> <timeout>
	local N="$1" S="$2" SEED="$3" T="${4:-300}"
	slot
	stage_free
	rm -f "$R/coverage.txt"
	bash "$TOOLS/orig.sh" "$S" "$SEED" "$T" > "$H/$N.log" 2>&1
	cp "$R/coverage.txt" "$H/$N.txt" || { echo "$N: no hits file"; return 1; }
	echo "$N: $(grep -vc '^#' "$H/$N.txt") functions; $(grep '^#' "$H/$N.txt" | tail -1); $(grep -E 'RESULT|SOUND LEAK' "$H/$N.log" | tail -1)"
}
what="$1"; shift
case "$what" in
orig) orig "$@" ;;
case)
	N="$1"; MODE="$2"; BS="$3"; GS="$4"; CAP="$5"; D=build/coverage/bookworm/in/$N; mkdir -p "$D"
	slot
	bash "$TOOLS/port.sh" "$D/bot" -seed=$GS -autoplay=$BS -autoplaymode=$MODE ${CAP:+-autoplaycap=$CAP} -autoplaylog="$(win "$D/bot.txt")" > /dev/null
	awk -v d="${SHIFT:-600}" '/^[0-9]/ { $1 += d } { print }' "$D/bot.txt" > "$D/input.txt"
	[ -n "${PORTONLY:-}" ] && exit 0
	python "$TOOLS/toorig.py" "$D/input.txt" > "$D/orig.txt"
	LAST=$(grep -v "^#" "$D/input.txt" | tail -1 | cut -d' ' -f1)
	orig "$N" "$D/orig.txt" "$GS" $((LAST / 150 + 90)) ;;
save)
	N="$1"; MODE="$2"; BS="$3"; GS="$4"; CAP="$5"; D=build/coverage/bookworm/in/$N; mkdir -p "$D"; SH="${SHIFT:-600}"
	slot
	bash "$TOOLS/port.sh" "$D/bot1" -seed=$GS -autoplay=$BS -autoplaymode=$MODE -autoplaycap=$CAP -autoplaylog="$(win "$D/bot1.txt")" > /dev/null
	awk -v d="$SH" '/^[0-9]/ { $1 += d } { print }' "$D/bot1.txt" > "$D/input1.txt"
	slot
	bash "$TOOLS/port.sh" "$D/port1" -seed=$GS -drawevery -script="$(win "$D/input1.txt")" > /dev/null
	python "$TOOLS/toorig.py" "$D/input1.txt" > "$D/orig1.txt"
	LAST=$(grep -v "^#" "$D/input1.txt" | tail -1 | cut -d' ' -f1)
	orig "$N-1" "$D/orig1.txt" "$GS" $((LAST / 150 + 90))
	[ -n "${ONLY1:-}" ] && exit 0
	rm -rf "$D/save-port"; mkdir -p "$D/save-port"; cp -r "$D/port1/user/players" "$D/save-port/" || exit 1
	slot
	PORT_SAVE="$D/save-port" bash "$TOOLS/port.sh" "$D/bot2" -seed=$GS -autoplay=$((BS + 1)) -autoplaymode=$MODE -autoplaycontinue=1 		-autoplaylog="$(win "$D/bot2.txt")" > /dev/null
	awk -v d="$SH" '/^[0-9]/ { $1 += d } { print }' "$D/bot2.txt" > "$D/input2.txt"
	python "$TOOLS/toorig.py" "$D/input2.txt" > "$D/orig2.txt"
	LAST=$(grep -v "^#" "$D/input2.txt" | tail -1 | cut -d' ' -f1)
	ORIG_SAVE="$D/save-port" orig "$N-2" "$D/orig2.txt" "$GS" $((LAST / 150 + 90)) ;;
extra)
	N="$1"; MODE="$2"; BS="$3"; GS="$4"; CAP="$5"; X=games/bookworm/tools/coverage/extras/$6.txt; D=build/coverage/bookworm/in/$N
	SH="${SHIFT:-600}"; rm -rf "$D"; mkdir -p "$D/shots"
	slot
	bash "$TOOLS/port.sh" "$D/bot" -seed=$GS -autoplay=$BS -autoplaymode=$MODE -autoplaycap=$CAP -autoplaylog="$(win "$D/bot.txt")" > /dev/null
	python games/bookworm/tools/coverage/extra.py "$D/bot.txt" "$X" $SH $CAP "$(win "$PWD/$D/shots")" port > "$D/input.txt"
	python games/bookworm/tools/coverage/extra.py "$D/bot.txt" "$X" $SH $CAP "$(win "$PWD/$D/shots")" orig > "$D/input-orig.txt"
	if [ -z "${NOPORT:-}" ]; then
		slot
		bash "$TOOLS/port.sh" "$D/portx" -seed=$GS -drawevery -script="$(win "$D/input.txt")" > /dev/null
	fi
	python "$TOOLS/toorig.py" "$D/input-orig.txt" > "$D/orig.txt"
	LAST=$(grep -v "^#" "$D/input-orig.txt" | tail -1 | cut -d' ' -f1)
	orig "$N" "$D/orig.txt" "$GS" $((LAST / 150 + 90)) ;;
tour)
	N="$1"; TOUR="$2"; SEED="$3"; T="${4:-300}"; D=build/coverage/bookworm/in/$N; mkdir -p "$D"
	F=games/bookworm/tools/coverage/tours/$TOUR.txt; [ -f "$F" ] || F=games/bookworm/tools/shots/$TOUR.txt
	python games/bookworm/tools/shots/make.py "$F" "$D" host > "$D/orig.txt"
	orig "$N" "$D/orig.txt" "$SEED" "$T" ;;
*) echo "run.sh: orig, case, save or tour"; exit 2 ;;
esac
