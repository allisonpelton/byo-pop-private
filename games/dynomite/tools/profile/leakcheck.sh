#!/bin/bash
# leakcheck.sh [build|run|report]...: the port's leak check (GAME_NOTES.md, "Performance"). "build" makes
# build/native-leak/dynomite.exe, a scratch build with leakhook.cpp linked in (never committed or shipped); "run" plays
# each pair of scripts below in it, once with one round of play and once with two, each ending on the same screen, and
# writes the live allocations at exit by caller (build/leak/<case>-<1|2>.txt); "report" prints, for each case, the
# callers whose live blocks grew with the second round, named by the function that holds the call (llvm-nm on the exe).
# Each run is one game process under rundesk with -fast -drawevery, one at a time. ONLY="<case> ..." runs those cases.
source "$(dirname "$0")/../identical/common.sh"
LM=${LLVM_MINGW_ROOT:-E:/retools/llvm-mingw-20261006-ucrt-x86_64}
OUTD=build/leak
EXE=build/native-leak/dynomite.exe
mkdir -p $OUTD
# case seed rng parts-for-one-round | parts-for-the-second-round (played after the first, from where it ends)
CASES=(
	"endless 2 3 title menu:endless shots:80:40 wait:300 text:Zed click:352:369 wait:200|click:400:400 wait:300 shots:80:40 wait:300 text:Amy click:352:369 wait:200"
	"stomped 4 5 title menu:stomped:pack shots:45:40 wait:300 text:Sam click:352:369 wait:200|click:400:400 wait:300 shots:45:40 wait:300 text:Sue click:352:369 wait:200"
	"fossil 6 7 title menu:fossil:strategic shots:55:40 wait:300 text:Fay click:352:369 wait:200|click:75:428 wait:200 menu:fossil:strategic shots:55:40 wait:300 text:Fox click:352:369 wait:200"
	"panic 8 9 title menu:fossil:panic shots:60:40 wait:300 text:Pat click:352:369 wait:200|click:75:428 wait:200 menu:fossil:panic shots:60:40 wait:300 text:Pam click:352:369 wait:200"
	"timetrial 10 11 title menu:timetrial:normal shots:150:40 wait:300|click:75:175 click:478:290 wait:300 shots:150:40 wait:300"
	"dialogs 2 3 title click:486:355 click:486:283 click:486:213 click:465:230 click:465:295 click:465:360 click:465:160 click:75:260 click:395:236 click:395:395 click:395:269 click:395:355 click:395:302 click:75:428|click:486:355 click:486:283 click:486:213 click:465:230 click:465:295 click:465:360 click:465:160 click:75:260 click:395:236 click:395:395 click:395:269 click:395:355 click:395:302 click:75:428"
	"save 12 13 title menu:endless shots:20:40 save:Sv cont shots:10:40 click:75:428 wait:200|menu:endless shots:20:40 save:Sw cont shots:10:40 click:75:428 wait:200"
)
for step in "${@:-build run report}"; do
case $step in
build)
	"$LM/bin/i686-w64-mingw32-clang++.exe" -O2 -c games/dynomite/tools/profile/leakhook.cpp -fno-exceptions -fno-rtti -o $OUTD/leakhook.obj || exit 1
	PATH="$APPDATA/Python/Python312/Scripts:$PATH" cmake -S . -B build/native-leak -G Ninja -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/llvm-mingw-i686.cmake" -DSDL2_DIR=E:/retools/SDL2-2.32.10/i686-w64-mingw32/lib/cmake/SDL2 \
		"-DCMAKE_EXE_LINKER_FLAGS=$(cygpath -m "$PWD/$OUTD/leakhook.obj")" > $OUTD/config.log 2>&1 || exit 1
	PATH="$APPDATA/Python/Python312/Scripts:$PATH" cmake --build build/native-leak --target dynomite > $OUTD/build.log 2>&1 || { tail $OUTD/build.log; exit 1; }
	echo "leakcheck: built $EXE" ;;
run)
	for c in "${CASES[@]}"; do
		set -- ${c%%|*}; NAME=$1; SEED=$2; R=$3; shift 3; ONE="$*"; TWO="${c#*|}"
		[ -z "$ONLY" ] || [[ " $ONLY " == *" $NAME "* ]] || continue
		for n in 1 2; do
			D=$OUTD/$NAME-$n; rm -rf "$D"; mkdir -p "$D/user"
			PARTS="$ONE"; [ $n = 2 ] && PARTS="$ONE $TWO"
			python $TOOLS/mkscript.py "$D/script.txt" 25000 "$R" $PARTS quit
			printf '%s\n' '[SOFTWARE\PopCap\Dynomite]' CustomCursors=dword:0 InProgress=dword:0 MusicVolume=dword:50 Muted=dword:0 \
				PreferredX=dword:-1 PreferredY=dword:-1 ScreenMode=dword:0 SfxVolume=dword:85 TimesExecuted=dword:5 \
				TimesPlayed=dword:0 WaitForVSync=dword:0 > "$D/user/registry.ini"
			python $TOOLS/mkhs.py "$D/user"
			LEAKHOOK_OUT="$(win "$PWD/$OUTD/$NAME-$n.txt")" "$RUNDESK" "$(win $EXE)" -gamedir=assets/dynomite -userdir="$(win "$D/user")" \
				-fast -drawevery -seed=$SEED -script="$(win "$D/script.txt")" > "$D/out.txt" 2>&1
			echo "leakcheck: $NAME-$n exit $? $(wc -l < $OUTD/$NAME-$n.txt) callers"
		done
	done ;;
report)
	"$LM/bin/llvm-nm.exe" --defined-only -n -C $EXE > $OUTD/nm.txt 2>/dev/null
	python - "$OUTD" <<'PY'
import bisect, sys, re
d = sys.argv[1]
syms = []
base = None
for l in open(d + '/nm.txt', encoding='utf-8', errors='replace'):
    m = re.match(r'([0-9a-f]{8}) [tT] (.*)', l)
    if m:
        syms.append((int(m.group(1), 16), m.group(2).strip()))
syms.sort()
addrs = [a for a, _ in syms]
IMAGE = 0x400000
def name(rva):
    a = IMAGE + rva
    i = bisect.bisect_right(addrs, a) - 1
    return syms[i][1] if i >= 0 else '?'
import os
cases = sorted({f.rsplit('-', 1)[0] for f in os.listdir(d) if re.match(r'.*-[12]\.txt$', f)})
for c in cases:
    def load(n):
        r = {}
        for l in open('%s/%s-%d.txt' % (d, c, n)):
            a, b, s = l.split()
            r[int(a, 16)] = (int(b), int(s))
        return r
    one, two = load(1), load(2)
    grew = [(k, one.get(k, (0, 0)), v) for k, v in two.items() if v[0] > one.get(k, (0, 0))[0]]
    grew.sort(key=lambda t: -(t[2][1] - t[1][1]))
    print('== %s: %d callers grew' % (c, len(grew)))
    for k, a, b in grew[:40]:
        print('  %+6d blocks %+9d bytes  %s' % (b[0] - a[0], b[1] - a[1], name(k)[:150]))
PY
	;;
esac
done
