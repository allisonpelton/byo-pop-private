#!/bin/bash
# run_all.sh [first|second|third]: the coverage runs of the original, one at a time (each waits for room first, see run.sh).
#   first   the phase-two scenarios (tools/identical/scenarios), made into origrun scripts by p2.py
#   second  the port bot's input (prep.sh makes the scripts): a classic game, a short one that ends in Abandon Hope, the tips,
#           the tutorial, an adventure, and the late game from a saved game
#   third   one run on the real clock (the sound code the virtual clock skips)
# Start it directly (not under rundesk) from the worktree.
R="$(pwd -W)"; T=games/mummymaze/tools; D=build/coverage/mummymaze/scripts
declare -A SEED=([classic21]=21 [classic8]=7 [tips]=5 [tutorial]=3 [adventure]=5 [late]=21)   # prep.sh's -seed for each
last() { grep -o "^at [0-9]*" "$1" | tail -1 | cut -d" " -f2; }
case "${1:-first}" in
first)
	mkdir -p build/p2
	for s in adv c6 d1 d2 d3 dark focus hs1 s1 tut; do
		[ -s build/coverage/mummymaze/hits/$s.txt ] && continue
		python -I $T/identical/p2.py make $T/identical/scenarios/$s.txt cov_$s build/p2 > /dev/null
		bash $T/coverage/run.sh $s "$R/build/p2/cov_$s.orig.txt" 21 $(( $(last build/p2/cov_$s.orig.txt) / 300 + 90 ))
	done ;;
second)
	for n in classic21 classic8 tips tutorial adventure; do
		[ -s build/coverage/mummymaze/hits/$n.txt ] && continue
		bash $T/coverage/run.sh $n "$R/$D/$n.script.txt" ${SEED[$n]} $(( ($(cat $D/$n.last) + 3000) / 300 + 60 ))
	done
	[ -s build/coverage/mummymaze/hits/late.txt ] || SAVE="$R/$D/late.classic.sav" bash $T/coverage/run.sh late "$R/$D/late.script.txt" ${SEED[late]} $(( ($(cat $D/late.last) + 3000) / 300 + 60 )) ;;
third)
	CLOCK=real bash $T/coverage/run.sh real "$R/build/p2/perfc6.orig.txt" 21 150 ;;
esac
