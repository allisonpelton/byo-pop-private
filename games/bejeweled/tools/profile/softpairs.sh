#!/bin/bash
# softpairs.sh <out dir> [rounds] [configs]: the software screen's memory and processor time like for like (GAME_NOTES.md,
# "The software screen at 1x"; docs/COORDINATION.md, "The target"). Each round runs the original and then the port's
# configurations one after another, one game process at a time, each on a performance core of its own (tools/pincore.py), over
# perfscript.py's minute of play (Play at 15 s, 40 swaps from 17 s to 75.5 s, quit at 90 s), each sampled from outside by
# perfcyc.py: the private bytes, and the process's cycle count (QueryProcessCycleTime) in the window 20 s to 80 s of the
# process's life. Before each run it waits for a free slot (fewer than 3 game processes on the machine, CLAUDE.md), and the
# machine's game processes are counted once a second during it (<out dir>/<run>.counts).
#
# The configurations (CONFIGS, default "orig soft1 before1 soft2"):
#   orig     the original, the carved exe in E:\byo-og\bej-perf\game under origrun, on the wall clock, windowed at its own size
#   soft1    this build (build/native/bejeweled.exe), -windowscale=1: the software screen, 640x480
#   before1  the exe of main before the opt-in (build/before/bejeweled.exe), -windowscale=1: the GPU path, 640x480
#   soft2    this build at the default 2x window with SOFTSCREEN_SCALED=1 (the software screen stays in software, GDI stretches)
#   gpu2     this build at the default 2x window: the software screen hands the window to the GPU there
# The port runs windowed (ScreenMode 0) from a fresh user folder, with no -commands channel and no per-update log, under rundesk.
# The original's saved game in C:\ProgramData\Steam\Bejeweled\userdata is taken away before each run and the one there before
# is put back at the end (the user allowed this, CLAUDE.md "Originals").
# Summed by games/bejeweled/tools/profile/softsum.py <out dir>. From the worktree root.
set -u
cd "$(dirname "$0")/../../../.."
OUT="$1"; ROUNDS="${2:-5}"; CONFIGS="${3:-orig soft1 before1 soft2}"
P=games/bejeweled/tools/profile
mkdir -p "$OUT"
win() { cygpath -w "$1"; }
USERDATA=/c/ProgramData/Steam/Bejeweled/userdata
cp -p "$USERDATA/savegame.dat" "$OUT/user-savegame.dat" 2>/dev/null
python -I $P/perfscript.py orig "$OUT/orig.txt" > /dev/null
python -I $P/perfscript.py port "$OUT/port.txt" > /dev/null
HERE="$(pwd | tr 'A-Z' 'a-z')"; HERE="${HERE#/c/}"  # the folder's name in the exe paths the sampler matches
SAMPLE_ORIG='bej-perf/game/bejeweledgame.exe'

slot() { until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 15; done; }
counts() { while [ ! -e "$1.stop" ]; do python -I tools/gamecount.py | head -1 | awk '{print $3}' >> "$1"; sleep 1; done; }

# run <name> <sampler's path part> <command...>: the sampler from outside, the command on its own core, the counts beside it
run() {
	local name="$1" part="$2"; shift 2
	slot
	local C="$OUT/$name.counts"; rm -f "$C" "$C.stop"; counts "$C" & local CP=$!
	python -I $P/perfcyc.py "$OUT/$name.json" 20,80 --attach "$part" > "$OUT/$name.perf.txt" 2>&1 & local PP=$!
	python -I tools/pincore.py -- "$@" > "$OUT/$name.log" 2>&1
	wait $PP; touch "$C.stop"; wait $CP
	echo "$name: $(tail -1 "$OUT/$name.perf.txt" | cut -c1-400)"
}

port() {  # port <name> <exe> <extra args...>; the environment (SOFTSCREEN_SCALED) is the caller's
	local name="$1" exe="$2"; shift 2
	local U="$OUT/u-$name"; rm -rf "$U"; mkdir -p "$U"
	printf '[SOFTWARE\\Steam\\Bejeweled]\nScreenMode=dword:0\n' > "$U/registry.ini"
	run "$name" "$HERE/${exe#./}" "$(win "$PWD/build/native/rundesk.exe")" "$(win "$PWD/$exe")" \
		-gamedir="$(win "$PWD/assets/bejeweled")" -userdir="$(win "$PWD/$U")" -script="$(win "$PWD/$OUT/port.txt")" "$@"
}

for n in $(seq 1 "$ROUNDS"); do
	for c in $CONFIGS; do
		case "$c" in
			orig)
				rm -f "$USERDATA/savegame.dat"
				run "orig$n" "$SAMPLE_ORIG" "$(win "$PWD/build/origrun-bin/origrun.exe")" --exe 'E:\byo-og\bej-perf\game\BejeweledGame.exe' \
					--run "soft-o$n" --sound stub --script "$(win "$PWD/$OUT/orig.txt")" --timeout 150 ;;
			soft1) port "soft1-$n" build/native/bejeweled.exe -windowscale=1 ;;
			before1) port "before1-$n" build/before/bejeweled.exe -windowscale=1 ;;
			soft2) SOFTSCREEN_SCALED=1 port "soft2-$n" build/native/bejeweled.exe ;;
			gpu2) port "gpu2-$n" build/native/bejeweled.exe ;;
		esac
	done
done
rm -f "$USERDATA/savegame.dat"
[ -f "$OUT/user-savegame.dat" ] && cp -p "$OUT/user-savegame.dat" "$USERDATA/savegame.dat" && echo "save restored"
echo finished
