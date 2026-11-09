#!/usr/bin/env bash
# The --coverage recorder from end to end, for check.sh's origrun suite: origrun gives covtest.exe (which imports no
# DirectSound) the stand-in as an import, runs it, and the hits file must hold the two functions it calls and not the
# one it never calls; the next run without --coverage must put the exe back. A second case does the same with the bytes
# after the section table in use (tools/origrun/fillslack.py, as Mahjong Escape's exes keep their copyright text there),
# so origrun must move the headers up to make room. covtest has no window and no sound.
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
echo 'wait 100' > "$work/script.txt"   # a run on the host needs a script; covtest has nothing to click
scr="$(cygpath -aw "$work/script.txt")"
st=0

# one_case <name> <the exe as it starts> <a line origrun's log must hold>
one_case() {
	local name="$1" start="$2" logline="$3" run="covtest-$$-$1"
	cp "$start" "$work/game/covtest.exe"
	"$bin/origrun.exe" --exe "$exe" --run "$run" --script "$scr" --timeout 20 --coverage "$(cygpath -aw "$work/functions.txt")" > "$work/$name.txt" 2>&1
	local hits="$repo/build/origrun/$run/coverage.txt"
	if [ ! -f "$hits" ]; then
		echo "FAIL ($name): no hits file ($hits)"; sed -n '1,40p' "$work/$name.txt"; st=1
	else
		local got expect
		got="$(grep -v '^#' "$hits" | awk '{print $1}' | sort | tr '\n' ' ')"
		expect="$(printf '%s\n' "${want[0]}" "${want[1]}" | sort | tr '\n' ' ')"
		if [ "$got" != "$expect" ]; then echo "FAIL ($name): reached '$got', expected '$expect'"; cat "$hits"; st=1; fi
		grep -q 'ended normally' "$hits" || { echo "FAIL ($name): no closing line"; st=1; }
	fi
	grep -q "$logline" "$work/$name.txt" "$repo/build/origrun/$run/logs/origrun.log" 2>/dev/null || { echo "FAIL ($name): origrun's log lacks '$logline'"; st=1; }
	# The next run, without --coverage, takes the import out again.
	"$bin/origrun.exe" --exe "$exe" --run "$run" --script "$scr" --timeout 20 > "$work/$name-2.txt" 2>&1
	cmp -s "$work/game/covtest.exe" "$start" || { echo "FAIL ($name): the exe was not put back after the coverage run"; st=1; }
	rm -rf "$repo/build/origrun/$run" "$work/game/covtest.exe.origrun-unpatched"
}

one_case plain "$bin/covtest.exe" 'now imports the stand-in'
cp "$bin/covtest.exe" "$work/crowded.exe"
python -I "$repo/tools/origrun/fillslack.py" "$work/crowded.exe" > /dev/null || { echo "FAIL: fillslack"; st=1; }
one_case crowded "$work/crowded.exe" 'headers move up over the DOS stub'
rm -rf "$bin"
[ $st -eq 0 ] && echo "coverage: 2 of 3 functions reached, as called, with the bytes after the section table free and in use; the exe put back after each"
exit $st
