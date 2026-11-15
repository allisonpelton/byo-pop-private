#!/bin/bash
# The coverage runs of GAME_NOTES.md, "Coverage check", one after another (each run waits for room: docs/COVERAGE.md). From the
# worktree root, with build/origrun-bin built and `python -I tools/coverage.py list noahsark` run:
#   bash games/noahsark/tools/coverage/all.sh [run names...]     (no names: every run)
# The scripts of the bot's games come from mkbot.sh, the tours from tours/*.txt through tools/tour.py (their shots dropped: the
# host takes none). The hits are build/coverage/noahsark/hits/<run>.txt; the summary lines go to build/coverage/noahsark/all.log.
cd "$(dirname "$0")/../../../.."
C=games/noahsark/tools/coverage
S=build/coverage/noahsark/scripts
mkdir -p "$S"
log() { echo "$(date +%H:%M:%S) $*" >> build/coverage/noahsark/all.log; }
bot() {   # bot <name> <mode> <diff> <cap> [quit update] [seed]: the bot's game, then its coverage run and the comparison of the two
	bash $C/mkbot.sh "$1" "$2" "$3" "$4" "${6:-1}" "${5:-}" > /dev/null 2>&1
	log "$(bash $C/run.sh "$1" "$S/$1.orig.txt" "${6:-1}" 2>&1 | tail -1)"
	log "$1: $(python -I games/noahsark/tools/origrun_replay.py compare "build/coverage/noahsark/hits/$1.log" "$S/$1.state" 2>&1 | tail -1)"
}
tour() {  # tour <name>: a tour from tours/<name>.txt
	python -I games/noahsark/tools/tour.py "$C/tours/$1.txt" "$S/tour_$1" > /dev/null
	grep -v -e ' shot ' -e ' state ' "$S/tour_$1/orig.txt" > "$S/$1.orig.txt"
	log "$(bash $C/run.sh "$1" "$S/$1.orig.txt" 1 2>&1 | tail -1)"
}
export REPLAY_TEXT_DELAY=3   # the name entry is typed 3 updates later in the original than in the port (docs/IDENTICAL_PLAY.md)
RUNS="${*:-act_e str_e puz_e act_h str_h puz_m lose_act lose_str rank_act rank_puz stamp dialogs rt drag_puz drag_act}"
for r in $RUNS; do
	case "$r" in
	act_e)    bot act_e action 0 17400 ;;
	str_e)    bot str_e strategy 0 19600 ;;
	puz_e)    bot puz_e puzzle 0 12000 ;;
	act_h)    bot act_h action 8 9000 ;;
	str_h)    bot str_h strategy 8 12000 ;;
	puz_m)    bot puz_m puzzle 4 12000 ;;
	lose_act) STOP=800 bot lose_act action 0 15000 ;;
	lose_str) STOP=800 bot lose_str strategy 0 20000 ;;
	rank_act) STOP=5500 bot rank_act action 0 26000 14000 ;;   # a score that ranks (1034), then the flood: the name, the list
	rank_puz) STOP=3000 bot rank_puz puzzle 0 40000 12800 ;;
	stamp)    log "$(bash $C/stamp.sh 2>&1 | tail -1)" ;;
	# boards with no move left (2026-11-14; not in the default list, which is the 15 runs of 2026-11-08). Hard strategy games whose
	# first level, the one a replay keeps in step, comes to a board with no pair: the rest is abandoned pair by pair (0x4074c0, then
	# 0x4067e0 every 78 updates). Seed 2: two pairs, then a pair can be joined again. Seed 4: one pair, and later one more and the
	# round's end with nothing left to abandon. Seed 7: one pair, later three more, the points taken at their cap of 60 from the
	# fifth animal on. Seed 8: eleven pairs in four stretches, the score down to 0. And a hard puzzle game that runs out of swaps
	# (0x4074c0's other branch, then the game over). The menu clicks come at 15001 and after, since the original's play link came
	# up as late as update 3160 that day (mkbot.sh, MENU_AT).
	nm_str2)  MENU_AT=15001 bot nm_str2 strategy 8 18400 "" 2 ;;
	nm_str4)  MENU_AT=15001 bot nm_str4 strategy 8 18300 "" 4 ;;
	nm_str7)  MENU_AT=15001 bot nm_str7 strategy 8 18300 "" 7 ;;
	nm_str8)  MENU_AT=15001 bot nm_str8 strategy 8 18300 "" 8 ;;
	nm_puz)   MENU_AT=15001 bot nm_puz puzzle 8 17700 "" 1 ;;
	dialogs)  tour dialogs ;;
	# the drags (origrun's moves carry the held button since 865a602e): the board's cells and what the drag does to them, in the puzzle; the link,
	# in the action mode, and the Options sliders (the tours' own comments)
	drag_puz) WATCHES='[[0x4d125c]+0x408]+0x310|[[0x4d125c]+0x408]+0x308|[[[0x4d125c]+0x408]+0x5c]+0x18|[[[0x4d125c]+0x408]+0x80]+0x18' tour drag_puz ;;
	drag_act) WATCHES='[[0x4d125c]+0x408]+0x310|[[0x4d125c]+0x408]+0x308|[[0x4d125c]+0x408]+0xdec|[0x4d125c]+0x474' tour drag_act ;;
	rt)       # the real clock and the silent stand-in, so that the sound code runs (75 s)
		cp build/nt/perf/orig.txt "$S/rt.orig.txt"
		log "$(bash $C/run.sh rt "$S/rt.orig.txt" 1 --clock real --timeout 300 2>&1 | tail -1)" ;;
	esac
done
log done
touch build/coverage/noahsark/all.done
