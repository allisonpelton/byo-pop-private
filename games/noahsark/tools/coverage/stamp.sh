#!/bin/bash
# stamp.sh: the Stampede round (level 4 of an easy action game) through a saved game, as docs/IDENTICAL_PLAY.md does it ("Stampede,
# through a saved game"): the bot's replay cannot reach level 4 in the original (the get-ready's end depends on the clicks), but the
# port writes the original's savegame.dat. A: the port's bot plays to the round (update 13539 of the run act_e), a script answers
# its dialog and quits to the menu, which saves. B: the port resumes the file with Continue and its bot plays the round (5100
# updates, to the round's end and into level 5); its clicks become the original's script. C: the original resumes the same file
# (run_original.sh's SAVE) and the script replays the clicks, with --coverage. Hits: build/coverage/noahsark/hits/stamp.txt.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
S=build/coverage/noahsark/scripts; H=build/coverage/noahsark/hits; mkdir -p "$S" "$H"
room() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done; }
MENU="$S/stamp.menu.txt"
printf '301 click 320 456\n360 click 198 188\n381 click 320 195\n420 click 400 405\n' > "$S/stampA.menu.txt"
# A: the dialog of the round (answered at 13560), the in-game Quit (13650) and its Yes (13680)
(cat "$S/stampA.menu.txt"; printf '13560 click 330 335\n13650 click 70 450\n13680 click 308 333\n13760 quit\n') > "$S/stampA.port.txt"
room; rm -rf build/coverage/noahsark/ud_stampA
games/noahsark/tools/run_script.sh "$S/stampA.port.txt" build/coverage/noahsark/ud_stampA 200 -seed=1 -autoplay=1 -autoplaymenu \
	-autoplaymode=action -autoplaydiff=0 -autoplaycap=14000 -autoplaylog="$S/stampA.bot" -statelog="$S/stampA.state" > "$S/stampA.log" 2>&1
cp build/coverage/noahsark/ud_stampA/savegame.dat "$S/stamp.save" || { echo "stamp: the port wrote no save"; exit 1; }
# B: Continue (the title's link at 301, the button at 360), the bot plays the round from the save
printf '301 click 320 456\n360 click 198 350\n' > "$MENU"
(cat "$MENU"; echo "5200 quit") > "$S/stampB.port.txt"
room; rm -rf build/coverage/noahsark/ud_stampB; mkdir -p build/coverage/noahsark/ud_stampB; cp "$S/stamp.save" build/coverage/noahsark/ud_stampB/savegame.dat
games/noahsark/tools/run_script.sh "$S/stampB.port.txt" build/coverage/noahsark/ud_stampB 200 -seed=1 -autoplay=1 -autoplaymenu \
	-autoplaymode=action -autoplaydiff=0 -autoplaycap=5100 -autoplaylog="$S/stampB.bot" -statelog="$S/stampB.state" > "$S/stampB.log" 2>&1
python -I games/noahsark/tools/origrun_replay.py script "$MENU" "$S/stampB.bot" "$S/stamp.orig.txt" 5100
# C: the original
SAVE="$ROOT/$S/stamp.save" bash games/noahsark/tools/coverage/run.sh stamp "$S/stamp.orig.txt" 1 2>&1 | tail -1
