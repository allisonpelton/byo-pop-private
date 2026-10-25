#!/usr/bin/env bash
# The project's builds and tests, one summary line each: what a session needs to read to know whether a
# change is good. Every log goes to a file; open one only when its suite fails (docs/TOKEN_ECONOMY.md).
#
#   tools/check.sh [--out <dir>] <suite or set>...
#
# Suites (rough times on this machine):
#   build        tools/build.sh native, incremental (seconds to minutes)
#   build-web    tools/build.sh web
#   build-nov    tools/build.sh novariants (Bejeweled without its variant code, for the oracle)
#   unit         rulestest, modsettingstest and fwtest -hotkeytest, native (under a minute); fwtest is skipped
#                without assets/bejeweled; then fwtest -hotkeytest -placeholders with no game files at all
#   unit-web     rulestest and modsettingstest under Node (needs build-web)
#   verify       Bejeweled's headless verifier tests (games/bejeweled/tools/verify/run_tests.sh)
#   modmenu      the mod menu's scripted runs, all games (tools/modmenutest/run_tests.sh all)
#   livesplit    Bejeweled's, Big Money's and BookWorm's LiveSplit tests (BookWorm's once its branch is merged)
#   bookworm     BookWorm's bot: classic and action games, and a classic rerun with the same screenshots
#   bigmoney     Big Money's bot: one game
#   oracle       Bejeweled's oracle three ways (compare.sh --variants default,forcevariant,novariants); over
#                ten minutes, so never in a subagent; refuses to start while another oracle runs
# Sets: quick = build unit; games = bookworm bigmoney; all = every suite, builds first.
#
# Prints "PASS <suite> (<seconds>s): <summary>" or "FAIL <suite> (<seconds>s): <summary> -> <log>" for each,
# then "check: N passed, M failed". Exits 0 only when all passed. Logs: build/logs/check/<suite>.txt (and
# the suites' own output folders). Runs itself under build/native/rundesk.exe, so every game it starts is
# invisible, silent and low priority (CLAUDE.md); suites run one after another.
#
# The bots' expected results are below. A change that is meant to alter how a game plays updates them, in
# the same commit, with the reason.
BW_CLASSIC="level 8, score 58970"
BW_ACTION="level 14, score 152360"
BM_GAME1="score 66646"

set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 2
OUT="build/logs/check"
SUITES=()
while [ $# -gt 0 ]; do
	case "$1" in
		--out) OUT="$2"; shift 2 ;;
		--out=*) OUT="${1#--out=}"; shift ;;
		-h|--help) sed -n '2,/^BW_CLASSIC/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//'; exit 0 ;;
		quick) SUITES+=(build unit); shift ;;
		games) SUITES+=(bookworm bigmoney); shift ;;
		all) SUITES+=(build build-web build-nov unit unit-web verify modmenu livesplit bookworm bigmoney oracle); shift ;;
		build|build-web|build-nov|unit|unit-web|verify|modmenu|livesplit|bookworm|bigmoney|oracle) SUITES+=("$1"); shift ;;
		*) echo "check.sh: unknown suite '$1' (tools/check.sh --help)" >&2; exit 2 ;;
	esac
done
[ ${#SUITES[@]} -gt 0 ] || { echo "check.sh: name a suite or set (tools/check.sh --help)" >&2; exit 2; }

RUNDESK="$ROOT/build/native/rundesk.exe"
if [ -z "${BYOPOP_TEST_RUN:-}" ]; then
	# The native build would have to relink rundesk.exe while this script runs under it, which fails, so a run
	# with the build suite brings rundesk up to date first (and makes it in a fresh checkout).
	if [[ " ${SUITES[*]} " == *" build "* ]]; then
		mkdir -p "$OUT"
		tools/build.sh rundesk > "$OUT/rundesk.txt" 2>&1 || echo "check.sh: building rundesk failed -> $OUT/rundesk.txt" >&2
	fi
	[ -x "$RUNDESK" ] || { echo "check.sh: $RUNDESK is missing (tools/build.sh rundesk)" >&2; exit 2; }
	exec "$RUNDESK" bash "$ROOT/tools/check.sh" --out "$OUT" "${SUITES[@]}"
fi

mkdir -p "$OUT"
NODE="$(ls "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe 2>/dev/null | head -n 1)"
W="$(pwd -W)"
SUMMARY=""

# the last line of a file that matches a pattern, or the file's last line
lastof() { local l; l="$(grep -E "$2" "$1" 2>/dev/null | tail -n 1)"; [ -n "$l" ] || l="$(tail -n 1 "$1" 2>/dev/null)"; echo "${l#INFO: }"; }

build_suite() {  # <kind> <log>
	tools/build.sh "$1" > "$2" 2>&1
	local st=$?
	if [ $st -eq 0 ]; then SUMMARY="$1 build ok"; else SUMMARY="$(grep -m 1 -E 'error' "$2" | cut -c1-160)"; fi
	return $st
}

suite_build() { build_suite native "$1"; }
suite_build-web() { build_suite web "$1"; }
suite_build-nov() { build_suite novariants "$1"; }

suite_unit() {
	local st=0 a b c d
	(cd build/native && timeout 120 ./rulestest.exe) > "$1" 2>&1 || st=1
	a="$(lastof "$1" 'checks')"
	(cd build/native && timeout 120 ./modsettingstest.exe) > "$1.mst" 2>&1 || st=1
	b="$(lastof "$1.mst" 'checks')"
	# fwtest draws with Bejeweled's files: without them (a worktree before tools/setup_worktree.sh) it is skipped
	if [ -d assets/bejeweled ]; then
		timeout 120 "build/native/fwtest.exe" -hotkeytest > "$1.fw" 2>&1 || st=1
		c="fwtest $(lastof "$1.fw" 'hotkey: [0-9]+ failed')"
	else
		echo "fwtest -hotkeytest skipped: no assets/bejeweled (tools/setup_worktree.sh)" > "$1.fw"
		c="fwtest skipped (no assets/bejeweled)"
	fi
	# -placeholders with no game files at all: every image, font, sound and song a stand-in (platform/Placeholders.h)
	mkdir -p "$OUT/no-game-files"
	timeout 120 "build/native/fwtest.exe" -hotkeytest -placeholders -gamedir="$OUT/no-game-files" > "$1.ph" 2>&1 || st=1
	d="placeholders: $(lastof "$1.ph" 'placeholders: [0-9]+ game files? missing' | sed 's/^placeholders: //'), $(lastof "$1.ph" 'hotkey: [0-9]+ failed')"
	grep -q -E 'placeholders: [1-9][0-9]* game files? missing' "$1.ph" || st=1
	cat "$1.mst" "$1.fw" "$1.ph" >> "$1"; rm -f "$1.mst" "$1.fw" "$1.ph"
	SUMMARY="$a; $b; $c; $d"
	return $st
}

suite_unit-web() {
	[ -n "$NODE" ] || { SUMMARY="no node.exe under EMSDK_ROOT"; return 1; }
	local st=0 a b
	(cd build/web && "$NODE" rulestest.js) > "$1" 2>&1 || st=1
	a="$(lastof "$1" 'checks')"
	(cd build/web && "$NODE" modsettingstest.js) > "$1.mst" 2>&1 || st=1
	b="$(lastof "$1.mst" 'checks')"
	cat "$1.mst" >> "$1"; rm -f "$1.mst"
	SUMMARY="$a; $b"
	return $st
}

suite_verify() {
	games/bejeweled/tools/verify/run_tests.sh --out "$OUT/verify" > "$1" 2>&1
	local st=$?
	SUMMARY="$(lastof "$1" 'verify tests:')"
	return $st
}

suite_modmenu() {
	tools/modmenutest/run_tests.sh all > "$1" 2>&1
	local st=$?
	SUMMARY="$(lastof "$1" 'failed')"
	return $st
}

suite_livesplit() {
	local st=0 a b c="bookworm: no tests yet"
	# fresh folders: a game's user folder left by an earlier run (its play count, high scores) changes the games
	rm -rf "$OUT/livesplit-bejeweled" "$OUT/livesplit-bigmoney" "$OUT/livesplit-bookworm"
	games/bejeweled/tools/livesplit/run_tests.sh --out "$OUT/livesplit-bejeweled" > "$1" 2>&1 || st=1
	a="bejeweled: $(lastof "$1" 'PASSED|FAILED|failed')"
	bash games/bigmoney/tools/livesplit/run_tests.sh "$OUT/livesplit-bigmoney" > "$1.bm" 2>&1
	if [ $? -eq 0 ]; then b="bigmoney: passed"; else b="bigmoney: FAILED"; st=1; fi
	cat "$1.bm" >> "$1"; rm -f "$1.bm"
	if [ -f games/bookworm/tools/livesplit/run_tests.sh ]; then  # about 4 minutes
		bash games/bookworm/tools/livesplit/run_tests.sh "$OUT/livesplit-bookworm" > "$1.bw" 2>&1
		if [ $? -eq 0 ]; then c="bookworm: passed"; else c="bookworm: FAILED"; st=1; fi
		cat "$1.bw" >> "$1"; rm -f "$1.bw"
	fi
	SUMMARY="$a; $b; $c"
	return $st
}

bookworm_game() {  # <name> <mode>: a bot game; prints its last "game over" line
	local d="$OUT/bookworm/$1"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 900 "$W/build/native/bookworm.exe" -userdir="$d/user" -fast -seed=1 -autoplay=1 -autoplaymode="$2" \
		-autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" > "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -o -i -E 'game over[^;]*' "$d/log.txt" 2>/dev/null | tail -n 1
}

suite_bookworm() {
	local st=0 c a r
	c="$(bookworm_game classic classic)"; a="$(bookworm_game action action)"; r="$(bookworm_game classic2 classic)"
	{ echo "classic: $c"; echo "action: $a"; echo "classic rerun: $r"; } > "$1"
	case "$c" in *"$BW_CLASSIC"*) ;; *) st=1 ;; esac
	case "$a" in *"$BW_ACTION"*) ;; *) st=1 ;; esac
	if diff -rq "$OUT/bookworm/classic/shots" "$OUT/bookworm/classic2/shots" >> "$1" 2>&1; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="classic ${c#*-> inactive, }; action ${a#*-> inactive, }; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected classic $BW_CLASSIC, action $BW_ACTION)"
	return $st
}

suite_bigmoney() {
	local d="$OUT/bigmoney" g
	rm -rf "$d"; mkdir -p "$d"
	timeout 600 "$W/build/native/bigmoney.exe" -userdir="$d/user" -fast -seed=1 -autoplay=1 -autoplaymode=0 \
		-autoplayskill=3 -autoplaylog="$d/input.txt" > "$1" 2>&1
	g="$(grep -o -E 'game 1 over, score [0-9]+' "$d/input.txt" 2>/dev/null | tail -n 1)"
	SUMMARY="${g:-no game over line}"
	case "$g" in *"$BM_GAME1") return 0 ;; esac
	SUMMARY="$SUMMARY (expected $BM_GAME1)"
	return 1
}

suite_oracle() {
	local others
	others="$(powershell.exe -NoProfile -Command "@(Get-CimInstance Win32_Process | Where-Object { \$_.CommandLine -match 'compare\.sh' }).Count" 2>/dev/null | tr -d '\r')"
	if [ "${others:-0}" != 0 ]; then SUMMARY="another oracle is running; not started"; return 1; fi
	games/bejeweled/tools/oracle/compare.sh --variants default,forcevariant,novariants > "$1" 2>&1
	local st=$?
	SUMMARY="$(grep -E '^variant [a-z]+: [0-9]+ cases' "$1" | sed -E 's/^variant ([a-z]+): ([0-9]+) cases, ([0-9]+) differences, ([0-9]+) notes.*/\1 \2 cases \3 diff \4 notes/' | paste -sd ',' - | sed 's/,/, /g')"
	[ -n "$SUMMARY" ] || SUMMARY="$(tail -n 1 "$1")"
	return $st
}

PASSED=0
FAILED=0
for s in "${SUITES[@]}"; do
	log="$OUT/$s.txt"
	start=$SECONDS
	SUMMARY=""
	if "suite_$s" "$log"; then
		echo "PASS $s ($((SECONDS - start))s): $SUMMARY"
		PASSED=$((PASSED + 1))
	else
		echo "FAIL $s ($((SECONDS - start))s): $SUMMARY -> $log"
		FAILED=$((FAILED + 1))
	fi
done
echo "check: $PASSED passed, $FAILED failed"
[ $FAILED -eq 0 ]
