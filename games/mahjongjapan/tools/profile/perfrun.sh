#!/bin/bash
# perfrun.sh port|orig <runs> [out dir]: the measured runs of the performance pass (docs/COORDINATION.md), for Ancient
# Japan, or GAME=mahjongchina for Ancient China. The play is the one tools/check.sh sends (perfscript.py turns its script into
# the port's and the original's input); perfmin.py samples the game's process every 50 ms: working set, private bytes,
# processor time and threads, with the numbers of the window 20 s to 50 s of its life (the play).
#   build/native/rundesk.exe bash games/mahjongjapan/tools/profile/perfrun.sh port 3     the native port, real time (EXEDIR=<folder> for another build)
#   bash games/mahjongjapan/tools/profile/perfrun.sh orig 3                              the original, through origrun (which
#                                                                                         puts it on rundesk's desktop itself:
#                                                                                         never wrapped in rundesk)
# The original's saves are removed before each run, so each is a first run, and the ones found are put back at the end.
cd "$(git rev-parse --show-toplevel)" || exit 1
export MSYS_NO_PATHCONV=1
GAME="${GAME:-mahjongjapan}"
KIND="$1"; RUNS="${2:-3}"; OUT="${3:-build/perf/$GAME}"
case "$GAME" in
	mahjongjapan) EXE=MahjongEscapeAJ.exe; SAVES='Mahjong Escape Ancient Japan'; CHECK=build/logs/check/mahjongjapan/game/script.txt ;;
	mahjongchina) EXE=MahjongEscapeAC.exe; SAVES='Mahjong Escape'; CHECK=build/logs/check/mahjongchina/game/script.txt ;;
esac
PROF=games/bejeweled/tools/profile
HERE=games/mahjongjapan/tools/profile
WIN="${WIN:-20,50}"
OFFSET="${OFFSET:-6}"      # the original's first click is this many seconds after the port's (perfscript.py); WIN moves with it for it
ROOT="$(cygpath -m "$PWD")"
mkdir -p "$OUT"
waitroom() { until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 15; done; }
if [ "$KIND" = port ]; then
	python -I "$HERE/perfscript.py" "$CHECK" port "$OUT/port-script.txt"
	for i in $(seq 1 "$RUNS"); do
		waitroom
		d="$OUT/port$i"; rm -rf "$d"; mkdir -p "$d/user"
		python -I "$PROF/perfmin.py" "$d/perf.json" "$WIN" -- "${EXEDIR:-$ROOT/build/native}/$GAME.exe" -gamedir="$ROOT/assets/$GAME" \
			-userdir="$ROOT/$d/user" -logos -script="$ROOT/$OUT/port-script.txt" | tail -n 1 | cut -c1-400
	done
else
	ST="${GAME}cov"; SD="C:/byo-og/$ST"; L="$LOCALAPPDATA/JollyBear/$SAVES"
	python -I "$HERE/perfscript.py" "$CHECK" orig "$OUT/orig-script.txt" "$OFFSET"
	mkdir -p "$OUT/saves"; cp "$L"/* "$OUT/saves/" 2> /dev/null
	if [ ! -f "$SD/game/$EXE" ]; then
		build/origrun-bin/origrun.exe --game "$GAME" --run "$ST" --stage-root 'C:\byo-og' --copy-only > "$OUT/stage.log" 2>&1
	fi
	PART="$(echo "$SD/game/$EXE" | tr 'A-Z' 'a-z')"
	for i in $(seq 1 "$RUNS"); do
		waitroom
		d="$OUT/orig$i"; rm -rf "$d"; mkdir -p "$d"
		rm -f "$L"/*.usr "$L"/*.sav
		OW="$(python -I -c "a,b='$WIN'.split(','); print('%g,%g' % (float(a) + $OFFSET - 1.3, float(b) + $OFFSET - 1.3))")"
		timeout 200 python -I "$PROF/perfmin.py" "$d/perf.json" "$OW" --attach "$PART" > "$d/perfmin.out" 2>&1 &
		PM=$!
		build/origrun-bin/origrun.exe --exe "$(cygpath -w "$SD/game/$EXE")" --run "$ST" --script "$(cygpath -w "$PWD/$OUT/orig-script.txt")" \
			--timeout 150 > "$d/origrun.log" 2>&1
		wait $PM
		mkdir -p "$d/saves"; cp "$L"/*.usr "$d/saves/" 2> /dev/null     # the player file shows whether the stage was won
		echo "saves: $(cat "$d"/saves/*.usr 2> /dev/null | md5sum | cut -c1-8) $(cat "$d"/saves/*.usr 2> /dev/null | wc -c) bytes"
		tail -n 1 "$d/perfmin.out" | cut -c1-400
		grep -E "RESULT|SOUND LEAK" "$d/origrun.log" | cut -c1-200
	done
	rm -f "$L"/*.usr "$L"/*.sav
	cp "$OUT/saves"/* "$L/" 2> /dev/null
fi
echo "perfrun $KIND done"
