#!/usr/bin/env bash
# --count from end to end, for check.sh's origrun suite: origrun gives counttest.exe (which imports no DirectSound) the stand-in
# as an import, resolves the function list's lengths into bytes, and the stand-in counts the cycles of five assembly functions
# with different calling conventions (ret N, a 64-bit result, this in ecx, recursion, an x87 result). counttest checks its own
# results and the counters' numbers from inside (COUNTTEST_RESULT names its file); here the host's two snapshots ("count mid"
# and the "exit" before the quit) must hold the same calls and plausible cycles, and the next run without --count must put the
# exe back. counttest keeps a window (on rundesk's private desktop, with no sound) for the snapshots; it is not a game.
#   tools/origrun/counttest.sh <folder with origrun.exe, dsound.dll and counttest.exe> <work folder>
# origrun finds the repository (and rundesk) two folders above itself, so it runs from a copy in build/.
set -uo pipefail
built="$1"; work="$2"
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
bin="$repo/build/counttest-bin-$$"
mkdir -p "$bin" && cp "$built/origrun.exe" "$built/dsound.dll" "$built/counttest.exe" "$bin/" || { echo "FAIL: no build in $built"; exit 1; }
rm -rf "$work"; mkdir -p "$work/game"
cp "$bin/counttest.exe" "$work/game/counttest.exe"
"$work/game/counttest.exe" --list "$work/functions.txt" || { echo "FAIL: counttest --list"; exit 1; }
exe="$(cygpath -aw "$work/game/counttest.exe")"
printf 'wait 1500\ncount mid\nquit\n' > "$work/script.txt"
run="counttest-$$"
st=0
export COUNTTEST_RESULT="$(cygpath -aw "$work/results.txt")"
"$bin/origrun.exe" --exe "$exe" --run "$run" --script "$(cygpath -aw "$work/script.txt")" --timeout 60 \
	--count "$(cygpath -aw "$work/functions.txt")" > "$work/origrun.txt" 2>&1
counts="$repo/build/origrun/$run/counts.jsonl"
if [ ! -f "$work/results.txt" ]; then echo "FAIL: counttest wrote no results"; sed -n '1,40p' "$work/origrun.txt"; st=1
elif ! grep -q '^PASSED' "$work/results.txt"; then echo "FAIL: counttest's own checks:"; cat "$work/results.txt"; st=1; fi
if [ ! -f "$counts" ]; then echo "FAIL: no snapshot file ($counts)"; sed -n '1,40p' "$work/origrun.txt"; st=1
else
	python -I - "$counts" <<'EOF' || st=1
import json, sys
snaps = [json.loads(l) for l in open(sys.argv[1]) if l.startswith('{')]
labels = [s['label'] for s in snaps]
if labels != ['mid', 'exit']:
    sys.exit('FAIL: snapshots %s, expected mid and exit' % labels)
for s in snaps:
    p = s['parts']
    want = {'ct_outer': 1, 'ct_leaf': 4, 'ct_this': 1, 'ct_rec': 4, 'ct_f': 1}
    for k, n in want.items():
        if p[k]['calls'] != n:
            sys.exit('FAIL: %s: %s counted %d calls, expected %d' % (s['label'], k, p[k]['calls'], n))
    counted = sum(v['cycles'] for v in p.values())
    if not (1e6 < p['ct_leaf']['cycles'] < 2.4e7 and counted < s['process_cycles']):
        sys.exit('FAIL: %s: implausible cycles %s of %s' % (s['label'], p, s['process_cycles']))
    if s['main_thread_cycles'] < counted or s['misses'] != 0 or not s['threads']:
        sys.exit('FAIL: %s: main thread %d cycles, counted %d, misses %d, %d threads' % (s['label'], s['main_thread_cycles'], counted, s['misses'], len(s['threads'])))
print('snapshots ok: leaf %d cycles in 4 calls, process %d' % (snaps[-1]['parts']['ct_leaf']['cycles'], snaps[-1]['process_cycles']))
EOF
fi
# The next run, without --count, takes the import out again.
rm -f "$work/results.txt"
"$bin/origrun.exe" --exe "$exe" --run "$run" --script "$(cygpath -aw "$work/script.txt")" --timeout 60 > "$work/origrun-2.txt" 2>&1
cmp -s "$work/game/counttest.exe" "$bin/counttest.exe" || { echo "FAIL: the exe was not put back after the counting run"; st=1; }
rm -rf "$repo/build/origrun/$run" "$work/game/counttest.exe.origrun-unpatched" "$bin"
[ $st -eq 0 ] && echo "count: five functions counted by their own cycles through ret N, a 64-bit result, ecx, recursion and an x87 result, in-process and from origrun's snapshots"
exit $st
