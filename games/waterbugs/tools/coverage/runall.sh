#!/bin/bash
# runall.sh: every coverage run of the original in turn (docs/COVERAGE.md; GAME_NOTES.md, "Coverage check" lists the runs). scripts.py
# writes the scripts and the hand-made saves, run.py runs one and waits for room among the machine's 3 game processes. Run it in the
# background from the worktree's root after `python -I tools/coverage.py list waterbugs`; the hits go to
# build/coverage/waterbugs/hits/<name>.txt (a run's log beside it) and one line for each run to build/coverage/waterbugs/runall.log.
# About 90 minutes of wall time when the machine has room.
D=build/coverage/waterbugs
S=$D/saves
G="python -I games/waterbugs/tools/coverage/scripts.py"
P="python -I games/waterbugs/tools/coverage/run.py"
mkdir -p $S $D/scripts
# a saved game for each run that starts on a late world (levels reached), and the ones with one life left or a game in progress
for r in 9 14 30 60 74 120 149 189 230 269 330 379 430 469 530 559 39; do $G save $r $S/save$r.dat; done
for r in 149 399 519; do $G save $r $S/saveover$r.dat 1 $((r + 1)) 5000; done
$G save 20 $S/saveresume.dat 4 21 500
RUNS="tour users typing leave play1 resumeyes resumeno deep9 deep14 deep30 deep60 deep120b deep230 deep330 deep430 deep530 deepmanb deepeasy deephard deepfocus"
RUNS="$RUNS more39 more74 more149 more189 more269 more379 more469 more559 over149 over399 over519 held74 held149 held189 held269"
for r in $RUNS; do $G $r $D/scripts; done
save_of() {  # the saved game a run starts from
  case $1 in
    tour|users|typing|leave|play1) ;;
    resumeyes|resumeno) echo "--cb $S/saveresume.dat" ;;
    deep120b) echo "--cb $S/save120.dat" ;;
    deepmanb|deepman) echo "--cb $S/save60.dat" ;;
    deepeasy) echo "--cb $S/save14.dat" ;;
    deephard) echo "--cb $S/save30.dat" ;;
    deepfocus) echo "--cb $S/save9.dat" ;;
    deep*) echo "--cb $S/save${1#deep}.dat" ;;
    more*) echo "--cb $S/save${1#more}.dat" ;;
    held*) echo "--cb $S/save${1#held}.dat" ;;
    over*) echo "--cb $S/saveover${1#over}.dat" ;;
  esac
}
{
for r in $RUNS; do $P $r $(save_of $r) --clock real --timeout 200; done
} 2>&1 | cut -c1-200 > $D/runall.log
