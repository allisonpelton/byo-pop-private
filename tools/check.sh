#!/usr/bin/env bash
# The project's builds and tests, one summary line each: what a session needs to read to know whether a
# change is good. Every log goes to a file; open one only when its suite fails (docs/TOKEN_ECONOMY.md).
#
#   tools/check.sh [--out <dir>] [--jobs <n>] <suite or set>...
#   tools/check.sh [--out <dir>] --changed <rev> [--inert] [--dry-run] [<suite or set>...]    (merge mode, below)
#
# Suites (rough times on this machine):
#   build        tools/build.sh native, incremental (seconds to minutes)
#   build-web    tools/build.sh web; `build-web:<target>,<target>` builds only those targets (TARGET="<a> <b>"
#                tools/build.sh web), as merge mode does for the games a merge touches
#   build-nov    tools/build.sh novariants (Bejeweled without its variant code, for the oracle)
#   unit         rulestest, modsettingstest and fwtest -hotkeytest, native (under a minute); fwtest is skipped
#                without assets/bejeweled; then fwtest -hotkeytest -placeholders with no game files at all; then
#                jp2test (JPEG 2000: the .jp2 images of assets/bookwormadv/images against re/bookwormadv/jp2ref, our
#                decoder's own earlier output, which tools/jp2test/write_refs.sh writes; the loader's
#                rules for .jp2 files, synthetic and damaged files), skipped without assets/bookwormadv/images;
#                songpostest on Bejeweled's, BookWorm's and Bejeweled Twist's modules, with the MO3s' tempo and
#                channel volumes (skipped without their assets); spectrumtest
#                (the music player's FFT against BASS 2.3's on the same tones); and soundtest (sounds kept as 16-bit at
#                their own rate, voices against the old stereo-float pipeline); and shotdiff's selftest (tools/shotdiff:
#                the screenshot comparison on synthetic pictures, its pages and its marks server); and geomtest
#                (framework/DisplayGeometry: letterbox, render scale, window mapping and cursor, against a table)
#   unit-web     rulestest and modsettingstest under Node (needs build-web); paktest (the page's pak reader), with
#                Bookworm Adventures' main.pak against assets/bookwormadv when both are there; profilestest (the page's
#                settings and players code, web/settings.js and web/profiles.js, on a fake localStorage), savestest (the
#                save backup's zip, paths and merge, web/saves.js), saveimporttest (the import of saves from an installed game:
#                platform/saveimport.json, the .reg reader and the conversion, web/saveimport.js), chattest (the chat's username,
#                text and rate rules, its stored history and local source, and that a message goes on the page as text only,
#                web/chat.js), gameimporttest (the import rules for a game's files and the sorting of dropped folders into games,
#                web/gameimport.js) and a11ycheck (tools/a11ycheck.js: the site's pages in the build, as index.html lists them; each page's
#                landmarks, headings, names, references, live regions and contrast) and stringstest (tools/stringstest.js: the
#                site's text, web/strings/en.txt, against the code that uses it, and web/i18n.js's rules), playcounttest (the play
#                counter's Worker and Durable Object, web/counter/playcount.mjs, on stand-ins for Cloudflare's runtime) and
#                playcountpagetest (its pages' side, web/playcount.js: when it may contact the counter) and termstest (web/terms.js:
#                when a page skips the terms card, the development hosts, ?terms=1 and the test key). The build folder is build/web;
#                BYOPOP_WEB_DIR=build/web-test names another
#   gpu          the GL GPUInterface's Direct3D 9 conventions on the real GPU (tools/gputest: clears, pixel centres,
#                depth, culling, matrices, textures, render targets, blending, alpha test, texture stages, scissor,
#                lighting, game shaders, buffers, precompiled programs, the null device), exact pixels (seconds)
#   origrun      tools/origrun/build.sh into build/logs/check/origrun-bin (not into build/origrun-bin, whose origrun.exe
#                may be running): that the host and its sound stand-in still compile (seconds); not in `all`
#   verify      Bejeweled's headless verifier tests (games/bejeweled/tools/verify/run_tests.sh)
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
#                spectrum of a song, both ways, with the mouse over the book (about two minutes, 90 s of them
#                the load of all 269 animations with their pictures; the quick form loads every 45th instead)
#   pixelus      Pixelus Deluxe's grid replay test (every puzzle of the game solved by its recorded solution), a scripted tour of the
#                options, credits and how-to-play screens, the bot's first 25 puzzles and a rerun with the same screenshots,
#                and the bot's whole game (210 puzzles in all 12 temples, to the ending; about 2 minutes; the quick form
#                leaves out the whole game: 40 s)
#   mummymaze    Mummy Maze's bot: a classic game of 8 chambers (the name box, the options and their dialogs photographed) and
#                a rerun with the same screenshots, a game from pyramid 3 (the tips), a whole classic game (225 chambers to the
#                victory screen), a whole adventure (15 pyramids, the map, the saved game) and the tutorial (about 1 minute)
#   tiptop       TipTop's bot: two regular games, a strategy game and a speed game, and a rerun of the regular ones
#                with the same screenshots (about two and a half minutes; the quick form leaves out the speed game and
#                reruns the strategy game instead of the regular ones: 50 s); with build/baseline-tiptop/ also three
#                mouse-played games (two regular games in a row, a strategy game, a regular game saved and continued)
#                that must give the frozen baseline's state at every update (about a minute more; not in the quick form)
#   ningpo       Ning Po Mahjong: a scripted game (the cheat's matches until no pair is left, the name entry, the best
#                times) and a rerun with the same screenshots and best time; with build/baseline-ningpo, every layout
#                and two games in a row that must give the frozen baseline's state log (about 20 s)
#   wordharmony  Word Harmony: a THINK game from a saved game (a ball dragged into a word, the word sent, the level
#                ended, level 2 saved) and a TSUNAMI game saved after 20 seconds, each run twice with the same
#                screenshots, and both again in build/baseline-wordharmony/, which must write the same state log
#                (about 30 seconds)
#   cosmicbugs   Cosmic Bugs: both art banks, the fonts and every animation the game ships loaded
#                (cosmicbugs_banktest), a scripted game played headless (cosmicbugs_run: two lines on the first
#                board) and a rerun with the same screenshots, the windowed exe twice with the same clicks: the
#                runner's state, the same screen, and the series of games that play out as the original's
#                (games/cosmicbugs/test/plays) against build/baseline-cosmicbugs/'s runner when there is one
#                (under a minute)
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
#                was saved (dynomite_load); with build/baseline-dynomite/ (frozen at its identical-play pass), three of
#                that pass's cases must give the baseline's state log (dynomite_state) (about a minute)
#   bonnies      Bonnie's Bookstore Deluxe: a tour from a script (games/bonnies/tools/tour_test.txt): a new player,
#                the classic game's tutorial words, the game's options, high scores, credits and help, the main menu's
#                options, an action game paused by a focus loss and played until its time is up, and Quit; a rerun
#                with the same screens and saves, which are the frozen baseline's (and build/baseline-bonnies/
#                bonnies.exe's, when there is one) (under a minute)
#   mahjongjapan Mahjong Escape: Ancient Japan: every layout loaded and dealt (mahjongjapan_dealtest), and a scripted
#                escape stage from a new player (the name, the map, stage 1 won with the solver's clicks, the level's
#                end and the map again) twice with the same screens, which are the frozen baseline's (under a minute)
#   mahjongchina Mahjong Escape: Ancient China: every layout loaded and dealt (mahjongchina_dealtest), and a scripted
#                Dynasty Adventure stage from a new player (the name, the map, stage 1 won with the solver's clicks, the
#                level's end and the map again) twice with the same screens, which are the frozen baseline's (under a
#                minute)
#   bejeweledtwist Bejeweled Twist's autoplay in 2D, seeded: a Classic game to update 6000 and a Blitz game to its
#                end and the high scores, with the same screens as frozen, a Classic rerun, and
#                build/baseline-bejeweledtwist/bejeweledtwist.exe's when there is one (about a minute)
#   oracle       Bejeweled's oracle three ways (compare.sh --variants default,forcevariant,novariants); over
#                ten minutes, so never in a subagent; refuses to start while another oracle runs
# Sets: quick = build unit; games = bookworm bigmoney noahsark atomica alchemy sevenseas pixelus bookwormadv bookwormadv2 tiptop mummymaze ningpo wordharmony cosmicbugs waterbugs rocketmania dynomite mahjongjapan mahjongchina bonnies bejeweledtwist; all = every suite, builds first.
#
# Prints "PASS <suite> (<seconds>s): <summary>" or "FAIL <suite> (<seconds>s): <summary> -> <log>" for each,
# then "check: N passed, M failed". Exits 0 only when all passed. Logs: build/logs/check/<suite>.txt (and
# the suites' own output folders). Runs itself under build/native/rundesk.exe, so every game it starts is
# invisible, silent and low priority (CLAUDE.md); suites run one after another (--jobs, below, is the exception).
#
# Quick forms: pixelus, bookwormadv and tiptop, the slowest game suites, have a shorter form that still starts the game,
# plays it and checks that a rerun gives the same result. `<suite>:quick` runs it (CHECK_QUICK=1 does so for every suite
# named); the suite's summary line says that it was the quick form. A quick form leaves out only what dominates the time
# (see the suites above) and never changes the normal suite. The other suites have none and ignore the request.
#
# --jobs <n> (1 to 3; 1 by default, 3 in merge mode) runs the game suites of the `games` set, longest first, up to n game
# processes at a time (bonnies, which starts two at once when build/baseline-bonnies/bonnies.exe exists, counts as two).
# Each suite's line is printed whole when it ends, so the lines come in the order the suites end. The builds, unit,
# unit-web, gpu, verify, modmenu, livesplit and the oracle always run alone.
#
# Merge mode, --changed <rev>: picks the suites a change can affect from `git diff --name-only <rev>` (the working tree,
# staged changes included, against <rev>: a merge made with --no-commit is checked with `--changed HEAD`; `<a>..<b>` is
# `git diff --name-only <a> <b>`) instead of running them all. Files that need nothing (docs/**, *.md, testnotes/**,
# .publishignore, games/*/docs/**, .gitignore, .gitattributes, .claude/**) choose no suite. The others choose, as a union
# in the order `all` uses:
#   games/<g>/**   build, unit, the game's own suites (bejeweled: build-nov verify modmenu livesplit oracle; bigmoney:
#                  bigmoney livesplit; bookworm: bookworm livesplit; any other game: the suite of its name, if there is one)
#                  and the web build of that game only (build-web:<g>; Bejeweled's also rulestest and bejeweled_verify)
#   web/**, cmake/ByopopWeb.cmake, cmake/ByopopStores.cmake, platform/saveimport.json
#                  build-web and unit-web (seconds when only the listing page changed, but a change to web/modpanel.js,
#                  web/shell.html or a link option relinks every game's wasm: about 6 minutes)
#   tools/<x>/**   shotdiff: unit. gputest: build gpu. modmenutest: modmenu. origrun: origrun. The web test scripts
#                  (tools/*.js, paktest): unit-web. fwtest, geomtest, jp2test, songpostest, soundtest, spectrumtest, build.sh,
#                  check.sh, rundesk: build unit. modsettingstest and fixtures: build unit, build-web:modsettingstest and
#                  unit-web. Anything else under tools/, and any file not named here: build
#   framework/**, platform/**, third_party/**, CMakeLists.txt, the other cmake/**: without --inert, every suite, as `all`.
#                  With --inert (the review found every shared change off unless a game opts in, or reached only from the
#                  merging game): build, unit, gpu when a GPU or GL file changed (framework/GPU*, platform/GL*), the web
#                  build of the games whose folders changed and of the shared libraries (sexyframework, sexyopenmpt,
#                  sexyopenjpeg, sexycompat2004: so that an error only wasm shows is found), and every game suite in its
#                  quick form (a game whose own folder changed runs in full). It skips the oracle, livesplit, verify,
#                  modmenu, build-nov and the full web build.
# --dry-run prints the choice and exits: one line for each suite with a rough time and the areas (a path, when one file)
# that chose it, then the total. A real run prints the same lines first. When nothing is chosen (docs only) nothing runs and
# the exit status is 0. Suites named besides --changed are added in full.
#
# The bots' expected results are below. A change that is meant to alter how a game plays updates them, in
# the same commit, with the reason.
BW_CLASSIC="level 5, score 19230"
BW_ACTION="level 11, score 87580"
BM_GAME1="score 66646"
NA_ACTION="level 4 done at update 17369: score 6524 mode stampede"
NA_STRATEGY="level 5 done at update 19538: score 6380 mode strategy"
NA_PUZZLE="game 1 over at update 6750: score 1450 level 1 moves 37"
AT_STRATEGY="game 1 over at update 2396: score 320 level 1 moves 31"
AT_ACTION="update 20000: score 2960 level 3 moves 567 atoms 58"
AL_STRATEGIC="game 1 over at update 12206: score 7432 pieces 830 forged 29 boards 3"
AL_TIMETRIAL="game 1 over at update 14641: score 15942 pieces 903 forged 87 boards 3"
AL_TUTORIAL="tutorial button 1 at update 231"
SS_BEGINNER="game 2 over at update 5868: score 380 level 4 moves 20"
SS_BONUS="game 1 over at update 2746: score 680 level 14 moves 20"
SS_LATE="game 1 over at update 1852: score 140 level 21 moves 13"
PX_GRID="gridtest: 213 level files, 211 solutions, 211 solved, 0 without a level file"
PX_START="done: 25 puzzles"
PX_FULL="done: no temple left, 210 puzzles, log 357cb9be"
MM_CLASSIC="game over: score 1500, list 1500 1000 900 800 700, the bot is number 1"
MM_TIPS="game over: score 1730, list 1730 1000 900 800 700, the bot is number 1"
MM_FULL="victory: score 251975, caught 0 times"
MM_ADVENTURE="victory: score 0, caught 49 times"
MM_TUTORIAL="tutorial done at update 6591"
TT_REGULAR="game 2 over at update 35544: level 7, score 37160, 352 moves"
TT_STRATEGY="game 1 over at update 2593: level 1, score 2100, 24 moves"
TT_SPEED="game 1 over at update 37679: level 8, score 49360, 682 moves"
NP_GAME="Test: 40 tiles left in 00:12"
WH_THINK="level 2 saved, score 2000"
WH_TSUNAMI="level 1 saved, score 0, 14 balls, 5 to come"
CB_BANKS="82 animations, 9 missing, 0 failed; 8 fonts, 0 failed"
CB_GAME="17000 ms: score 526, lives 6, board 0, filled 426 of 85%; 28000 ms: score 729, lives 6, board 0, filled 629 of 85%"
WB_BANKS="77 animations, 10 missing, 0 failed; 7 fonts, 3 empty as in the game, 0 failed"
WB_GAME="20000 ms: score 0, lives 6, board 0, filled 0 of 85%; 32000 ms: score 494, lives 6, board 0, filled 494 of 85%"
WB_LOSS="30000 ms: score 0, lives 0, board 0, filled 0 of 90%"
WB_SHOTS="2a54908cecb8 42df2d86a04f 983a569fb1b6"  # the first 12 hex digits of the sha256 of game/start.png, game/beam.png and loss/scores.png (pixels, with the bubble column masked, see wb_hash): the frozen baseline
MJ_DEAL="222 layouts, 0 failed"
BB_SHOTS="90f2d44e0574 17f38e9b9790 fa3847002043"  # the first 12 hex digits of the sha256 of played.png, highscores.png and timeup.png (games/bonnies/tools/tour_test.txt): the frozen baseline
MJ_SHOTS="2fff3f87d787 6815b4445dc6 7bede5c481fd"  # the first 12 hex digits of the sha256 of board.png, won.png and map.png: the frozen baseline
MJC_DEAL="178 layouts, 0 failed"
MJC_SHOTS="4c18c17f02f2 b63352c04770 7a72965e0cee"  # the first 12 hex digits of the sha256 of board.png, won.png and map.png: the frozen baseline
DY_GAME="board mode 0 score 10 eggs 12 cells ec5704d5"
BT_CLASSIC="849661ca3941 645da06bba91 79a58137ffb5 b3041b28645e"  # the first 12 hex digits of the sha256 of the Classic game's screens at 1500, 3000, 4500 and 6000: the frozen baseline
BT_BLITZ="1a6389926f12 9f06a4076f1f 8a0ccc1e9da8 cdf1d61302d0 43b0fcda6095"  # the same for the Blitz game's at 3500, 6000, 8000 (the explosion), 8450 (space, on the way to the scores) and 9000 (the high scores)

set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 2
OUT="build/logs/check"
QUICK_ENV="${CHECK_QUICK:-0}"  # 1: every suite named runs its quick form

# The suites in the order `all` runs them (origrun is not in `all`), and the game suites among them
GAME_SUITES=(bookworm bigmoney noahsark atomica alchemy sevenseas pixelus bookwormadv bookwormadv2 tiptop mummymaze ningpo wordharmony cosmicbugs waterbugs rocketmania dynomite mahjongjapan mahjongchina bonnies bejeweledtwist)
ALL_SUITES=(build build-web build-nov unit unit-web gpu verify modmenu livesplit "${GAME_SUITES[@]}" oracle)
ORDER=(build build-web build-nov unit unit-web gpu origrun verify modmenu livesplit "${GAME_SUITES[@]}" oracle)
known_suite() { case " ${ORDER[*]} " in *" $1 "*) return 0 ;; esac; return 1; }
is_game_suite() { case " ${GAME_SUITES[*]} " in *" $1 "*) return 0 ;; esac; return 1; }
# suite_weight <suite>: sets WEIGHT to the game processes the suite has at once (bonnies runs its rerun and the frozen baseline's together)
suite_weight() { WEIGHT=1; [ "$1" = bonnies ] && [ -f build/baseline-bonnies/bonnies.exe ] && WEIGHT=2; return 0; }

# ---- merge mode (--changed): the suites a change can affect ----

# Rough seconds on this machine (the last `all` run, 2026-11-02); the plan adds them up, no more
declare -A EST=([build]=77 [build-web]=356 [build-nov]=72 [unit]=11 [unit-web]=8 [gpu]=1 [origrun]=5 [verify]=42 [modmenu]=50
	[livesplit]=472 [bookworm]=8 [bigmoney]=7 [noahsark]=23 [atomica]=11 [alchemy]=12 [sevenseas]=7 [pixelus]=127
	[bookwormadv]=129 [bookwormadv2]=55 [tiptop]=156 [mummymaze]=38 [ningpo]=21 [wordharmony]=17 [cosmicbugs]=5
	[waterbugs]=8 [rocketmania]=11 [dynomite]=8 [mahjongjapan]=11 [mahjongchina]=8 [bonnies]=46 [bejeweledtwist]=60 [oracle]=1080)
declare -A EST_QUICK=([pixelus]=40 [bookwormadv]=57 [tiptop]=52)

declare -A SEL=() WHY_N=() WHY_P=() AREAS=() WEB_SEEN=() SH_N=() SH_P=()
SH_ORDER=() WEB_T=() GL_FILES=()
NOTHING=0 AREA="" GS="" REASONS="" EST_S=0 WEIGHT=1 SAVEIMPORT=0
WEB_FULL_EST=10  # the whole web build, unless a change relinks every game's wasm (356 s; 10 s when it only writes the listing page)

area_of() {  # <path>: sets AREA to the folder a path is listed under in the plan
	local p="$1" top rest
	top="${p%%/*}"; rest="${p#*/}"
	if [ "$top" = "$p" ]; then AREA="$p"; return; fi
	case "$top" in
		games|tools) if [[ "$rest" == */* ]]; then AREA="$top/${rest%%/*}"; else AREA="$p"; fi ;;
		platform|third_party) if [[ "$rest" == */* ]]; then AREA="$top/${rest%%/*}"; else AREA="$top"; fi ;;
		cmake) AREA="$p" ;;
		*) AREA="$top" ;;
	esac
}

# note <suite> <area> <path> [count]: <count> files of <area> (the first is <path>) chose <suite>
note() {
	local k="$1|$2"
	if [ -z "${WHY_N[$k]:-}" ]; then WHY_N[$k]=0; WHY_P[$k]="$3"; AREAS[$1]+="$2"$'\n'; fi
	WHY_N[$k]=$((WHY_N[$k] + ${4:-1}))
}

# pick <suite> <area> <path> [full|quick] [count]: a suite is chosen; a full choice beats a quick one, and a suite
# with no quick form (QUICK_FORMS) is its own quick form
QUICK_FORMS=(pixelus bookwormadv tiptop)
pick() {
	local how="${4:-full}"
	case " ${QUICK_FORMS[*]} " in *" $1 "*) ;; *) how=full ;; esac
	note "$1" "$2" "$3" "${5:-1}"
	if [ "$how" = full ] || [ -z "${SEL[$1]:-}" ]; then SEL[$1]="$how"; fi
}

web_add() { [ -n "${WEB_SEEN[$1]:-}" ] || { WEB_SEEN[$1]=1; WEB_T+=("$1"); }; }  # <target>: a web target to build (when the whole web build is not chosen)

# game_suites <game>: sets GS to the suites that test the game, besides build and unit
game_suites() {
	GS=""
	case "$1" in
		bejeweled) GS="build-nov verify modmenu livesplit oracle" ;;
		bigmoney) GS="bigmoney livesplit" ;;
		bookworm) GS="bookworm livesplit" ;;
		*) if is_game_suite "$1"; then GS="$1"; fi ;;
	esac
}

classify() {  # <path>: chooses the suites a changed file needs
	local p="$1" g rest x s
	area_of "$p"
	case "$p" in
		*.md|docs/*|testnotes/*|.publishignore|.gitignore|.gitattributes|.claude/*) NOTHING=$((NOTHING + 1)); return ;;
	esac
	case "$p" in
		games/*/*)
			g="${p#games/}"; g="${g%%/*}"; rest="${p#games/$g/}"
			if [[ "$rest" == docs/* ]]; then NOTHING=$((NOTHING + 1)); return; fi
			pick build "$AREA" "$p"; pick unit "$AREA" "$p"
			# a game's page.js: also the listing's import rules (web/gameimport.js), which unit-web's gameimporttest checks
			if [ "$rest" = web/page.js ]; then pick unit-web "$AREA" "$p"; fi
			game_suites "$g"
			for s in $GS; do pick "$s" "$AREA" "$p"; done
			if [ -f "games/$g/CMakeLists.txt" ]; then  # the game's web targets, and nothing else of the web build
				note build-web "$AREA" "$p"
				if [ "$g" = bejeweled ]; then web_add bejeweled; web_add rulestest; web_add bejeweled_verify; else web_add "$g"; fi
			fi ;;
		web/*|cmake/ByopopWeb.cmake|cmake/ByopopStores.cmake)
			pick build-web "$AREA" "$p"; pick unit-web "$AREA" "$p"
			# what every game's page and link depend on (cmake/ByopopWeb.cmake: the shell, the post-js, the footer, the licenses,
			# and the site's text, which fills the shell)
			case "$p" in web/modpanel.js|web/shell.html|web/footer.*|web/copyright.html|web/terms.html|web/licenses/*|web/strings/*) WEB_FULL_EST=356 ;; esac ;;
		tools/*)
			x="${p#tools/}"
			case "$x" in
				shotdiff/*) pick unit "$AREA" "$p" ;;
				gputest/*) pick build "$AREA" "$p"; pick gpu "$AREA" "$p" ;;
				modmenutest/*) pick modmenu "$AREA" "$p" ;;
				origrun/*) pick origrun "$AREA" "$p" ;;
				paktest/*) pick unit-web "$AREA" "$p" ;;
				modsettingstest/*|fixtures/*)  # also built for the web, where unit-web runs it under Node
					pick build "$AREA" "$p"; pick unit "$AREA" "$p"; pick unit-web "$AREA" "$p"
					note build-web "$AREA" "$p"; web_add modsettingstest ;;
				fwtest/*|geomtest/*|jp2test/*|songpostest/*|soundtest/*|spectrumtest/*|rundesk/*|build.sh|check.sh)
					pick build "$AREA" "$p"; pick unit "$AREA" "$p" ;;
				*/*) pick build "$AREA" "$p" ;;  # smoke, ghidra_scripts and the rest: nothing runs them, but a build shows a broken tree
				*.js|*.mjs) pick unit-web "$AREA" "$p" ;;  # a11ycheck.js, chattest.js and the other web test scripts
				*) pick build "$AREA" "$p" ;;
			esac ;;
		framework/*|platform/*|third_party/*|CMakeLists.txt|cmake/*)
			if [ -z "${SH_N[$AREA]:-}" ]; then SH_N[$AREA]=0; SH_P[$AREA]="$p"; SH_ORDER+=("$AREA"); fi
			SH_N[$AREA]=$((SH_N[$AREA] + 1))
			case "$p" in framework/GPU*|framework/*GL*|platform/GL*|platform/*GL*) GL_FILES+=("$p") ;; esac
			# the table the web build embeds (build-web) and unit-web's saveimporttest reads
			if [ "$p" = platform/saveimport.json ]; then SAVEIMPORT=1; fi ;;
		*) pick build "$AREA" "$p" ;;  # not known: a build at least
	esac
}

# apply_shared: shared code (framework, platform, third_party, CMakeLists.txt, the other cmake files) chose its suites
apply_shared() {
	local a n p s x f
	for a in "${SH_ORDER[@]}"; do
		n="${SH_N[$a]}"; p="${SH_P[$a]}"
		if [ "$INERT" = 1 ]; then
			pick build "$a" "$p" full "$n"; pick unit "$a" "$p" full "$n"
			for s in "${GAME_SUITES[@]}"; do pick "$s" "$a" "$p" quick "$n"; done
			note build-web "$a" "$p" "$n"
			for x in sexyframework sexyopenmpt sexyopenjpeg sexycompat2004; do web_add "$x"; done
		else
			for s in "${ALL_SUITES[@]}"; do pick "$s" "$a" "$p" full "$n"; done
			WEB_FULL_EST=356
		fi
	done
	if [ "$INERT" = 1 ]; then for f in "${GL_FILES[@]}"; do pick gpu "$f" "$f"; done; fi
	if [ "$SAVEIMPORT" = 1 ]; then  # the table is in the web build (every game's wasm) and unit-web's saveimporttest reads it
		pick build-web platform platform/saveimport.json full 0
		pick unit-web platform platform/saveimport.json
		WEB_FULL_EST=356
	fi
}

reasons_of() {  # <suite>: sets REASONS to the areas that chose it (a path where one file did)
	local s="$1" a k n l shown=0 total=0
	REASONS=""
	while IFS= read -r a; do
		[ -n "$a" ] || continue
		total=$((total + 1))
		[ $shown -lt 5 ] || continue
		shown=$((shown + 1))
		k="$s|$a"; n="${WHY_N[$k]}"
		if [ "$n" -eq 1 ]; then l="${WHY_P[$k]}"; else l="$a ($n files)"; fi
		REASONS+="${REASONS:+, }$l"
	done <<< "${AREAS[$s]:-}"
	[ $total -le 5 ] || REASONS+=", +$((total - 5)) more"
}

web_est() {  # <target,target...>: sets EST_S to a rough time for building only those web targets
	local t libs=0
	EST_S=0
	for t in ${1//,/ }; do
		case "$t" in
			sexy*) libs=1 ;;
			bejeweled|bejeweledtwist|bookwormadv|bookwormadv2) EST_S=$((EST_S + 120)) ;;
			rulestest|bejeweled_verify|modsettingstest) EST_S=$((EST_S + 20)) ;;
			*) EST_S=$((EST_S + 45)) ;;
		esac
	done
	[ $libs -eq 0 ] || EST_S=$((EST_S + 65))
}

# select_changed <rev>: fills SUITES from the files that differ from <rev>, prints the plan
select_changed() {
	local rev="$1" f a b t tok base arg name seq=0 games=0 longest=0 wall total e q targets listing shared=""
	local -a named=("${SUITES[@]}") files=() args
	SUITES=()
	case "$rev" in
		*...*) args=("$rev") ;;
		*..*) a="${rev%%..*}"; b="${rev#*..}"; args=("$a" "$b") ;;
		*) args=("$rev") ;;
	esac
	listing="$(git -c core.quotepath=off diff --name-only --no-renames "${args[@]}" 2>/dev/null)" || {
		echo "check: git diff --name-only ${args[*]} failed: $(git diff --name-only "${args[@]}" 2>&1 >/dev/null | head -n 1)" >&2
		exit 2
	}
	[ -z "$listing" ] || mapfile -t files <<< "$listing"
	for f in "${files[@]}"; do classify "$f"; done
	[ ${#SH_ORDER[@]} -eq 0 ] || apply_shared
	for tok in "${named[@]}"; do  # suites named besides --changed
		base="${tok%%:*}"; arg=""
		[[ "$tok" == *:* ]] && arg="${tok#*:}"
		if [ "$base" = build-web ] && [ -n "$arg" ] && [ "$arg" != quick ]; then
			note build-web "(named)" "$tok"
			for t in ${arg//,/ }; do web_add "$t"; done
		elif [ "$arg" = quick ]; then pick "$base" "(named)" "$tok" quick
		else pick "$base" "(named)" "$tok"
		fi
	done
	if [ ${#SH_ORDER[@]} -gt 0 ]; then
		if [ "$INERT" = 1 ]; then shared="; shared code changed, inert (--inert): quick forms of the game suites, no oracle, livesplit, verify, modmenu or full web build"
		else shared="; shared code changed, not inert (no --inert): every suite, as all"; fi
	fi
	echo "check: --changed $rev: ${#files[@]} files, $NOTHING need no suite$shared"
	for name in "${ORDER[@]}"; do
		targets=""; q=""
		if [ "$name" = build-web ]; then
			if [ -n "${SEL[build-web]:-}" ]; then tok="build-web"; e="$WEB_FULL_EST"
			elif [ ${#WEB_T[@]} -gt 0 ]; then
				targets="$(IFS=,; echo "${WEB_T[*]}")"; tok="build-web:$targets"; web_est "$targets"; e="$EST_S"
			else continue
			fi
		else
			[ -n "${SEL[$name]:-}" ] || continue
			tok="$name"; e="${EST[$name]}"
			if [ "${SEL[$name]}" = quick ]; then tok="$name:quick"; q=":quick"; e="${EST_QUICK[$name]:-${EST[$name]}}"; fi
		fi
		SUITES+=("$tok")
		if [ "$JOBS" -gt 1 ] && is_game_suite "$name"; then
			suite_weight "$name"; games=$((games + e * WEIGHT)); [ "$e" -le "$longest" ] || longest="$e"
		else
			seq=$((seq + e))
		fi
		reasons_of "$name"
		printf '  %-18s ~%4ss  %s<- %s\n' "$name$q" "$e" "${targets:+[${targets//,/ }] }" "$REASONS"
	done
	if [ ${#SUITES[@]} -eq 0 ]; then echo "check: nothing to run"; return; fi
	wall=$(((games * 115 / 100 + JOBS - 1) / JOBS)); [ "$wall" -ge "$longest" ] || wall="$longest"
	total=$((seq + wall))
	e="${seq}s for the suites that run alone"
	[ "$games" -eq 0 ] || e="$e, ${wall}s for the game suites ($JOBS at a time)"
	if [ "$total" -lt 90 ]; then t="under 2 minutes"; else t="about $(((total + 30) / 60)) minutes"; fi
	echo "check: $t ($e; the time of a web build of some targets is a guess)"
}

# ---- arguments ----

SUITES=()
CHANGED=""; INERT=0; DRY=0; JOBS=""
while [ $# -gt 0 ]; do
	case "$1" in
		--out) [ $# -ge 2 ] || { echo "check.sh: --out needs a folder" >&2; exit 2; }; OUT="$2"; shift 2 ;;
		--out=*) OUT="${1#--out=}"; shift ;;
		--changed) [ $# -ge 2 ] || { echo "check.sh: --changed needs a revision" >&2; exit 2; }; CHANGED="$2"; shift 2 ;;
		--changed=*) CHANGED="${1#--changed=}"; shift ;;
		--inert) INERT=1; shift ;;
		--dry-run) DRY=1; shift ;;
		--jobs) [ $# -ge 2 ] || { echo "check.sh: --jobs needs a number" >&2; exit 2; }; JOBS="$2"; shift 2 ;;
		--jobs=*) JOBS="${1#--jobs=}"; shift ;;
		-h|--help) sed -n '2,/^BW_CLASSIC/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//'; exit 0 ;;
		quick) SUITES+=(build unit); shift ;;
		games) SUITES+=("${GAME_SUITES[@]}"); shift ;;
		all) SUITES+=("${ALL_SUITES[@]}"); shift ;;
		build-web:*) SUITES+=("$1"); shift ;;  # build-web:<target>,<target>...: only those web targets
		*:quick) if known_suite "${1%:quick}"; then SUITES+=("$1"); shift; else echo "check.sh: unknown suite '$1' (tools/check.sh --help)" >&2; exit 2; fi ;;
		*) if known_suite "$1"; then SUITES+=("$1"); shift; else echo "check.sh: unknown suite '$1' (tools/check.sh --help)" >&2; exit 2; fi ;;
	esac
done
[ -n "$JOBS" ] || { [ -n "$CHANGED" ] && JOBS=3 || JOBS=1; }
case "$JOBS" in
	1|2|3) ;;
	""|0|*[!0-9]*) echo "check.sh: --jobs takes 1, 2 or 3" >&2; exit 2 ;;
	*) echo "check.sh: --jobs $JOBS is more than the 3 game processes at a time CLAUDE.md allows; using 3" >&2; JOBS=3 ;;
esac

if [ -n "$CHANGED" ]; then
	select_changed "$CHANGED"
	[ "$DRY" -eq 0 ] || exit 0
	[ ${#SUITES[@]} -gt 0 ] || exit 0
elif [ "$DRY" -eq 1 ]; then
	echo "check: would run: ${SUITES[*]:-nothing}"
	exit 0
fi
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
	exec "$RUNDESK" bash "$ROOT/tools/check.sh" --out "$OUT" --jobs "$JOBS" "${SUITES[@]}"
fi

mkdir -p "$OUT"
NODE="$(ls "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe 2>/dev/null | head -n 1)"
W="$(pwd -W)"
SUMMARY=""
if [ "$JOBS" -gt 1 ]; then  # --jobs needs `wait -n -p` (bash 5.1)
	wait -n -p _pid 2>/dev/null
	if [ $? -eq 2 ]; then echo "check.sh: this bash has no wait -n -p; running one suite at a time" >&2; JOBS=1; fi
fi

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
suite_build-web() {  # <log> [<target>,<target>...]: the whole web build, or only those targets
	local st t="${2//,/ }"
	if [ -z "$t" ]; then build_suite web "$1"; return; fi
	export TARGET="$t"
	build_suite web "$1"; st=$?
	unset TARGET
	[ $st -ne 0 ] || SUMMARY="web build ok: $t only"
	return $st
}
suite_build-nov() { build_suite novariants "$1"; }

suite_unit() {
	local st=0 a b c d e f g h i j
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
	# jp2test decodes Bookworm Adventures' .jp2 images against reference PNGs: the decoder's own output in
	# re/bookwormadv/jp2ref (tools/jp2test/write_refs.sh), a regression check; skipped without the staged folder
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
		n=3
		# Bejeweled Twist's MO3s: also their tempo and channel volumes
		if [ -d assets/bejeweledtwist ]; then
			timeout 120 build/native/songpostest.exe assets/bejeweledtwist "music/bejewel twist 06.10.mo3" \
				music/bejeweledtwist24.mo3 >> "$1.sp" 2>&1 || st=1
			n=5
		fi
		f="songpos: $(grep -c ', start at 2 ok' "$1.sp") of $n songs ok, tempo: $(grep -c 'silent channels ok' "$1.sp") ok"
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
	# framework/DisplayGeometry (letterbox, render scale, window mapping, cursor) against a table from DDInterface's code
	(cd build/native && timeout 60 ./geomtest.exe) > "$1.geo" 2>&1 || st=1
	j="$(lastof "$1.geo" 'geometry: [0-9]+ cases, [0-9]+ checks, [0-9]+ failed')"
	cat "$1.mst" "$1.fw" "$1.ph" "$1.jp2" "$1.sp" "$1.fft" "$1.snd" "$1.sdf" "$1.geo" >> "$1"
	rm -f "$1.mst" "$1.fw" "$1.ph" "$1.jp2" "$1.sp" "$1.fft" "$1.snd" "$1.sdf" "$1.geo"
	SUMMARY="$a; $b; $c; $d; $e; $f; $g; $h; $i; $j"
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
	local d e f g h i j
	"$NODE" tools/profilestest.js > "$1.pt" 2>&1 || st=1
	d="$(lastof "$1.pt" 'profilestest:')"
	# the save backup's zip, paths and merge (web/saves.js)
	"$NODE" tools/savestest.js > "$1.sv" 2>&1 || st=1
	e="$(lastof "$1.sv" 'savestest:')"
	# the import of saves from an installed game (web/saveimport.js, and the table platform/saveimport.json it shares with the native games)
	"$NODE" tools/saveimporttest.js > "$1.si" 2>&1 || st=1
	g="$(lastof "$1.si" 'saveimporttest:')"
	# the import rules for a game's files and the sorting of dropped folders into games (web/gameimport.js)
	"$NODE" tools/gameimporttest.js > "$1.gi" 2>&1 || st=1
	i="$(lastof "$1.gi" 'gameimporttest:')"
	# the chat's rules, stored history and local source, and its plain-text rendering (web/chat.js)
	"$NODE" tools/chattest.js > "$1.ch" 2>&1 || st=1
	h="$(lastof "$1.ch" 'chattest:')"
	# the built pages' accessibility (tools/a11ycheck.js): one line per page in the log
	"$NODE" tools/a11ycheck.js --selftest > "$1.a11y" 2>&1 || st=1
	"$NODE" tools/a11ycheck.js "$web" >> "$1.a11y" 2>&1 || st=1
	f="$(lastof "$1.a11y" 'a11ycheck:')"
	# the site's text (web/strings/en.txt) against the code that uses it, and web/i18n.js's rules
	"$NODE" tools/stringstest.js > "$1.str" 2>&1 || st=1
	j="$(lastof "$1.str" 'stringstest:')"
	local k l
	"$NODE" tools/playcounttest.mjs > "$1.pc" 2>&1 || st=1
	k="$(lastof "$1.pc" 'playcounttest:')"
	"$NODE" tools/playcountpagetest.js > "$1.pcp" 2>&1 || st=1
	l="$(lastof "$1.pcp" 'playcountpagetest:')"
	local m
	"$NODE" tools/termstest.js > "$1.tt" 2>&1 || st=1
	m="$(lastof "$1.tt" 'termstest:')"
	cat "$1.mst" "$1.pak" "$1.pt" "$1.sv" "$1.si" "$1.ch" "$1.gi" "$1.a11y" "$1.str" "$1.pc" "$1.pcp" "$1.tt" >> "$1"
	rm -f "$1.mst" "$1.pak" "$1.pt" "$1.sv" "$1.si" "$1.ch" "$1.gi" "$1.a11y" "$1.str" "$1.pc" "$1.pcp" "$1.tt"
	SUMMARY="$a; $b; $c; $d; $e; $g; $h; $i; $f; $j; $k; $l; $m"
	return $st
}

suite_gpu() {
	(cd build/native && timeout 120 ./gputest.exe) > "$1" 2>&1
	local st=$?
	SUMMARY="$(lastof "$1" '^gpu:')"
	return $st
}

suite_origrun() {  # the host and its sound stand-in compile (into a folder of this run: origrun.exe may be running)
	ORIGRUN_OUT="$W/$OUT/origrun-bin" tools/origrun/build.sh > "$1" 2>&1
	local st=$?
	if [ $st -eq 0 ]; then SUMMARY="origrun.exe and dsound.dll compile"; else SUMMARY="$(grep -m 1 -E 'error|not found' "$1" | cut -c1-160)"; fi
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

bookworm_state_game() {  # <name> <exe> <user dir> <port arguments...>: a bot run with no mods; its state log is <name>/state.txt
	local d="$OUT/bookworm/$1" e="$2" u="$3"; shift 3
	rm -rf "$d"; mkdir -p "$d/mods"
	[ -d "$u" ] || mkdir -p "$u"
	timeout 900 "$e" -userdir="$u" -modsdir="$d/mods" -fast -statelog="$d/state.txt" "$@" > "$d/stdout.txt" 2> "$d/stderr.txt"
}

suite_bookworm() {
	local st=0 c a r
	need_assets bookworm || return 1
	c="$(bookworm_game classic classic)"; a="$(bookworm_game action action)"; r="$(bookworm_game classic2 classic)"
	{ echo "classic: $c"; echo "action: $a"; echo "classic rerun: $r"; } > "$1"
	case "$c" in *"$BW_CLASSIC"*) ;; *) st=1 ;; esac
	case "$a" in *"$BW_ACTION"*) ;; *) st=1 ;; esac
	if diff -rq "$OUT/bookworm/classic/shots" "$OUT/bookworm/classic2/shots" >> "$1" 2>&1; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	# the frozen baseline (build/baseline-bookworm/, the port when its identical-play pass was done): games the pass played
	# against the original (two and three games in a session, a Classic game quit half way and continued from its saved
	# file) must give the same state at every update in both
	local base="$W/build/baseline-bookworm/bookworm.exe" bl="" k x
	if [ -f "$base" ]; then
		for k in cur base; do
			x="$W/build/native/bookworm.exe"; [ $k = base ] && x="$base"
			rm -rf "$OUT/bookworm/$k-user"
			bookworm_state_game "$k-c2" "$x" "$OUT/bookworm/$k-user/c2" -seed=7 -autoplay=2 -autoplaymode=both -autoplaycap=60000
			bookworm_state_game "$k-m1" "$x" "$OUT/bookworm/$k-user/m1" -seed=9 -autoplay=7 -autoplaymode=classic,action,classic
			bookworm_state_game "$k-s1" "$x" "$OUT/bookworm/$k-user/s1" -seed=11 -autoplay=3 -autoplaymode=classic -autoplaycap=3000
			bookworm_state_game "$k-s1b" "$x" "$OUT/bookworm/$k-user/s1" -seed=11 -autoplay=4 -autoplaymode=classic -autoplaycontinue=1
		done
		for k in c2 m1 s1 s1b; do
			[ -s "$OUT/bookworm/cur-$k/state.txt" ] && [ -s "$OUT/bookworm/base-$k/state.txt" ] &&
				cmp -s "$OUT/bookworm/cur-$k/state.txt" "$OUT/bookworm/base-$k/state.txt" || bl="$bl $k"
		done
		[ -z "$bl" ] && bl="; baseline identical" || { bl="; BASELINE DIFFERS:$bl"; st=1; }
	fi
	SUMMARY="classic ${c#*-> inactive, }; action ${a#*-> inactive, }; $r$bl"
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

noahsark_game() {  # <name> <mode> <cap> [exe]: a bot game; prints the last "level done" or "game over" line
	local d="$OUT/noahsark/$1"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "${4:-$W/build/native/noahsark.exe}" -userdir="$d/user" -fast -seed=1 -autoplay=1 -autoplaymode="$2" \
		-autoplaycap="$3" -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" > "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(level [0-9]+ done|game [0-9]+ over)' "$d/log.txt" 2>/dev/null | tail -n 1
}

suite_noahsark() {
	local st=0 a s p r m bl="" base="$W/build/baseline-noahsark/noahsark.exe"
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
	# the frozen baseline (build/baseline-noahsark/, a copy of the port made when phase one of the identical-play pass was done): the
	# same bot games must give the same lines and the same screens, so that later fixes and mods leave unmodded play unchanged
	if [ -f "$base" ]; then
		if [ "$(noahsark_game base-action action 20000 "$base")" = "$a" ] && [ "$(noahsark_game base-strategy strategy 20000 "$base")" = "$s" ] &&
			[ "$(noahsark_game base-puzzle puzzle 30000 "$base")" = "$p" ] &&
			diff -rq "$OUT/noahsark/action/shots" "$OUT/noahsark/base-action/shots" >> "$1" 2>&1 &&
			diff -rq "$OUT/noahsark/strategy/shots" "$OUT/noahsark/base-strategy/shots" >> "$1" 2>&1 &&
			diff -rq "$OUT/noahsark/puzzle/shots" "$OUT/noahsark/base-puzzle/shots" >> "$1" 2>&1; then
			bl="; baseline identical"
		else
			bl="; BASELINE DIFFERS"; st=1
		fi
	fi
	SUMMARY="action ${a#level }; strategy ${s#level }; puzzle ${p#game }; $r; $m; $h$bl"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected action '$NA_ACTION', strategy '$NA_STRATEGY', puzzle '$NA_PUZZLE')"
	return $st
}

atomica_game() {  # <name> <mode> <cap>: a bot game; prints its last "game over" or progress line
	local d="$OUT/atomica/$1"
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "$W/build/native/atomica.exe" -userdir="$d/user" -fast -seed=1 -autoplay=1 -autoplaymode="$2" 		-autoplaycap="$3" -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" > "$d/stdout.txt" 2> "$d/stderr.txt"
	grep -E '^(level [0-9]+ done|game [0-9]+ over|update [0-9]+:)' "$d/log.txt" 2>/dev/null | tail -n 1 | sed 's/ (f8c.*//'
}

atomica_replay() {  # <name> <exe> <scenario> <end>: the port plays a scenario of games/atomica/tools/identical; its state log is $OUT/atomica/<name>/trace.txt
	local d="$OUT/atomica/$1"
	rm -rf "$d"; mkdir -p "$d"
	python "$W/games/atomica/tools/identical/portscript.py" "$W/games/atomica/tools/identical/$3" "$d/script.txt" --end "$4"
	timeout 600 "$2" -userdir="$d/user" -fast -seed=1 -drawevery -script="$d/script.txt" -statetrace="$d/trace.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
}

suite_atomica() {
	local st=0 s a r bl="" base="$W/build/baseline-atomica/atomica.exe" sc
	need_assets atomica || return 1
	if [ -f "$base" ]; then
		for sc in "s2.txt 4000" "g5.txt 6500"; do
			read -r f e <<< "$sc"
			atomica_replay "cur-$f" "$W/build/native/atomica.exe" "$f" "$e"
			atomica_replay "base-$f" "$base" "$f" "$e"
			[ -s "$OUT/atomica/cur-$f/trace.txt" ] && [ -s "$OUT/atomica/base-$f/trace.txt" ] &&
				cmp -s "$OUT/atomica/cur-$f/trace.txt" "$OUT/atomica/base-$f/trace.txt" || bl="x"
		done
		[ -z "$bl" ] && bl="; baseline identical" || { bl="; BASELINE DIFFERS"; st=1; }
	fi
	s="$(atomica_game strategy strategy 20000)"; a="$(atomica_game action action 20000)"; r="$(atomica_game strategy2 strategy 20000)"
	{ echo "strategy: $s"; echo "action: $a"; echo "strategy rerun: $r"; } > "$1"
	[ "$s" = "$AT_STRATEGY" ] || st=1
	[ "$a" = "$AT_ACTION" ] || st=1
	if diff -rq "$OUT/atomica/strategy/shots" "$OUT/atomica/strategy2/shots" >> "$1" 2>&1 && [ "$s" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="strategy ${s#game }; action ${a#update }; $r$bl"
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

alchemy_state_game() {  # <name> <exe> <mode> <bot seed> <game seed> <skill>: a game played by mouse; its state log is <name>/state.txt
	local d="$OUT/alchemy/$1"; rm -rf "$d"; mkdir -p "$d"
	timeout 600 "$2" -userdir="$d/user" -fast -seed="$5" -autoplayui -autoplaystart=450 -autoplay="$4" -autoplaymode="$3" -autoplayskill="$6" -autoplaycap=20000 		-statelog="$d/state.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
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
	# the frozen baseline (build/baseline-alchemy/, the port when its identical-play pass was done): the same mouse-played
	# games must give the same state at every update (the values the pass compared with the original)
	local base="$W/build/baseline-alchemy/alchemy.exe" bl="" a
	if [ -f "$base" ]; then
		for a in "strategic 7 1 -1" "timetrial 1 1 -1" "strategic 3 4 2"; do
			read -r m bs gs sk <<< "$a"
			alchemy_state_game "cur-$m-$gs" "$W/build/native/alchemy.exe" "$m" "$bs" "$gs" "$sk"
			alchemy_state_game "base-$m-$gs" "$base" "$m" "$bs" "$gs" "$sk"
			[ -s "$OUT/alchemy/cur-$m-$gs/state.txt" ] && [ -s "$OUT/alchemy/base-$m-$gs/state.txt" ] &&
				cmp -s "$OUT/alchemy/cur-$m-$gs/state.txt" "$OUT/alchemy/base-$m-$gs/state.txt" || bl="x"
		done
		[ -z "$bl" ] && bl="; baseline identical" || { bl="; BASELINE DIFFERS"; st=1; }
	fi
	SUMMARY="strategic ${s#game 1 over at update }; time trial ${t#game 1 over at update }; ${u}; saved game loaded $v time; $r$bl"
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

pixelus_state_game() {  # <name> <exe> <bot seed> <game seed>: three puzzles played by mouse; its state log is <name>/state.txt
	local d="$OUT/pixelus/$1"; rm -rf "$d"; mkdir -p "$d"
	timeout 600 "$2" -userdir="$W/$d/user" -fast -seed="$4" -autoplay="$3" -autoplayui -autoplayskip -autoplaypuzzles=3 -autoplaycap=200000 \n		-autoplaylog="$d/log.txt" -statelog="$d/state.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
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
	if [ "${CHECK_QUICK:-0}" = 1 ]; then  # the quick form leaves out the bot's whole game (87 of the 127 seconds)
		f="skipped (quick form)"
	else
		f="$(pixelus_game full 400)"
		n="$(grep -c '^completed' "$d/full/log.txt" 2>/dev/null)"; h="$(sha1sum "$d/full/log.txt" 2>/dev/null | cut -c1-8)"
		f="$f, $n puzzles, log $h"
	fi
	{ echo "$g"; echo "$t"; echo "start: $a"; echo "start rerun: $r"; echo "full: $f"; } > "$1"
	[ "$g" = "$PX_GRID" ] || st=1
	[ "$a" = "$PX_START" ] || st=1
	[ "$f" = "$PX_FULL" ] || [ "${CHECK_QUICK:-0}" = 1 ] || st=1
	if diff -rq "$d/start/shots" "$d/start2/shots" >> "$1" 2>&1 && diff -q "$d/start/log.txt" "$d/start2/log.txt" >> "$1" 2>&1; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	# the frozen baseline (build/baseline-pixelus/, the port when the first phase of its identical-play pass was done): the
	# same mouse-played games must give the same state at every update (the values the pass compared with the original)
	local base="$W/build/baseline-pixelus/pixelus.exe" bl="" k bs gs
	if [ -f "$base" ]; then
		for k in "1 1" "2 7"; do
			read -r bs gs <<< "$k"
			pixelus_state_game "cur-$gs" "$W/build/native/pixelus.exe" "$bs" "$gs"
			pixelus_state_game "base-$gs" "$base" "$bs" "$gs"
			[ -s "$d/cur-$gs/state.txt" ] && [ -s "$d/base-$gs/state.txt" ] && cmp -s "$d/cur-$gs/state.txt" "$d/base-$gs/state.txt" || bl="x"
		done
		[ -z "$bl" ] && r="$r; baseline identical" || { r="$r; BASELINE DIFFERS"; st=1; }
	fi
	SUMMARY="${g#gridtest: }; $t; start ${a#done: }; $r; full ${f#done: }"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected grid '$PX_GRID', start '$PX_START', full '$PX_FULL')"
	return $st
}

mummymaze_game() {  # <name> <bot arguments>...: a bot game; prints the last result line of its log
	local n="$1" d
	d="$OUT/mummymaze/$n"; shift
	rm -rf "$d"; mkdir -p "$d/shots"
	timeout 600 "${MM_EXE:-$W/build/native/mummymaze.exe}" -userdir="$d/user" -fast "$@" -autoplaylog="$d/log.txt" -autoplayshots="$d/shots/s" 		> "$d/stdout.txt" 2> "$d/stderr.txt"
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
	# the frozen baseline (build/baseline-mummymaze/mummymaze.exe, a copy of the port made when it was last checked against
	# the original, 2026-11-03): the same games must give the same log lines and the same screenshots. Refreeze it only on purpose,
	# with the reason in games/mummymaze/docs/IDENTICAL_PLAY.md.
	local base="$W/build/baseline-mummymaze/mummymaze.exe" bl="" k
	if [ -f "$base" ]; then
		MM_EXE="$base" mummymaze_game base-classic -seed=1 -autoplay=7 -autoplaygames=1 -autoplaylevels=8 -autoplaycap=60000 > /dev/null
		MM_EXE="$base" mummymaze_game base-tips -seed=3 -autoplay=5 -autoplaygames=1 -autoplaystart=3,0 -autoplaylevels=4 -autoplaycap=60000 > /dev/null
		MM_EXE="$base" mummymaze_game base-tutorial -seed=1 -autoplay=3 -autoplaymode=tutorial -autoplaycap=100000 > /dev/null
		bl="baseline identical"
		for k in classic tips tutorial; do
			cmp -s "$OUT/mummymaze/$k/log.txt" "$OUT/mummymaze/base-$k/log.txt" && diff -rq "$OUT/mummymaze/$k/shots" "$OUT/mummymaze/base-$k/shots" >> "$1" 2>&1 || { bl="BASELINE DIFFERS ($k)"; st=1; }
		done
		r="$r; $bl"
	fi
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
	if [ "${CHECK_QUICK:-0}" = 1 ]; then  # the quick form loads a sample of the animations: the whole set takes 80 to 90 of the 129 seconds
		p="$(bookwormadv_pam_sample "$log")"
	else
		p="$("$W/build/native/bwa_pamtest.exe" 2>&1 | tee -a "$log" | tail -n 1 | tr -d '\r')"
	fi
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

bookwormadv_pam_sample() {  # <log>: bwa_pamtest's load of every animation, on every 45th of the sorted .pam files (-info loads one with its pictures); prints "loaded N of M ..."
	local f n=0 ok=0 list
	list="$(cd assets/bookwormadv && shopt -s globstar nullglob nocaseglob && printf '%s\n' images/**/*.pam | sed -n '1~45p' | tr '/' '\\')"
	while IFS= read -r f; do
		[ -n "$f" ] || continue
		n=$((n + 1))
		"$W/build/native/bwa_pamtest.exe" -info "$f" >> "$1" 2>&1 && ok=$((ok + 1))
	done <<< "$list"
	echo "loaded $ok of $n (a sample of the whole set)"
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

tiptop_state_game() {  # <name> <exe> <mode> <bot seed> <game seed> <games> [port arguments]: games played by mouse; the state log is <name>/state.txt
	local d="$OUT/tiptop/$1" exe="$2" m="$3" bs="$4" gs="$5" n="$6"; shift 6
	rm -rf "$d"; mkdir -p "$d"
	timeout 600 "$exe" -gamedir=assets/tiptop -userdir="$d/user" -fast -seed="$gs" -autoplay="$bs" -autoplayui -autoplaymode="$m" \
		-autoplaygames="$n" -autoplaycap=80000 -statelog="$d/state.txt" "$@" > "$d/stdout.txt" 2> "$d/stderr.txt"
}

suite_tiptop() {
	local st=0 g s p r again ref
	need_assets tiptop || return 1
	g="$(tiptop_game regular regular 2)"; s="$(tiptop_game strategy strategy 1)"
	if [ "${CHECK_QUICK:-0}" = 1 ]; then  # the quick form leaves out the speed game (65 s) and reruns the short strategy game, not the regular ones (42 s)
		p="skipped (quick form)"; again=strategy; ref="$s"
		r="$(tiptop_game strategy2 strategy 1)"
	else
		p="$(tiptop_game speed speed 1)"; again=regular; ref="$g"
		r="$(tiptop_game regular2 regular 2)"
	fi
	{ echo "regular: $g"; echo "strategy: $s"; echo "speed: $p"; echo "$again rerun: $r"; } > "$1"
	[ "$g" = "$TT_REGULAR" ] || st=1
	[ "$s" = "$TT_STRATEGY" ] || st=1
	[ "$p" = "$TT_SPEED" ] || [ "${CHECK_QUICK:-0}" = 1 ] || st=1
	if diff -rq "$OUT/tiptop/$again/shots" "$OUT/tiptop/${again}2/shots" >> "$1" 2>&1 && [ "$ref" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	# the frozen baseline (build/baseline-tiptop/, the port when phase one of its identical-play pass was done): the same
	# mouse-played games must give the same state at every update (the values the pass compared with the original), the
	# meter left out (outside the timed mode it holds uninitialised heap, which differs from run to run), and so are the
	# level and blocks left at update 1, read before the first game sets them
	local base="$W/build/baseline-tiptop/tiptop.exe" bl="" a
	if [ -f "$base" ] && [ "${CHECK_QUICK:-0}" != 1 ]; then
		for a in "reg regular 1 1 2" "str strategy 2 3 1" "save regular 2 2 1 -autoplaysave=30"; do
			read -r k m bs gs n x <<< "$a"
			tiptop_state_game "cur-$k" "$W/build/native/tiptop.exe" "$m" "$bs" "$gs" "$n" $x
			tiptop_state_game "base-$k" "$base" "$m" "$bs" "$gs" "$n" $x
			[ -s "$OUT/tiptop/cur-$k/state.txt" ] && [ -s "$OUT/tiptop/base-$k/state.txt" ] &&
				cmp -s <(grep -Ev ' meter |^1 (level|blocksleft) ' "$OUT/tiptop/cur-$k/state.txt") <(grep -Ev ' meter |^1 (level|blocksleft) ' "$OUT/tiptop/base-$k/state.txt") ||
				bl="x"
		done
		[ -z "$bl" ] && bl="; baseline identical" || { bl="; BASELINE DIFFERS"; st=1; }
	fi
	SUMMARY="regular ${g#game }; strategy ${s#game }; speed ${p#game }; $r$bl"
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

ningpo_state_game() {  # <name> <exe> <case> <seed>: an identical-play case's script; its state log is <name>/state.txt
	local d="$OUT/ningpo/$1"; rm -rf "$d"; mkdir -p "$d"
	OUT="$OUT/ningpo" "$W/games/ningpo/tools/identical/cases.sh" "$3" "$d/script.txt"
	timeout 300 "$2" -gamedir=assets/ningpo -userdir="$d/user" -fast -seed="$4" -script="$d/script.txt" \
		-statelog="$d/state.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
}

suite_ningpo() {
	local st=0 g r base="$W/build/baseline-ningpo/ningpo.exe" bl="" a
	need_assets ningpo || return 1
	g="$(ningpo_game seed7)"; r="$(ningpo_game seed7b)"
	{ echo "game: $g"; echo "rerun: $r"; } > "$1"
	[ "$g" = "$NP_GAME" ] || st=1
	if diff -rq "$OUT/ningpo/seed7/shots" "$OUT/ningpo/seed7b/shots" >> "$1" 2>&1 && [ "$g" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	# the frozen port (games/ningpo/docs/IDENTICAL_PLAY.md) on the identical-play pass's cases: every layout, and two
	# games in a row with the best times, each with the same non-empty state log as the build's
	if [ -f "$base" ]; then
		for a in layouts:11 won39:39; do
			ningpo_state_game "cur-${a%:*}" "$W/build/native/ningpo.exe" "${a%:*}" "${a#*:}"
			ningpo_state_game "base-${a%:*}" "$base" "${a%:*}" "${a#*:}"
			[ -s "$OUT/ningpo/cur-${a%:*}/state.txt" ] && [ -s "$OUT/ningpo/base-${a%:*}/state.txt" ] &&
				cmp -s "$OUT/ningpo/cur-${a%:*}/state.txt" "$OUT/ningpo/base-${a%:*}/state.txt" || bl="x"
		done
		[ -z "$bl" ] && bl="; baseline identical" || { bl="; BASELINE DIFFERS"; st=1; }
	fi
	SUMMARY="$g; $r$bl"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected '$NP_GAME')"
	return $st
}

wordharmony_game() {  # <name> <think|tsunami> [exe]: a scripted game (-fast, seed 1) that ends saved; prints the saved game;
	# its state log (the values the identical-play pass compared with the original) is <name>/state.txt
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
	timeout 300 "${3:-$W/build/native/wordharmony.exe}" -gamedir=assets/wordharmony -userdir="$d/user" -fast -seed=1 \
		-statelog="$d/state.txt" -script="$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
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
	# the frozen baseline (build/baseline-wordharmony/, the port when its identical-play pass was done): the same games must
	# write the same state at every update (games/wordharmony/docs/IDENTICAL_PLAY.md)
	local base="$W/build/baseline-wordharmony/wordharmony.exe" bl="" g
	if [ -f "$base" ]; then
		for g in think tsunami; do
			wordharmony_game "base-$g" "$g" "$base" > /dev/null
			[ -s "$OUT/wordharmony/$g/state.txt" ] && [ -s "$OUT/wordharmony/base-$g/state.txt" ] &&
				cmp -s "$OUT/wordharmony/$g/state.txt" "$OUT/wordharmony/base-$g/state.txt" || bl="x"
		done
		[ -z "$bl" ] && bl="; baseline identical" || { bl="; BASELINE DIFFERS"; st=1; }
	fi
	SUMMARY="THINK $t; TSUNAMI $a; $r$bl"
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

rocketmania_state_game() {  # <name> <exe> <mode> <difficulty> <bot seed> <game seed>: a game played by mouse (the bot of testhooks/UiBot.cpp); its state log is <name>/state.txt
	local d="$OUT/rocketmania/$1"; rm -rf "$d"; mkdir -p "$d"
	timeout 600 "$2" -userdir="$d/user" -fast -drawevery -seed="$6" -uibot="$5" -uistart=300000 -uimodes="$3" -uidiffs="$4" -uigames=1 -uicap=3000 		-statelog="$d/state.txt" -uiscript="$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
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
	# the frozen baseline (build/baseline-rocketmania/, the port when its identical-play pass was done): the same mouse-played
	# games must give the same state at every update (the values the pass compared with the original)
	local base="$W/build/baseline-rocketmania/rocketmania.exe" bl="" g
	if [ -f "$base" ]; then
		for g in "0 2 2 777" "1 2 1 12345" "2 3 3 4242"; do
			read -r m df bs gs <<< "$g"
			rocketmania_state_game "cur-$m" "$W/build/native/rocketmania.exe" "$m" "$df" "$bs" "$gs"
			rocketmania_state_game "base-$m" "$base" "$m" "$df" "$bs" "$gs"
			[ -s "$OUT/rocketmania/cur-$m/state.txt" ] && [ -s "$OUT/rocketmania/base-$m/state.txt" ] &&
				cmp -s "$OUT/rocketmania/cur-$m/state.txt" "$OUT/rocketmania/base-$m/state.txt" || bl="x"
		done
		[ -z "$bl" ] && bl="; baseline identical" || { bl="; BASELINE DIFFERS"; st=1; }
	fi
	SUMMARY="loaded, menu, hover, difficulty dialog, and a game of each mode played (scores$scores)$bl"
	return $st
}

cosmicbugs_plays() {  # <exe> <tag>: the series of games checked against the original (games/cosmicbugs/test/plays), each
	# played by the runner with origrun's watches; prints the checksum of all their watch lines, and EMPTY with the games
	# that printed none (an exe that fails at start-up would otherwise give the checksum of nothing on both sides)
	local exe="$1" d="$OUT/cosmicbugs/plays-$2" f n empty=""
	rm -rf "$d"; mkdir -p "$d"
	for f in games/cosmicbugs/test/plays/*.txt; do
		n="$(basename "$f" .txt)"; case "$n" in *.orig) continue ;; esac
		mkdir -p "$d/$n"
		CB_NOWARP=1 CB_WATCH=1 timeout 300 "$exe" assets/cosmicbugs "$d/$n" "$f" 2>&1 | tr -d '\r' | grep '^watch ' > "$d/$n.txt"
		[ -s "$d/$n.txt" ] || empty="$empty $n"
	done
	ls "$d"/*.txt > /dev/null 2>&1 || empty=" (no games)"
	echo "$(cat "$d"/*.txt 2>/dev/null | md5sum | cut -c1-12), $(cat "$d"/*.txt 2>/dev/null | wc -l) watch lines${empty:+; EMPTY:$empty}"
}

suite_cosmicbugs() {
	local st=0 b g r w w2 p pb c="$OUT/cosmicbugs" ea=0 ef=0
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
	# the series of games that play out as the original's (games/cosmicbugs/docs/IDENTICAL_PLAY.md), against the frozen
	# baseline (build/baseline-cosmicbugs/, the runner as it was when they were last checked against the original)
	p="$(cosmicbugs_plays build/native/cosmicbugs_run.exe now)"; pb=""
	[ -f build/baseline-cosmicbugs/cosmicbugs_run.exe ] && pb="$(cosmicbugs_plays build/baseline-cosmicbugs/cosmicbugs_run.exe base)"
	{ echo "banks: $b"; echo "game: $g"; echo "rerun: $r"; echo "window: $w"; echo "window rerun: $w2"; echo "plays: $p"; echo "baseline plays: $pb"; } > "$1"
	[ "$b" = "$CB_BANKS" ] || st=1
	[ "$g" = "$CB_GAME" ] || st=1
	if diff -q "$c/game/line1.png" "$c/game2/line1.png" >> "$1" 2>&1 && diff -q "$c/game/line2.png" "$c/game2/line2.png" >> "$1" 2>&1 &&
		[ "$g" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	if [ "$w" = "$(echo "${g%%;*}" | sed 's/^[0-9]* ms: //')" ] && diff -q "$c/window/line1.png" "$c/window2/line1.png" >> "$1" 2>&1 &&
		[ "$w" = "$w2" ]; then w="window the same, twice"; else w="WINDOW DIFFERS ('$w')"; st=1; fi
	if [[ "$p$pb" == *EMPTY* ]]; then p="PLAYS EMPTY (now: $p; baseline: $pb)"; st=1
	elif [ -z "$pb" ]; then p="plays $p, no baseline"; elif [ "$p" = "$pb" ]; then p="plays as the baseline"; else p="PLAYS DIFFER FROM THE BASELINE"; st=1; fi
	SUMMARY="$b; $g; $r; $w; $p"
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
	python -I -c "import hashlib,sys;from PIL import Image;i=Image.open(sys.argv[1]).convert('RGB');i.paste((0,0,0),(560,240,690,520)) if 'scores' in sys.argv[1] else i.paste((0,0,0),(405,220,425,235)) if 'beam' in sys.argv[1] else 0;print(hashlib.sha256(i.tobytes()).hexdigest()[:12])" "$1" 2>/dev/null | tr -d '\r'
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
	if [ "$(wb_hash "$c/game/start.png")" = "$(wb_hash "$c/game2/start.png")" ] && [ "$(wb_hash "$c/game/beam.png")" = "$(wb_hash "$c/game2/beam.png")" ] &&
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

dynomite_state() {  # <name> <exe> <seed> <mkscript rng seed> <parts...>: an identical-play case's script (mkscript.py)
	# played with -statelog, as the pass played it against the original; the log is <name>/state.txt
	local d="$OUT/dynomite/$1" e="$2" seed="$3" r="$4" u
	shift 4
	rm -rf "$d"; dynomite_user "$d/user"
	python games/dynomite/tools/identical/mkhs.py "$d/user"
	python games/dynomite/tools/identical/mkscript.py "$d/script.txt" 25000 "$r" "$@"
	u="$(cygpath -ma "$d")"
	timeout 300 "$e" -gamedir=assets/dynomite -userdir="$u/user" -fast -drawevery -seed="$seed" -script="$u/script.txt" 		-statelog="$u/state.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
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
	# the frozen baseline (build/baseline-dynomite/, the port when its identical-play pass was done): three of the pass's
	# cases (games/dynomite/tools/identical/cases.sh) must give the same state at every update
	local base="$W/build/baseline-dynomite/dynomite.exe" bl="" a n
	if [ -f "$base" ]; then
		for a in "en 2 3 title menu:endless shots:80:40 wait:300 text:Zed click:352:369 wait:200 click:400:400 wait:300 shots:80:40 quit" 			"st 4 5 title menu:stomped:pack shots:45:40 wait:300 text:Sam click:352:369 wait:200 quit" 			"fp 8 9 title menu:fossil:panic shots:60:40 wait:300 quit"; do
			set -- $a; n=$1; shift
			dynomite_state "cur-$n" "$W/build/native/dynomite.exe" "$@"
			dynomite_state "base-$n" "$base" "$@"
			[ -s "$c/cur-$n/state.txt" ] && [ -s "$c/base-$n/state.txt" ] &&
				cmp -s "$c/cur-$n/state.txt" "$c/base-$n/state.txt" || bl="x"
		done
		[ -z "$bl" ] && bl="; baseline identical" || { bl="; BASELINE DIFFERS"; st=1; }
	fi
	SUMMARY="$g; $r; $l$bl"
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

mahjongchina_run() {  # <name>: a new player, Dynasty Adventure stage 1 won with the solver's clicks; prints the moves and the shots
	local d="$OUT/mahjongchina/$1" t=3400 x1 y1 x2 y2 n=0
	rm -rf "$d"; mkdir -p "$d/shots" "$d/user"
	# Dynasty Adventure, the name "Test", the map's PLAY, the How To Play tip's OK; the solver's pairs; the Magic
	# Gold Tiles tip's OK (its box is at the left), the level's end and its OK, the map. No click reaches a link.
	{
		printf '%s\n' "1400 click 400 262" "1720 char T" "1725 char e" "1730 char s" "1735 char t" "1760 key RETURN" \
			"2600 click 725 565" "3000 click 400 407" "3300 screenshot $d/shots/board.png"
		while read -r x1 y1 x2 y2; do
			echo "$t click $x1 $y1"; echo "$((t + 60)) click $x2 $y2"; t=$((t + 180)); n=$((n + 1))
		done < "$OUT/mahjongchina/sol1.txt"
		printf '%s\n' "$((t + 400)) click 187 330" "$((t + 2400)) screenshot $d/shots/won.png" "$((t + 2600)) click 397 400" \
			"$((t + 5600)) screenshot $d/shots/map.png" "$((t + 5700)) quit"
	} > "$d/script.txt"
	timeout 300 "$W/build/native/mahjongchina.exe" -gamedir=assets/mahjongchina -userdir="$d/user" -fast -script="$d/script.txt" \
		> "$d/stdout.txt" 2> "$d/stderr.txt"
	echo "$n pairs; $(ls "$d/shots" | wc -l) shots; player files: $(ls "$d/user" | grep -c '\.usr$')"
}

suite_mahjongchina() {
	local st=0 dl g r sh c="$OUT/mahjongchina"
	need_assets mahjongchina || return 1
	mkdir -p "$c"
	dl="$(timeout 120 build/native/mahjongchina_dealtest.exe assets/mahjongchina 2>&1 | tail -n 1 | tr -d '\r' | sed 's/, slowest.*//')"
	[ "$dl" = "$MJC_DEAL" ] || st=1
	timeout 120 build/native/mahjongchina_dealtest.exe assets/mahjongchina --stage 1 > "$c/sol1.txt" 2> "$c/solve.txt" || st=1
	g="$(mahjongchina_run game)"; r="$(mahjongchina_run game2)"
	{ echo "deal: $dl"; echo "game: $g"; echo "rerun: $r"; } > "$1"
	sh="$(for f in board won map; do sha256sum "$c/game/shots/$f.png" 2>/dev/null | cut -c1-12; done | paste -sd" " -)"
	if [ "$sh" = "$MJC_SHOTS" ]; then sh="screens as frozen"; else sh="SCREENS CHANGED ($sh)"; st=1; fi
	if diff -rq "$c/game/shots" "$c/game2/shots" >> "$1" 2>&1 && [ "$g" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	SUMMARY="$dl; $g; $r; $sh"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected '$MJC_DEAL')"
	return $st
}

twist_game() {  # <name> <exe> <autoplay mode> <last update> <updates...>: a seeded 2D autoplay game (-nod3d9: 3D is the default); prints its screens' hashes
	local d="$OUT/bejeweledtwist/$1" exe="$2" mode="$3" last="$4" n
	shift 4
	rm -rf "$d"; mkdir -p "$d/shots" "$d/user"
	printf '[SOFTWARE\\Steam\\BejeweledTwist]\nScreenMode=dword:0\nArtRes=dword:768\n' > "$d/user/registry.ini"
	{ for n in "$@"; do echo "$n screenshot $d/shots/$n.png"; done; echo "$last quit"; } > "$d/script.txt"
	# autoplay clicks only the board and the mode select, never a link
	timeout 300 "$exe" -gamedir=assets/bejeweledtwist -userdir="$d/user" -nod3d9 -fast -seed=1234 -autoplay="$mode" \
		-script="$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
	for n in "$@"; do sha256sum "$d/shots/$n.png" 2>/dev/null | cut -c1-12; done | paste -sd" " -
}

suite_bejeweledtwist() {
	local st=0 c b r sh bl="" exe="$W/build/native/bejeweledtwist.exe" base="$W/build/baseline-bejeweledtwist/bejeweledtwist.exe"
	need_assets bejeweledtwist || return 1
	c="$(twist_game classic "$exe" 1 6010 1500 3000 4500 6000)"
	b="$(twist_game blitz "$exe" blitz 9010 3500 6000 8000 8450 9000)"
	r="$(twist_game classic2 "$exe" 1 6010 1500 3000 4500 6000)"
	{ echo "classic: $c"; echo "blitz: $b"; echo "classic rerun: $r"; } > "$1"
	if [ "$c" = "$BT_CLASSIC" ] && [ "$b" = "$BT_BLITZ" ]; then sh="screens as frozen"; else sh="SCREENS CHANGED"; st=1; fi
	if [ -n "$c" ] && [ "$c" = "$r" ]; then r="rerun identical"; else r="RERUN DIFFERS"; st=1; fi
	# the frozen baseline (build/baseline-bejeweledtwist/, a copy of the port with its DLLs made when it was last
	# checked against the original): the same games must give the same screens
	if [ -f "$base" ]; then
		if [ "$(twist_game base-classic "$base" 1 6010 1500 3000 4500 6000)" = "$c" ] &&
			[ "$(twist_game base-blitz "$base" blitz 9010 3500 6000 8000 8450 9000)" = "$b" ]; then
			bl="; baseline identical"
		else
			bl="; BASELINE DIFFERS"; st=1
		fi
	fi
	SUMMARY="Classic to 6000 and Blitz to its high scores, 9 screens; $r; $sh$bl"
	[ $st -eq 0 ] || SUMMARY="$SUMMARY (expected classic '$BT_CLASSIC', blitz '$BT_BLITZ')"
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

run_suite() {  # <suite>[:<quick or targets>]: runs it and prints its summary line whole; the status is the suite's
	local s="$1" name arg quick log start
	name="${s%%:*}"; arg=""
	[[ "$s" == *:* ]] && arg="${s#*:}"
	quick="$QUICK_ENV"
	if [ "$arg" = quick ]; then quick=1; arg=""; fi
	export CHECK_QUICK="$quick"
	log="$OUT/$name.txt"
	start=$SECONDS
	SUMMARY=""
	if "suite_$name" "$log" "$arg"; then
		echo "PASS $name ($((SECONDS - start))s): $SUMMARY"
		return 0
	fi
	echo "FAIL $name ($((SECONDS - start))s): $SUMMARY -> $log"
	return 1
}

PASSED=0
FAILED=0
count() { if [ "$1" -eq 0 ]; then PASSED=$((PASSED + 1)); else FAILED=$((FAILED + 1)); fi; }

# run_batch <suite>...: game suites, up to $JOBS game processes at a time (a suite counts as the processes it starts
# at once), the longest first; each prints its line when it ends
run_batch() {
	local queue=() used=0 s t pid st
	local -A weight=()
	for t in tiptop bookwormadv pixelus bookwormadv2 bonnies mummymaze noahsark; do
		for s in "$@"; do [ "${s%%:*}" = "$t" ] && queue+=("$s"); done
	done
	for s in "$@"; do
		case " tiptop bookwormadv pixelus bookwormadv2 bonnies mummymaze noahsark " in *" ${s%%:*} "*) ;; *) queue+=("$s") ;; esac
	done
	trap 'kill $(jobs -p) 2>/dev/null; exit 130' INT TERM
	while [ ${#queue[@]} -gt 0 ] || [ $used -gt 0 ]; do
		while [ ${#queue[@]} -gt 0 ]; do
			s="${queue[0]}"; suite_weight "${s%%:*}"
			if [ $used -gt 0 ] && [ $((used + WEIGHT)) -gt "$JOBS" ]; then break; fi
			queue=("${queue[@]:1}")
			run_suite "$s" &
			weight[$!]=$WEIGHT; used=$((used + WEIGHT))
		done
		wait -n -p pid; st=$?
		used=$((used - ${weight[$pid]:-1})); unset 'weight[$pid]'
		count $st
	done
	trap - INT TERM
}

BATCH=()
for s in "${SUITES[@]}"; do
	if [ "$JOBS" -gt 1 ] && is_game_suite "${s%%:*}"; then BATCH+=("$s"); continue; fi
	if [ ${#BATCH[@]} -gt 0 ]; then run_batch "${BATCH[@]}"; BATCH=(); fi
	run_suite "$s"; count $?
done
if [ ${#BATCH[@]} -gt 0 ]; then run_batch "${BATCH[@]}"; fi
echo "check: $PASSED passed, $FAILED failed"
[ $FAILED -eq 0 ]
