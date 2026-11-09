#!/bin/bash
# runall.sh: every coverage run of the original in turn (docs/COVERAGE.md; scripts.py writes the scripts, run.py runs one and waits for
# room among the machine's 3 game processes). Run it in the background from the worktree's root; the hits go to
# build/coverage/waterbugs/hits/<name>.txt and one line for each run to build/coverage/waterbugs/runall.log.
S=build/coverage/waterbugs/saves
P="python -I games/waterbugs/tools/coverage/run.py"
{
for r in tour users; do $P $r --clock real --timeout 120; done
$P play1 --clock real --timeout 150
for r in 9 14 30 60 120 230 330 430 530; do $P deep$r --cb $S/save$r.dat --clock real --timeout 150; done
$P deepman --cb $S/save60.dat --clock real --timeout 150
$P deepeasy --cb $S/save14.dat --clock real --timeout 150
$P deephard --cb $S/save30.dat --clock real --timeout 150
$P deepfocus --cb $S/save9.dat --clock real --timeout 150
$P resumeyes --cb $S/saveresume.dat --clock real --timeout 150
$P resumeno --cb $S/saveresume.dat --clock real --timeout 150
} 2>&1 | cut -c1-200 > build/coverage/waterbugs/runall.log
