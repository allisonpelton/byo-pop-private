#!/bin/bash
# pairs.sh <out dir> [pairs] [max pairs]: the performance pass's measurement (GAME_NOTES.md section 19), the original and
# the port back to back in pairs over perfscript.py's minute of play, each run sampled by perfmin.py, one game process at
# a time. While a run goes, the machine's game processes are counted once a second (tools/gamecount.py); a pair in which
# another game process appeared is run again, up to [max pairs] (default 8) until [pairs] (default 4) are clean.
# Waits first, up to WAIT_S seconds (default 7200), for no game process anywhere; past that it measures anyway and the
# counts beside each pair say so (<out dir>/pairs.tsv).
# The original: the copy in C:\byo-og\bej-perf\game (the exe carved out of WinBej.exe) under origrun, on the real clock with
# the silent stand-in; its saved game in C:\ProgramData\Steam\Bejeweled\userdata is taken away before each run (a game
# quit during play is saved, and the next run would open on "continue?") and the one there before is put back at the end.
# The port: build/native/bejeweled.exe under rundesk, windowed, from a fresh user folder, on a -script with the same input.
set -u
cd "$(dirname "$0")/../../../.."
OUT="$1"; WANT="${2:-4}"; MAX="${3:-8}"; WAIT_S="${WAIT_S:-7200}"
P=games/bejeweled/tools/profile
mkdir -p "$OUT"
win() { cygpath -w "$1"; }
USERDATA=/c/ProgramData/Steam/Bejeweled/userdata
cp -p "$USERDATA/savegame.dat" "$OUT/user-savegame.dat" 2>/dev/null
python -I $P/perfscript.py orig "$OUT/orig.txt" > /dev/null
python -I $P/perfscript.py port "$OUT/port.txt" > /dev/null

start=$(date +%s); quiet=yes
until python -I tools/gamecount.py --limit 1 > /dev/null; do
	if [ $(( $(date +%s) - start )) -ge "$WAIT_S" ]; then quiet=no; break; fi
	sleep 15
done
echo "start $(date '+%F %T'), quiet=$quiet, waited $(( $(date +%s) - start )) s" | tee "$OUT/start.txt"

# counts <file>: the machine's game processes once a second until the file <file>.stop appears
counts() { while [ ! -e "$1.stop" ]; do python -I tools/gamecount.py | head -1 | awk '{print $3}' >> "$1"; sleep 1; done; }
# the most game processes seen in a run, less the one measured
others() { local m; m=$(sort -n "$1" | tail -1); echo $(( ${m:-1} - 1 )); }

printf 'pair\torig others\tport others\tkept\n' > "$OUT/pairs.tsv"
clean=0; n=0
while [ $clean -lt "$WANT" ] && [ $n -lt "$MAX" ]; do
	n=$((n + 1))
	# the original
	rm -f "$USERDATA/savegame.dat"
	C="$OUT/o$n.counts"; rm -f "$C" "$C.stop"; counts "$C" & CP=$!
	python -I $P/perfmin.py "$OUT/o$n.json" 20,80 --attach 'bej-perf/game/BejeweledGame.exe' > "$OUT/o$n.perf.txt" 2>&1 & PP=$!
	build/origrun-bin/origrun.exe --exe 'C:\byo-og\bej-perf\game\BejeweledGame.exe' --run "perf-o$n" --sound stub \
		--script "$(win "$PWD/$OUT/orig.txt")" --timeout 150 > "$OUT/o$n.origrun.txt" 2>&1
	wait $PP; touch "$C.stop"; wait $CP
	oo=$(others "$C")
	# the port
	U="$OUT/u$n"; rm -rf "$U"; mkdir -p "$U"
	printf '[SOFTWARE\\Steam\\Bejeweled]\nScreenMode=dword:0\n' > "$U/registry.ini"
	C="$OUT/p$n.counts"; rm -f "$C" "$C.stop"; counts "$C" & CP=$!
	build/native/rundesk.exe python -I $P/perfmin.py "$OUT/p$n.json" 20,80 -- "$(win "$PWD/build/native/bejeweled.exe")" \
		-gamedir="$(win "$PWD/assets/bejeweled")" -userdir="$(win "$PWD/$U")" -script="$(win "$PWD/$OUT/port.txt")" \
		> "$OUT/p$n.perf.txt" 2>&1
	touch "$C.stop"; wait $CP
	po=$(others "$C")
	kept=no; if [ "$oo" -eq 0 ] && [ "$po" -eq 0 ]; then kept=yes; clean=$((clean + 1)); fi
	[ "$quiet" = no ] && kept=yes && clean=$((clean + 1))
	printf '%d\t%d\t%d\t%s\n' $n "$oo" "$po" $kept | tee -a "$OUT/pairs.tsv"
done
rm -f "$USERDATA/savegame.dat"
[ -f "$OUT/user-savegame.dat" ] && cp -p "$OUT/user-savegame.dat" "$USERDATA/savegame.dat" && echo "save restored"
