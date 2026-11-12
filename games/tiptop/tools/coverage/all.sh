#!/bin/bash
# all.sh: every coverage run of the check in turn (tools/coverage/run.sh), one at a time; a run whose hits file exists is
# skipped (rm build/coverage/tiptop/hits/<name>.txt to repeat one). Prints one line a run.
cd "$(git rev-parse --show-toplevel)" || exit 1
R=games/tiptop/tools/coverage/run.sh
H=build/coverage/tiptop/hits
go() { local n="$1"; shift; [ -s "$H/$n.txt" ] && { echo "$n: have it"; return; }; bash "$R" "$@"; }
go tour-tour1 tour tour1 1 150
go extra1 script extra1 games/tiptop/tools/coverage/extra1.txt 1 150
go extra2 script extra2 games/tiptop/tools/coverage/extra2.txt 1 150
go tour-tour3 tour tour3 1 150
go tour-tour2 tour tour2 1 300
go str case str strategy 2 3 1
go spd case spd speed 3 4 1
go save case save regular 2 2 1 -autoplaysave=30
go reg2 case reg2 regular 1 1 2
echo "all done"
