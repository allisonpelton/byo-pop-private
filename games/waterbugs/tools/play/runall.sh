# runall.sh: the screen tours of games/waterbugs/test/tours, each on the original (origrun in Windows Sandbox: one sandbox at a
# time on the machine, so a run may queue for a long while) and on the port's runner, from build/wbs/*.txt and build/origrun/*.txt
# (gentours.py, then tour.py, write them). The saves: end starts from build/wbs/save14.dat, worlds from save539.dat
# (games/waterbugs/tools/coverage/scripts.py save).
#   build/native/rundesk.exe is not needed: pair.sh starts the runner under it itself where it must.
cd "$(dirname "$0")/../../../.."
for n in "$@"; do
	case $n in end) export SAVE=build/wbs/save14.dat ;; worlds) export SAVE=build/wbs/save539.dat ;; *) unset SAVE ;; esac
	bash games/waterbugs/tools/play/pair.sh $n --sandbox-wait 7200 > build/wbs/pair-$n.txt 2>&1
	echo "$n: exit $?" >> build/wbs/runall.txt
done
