#!/bin/bash
# run.sh <name> <origrun script> [seed] [timeout s]: one coverage run of the original (docs/COVERAGE.md) on the host: origrun's
# --coverage on the list that `python -I tools/coverage.py list bejeweled2` writes, the virtual clock and the scripted pointer
# (CLOCK=real runs on the real clock, so the sound code the virtual clock skips runs). The hits go to
# build/coverage/bejeweled2/hits/<name>.txt, the run's log beside them. It runs the original as the identical-play comparisons do
# (origcmp/run_original.sh: the copy under build/origrun/bej2, the players' folder and the registry key put back to a first
# run's), so only one runs at a time (they share that state); it waits for room first (at most 3 game processes on the
# machine). Start it directly from the worktree, not under rundesk (origrun puts the game there itself).
#   PROFILE=<folder>                  the player the run starts with (make_profile.py's, with the secret modes open; the default,
#                                     build/logs/bej2/profile): the recorded plays of the modes assume one, and a first run shows the
#                                     new-player dialog first. PROFILE= (empty) starts as a first run does, for the tours that type a name
#   KEEP_STATE=1                      keeps the players and the registry key as they are (a run that continues another)
#   HIRES=1                           the 1024 by 768 game, the original under dgVoodoo2 (the 3D)
#   EXTRA="--clock real ..."          more origrun options
#   WATCHES="expr expr"               other watches (up to four; the first must be the board's state for the summary line)
NAME="$1"; S="$2"; SEED="${3:-1}"; T="${4:-300}"
export PROFILE="${PROFILE-build/logs/bej2/profile}"
cd "$(dirname "$0")/../../../.." || exit 2
R="$(pwd -W)"; H=build/coverage/bejeweled2/hits; mkdir -p "$H"
[ -f build/coverage/bejeweled2/functions.txt ] || { echo "run: python -I tools/coverage.py list bejeweled2 first"; exit 2; }
# Host runs take no shots, so a script's shot lines go. The recorded plays assume the original's timing: its title loaded at update 319
# and the first click at 566 or later, and the board's random layout comes from the draws up to the click, so the script keeps its
# `at N` of the recording. The title's loading runs on the wall clock while the updates run on the virtual one, and on a busy machine
# or with a cold cache it can come later than the first input (a click at 566 is then lost, or lands on another screen and the run
# plays another game): the script first waits for the title's loaded flag (TitleScreen +0xa0, a byte: the dword at +0x9d has it on
# top and the three bytes below it are padding, so it reads 0x01000000 and more once the Play link shows; the load progress at +0x90
# reads 1.0 long before the loading is done), and a `mark` logs the update that was. The run is made again (up to three times) when
# that update is later than 30 before the first input (the port plays its first click 31 updates after the title has loaded).
python - "$S" "$H/$NAME.script.txt" "$H/$NAME.first" << 'PY'
import re, sys
out, first = [], None
for l in open(sys.argv[1]).read().split('\n'):
    m = re.match(r'at (\d+) (\S+)(.*)$', l.strip())
    if not m:
        if l.strip() and not l.startswith('#') and l.strip() != 'mark':
            out.append(l)
        continue
    n, cmd = int(m.group(1)), m.group(2)
    if cmd in ('shot', 'waitshot', 'burst', 'record', 'hwmove', 'hwclick', 'hwrclick', 'hwkey', 'mark', 'gate'):
        continue
    if first is None:
        first = n
        out.insert(0, 'waitfor [[0x67ccd8]+0xe78]+0x9d >= 0x01000000 180000')
        out.insert(1, 'mark')
    out.append(l.strip())
quit_at = [i for i, l in enumerate(out) if l.strip() == 'quit']
if quit_at:
    out.append(out.pop(quit_at[0]))
open(sys.argv[2], 'w').write('\n'.join(out) + '\n')
open(sys.argv[3], 'w').write('%d\n' % (first or 0))
PY
A="--coverage $R/build/coverage/bejeweled2/functions.txt $EXTRA"
[ -n "$HIRES" ] && A="--dgvoodoo E:/retools/dgVoodoo2_87_5 $A"
[ -n "$CLOCK" ] && A="--clock $CLOCK $A"
# run_original.sh waits for room only for half an hour and then starts anyway: wait here, for as long as it takes
until python -I tools/gamecount.py --limit "${GAMELIMIT:-6}" > /dev/null; do sleep 20; done
# one watch only: run_original.sh runs a script again when its board watch never moves, and a run that never plays a board is no flake
for try in 1 2 3; do
	rm -rf build/origrun/bej2/coverage.txt
	ORIGRUN_ARGS="$A" WATCHES="${WATCHES:-[[0x67ccd8]+0xe68]+0x170c}" \
		bash games/bejeweled2/tools/origcmp/run_original.sh "$H/$NAME.script.txt" "$H/$NAME.log" "$SEED" "$T"
	loaded=$(grep -m1 'mark: the counter reads' "$H/$NAME.log" | sed -E 's/.*reads ([0-9]+).*/\1/')
	[ -n "$loaded" ] && [ "$loaded" -le $(( $(cat "$H/$NAME.first") - 30 )) ] && break
	echo "$NAME: the title loaded at update '$loaded', too late for the script's first input at $(cat "$H/$NAME.first"): again ($try)" >&2
done
cp build/origrun/bej2/coverage.txt "$H/$NAME.txt" || { echo "$NAME: no hits file"; tail -5 "$H/$NAME.log"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; board state changes: $(grep -c 'watch 1: .* -> [0-9]' "$H/$NAME.log"); $(grep '^#' "$H/$NAME.txt" | tail -1)"
