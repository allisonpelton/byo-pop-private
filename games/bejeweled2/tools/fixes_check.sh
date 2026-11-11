#!/usr/bin/env bash
# games/bejeweled2/tools/fixes_check.sh: the fixes mod's toggles against the original's behaviour (docs/GAME_NOTES.md, "The fixes
# mod"). Two tours in 3D are recorded once (origcmp/record_port.sh tour:<mode>, into build/fixes/tour_<mode>) and replayed with the
# same seed and inputs under four settings each, taking the tour's shots and the board's -trace lines:
#   off      -nomods: the original
#   default  the mod as the site serves it: every toggle at its default (tunnelEdge on, podFade off: mod.json)
#   on       the mod with every toggle on (mod-settings.json in the player's folder)
#   none     the mod with every toggle off (mod-settings.json in the player's folder)
#   no<fix>  the mod with every toggle on but one: tunnelEdge in the Hyper tour (whose level change runs through the hyperspace
#            tunnel: the board's state 10, from the whirlpool's end to the new board's), podFade in the Puzzle tour (whose solved
#            puzzle lights its pod, which takes 250 updates)
# Shots are added to each recording where the toggle's picture is on the screen: along the tunnel, and after the puzzle.
# Expected: the trace lines are the same in all five (a fix mends a picture and never changes play), the shots of "none" are the
# shots of "off" (a toggle that is off is the original), "on" has shots that "off" has not, the toggle left out of "no<fix>"
# is what changes shots between "on" and it, and "default" has the shots of "on" where the tour's toggle is on by default and the
# shots of "none" where it is off.
# Prints a line for each run and each finding, and "fixes: ok" or "fixes: FAILED". Logs and shots: build/fixes/<tour>-<run>/.
# Needs a free game slot for each run.
set -u
cd "$(dirname "$0")/../../.." || exit 2
W="$(pwd -W)"
OUT=build/fixes
PROFILE=build/logs/bej2/profile
MODS="-modsdir=$W/games/bejeweled2/mods"
mkdir -p "$OUT"
wait_slot() { until python tools/gamecount.py --limit "${GAMELIMIT:-3}" > /dev/null 2>&1; do sleep 15; done; }

# the player with every mode unlocked (make_profile.py, 2D)
if [ ! -d "$PROFILE/users" ]; then
	wait_slot
	python games/bejeweled2/tools/origcmp/make_profile.py --exe build/native/bejeweled2.exe --out "$PROFILE" > /dev/null || exit 2
fi

# <recording dir> <tunnel | after>: events_x.json is the recording with shots along the tunnel (every 40 updates while the board's state
# is 10), or six shots 40 updates apart after its last event
add_shots() {
	python - "$1" "$2" << 'PY'
import json, re, sys
d, how = sys.argv[1:3]
ev = json.load(open(d + '/events.json'))
e = ev['events']
if how == 'tunnel':
    log = open(d + '/game.log').read()
    a = int(re.search(r'watch 3: \d+ -> 10 at update (\d+)', log).group(1))
    b = int(re.search(r'watch 3: 10 -> 13 at update (\d+)', log).group(1))
    ups = range(a + 20, b, 40)
else:
    ups = [e[-1][0] + 40 * k for k in range(1, 7)]
e += [[u, 'shot x%d' % u] for u in ups]
e.sort(key=lambda x: x[0])
json.dump(ev, open(d + '/events_x.json', 'w'))
PY
}

run() {  # <tour> <name> <-nomods | -modsdir=...> <mod-settings.json or "">
	local d="$OUT/$1-$2"
	rm -rf "$d"; mkdir -p "$d/cwd/shots" "$d/user"
	cp -r "$PROFILE/users" "$d/user/users"
	: > "$d/empty.ri"
	printf '[SOFTWARE\\Steam\\Bejeweled2]\nScreenMode=dword:0\nHighResolution=dword:1\n' > "$d/user/registry.ini"
	[ -n "$4" ] && printf '%s\n' "$4" > "$d/user/mod-settings.json"
	wait_slot
	(cd "$d/cwd" && "$W/build/native/rundesk.exe" "$W/build/native/bejeweled2.exe" -gamedir= -userdir="$W/$d/user" -commands="$W/$OUT/$1.cmd" \
		-realinput="$W/$d/empty.ri" -hold -fast -drawevery -seed=909 -trace $3 > ../log.txt 2>&1)
	grep ' watch ' "$d/log.txt" > "$d/trace.txt"
	echo "$1-$2: $(ls "$d/cwd/shots" | wc -l) shots, $(wc -l < "$d/trace.txt") trace lines"
}

differing() {  # <dir a> <dir b>: the names of the shots that differ
	local n=""
	for f in "$OUT/$1/cwd/shots/"*.png; do
		cmp -s "$f" "$OUT/$2/cwd/shots/$(basename "$f")" || n="$n $(basename "$f" .png)"
	done
	echo "$n"
}

fail=0
check_tour() {  # <mode> <fix id> <tunnel | after> <the fix's default: on | off>
	local t=$1 fix=$2
	if [ ! -f "$OUT/tour_$t/events.json" ]; then
		PROFILE=$PROFILE DRAWEVERY=1 bash games/bejeweled2/tools/origcmp/record_port.sh "tour:$t" 12 "$OUT/tour_$t" 909 > /dev/null || exit 2
	fi
	add_shots "$OUT/tour_$t" "$3" || exit 2
	python games/bejeweled2/tools/origcmp/to_commands.py "$OUT/tour_$t/events_x.json" "$OUT/$t.cmd" --shots shots > /dev/null || exit 2
	run "$t" off -nomods ""
	run "$t" default "$MODS" ""
	run "$t" on "$MODS" '{"v":1,"mods":{"fixes":{"tunnelEdge":true,"podFade":true}}}'
	run "$t" none "$MODS" '{"v":1,"mods":{"fixes":{"tunnelEdge":false,"podFade":false}}}'
	run "$t" "no$fix" "$MODS" "$(python -c "import json,sys;print(json.dumps({'v':1,'mods':{'fixes':{k:k!=sys.argv[1] for k in ('tunnelEdge','podFade')}}}))" "$fix")"
	for r in default on none "no$fix"; do
		if cmp -s "$OUT/$t-off/trace.txt" "$OUT/$t-$r/trace.txt" && [ -s "$OUT/$t-off/trace.txt" ]; then
			echo "$t-$r: the board's trace is the original's"
		else
			echo "$t-$r: the board's trace differs from the original's"; fail=1
		fi
	done
	local d
	d=$(differing "$t-off" "$t-none")
	if [ -z "$d" ]; then echo "$t-none: every shot is the original's"; else echo "$t-none: shots differ from the original's:$d"; fail=1; fi
	d=$(differing "$t-off" "$t-on")
	if [ -n "$d" ]; then echo "$t-on: shots that differ from the original's:$d"; else echo "$t-on: no shot differs from the original's"; fail=1; fi
	local want=none
	[ "$4" = on ] && want=on
	d=$(differing "$t-$want" "$t-default")
	if [ -z "$d" ]; then echo "$t-default: every shot is that of the $want setting ($fix is $4 by default)"; else echo "$t-default: shots differ from the $want setting's:$d"; fail=1; fi
	d=$(differing "$t-on" "$t-no$fix")
	if [ -n "$d" ]; then echo "$fix: the shots it changes:$d"; else echo "$fix: changes no shot of the $t tour"; fail=1; fi
}
check_tour hyper tunnelEdge tunnel on
check_tour puzzle podFade after off
[ $fail = 0 ] && echo "fixes: ok" || echo "fixes: FAILED"
exit $fail
