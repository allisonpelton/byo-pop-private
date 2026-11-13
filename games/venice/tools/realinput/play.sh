#!/bin/bash
# play.sh <mode> <display> [steps] [extra playbymove options]: a whole game of one mode played by the port's bot as real input
# (docs/REAL_INPUT.md, docs/TEST_COMMANDS.md "The bot behind move"; tools/playbymove). The menus are clicked at the places the `buttons`
# command gives, as real SDL events (move, a press and a release 3 updates apart: the game reads the button once an update), then the
# loop plays the bot's `move` answers until the play is over.
#   mode     journey (a new player: the tutorial, then the levels), flood, survival or trickshot (every mode unlocked: a new game)
#   display  2d (the original's software display, the registry's Is3D 0), 3d (the port's 3D display) or full2d / full3d (full screen: the
#            visible desktop, so only between 22:00 and 08:00, CLAUDE.md "Overnight")
# Each run's folder is build/realinput/<mode>-<display> (the driver's transcript, the game's log, the last screen as end.png and the
# summary line). Run from the worktree root, one game at a time (tools/gamecount.py --limit 6 first).
cd "$(dirname "$0")/../../../.." || exit 2
W="$(pwd -W)"
MODE="$1"; DISPLAY_KIND="$2"; STEPS="${3:-2500}"; shift 3 2>/dev/null
D="build/realinput/$MODE-$DISPLAY_KIND"
mkdir -p "$D/user"; rm -f "$D/user/profiles.dat" "$D/user/registry.ini" "$D/driver.txt" "$D/game.log" "$D/summary.txt" "$D/end.png"
case "$DISPLAY_KIND" in
	2d) printf '[SOFTWARE\PopCap\Venice]\nIs3D=dword:0\nScreenMode=dword:0\n' > "$D/user/registry.ini" ;;
	3d) printf '[SOFTWARE\PopCap\Venice]\nScreenMode=dword:0\n' > "$D/user/registry.ini" ;;
	full2d) printf '[SOFTWARE\PopCap\Venice]\nIs3D=dword:0\nScreenMode=dword:1\n' > "$D/user/registry.ini" ;;
	full3d) printf '[SOFTWARE\PopCap\Venice]\nScreenMode=dword:1\n' > "$D/user/registry.ini" ;;
	*) echo "display: 2d, 3d, full2d or full3d"; exit 2 ;;
esac
F=()
add() { F+=(--first "$1"); }
click() { add "move game $1 $2"; add "wait 3"; add "down game $1 $2"; add "wait 3"; add "up game $1 $2"; add "wait $3"; }
add "seed 1"; add "player Test"
add "goto menu"; add "wait 400"; add "bot 0 continue"
[ "$MODE" != journey ] && add "unlock all"   # the level lists exist once the loading has ended
click 400 232 700                       # Play Now
case "$MODE" in
	journey) click 400 220 700 ;;                  # Journey: a new player goes to the tutorial
	flood) click 401 270 700; click 470 330 700 ;; # Flood, and "new game" of the continue prompt
	survival) click 400 370 700 ;;
	trickshot) click 398 320 700 ;;
	*) echo "mode: journey, flood, survival or trickshot"; exit 2 ;;
esac
until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done
python -I games/venice/tools/realinput/drive.py --dir "$W/$D" --steps "$STEPS" --timeout 180 --shot "$W/$D/end.png" "${F[@]}" "$@" \
	"$W/build/native/venice.exe" "-gamedir=$W/assets/venice" "-userdir=$W/$D/user" -seed=1 > "$D/summary.txt" 2>&1
tail -1 "$D/summary.txt" | cut -c1-500
