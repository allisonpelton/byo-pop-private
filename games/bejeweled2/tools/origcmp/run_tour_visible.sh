#!/bin/bash
# The 3D game's tour (1024 by 768), the original on the visible desktop and the port natively, then shotdiff. Run from the
# repository's root; the port's exe is build/native/bejeweled2.exe.
#
#   bash games/bejeweled2/tools/origcmp/run_tour_visible.sh <events.json> <tag> [<port-only>]
#
# <events.json> is a recording of the port's tour (PLAY_AT=940 DRAWEVERY=1 bash games/bejeweled2/tools/origcmp/record_port.sh tour 6
# <dir> 909, the <dir>/events.json). The recording becomes an origrun script (to_origrun.py, with --dialog-shift -1) with the gate
# in front of its first input, visrun.py plays it with the original on the user's desktop under the stand-in's virtual clock
# (docs/ORIGRUN.md; the copy, the registry key and the players' folder are put back when it ends), and the port replays the inputs
# at the updates they landed at (landed_to_events.py, then to_commands.py --shots) in a run under rundesk.exe, in the real 3D (no
# SDL_RENDER_DRIVER). LDARGS gives landed_to_events.py its options. The default is the mouse's leave after each click, with the
# manager's last position left where the click was (the original's runs have the same: the real cursor is never over its window, and
# its WARP button's glow reads the posted position), and the click that closes the first game's help screen, which the original reads
# one update after the port, going on by one update (--shift FROM TO 1, found in the original's landed.tsv; IDENTICAL_PLAY.md).
# With a third argument the original's run is not made again: <tag> then names an earlier run in build/visrun. ORIG_ONLY=1 makes the
# original's run alone (the night's part: the visible desktop), and the port's replay and shotdiff come later with the third argument.
set -u
ev=$1; tag=$2; portonly=${3:-}
cd "$(dirname "$0")/../../../.." || exit 2
ABS=$PWD
# the click that closes the first game's help screen (743, 678) is read by the original one update after the port's (GAME_NOTES, "Oddities"),
# so the replay gives it (and the move before it) that update: --shift FROM TO 1, found in the original's landed.tsv (LDARGS= sets all by hand)
if [ -z "${LDARGS+x}" ]; then
  LDARGS="--leave-after 1 --leave-keep $(python - build/visrun/$tag/landed.tsv <<'PYEOF'
import sys
rows = [l.rstrip(chr(10)).split(chr(9)) for l in open(sys.argv[1])]
for i, r in enumerate(rows):
    if r[0] == 'click' and r[1] == '743 678':
        lo = int(rows[i - 1][2]) if i and rows[i - 1][0] == 'move' else int(r[2])
        print('--shift %d %d 1' % (lo, int(r[2])))
        break
PYEOF
)"
fi
# the player of the recording (record_3d_tours.sh's PROFILE: make_profile.py's, every mode unlocked) is the original's players while
# its run lasts, and the port's replay has it in its user folder; PROFILE= (empty) starts both with no player
PROFILE=${PROFILE-build/logs/bej2/profile}
mkdir -p build/visrun build/origrun

# at most three game processes on the machine (docs/COORDINATION.md): wait for a free place
wait_slot() { for i in $(seq 1 360); do python tools/gamecount.py --limit 6 > /dev/null && return 0; sleep 15; done; return 1; }

# the original runs on the visible desktop: only from 22:00 to 08:00 local (CLAUDE.md, "Overnight"); visrun.py checks too
h=$(date +%H); h=$((10#$h))
if [ -z "$portonly" ] && [ $h -ge 8 ] && [ $h -lt 22 ]; then echo "the visible desktop is for 22:00 to 08:00 local; it is $(date +%H:%M)"; exit 5; fi

if [ -z "$portonly" ]; then
  script=build/origrun/bej2_$tag.txt
  python games/bejeweled2/tools/origcmp/to_origrun.py $ev $script --dialog-shift -1 > /dev/null || exit 2
  python - $script <<'EOF' || exit 2
import sys
p = sys.argv[1]
lines = open(p).read().split('\n')
first = next(i for i, l in enumerate(lines) if l.startswith('at '))
update = int(lines[first].split()[1])
# the title's loading runs on the wall clock: the first input waits for the Play link (visrun.py, "gate")
lines.insert(first, 'at 330 gate %d' % update)
open(p, 'w').write('\n'.join(lines))
EOF
  wait_slot || { echo "no room for a game process"; exit 3; }
  python games/bejeweled2/tools/origcmp/visrun.py ${PROFILE:+--profile $PROFILE} build/visrun/bej2/game/Bejeweled2Game.exe $script build/visrun/$tag --timeout 120
  echo "visrun rc=$?"
  cat build/visrun/$tag.visrun.log
fi

[ -n "${ORIG_ONLY:-}" ] && exit 0

V=$ABS/build/visrun/p_$tag
rm -rf $V; mkdir -p $V/cwd/shots $V/user
python games/bejeweled2/tools/origcmp/landed_to_events.py $ev build/visrun/$tag/landed.tsv $V/events.json $LDARGS > /dev/null || exit 2
python games/bejeweled2/tools/origcmp/to_commands.py $V/events.json $V/cmd.txt --shots shots > /dev/null
: > $V/empty.ri
[ -n "$PROFILE" ] && cp -r $PROFILE/users $V/user/users
printf '[SOFTWARE\\Steam\\Bejeweled2]\nScreenMode=dword:0\nHighResolution=dword:1\n' > $V/user/registry.ini
wait_slot || { echo "no room for a game process"; exit 3; }
(cd $V/cwd && timeout 1500 $ABS/build/native/rundesk.exe $ABS/build/native/bejeweled2.exe -gamedir= -userdir=$V/user -commands=$V/cmd.txt -realinput=$V/empty.ri -hold -fast -drawevery -seed=909 -trace > $V/port.log 2>&1)
echo "$tag: $(ls $V/cwd/shots | wc -l) shots"
python tools/shotdiff/shotdiff.py --game bejeweled2 --orig build/visrun/$tag --port $V/cwd/shots --run v3d_$tag 2>&1 | tail -n 1
