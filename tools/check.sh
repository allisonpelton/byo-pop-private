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
#                without assets/bejeweled; then fwtest -hotkeytest -placeholders with no game files at all; then
#                jp2test (JPEG 2000: the .jp2 images of assets/bookwormadv/images against re/bookwormadv/jp2ref, the loader's
#                rules for .jp2 files, synthetic and damaged files), skipped without assets/bookwormadv/images; and
#                songpostest on Bejeweled's and BookWorm's modules (skipped without their assets)
#   unit-web     rulestest and modsettingstest under Node (needs build-web); paktest (the page's pak reader), with
#                Bookworm Adventures' main.pak against assets/bookwormadv when both are there
#   verify       Bejeweled's headless verifier tests (games/bejeweled/tools/verify/run_tests.sh)
#   modmenu      the mod menu's scripted runs, all games (tools/modmenutest/run_tests.sh all)
#   livesplit    Bejeweled's, Big Money's and BookWorm's LiveSplit tests
#   bookworm     BookWorm's bot: classic and action games, and a classic rerun with the same screenshots
#   bigmoney     Big Money's bot: one game
#   noahsark     Noah's Ark's bot: action, strategy and puzzle games, an action rerun with the same screenshots, the
#                mods' tests (games/noahsark/tools/run_mod_tests.sh) and the high-score loader's (run_hiscores_test.sh)
#   atomica      Atomica's bot: a strategy game, an action game, and a strategy rerun with the same screenshots
#   sevenseas    Seven Seas' bot: a beginner's game from level 1, one from the bonus screen (level 10) and one from level 21,
#                and a rerun of the first with the same screenshots (about 15 seconds)
#   bookwormadv  Bookworm Adventures: the PopLua compiler's dialect checks and every compiled script of the game
#                loaded by the port's VM (bwa_luarun), every PopAnim loaded (bwa_pamtest), every cutscene played
#                to its end (bwa_swftest), a scripted run from the title to the main menu whose quit asks,
#                Play! stays and Quit quits, and the test profile's first chapter won and saved by the port's
#                "word attack" script command (under two minutes)
#   mummymaze    Mummy Maze's bot: a classic game of 8 chambers (the name box, the options and their dialogs photographed) and
#                a rerun with the same screenshots, a game from pyramid 3 (the tips), a whole classic game (225 chambers to the
#                victory screen), a whole adventure (15 pyramids, the map, the saved game) and the tutorial (about 1 minute)
#   tiptop       TipTop's bot: two regular games, a strategy game and a speed game, and a rerun of the regular ones
#                with the same screenshots (about two and a half minutes)
#   oracle       Bejeweled's oracle three ways (compare.sh --variants default,forcevariant,novariants); over
#                ten minutes, so never in a subagent; refuses to start while another oracle runs
# Sets: quick = build unit; games = bookworm bigmoney noahsark atomica sevenseas bookwormadv tiptop mummymaze; all = every suite, builds first.
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
NA_ACTION="level 4 done at update 17083: score 7622 mode stampede"
NA_STRATEGY="level 4 done at update 15469: score 4430 mode strategy"
NA_PUZZLE="game 1 over at update 17550: score 4830 level 1 moves 117"
AT_STRATEGY="game 1 over at update 6876: score 1600 level 2 moves 90"
AT_ACTION="update 20000: score 2480 level 3 moves 680 atoms 66"
SS_BEGINNER="game 2 over at update 5308: score 165 level 2 moves 23"
SS_BONUS="game 1 over at update 2286: score 330 level 12 moves 15"
SS_LATE="game 1 over at update 2285: score 390 level 22 moves 13"
MM_CLASSIC="game over: score 1500, list 1500 1000 900 800 700, the bot is number 1"
MM_TIPS="game over: score 1730, list 1730 1000 900 800 700, the bot is number 1"
MM_FULL="victory: score 251975, caught 0 times"
MM_ADVENTURE="victory: score 0, caught 49 times"
MM_TUTORIAL="tutorial done at update 6591"
TT_REGULAR="game 2 over at update 35544: level 7, score 37160, 352 moves"
TT_STRATEGY="game 1 over at update 2593: level 1, score 2100, 24 moves"
TT_SPEED="game 1 over at update 57249: level 11, score 78250, 1081 moves"

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
		games) SUITES+=(bookworm bigmoney noahsark atomica sevenseas bookwormadv tiptop mummymaze); shift ;;
		all) SUITES+=(build build-web build-nov unit unit-web verify modmenu livesplit bookworm bigmoney noahsark atomica sevenseas bookwormadv tiptop mummymaze oracle); shift ;;
		build|build-web|build-nov|unit|unit-web|verify|modmenu|livesplit|bookworm|bigmoney|noahsark|atomica|sevenseas|bookwormadv|tiptop|mummymaze|oracle) SUITES+=("$1"); shift ;;
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

# need_assets <game>...: a suite that plays a game fails at once when the game's files are not staged (a worktree
# before tools/setup_worktree.sh); without them every run sits on the game's missing-files dialog until its timeout
need_assets() {
	local g missing=""
	for g in "$@"; do [ -d "assets/$g" ] || missing="$missing assets/$g"; done
	[ -z "$missing" ] && return 0
	SUMMARY="missing$missing: run tools/setup_worktree.sh"
	return 1
}

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
	local st=0 a b c d e f
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
	# jp2test decodes Bookworm Adventures' .jp2 images against reference PNGs: staging's in re/bookwormadv/jp2ref, or
	# (older staging) PNGs beside them; skipped without the staged folder
	if [ -d assets/bookwormadv/images ]; then
		local ref=()
		[ -d re/bookwormadv/jp2ref/images ] && ref=("-ref=$W/re/bookwormadv/jp2ref/images")
		(cd build/native && timeout 120 ./jp2test.exe "$W/assets/bookwormadv/images" "${ref[@]}") > "$1.jp2" 2>&1 || st=1
		e="$(lastof "$1.jp2" 'jp2: [0-9]+ files, max channel difference')"
	else
		echo "jp2test skipped: no assets/bookwormadv/images (tools/setup_worktree.sh)" > "$1.jp2"
		e="jp2test skipped (no assets/bookwormadv)"
	fi
	# Mixer::GetSongPosition on Bejeweled's modules (libxmp-lite) and BookWorm's (libopenmpt), where they are staged
	f="songpos skipped (no assets/bejeweled or assets/bookworm)"
	if [ -d assets/bejeweled ] && [ -d assets/bookworm ]; then
		{ timeout 120 build/native/songpostest.exe assets/bejeweled music/1980-autonomus.s3m music/1984-network.s3m &&
		  timeout 120 build/native/songpostest.exe assets/bookworm music/bookworm1.mo3; } > "$1.sp" 2>&1 || st=1
		f="songpos: $(grep -c ', start at 2 ok' "$1.sp") of 3 songs ok"
	else
		echo "$f" > "$1.sp"
	fi
	cat "$1.mst" "$1.fw" "$1.ph" "$1.jp2" "$1.sp" >> "$1"; rm -f "$1.mst" "$1.fw" "$1.ph" "$1.jp2" "$1.sp"
	SUMMARY="$a; $b; $c; $d; $e; $f"
	return $st
}

suite_unit-web() {
	[ -n "$NODE" ] || { SUMMARY="no node.exe under EMSDK_ROOT"; return 1; }
	local st=0 a b
	(cd build/web && "$NODE" rulestest.js) > "$1" 2>&1 || st=1
	a="$(lastof "$1" 'checks')"
	(cd build/web && "$NODE" modsettingstest.js) > "$1.mst" 2>&1 || st=1
	b="$(lastof "$1.mst" 'checks')"
	# the page's pak reader (web/modpanel.js); with Bookworm Adventures installed and staged, its main.pak against the
	# staged files (BWA_PAK names another copy)
	local pak="${BWA_PAK:-C:/Program Files (x86)/PopCap Games/Bookworm Adventures Deluxe/main.pak}" c
	if [ -f "$pak" ] && [ -d assets/bookwormadv ]; then
		"$NODE" tools/paktest/paktest.js "$pak" assets/bookwormadv > "$1.pak" 2>&1 || st=1
		c="$(lastof "$1.pak" 'paktest: main.pak')"
	else
		"$NODE" tools/paktest/paktest.js > "$1.pak" 2>&1 || st=1
		c="$(lastof "$1.pak" 'paktest: synthetic') (no BWA main.pak or assets/bookwormadv)"
	fi
	grep -q '^paktest: 0 failed' "$1.pak" || st=1
	cat "$1.mst" "$1.pak" >> "$1"; rm -f "$1.mst" "$1.pak"
	SUMMARY="$a; $b; $c"
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
	need_assets bejeweled bigmoney || return 1
	[ -f games/bookworm/tools/livesplit/run_tests.sh ] && { need_assets bookworm || return 1; }
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
	need_assets bookworm || return 1
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
	need_assets bigmoney || return 1
	rm -rf "$d"; mkdir -p "$d"
	timeout 600 "$W/build/native/bigmoney.exe" -userdir="$d/user" -fast -seed=1 -autoplay=1 -autoplaymode=0 \
		-autoplayskill=3 -autoplaylog="$d/input.txt" > "$1" 2>&1
	g="$(grep -o -E 'game 1 over, score [0-9]+' "$d/input.txt" 2>/dev/null | tail -n 1)"
	SUMMARY="${g:-no game over line}"
	case "$g" in *"$BM_GAME1") return 0 ;; esac
	SUMMARY="$SUMMARY (expected $BM_GAME1)"
	return 1
}

noahsark_game() {  # <name> <mode> <cap>: a bot game; prints the last "level done" or "game over" line
	local d="$OUT/noahsark/$1"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/noahsark.exe" -userdir="$d/user" -fast -seed=1 -autoplay=1 -autoplaymode="$2" \
		-autoplaycap="$3" -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" > "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(level [0-9]+ done|game [0-9]+ over)' "$d/log.txt" 2>/dev/null | tail -n 1
}

suite_noahsark() {
	local st=0 a s p r m
	need_assets noahsark || return 1
	a="$(noahsark_game action action 20000)"; s="$(noahsark_game strategy strategy 20000)"
	p="$(noahsark_game puzzle puzzle 30000)"; r="$(noahsark_game action2 action 20000)"
	{ echo "action: $a"; echo "strategy: $s"; echo "puzzle: $p"; echo "action rerun: $r"; } > "$1"
	[ "$a" = "$NA_ACTION" ] || st=1
	[ "$s" = "$NA_STRATEGY" ] || st=1
	[ "$p" = "$NA_PUZZLE" ] || st=1
	if diff -rq "$OUT/noahsark/action/shots" "$OUT/noahsark/action2/shots" >> "$1" 2>&1 && [ "$a" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	mkdir -p "$OUT/noahsark"
	if bash games/noahsark/tools/run_mod_tests.sh "$OUT/noahsark/mods" > "$OUT/noahsark/mods.txt" 2>&1; then m="mods $(grep -c '^ok' "$OUT/noahsark/mods.txt") checks ok"; else m="MODS FAILED (see $OUT/noahsark/mods.txt)"; st=1; fi
	cat "$OUT/noahsark/mods.txt" >> "$1"
	if bash games/noahsark/tools/run_hiscores_test.sh "$OUT/noahsark/hiscores" > "$OUT/noahsark/hiscores.txt" 2>&1; then h="hiscores $(grep -c '^ok' "$OUT/noahsark/hiscores.txt") checks ok"; else h="HISCORES FAILED (see $OUT/noahsark/hiscores.txt)"; st=1; fi
	cat "$OUT/noahsark/hiscores.txt" >> "$1"
	SUMMARY="action ${a#level }; strategy ${s#level }; puzzle ${p#game }; $r; $m; $h"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected action '$NA_ACTION', strategy '$NA_STRATEGY', puzzle '$NA_PUZZLE')"
	return $st
}

atomica_game() {  # <name> <mode> <cap>: a bot game; prints its last "game over" or progress line
	local d="$OUT/atomica/$1"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/atomica.exe" -userdir="$d/user" -fast -seed=1 -autoplay=1 -autoplaymode="$2" 		-autoplaycap="$3" -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" > "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(level [0-9]+ done|game [0-9]+ over|update [0-9]+:)' "$d/log.txt" 2>/dev/null | tail -n 1 | sed 's/ (f8c.*//'
}

suite_atomica() {
	local st=0 s a r
	need_assets atomica || return 1
	s="$(atomica_game strategy strategy 20000)"; a="$(atomica_game action action 20000)"; r="$(atomica_game strategy2 strategy 20000)"
	{ echo "strategy: $s"; echo "action: $a"; echo "strategy rerun: $r"; } > "$1"
	[ "$s" = "$AT_STRATEGY" ] || st=1
	[ "$a" = "$AT_ACTION" ] || st=1
	if diff -rq "$OUT/atomica/strategy/shots" "$OUT/atomica/strategy2/shots" >> "$1" 2>&1 && [ "$s" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="strategy ${s#game }; action ${a#update }; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected strategy '$AT_STRATEGY', action '$AT_ACTION')"
	return $st
}

sevenseas_game() {  # <name> <seed> <difficulty> <start level> <games>: a bot game; prints the last "game N over" line
	local d="$OUT/sevenseas/$1"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/sevenseas.exe" -userdir="$d/user" -fast -seed="$2" -autoplay="$2" -autoplaydiff="$3" -autoplaylevel="$4" \
		-autoplaygames="$5" -autoplaycap=60000 -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" > "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^game [0-9]+ over' "$d/log.txt" 2>/dev/null | tail -n 1
}

suite_sevenseas() {
	local st=0 a b c r
	need_assets sevenseas || return 1
	a="$(sevenseas_game beginner 1 1 0 2)"; b="$(sevenseas_game bonus 2 2 10 1)"
	c="$(sevenseas_game late 3 3 21 1)"; r="$(sevenseas_game beginner2 1 1 0 2)"
	{ echo "beginner: $a"; echo "bonus: $b"; echo "late: $c"; echo "beginner rerun: $r"; } > "$1"
	[ "$a" = "$SS_BEGINNER" ] || st=1
	[ "$b" = "$SS_BONUS" ] || st=1
	[ "$c" = "$SS_LATE" ] || st=1
	if diff -rq "$OUT/sevenseas/beginner/shots" "$OUT/sevenseas/beginner2/shots" >> "$1" 2>&1 && [ "$a" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="beginner ${a#game }; bonus ${b#game }; late ${c#game }; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected beginner '$SS_BEGINNER', bonus '$SS_BONUS', late '$SS_LATE')"
	return $st
}

mummymaze_game() {  # <name> <bot arguments>...: a bot game; prints the last result line of its log
	local n="$1" d
	d="$OUT/mummymaze/$n"; shift
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/mummymaze.exe" -userdir="$d/user" -fast "$@" -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" 		> "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(game over|victory|tutorial done|stalled|cap reached)' "$d/log.txt" 2>/dev/null | tail -n 1
}

suite_mummymaze() {
	local st=0 a b c t r v
	need_assets mummymaze || return 1
	a="$(mummymaze_game classic -seed=1 -autoplay=7 -autoplaygames=1 -autoplaylevels=8 -autoplaycap=60000)"
	r="$(mummymaze_game classic2 -seed=1 -autoplay=7 -autoplaygames=1 -autoplaylevels=8 -autoplaycap=60000)"
	b="$(mummymaze_game tips -seed=3 -autoplay=5 -autoplaygames=1 -autoplaystart=3,0 -autoplaylevels=4 -autoplaycap=60000)"
	c="$(mummymaze_game full -seed=21 -autoplay=21 -autoplaygames=1 -autoplaylevels=250 -autoplaycap=3000000)"
	t="$(mummymaze_game tutorial -seed=1 -autoplay=3 -autoplaymode=tutorial -autoplaycap=100000)"
	v="$(mummymaze_game adventure -seed=1 -autoplay=5 -autoplaymode=adventure -autoplaygames=1 -autoplaylevels=400 -autoplaycap=8000000)"
	{ echo "classic: $a"; echo "classic rerun: $r"; echo "tips: $b"; echo "full: $c"; echo "adventure: $v"; echo "tutorial: $t"; } > "$1"
	[ "$a" = "$MM_CLASSIC" ] || st=1
	[ "$b" = "$MM_TIPS" ] || st=1
	[ "$c" = "$MM_FULL" ] || st=1
	[ "$t" = "$MM_TUTORIAL" ] || st=1
	[ "$v" = "$MM_ADVENTURE" ] || st=1
	if diff -rq "$OUT/mummymaze/classic/shots" "$OUT/mummymaze/classic2/shots" >> "$1" 2>&1 && cmp -s "$OUT/mummymaze/classic/log.txt" "$OUT/mummymaze/classic2/log.txt" && [ "$a" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="classic ${a#game over: }; tips ${b#game over: }; full ${c}; adventure ${v}; ${t}; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected classic '$MM_CLASSIC', tips '$MM_TIPS', full '$MM_FULL', tutorial '$MM_TUTORIAL', adventure '$MM_ADVENTURE')"
	return $st
}

suite_bookwormadv() {
	local log="$1" b="$W/build/native/bwa_luarun.exe" d l p s m c
	need_assets bookwormadv || return 1
	[ -x "$b" ] || { SUMMARY="no $b (build first)"; return 1; }
	d="$("$b" -gamedir=games/bookwormadv/tests/poplua dialect.lua 2>&1 | tee -a "$log" | tail -n 1)"
	l="$("$b" -load 2>&1 | tee -a "$log" | tail -n 1)"
	p="$("$W/build/native/bwa_pamtest.exe" 2>&1 | tee -a "$log" | tail -n 1 | tr -d '\r')"
	s="$("$W/build/native/bwa_swftest.exe" 2>&1 | tee -a "$log" | grep -c '; unsupported: none')"
	m="$(bookwormadv_menu "$log")"
	c="$(bookwormadv_chapter "$log")"
	SUMMARY="$d; $l; pam $p; swf $s of 4 to their end; menu: $m; chapter 1: $c"
	case "$d" in "dialect: "*" checks passed") ;; *) return 1 ;; esac
	bookwormadv_all "$l" || return 1
	bookwormadv_all "$p" || return 1
	[ "$s" = 4 ] || return 1
	[ "$m" = "quit at update 1310" ] || return 1
	[ "$c" = "6 words, won, saved" ] || return 1
	return 0
}

bookwormadv_all() {  # <"loaded N of M ...">: true when all of at least one loaded
	set -- $1
	[ "$1" = loaded ] && [ "$2" = "$4" ] && [ "$2" -gt 0 ]
}

bookwormadv_menu() {  # <log>: the test profile (tests/menu) from the title to the main menu, its quit asked, Play!, Quit
	local d="$OUT/bookwormadv/menu" u n
	rm -rf "$d"; mkdir -p "$d"
	cp -r games/bookwormadv/tests/menu/user "$d/user"
	# the quit sign, its dialog's Play! (it stays), the quit sign again and the dialog's Quit (it quits)
	printf '%s\n' "900 move 680 520" "920 click 680 520" "1000 screenshot $d/dialog.png" "1060 click 497 476" \
		"1150 screenshot $d/menu.png" "1200 move 680 520" "1220 click 680 520" "1310 click 325 476" "2000 quit" > "$d/script.txt"
	# (an absolute user folder: the game reads its profiles by GetAppDataFolder() paths)
	u="$d/user"; case "$u" in /*|[A-Za-z]:*) ;; *) u="$W/$u" ;; esac
	timeout 120 "$W/build/native/bookwormadv.exe" -userdir="$u" -script="$d/script.txt" -fast > "$d/stdout.txt" 2>&1
	cat "$d/stdout.txt" >> "$1"
	[ -f "$d/dialog.png" ] && [ -f "$d/menu.png" ] || { echo "no screenshots"; return; }
	n="$(grep -o 'Update Count = [0-9]*' "$d/stdout.txt" | tail -n 1)"
	echo "quit at update ${n#Update Count = }"
}

bookwormadv_chapter() {  # <log>: the test profile's first chapter, its battles won by the port's "word attack" (seed 1)
	local d="$OUT/bookwormadv/chapter" n k t u
	rm -rf "$d"; mkdir -p "$d"
	cp -r games/bookwormadv/tests/menu/user "$d/user"
	# Adventure, the map's Enter, the conversations clicked through (400 150), then the longest word and the
	# attack every 500 updates (the command waits for an idle board), with a click after each
	{ printf '%s\n' "900 move 230 300" "920 click 230 300" "2400 move 400 575" "2420 click 400 575" \
		"4000 click 400 150" "4400 click 400 150" "4800 click 400 150" "5200 click 400 150"
	  for k in $(seq 0 39); do t=$((5600 + k * 500)); printf '%s\n' "$t word attack" "$((t + 400)) click 400 150"; done
	  printf '%s\n' "25000 screenshot $d/map.png" "25600 quit"; } > "$d/script.txt"
	u="$d/user"; case "$u" in /*|[A-Za-z]:*) ;; *) u="$W/$u" ;; esac
	timeout 180 "$W/build/native/bookwormadv.exe" -userdir="$u" -script="$d/script.txt" -fast -seed=1 \
		> "$d/stdout.txt" 2>&1
	cat "$d/stdout.txt" >> "$1"
	grep -q "FATAL" "$d/stdout.txt" && { echo "failed: $(grep -m 1 -A 2 FATAL "$d/stdout.txt" | tail -n 1)"; return; }
	n="$(grep -c 'script: word [A-Z]' "$d/stdout.txt")"
	# the profile then holds the chapter as completed (its key, in the saved Lua table)
	if grep -q "CompletedChapters" "$d/user/users/Tester.bwa" && [ -f "$d/map.png" ]; then
		echo "$n words, won, saved"
	else
		echo "$n words, not won"
	fi
}

tiptop_game() {  # <name> <mode> <games>: bot games; prints the last "game N over" line
	local d="$OUT/tiptop/$1"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/tiptop.exe" -gamedir=assets/tiptop -userdir="$d/user" -fast -seed=2 -autoplay=7 \
		-autoplaymode="$2" -autoplaygames="$3" -autoplaycap=200000 -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" \
		> "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(game [0-9]+ over|update [0-9]+:)' "$d/log.txt" 2>/dev/null | tail -n 1
}

suite_tiptop() {
	local st=0 g s p r
	need_assets tiptop || return 1
	g="$(tiptop_game regular regular 2)"; s="$(tiptop_game strategy strategy 1)"; p="$(tiptop_game speed speed 1)"
	r="$(tiptop_game regular2 regular 2)"
	{ echo "regular: $g"; echo "strategy: $s"; echo "speed: $p"; echo "regular rerun: $r"; } > "$1"
	[ "$g" = "$TT_REGULAR" ] || st=1
	[ "$s" = "$TT_STRATEGY" ] || st=1
	[ "$p" = "$TT_SPEED" ] || st=1
	if diff -rq "$OUT/tiptop/regular/shots" "$OUT/tiptop/regular2/shots" >> "$1" 2>&1 && [ "$g" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="regular ${g#game }; strategy ${s#game }; speed ${p#game }; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected regular '$TT_REGULAR', strategy '$TT_STRATEGY', speed '$TT_SPEED')"
	return $st
}

suite_oracle() {
	local others
	need_assets bejeweled || return 1
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
