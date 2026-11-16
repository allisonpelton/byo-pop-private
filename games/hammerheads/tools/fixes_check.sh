#!/usr/bin/env bash
# games/hammerheads/tools/fixes_check.sh: the fixes mod's three fixes (games/hammerheads/mods/fixes/mod.json; GAME_NOTES.md, "Fixes"),
# under rundesk with -fast -seed=1. tools/fixes_test.txt runs five times: as the original (-original -nomods), with the mod at its
# defaults (a copy of mods/fixes in the run's folder as -modsdir), and with -fix=quitEscape and -fix=menuTrophySting alone. Expected: the quit box stays open after Escape in the original and goes with quitEscape; the sting
# (BYOPOP_SOUNDLOG) plays once more in the original's trophy room visit than with menuTrophySting. restartCoinCount needs a Marathon
# game to its first restart point (not in this tour); it only changes which font measures the number.
# Prints one line a run and "fixes: ok" or "fixes: FAILED". Logs: build/fixes/hammerheads/<run>/.
set -u
cd "$(dirname "$0")/../../.." || exit 2
W="$(pwd -W)"
D=build/fixes/hammerheads
fail=0
run() {  # <name> <extra arguments...>
	local n="$1"; shift
	local d="$D/$n"
	rm -rf "$d"; mkdir -p "$d/user" "$d/mods"
	cp -r games/hammerheads/mods/fixes "$d/mods/fixes"
	sed "s|@DIR@|$W/$d|" games/hammerheads/tools/fixes_test.txt > "$d/script.txt"
	BYOPOP_SOUNDLOG="$W/$d/sound.log" timeout 300 "$W/build/native/rundesk.exe" "$W/build/native/hammerheads.exe" -gamedir=assets/hammerheads \
		-userdir="$W/$d/user" -fast -seed=1 "$@" -script="$W/$d/script.txt" > "$d/stdout.txt" 2>&1
	grep -E '^(OK|ERROR|STATE) ' "$d/stdout.txt" > "$d/states.txt"
	echo "$n: $(grep -c '^STATE' "$d/states.txt") states, $(grep -c '^ERROR' "$d/states.txt") errors, stings $(grep -c '^[0-9]* play 5 ' "$d/sound.log" 2>/dev/null)"
}
run off -original -nomods
run mod "-modsdir=$W/$D/mod/mods"
run quit -nomods -fix=quitEscape
run sting -nomods -fix=menuTrophySting
grep -q '^ERROR' $D/*/states.txt && { echo "a test command failed"; fail=1; }
# the quit box after Escape: the third state line of the run (quit.png's state, then afterescape's)
dlg() { grep '^STATE' "$1/states.txt" | sed -n "$2p" | sed 's/.*"dialogs":\(\[[^]]*\]\).*/\1/'; }
[ "$(dlg $D/off 3)" = "$(dlg $D/off 2)" ] || { echo "the original's quit box closed on Escape"; fail=1; }
[ "$(dlg $D/quit 3)" != "$(dlg $D/quit 2)" ] || { echo "quitEscape: the quit box stayed open on Escape"; fail=1; }
[ "$(dlg $D/mod 3)" != "$(dlg $D/mod 2)" ] || { echo "the mod's quit box stayed open on Escape"; fail=1; }
s_off="$(grep -c '^[0-9]* play 5 ' $D/off/sound.log 2>/dev/null)"; s_fix="$(grep -c '^[0-9]* play 5 ' $D/sting/sound.log 2>/dev/null)"
[ "${s_off:-0}" -gt "${s_fix:-0}" ] || { echo "menuTrophySting: the sting count is $s_fix with it and $s_off without"; fail=1; }
[ $fail = 0 ] && echo "fixes: ok" || { echo "fixes: FAILED"; exit 1; }
