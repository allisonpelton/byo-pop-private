#!/bin/bash
# reset.sh <port user dir>: the same start state on both sides. The original: the user's WinBAD values with LastUser
# Tester, and Allison.bwa (theirs) plus Tester.bwa in its users folder. The port: a fresh copy of portuser0.
P=$(cd "$(dirname "$0")/../../../.." && pwd)/build/scratch/pilot
U="/c/ProgramData/PopCap Games/WinBAD/users"
export MSYS_NO_PATHCONV=1
reg import "$(cygpath -w "$P/backup/WinBAD.reg")" 2>/dev/null
reg add 'HKCU\Software\PopCap\WinBAD' /v LastUser /t REG_SZ /d Tester /f > /dev/null
reg add 'HKCU\Software\PopCap\WinBAD' /v InProgress /t REG_DWORD /d 0 /f > /dev/null
rm -f "${U:?}"/*.bwa
cp "$P/backup/users/Allison.bwa" "$P/portuser0/users/Tester.bwa" "$U/"
if [ -n "$1" ]; then rm -rf "${1:?}"; cp -r "$P/portuser0" "$1"; fi
