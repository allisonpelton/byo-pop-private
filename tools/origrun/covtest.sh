#!/usr/bin/env bash
# The --coverage recorder from end to end, for check.sh's origrun suite: origrun gives covtest.exe (which imports no
# DirectSound) the stand-in as an import, runs it, and the hits file must hold the two functions it calls and not the
# one it never calls; the next run without --coverage must put the exe back. covtest has no window and no sound.
#   tools/origrun/covtest.sh <folder with origrun.exe, dsound.dll and covtest.exe> <work folder>
# origrun finds the repository (and rundesk) two folders above itself, so it runs from a copy in build/.
set -uo pipefail
built="$1"; work="$2"
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
bin="$repo/build/covtest-bin-$$"
mkdir -p "$bin" && cp "$built/origrun.exe" "$built/dsound.dll" "$built/covtest.exe" "$bin/" || { echo "FAIL: no build in $built"; exit 1; }
rm -rf "$work"; mkdir -p "$work/game"
cp "$bin/covtest.exe" "$work/game/covtest.exe"
"$work/game/covtest.exe" --addrs "$work/functions.txt" || { echo "FAIL: covtest --addrs"; exit 1; }
mapfile -t want < <(grep -v '^#' "$work/functions.txt")
exe="$(cygpath -aw "$work/game/covtest.exe")"
run="covtest-$$"
echo 'wait 100' > "$work/script.txt"   # a run on the host needs a script; covtest has nothing to click
scr="$(cygpath -aw "$work/script.txt")"
"$bin/origrun.exe" --exe "$exe" --run "$run" --script "$scr" --timeout 20 --coverage "$(cygpath -aw "$work/functions.txt")" > "$work/origrun.txt" 2>&1
hits="$repo/build/origrun/$run/coverage.txt"
st=0
if [ ! -f "$hits" ]; then
	echo "FAIL: no hits file ($hits)"; sed -n '1,40p' "$work/origrun.txt"; st=1
else
	got="$(grep -v '^#' "$hits" | awk '{print $1}' | sort | tr '\n' ' ')"
	expect="$(printf '%s\n' "${want[0]}" "${want[1]}" | sort | tr '\n' ' ')"
	if [ "$got" != "$expect" ]; then echo "FAIL: reached '$got', expected '$expect'"; cat "$hits"; st=1; fi
	grep -q 'ended normally' "$hits" || { echo "FAIL: no closing line"; st=1; }
fi
grep -q 'now imports the stand-in' "$work/origrun.txt" "$repo/build/origrun/$run/logs/origrun.log" 2>/dev/null || { echo "FAIL: origrun did not add the import"; st=1; }
# The next run, without --coverage, takes the import out again.
"$bin/origrun.exe" --exe "$exe" --run "$run" --script "$scr" --timeout 20 > "$work/origrun2.txt" 2>&1
cmp -s "$work/game/covtest.exe" "$bin/covtest.exe" || { echo "FAIL: the exe was not put back after the coverage run"; st=1; }
rm -rf "$repo/build/origrun/$run" "$bin"
[ $st -eq 0 ] && echo "coverage: 2 of 3 functions reached, as called; the exe put back after"
exit $st
