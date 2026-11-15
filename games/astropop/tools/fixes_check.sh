#!/usr/bin/env bash
# games/astropop/tools/fixes_check.sh: the fixes mod's toggle "weaponBoxTick" (games/astropop/mods/fixes/mod.json; GAME_NOTES.md,
# section 7). The port's own bot plays one classic game with real input (tools/realplay.py) whose weapon is charged from the start, so
# the box that explains the weapon comes up and the bot clicks its OK, twice: with no mods (the original: the button plays the
# one-second timer's tick) and with the fixes mod on (no tick). The sound logs (BYOPOP_SOUNDLOG) are compared: the same game must
# give the same sounds but for one play of sounds/timer1, and no other state may change (the final STATE line is the same).
# Prints one line per run and "fixes: ok" or "fixes: FAILED". Logs: build/fixes/astropop/<run>/ and <run>.out.
set -u
cd "$(dirname "$0")/../../.." || exit 2
W="$(pwd -W)"
MOVES="${1:-60}"
SEED="${2:-3}"
fail=0
run() {  # <name> <extra realplay arguments...>
	local n="$1"; shift
	local d="build/fixes/astropop/$n"
	rm -rf "$d" "$d.out"; mkdir -p "build/fixes/astropop"
	BYOPOP_SOUNDLOG="$W/$d/sound.log" python -I games/astropop/tools/realplay.py --mode classic --moves "$MOVES" --seed "$SEED" \
		--charge 18000 --no-shots --dir "$d" "$@" > "$d.out" 2>&1
	tail -1 "$d.out" | cut -c1-200
}
run off
run on --mods "$W/games/astropop/mods"
index=$(sed -n 's/^0 load \([0-9]*\) sounds\/timer1 .*/\1/p' build/fixes/astropop/off/sound.log | head -1)
if [ -z "$index" ]; then echo "fixes: FAILED (no sounds/timer1 in the sound log)"; exit 1; fi
plays_off=$(grep -c "^[0-9]* play $index " build/fixes/astropop/off/sound.log)
plays_on=$(grep -c "^[0-9]* play $index " build/fixes/astropop/on/sound.log)
echo "plays of sounds/timer1: $plays_off without the mod, $plays_on with it"
[ "$plays_off" -eq $((plays_on + 1)) ] || fail=1
# the rest of the sounds are the same (the instance numbers shift by one after the missing play, so they are left out, and so are the instance, play and stop of the timer's tick)
strip() { sed -E 's/ #[0-9]+//' "$1" | grep -Ev "^[0-9]+ (instance|play|stop) $index( |\$)" | sort; }
if cmp -s <(strip build/fixes/astropop/off/sound.log) <(strip build/fixes/astropop/on/sound.log); then
	echo "the other sounds: the same"
else
	echo "the other sounds differ"; fail=1
fi
# the game's end state: every STATE line of the two runs' driver logs but the sound-free fields are the same
if cmp -s <(tail -1 build/fixes/astropop/off.out) <(tail -1 build/fixes/astropop/on.out); then
	echo "the summary line: the same"
else
	echo "the summary lines differ"; fail=1
fi
[ $fail = 0 ] && echo "fixes: ok" || echo "fixes: FAILED"
exit $fail
