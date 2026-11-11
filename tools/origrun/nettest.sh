#!/usr/bin/env bash
# The network block (tools/origrun/net.c) from end to end, for check.sh's origrun suite: origrun runs nettest.exe, which
# loads the stand-in and calls every function the block hooks at a listener on the loopback (some through their imports,
# some through GetProcAddress, urlmon and winhttp after the stand-in loaded). Every call must fail as the block makes it fail
# (nettest.out, "NETTEST n of n as expected"), nothing may reach the listener, net.log must hold one "NET BLOCKED" line for
# each call, and origrun's RESULT must say how many were blocked. A second run of the same run folder must count only its own
# lines. The launch guard has the same test: nettest starts cmd.exe through ShellExecute, WinExec and CreateProcess (it would
# write nettest-launched.txt if one ran), each must fail with ERROR_ACCESS_DENIED, net.log must hold a "LAUNCH BLOCKED" line for
# each and origrun's RESULT: LAUNCH must count them. nettest has no window and no sound.
#   tools/origrun/nettest.sh <folder with origrun.exe, dsound.dll and nettest.exe> <work folder>
# origrun finds the repository (and rundesk) two folders above itself, so it runs from a copy in build/.
set -uo pipefail
built="$1"; work="$2"
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
bin="$repo/build/nettest-bin-$$"
mkdir -p "$bin" && cp "$built/origrun.exe" "$built/dsound.dll" "$built/nettest.exe" "$bin/" || { echo "FAIL: no build in $built"; exit 1; }
rm -rf "$work"; mkdir -p "$work/game"
cp "$bin/nettest.exe" "$work/game/nettest.exe"
exe="$(cygpath -aw "$work/game/nettest.exe")"
echo 'wait 100' > "$work/script.txt"   # a run on the host needs a script; nettest has nothing to click
scr="$(cygpath -aw "$work/script.txt")"
run="nettest-$$"; st=0
rd="$repo/build/origrun/$run"

for pass in 1 2; do
	"$bin/origrun.exe" --exe "$exe" --run "$run" --script "$scr" --timeout 30 > "$work/run$pass.txt" 2>&1
	out="$work/game/nettest.out"; rm -f "$out.keep"
	if [ ! -f "$out" ]; then echo "FAIL (run $pass): no nettest.out ($out)"; sed -n '1,30p' "$work/run$pass.txt"; st=1; continue; fi
	grep -q '^NETTEST \([0-9]*\) of \1 as expected' "$out" || { echo "FAIL (run $pass): nettest's calls were not all blocked as expected"; grep -v '^ok' "$out"; st=1; }
	total=$(grep -c '^ok\|^FAIL' "$out")
	# the calls that are blocked (the ones that name a hooked function): connect ... WinHttpSendRequest, each at least once
	for fn in connect WSAConnect WSAConnectByNameA gethostbyname WSAAsyncGetHostByName getaddrinfo GetAddrInfoW sendto WSASendTo \
		InternetOpenA InternetOpenW InternetOpenUrlA InternetConnectA HttpSendRequestA HttpSendRequestW URLDownloadToFileA \
		URLDownloadToFileW URLOpenStreamA URLOpenBlockingStreamA WinHttpOpen WinHttpConnect WinHttpSendRequest; do
		grep -q "NET BLOCKED $fn " "$rd/logs/net.log" 2>/dev/null || { echo "FAIL (run $pass): net.log has no NET BLOCKED line for $fn"; st=1; }
	done
	n=$(grep -c 'NET BLOCKED' "$rd/logs/net.log" 2>/dev/null) || true   # grep -c prints 0 and exits 1 on an empty file
	n=${n:-0}
	want=$((n))   # after the second run the file holds both runs' lines
	grep -q "RESULT: NET: [0-9]* network call" "$rd/logs/origrun.log" || { echo "FAIL (run $pass): origrun's log has no RESULT: NET line with a count"; grep 'NET' "$rd/logs/origrun.log" | head -5; st=1; }
	cnt=$(grep -o 'RESULT: NET: [0-9]*' "$rd/logs/origrun.log" | grep -o '[0-9]*$' | head -1)
	if [ "$pass" = 1 ]; then first=$cnt; [ "${cnt:-0}" -ge 20 ] || { echo "FAIL (run 1): RESULT counts ${cnt:-none} blocked calls, expected at least 20"; st=1; }
	else [ "$cnt" = "$first" ] || { echo "FAIL (run 2): RESULT counts $cnt blocked calls, the first run counted $first (the second must count only its own lines)"; st=1; }; fi
	for fn in ShellExecuteA ShellExecuteW ShellExecuteExA ShellExecuteExW WinExec CreateProcessA CreateProcessW; do
		grep -q "LAUNCH BLOCKED $fn " "$rd/logs/net.log" 2>/dev/null || { echo "FAIL (run $pass): net.log has no LAUNCH BLOCKED line for $fn"; st=1; }
	done
	grep -q "RESULT: LAUNCH: [0-9]* launch" "$rd/logs/origrun.log" || { echo "FAIL (run $pass): origrun's log has no RESULT: LAUNCH line with a count"; grep 'LAUNCH' "$rd/logs/origrun.log" | head -5; st=1; }
	lcnt=$(grep -o 'RESULT: LAUNCH: [0-9]*' "$rd/logs/origrun.log" | grep -o '[0-9]*$' | head -1)
	if [ "$pass" = 1 ]; then lfirst=$lcnt; [ "${lcnt:-0}" -ge 7 ] || { echo "FAIL (run 1): RESULT counts ${lcnt:-none} blocked launches, expected at least 7"; st=1; }
	else [ "$lcnt" = "$lfirst" ] || { echo "FAIL (run 2): RESULT counts $lcnt blocked launches, the first run counted $lfirst"; st=1; }; fi
	[ ! -e "$work/game/nettest-launched.txt" ] || { echo "FAIL (run $pass): a launch got through: nettest-launched.txt exists"; st=1; }
	grep -q 'NET BLOCKED connect 127.0.0.1:' "$rd/logs/origrun.log" || { echo "FAIL (run $pass): origrun's log lacks the connect line with its address"; st=1; }
done
rm -rf "$rd" "$bin"
[ $st -eq 0 ] && echo "network block and launch guard: every hooked call failed at once and was logged ($first network calls and $lfirst launches blocked in a run; $total checks), nothing reached the listener or started, the second run counted only its own lines"
exit $st
