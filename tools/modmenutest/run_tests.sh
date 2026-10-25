#!/usr/bin/env bash
# Mod menu tests (docs/MOD_MENU_DESIGN.md 7.3): scripted runs of a game with the mod menu, each in a fresh user dir
# and a fresh mods dir holding the fixture mods (tools/fixtures/mods/settings-demo, tools/modmenutest/mods/
# plain-demo). Every game exe runs under build/native/rundesk.exe (this script re-runs itself there): invisible,
# silent, below-normal priority. One game process at a time.
#   tools/modmenutest/run_tests.sh [--gamedir GAME=PATH]... [bejeweled|bigmoney|all]
#       (default all; needs tools/build.sh native)
#   --gamedir GAME=PATH   (repeatable; also --gamedir=GAME=PATH) the staged game files of GAME (bejeweled or
#                         bigmoney); default $ROOT/assets/<game>. A relative PATH is taken from the current directory.
#                         A game's worktree points it at its own staged files or at another checkout's, e.g. from a
#                         worktree's root: --gamedir bejeweled=../../../assets/bejeweled
# Cases (scripts/), for each game:
#   edit     the "modmenu" edit commands: the exact bytes of mods.json and mod-settings.json (unknown keys, a mod
#            that is not installed and its values kept), the values as normalised, the refusals logged
#   noop     edits that change nothing write nothing: hand-written files keep their bytes
#   strings  a mod's texts with % (no settings): kept, except for a key the game uses as a printf format
#   keys     the keyboard alone: rows, every type of value, Shift and Alt, text editing, Delete, Tab, Esc
#   clicks   the mouse alone: switches, order, values, a slider drag, a list drag, a confirmation, Close
#   restart  "modmenu restart" in a test run: the game's shutdown save and the arguments a restart keeps; then a
#            real restart (BYOPOP_RELAUNCH_TEST_ARGS) that rundesk waits for: the second process applies the value
#   screens  screenshots for a person to look at (the menu over the title, a game and Options; empty; -nomods)
# Bejeweled also:
#   nextgame a value held for the next fresh game: it waits during a game and while a save waits for "Continue?",
#            and "No" makes the untouched board again with it (the score key it announced); Options shows MODS
#   pause    the menu pauses and resumes the board exactly as Options does: the same -tracehash (board, both
#            RNGs) whether it closes by command or by Esc after keyboard use
# And fwtest -hotkeytest (F2 through HandleEvent), and check_no_rand.py (no random draw in the menu's sources).
# fwtest runs on Bejeweled's files (--gamedir bejeweled=PATH): when they are missing and only one other game is
# selected ("run_tests.sh bigmoney"), the fwtest step is skipped with a note and does not count as failed; with
# "all" or "bejeweled" it stays a failure.
# Output: build/modmenutest/<game>/<case>/. Every case also checks that nothing was written into its mods dir.
# Exit status: the number of failed checks.
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$ROOT/tools/modmenutest"
OUT="$ROOT/build/modmenutest"
ALL_GAMES="bejeweled bigmoney"
ARGS=("$@")

usage() { echo "usage: $0 [--gamedir GAME=PATH]... [bejeweled|bigmoney|all]" >&2; exit 2; }
declare -A GAMEDIR=()
# add_gamedir GAME=PATH: PATH made absolute when it exists (the runs start in their case dirs)
add_gamedir() {
	local game="${1%%=*}" dir="${1#*=}"
	[ "$game" != "$1" ] && [ -n "$game" ] && [ -n "$dir" ] || { echo "run_tests.sh: --gamedir wants GAME=PATH, not \"$1\"" >&2; usage; }
	case " $ALL_GAMES " in *" $game "*) ;; *) echo "run_tests.sh: --gamedir: unknown game \"$game\" (one of: $ALL_GAMES)" >&2; usage ;; esac
	[ -d "$dir" ] && dir="$(cd "$dir" && pwd)"
	GAMEDIR[$game]="$dir"
}
SELECTION=""
while [ $# -gt 0 ]; do
	case "$1" in
		--gamedir) [ $# -ge 2 ] || usage; add_gamedir "$2"; shift 2 ;;
		--gamedir=*) add_gamedir "${1#--gamedir=}"; shift ;;
		-*) echo "run_tests.sh: unknown option $1" >&2; usage ;;
		*) [ -z "$SELECTION" ] || usage; SELECTION="$1"; shift ;;
	esac
done
SELECTION="${SELECTION:-all}"
case " all $ALL_GAMES " in *" $SELECTION "*) ;; *) echo "run_tests.sh: unknown game \"$SELECTION\"" >&2; usage ;; esac
GAMES="$SELECTION"
[ "$GAMES" = "all" ] && GAMES="$ALL_GAMES"
# gamedir GAME: the folder with GAME's staged files
gamedir() { echo "${GAMEDIR[$1]:-$ROOT/assets/$1}"; }

RUNDESK="$ROOT/build/native/rundesk.exe"
if [ -z "${BYOPOP_TEST_RUN:-}" ]; then
	[ -x "$RUNDESK" ] || { echo "run_tests.sh: $RUNDESK is missing (tools/build.sh native)" >&2; exit 2; }
	exec "$RUNDESK" bash "$0" "${ARGS[@]}"
fi

winpath() { if command -v cygpath > /dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi; }
PY="${PYTHON:-python}"
FAILED=0
pass() { echo "  ok: $1"; }
fail() { echo "  FAILED: $1"; FAILED=$((FAILED + 1)); }
check() { if eval "$2"; then pass "$1"; else fail "$1"; fi; }
tree_hash() { (cd "$1" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 -r sha256sum | sha256sum | cut -c1-16); }
# The value of setting $3 of mod $2 in the state JSON $1 (a "modmenu dump"), and its applied value
state_value() { "$PY" -c "import json,sys; s=json.load(open(sys.argv[1])); print(json.dumps([x.get(sys.argv[4]) for m in s['mods'] if m['id']==sys.argv[2] for x in m['settings'] if x.get('id')==sys.argv[3]][0]))" "$(winpath "$1")" "$2" "$3" "${4:-value}"; }

# run <game> <case dir> <script> [prepared user dir|-] [game args...]: a fresh user and mods dir in the case dir,
# the game on the script; its log in log.txt
run() {
	local game=$1 dir=$2 script=$3 prep=${4:--}
	shift 4
	rm -rf "$dir"
	mkdir -p "$dir/mods" "$dir/user" "$dir/shots"
	cp -r "$ROOT/tools/fixtures/mods/settings-demo" "$HERE/mods/plain-demo" "$dir/mods/"
	[ "$prep" != "-" ] && cp "$prep"/* "$dir/user/"
	MODS_HASH=$(tree_hash "$dir/mods")
	(cd "$dir" && timeout 300 "$ROOT/build/native/$game.exe" -gamedir="$(winpath "$(gamedir "$game")")" -userdir=user \
		-modsdir=mods -seed=1 -fast -script="$(winpath "$script")" "$@" > stdout.txt 2> log.txt; echo $? > exit.txt)
	check "$(basename "$dir"): the run ended (exit $(cat "$dir/exit.txt"))" '[ "$(cat "$dir/exit.txt")" = 0 ]'
	check "$(basename "$dir"): nothing written into the mods dir" '[ "$(tree_hash "$dir/mods")" = "$MODS_HASH" ]'
}

logged() { grep -q -F -- "$2" "$1/log.txt"; }
# state_json <state JSON> <expression of s>: the expression's value as JSON
state_json() { "$PY" -c "import json,sys; s=json.load(open(sys.argv[1])); print(json.dumps(eval(sys.argv[2])))" "$(winpath "$1")" "$2"; }
# forgame <dir> <file>: the game's own copy (<dir>/<game>-<file>) when there is one, else the shared one. Bejeweled
# has its own keys and clicks results (its rule values wait for the next game) and clicks script (its message line
# on the mod's page)
forgame() { if [ -f "$1/$GAME-$2" ]; then echo "$1/$GAME-$2"; else echo "$1/$2"; fi; }

for GAME in $GAMES; do
	[ -f "$ROOT/build/native/$GAME.exe" ] || { echo "$GAME: build/native/$GAME.exe is missing"; FAILED=$((FAILED + 1)); continue; }
	[ -d "$(gamedir "$GAME")" ] || { echo "$GAME: $(gamedir "$GAME") is not staged (give --gamedir $GAME=PATH)"; FAILED=$((FAILED + 1)); continue; }
	echo "$GAME:"
	G="$OUT/$GAME"
	S="$HERE/scripts"
	E="$HERE/expect"

	# edit
	D="$G/edit"
	run "$GAME" "$D" "$S/edit.txt" "$HERE/user"
	check "edit: mods.json" 'cmp -s "$D/user/mods.json" "$E/edit-mods.json"'
	check "edit: mod-settings.json" 'cmp -s "$D/user/mod-settings.json" "$E/edit-mod-settings.json"'
	check "edit: values as normalised (clamped, snapped, charset, lower-case colour)" \
		'[ "$(state_value "$D/edit-state1.json" settings-demo minRun) $(state_value "$D/edit-state1.json" settings-demo speed) $(state_value "$D/edit-state1.json" settings-demo praise) $(state_value "$D/edit-state1.json" settings-demo tint)" = "5 1.3 \"50? OFF\" \"#00ff80\"" ]'
	check "edit: a move that cannot be made is refused" 'logged "$D" "line 8: modmenu move: \"plain-demo\" cannot move there"'
	check "edit: a wrong value is refused" 'logged "$D" "line 20: modmenu set: Undo: \"yes\" is not true or false"'
	check "edit: an unknown setting is refused" 'logged "$D" "line 21: modmenu set: mod \"settings-demo\" has no setting \"nosuch\""'
	check "edit: an unknown mod is refused" 'logged "$D" "line 22: modmenu enable: there is no mod \"nosuch\""'
	check "edit: a value that is not JSON is refused" 'logged "$D" "line 23: modmenu set: the value is not JSON"'

	# noop
	D="$G/noop"
	run "$GAME" "$D" "$S/noop.txt" "$HERE/user-noop"
	check "noop: mods.json keeps its bytes" 'cmp -s "$D/user/mods.json" "$HERE/user-noop/mods.json"'
	check "noop: mod-settings.json keeps its bytes" 'cmp -s "$D/user/mod-settings.json" "$HERE/user-noop/mod-settings.json"'

	# strings: a mod without settings keeps its texts, a % in them included; only a text for a key whose game text
	# has % codes (a printf format) must have the same codes, or the game's text is used
	D="$G/strings"
	rm -rf "$D"
	mkdir -p "$D/mods/texts" "$D/user"
	FORMAT_KEY=LEVEL
	[ "$GAME" = bigmoney ] && FORMAT_KEY=NEXT_RANK_AT_X_PTS
	printf '{ "id": "texts", "strings": { "EXCELLENT": "100%% GEMS!", "GO": "GO 100%%!", "MODMENUTEST_NEW": "50%%", "%s": "%%s %%n" } }\n' \
		"$FORMAT_KEY" > "$D/mods/texts/mod.json"
	(cd "$D" && timeout 300 "$ROOT/build/native/$GAME.exe" -gamedir="$(winpath "$(gamedir "$GAME")")" -userdir=user -modsdir=mods \
		-seed=1 -fast -script="$(winpath "$S/strings.txt")" > stdout.txt 2> log.txt; echo $? > exit.txt)
	check "strings: the run ended (exit $(cat "$D/exit.txt"))" '[ "$(cat "$D/exit.txt")" = 0 ]'
	check "strings: texts the game only draws, and new keys, keep a mod's %" \
		'! grep -q -e "mod strings: EXCELLENT:" -e "mod strings: GO:" -e "mod strings: MODMENUTEST_NEW:" "$D/log.txt"'
	check "strings: a text for a printf format key ($FORMAT_KEY) with other % codes is refused" \
		'logged "$D" "mod strings: $FORMAT_KEY: its % codes differ from the game'"'"'s text"'

	# keys
	D="$G/keys"
	run "$GAME" "$D" "$S/keys.txt" -
	check "keys: mods.json" 'cmp -s "$D/user/mods.json" "$E/keys-mods.json"'
	check "keys: mod-settings.json" 'cmp -s "$D/user/mod-settings.json" "$(forgame "$E" keys-mod-settings.json)"'
	check "keys: a live value applies at once" '[ "$(state_value "$D/keys-state.json" settings-demo volume applied)" = 90 ]'
	check "keys: the menu closed (no screenshot error, no unknown command)" '! grep -q "script: line" "$D/log.txt"'

	# clicks
	D="$G/clicks"
	run "$GAME" "$D" "$(forgame "$S" clicks.txt)" -
	check "clicks: mods.json" 'cmp -s "$D/user/mods.json" "$E/clicks-mods.json"'
	check "clicks: mod-settings.json" 'cmp -s "$D/user/mod-settings.json" "$(forgame "$E" clicks-mod-settings.json)"'
	check "clicks: nothing logged by the script" '! grep -q "script: line" "$D/log.txt"'

	# restart: the first run's own restart is only logged (a test run); with BYOPOP_RELAUNCH_TEST_ARGS the game
	# starts again, under this rundesk, which waits for it
	D="$G/restart"
	run "$GAME" "$D" "$S/$GAME-restart.txt" -
	check "restart: the arguments a restart keeps are logged" \
		'logged "$D" "restart: -gamedir=$(winpath "$(gamedir "$GAME")") -userdir=user -modsdir=mods (a test run: the game does not start again)"'
	if [ "$GAME" = bejeweled ]; then
		check "restart: the game in progress was saved" 'ls "$D/user" | grep -q "^savegame-.*\.dat$"'
	fi
	D="$G/relaunch"
	export BYOPOP_RELAUNCH_TEST_ARGS="-script=$(winpath "$S/$GAME-relaunched.txt") -fast -seed=1"
	run "$GAME" "$D" "$S/$GAME-restart.txt" -
	unset BYOPOP_RELAUNCH_TEST_ARGS
	# The second process runs on after the first: wait for this checkout's game exe to end (at most 2 minutes)
	EXE_W="$(cygpath -w "$ROOT/build/native/$GAME.exe" 2>/dev/null || echo "$ROOT/build/native/$GAME.exe")"
	for i in $(seq 1 60); do
		n=$(powershell -NoProfile -Command "@(Get-CimInstance Win32_Process -Filter \"Name='$GAME.exe'\" | Where-Object { \$_.ExecutablePath -eq '$EXE_W' }).Count" 2> /dev/null | tr -d '\r')
		[ "${n:-0}" = 0 ] && break
		sleep 2
	done
	check "relaunch: the restart was not only logged" 'logged "$D" "restart: -gamedir=" && ! logged "$D" "(a test run"'
	check "relaunch: the second process ran (rundesk waited for it)" '[ -f "$D/relaunched-state.json" ]'
	check "relaunch: it applied the value changed before the restart" \
		'[ -f "$D/relaunched-state.json" ] && [ "$(state_value "$D/relaunched-state.json" settings-demo praise applied)" = "\"WOW\"" ]'
	check "relaunch: no restart is pending any more" \
		'[ -f "$D/relaunched-state.json" ] && grep -q "\"restart\": \[\]" "$D/relaunched-state.json"'

	# pause (Bejeweled): the menu pauses as Options does
	if [ -f "$S/$GAME-pause-options.txt" ]; then
		for v in options menu esc; do
			run "$GAME" "$G/pause-$v" "$S/$GAME-pause-$v.txt" - -tracehash=trace.txt
		done
		# While the game is paused (updates 620..719) Options is an open dialog, which both hashes include, and the
		# menu is not; before and after, every update must hash the same. The two menu runs are the same throughout.
		outside() { awk '$1 < 620 || $1 >= 720' "$1"; }
		check "pause: the menu opened and closed by command traces as Options" \
			'[ -s "$G/pause-options/trace.txt" ] && [ "$(outside "$G/pause-options/trace.txt")" = "$(outside "$G/pause-menu/trace.txt")" ]'
		check "pause: the board stays paused while the menu is open (its state does not change)" \
			'[ "$(awk '"'"'$1 >= 621 && $1 < 720 { print $2, $3 }'"'"' "$G/pause-menu/trace.txt" | sort -u | wc -l)" = 1 ]'
		check "pause: the menu used by keyboard and closed by Esc traces the same" 'cmp -s "$G/pause-menu/trace.txt" "$G/pause-esc/trace.txt"'
	fi

	# nextgame (Bejeweled): a value held for the next fresh game (docs/MOD_MENU_DESIGN.md 6.2)
	if [ -f "$S/$GAME-nextgame-held.txt" ]; then
		D="$G/nextgame-held"
		run "$GAME" "$D" "$S/$GAME-nextgame-held.txt" -
		check "nextgame: a value set during a game waits (the game keeps speed 1)" \
			'[ "$(state_value "$D/held-state.json" settings-demo speed) $(state_value "$D/held-state.json" settings-demo speed applied)" = "2 1" ]'
		check "nextgame: the menu lists it as waiting for the next game" '[ "$(state_json "$D/held-state.json" "len(s[\"pending\"][\"nextGame\"])")" = 1 ]'
		check "nextgame: the quit saved the game under its score key, and the value waits in \"next\"" \
			'ls "$D/user" | grep -q "^savegame-.*\.dat$" && grep -q "\"next\"" "$D/user/mod-settings.json"'
		D2="$G/nextgame-applied"
		run "$GAME" "$D2" "$S/$GAME-nextgame-applied.txt" "$D/user"
		check "nextgame: at the next start it still waits while the save waits for Continue" \
			'[ "$(state_value "$D2/start-state.json" settings-demo speed applied)" = 1 ]'
		check "nextgame: Continue? No: the untouched board is made again with the value" \
			'logged "$D2" "the untouched board is made again" && [ "$(state_value "$D2/applied-state.json" settings-demo speed applied)" = 2 ]'
		check "nextgame: nothing waits any more, and the score key is the one announced" \
			'[ "$(state_json "$D2/applied-state.json" "len(s[\"pending\"][\"nextGame\"])")" = 0 ] && [ "$(state_json "$D2/applied-state.json" "s[\"scoreKey\"]")" = "$(state_json "$D2/start-state.json" "s[\"nextScoreKey\"]")" ] && [ "$(state_json "$D2/applied-state.json" "s[\"scoreKey\"]")" != "$(state_json "$D2/start-state.json" "s[\"scoreKey\"]")" ]'
		check "nextgame: screenshots (Continue, the board, Options with Mods, the menu it opens)" '[ -f "$D2/shots/n04_menu_over_options.png" ]'
	fi

	# screens: for a person to look at
	run "$GAME" "$G/screens" "$S/$GAME-screens.txt" -
	check "screens: written" '[ "$(ls "$G/screens/shots" | wc -l)" -ge 8 ]'
	D="$G/screens-empty"
	rm -rf "$D"
	mkdir -p "$D/mods" "$D/user" "$D/shots"
	(cd "$D" && timeout 300 "$ROOT/build/native/$GAME.exe" -gamedir="$(winpath "$(gamedir "$GAME")")" -userdir=user -modsdir=mods \
		-seed=1 -fast -nomods -script="$(winpath "$S/screens-empty.txt")" > stdout.txt 2> log.txt; echo $? > exit.txt)
	check "screens: empty and -nomods" '[ -f "$D/shots/e01_empty_nomods.png" ]'
	echo "  screenshots: $(winpath "$G")/screens*/shots"
done

echo "sources:"
check "no random draw in the menu's sources (check_no_rand.py)" '"$PY" "$(winpath "$HERE/check_no_rand.py")" > /dev/null'

echo "fwtest:"
BEJ_DIR="$(gamedir bejeweled)"
if [ ! -f "$ROOT/build/native/fwtest.exe" ]; then
	fail "build/native/fwtest.exe is missing"
elif [ ! -d "$BEJ_DIR" ]; then
	if [ "$SELECTION" != all ] && [ "$SELECTION" != bejeweled ]; then
		echo "  skipped: fwtest -hotkeytest needs Bejeweled's files, not found at $(winpath "$BEJ_DIR") (give --gamedir bejeweled=PATH)"
	else
		fail "fwtest -hotkeytest: Bejeweled's files are not found at $(winpath "$BEJ_DIR") (give --gamedir bejeweled=PATH)"
	fi
else
	mkdir -p "$OUT"
	(cd "$ROOT/build/native" && timeout 300 ./fwtest.exe -hotkeytest -gamedir="$(winpath "$BEJ_DIR")" > "$OUT/fwtest-hotkey.txt" 2>&1)
	FW=$?
	grep "hotkey:" "$OUT/fwtest-hotkey.txt" | sed 's/^INFO: /  /'
	check "fwtest -hotkeytest" '[ $FW = 0 ]'
fi

echo "$FAILED failed"
exit $FAILED
