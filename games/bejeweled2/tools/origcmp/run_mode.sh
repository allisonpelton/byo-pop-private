#!/bin/bash
# Phase one for one mode: the port plays it with real input (record.py), the original gets the same inputs on origrun's virtual clock
# (to_origrun.py, run_original.sh), and compare.py lines up their watches. One line comes out: IDENTICAL or DIFFERENT, with the logs
# in build/logs/bej2/<tag>/.
#   [HIRES=1] [NOSPARKLE=1] [REUSE=1] run_mode.sh <mode> <moves> [seed] [tag]    (REUSE=1: the tag's recording is kept, only the original runs again)
# The secret modes (twilight, hyper, cognito, finity) start from the player that make_profile.py makes (PROFILE=<its folder>, by default
# build/logs/bej2/profile, made on the first use), on both sides: the original has no `unlock all`.
# By default the port plays in true 2D at 640 by 480 and the original at 640 by 480 too (what it does on rundesk's desktop), where every
# change of the board's values falls on the same update in both. HIRES=1 plays both at 1024 by 768 (the original under dgVoodoo2, so
# that its 3D test passes); there the original's watch lines fall one update after the port's (its 3D loop), which compare.py reports
# as offsets, and an input that reaches a dialog (not the Escape key that closes a hint dialog) is given to the original one update earlier (to_origrun.py --dialog-shift -1: the
# board resumes its ticks, and so the phase of every clear, one update later in the original otherwise).
mode=$1; moves=$2; seed=${3:-1}; tag=${4:-$mode}
cd "$(dirname "$0")/../../../.." || exit 2
T=games/bejeweled2/tools/origcmp
R=build/logs/bej2/$tag
mkdir -p build/logs/bej2/og_$tag
[ -z "$HIRES" ] && export LOWRES=1
case "$mode" in twilight|hyper|cognito|finity)
  if [ -z "$PROFILE" ]; then
    [ -d build/logs/bej2/profile/users ] || python $T/make_profile.py --exe build/native/bejeweled2.exe --out build/logs/bej2/profile > /dev/null
    export PROFILE=build/logs/bej2/profile
  fi ;;
esac
[ -n "$REUSE" ] || PLAY_AT=${PLAY_AT:-500} bash $T/record_port.sh "$mode" "$moves" "$R" "$seed" > "$R.record.txt" 2>&1 || { echo "$tag: the recording failed (see $R.record.txt)"; exit 1; }
python $T/to_origrun.py "$R/events.json" "build/origrun/bej2_$tag.txt" ${HIRES:+--dialog-shift -1} > /dev/null  # in the 3D game a click on a dialog reaches the original one update after the port (--dialog-shift)
if [ -n "$HIRES" ]; then export ORIGRUN_ARGS="${ORIGRUN_ARGS:---dgvoodoo E:/retools/dgVoodoo2_87_5}"; fi  # 1024 by 768 needs the 3D test to pass: dgVoodoo2
if [ -n "$NOSPARKLE" ]; then export ORIGRUN_ARGS="$ORIGRUN_ARGS --patches build/origrun/bej2_nosparkle.txt"; fi  # the original and the port both leave out AddAlphaSparkle (GAME_NOTES, "Oddities")
export BOARD_BY=$(python -c "import json;print(json.load(open('$R/events.json'))['start'] + 100)")  # a board that comes later than this is a flake of the copy: run_original.sh runs again
bash $T/run_original.sh "build/origrun/bej2_$tag.txt" "build/logs/bej2/og_$tag/origrun.log" "$seed" ${TIMEOUT:-900}
last=$(grep -E '^at [0-9]+ mark' "build/origrun/bej2_$tag.txt" | tail -1 | awk '{print $2}')  # the original's script runs to the update at which the port's recording ended
python $T/compare.py "build/logs/bej2/og_$tag/origrun.log" "$R/game.log" --last "$last" > "$R.cmp.txt"
rc=$?
echo "$tag: $(tail -1 $R.cmp.txt) ($(head -1 $R.record.txt | cut -c1-110))"
exit $rc
