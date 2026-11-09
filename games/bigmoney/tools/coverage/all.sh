#!/usr/bin/env bash
# The coverage runs of the identical-play games (docs/IDENTICAL_PLAY.md, "The games compared"), one after the other, and the
# menus tour. Each line of build/coverage/bigmoney/runs.txt is one run's summary. Usage: games/bigmoney/tools/coverage/all.sh [first run to do]
cd "$(dirname "$0")/../../../.." || exit 1
R=games/bigmoney/tools/coverage/run.sh
mkdir -p build/coverage/bigmoney
go() { name="$1"; shift; if [ -n "${FROM:-}" ] && [ "$name" != "$FROM" ] && [ -z "${started:-}" ]; then return; fi; started=1; bash "$R" "$@" | tee -a build/coverage/bigmoney/runs.txt; }
FROM="${1:-}"
go cov-a1 game cov-a1 0 1 1 5000
go cov-s1 game cov-s1 1 1 3 5000
go cov-p1 game cov-p1 2 1 5 5000
go cov-s2 game cov-s2 1 2 8 3000
go cov-p3 game cov-p3 2 3 6 4000
go cov-s3 game cov-s3 1 3 4 60000 -autoplaygames=2
go cov-p2 game cov-p2 2 2 9 8000 -autoplaygames=2
go cov-a2 game cov-a2 0 2 7 60000
go cov-a3 game cov-a3 0 3 2 62000
go cov-menus script cov-menus games/bigmoney/tools/coverage/menus.txt 1
