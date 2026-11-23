#!/bin/bash
# case.sh <name> <seed> <bot moves> [playbymove options...]: one comparison.
# 1. The port's bot plays from a first launch (an empty user folder) for that many moves by tools/playbymove (held,
#    -fast); the framework writes its input at the updates that ran it (-recordlive), and record.py makes that the
#    case's input file (build/identical/<name>/input.txt).
# 2. The port plays that file again as real input (-realinput, -fast) and writes its state log (state.txt);
#    follows.py checks that the replay is the bot's game.
# 3. The original plays the same input (toorig.py) on the same seed, watching four values a run, and cmp.py compares
#    each with the port's log, change by change with the frame each came at. A last run peeks the random state at
#    every update, and randseq.py compares the order of its values.
# FIELDS=<name,...> picks the values; INPUT=<file> plays a given input file instead of recording one (step 1 skipped);
# RECORD_ONLY=1 stops after step 2 (shots.sh takes the input and the state log from there).
# Prints one line per value; exits 1 when one differs.
source "$(dirname "$0")/common.sh"
N="$1"; SEED="$2"; MOVES="$3"; shift 3
D="$OUT/$N"
mkdir -p "$D"; rm -f "$D"/orig*.log "$D/orig_pizzas.xml"
# PLAYER_DIR=<user folder>: both sides start with its player (the port and the bot from copies of the folder, the
# original from its registry.ini as a .reg); the pizzas the run saves are compared at the end
if [ -n "$PLAYER_DIR" ]; then
	python -I "$TOOLS/ini2reg.py" "$PLAYER_DIR/registry.ini" > "$D/user.reg" || exit 1
	export REG="$D/user.reg"
	rm -rf "$D/port/user"; mkdir -p "$D/port"; cp -r "$PLAYER_DIR" "$D/port/user"
	export KEEP_USER=1
fi
if [ -n "$INPUT" ]; then
	cp "$INPUT" "$D/input.txt"
else
	rm -rf "$D/bot"; mkdir -p "$D/bot"
	if [ -n "$PLAYER_DIR" ]; then cp -r "$PLAYER_DIR" "$D/bot/user"; else mkdir -p "$D/bot/user"; fi
	timeout 3600 python -I tools/playbymove/playbymove.py --dir "$D/bot/run" --steps "$MOVES" "$@" \
		build/native/pizzafrenzy.exe -gamedir=assets/pizzafrenzy -userdir="$D/bot/user" -seed="$SEED" \
		-recordlive="$D/bot/live.txt" -statelog="$D/bot/state.txt" > "$D/bot/out.txt" 2>&1
	# the framework's record of the input at the updates that ran it (-recordlive); the bot's own lines are comments
	python -I "$TOOLS/record.py" "$D/bot/run/driver.txt" --live "$D/bot/live.txt" > "$D/input.txt"
fi
"$TOOLS/port.sh" "$D/port" -seed="$SEED" -realinput="$(win "$D/input.txt")" -statelog="$(win "$D/state.txt")" \
	> /dev/null
# the replay should be the bot's game: their random states' values in the same order
[ -f "$D/bot/state.txt" ] && python -I "$TOOLS/follows.py" "$D/bot/state.txt" "$D/state.txt"
# RECORD_ONLY=1 stops here: the input and the port's state log, for shots.sh
[ -n "$RECORD_ONLY" ] && exit 0
python -I "$TOOLS/toorig.py" "$D/input.txt" > "$D/orig.txt" || exit 1
QUIT=$(tail -1 "$D/input.txt" | cut -d" " -f1)
# the game's values are compared from the frame the port's first level starts (the stats are made)
export MODE_FROM=$(awk '$2 == "score" && $3 != "-" { print $1; exit }' "$D/state.txt")
# a case with no level (the editor's) compares from its first frame
[ -n "$MODE_FROM" ] || unset MODE_FROM

# the original's game is the object whose address is at 0x532900; its stats at +0x120, its mode's game at +0x110;
# rand's state is mirrored at 0x541a04 (setup.sh)
G='[0x532900]'; ST="[$G+0x120]"; M="[$G+0x110]"
declare -A EX=(
	[rand]=0x541a04 [state]="[$G+0x30]" [city]="$G+0x4c" [level]="$G+0x48"
	[score]="$ST+0xc" [elapsed]="$ST+0x10" [delivered]="$ST+0x20" [ordered]="$ST+0x24" [happiness]="$ST+0x28"
	[served]="$ST+0x2c" [tipmoney]="$ST+0x34" [waves]="$ST+0x3c" [combo]="$ST+0x40"
	[wavetimer]="$M+0xd8" [spawntimer]="$M+0xdc" [deliverytimer]="$M+0x114" [customersleft]="$M+0xe8"
	[ordersbegun]="$M+0xec" [streak]="$M+0xf0")
ALL=(state city level score elapsed delivered ordered happiness served tipmoney waves combo wavetimer spawntimer
	deliverytimer customersleft ordersbegun streak)
if [ -n "$FIELDS" ]; then IFS=, read -ra NAMES <<< "${FIELDS//rand/}"; else NAMES=("${ALL[@]}"); fi
NAMES=($(printf "%s\n" "${NAMES[@]}" | grep .))
# origrun stops a run at its timeout (seconds); the virtual clock runs about a hundred frames a second
TO=$((QUIT / 50 + 60))
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	E=(); for n in "${NAMES[@]:i:4}"; do E+=("${EX[$n]}"); done
	"$TOOLS/orig.sh" "$D/orig.txt" "$SEED" "$TO" "${E[@]}" > "$D/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${NAMES[*]:i:4}")"
	python -I "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "$names" $((QUIT - 2)) || st=1
done
# the random state is peeked at every update (a watch can miss a value) and compared by the order of its values
if [ -z "$FIELDS" ] || [[ ",$FIELDS," == *,rand,* ]]; then
	awk -v q="$QUIT" 'BEGIN { k = 1 } $1 == "at" { while (k <= $2 + 0 && k <= q) print "at " k++ " peek 0x541a04" }
		{ print }' "$D/orig.txt" > "$D/orig_rand.txt"
	PIZZAS="$D/orig_pizzas.xml" "$TOOLS/orig.sh" "$D/orig_rand.txt" "$SEED" "$TO" > "$D/orig_rand.log" 2>&1
	python -I "$TOOLS/randseq.py" "$D/state.txt" "$D/orig_rand.log" || st=1
fi
# the pizzas both sides saved (the original's beside its exe, the port's in its user folder)
if [ -f "$D/orig_pizzas.xml" ] || [ -f "$D/port/user/userPizzas.xml" ]; then
	if cmp -s <(tr -d '\r' < "$D/orig_pizzas.xml") <(tr -d '\r' < "$D/port/user/userPizzas.xml"); then
		echo "pizzas      same"
	else
		echo "pizzas      DIFFER (orig_pizzas.xml and port/user/userPizzas.xml)"; st=1
	fi
fi
exit $st
