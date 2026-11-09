#!/bin/bash
# perf.sh <tour dir> [runs]: the performance numbers of the final merge (build/slots/news.md, 2026-11-08), the original
# and the port back to back, on the screen tour (tools/shots/tour.py: the title, options, rules, a game dealt, hint,
# show moves, pause, the focus, the cheat's 74 matches, no moves, the name and the best times: 65 s) in real time.
# Each run is sampled by games/bejeweled/tools/profile/perfmin.py from the process's creation to its exit; the window
# is 20 s to 50 s (the game, the cheat's matches). The original runs on origrun's real clock with no sound device
# (tools/sound/orig.sh's script with --sound nodriver: with the stand-in its mixer spins from the deal on, a core's
# worth, IDENTICAL_PLAY.md, "Oddities"), stopped at 90 s (its script's closing quit only asks the exit question); the
# port natively under rundesk without -fast. perfload.py reads the loading time
# from the samples. Writes <tour dir>/perf/{orig,port}-<n>.json.
source "$(dirname "$0")/../identical/common.sh"
D="$1"; N="${2:-3}"
P="$D/perf"; mkdir -p "$P"
PERFMIN=games/bejeweled/tools/profile/perfmin.py
grep -v " screenshot " "$D/port.txt" > "$P/port-tour.txt"
for ((i = 1; i <= N; i++)); do
	until python tools/gamecount.py --limit 2 > /dev/null; do sleep 20; done
	python "$PERFMIN" "$P/orig-$i.json" 20,50 --attach 'byo-og\np\game\Ningpo.exe' > "$P/orig-$i.txt" 2>&1 &
	ORIG_SOUND=nodriver bash "$(dirname "$0")/../sound/orig.sh" "$D" 90 > /dev/null 2>&1
	wait
	until python tools/gamecount.py --limit 2 > /dev/null; do sleep 20; done
	U="$P/user"; rm -rf "$U"; mkdir -p "$U"
	"$RUNDESK" python "$PERFMIN" "$P/port-$i.json" 20,50 -- "$(win "$ROOT/build/native/ningpo.exe")" -gamedir=assets/ningpo \
		-userdir="$U" -seed=7 -realinput="$(win "$PWD/$P/port-tour.txt")" > "$P/port-$i.txt" 2>&1
	echo "run $i: orig $(tail -1 "$P/orig-$i.txt" | cut -c1-200)"
	echo "run $i: port $(tail -1 "$P/port-$i.txt" | cut -c1-200)"
done
python "$(dirname "$0")/perfload.py" "$P"/*.json
