#!/bin/bash
# run.sh <name> <scenario> [seed] [timeout s] [clock] [reuse run]: one coverage run of the original (docs/COVERAGE.md, cov.py) once the
# machine has room for another game process (at most 3: tools/gamecount.py) and no oracle runs. Never in the foreground of a session:
# run it in the background (it ends by itself).
#   games/atomica/tools/coverage/run.sh s2 games/atomica/tools/identical/s2.txt 1 240
# Several runs in a row:
#   run.sh --all     every scenario of tools/identical that the identical-play pass runs (the first report)
#   run.sh --record  the expert modes' scenarios x-expert-*.txt, recorded from the port's bot
#   run.sh --extra   the short runs for what those miss: tools/coverage/x-*.txt (named by the file without x- and .txt), the
#                    games from crafted saved games, tools/coverage/sv-*.txt (after --all, and after make-saves.sh has made the
#                    saves they start from), and three on the real clock for the
#                    sound code
cd "$(dirname "$0")/../../../.." || exit 2
# no oracle runs meanwhile (docs/COVERAGE.md): a process whose command line runs a compare.sh of another checkout
oracle() { powershell -NoProfile -Command "@(Get-CimInstance Win32_Process | Where-Object { \$_.CommandLine -like '*compare.sh*' -and \$_.CommandLine -notlike '*atomica-finish-e73ddc*' }).Count" | tr -d '\r'; }
room() { until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1 && [ "$(oracle)" = "0" ]; do sleep 20; done; }
if [ "$1" = "--all" ]; then
	for s in s1 s2 a1 sv g4 g5 p2-title p2-start p2-start2 p2-dialogs p2-focus p2-hs p2-d1 p2-g5; do
		bash "$0" "$s" "games/atomica/tools/identical/$s.txt" 1 300
	done
	# c1 continues the game that sv saved (compare.py's --savefile): it starts from sv's staged copy
	bash "$0" c1 games/atomica/tools/identical/c1.txt 1 300 virtual sv
	exit 0
fi
if [ "$1" = "--record" ]; then
	room
	EXPERT=1 bash games/atomica/tools/identical/record.sh strategy 7 5000 games/atomica/tools/coverage/x-expert-strategy.txt
	room
	EXPERT=1 bash games/atomica/tools/identical/record.sh action 7 5000 games/atomica/tools/coverage/x-expert-action.txt
	exit 0
fi
if [ "$1" = "--extra" ]; then
	for f in games/atomica/tools/coverage/x-*.txt; do
		n=$(basename "$f" .txt); n=${n#x-}
		bash "$0" "$n" "$f" 1 300
	done
	# the games that crafted saved games start (mksave.py, the scenarios' "# save: <kind>" line): they need sv's staged copy, which --all leaves
	for f in games/atomica/tools/coverage/sv-*.txt; do
		n=$(basename "$f" .txt)
		bash "$0" "$n" "$f" 1 300
	done
	# the real clock, so that the sound code (FMOD, through the silent stand-in) runs in time: focus lost and regained, a strategy
	# game and an action game
	bash "$0" real-focus games/atomica/tools/identical/p2-focus.txt 1 300 real
	bash "$0" real-a1 games/atomica/tools/identical/a1.txt 1 300 real
	bash "$0" real-s2 games/atomica/tools/identical/s2.txt 1 300 real
	exit 0
fi
room
python -I games/atomica/tools/coverage/cov.py "$@"
