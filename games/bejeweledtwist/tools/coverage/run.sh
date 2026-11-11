#!/bin/bash
# run.sh <name> <origrun script> [origrun options...]: one coverage run of Bejeweled Twist's original (docs/COVERAGE.md)
# on the host, where origrun puts the game under rundesk itself: the prepared copy (build/origrun/bjtlvl/game, the
# Steam copy with tools/origrun/patches/bejeweledtwist.txt) copied to build/origrun/cov-<name>/game, the virtual clock,
# seed 1234, 2D, the scripted cursor, and --coverage on the list `python -I tools/coverage.py list bejeweledtwist`
# writes. The game's own registry key is set to a window at the 768 art (the user allows changes to PopCap games'
# registry values). Extra options go to origrun after these (e.g. --arg -autoplay=classic, --timeout 600, or
# --clock real for the run that reaches the sound code). The hits go to build/coverage/bejeweledtwist/hits/<name>.txt,
# origrun's log beside them.
cd "$(dirname "$0")/../../../.."
NAME="$1"; S="$2"; shift 2
H=build/coverage/bejeweledtwist/hits; mkdir -p "$H"
R="cov-$NAME"; G="build/origrun/$R/game"
if [ ! -f "$G/BejeweledTwist.exe" ]; then
	mkdir -p "$G" && cp -r build/origrun/bjtlvl/game/. "$G/"
fi
rm -f "build/origrun/$R/coverage.txt"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
K='HKCU\Software\Steam\BejeweledTwist'
MSYS_NO_PATHCONV=1 reg add "$K" /v ScreenMode /t REG_DWORD /d 0 /f > /dev/null
MSYS_NO_PATHCONV=1 reg add "$K" /v ArtRes /t REG_DWORD /d 768 /f > /dev/null
CLOCK="--clock virtual"
case " $* " in *" --clock "*) CLOCK= ;; esac
build/origrun-bin/origrun.exe --exe "$(cygpath -w "$PWD/$G/BejeweledTwist.exe")" --run "$R" $CLOCK \
	--patches tools/origrun/patches/bejeweledtwist.txt --seed 1234 --3d off \
	--3d-key 'Software\Steam\BejeweledTwist' --3d-flag 0xe13bc0 --cursor scripted \
	--counter "[0xe0b698]+0x48c" --pending-draw "[0xe0b698]+0x462" --update-state "[0xe0b698]+0x490" \
	--draw-empty "[0xe0b698]+0x461" --coverage "$(cygpath -w "$PWD/build/coverage/bejeweledtwist/functions.txt")" \
	--script "$(cygpath -w "$PWD/$S")" "$@" > "$H/$NAME.log" 2>&1
rc=$?
cp "build/origrun/$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file (origrun exit $rc)"; exit 1; }
echo "$NAME: exit $rc, $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
