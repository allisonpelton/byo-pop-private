#!/usr/bin/env bash
# The performance pass's like-for-like runs (README "Performance"): the port and the original turn about, <pairs> pairs,
# each run held at the title with the test profile's or the user's saved player, idle, for 105 s in real time, and read
# from outside by memrun.py: private bytes every 50 ms, the cycle count of the process and of each thread from 40 to
# 100 s, the memory map at 105 s. The port runs with no -commands channel (nothing of the test machinery runs) in its own
# 800x600 window (ScreenMode 0, -windowscale=1) in 3D, as on a card that passes the original's test; the original
# through origrun on rundesk's desktop (2D: DirectDraw has no hardware there), or with ORIG3D=1 in 3D through
# dgVoodoo (ORIGRUN_DGVOODOO or E:\retools\dgVoodoo2_87_5). Each run is pinned to a performance core of its own
# (tools/pincore.py), the trunk's way for a GPU-drawn port (docs/MEMORY.md, "Like for like, for a renderer whose cost
# lands in a driver thread"): unpinned, the port's vsync present spins on free cores of rundesk's desktop, which has no
# vertical blank (5.7 to 25.6 G cycles of its main thread a minute). PIN=0 runs unpinned. A pinned original can take
# over 20 s to show its window, so the window starts at 40 s. Each run waits for fewer than LIMIT (default 6) game
# processes on the machine.
#   bash games/bookwormadv/tools/profile/pairs.sh <out dir> [pairs]       (from the worktree root)
# Then: python games/bookwormadv/tools/profile/pairsum.py <out dir>
set -u
OUT="$1"; N="${2:-5}"; LIMIT="${LIMIT:-6}"
mkdir -p "$OUT"
printf 'wait 115000\n' > "$OUT/idle.txt"
DG="${ORIGRUN_DGVOODOO:-E:\\retools\\dgVoodoo2_87_5}"
O3D=(); [ "${ORIG3D:-0}" = 1 ] && O3D=(--dgvoodoo "$DG" --3d on)
PINC=(python -I tools/pincore.py --); [ "${PIN:-1}" = 0 ] && PINC=()
slot() { while ! python tools/gamecount.py --limit "$LIMIT" >/dev/null 2>&1; do sleep 10; done; }
for i in $(seq 1 "$N"); do
	slot
	"${PINC[@]}" build/native/rundesk.exe python -I games/bookwormadv/tools/profile/memrun.py port "$OUT/p$i" 105 \
		--nochannel --cycles 40,100 --reg ScreenMode=dword:0 -- -windowscale=1 > "$OUT/p$i.txt" 2>&1
	slot
	"${PINC[@]}" python -I games/bookwormadv/tools/profile/memrun.py orig "$OUT/o$i" 105 --cycles 40,100 -- \
		--game bookwormadv --run "bwaperf$i" --stage-root 'E:\byo-og' --timeout 115 "${O3D[@]}" \
		--script "$(cygpath -w "$(pwd)/$OUT/idle.txt")" > "$OUT/o$i.txt" 2>&1
done
echo finished
