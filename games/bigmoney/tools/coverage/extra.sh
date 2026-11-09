#!/usr/bin/env bash
# The short runs that the identical-play games miss (docs/COVERAGE.md): a saved game continued and declined, an Action game nobody plays, the window closed
# from the title and the selector, and two runs on the real clock with the silent stand-in so that the sound code runs.
# build/coverage/bigmoney/save-a1.dat is the savegame.dat that cov-a1 (an Action game closed in the middle) left behind.
cd "$(dirname "$0")/../../../.." || exit 1
R=games/bigmoney/tools/coverage/run.sh; C=games/bigmoney/tools/coverage
S="$PWD/build/coverage/bigmoney/save-a1.dat"
go() { bash "$R" "$@" | tee -a build/coverage/bigmoney/runs.txt; }
ORIG_SAVE="$S" go script cov-cont-yes "$C/cont-yes.txt" 1
ORIG_SAVE="$S" go script cov-cont-no "$C/cont-no.txt" 1
go script cov-quit-title "$C/quit-title.txt" 1
go script cov-quit-selector "$C/quit-selector.txt" 1
go script cov-idle-action "$C/idle-action.txt" 1
ORIG_CLOCK=real ORIG_TIMEOUT=300 go script cov-real-menus "$C/menus.txt" 1
ORIG_CLOCK=real ORIG_TIMEOUT=300 go script cov-real-a1 build/bmcmp/cov-a1-bot/orig.txt 1
