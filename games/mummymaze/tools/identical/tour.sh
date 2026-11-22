#!/bin/bash
# tour.sh <exe> <out folder>: the screen tour of phase two's screens (SCREENS.md), port only. The scenarios of
# games/mummymaze/tools/identical/scenarios/ are run by <exe> (-fast, a fresh user folder each, seed 21) and their shots land in
# <out>/<scenario>/. tools/check.sh runs it for the port and for the frozen baseline and compares the two folders.
# The saves the scenarios continue are made here, by the port that runs them: a classic game saved 300 updates in at a chosen
# pyramid and chamber (-autoplaystart, -autoplaysave), and a saved adventure that craft_saves.py rewrites into five explorers
# at their places on the map (Red, Two, Scorp, Trap, Ankh), plus one of another format number.
#   build/native/rundesk.exe bash games/mummymaze/tools/identical/tour.sh build/native/mummymaze.exe build/tour/port
EXE="$(cd "$(dirname "$1")" && pwd -W)/$(basename "$1")"
mkdir -p "$2"; OUT="$(cd "$2" && pwd -W)"
T="$(cd "$(dirname "$0")" && pwd -W)"
SAVEDIR="$OUT/saves"; rm -rf "$SAVEDIR"; mkdir -p "$SAVEDIR/classic1414" "$SAVEDIR/classic014" "$SAVEDIR/adv" "$SAVEDIR/bad"
mk() {  # <folder> <bot arguments>...: a game that saves 300 updates in
	local f="$1"; shift
	"$EXE" -userdir="$f/u" -fast -seed=1 -autoplay=5 -autoplaygames=1 -autoplaysave=300 -autoplaycap=100000 "$@" > "$f/log.txt" 2>&1
}
mk "$SAVEDIR/classic1414" -autoplaystart=14,14; cp "$SAVEDIR/classic1414/u/classic.sav" "$SAVEDIR/classic1414.sav"
mk "$SAVEDIR/classic014" -autoplaystart=0,14; cp "$SAVEDIR/classic014/u/classic.sav" "$SAVEDIR/classic014.sav"
mk "$SAVEDIR/adv" -autoplaymode=adventure; cp "$SAVEDIR/adv/u/adv0.sav" "$SAVEDIR/base.sav"
mkdir -p "$SAVEDIR/advset" && cp "$SAVEDIR/classic1414.sav" "$SAVEDIR/advset/classic.sav"
python -I "$T/craft_saves.py" "$SAVEDIR/base.sav" "$SAVEDIR/advset" 0:Red:7 1:Two:9 2:Scorp:15,6:1:3725.5:7 3:Trap:15,15,9:2 4:Ankh:15,15,15,15,15,3:5
cp "$SAVEDIR/base.sav" "$SAVEDIR/bad/adv0.sav" && python -I "$T/craft_saves.py" "$SAVEDIR/base.sav" "$SAVEDIR/bad" 0:Old:7 bad:0
run() {  # <scenario> <saves folder or empty> <saved classic game or empty>
	local n="$1" u="$OUT/$1.user"
	rm -rf "$u" "$OUT/$n"; mkdir -p "$u" "$OUT/$n/port"
	python -I "$T/p2.py" make "$T/scenarios/$n.txt" "$n" "$OUT" > /dev/null || return 1
	[ -z "$2" ] || cp "$2"/*.sav "$u/"
	[ -z "$3" ] || cp "$3" "$u/classic.sav"
	"$EXE" -userdir="$u" -fast -seed=21 -script="$OUT/$n.port.txt" > "$OUT/$n.log" 2>&1
	rm -rf "$u"
}
run menus "" ""
run clsgo "" ""
run tut "" ""
run advplay "" ""
run ankhmsg "" ""
run late "" "$SAVEDIR/classic1414.sav"
run pyr "" "$SAVEDIR/classic014.sav"
run advsaves "$SAVEDIR/advset" ""
run badsave "$SAVEDIR/bad" ""
rm -rf "$SAVEDIR"
for n in menus clsgo tut advplay ankhmsg late pyr advsaves badsave; do echo "$n: $(ls "$OUT/$n/port" | wc -l) shots"; done
