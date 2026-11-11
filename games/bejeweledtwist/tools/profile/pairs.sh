#!/bin/bash
# pairs.sh <out dir> [pairs] [max pairs]: the performance pass's measurement (NOTES.md "The performance pass"), the
# original and the port back to back in pairs over the same seeded Classic autoplay, in 2D at the 768 art, windowed, on
# the real clock and silent, each run one process sampled by games/bejeweled/tools/profile/perfmin.py from its creation
# to its exit, one game process at a time. The window measured is 30 s to 90 s of the process's life, inside the game on
# both sides (the original reaches its menu about 11 s in, the port under 3 s). While a run goes, the machine's game
# processes are counted once a second (tools/gamecount.py); a pair in which another game process appeared is run again,
# up to [max pairs] (default 6) until [pairs] (default 3) are clean. Waits first, up to WAIT_S seconds (default 1800),
# for no game process anywhere; past that it measures anyway and the counts beside each pair say so (<out dir>/pairs.tsv).
# The original: the prepared copy (build/origrun/bjtlvl/game) copied to build/origrun/bjtpair/game, under origrun with
# its silent stand-in, the seed patch (1234) and -autoplay=classic; the registry's ScreenMode and ArtRes are set for it
# and the user's values put back at the end, as is the user's players folder (autoplay plays as the current player).
# The port: build/native/bejeweledtwist.exe under rundesk, -nod3d9 -autoplay=classic -seed=1234, a fresh user folder.
set -u
cd "$(dirname "$0")/../../../.."
OUT="$1"; WANT="${2:-3}"; MAX="${3:-6}"; WAIT_S="${WAIT_S:-1800}"
P=games/bejeweled/tools/profile
mkdir -p "$OUT"
win() { cygpath -w "$1"; }
G=build/origrun/bjtpair/game
[ -f "$G/BejeweledTwist.exe" ] || { mkdir -p "$G" && cp -r build/origrun/bjtlvl/game/. "$G/"; }
K='HKCU\Software\Steam\BejeweledTwist'
MSYS_NO_PATHCONV=1 reg export "$K" "$(win "$PWD/$OUT/user-registry.reg")" /y > /dev/null
USERS=/c/ProgramData/Steam/BejeweledTwist/users
rm -rf "$OUT/user-players"; cp -a "$USERS" "$OUT/user-players"
printf 'wait 100000\nquit\n' > "$OUT/orig.txt"
printf '9700 quit\n' > "$OUT/port.txt"

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
	MSYS_NO_PATHCONV=1 reg add "$K" /v ScreenMode /t REG_DWORD /d 0 /f > /dev/null
	MSYS_NO_PATHCONV=1 reg add "$K" /v ArtRes /t REG_DWORD /d 768 /f > /dev/null
	C="$OUT/o$n.counts"; rm -f "$C" "$C.stop"; counts "$C" & CP=$!
	python -I $P/perfmin.py "$OUT/o$n.json" 30,90 --attach 'bjtpair/game/BejeweledTwist.exe' > "$OUT/o$n.perf.txt" 2>&1 & PP=$!
	build/origrun-bin/origrun.exe --exe "$(win "$PWD/$G/BejeweledTwist.exe")" --run bjtpair \
		--patches tools/origrun/patches/bejeweledtwist.txt --seed 1234 --3d off \
		--3d-key 'Software\Steam\BejeweledTwist' --3d-flag 0xe13bc0 --arg -autoplay=classic \
		--script "$(win "$PWD/$OUT/orig.txt")" --timeout 150 > "$OUT/o$n.origrun.txt" 2>&1
	wait $PP; touch "$C.stop"; wait $CP
	oo=$(others "$C")
	# the port
	U="$OUT/u$n"; rm -rf "$U"; mkdir -p "$U"
	printf '[SOFTWARE\\Steam\\BejeweledTwist]\nArtRes=dword:768\nScreenMode=dword:0\n' > "$U/registry.ini"
	C="$OUT/p$n.counts"; rm -f "$C" "$C.stop"; counts "$C" & CP=$!
	build/native/rundesk.exe python -I $P/perfmin.py "$OUT/p$n.json" 30,90 -- "$(win "$PWD/build/native/bejeweledtwist.exe")" \
		-gamedir="$(win "$PWD/assets/bejeweledtwist")" -userdir="$(win "$PWD/$U")" -nod3d9 -autoplay=classic -seed=1234 \
		-script="$(win "$PWD/$OUT/port.txt")" > "$OUT/p$n.perf.txt" 2>&1
	touch "$C.stop"; wait $CP
	po=$(others "$C")
	kept=no; if [ "$oo" -eq 0 ] && [ "$po" -eq 0 ]; then kept=yes; clean=$((clean + 1)); fi
	[ "$quiet" = no ] && kept=yes && clean=$((clean + 1))
	printf '%d\t%d\t%d\t%s\n' $n "$oo" "$po" $kept | tee -a "$OUT/pairs.tsv"
	tail -1 "$OUT/o$n.perf.txt"; tail -1 "$OUT/p$n.perf.txt"
done
MSYS_NO_PATHCONV=1 reg import "$(win "$PWD/$OUT/user-registry.reg")" > /dev/null 2>&1
cp -a "$OUT/user-players/." "$USERS/"
echo "done $(date '+%F %T'): $clean clean of $n pairs"
