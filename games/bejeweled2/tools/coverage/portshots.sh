#!/bin/bash
# portshots.sh <origrun script> <out folder> [seed]: plays a coverage script's inputs in the port (the same updates, -fast, under
# rundesk) and takes the script's `shot <name>` lines as <out>/cwd/shots/<name>.png: a coverage run on the host takes no shots, so a
# script's click positions are checked in the port's pictures, which are the original's (phase two). HIRES=1 plays the 1024 by 768
# game (3D), else the 640 by 480 one (2D); PROFILE=<folder of make_profile.py> is the player it starts with. Waits for a free place.
# PORT_EXE=<exe> plays another copy of the port (tools/collection/run.sh: the one beside the Collection's files), and REGKEY=<section>
# names the settings section of that set's partner.xml (the Collection's is SOFTWARE\PopCap\Bejeweled2).
S="$1"; OUT="$2"; SEED="${3:-1}"
PROFILE="${PROFILE-build/logs/bej2/profile}"  # the player of run.sh's runs; PROFILE= (empty) starts as a first run does
cd "$(dirname "$0")/../../../.." || exit 2
W="$(pwd -W)"
rm -rf "$OUT"; mkdir -p "$OUT/cwd/shots" "$OUT/user"
[ -n "$PROFILE" ] && cp -r "$PROFILE/users" "$OUT/user/users"
: > "$OUT/empty.ri"
HR=0; [ -n "$HIRES" ] && HR=1
printf '[%s]\nScreenMode=dword:0\nHighResolution=dword:%d\n' "${REGKEY:-SOFTWARE\\Steam\\Bejeweled2}" "$HR" > "$OUT/user/registry.ini"
python - "$S" "$OUT/events.json" << 'PY'
import json, re, sys
ev = []
for l in open(sys.argv[1]):
    m = re.match(r'at (\d+) (.*)$', l.strip())
    if not m or m.group(2) in ('mark', 'quit') or m.group(2).startswith('gate'):
        continue
    cmd = m.group(2).split(' ')
    key = {'13': 'Return', '27': 'Escape', '32': 'space', '9': 'Tab'}
    if cmd[0] in ('move', 'click', 'down', 'up'):     # the port's verbs name the pointer's space: "click game 320 401"
        line = '%s game %s %s' % (cmd[0], cmd[1], cmd[2])
    elif cmd[0] == 'text':
        line = 'type ' + m.group(2)[5:]
    elif cmd[0] == 'key' and cmd[1] in key:
        line = 'key ' + key[cmd[1]]
    elif cmd[0] == 'shot':
        line = m.group(2)
    else:
        print('portshots: the port has no counterpart for "%s": skipped' % m.group(2), file=sys.stderr)
        continue
    ev.append([int(m.group(1)), line])
json.dump({'events': ev}, open(sys.argv[2], 'w'))
PY
python games/bejeweled2/tools/origcmp/to_commands.py "$OUT/events.json" "$OUT/cmd.txt" --shots shots > /dev/null || exit 2
[ -z "$HIRES" ] && export SDL_RENDER_DRIVER=software  # true 2D: SDL's software renderer, so DDInterface::mIs3D is false (driver_base.py)
until python -I tools/gamecount.py --limit "${GAMELIMIT:-6}" > /dev/null; do sleep 20; done
(cd "$OUT/cwd" && "$W/build/native/rundesk.exe" "${PORT_EXE:-$W/build/native/bejeweled2.exe}" -gamedir= -userdir="$W/$OUT/user" -commands="$W/$OUT/cmd.txt" \
	-realinput="$W/$OUT/empty.ri" -hold -fast -drawevery -trace -seed="$SEED" > ../port.log 2>&1)
echo "$OUT: $(ls "$OUT/cwd/shots" | wc -l) shots"
