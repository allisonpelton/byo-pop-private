# pair.sh <name>: the original (origrun, virtual clock, the seed pinned and 10 ms frames by the copy's patches) and the
# runner on build/origrun/<name>.txt and build/wbs/<name>.txt (tour.py writes both from a tour of test/tours). SAVE=<cb.dat>
# starts both with that save. The shots are then compared with tools/shotdiff (see the tour's notes).
cd "$(dirname "$0")/../../../.."
mkdir -p build/wbs build/origrun
N=$1; shift
bash games/waterbugs/tools/play/or.sh $N build/origrun/$N.txt --patches "$(cygpath -wa tools/origrun/patches/waterbugs.txt)" --seed 0 --clock virtual "$@" > /dev/null
rm -rf build/wbs/$N; mkdir -p build/wbs/$N
[ -n "$SAVE" ] && cp "$SAVE" build/wbs/$N/cb.dat
# (the original takes the Windows user name, SYSTEM in the sandbox, for the first player: the runner's WB_USER)
WB_USER=SYSTEM WB_NOWARP=1 timeout 600 build/native/waterbugs_run.exe assets/waterbugs build/wbs/$N build/wbs/$N.txt > build/wbs/$N.out 2>&1
grep "virtual clock (pid" build/origrun/$N.out | sed 's/;.*//' | cut -c1-120
tail -1 build/wbs/$N.out
