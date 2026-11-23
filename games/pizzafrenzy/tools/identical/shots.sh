#!/bin/bash
# shots.sh <case> <seed> <shot list> <run id>: the looks phase's pictures. Plays a case's recorded input
# (build/identical/<case>/input.txt, made by case.sh, RECORD_ONLY=1 for a case that only needs it) in the port and in
# the original, takes a picture at each frame the shot list names ("<frame> <name>" lines, # comments), and runs
# tools/shotdiff on the two folders (docs/SHOTDIFF.md).
# - The port: port.sh (rundesk, -fast) with -renderscale=1 -drawevery; each shot is a "screenshot" line of its input.
# - The original: origrun inside Windows Sandbox (the only place it takes pictures, docs/ORIGRUN.md), on the virtual
#   clock with the patched frame counter, from a copy of the staged game in the run folder (build/origrun/<run>/game,
#   which --sandbox needs), with setup.sh's second patch list: a window capture shows nothing (and makes the game
#   crash), so present copies each frame to a buffer that origrun reads (--surface-screen); a fresh sandbox has no
#   registry key, so it is a first launch. REG=<file.reg> imports a player first (a case's user.reg). An "at N shot"
#   shows frame N or an earlier one (no SexyAppBase flags here).
# PORT_ONLY=1 or ORIG_ONLY=1 takes one side; VGPU=1 gives the sandbox the host's GPU.
# The pictures: build/shots-pizzafrenzy/<run>/{port,orig}.
source "$(dirname "$0")/common.sh"
C="$1"; SEED="$2"; LIST="$3"; RUN="$4"
D="$OUT/$C"
[ -f "$D/input.txt" ] || { echo "shots.sh: no $D/input.txt (case.sh with RECORD_ONLY=1 makes it)"; exit 1; }
S="build/shots-pizzafrenzy/$RUN"
mkdir -p "$S"
SHOTS=$(grep -v '^#' "$LIST" | awk 'NF >= 2')
QUIT=$(tail -1 "$D/input.txt" | cut -d" " -f1)
if [ -z "$ORIG_ONLY" ]; then
	rm -rf "$S/port"; mkdir -p "$S/port"
	# the shots go in before the input of their frame (both run at once, before the frame's update); quit stays last
	{ grep -v ' quit$' "$D/input.txt" | awk '{ print $1, 1, $0 }'
	  echo "$SHOTS" | while read -r n name; do echo "$n 0 $n screenshot $(win "$S/port/$name.png")"; done
	} | sort -s -n -k1,1 -k2,2 | cut -d" " -f3- > "$S/port_input.txt"
	tail -1 "$D/input.txt" >> "$S/port_input.txt"
	# a case that starts from a saved player (REG=) starts the port from a fresh copy of the case's player folder
	if [ -n "$REG" ] && [ -d "$D/port/user" ]; then
		rm -rf "$S/portrun/user"; mkdir -p "$S/portrun"; cp -r "$D/port/user" "$S/portrun/user"
		export KEEP_USER=1
	fi
	"$TOOLS/port.sh" "$S/portrun" -seed="$SEED" -renderscale=1 -drawevery -realinput="$(win "$S/port_input.txt")"
	echo "port: $(ls "$S/port" | wc -l) pictures"
fi
if [ -z "$PORT_ONLY" ]; then
	R="build/origrun/pfshot-$RUN"
	rm -rf "$R"; mkdir -p "$R"
	cp -r "$OG_GAME" "$R/game"
	rm -f "$R/game/userPizzas.xml" "$R/game/PizzaFrenzyGame.exe.origrun-unpatched"
	# the carved exe, which origrun patches in the copy (--patches)
	cp "$GAME_EXE" "$R/game/PizzaFrenzyGame.exe"
	python -I "$TOOLS/toorig.py" "$D/input.txt" | grep -v '^quit$' | awk '{ print $2, 1, $0 }' > "$S/orig_lines.txt"
	echo "$SHOTS" | while read -r n name; do echo "$n 0 at $n shot $name"; done >> "$S/orig_lines.txt"
	{ sort -s -n -k1,1 -k2,2 "$S/orig_lines.txt" | cut -d" " -f3-; echo quit; } > "$S/orig_script.txt"
	ARGS=(--exe "$(cygpath -aw "$R/game/PizzaFrenzyGame.exe")" --run "pfshot-$RUN" --script "$(win "$S/orig_script.txt")"
		--clock virtual --cursor scripted --counter 0x541a00 --update-ms 33 --seed "$SEED"
		--patches "${PATCHES%.txt}-shots.txt"
		--surface-screen 0x541a08,0x541a0c,800,600,32 --surface-gate 0x541a10
		--sandbox ${VGPU:+--sandbox-vgpu} --timeout $((QUIT / 50 + 120)))
	[ -n "$REG" ] && ARGS+=(--reg "$(win "$REG")")
	"$ORIGRUN" "${ARGS[@]}" > "$S/orig.log" 2>&1
	grep -E "RESULT|SOUND LEAK" "$S/orig.log" | cut -c1-200
	rm -rf "$S/orig"; mkdir -p "$S/orig"
	[ -d "$R/shots" ] && cp "$R/shots/"*.png "$S/orig/" 2>/dev/null
	echo "original: $(ls "$S/orig" | wc -l) pictures"
	cp "$R/game/PizzaFrenzyGame.log" "$S/orig_game.log" 2>/dev/null
	rm -rf "$R/game"
fi
if [ -z "$PORT_ONLY" ] && [ -z "$ORIG_ONLY" ]; then
	python -I tools/shotdiff/shotdiff.py --game pizzafrenzy --orig "$S/orig" --port "$S/port" --run "$RUN" | tail -15
fi
