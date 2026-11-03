#!/bin/bash
# case.sh <name> <mode> <bot seed> <game seed> <games> [extra port arguments]: one comparison. The port's bot plays
# <games> games of a mode (regular, strategy or speed) through the mouse (-autoplayui), its input is written as a script,
# the original plays the same input on the same game seed, and the original's values are compared with the port's state
# log (testhooks/Autoplay.cpp, -statelog), change by change with the update it came at. origrun watches four values at
# a time, so the original runs once per four values (NAMES=a,b,... runs only those; [meter]'s watch is there for a timed
# game, but the other modes' level lists never set the meter's start, so it holds leftover heap on both sides and is not
# compared). Prints the bot's last line and one line per value.
#   PORT_SCRIPT=<file>  play this script on both sides instead of the bot's (end it with a quit line, or the port runs on)
source "$(dirname "$0")/common.sh"
N="$1"; MODE="$2"; BS="$3"; GS="$4"; G="$5"; shift 5
D="$OUT/$N"
if [ -n "$PORT_SCRIPT" ]; then
	mkdir -p "$D"; cp "$PORT_SCRIPT" "$D/script.txt"
	"$TOOLS/port.sh" "$D" -seed=$GS -script="$(win "$D/script.txt")" -statelog="$D/state.txt" "$@" > /dev/null
else
	"$TOOLS/port.sh" "$D" -seed=$GS -autoplay=$BS -autoplayui -autoplaymode=$MODE -autoplaygames=$G -autoplaycap=80000 \
		-autoplaylog="$D/log.txt" -autoplayscript="$D/script.txt" -statelog="$D/state.txt" "$@" > /dev/null
	grep -E "over at|cap reached" "$D/log.txt" | tail -1
fi
python "$TOOLS/toorig.py" "$D/script.txt" -1 > "$D/orig.txt"

# the app is at 0x488710: its screen at +0xdc, its board at +0xe8; the board's field at +0x34c
B='[0x4887f8]'
F="[$B+0x34c]"
ALL=(screen mode rng draws score level blocksleft gameover levelcomplete ending next0 next1 fnext lowrow highrow
	pending floaters bestchain bestmatch totaltime leveltime spawndelay paused bonus queueindex busy lockfall lockspawn shifting)
declare -A EXPR=([screen]=0x4887ec [mode]=0x488700 [rng]=0x48cff4 [draws]=0x48cff8 [score]="$F+0x4" [level]="$B+0x35c"
	[blocksleft]="$B+0x360" [meter]="$B+0x364" [gameover]="$B+0x350" [levelcomplete]="$B+0x354" [ending]="$B+0x358"
	[next0]="$B+0x40" [next1]="$B+0x9c" [fnext]="$F+0x20" [lowrow]="$F+0x8dc" [highrow]="$F+0x8e0" [pending]="$F+0x8f4"
	[floaters]="$F+0x8fc" [bestchain]="$F+0x84" [bestmatch]="$F+0x8c" [totaltime]="$F+0x7c" [leveltime]="$F+0x80"
	[spawndelay]="$F+0x900" [paused]=0x4886f0 [bonus]=0x488704
	[queueindex]="$B+0x348" [busy]=0x4884a0 [lockfall]="$F+0xc" [lockspawn]="$F+0x10" [shifting]="$B+0x344")
# the high score table of the case's mode (four tables of five 0x1c-byte entries at 0x4884b0): its first four scores
# and the first four bytes of the top name. The run is killed at its timeout, so the original never writes
# Preferences.dat; the table is read in memory instead
HS=$((0x4884b0 + $(awk '$2 == "mode" { m = $3 } END { print m + 0 }' "$D/state.txt") * 5 * 0x1c))
for i in 0 1 2 3; do EXPR[hs$i]=$(printf '0x%x' $((HS + i * 0x1c + 0x14))); done
EXPR[hsname]=$(printf '0x%x' $HS)
ALL+=(hs0 hs1 hs2 hs3 hsname)
if [ -n "$NAMES" ]; then IFS=, read -r -a SEL <<< "$NAMES"; else SEL=("${ALL[@]}"); fi
# the virtual clock plays about 500 updates a second, fewer when the machine is busy (the timeout allows 250); the run
# ends at its timeout (the quit at the script's end reaches a game that asks first); the comparison stops 15 updates
# after the last input (the quit comes 20 after)
LAST=$(tail -1 "$D/script.txt" | cut -d' ' -f1)
# the first game's start: the field's score appears
FROM=$(awk '$2 == "score" && $3 != "-" { print $1; exit }' "$D/state.txt")
TO=$((LAST / 250 + 30)); [ $TO -gt 400 ] && TO=400
st=0
for ((i = 0; i < ${#SEL[@]}; i += 4)); do
	part=("${SEL[@]:i:4}"); ex=()
	for n in "${part[@]}"; do ex+=("${EXPR[$n]}"); done
	"$TOOLS/orig.sh" "$D/orig.txt" $GS $TO "${ex[@]}" > "$D/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${part[*]}")"
	python "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "$names" 1 $((LAST + 15)) ${FROM:-0} || st=1
done
exit $st
