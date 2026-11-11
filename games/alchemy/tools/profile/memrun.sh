#!/bin/bash
# memrun.sh: what the port holds in memory at the title (update 300), in the middle of the scripted minute (3000) and at its end
# (6440), by the framework's `memreport <file>` test command (docs/TEST_COMMANDS.md). It takes build/perf/port-play.txt (the
# port's script of the performance runs), adds the three commands, writes build/perf/port-mem.txt, and plays it with -fast under
# rundesk once, after waiting for room (at most 3 game processes machine-wide). The reports are build/perf/memreport-title.txt,
# -play.txt and -end.txt. Run from the worktree root.
cd "$(git rev-parse --show-toplevel)" || exit 1
W=$(pwd -W)
python -I - "$W" <<'PYEOF'
import sys
root = sys.argv[1]
lines = [l.rstrip('\n') for l in open('build/perf/port-play.txt')]
quit_line = [l for l in lines if l.strip() == 'quit' or l.endswith(' quit')]
body = [l for l in lines if l not in quit_line]
extra = {300: 'title', 3000: 'play', 6440: 'end'}
items = [(int(l.split()[0]), l) for l in body]
items += [(k, '%d memreport %s/build/perf/memreport-%s.txt' % (k, root, v)) for k, v in extra.items()]
items.sort(key=lambda t: t[0])
open('build/perf/port-mem.txt', 'w').write('\n'.join(l for _, l in items) + '\n' + quit_line[0] + '\n')
PYEOF
for i in $(seq 1 360); do
	python -I tools/gamecount.py --limit 3 > /dev/null && break
	sleep 10
done
rm -rf build/perf/memrun
mkdir -p build/perf/memrun/user
timeout 300 build/native/rundesk.exe "$W/build/native/alchemy.exe" "-userdir=$W/build/perf/memrun/user" -seed=11 -fast "-script=$W/build/perf/port-mem.txt" > build/perf/memrun/out.log 2>&1
echo "memrun rc=$?"
