#!/bin/bash
# gate.sh <max> -- <command...>: waits until fewer than <max> game processes are running on the machine (every session's:
# executables of the repository's builds and staged originals; CLAUDE.md, "Running games in tests"), then runs the command.
MAX=$1; shift; shift
count() {
	powershell -NoProfile -Command "(Get-CimInstance Win32_Process | Where-Object { \$_.ExecutablePath -and (\$_.ExecutablePath -match '\\\\build\\\\(native|baseline|origrun|web)|byo-og|\\\\build\\\\tt\\\\') -and \$_.Name -notmatch 'cmake|ninja|clang|python|bash|rundesk|origrun|lsprobe|nm|objdump' } | Measure-Object).Count" | tr -d '\r'
}
while true; do
	N=$(count)
	[ -n "$N" ] && [ "$N" -lt "$MAX" ] && break
	sleep 30
done
echo "gate: $N game processes running, starting: $*"
"$@"
