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
#                rules for .jp2 files, synthetic and damaged files), skipped without assets/bookwormadv/images;
#                songpostest on Bejeweled's and BookWorm's modules (skipped without their assets); spectrumtest
#                (the music player's FFT against BASS 2.3's on the same tones); and soundtest (sounds kept as 16-bit at
#                their own rate, voices against the old stereo-float pipeline); and shotdiff's selftest (tools/shotdiff:
#                the screenshot comparison on synthetic pictures, its pages and its marks server)
#   unit-web     rulestest and modsettingstest under Node (needs build-web); paktest (the page's pak reader), with
#                Bookworm Adventures' main.pak against assets/bookwormadv when both are there; profilestest (the page's
#                settings and players code, web/settings.js and web/profiles.js, on a fake localStorage), savestest (the
#                save backup's zip, paths and merge, web/saves.js), saveimporttest (the import of saves from an installed game:
#                platform/saveimport.json, the .reg reader and the conversion, web/saveimport.js), chattest (the chat's username,
#                text and rate rules, its stored history and local source, and that a message goes on the page as text only,
#                web/chat.js) and a11ycheck (tools/a11ycheck.js: the site's pages in the build, as index.html lists them; each page's
#                landmarks, headings, names, references, live regions and contrast). The build folder is build/web;
#                BYOPOP_WEB_DIR=build/web-test names another
#   verify       Bejeweled's headless verifier tests (games/bejeweled/tools/verify/run_tests.sh)
#   modmenu      the mod menu's scripted runs, all games (tools/modmenutest/run_tests.sh all)
#   livesplit    Bejeweled's, Big Money's and BookWorm's LiveSplit tests
#   bookworm     BookWorm's bot: classic and action games, and a classic rerun with the same screenshots
#   bigmoney     Big Money's bot: one game
#   noahsark     Noah's Ark's bot: action, strategy and puzzle games, an action rerun with the same screenshots, the
#                mods' tests (games/noahsark/tools/run_mod_tests.sh) and the high-score loader's (run_hiscores_test.sh)
#   atomica      Atomica's bot: a strategy game, an action game, and a strategy rerun with the same screenshots
#   alchemy      Alchemy's bot: a strategic game, a time trial game and the tutorial, a strategic rerun with the same
#                screenshots, and a game quit half way whose saved game the next run loads (about 20 seconds)
#   sevenseas    Seven Seas' bot: a beginner's game from level 1, one from the bonus screen (level 10) and one from level 21,
#                and a rerun of the first with the same screenshots (about 15 seconds)
#   bookwormadv  Bookworm Adventures: the PopLua compiler's dialect checks and every compiled script of the game
#                loaded by the port's VM (bwa_luarun), every PopAnim loaded (bwa_pamtest), every cutscene played
#                to its end (bwa_swftest), a scripted run from the title to the main menu whose quit asks,
#                Play! stays and Quit quits, the test profile's first chapter won and saved by the port's
#                "word attack" script command, a run through the screens off the main menu (the credits, the
#                players dialog, the mini-games' menu, the arena to its first word), and the media player's
#                spectrum of a song, both ways, with the mouse over the book (about two and a half minutes)
#   pixelus      Pixelus Deluxe's grid replay test (every puzzle of the game solved by its recorded solution), a scripted tour of the
#                options, credits and how-to-play screens, the bot's first 25 puzzles and a rerun with the same screenshots,
#                and the bot's whole game (210 puzzles in all 12 temples, to the ending; about 2 minutes)
#   mummymaze    Mummy Maze's bot: a classic game of 8 chambers (the name box, the options and their dialogs photographed) and
#                a rerun with the same screenshots, a game from pyramid 3 (the tips), a whole classic game (225 chambers to the
#                victory screen), a whole adventure (15 pyramids, the map, the saved game) and the tutorial (about 1 minute)
#   tiptop       TipTop's bot: two regular games, a strategy game and a speed game, and a rerun of the regular ones
#                with the same screenshots (about two and a half minutes)
#   ningpo       Ning Po Mahjong: a scripted game (the cheat's matches until no pair is left, the name entry, the best
#                times) and a rerun with the same screenshots and best time (under a minute)
#   wordharmony  Word Harmony: a THINK game from a saved game (a ball dragged into a word, the word sent, the level
#                ended, level 2 saved) and a TSUNAMI game saved after 20 seconds, each run twice with the same
#                screenshots (about 20 seconds)
#   cosmicbugs   Cosmic Bugs: both art banks, the fonts and every animation the game ships loaded
#                (cosmicbugs_banktest), a scripted game played headless (cosmicbugs_run: two lines on the first
#                board) and a rerun with the same screenshots, and the windowed exe twice with the same clicks: the
#                runner's state, the same screen (under a minute)
#   bookwormadv2 Bookworm Adventures Volume 2: PopLua on the game's 462 scripts (bwa2_luarun): the compiler's
#                dialect checks, every compiled script loaded and its main chunk run, and the 8 scripts/common
#                sources recompiled instruction for instruction against their shipped chunks; fails on a bad exit
#                status, a failed load or a count that is not the whole set; the Flash player's self test and its
#                five movies played to their end (bwa2_swftest); a new player's run from the title through the
#                prologue's tutorial (PLAY spelled, then the attack) and the same profile's run to the main menu
#                and the mini-games' menu, and Golden Coins' first words (the game's own runs join it as the
#                port grows; about 50 seconds)
#   waterbugs    Water Bugs: the fonts and every animation the game ships loaded
#                (waterbugs_banktest), a scripted game played headless (waterbugs_run: a beam across the first
#                board, two state lines) and a rerun with the same screenshots, and the windowed exe twice with the
#                same clicks: the runner's state, the same screen (under a minute)
#   rocketmania  Rocket Mania Deluxe: the loading screen, the menu, a hover hint, the difficulty dialog and a new game
#                started from a script (games/rocketmania/tools/menu_test.txt), four screenshots that must differ (under a
#                minute)
#   dynomite     Dynomite Deluxe: a scripted Stomped Puzzle (dynomite_run: five shots and the board's state, then the
#                save dialog's first slot and the name's Save) and a rerun with the same screens and the same saved
#                game but for its tick counts, and that saved game continued from the main menu with the board as it
#                was saved (dynomite_load) (under a minute)
#   bonnies      Bonnie's Bookstore Deluxe: a tour from a script (games/bonnies/tools/tour_test.txt): a new player,
#                the classic game's tutorial words, the game's options, high scores, credits and help, the main menu's
#                options, an action game paused by a focus loss and played until its time is up, and Quit; a rerun
#                with the same screens and saves, which are the frozen baseline's (and build/baseline-bonnies/
#                bonnies.exe's, when there is one) (under a minute)
#   mahjongjapan Mahjong Escape: Ancient Japan: every layout loaded and dealt (mahjongjapan_dealtest), and a scripted
#                escape stage from a new player (the name, the map, stage 1 won with the solver's clicks, the level's
#                end and the map again) twice with the same screens, which are the frozen baseline's (under a minute)
#   oracle       Bejeweled's oracle three ways (compare.sh --variants default,forcevariant,novariants); over
#                ten minutes, so never in a subagent; refuses to start while another oracle runs
# Sets: quick = build unit; games = bookworm bigmoney noahsark atomica alchemy sevenseas pixelus bookwormadv bookwormadv2 tiptop mummymaze ningpo wordharmony cosmicbugs waterbugs rocketmania dynomite mahjongjapan bonnies; all = every suite, builds first.
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
AL_STRATEGIC="game 1 over at update 12206: score 7432 pieces 830 forged 29 boards 3"
AL_TIMETRIAL="game 1 over at update 14641: score 15942 pieces 903 forged 87 boards 3"
AL_TUTORIAL="tutorial button 1 at update 231"
SS_BEGINNER="game 2 over at update 5042: score 320 level 4 moves 25"
SS_BONUS="game 1 over at update 2090: score 420 level 13 moves 13"
SS_LATE="game 1 over at update 2131: score 370 level 22 moves 17"
PX_GRID="gridtest: 213 level files, 211 solutions, 211 solved, 0 without a level file"
PX_START="done: 25 puzzles"
PX_FULL="done: no temple left, 210 puzzles, log 2edb2003"
MM_CLASSIC="game over: score 1500, list 1500 1000 900 800 700, the bot is number 1"
MM_TIPS="game over: score 1730, list 1730 1000 900 800 700, the bot is number 1"
MM_FULL="victory: score 251975, caught 0 times"
MM_ADVENTURE="victory: score 0, caught 49 times"
MM_TUTORIAL="tutorial done at update 6591"
TT_REGULAR="game 2 over at update 35544: level 7, score 37160, 352 moves"
TT_STRATEGY="game 1 over at update 2593: level 1, score 2100, 24 moves"
TT_SPEED="game 1 over at update 57249: level 11, score 78250, 1081 moves"
NP_GAME="Test: 40 tiles left in 00:12"
WH_THINK="level 2 saved, score 1600"
WH_TSUNAMI="level 1 saved, score 0, 8 balls, 6 to come"
CB_BANKS="82 animations, 9 missing, 0 failed; 8 fonts, 0 failed"
CB_GAME="17000 ms: score 526, lives 6, board 0, filled 426 of 85%; 28000 ms: score 729, lives 6, board 0, filled 629 of 85%"
WB_BANKS="77 animations, 10 missing, 0 failed; 7 fonts, 3 empty as in the game, 0 failed"
WB_GAME="20000 ms: score 0, lives 6, board 0, filled 0 of 85%; 32000 ms: score 494, lives 6, board 0, filled 494 of 85%"
WB_LOSS="30000 ms: score 0, lives 0, board 0, filled 0 of 90%"
WB_SHOTS="4ae35219b586 01d50e63144e 640e34bc8007"  # the first 12 hex digits of the sha256 of game/start.png, game/beam.png and loss/scores.png (pixels, with the bubble column masked, see wb_hash): the frozen baseline
MJ_DEAL="222 layouts, 0 failed"
BB_SHOTS="90f2d44e0574 1a20ef2fe747 fa3847002043"  # the first 12 hex digits of the sha256 of played.png, highscores.png and timeup.png (games/bonnies/tools/tour_test.txt): the frozen baseline
MJ_SHOTS="2fff3f87d787 6815b4445dc6 7bede5c481fd"  # the first 12 hex digits of the sha256 of board.png, won.png and map.png: the frozen baseline
DY_GAME="board mode 0 score 10 eggs 12 cells ec5704d5"

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
		games) SUITES+=(bookworm bigmoney noahsark atomica alchemy sevenseas pixelus bookwormadv bookwormadv2 tiptop mummymaze ningpo wordharmony cosmicbugs waterbugs rocketmania dynomite mahjongjapan bonnies); shift ;;
		all) SUITES+=(build build-web build-nov unit unit-web verify modmenu livesplit bookworm bigmoney noahsark atomica alchemy sevenseas pixelus bookwormadv bookwormadv2 tiptop mummymaze ningpo wordharmony cosmicbugs waterbugs rocketmania dynomite mahjongjapan bonnies oracle); shift ;;
		build|build-web|build-nov|unit|unit-web|verify|modmenu|livesplit|bookworm|bigmoney|noahsark|atomica|alchemy|sevenseas|pixelus|bookwormadv|bookwormadv2|tiptop|mummymaze|ningpo|wordharmony|cosmicbugs|waterbugs|rocketmania|dynomite|mahjongjapan|bonnies|oracle) SUITES+=("$1"); shift ;;
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
	local st=0 a b c d e f g h i
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
	# Mixer::GetSongFFT (MusicInterface::GetMusicFFT, BASS_ChannelGetData's FFT) on tones; needs no game files
	(cd build/native && timeout 60 ./spectrumtest.exe) > "$1.fft" 2>&1 || st=1
	g="$(lastof "$1.fft" 'spectrum: [0-9]+ checks, [0-9]+ failed')"
	# Mixer's sounds: 16-bit at their own rate, resampled by the voices; needs no game files
	(cd build/native && timeout 60 ./soundtest.exe) > "$1.snd" 2>&1 || st=1
	h="$(lastof "$1.snd" 'sound: [0-9]+ checks, [0-9]+ failed')"
	# tools/shotdiff: the screenshot comparison on synthetic pictures (statuses, boxes, alignment, pages, the marks server)
	timeout 120 python tools/shotdiff/selftest.py > "$1.sdf" 2>&1 || st=1
	i="$(lastof "$1.sdf" 'shotdiff: [0-9]+ checks, [0-9]+ failed')"
	cat "$1.mst" "$1.fw" "$1.ph" "$1.jp2" "$1.sp" "$1.fft" "$1.snd" "$1.sdf" >> "$1"
	rm -f "$1.mst" "$1.fw" "$1.ph" "$1.jp2" "$1.sp" "$1.fft" "$1.snd" "$1.sdf"
	SUMMARY="$a; $b; $c; $d; $e; $f; $g; $h; $i"
	return $st
}

suite_unit-web() {
	[ -n "$NODE" ] || { SUMMARY="no node.exe under EMSDK_ROOT"; return 1; }
	local st=0 a b web="${BYOPOP_WEB_DIR:-build/web}"
	(cd "$web" && "$NODE" rulestest.js) > "$1" 2>&1 || st=1
	a="$(lastof "$1" 'checks')"
	(cd "$web" && "$NODE" modsettingstest.js) > "$1.mst" 2>&1 || st=1
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
	# the page's shared settings and players code (web/settings.js, web/profiles.js), on a fake localStorage
	local d e f g h
	"$NODE" tools/profilestest.js > "$1.pt" 2>&1 || st=1
	d="$(lastof "$1.pt" 'profilestest:')"
	# the save backup's zip, paths and merge (web/saves.js)
	"$NODE" tools/savestest.js > "$1.sv" 2>&1 || st=1
	e="$(lastof "$1.sv" 'savestest:')"
	# the import of saves from an installed game (web/saveimport.js, and the table platform/saveimport.json it shares with the native games)
	"$NODE" tools/saveimporttest.js > "$1.si" 2>&1 || st=1
	g="$(lastof "$1.si" 'saveimporttest:')"
	# the chat's rules, stored history and local source, and its plain-text rendering (web/chat.js)
	"$NODE" tools/chattest.js > "$1.ch" 2>&1 || st=1
	h="$(lastof "$1.ch" 'chattest:')"
	# the built pages' accessibility (tools/a11ycheck.js): one line per page in the log
	"$NODE" tools/a11ycheck.js --selftest > "$1.a11y" 2>&1 || st=1
	"$NODE" tools/a11ycheck.js "$web" >> "$1.a11y" 2>&1 || st=1
	f="$(lastof "$1.a11y" 'a11ycheck:')"
	cat "$1.mst" "$1.pak" "$1.pt" "$1.sv" "$1.si" "$1.ch" "$1.a11y" >> "$1"; rm -f "$1.mst" "$1.pak" "$1.pt" "$1.sv" "$1.si" "$1.ch" "$1.a11y"
	SUMMARY="$a; $b; $c; $d; $e; $g; $h; $f"
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

alchemy_game() {  # <name> <mode> <cap> [user dir to keep]: a bot game; prints its last "game over" or tutorial line
	local d="$OUT/alchemy/$1" u
	if [ -n "${4:-}" ]; then u="$OUT/alchemy/$4"; else rm -rf "$d"; u="$d/user"; fi
	rm -rf "$d/shots" "$d/log.txt"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/alchemy.exe" -userdir="$u" -fast -seed=1 -autoplay=1 -autoplaymode="$2" -autoplaycap="$3" 		-autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" > "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(game [0-9]+ over|tutorial button|cap reached)' "$d/log.txt" 2>/dev/null | tail -n 1
}

suite_alchemy() {
	local st=0 s t u r v sv
	need_assets alchemy || return 1
	s="$(alchemy_game strategic strategic 20000)"; t="$(alchemy_game timetrial timetrial 20000)"
	u="$(alchemy_game tutorial tutorial 20000)"; r="$(alchemy_game strategic2 strategic 20000)"
	# a game quit half way is saved, and the next run offers it and plays on
	sv="$(alchemy_game save strategic 700)"
	v="$(alchemy_game save2 strategic 1500 save/user >/dev/null; grep -c '^saved game loaded' "$OUT/alchemy/save2/log.txt")"
	[ -f "$OUT/alchemy/save/user/savegame.dat" ] || v="no saved game"
	{ echo "strategic: $s"; echo "time trial: $t"; echo "tutorial: $u"; echo "strategic rerun: $r"; echo "save: $sv; loaded: $v"; } > "$1"
	[ "$s" = "$AL_STRATEGIC" ] || st=1
	[ "$t" = "$AL_TIMETRIAL" ] || st=1
	[ "$u" = "$AL_TUTORIAL" ] || st=1
	[ "$v" = 1 ] || st=1
	if diff -rq "$OUT/alchemy/strategic/shots" "$OUT/alchemy/strategic2/shots" >> "$1" 2>&1 && cmp -s "$OUT/alchemy/strategic/log.txt" "$OUT/alchemy/strategic2/log.txt" && [ "$s" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="strategic ${s#game 1 over at update }; time trial ${t#game 1 over at update }; ${u}; saved game loaded $v time; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected strategic '$AL_STRATEGIC', time trial '$AL_TIMETRIAL', tutorial '$AL_TUTORIAL')"
	return $st
}

sevenseas_game() {  # <name> <seed> <difficulty> <start level> <games>: a bot game; prints the last "game N over" line
	local d="$OUT/sevenseas/$1" exe="${SS_EXE:-$W/build/native/sevenseas.exe}"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$exe" -userdir="$d/user" -fast -seed="$2" -autoplay="$2" -autoplaydiff="$3" -autoplaylevel="$4" \
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
	# the frozen baseline (build/baseline-sevenseas/sevenseas.exe, a copy of the port made when it was last checked
	# against the original): the same three games must give the same lines and the same screenshots
	local base="$W/build/baseline-sevenseas/sevenseas.exe" bl=""
	if [ -f "$base" ]; then
		SS_EXE="$base" sevenseas_game base-beginner 1 1 0 2 > /dev/null; SS_EXE="$base" sevenseas_game base-bonus 2 2 10 1 > /dev/null
		SS_EXE="$base" sevenseas_game base-late 3 3 21 1 > /dev/null
		bl="baseline identical"
		for k in beginner bonus late; do
			diff -q "$OUT/sevenseas/$k/log.txt" "$OUT/sevenseas/base-$k/log.txt" >> "$1" 2>&1 && diff -rq "$OUT/sevenseas/$k/shots" "$OUT/sevenseas/base-$k/shots" >> "$1" 2>&1 || { bl="BASELINE DIFFERS ($k)"; st=1; }
		done
		r="$r; $bl"
	fi
	SUMMARY="beginner ${a#game }; bonus ${b#game }; late ${c#game }; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected beginner '$SS_BEGINNER', bonus '$SS_BONUS', late '$SS_LATE')"
	return $st
}

pixelus_game() {  # <name> <puzzles>: a bot game of that many puzzles (at most); prints its last line
	local d="$OUT/pixelus/$1"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/pixelus.exe" -userdir="$W/$d/user" -fast -seed=1 -autoplay=1 -autoplaypuzzles="$2" -autoplaycap=1000000 		-autoplaylog="$W/$d/log.txt" -autoplayshots="$W/$d/shots/s" > "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(done|cap reached)' "$d/log.txt" 2>/dev/null | tail -n 1
}

suite_pixelus() {
	local st=0 g t a r f n h d="$OUT/pixelus"
	need_assets pixelus || return 1
	g="$(build/native/pixelus_gridtest.exe "$W/assets/pixelus" 2>&1 | tail -n 1)"
	# the options, credits and how-to-play screens: a scripted run in real time (the script counts updates)
	rm -rf "$d/tour"; mkdir -p "$d/tour"
	sed "s#@OUT@#$W/$d/tour#g" games/pixelus/tests/menu_tour.txt.in > "$d/tour/script.txt"
	timeout 300 "$W/build/native/pixelus.exe" -userdir="$W/$d/tour/user" -script="$W/$d/tour/script.txt" -seed=1 > "$d/tour/stdout.txt" 2> "$d/tour/stderr.txt"
	t=$?
	for f in options credits help menu; do [ -s "$d/tour/$f.png" ] || t="$t, $f.png missing"; done
	[ "$t" = 0 ] && t="tour ok" || { t="TOUR FAILED ($t)"; st=1; }
	a="$(pixelus_game start 25)"; r="$(pixelus_game start2 25)"
	f="$(pixelus_game full 400)"
	n="$(grep -c '^completed' "$d/full/log.txt" 2>/dev/null)"; h="$(sha1sum "$d/full/log.txt" 2>/dev/null | cut -c1-8)"
	f="$f, $n puzzles, log $h"
	{ echo "$g"; echo "$t"; echo "start: $a"; echo "start rerun: $r"; echo "full: $f"; } > "$1"
	[ "$g" = "$PX_GRID" ] || st=1
	[ "$a" = "$PX_START" ] || st=1
	[ "$f" = "$PX_FULL" ] || st=1
	if diff -rq "$d/start/shots" "$d/start2/shots" >> "$1" 2>&1 && diff -q "$d/start/log.txt" "$d/start2/log.txt" >> "$1" 2>&1; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="${g#gridtest: }; $t; start ${a#done: }; $r; full ${f#done: }"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected grid '$PX_GRID', start '$PX_START', full '$PX_FULL')"
	return $st
}

mummymaze_game() {  # <name> <bot arguments>...: a bot game; prints the last result line of its log
	local n="$1" d
	d="$OUT/mummymaze/$n"; shift
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/mummymaze.exe" -userdir="$d/user" -fast "$@" -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" 		> "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(game over|victory|tutorial done|stalled|cap reached)' "$d/log.txt" 2>/dev/null | tail -n 1
}

mummymaze_resume() {  # a game saved before its clock starts, then continued: the time of the level must be small (a negative saved time once came out near 2^32 / 100 s)
	local d="$OUT/mummymaze/resume" t
	rm -rf "$d"; mkdir -p "$d"
	timeout 300 "$W/build/native/mummymaze.exe" -userdir="$d/user" -fast -seed=1 -autoplay=3 -autoplaygames=1 -autoplaysave=3 -autoplaycap=3000 -autoplaylog="$d/log1.txt" > "$d/stdout1.txt" 2> "$d/stderr1.txt"
	timeout 300 "$W/build/native/mummymaze.exe" -userdir="$d/user" -fast -seed=1 -autoplay=3 -autoplaygames=1 -autoplaylevels=1 -autoplaycontinue -autoplaycap=60000 -autoplaylog="$d/log2.txt" > "$d/stdout2.txt" 2> "$d/stderr2.txt"
	t="$(grep -m1 'level time' "$d/log2.txt" | awk '{print $3}')"
	MM_RESUME="resume: level time ${t:-none}"
	[ -n "$t" ] && [ "$t" -gt -1000 ] && [ "$t" -lt 1000 ]
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
	mummymaze_resume || st=1
	if diff -rq "$OUT/mummymaze/classic/shots" "$OUT/mummymaze/classic2/shots" >> "$1" 2>&1 && cmp -s "$OUT/mummymaze/classic/log.txt" "$OUT/mummymaze/classic2/log.txt" && [ "$a" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="classic ${a#game over: }; tips ${b#game over: }; full ${c}; adventure ${v}; ${t}; $MM_RESUME; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected classic '$MM_CLASSIC', tips '$MM_TIPS', full '$MM_FULL', tutorial '$MM_TUTORIAL', adventure '$MM_ADVENTURE')"
	return $st
}

suite_bookwormadv() {
	local log="$1" b="$W/build/native/bwa_luarun.exe" d l p s m c w v
	need_assets bookwormadv || return 1
	[ -x "$b" ] || { SUMMARY="no $b (build first)"; return 1; }
	d="$("$b" -gamedir=games/bookwormadv/tests/poplua dialect.lua 2>&1 | tee -a "$log" | tail -n 1)"
	l="$("$b" -load 2>&1 | tee -a "$log" | tail -n 1)"
	p="$("$W/build/native/bwa_pamtest.exe" 2>&1 | tee -a "$log" | tail -n 1 | tr -d '\r')"
	s="$("$W/build/native/bwa_swftest.exe" 2>&1 | tee -a "$log" | grep -c '; unsupported: none')"
	m="$(bookwormadv_menu "$log")"
	c="$(bookwormadv_chapter "$log")"
	w="$(bookwormadv_screens "$log")"
	v="$(bookwormadv_media "$log")"
	SUMMARY="$d; $l; pam $p; swf $s of 4 to their end; menu: $m; chapter 1: $c; screens: $w; media player: $v"
	case "$d" in "dialect: "*" checks passed") ;; *) return 1 ;; esac
	bookwormadv_all "$l" || return 1
	bookwormadv_all "$p" || return 1
	[ "$s" = 4 ] || return 1
	[ "$m" = "quit at update 1310" ] || return 1
	[ "$c" = "7 words, won, saved" ] || return 1
	[ "$w" = "credits, players, mini-games, arena word ANTIBIOSIS" ] || return 1
	[ "$v" = "spectrum lit both ways" ] || return 1
	return 0
}

suite_bookwormadv2() {
	local log="$1" b="$W/build/native/bwa2_luarun.exe" d l r p st s f m
	need_assets bookwormadv2 || return 1
	[ -x "$b" ] || { SUMMARY="no $b (build first)"; return 1; }
	# (each tool's own exit status, not tee's or tail's, is the substitution's: a crash after the last line still fails)
	st=0
	d="$("$b" -gamedir=games/bookwormadv2/tests/poplua dialect.lua 2>&1 | tee -a "$log" | tail -n 1; exit "${PIPESTATUS[0]}")" || st=1
	l="$("$b" -gamedir=assets/bookwormadv2 -load 2>&1 | tee -a "$log" | tail -n 1; exit "${PIPESTATUS[0]}")" || st=1
	r="$("$b" -gamedir=assets/bookwormadv2 -runall 2>&1 | tee -a "$log" | tail -n 1; exit "${PIPESTATUS[0]}")" || st=1
	p="$(python games/bookwormadv2/tests/poplua/pairs.py "$b" assets/bookwormadv2 2>&1 | tee -a "$log" | tail -n 1; exit "${PIPESTATUS[0]}")" || st=1
	# the Flash player: its self test (every line a PASS), then the game's movies to their end with nothing skipped
	s="$("$W/build/native/bwa2_swftest.exe" -selftest 2>&1 | tee -a "$log" | grep -c '^PASS'; exit "${PIPESTATUS[0]}")" || st=1
	grep -q '^FAIL' "$log" && st=1
	f="$("$W/build/native/bwa2_swftest.exe" 2>&1 | tee -a "$log" | grep -c 'done@.*unsupported: none'; exit "${PIPESTATUS[0]}")" || st=1
	m="$(bookwormadv2_smoke "$log")"
	SUMMARY="$d; $l; $r; $p; swf: $s checks, $f of 5 movies to their end; $m"
	[ "$d" = "dialect: 150 checks passed" ] || st=1
	[ "$l" = "loaded 462 of 462 scripts" ] || st=1
	[ "$r" = "ran 462 of 462 scripts" ] || st=1
	[ "$p" = "pairs: 8 of 8 identical" ] || st=1
	[ "$s" -gt 0 ] && [ "$f" = 5 ] || st=1
	[ "$m" = "new player: PLAY attacked; main menu, mini-games: ok; a mini-game plays" ] || st=1
	return $st
}

bookwormadv2_smoke() {  # <log>: a new player (an empty user folder) through the tutorial's first attack, then its profile to the main menu and the mini-games' menu
	local d="$W/$OUT/bookwormadv2/smoke" b="$W/build/native/bookwormadv2.exe" a="$W/assets/bookwormadv2" k t n
	rm -rf "$d"; mkdir -p "$d/user"
	# Run 1: the title, the new player's name (the default) and the intro's pages, the first battle's tutorial:
	# P, L, A, Y, then the attack button; the board's word lookup answers from the script (IsNoun, IsVerb, ...)
	{ printf '%s\n' "800 click 400 435"
	  for k in $(seq 0 8); do printf '%s\n' "$((2700 + k * 100)) click 400 300"; done
	  printf '%s\n' "4300 click 650 580" "4500 click 405 313" "4800 click 330 389" "5200 click 376 485" "5600 click 426 335" \
		"6000 click 476 435" "6400 screenshot $d/word.png" "6600 click 400 568" "8500 screenshot $d/attack.png" \
		'8600 lua print("SMOKE", gBoard:IsNoun("PLAY"), gBoard:IsVerb("PLAY"), gBoard:IsAdjective("PLAY"), gBoard:IsAdjective("RED"))' \
		"11000 quit"; } > "$d/newuser.txt"
	timeout 120 "$b" -gamedir="$a" -userdir="$d/user" -script="$d/newuser.txt" -fast > "$d/newuser.out" 2>&1
	t=$?
	cat "$d/newuser.out" >> "$1"
	[ $t -eq 0 ] || { echo "new player: exit $t"; return; }
	grep -qiE 'attempt to|stack traceback|FATAL|\.lua:[0-9]+:' "$d/newuser.out" && { echo "new player: a script failed"; return; }
	grep -q "SMOKE.true.true.false.true" "$d/newuser.out" || { echo "new player: word flags wrong"; return; }
	for n in word attack; do
		[ "$(stat -c %s "$d/$n.png" 2>/dev/null || echo 0)" -gt 50000 ] || { echo "new player: no $n picture"; return; }
	done
	cmp -s "$d/word.png" "$d/attack.png" && { echo "new player: the attack changed nothing"; return; }
	# Run 2: that profile again: its settings set so that the main menu comes up (the profile of a finished game),
	# the player picked, the main menu, then the mini-games' menu
	{ for k in $(seq 0 6); do
		printf '%s\n' "$((600 + k * 200)) lua profile.Set(\"NeedsIntroTileHelp\", false); profile.Set(\"AdventureComplete\", true); profile.Set(\"InGame\", false); profile.Set(\"UnlockedMiniGames\", true); profile.Set(\"UnlockedBestiary\", true); profile.Set(\"CurrentBook\", 6)"
	  done
	  printf '%s\n' "4000 click 250 330" "5500 click 400 330" "5700 click 300 170" "5800 click 300 170" "6100 click 400 575" \
		"8000 click 712 505" "8001 click 712 505" "8300 click 405 313" "11000 screenshot $d/menu.png" "11200 click 120 405" \
		"13500 screenshot $d/minigames.png" "13600 quit"; } > "$d/menus.txt"
	timeout 120 "$b" -gamedir="$a" -userdir="$d/user" -script="$d/menus.txt" -fast > "$d/menus.out" 2>&1
	t=$?
	cat "$d/menus.out" >> "$1"
	[ $t -eq 0 ] || { echo "main menu: exit $t"; return; }
	grep -qiE 'attempt to|stack traceback|FATAL|\.lua:[0-9]+:' "$d/menus.out" && { echo "main menu: a script failed"; return; }
	for n in menu minigames; do
		[ "$(stat -c %s "$d/$n.png" 2>/dev/null || echo 0)" -gt 50000 ] || { echo "main menu: no $n picture"; return; }
	done
	cmp -s "$d/menu.png" "$d/minigames.png" && { echo "main menu: the mini-games' menu did not open"; return; }
	# Run 3: the same route up to the mini-games' menu, then Golden Coins: its Welcome's OK and the test hook's bot
	# for a few words (a mini-game has no battle engine, and its first scramble once crashed the board)
	{ grep -vE ' (quit|screenshot)( |$)' "$d/menus.txt"
	  printf '%s\n' '12500 lua main.StartMinigameFromMainMenu("GoldenCoins")' "15500 click 400 293" "16000 soak 19000" \
		"19010 quit"; } > "$d/minigame.txt"
	timeout 120 "$b" -gamedir="$a" -userdir="$d/user" -script="$d/minigame.txt" -fast > "$d/minigame.out" 2>&1
	t=$?
	cat "$d/minigame.out" >> "$1"
	[ $t -eq 0 ] || { echo "mini-game: exit $t"; return; }
	grep -qiE 'attempt to|stack traceback|FATAL|\.lua:[0-9]+:' "$d/minigame.out" && { echo "mini-game: a script failed"; return; }
	[ "$(grep -cE 'script: word [A-Z]+ ' "$d/minigame.out")" -ge 2 ] || { echo "mini-game: the bot spelled no words"; return; }
	echo "new player: PLAY attacked; main menu, mini-games: ok; a mini-game plays"
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

bookwormadv_screens() {  # <log>: the test profile through the screens off the main menu: the credits, the players, the mini-games, the arena
	local d="$OUT/bookwormadv/screens" u n k t
	rm -rf "$d"; mkdir -p "$d"
	cp -r games/bookwormadv/tests/menu/user "$d/user"
	# the profile unlocks the mini-games and the arena (for a player past the first book); then the options'
	# credits, skipped; the players dialog, cancelled; the mini-games' menu and its way back; the arena: its
	# intro, the treasure room's continue (enabled from Lua, as no treasure is chosen), Cassandra's tutorial clicked
	# through off the grid, and the longest word with the attack
	{ printf '%s\n' "950 lua profile.Set('UnlockedMiniGames', true) profile.Set('UnlockedArena', true) profile.Set('CurrentBook', 2)" \
		"1000 move 50 500" "1010 click 50 500" "1100 move 275 458" "1110 click 275 458" "1600 screenshot $d/credits.png" \
		"1700 click 400 300" "2300 move 400 209" "2310 click 400 209" "2500 screenshot $d/players.png" \
		"2550 move 470 464" "2560 click 470 464" "2700 move 110 400" "2710 click 110 400" "3200 screenshot $d/minigames.png" \
		"3250 move 393 494" "3260 click 393 494" "3900 move 530 270" "3920 click 530 270" "5300 move 400 385" \
		"5320 click 400 385" "5500 lua gTreasureScreen.mCObj:SetButtonState(gTreasureScreen.ButtonId_Continue, true, true)" \
		"5600 move 400 515" "5620 click 400 515"
	  for k in $(seq 0 9); do t=$((6500 + k * 200)); printf '%s\n' "$t move 700 500" "$((t + 20)) click 700 500"; done
	  printf '%s\n' "8700 word attack" "9300 screenshot $d/arena.png" "9400 quit"; } > "$d/script.txt"
	u="$d/user"; case "$u" in /*|[A-Za-z]:*) ;; *) u="$W/$u" ;; esac
	timeout 180 "$W/build/native/bookwormadv.exe" -userdir="$u" -script="$d/script.txt" -fast -seed=1 \
		> "$d/stdout.txt" 2>&1
	cat "$d/stdout.txt" >> "$1"
	grep -q "FATAL" "$d/stdout.txt" && { echo "failed: $(grep -m 1 -A 2 FATAL "$d/stdout.txt" | tail -n 1)"; return; }
	grep -q "script: lua:" "$d/stdout.txt" && { echo "failed: $(grep -m 1 'script: lua:' "$d/stdout.txt")"; return; }
	for n in credits players minigames arena; do [ -f "$d/$n.png" ] || { echo "no $n screenshot"; return; }; done
	n="$(grep -m 1 -o 'script: word [A-Z]*' "$d/stdout.txt")"
	echo "credits, players, mini-games, arena ${n#script: }"
}

bookwormadv_media() {  # <log>: the test profile's media player: a song's spectrum both ways, the mouse over the book
	local d="$OUT/bookwormadv/media" u n a b
	rm -rf "$d"; mkdir -p "$d"
	cp -r games/bookwormadv/tests/menu/user "$d/user"
	# the profile lists one song (the book's script wants the credits' theme among them); the book, opened from Lua,
	# plays it; once its first 100 updates are past the bars are counted, then a click on the screen changes them to
	# columns
	printf '%s\n' "940 lua profile.Set('MediaPlayer', {{mType=1, mTitle='Credits Theme', mData=78}})" \
		"950 lua gCApp:AddStatsBook(false, false, true)" "1100 lua gStatsBook.mCObj:PlayMusic(78)" "1400 spectrum" \
		"1420 screenshot $d/bars.png" "1440 move 570 224" "1450 click 570 224" "1650 spectrum" \
		"1660 screenshot $d/columns.png" "1700 quit" > "$d/script.txt"
	u="$d/user"; case "$u" in /*|[A-Za-z]:*) ;; *) u="$W/$u" ;; esac
	timeout 120 "$W/build/native/bookwormadv.exe" -userdir="$u" -script="$d/script.txt" -fast -seed=1 \
		> "$d/stdout.txt" 2>&1
	cat "$d/stdout.txt" >> "$1"
	grep -q "FATAL" "$d/stdout.txt" && { echo "failed: $(grep -m 1 -A 2 FATAL "$d/stdout.txt" | tail -n 1)"; return; }
	grep -q "script: lua:" "$d/stdout.txt" && { echo "failed: $(grep -m 1 'script: lua:' "$d/stdout.txt")"; return; }
	for n in bars columns; do [ -f "$d/$n.png" ] || { echo "no $n screenshot"; return; }; done
	a="$(grep -o 'script: spectrum [0-9]*' "$d/stdout.txt" | sed -n 1p)"; b="$(grep -o 'script: spectrum [0-9]*' "$d/stdout.txt" | sed -n 2p)"
	a="${a##* }"; b="${b##* }"
	[ "${a:-0}" -gt 0 ] && [ "${b:-0}" -gt 0 ] || { echo "spectrum dark (${a:-none}, ${b:-none} lit)"; return; }
	echo "spectrum lit both ways"
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

ningpo_game() {  # <name>: a scripted game (-seed=7): the cheat's matches to the end, the name entry, the best times
	local d="$OUT/ningpo/$1" u h n="" i
	rm -rf "$d"; mkdir -p "$d/shots"
	# start, the cheat (Backspace q Backspace) and its match key until no pair is left, End Game in the no-moves
	# dialog, continue, the name "Test", the best times' OK and the title's quit
	{
		printf '%s\n' "150 click 320 425" "350 click 340 440" "500 key BACK" "510 char q" "520 key BACK"
		for ((u = 550; u <= 2030; u += 20)); do echo "$u char m"; done
		printf '%s\n' "2150 click 385 368" "2300 screenshot $d/shots/over.png" "2350 click 270 345" \
			"2500 screenshot $d/shots/name.png" "2550 char T" "2555 char e" "2560 char s" "2565 char t" "2630 key RETURN" \
			"2770 screenshot $d/shots/best.png" "2820 click 495 435" "2970 screenshot $d/shots/title.png" "3020 quit"
	} > "$d/script.txt"
	timeout 300 "$W/build/native/ningpo.exe" -gamedir=assets/ningpo -userdir="$d/user" -fast -seed=7 -script="$d/script.txt" \
		> "$d/stdout.txt" 2> "$d/stderr.txt"
	# the first best time of the first layout (HS:0x0, 0x30 bytes: the name at +4, the time at +0x24, the tiles left at +0x2c)
	h="$(grep -m 1 '^HS:0x0=hex:' "$d/user/registry.ini" 2>/dev/null | tr -d '\r')"; h="${h#*hex:}"
	[ ${#h} -eq 96 ] || { echo "no best time"; return; }
	for ((i = 8; i < 72; i += 2)); do [ "${h:i:2}" = 00 ] && break; n="$n$(printf "\\x${h:i:2}")"; done
	i=$(hexle "${h:72:8}")
	printf '%s: %d tiles left in %02d:%02d\n' "$n" "$(hexle "${h:88:8}")" $((i / 60)) $((i % 60))
}

hexle() { echo $((16#${1:6:2}${1:4:2}${1:2:2}${1:0:2})); }  # <8 hex digits>: the little-endian 32-bit number

suite_ningpo() {
	local st=0 g r
	need_assets ningpo || return 1
	g="$(ningpo_game seed7)"; r="$(ningpo_game seed7b)"
	{ echo "game: $g"; echo "rerun: $r"; } > "$1"
	[ "$g" = "$NP_GAME" ] || st=1
	if diff -rq "$OUT/ningpo/seed7/shots" "$OUT/ningpo/seed7b/shots" >> "$1" 2>&1 && [ "$g" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="$g; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected '$NP_GAME')"
	return $st
}

wordharmony_game() {  # <name> <think|tsunami>: a scripted game (-fast) that ends saved; prints the saved game
	local d="$OUT/wordharmony/$1" m g rc f s shots="" re=' level="([0-9]+)" score="([0-9]+)"'
	rm -rf "$d"; mkdir -p "$d/shots" "$d/user"
	if [ "$2" = think ]; then
		# a saved THINK game of the suite's own making: the chain ORD on the top ledge of level template 4, a W below
		{
			printf '%s\n' '[SOFTWARE\GameHouse\WordHarmony]'
			printf '%s' 'SolitaireSavedGame=sz:<level mode="1" level="0" score="0" respawnTimer="0" wc="0" hug="0" hul="0" '
			printf '%s' 'ha="1" lp="0" lba="0" bonus="" ilg="4" time="3" ><rope text="ORD" ><bubble letter="O" origin="194.2,66" '
			printf '%s' 'special="" /><bubble letter="R" origin="238.2,66" special="" /><bubble letter="D" origin="282.2,66" '
			printf '%s\n' 'special="" /></rope><bubble letter="W" origin="355.3,198" special="" /><HUD></HUD><spawnqueue></spawnqueue></level>'
		} > "$d/user/registry.ini"
		# THINK, Continue, the help's Play Game; the W pulled up, over the ledge and down before the O; a right click
		# sends WORD; the level's end and its Continue
		printf '%s\n' "480 move 530 395" "500 click 530 395" "680 move 330 350" "700 click 330 350" "880 move 165 415" \
			"900 click 165 415" "1250 screenshot $d/shots/board.png" "1260 move 564 305" "1270 down 564 305" "1300 move 564 250" \
			"1340 move 564 140" "1400 move 500 140" "1440 move 430 140" "1480 move 360 140" "1520 move 359 172" "1600 up 359 172" \
			"1650 screenshot $d/shots/word.png" "1700 rclick 600 500" "2400 screenshot $d/shots/end.png" "2480 move 430 500" \
			"2500 click 430 500" > "$d/script.txt"
		m=Solitaire shots="board word end"
	else
		# TSUNAMI, level 1, the help's Play Game, and 20 seconds of falling balls
		printf '%s\n' "480 move 447 237" "500 click 447 237" "700 move 300 200" "720 click 300 200" "880 move 165 415" \
			"900 click 165 415" > "$d/script.txt"
		m=Action
	fi
	# Save/Quit and Save Game
	printf '%s\n' "2900 screenshot $d/shots/play.png" "2980 move 155 30" "3000 click 155 30" "3150 move 360 325" \
		"3170 click 360 325" "3400 screenshot $d/shots/menu.png" "3420 quit" >> "$d/script.txt"
	timeout 300 "$W/build/native/wordharmony.exe" -gamedir=assets/wordharmony -userdir="$d/user" -fast -script="$d/script.txt" \
		> "$d/stdout.txt" 2> "$d/stderr.txt"
	rc=$?
	[ $rc -eq 0 ] || { echo "exit status $rc"; return; }
	# the engine's exceptions (WordHarmonyApp::EngineFailed) and any message box the test run did not show
	f="$(grep -h -m 1 -F -e "Word Harmony: " -e "(a test run: not shown)" "$d/stdout.txt" "$d/stderr.txt" | head -n 1)"
	[ -z "$f" ] || { echo "failed: ${f#INFO: }"; return; }
	for s in $shots play menu; do [ -f "$d/shots/$s.png" ] || { echo "no screenshot $s"; return; }; done
	g="$(grep -m 1 "^${m}SavedGame=sz:<level" "$d/user/registry.ini" 2>/dev/null | tr -d '\r')"
	[ -n "$g" ] || { echo "no saved game"; return; }
	[[ "$g" =~ $re ]] || { echo "saved game unreadable"; return; }
	if [ "$2" = think ]; then
		echo "level $((BASH_REMATCH[1] + 1)) saved, score ${BASH_REMATCH[2]}"
	else
		echo "level $((BASH_REMATCH[1] + 1)) saved, score ${BASH_REMATCH[2]}, $(grep -o '<bubble' <<< "$g" | wc -l) balls," \
			"$(grep -o '<letters' <<< "$g" | wc -l) to come"
	fi
}

suite_wordharmony() {
	local st=0 t t2 a a2 r
	need_assets wordharmony || return 1
	t="$(wordharmony_game think think)"; t2="$(wordharmony_game think2 think)"
	a="$(wordharmony_game tsunami tsunami)"; a2="$(wordharmony_game tsunami2 tsunami)"
	{ echo "think: $t"; echo "think rerun: $t2"; echo "tsunami: $a"; echo "tsunami rerun: $a2"; } > "$1"
	[ "$t" = "$WH_THINK" ] || st=1
	[ "$a" = "$WH_TSUNAMI" ] || st=1
	if diff -rq "$OUT/wordharmony/think/shots" "$OUT/wordharmony/think2/shots" >> "$1" 2>&1 &&
		diff -rq "$OUT/wordharmony/tsunami/shots" "$OUT/wordharmony/tsunami2/shots" >> "$1" 2>&1 &&
		[ "$t" = "$t2" ] && [ "$a" = "$a2" ]; then r="reruns identical"; else r="RERUNS DIFFER"; st=1; fi
	SUMMARY="THINK $t; TSUNAMI $a; $r"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected THINK '$WH_THINK', TSUNAMI '$WH_TSUNAMI')"
	return $st
}

cosmicbugs_run() {  # <name>: the headless runner's scripted game; prints its state lines (and its exit status if not 0)
	local d="$OUT/cosmicbugs/$1" e=0 l
	rm -rf "$d"; mkdir -p "$d"
	# Start Game, the instructions' Play, a line down the first board, the aim switched (the right button) and a line
	# across (times in ms; a click holds the button 30 ms)
	printf '%s\n' "3000 click 175 313" "4000 click 400 563" "7500 click 400 300" "17000 state" "17000 shot line1" \
		"18000 down 2" "18030 up 2" "18500 click 250 200" "28000 state" "28000 shot line2" "28010 end" > "$d/script.txt"
	timeout 300 build/native/cosmicbugs_run.exe assets/cosmicbugs "$d" "$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt" || e=$?
	l="$(grep -E '^[0-9]+ ms: score' "$d/stdout.txt" | tr -d '\r' | paste -sd ';' - | sed 's/;/; /g')"
	[ $e -eq 0 ] || l="$l; EXIT $e"
	echo "$l"
}

cosmicbugs_window() {  # <name>: the windowed exe with the runner's left clicks; prints its state after the first line
	local d="$OUT/cosmicbugs/$1" e=0 l u
	rm -rf "$d"; mkdir -p "$d/user"
	u="$(cygpath -ma "$d")"  # the exe's paths, also inside its script: absolute Windows ones, from any --out
	printf '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n' > "$d/user/cp.cfg"
	# an update is 10 ms, a click holds the button 3 updates (pressed and let go in one update, it is lost, as it was
	# between two of the original's frames)
	printf '%s\n' "300 down 175 313" "303 up 175 313" "400 down 400 563" "403 up 400 563" "750 down 400 300" "753 up 400 300" \
		"1700 state" "1700 screenshot $u/line1.png" "1710 quit" > "$d/script.txt"
	timeout 300 "$W/build/native/cosmicbugs.exe" -gamedir=assets/cosmicbugs -userdir="$u/user" -fast -script="$u/script.txt" \
		> "$d/stdout.txt" 2> "$d/stderr.txt" || e=$?
	l="$(cat "$d/stdout.txt" "$d/stderr.txt" | tr -d '\r' | sed -n 's/^INFO: state: //p' | head -n 1)"
	[ $e -eq 0 ] || l="$l; EXIT $e"
	echo "$l"
}

suite_rocketmania() {
	local st=0 d="$OUT/rocketmania" e=0 f n=""
	need_assets rocketmania || return 1
	rm -rf "$d"; mkdir -p "$d/user"
	sed "s|@DIR@|$d|" games/rocketmania/tools/menu_test.txt > "$d/script.txt"
	timeout 300 "$W/build/native/rocketmania.exe" -userdir="$d/user" -fast -seed=1 -script="$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt" || e=$?
	cat "$d/stdout.txt" "$d/stderr.txt" > "$1"
	[ "$e" -eq 0 ] || { SUMMARY="the exe exited with status $e"; return 1; }
	grep -q "Resource Loading Time" "$1" || { SUMMARY="the game did not finish loading"; return 1; }
	if grep -qE "Failed to load|texts do not match|not found in the game folder" "$1"; then SUMMARY="a load failed (see the log)"; return 1; fi
	for f in menu hover difficulty after; do
		[ -s "$d/$f.png" ] || { SUMMARY="no screenshot $f.png"; return 1; }
	done
	cmp -s "$d/menu.png" "$d/hover.png" && { SUMMARY="the hover changed nothing"; return 1; }
	cmp -s "$d/hover.png" "$d/difficulty.png" && { SUMMARY="the click opened no dialog"; return 1; }
	grep -q "StartBoard: mode 0 difficulty 1" "$1" || { SUMMARY="easy arcade did not start a game"; return 1; }
	# the Board: a game of each mode played for 3000 updates by the autoplay hook (random clicks; launches are forced)
	local m line score scores=""
	for m in 0 1 2; do
		sed -e "s|@MODE@|$m|" -e "s|@DIFF@|$((m + 1))|" games/rocketmania/tools/board_test.txt > "$d/board$m.txt"
		e=0
		timeout 300 "$W/build/native/rocketmania.exe" -userdir="$d/user" -fast -seed=1 -script="$d/board$m.txt" > "$d/board$m.out" 2>&1 || e=$?
		cat "$d/board$m.out" >> "$1"
		[ "$e" -eq 0 ] || { SUMMARY="mode $m: the exe exited with status $e"; return 1; }
		line="$(grep "autoplay: done" "$d/board$m.out" | tail -n 1)"
		[ -n "$line" ] || { SUMMARY="mode $m: the autoplay did not finish"; return 1; }
		case "$line" in *"bad cells 0"*) ;; *) SUMMARY="mode $m: a tile stood in the wrong cell ($line)"; return 1;; esac
		score="$(echo "$line" | sed 's/.* score \([0-9]*\) .*/\1/')"
		[ "${score:-0}" -gt 0 ] || { SUMMARY="mode $m: no launch scored ($line)"; return 1; }
		scores="$scores $score"
	done
	SUMMARY="loaded, menu, hover, difficulty dialog, and a game of each mode played (scores$scores)"
	return $st
}

suite_cosmicbugs() {
	local st=0 b g r w w2 c="$OUT/cosmicbugs" ea=0 ef=0
	need_assets cosmicbugs || return 1
	mkdir -p "$c/banks"
	# Every animation bugs.xml names and every font, each listing kept (bugs.xml names nine files the game does not ship)
	timeout 120 build/native/cosmicbugs_banktest.exe assets/cosmicbugs "$c/banks" anims > "$c/banks/anims.txt" 2>&1 || ea=$?
	timeout 120 build/native/cosmicbugs_banktest.exe assets/cosmicbugs "$c/banks" fonts > "$c/banks/fonts.txt" 2>&1 || ef=$?
	b="$(tail -n 1 "$c/banks/anims.txt" | tr -d '\r'); $(tail -n 1 "$c/banks/fonts.txt" | tr -d '\r')"
	[ $ea -eq 0 ] && [ $ef -eq 0 ] || b="$b; EXIT $ea $ef"
	g="$(cosmicbugs_run game)"; r="$(cosmicbugs_run game2)"
	# The windowed exe's -fast clock does not step 10 ms on every update, so its animations' phases are not the
	# runner's: its state after the first line is, and its screen is its rerun's
	w="$(cosmicbugs_window window)"; w2="$(cosmicbugs_window window2)"
	{ echo "banks: $b"; echo "game: $g"; echo "rerun: $r"; echo "window: $w"; echo "window rerun: $w2"; } > "$1"
	[ "$b" = "$CB_BANKS" ] || st=1
	[ "$g" = "$CB_GAME" ] || st=1
	if diff -q "$c/game/line1.png" "$c/game2/line1.png" >> "$1" 2>&1 && diff -q "$c/game/line2.png" "$c/game2/line2.png" >> "$1" 2>&1 &&
		[ "$g" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	if [ "$w" = "$(echo "${g%%;*}" | sed 's/^[0-9]* ms: //')" ] && diff -q "$c/window/line1.png" "$c/window2/line1.png" >> "$1" 2>&1 &&
		[ "$w" = "$w2" ]; then w="window the same, twice"; else w="WINDOW DIFFERS ('$w')"; st=1; fi
	SUMMARY="$b; $g; $r; $w"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected '$CB_BANKS', '$CB_GAME')"
	return $st
}

waterbugs_run() {  # <name>: the headless runner's scripted game; prints its state lines (and its exit status if not 0)
	local d="$OUT/waterbugs/$1" e=0 l
	rm -rf "$d"; mkdir -p "$d"
	# Start Game, the world map's first area, the first board's start, a beam across the board (times in ms)
	printf '%s\n' "3000 click 400 303" "8000 click 75 260" "14000 click 400 465" "20000 state" "20000 shot start" \
		"24000 click 300 300" "32000 state" "32000 shot beam" "32010 end" > "$d/script.txt"
	timeout 300 build/native/waterbugs_run.exe assets/waterbugs "$d" "$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt" || e=$?
	l="$(grep -E '^[0-9]+ ms: score' "$d/stdout.txt" | tr -d '\r' | paste -sd ';' - | sed 's/;/; /g')"
	[ $e -eq 0 ] || l="$l; EXIT $e"
	echo "$l"
}

wb_hash() {  # <png>: the first 12 hex digits of the sha256 of the pixels; the loss screen's rising bubbles in x 560..690, y 240..520 are zeroed first, because one of them differs by a few pixels from run to run
	python -I -c "import hashlib,sys;from PIL import Image;i=Image.open(sys.argv[1]).convert('RGB');i.paste((0,0,0),(560,240,690,520)) if 'scores' in sys.argv[1] else 0;print(hashlib.sha256(i.tobytes()).hexdigest()[:12])" "$1" 2>/dev/null | tr -d '\r'
}

waterbugs_loss() {  # the last life lost, the click on the game over: the high scores (the game file is read again; it once crashed)
	local d="$OUT/waterbugs/loss" e=0 l
	rm -rf "$d"; mkdir -p "$d"
	printf '%s\n' "3000 click 400 303" "8000 click 75 260" "14000 click 400 465" "20000 lives 1" "21000 die" "24000 click 400 300" \
		"30000 state" "30000 shot scores" "30010 end" > "$d/script.txt"
	timeout 300 build/native/waterbugs_run.exe assets/waterbugs "$d" "$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt" || e=$?
	l="$(grep -E '^[0-9]+ ms: score' "$d/stdout.txt" | tr -d '\r')"
	[ -f "$d/scores.png" ] || l="$l; NO SCREENSHOT"
	[ $e -eq 0 ] || l="$l; EXIT $e"
	echo "$l"
}

waterbugs_window() {  # <name>: the windowed exe with the runner's left clicks; prints its state after the beam
	local d="$OUT/waterbugs/$1" e=0 l u
	rm -rf "$d"; mkdir -p "$d/user"
	u="$(cygpath -ma "$d")"  # the exe's paths, also inside its script: absolute Windows ones, from any --out
	printf '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n' > "$d/user/cp.cfg"
	# an update is 10 ms, a click holds the button 3 updates (as in the Cosmic Bugs suite)
	printf '%s\n' "300 down 400 303" "303 up 400 303" "800 down 75 260" "803 up 75 260" "1400 down 400 465" "1403 up 400 465" \
		"2400 down 300 300" "2403 up 300 300" "3200 state" "3200 screenshot $u/beam.png" "3210 quit" > "$d/script.txt"
	timeout 300 "$W/build/native/waterbugs.exe" -gamedir=assets/waterbugs -userdir="$u/user" -fast -script="$u/script.txt" \
		> "$d/stdout.txt" 2> "$d/stderr.txt" || e=$?
	l="$(cat "$d/stdout.txt" "$d/stderr.txt" | tr -d '\r' | sed -n 's/^INFO: state: //p' | head -n 1)"
	[ $e -eq 0 ] || l="$l; EXIT $e"
	echo "$l"
}

suite_waterbugs() {
	local st=0 b g r w w2 l sh c="$OUT/waterbugs" ea=0 ef=0
	need_assets waterbugs || return 1
	mkdir -p "$c/banks"
	# Every animation the game file names and every font, each listing kept (the game file names ten files the game does not ship)
	timeout 120 build/native/waterbugs_banktest.exe assets/waterbugs "$c/banks" anims > "$c/banks/anims.txt" 2>&1 || ea=$?
	timeout 120 build/native/waterbugs_banktest.exe assets/waterbugs "$c/banks" fonts > "$c/banks/fonts.txt" 2>&1 || ef=$?
	b="$(tail -n 1 "$c/banks/anims.txt" | tr -d '\r'); $(tail -n 1 "$c/banks/fonts.txt" | tr -d '\r')"
	[ $ea -eq 0 ] && [ $ef -eq 0 ] || b="$b; EXIT $ea $ef"
	g="$(waterbugs_run game)"; r="$(waterbugs_run game2)"
	# The windowed exe's -fast clock does not step 10 ms on every update, so its animations' phases are not the
	# runner's: its state after the beam is, and its screen is its rerun's
	w="$(waterbugs_window window)"; w2="$(waterbugs_window window2)"
	l="$(waterbugs_loss)"
	{ echo "banks: $b"; echo "game: $g"; echo "rerun: $r"; echo "window: $w"; echo "window rerun: $w2"; } > "$1"
	[ "$b" = "$WB_BANKS" ] || st=1
	[ "$g" = "$WB_GAME" ] || st=1
	[ "$l" = "$WB_LOSS" ] || st=1
	sh="$(for f in game/start game/beam loss/scores; do wb_hash "$c/$f.png"; done | paste -sd" " -)"
	if [ "$sh" = "$WB_SHOTS" ]; then sh="screens as frozen"; else sh="SCREENS CHANGED ($sh)"; st=1; fi
	if diff -q "$c/game/start.png" "$c/game2/start.png" >> "$1" 2>&1 && diff -q "$c/game/beam.png" "$c/game2/beam.png" >> "$1" 2>&1 &&
		[ "$g" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	if [ "$w" = "$(echo "${g##*; }" | sed 's/^[0-9]* ms: //')" ] && diff -q "$c/window/beam.png" "$c/window2/beam.png" >> "$1" 2>&1 &&
		[ "$w" = "$w2" ]; then w="window the same, twice"; else w="WINDOW DIFFERS ('$w')"; st=1; fi
	SUMMARY="$b; $g; $r; $w; loss: $l; $sh"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected '$WB_BANKS', '$WB_GAME', '$WB_LOSS')"
	return $st
}

# Dynomite's settings as a run of the game leaves them (no game data), so every run starts from the same ones
dynomite_user() {  # <user dir>
	mkdir -p "$1"
	printf '%s\n' '[SOFTWARE\PopCap\Dynomite]' CustomCursors=dword:0 InProgress=dword:0 MusicVolume=dword:50 Muted=dword:0 \
		PreferredX=dword:-1 PreferredY=dword:-1 ScreenMode=dword:1 SfxVolume=dword:85 TimesExecuted=dword:5 \
		TimesPlayed=dword:0 WaitForVSync=dword:0 > "$1/registry.ini"
}

dynomite_exe() {  # <dir>: the game with <dir>/script.txt and <dir>/user; prints its "board" lines
	local e=0 l u
	u="$(cygpath -ma "$1")"
	timeout 300 "$W/build/native/dynomite.exe" -gamedir=assets/dynomite -userdir="$u/user" -fast -seed=2 \
		-script="$u/script.txt" > "$1/stdout.txt" 2> "$1/stderr.txt" || e=$?
	l="$(cat "$1/stdout.txt" "$1/stderr.txt" | tr -d '\r' | sed -n 's/^INFO: script: //p' | grep '^board' | paste -sd ';' - | sed 's/;/; /g')"
	[ $e -eq 0 ] || l="$l; EXIT $e"
	echo "$l"
}

dynomite_run() {  # <name>: Stomped Puzzle, five shots, the board, then saved into the first slot; prints the board
	local d="$OUT/dynomite/$1" u
	rm -rf "$d"; dynomite_user "$d/user"
	u="$(cygpath -ma "$d")"
	# the title, New Game, Stomped Puzzle and OK, Easy and OK; the shots; the side bar's Save, the first slot, Save,
	# and the name's Save (an update is 10 ms)
	printf '%s\n' "600 click 320 450" "910 click 486 213" "970 click 465 230" "1030 move 488 432" "1040 click 488 432" \
		"1100 click 465 162" "1150 move 488 432" "1160 click 488 432" "1700 click 260 140" "1745 click 297 140" \
		"1790 click 334 140" "1835 click 371 140" "1880 click 408 140" "2095 board" "2100 screenshot $u/board.png" \
		"2110 move 45 297" "2120 click 45 297" "2205 move 284 207" "2210 click 284 207" "2265 move 354 399" \
		"2270 click 354 399" "2355 move 357 361" "2360 click 357 361" "2450 screenshot $u/saved.png" "2460 quit" > "$d/script.txt"
	dynomite_exe "$d"
}

dynomite_load() {  # <name> <run>: the run's saved game continued from the main menu; prints the board
	local d="$OUT/dynomite/$1" u
	rm -rf "$d"; mkdir -p "$d"; cp -r "$OUT/dynomite/$2/user" "$d/user"
	u="$(cygpath -ma "$d")"
	# the title, Continue Game and Next, the first saved game and Let's Play
	printf '%s\n' "600 click 320 450" "910 move 487 286" "920 click 487 286" "960 move 491 434" "970 click 491 434" \
		"1005 move 380 175" "1010 click 380 175" "1065 move 491 434" "1070 click 491 434" "1110 board" \
		"1120 screenshot $u/loaded.png" "1130 quit" > "$d/script.txt"
	dynomite_exe "$d"
}

suite_dynomite() {
	local st=0 g r l c="$OUT/dynomite" t
	need_assets dynomite || return 1
	g="$(dynomite_run game)"; r="$(dynomite_run game2)"; l="$(dynomite_load load game)"
	{ echo "game: $g"; echo "rerun: $r"; echo "load: $l"; } > "$1"
	[ "$g" = "$DY_GAME" ] || st=1
	# a saved game's only bytes that change from run to run: the tick counts at the start of its board (0x3868)
	if [ -f "$c/game/user/SLOT0.SAV" ]; then
		t="$(cmp -l "$c/game/user/SLOT0.SAV" "$c/game2/user/SLOT0.SAV" 2>&1 | awk '$1 < 14441 || $1 > 14448' | head -n 3)"
	else t="no SLOT0.SAV"; fi
	if diff -q "$c/game/board.png" "$c/game2/board.png" >> "$1" 2>&1 && diff -q "$c/game/saved.png" "$c/game2/saved.png" >> "$1" 2>&1 &&
		[ "$g" = "$r" ] && [ -z "$t" ]; then r="rerun identical"; else r="RERUN DIFFERS${t:+ ($t)}"; st=1; fi
	if [ "$l" = "$g" ]; then l="loads as saved"; else l="LOAD DIFFERS ('$l')"; st=1; fi
	SUMMARY="$g; $r; $l"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected '$DY_GAME')"
	return $st
}

bonnies_run() {  # <name> [<exe>]: games/bonnies/tools/tour_test.txt into $OUT/bonnies/<name>; prints the exit status
	local d="$OUT/bonnies/$1" e=0
	rm -rf "$d"; mkdir -p "$d/user"
	sed "s|@DIR@|$d|" games/bonnies/tools/tour_test.txt > "$d/script.txt"
	timeout 300 "${2:-$W/build/native/bonnies.exe}" -gamedir=assets/bonnies -userdir="$d/user" -fast -seed=1 \
		-script="$d/script.txt" > "$d/stdout.txt" 2>&1 || e=$?
	echo "$e"
}

BB_SHOTS_ALL="logo menu name difficulty story board tutorial word played cake tale gameoptions highscores credits help1 help2 help3 mainmenu options action actionword paused resumed timeup endmenu"

bonnies_same() {  # <a> <b>: whether two runs gave the same screens and the same saved files
	local f
	for f in $BB_SHOTS_ALL; do cmp -s "$1/$f.png" "$2/$f.png" || return 1; done
	diff -rq "$1/user" "$2/user" > /dev/null
}

suite_bonnies() {
	local d="$OUT/bonnies" e f sh r bl="" base="$W/build/baseline-bonnies/bonnies.exe"
	need_assets bonnies || return 1
	e="$(bonnies_run game)"
	cat "$d/game/stdout.txt" > "$1"
	[ "$e" -eq 0 ] || { SUMMARY="the exe exited with status $e"; return 1; }
	grep -q "Resource Loading Time" "$1" || { SUMMARY="the game did not finish loading"; return 1; }
	for f in $BB_SHOTS_ALL; do
		[ -s "$d/game/$f.png" ] || { SUMMARY="no screenshot $f.png"; return 1; }
	done
	cmp -s "$d/game/menu.png" "$d/game/name.png" && { SUMMARY="Classic Game opened no name dialog"; return 1; }
	cmp -s "$d/game/tutorial.png" "$d/game/word.png" && { SUMMARY="the clicks chose no letters"; return 1; }
	cmp -s "$d/game/word.png" "$d/game/played.png" && { SUMMARY="Submit played no word"; return 1; }
	[ -s "$d/game/user/SavedGames/te.GAM" ] || { SUMMARY="the new player's te.GAM was not saved"; return 1; }
	# the main menu's Quit ends the game before the script's own quit at update 70000
	grep -q "Update Count = 6[0-9]\{4\}," "$1" || { SUMMARY="the main menu's Quit did not end the game"; return 1; }
	# the rerun, and the frozen baseline (build/baseline-bonnies/bonnies.exe, a copy of the port made when it was last
	# checked against the original) when there is one, at the same time
	bonnies_run rerun > "$d/rerun.status" &
	[ -f "$base" ] && bonnies_run base "$base" > "$d/base.status" &
	wait
	cat "$d/rerun/stdout.txt" >> "$1"
	[ "$(cat "$d/rerun.status")" = 0 ] || { SUMMARY="the rerun exited with status $(cat "$d/rerun.status")"; return 1; }
	bonnies_same "$d/game" "$d/rerun" || { SUMMARY="the rerun's screens or saves differ"; return 1; }
	sh="$(for f in played highscores timeup; do sha256sum "$d/game/$f.png" | cut -c1-12; done | paste -sd" " -)"
	[ "$sh" = "$BB_SHOTS" ] || { SUMMARY="SCREENS CHANGED ($sh; frozen $BB_SHOTS)"; return 1; }
	if [ -f "$base" ]; then
		bonnies_same "$d/game" "$d/base" || { SUMMARY="the frozen baseline's screens or saves differ"; return 1; }
		bl="; baseline identical"
	fi
	SUMMARY="the tour (classic tutorial, options, high scores, credits, help, an action game paused by a focus loss and played to its end, Quit); rerun identical; screens as frozen$bl"
	return 0
}

mahjongjapan_run() {  # <name>: a new player, escape stage 1 won with the solver's clicks; prints the moves and the shots
	local d="$OUT/mahjongjapan/$1" t=3400 x1 y1 x2 y2 n=0
	rm -rf "$d"; mkdir -p "$d/shots" "$d/user"
	# Emperor's Adventure, the name "Test", the map's PLAY, the How To Play tip's OK; the solver's pairs; the Magic
	# Gold Tiles tip's OK, the level's end and its OK, the map. No click reaches a link.
	{
		printf '%s\n' "1400 click 400 270" "1720 char T" "1725 char e" "1730 char s" "1735 char t" "1760 key RETURN" \
			"2600 click 715 573" "3000 click 400 407" "3300 screenshot $d/shots/board.png"
		while read -r x1 y1 x2 y2; do
			echo "$t click $x1 $y1"; echo "$((t + 60)) click $x2 $y2"; t=$((t + 180)); n=$((n + 1))
		done < "$OUT/mahjongjapan/sol385.txt"
		printf '%s\n' "$((t + 400)) click 403 330" "$((t + 2400)) screenshot $d/shots/won.png" "$((t + 2600)) click 397 400" \
			"$((t + 5600)) screenshot $d/shots/map.png" "$((t + 5700)) quit"
	} > "$d/script.txt"
	timeout 300 "$W/build/native/mahjongjapan.exe" -gamedir=assets/mahjongjapan -userdir="$d/user" -fast -script="$d/script.txt" \
		> "$d/stdout.txt" 2> "$d/stderr.txt"
	echo "$n pairs; $(ls "$d/shots" | wc -l) shots; player files: $(ls "$d/user" | grep -c '\.usr$')"
}

suite_mahjongjapan() {
	local st=0 dl g r sh c="$OUT/mahjongjapan"
	need_assets mahjongjapan || return 1
	mkdir -p "$c"
	dl="$(timeout 120 build/native/mahjongjapan_dealtest.exe assets/mahjongjapan 2>&1 | tail -n 1 | tr -d '\r' | sed 's/, slowest.*//')"
	[ "$dl" = "$MJ_DEAL" ] || st=1
	timeout 120 build/native/mahjongjapan_dealtest.exe assets/mahjongjapan --solve 385 > "$c/sol385.txt" 2> "$c/solve.txt" || st=1
	g="$(mahjongjapan_run game)"; r="$(mahjongjapan_run game2)"
	{ echo "deal: $dl"; echo "game: $g"; echo "rerun: $r"; } > "$1"
	sh="$(for f in board won map; do sha256sum "$c/game/shots/$f.png" 2>/dev/null | cut -c1-12; done | paste -sd" " -)"
	if [ "$sh" = "$MJ_SHOTS" ]; then sh="screens as frozen"; else sh="SCREENS CHANGED ($sh)"; st=1; fi
	if diff -rq "$c/game/shots" "$c/game2/shots" >> "$1" 2>&1 && [ "$g" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="$dl; $g; $r; $sh"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected '$MJ_DEAL')"
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
