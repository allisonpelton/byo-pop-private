#!/usr/bin/env bash
# The activation hook (tools/origrun/activation.c) from end to end, for check.sh's origrun suite: origrun runs actest.exe, which
# loads the stand-in, shows a window without activating it and calls SetForegroundWindow, SetActiveWindow, BringWindowToTop,
# ShowWindow (every command), SetWindowPos and DeferWindowPos on it, through its imports, through GetProcAddress and from a second
# DLL (actdll.dll). Every check must pass (actest.out, "ACTTEST n of n as expected"): the window is never the foreground or the
# active window, and has the state the mapped command gives. net.log must hold the install line naming all six functions and the
# exit line with counts, three SW_SHOWMAXIMIZED passes among them. A second run with ORIGRUN_ACTIVATION=allow must show the
# "hook off" line and let the calls through (the window becomes the active one). actest has no sound and a window only on the
# private desktop the whole suite runs on (check.sh runs under rundesk).
#   tools/origrun/actest.sh <folder with origrun.exe, dsound.dll, actest.exe and actdll.dll> <work folder>
# origrun finds the repository (and rundesk) two folders above itself, so it runs from a copy in build/.
set -uo pipefail
built="$1"; work="$2"
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
bin="$repo/build/actest-bin-$$"
mkdir -p "$bin" && cp "$built/origrun.exe" "$built/dsound.dll" "$built/actest.exe" "$built/actdll.dll" "$bin/" || { echo "FAIL: no build in $built"; exit 1; }
rm -rf "$work"; mkdir -p "$work"
echo 'wait 100' > "$work/script.txt"   # a run on the host needs a script; actest has nothing to click
scr="$(cygpath -aw "$work/script.txt")"
st=0; checks=0; maxn=0

for mode in block allow; do
	game="$work/game-$mode"; mkdir -p "$game"
	cp "$bin/actest.exe" "$bin/actdll.dll" "$game/"
	exe="$(cygpath -aw "$game/actest.exe")"
	run="actest-$mode-$$"; rd="$repo/build/origrun/$run"
	if [ "$mode" = allow ]; then export ORIGRUN_ACTIVATION=allow; else unset ORIGRUN_ACTIVATION; fi
	"$bin/origrun.exe" --exe "$exe" --run "$run" --script "$scr" --timeout 60 > "$work/run-$mode.txt" 2>&1
	unset ORIGRUN_ACTIVATION
	out="$game/actest.out"
	if [ ! -f "$out" ]; then echo "FAIL ($mode): no actest.out ($out)"; sed -n '1,30p' "$work/run-$mode.txt"; st=1; continue; fi
	grep -q '^ACTTEST \([0-9]*\) of \1 as expected' "$out" || { echo "FAIL ($mode): actest's checks did not all pass"; grep -v '^ok' "$out"; st=1; }
	n=$(grep -c '^ok\|^FAIL' "$out"); checks=$((checks + n))
	nl="$rd/logs/net.log"
	if [ "$mode" = block ]; then
		grep -q 'activation hook installed: reached SetForegroundWindow, SetActiveWindow, BringWindowToTop, ShowWindow, SetWindowPos, DeferWindowPos, ShowWindowAsync, SetWindowPlacement, SwitchToThisWindow (' "$nl" 2>/dev/null \
			|| { echo "FAIL (block): net.log lacks the install line naming all six functions"; grep -i 'activation' "$nl" | head -3; st=1; }
		grep -q 'exits: activation hook answered SetForegroundWindow [1-9]' "$nl" 2>/dev/null || { echo "FAIL (block): net.log lacks the exit counts"; grep -i 'activation' "$nl" | head -3; st=1; }
		grep -q 'SW_SHOWMAXIMIZED passed on 3 ' "$nl" 2>/dev/null || { echo "FAIL (block): net.log does not count the three SW_SHOWMAXIMIZED passes"; grep 'exits: activation' "$nl"; st=1; }
		maxn=$(grep -o 'exits: activation hook.*' "$nl" | head -1 | cut -c1-200)
	else
		grep -q 'activation hook off (ORIGRUN_ACTIVATION=allow)' "$nl" 2>/dev/null || { echo "FAIL (allow): net.log lacks the hook-off line"; grep -i 'activation' "$nl" | head -3; st=1; }
		grep -q 'activation hook installed' "$nl" 2>/dev/null && { echo "FAIL (allow): the hook was installed"; st=1; }
	fi
	rm -rf "$rd"
done
rm -rf "$bin"
[ $st -eq 0 ] && echo "activation hook: the window never took the foreground or became active through any of the nine functions (import table, GetProcAddress, second DLL), each mapped command left the window as its non-activating form does, SW_SHOWMAXIMIZED passed on and was counted, ORIGRUN_ACTIVATION=allow let the calls through ($checks checks)"
exit $st
