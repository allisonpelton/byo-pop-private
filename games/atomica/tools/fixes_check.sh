#!/usr/bin/env bash
# games/atomica/tools/fixes_check.sh: the fixes mod's "drawRandom" fix against the original's behaviour (docs/GAME_NOTES.md, "The fixes
# mod"). The bot plays one strategy game under -fast twice, once drawing a picture every few updates (the default under -fast) and once
# drawing every update (-drawevery), each without the mod and with it. Without the mod the pictures draw random numbers from the game's
# sequence, so the two runs deal other atoms and their state traces differ; with the mod they must be the same, update for update.
# Prints one line per pair and "fixes: ok" when the unmodded pair differs and the modded pair is equal. Logs: build/fixes/<run>/.
set -u
cd "$(dirname "$0")/../../.." || exit 2
W="$(pwd -W)"
CAP="${1:-12000}"
fail=0
run() {  # <name> <mods: off|on> <drawevery: 0|1>
	local d="build/fixes/$1"
	rm -rf "$d"; mkdir -p "$d/user"
	until python tools/gamecount.py --limit "${GAMELIMIT:-8}" > /dev/null 2>&1; do sleep 20; done
	local mods="-nomods" draw=""
	[ "$2" = on ] && mods="-modsdir=$W/games/atomica/mods"
	[ "$3" = 1 ] && draw="-drawevery"
	build/native/rundesk.exe "$W/build/native/atomica.exe" -gamedir="$W/assets/atomica" -userdir="$W/$d/user" -fast -seed=1 $draw $mods \
		-autoplay=1 -autoplaymode=strategy -autoplaycap="$CAP" -autoplaylog="$W/$d/log.txt" -statetrace="$W/$d/trace.txt" \
		> "$d/stdout.txt" 2> "$d/stderr.txt"
}
for mods in off on; do
	run "$mods-plain" "$mods" 0
	run "$mods-every" "$mods" 1
	if cmp -s "build/fixes/$mods-plain/trace.txt" "build/fixes/$mods-every/trace.txt" && [ -s "build/fixes/$mods-plain/trace.txt" ]; then
		echo "mods $mods: the two runs have the same state trace ($(wc -l < "build/fixes/$mods-plain/trace.txt") lines)"
		[ "$mods" = off ] && fail=1
	else
		echo "mods $mods: the two runs differ at line $(cmp "build/fixes/$mods-plain/trace.txt" "build/fixes/$mods-every/trace.txt" 2>&1 | sed -n 's/.*line \([0-9]*\).*/\1/p')"
		[ "$mods" = on ] && fail=1
	fi
done
[ $fail = 0 ] && echo "fixes: ok" || echo "fixes: FAILED"
exit $fail
