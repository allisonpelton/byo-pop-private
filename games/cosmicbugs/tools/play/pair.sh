# pair.sh <name>: the original (origrun, virtual clock, the seed pinned and 10 ms frames by the copy's patches) and the
# runner on build/origrun/<name>.txt and build/cbw/<name>.txt (gen.py writes both; games/cosmicbugs/test/plays has the
# series), then the comparison of their watches (cmp.py). SAVE=<cb.dat> starts both with that save.
cd "$(dirname "$0")/../../../.."
mkdir -p build/cbw build/origrun
N=$1; shift
bash games/cosmicbugs/tools/play/or.sh $N build/origrun/$N.txt --patches "$(cygpath -wa tools/origrun/patches/cosmicbugs.txt)" --seed 0 --clock virtual --watch-max 100000 "$@" > /dev/null
rm -rf build/cbw/$N; mkdir -p build/cbw/$N
[ -n "$SAVE" ] && cp "$SAVE" build/cbw/$N/cb.dat
CB_NOWARP=1 CB_WATCH=1 timeout 600 build/native/cosmicbugs_run.exe assets/cosmicbugs build/cbw/$N build/cbw/$N.txt > build/cbw/$N.out 2>&1
grep "virtual clock (pid" build/origrun/$N.out | sed 's/;.*//' | cut -c1-120
python games/cosmicbugs/tools/play/cmp.py build/origrun/$N.out build/cbw/$N.out
