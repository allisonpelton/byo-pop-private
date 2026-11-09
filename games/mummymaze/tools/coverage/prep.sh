#!/bin/bash
# prep.sh [late]: the origrun scripts of the coverage runs that are the port's bot's input (identical.py script), and the saved game of
# the late-game run. Run it under rundesk from the worktree, one port process at a time:
#   build/native/rundesk.exe bash games/mummymaze/tools/coverage/prep.sh
# The scripts go to build/coverage/mummymaze/scripts/<name>.script.txt (their last update is in <name>.last), the port's logs
# beside them. run_all.sh feeds them to run.sh.
R="$(pwd -W)"; T="$R/games/mummymaze/tools/identical"; D="$R/build/coverage/mummymaze/scripts"; mkdir -p "$D"
bot() {  # bot <name> <seed> <bot arguments...>
	local n="$1" seed="$2"; shift 2
	bash "$T/run_port.sh" "$R" "$D/$n.port.txt" -seed="$seed" -autoplay="$seed" "$@"
	python -I "$T/identical.py" script "$D/$n.port.txt" "$D/$n.script.txt" --offset 2301
	grep -o "^at [0-9]*" "$D/$n.script.txt" | tail -1 | cut -d" " -f2 > "$D/$n.last"
	echo "$n: last update $(cat "$D/$n.last")"
}
# a classic game for 16000 updates (the phase-one case), a short one that ends with Abandon Hope, the name box and the table,
# the game that starts in pyramid 3 (the tips), the tutorial, an adventure
if [ "$1" != late ]; then
bot classic21 21 -autoplaygames=1 -autoplaylevels=250 -autoplaycap=16000
bot classic8 7 -autoplaygames=1 -autoplaylevels=8 -autoplaycap=60000
bot tips 5 -autoplaygames=1 -autoplaystart=3,0 -autoplaylevels=4 -autoplaycap=60000
bot tutorial 3 -autoplaymode=tutorial -autoplaycap=100000
bot adventure 5 -autoplaymode=adventure -autoplaygames=1 -autoplaylevels=400 -autoplaycap=60000
fi
# the late game: a saved game in the last chamber of the last pyramid, continued to the victory screen, the name box and the
# table (resume.sh's way: the port writes the save, the original is given the file and answers its Continue question with
# the clicks the port's bot logs)
U="$D/late.user"; rm -rf "$U"; mkdir -p "$U"
"$R/build/native/mummymaze.exe" -userdir="$U" -fast -seed=21 -autoplay=21 -autoplaygames=1 -autoplaystart=14,14 -autoplaysave=300 -autoplaycap=100000 \
	-autoplaylog="$D/late.save.txt" > "$D/late.save.stdout" 2> "$D/late.save.stderr"
cp "$U/classic.sav" "$D/late.classic.sav"
USERDIR="$U" bash "$T/run_port.sh" "$R" "$D/late.port.txt" -seed=21 -autoplay=21 -autoplaygames=1 -autoplaycontinue
python -I "$T/identical.py" script "$D/late.port.txt" "$D/late.script.txt" --offset 2301
grep -o "^at [0-9]*" "$D/late.script.txt" | tail -1 | cut -d" " -f2 > "$D/late.last"
echo "late: last update $(cat "$D/late.last")"
