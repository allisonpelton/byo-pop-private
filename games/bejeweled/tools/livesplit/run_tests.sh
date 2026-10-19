#!/usr/bin/env bash
# LiveSplit auto splitter tests (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15, tests 3 to 6). LiveSplit
# itself is not needed. Paths are from the repository root.
#
#   games/bejeweled/tools/livesplit/run_tests.sh [--exe <bejeweled.exe>] [--out <dir>] [--jobs <n>]
#       [--checks-only]
#
# 1. Games with the livesplit oracle fixture mod (games/bejeweled/tools/oracle/mods/stage2b/on), each under
#    lsprobe (build/native/lsprobe.exe), which finds and reads the block the way the script does: scripted
#    games and two bot games. "rt" runs play in real time with the script's polling (refreshRate 100);
#    "fast" runs play with -fast while lsprobe polls every millisecond (the stress run: reads overlap the
#    game's writes, many updates fall between two polls). One run without the mod (the block stays
#    inactive). Each run directory: ls.txt (the game's -livesplitlog), probe.txt (lsprobe's log), stdout,
#    stderr, user/.
# 2. check_lslog.py: every snapshot lsprobe accepted is one the game wrote, at the address it logged.
# 3. check_asl_offsets.py: the script's magic and field table against "lsprobe -dumplayout"; the magic once
#    in the exe's .data.
# 4. asltest.exe (built here with the .NET Framework's csc.exe, C# 5): the script compiles as LiveSplit
#    compiles it.
# 5. lsdecide.py: every scenario's events from the game log and from the probe log, as expected, and the
#    real script (through asltest.exe) decides the same on the same polls.
# Every game runs on an invisible desktop (tools/rundesk), at most --jobs (default 2) at a time; the real-time
# runs take about 20 s each. --checks-only reruns 2 to 5 on the runs already in <out>. Exit status 0 only if
# every check passes. Keep <out> under build/ (it holds user dirs made from the game files).
set -u

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
GAME="$ROOT/games/bejeweled"
LS="$GAME/tools/livesplit"
EXE="$ROOT/build/native/bejeweled.exe"
OUT="$ROOT/build/livesplit-test"
JOBS=2
CHECKS_ONLY=0
ARGS=("$@")
while [ $# -gt 0 ]; do
	case "$1" in
		--exe) EXE="$2"; shift 2 ;;
		--out) OUT="$(mkdir -p "$2" && cd "$2" && pwd)"; shift 2 ;;
		--jobs) JOBS="$2"; shift 2 ;;
		--checks-only) CHECKS_ONLY=1; shift ;;
		*) sed -n '2,27p' "$0"; exit 2 ;;
	esac
done

RUNDESK="$ROOT/build/native/rundesk.exe"
if [ -z "${BEJ_TEST_DESKTOP:-}" ]; then
	[ -x "$RUNDESK" ] || { echo "run_tests.sh: build $RUNDESK first (cmake --build build/native --target rundesk)" >&2; exit 2; }
	export BEJ_TEST_DESKTOP=1
	exec "$RUNDESK" bash "$0" "${ARGS[@]}"
fi

winpath() { if command -v cygpath > /dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi; }
# csc.exe takes "a/b" for an option: Windows separators for it
winpath_bs() { if command -v cygpath > /dev/null 2>&1; then cygpath -w "$1"; else echo "$1"; fi; }
PY="${PYTHON:-python}"
EXE="$(cd "$(dirname "$EXE")" && pwd)/$(basename "$EXE")"
PROBE="$ROOT/build/native/lsprobe.exe"
ASLTEST="$ROOT/build/native/asltest/asltest.exe"
CSC="${CSC:-C:/Windows/Microsoft.NET/Framework/v4.0.30319/csc.exe}"
ASL="$LS/BejeweledDeluxe-byopop.asl"
PT="$GAME/tools/oracle/scripts"
MOD="$GAME/tools/oracle/mods/stage2b/on"
ASSETS_W="$(winpath "$ROOT/assets/bejeweled")"
for f in "$EXE" "$PROBE"; do [ -f "$f" ] || { echo "missing $f"; exit 2; }; done
mkdir -p "$OUT"

# run <name> rt|fast <seed> <script or -> [from=<run>] [nomod] [arg=<switch>]...
run() {
	local name=$1 speed=$2 seed=$3 script=$4
	shift 4
	local dir="$OUT/$name" from="" modsdir="$(winpath "$MOD")" args=() probe=()
	for opt in "$@"; do
		case "$opt" in
			from=*) from="${opt#from=}" ;;
			nomod) modsdir="mods" ;;
			arg=*) args+=("${opt#arg=}") ;;
		esac
	done
	rm -rf "$dir"
	mkdir -p "$dir/shots" "$dir/mods"
	if [ -n "$from" ]; then
		cp -r "$OUT/$from/user" "$dir/user"
		rm -rf "$dir/user/livesplit"
	else
		mkdir -p "$dir/user"
		printf '; test settings\n\n[SOFTWARE\Steam\Bejeweled]\nScreenMode=dword:0\n' > "$dir/user/registry.ini"
	fi
	[ "$script" != - ] && { cp "$script" "$dir/script.txt"; args+=("-script=script.txt"); }
	if [ "$speed" = fast ]; then
		args+=(-fast)
		probe=(-interval=1 -scan=5)
	fi
	(cd "$dir" && timeout "${LS_TIMEOUT:-900}" "$PROBE" -log=probe.txt "${probe[@]}" -- "$EXE" -gamedir="$ASSETS_W" -userdir=user \
		-modsdir="$modsdir" -seed="$seed" -livesplitlog=ls.txt "${args[@]}" > stdout.txt 2> stderr.txt; echo $? > exit.txt)
	echo "  $name ($speed): exit $(cat "$dir/exit.txt"), $(grep -c '^snap' "$dir/probe.txt" 2> /dev/null || echo 0) snapshots"
}

# at most $JOBS runs at a time
pool() {
	while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n; done
	"$@" &
}

if [ $CHECKS_ONLY -eq 0 ]; then
	echo "games ($JOBS at a time):"
	pool run main fast 1 "$PT/playtest.txt"
	pool run level rt 3 "$PT/playtest_level.txt"
	pool run newgame rt 1 "$PT/stage2/seed_newgame.txt" arg=-gameseed=4242
	pool run mode_switch rt 1 "$PT/stage0/mode_switch.txt"
	pool run off rt 3 "$PT/playtest_level.txt" nomod
	pool run nomoves fast 1 "$PT/stage0/nomoves_normal.txt"
	pool run timed_fallout fast 1 "$PT/stage0/timed_fallout.txt"
	pool run bot_normal fast 1 - arg=-gameseed=4242 arg=-autoplay=1
	pool run bot_timed fast 1 - arg=-autoplay=2 arg=-autoplaytimed
	wait
	run load fast 1 "$PT/playtest_load.txt" from=main
fi

status=0
echo
echo "probe reads (check_lslog.py):"
for d in "$OUT"/*/; do
	name=$(basename "$d")
	[ "$name" = off ] && continue
	printf '  %-14s ' "$name"
	"$PY" "$(winpath "$LS/check_lslog.py")" "$(winpath "$d/probe.txt")" "$(winpath "$d/ls.txt")" "$(winpath "$d/stderr.txt")" || status=1
	[ "$(cat "$d/exit.txt")" = 0 ] || { echo "  $name: the game exited with $(cat "$d/exit.txt")"; status=1; }
done
# without the mod: nothing writes the block (no ls.txt), the probe finds it inactive (self 0) until the end
printf '  %-14s ' off
if [ ! -s "$OUT/off/ls.txt" ] && grep -q '^found' "$OUT/off/probe.txt" && ! grep '^snap' "$OUT/off/probe.txt" | grep -qv ' seq=0 active=0 .* self=00000000 '; then
	echo "ok: the block found, never written (seq 0, active 0, self 0 in every snapshot), no -livesplitlog lines"
else
	echo "FAIL: the block of a game without the mod was written or not found"
	status=1
fi

echo
echo "layout (check_asl_offsets.py):"
"$PROBE" -dumplayout > "$OUT/layout.txt"
"$PY" "$(winpath "$LS/check_asl_offsets.py")" "$(winpath "$ASL")" "$(winpath "$OUT/layout.txt")" "$(winpath "$EXE")" || status=1
# the livesplit mod carries a copy of the script for loading by hand (make_example_mods.py --only livesplit)
if cmp -s "$ASL" "$GAME/mods/livesplit/$(basename "$ASL")"; then
	echo "ok: games/bejeweled/mods/livesplit has the same script"
else
	echo "FAIL: games/bejeweled/mods/livesplit/$(basename "$ASL") differs from the script (make_example_mods.py --only livesplit)"
	status=1
fi

echo
echo "script compile (asltest.exe, the .NET Framework's C# 5 compiler):"
mkdir -p "$(dirname "$ASLTEST")"
"$CSC" -nologo -out:"$(winpath_bs "$ASLTEST")" -r:Microsoft.CSharp.dll -r:System.Windows.Forms.dll "$(winpath_bs "$LS/asltest/asltest.cs")" \
	| grep -v -e '^$' -e 'language versions' -e 'no longer the latest' -e 'LinkID' -e 'Compiler version' -e 'for C# 5' -e 'Copyright'
"$ASLTEST" "$(winpath "$ASL")" -compileonly || status=1

echo
echo "decisions (lsdecide.py, and the script through asltest.exe):"
rm -f "$OUT"/*/*.polls.txt
"$PY" "$(winpath "$LS/lsdecide.py")" run "$(winpath "$OUT")" --asltest "$(winpath "$ASLTEST")" --asl "$(winpath "$ASL")" \
	--layout "$(winpath "$OUT/layout.txt")" || status=1

echo
[ $status -eq 0 ] && echo "ALL PASSED" || echo "FAILURES"
exit $status
