#!/usr/bin/env bash
# Wait (up to 90 minutes) until the machine has room for another game process (tools/gamecount.py: at most 3 in all) and fewer than 2 of this
# job's own runs are going (typershark.exe with build/ts-p2 in its command line, origrun.exe with typershark-p2), as every test run must.
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
mine() {
	powershell -NoProfile -Command "@(Get-CimInstance Win32_Process | Where-Object { (\$_.Name -eq 'typershark.exe' -and \$_.CommandLine -like '*ts-p2*') -or (\$_.Name -eq 'origrun.exe' -and \$_.CommandLine -like '*typershark-p2*') }).Count" 2>/dev/null | tr -d '\r'
}
n=0
while true; do
	if python "$ROOT/tools/gamecount.py" --limit 6 > /dev/null; then
		sleep $((RANDOM % 7)); m="$(mine)"
		if [ "${m:-0}" -lt 2 ] && python "$ROOT/tools/gamecount.py" --limit 6 > /dev/null; then exit 0; fi
	fi
	n=$((n+1)); [ $n -gt 270 ] && { echo "waitroom: no room for a game process after 90 minutes" >&2; exit 3; }
	sleep 20
done
