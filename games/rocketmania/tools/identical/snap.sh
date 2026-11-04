#!/bin/bash
# snap.sh <case dir> <game seed> <update N>: both programs replay the case's input up to update N, quit there and so save the game
# in progress (saved games: the port's userdir, the original's Players folder); prints whether the two saved games are the
# same file. The port's script line "u ..." is the original's "at u-1" (toorig.py), the port quits in update N+1, the
# original after its input at counter N.
source "$(dirname "$0")/common.sh"
D="$1"; GS="$2"; N="$3"
S="$D/snap$N"; mkdir -p "$S"
awk -v n="$N" '$1 <= n + 1 && $2 != "quit"' "$D/script.txt" > "$S/port.txt"
echo "$((N + 1)) quit" >> "$S/port.txt"
python "$(dirname "$0")/toorig.py" "$S/port.txt" -1 | grep -v '^quit' > "$S/orig.txt"
echo "at $N move 0 0" >> "$S/orig.txt"; echo quit >> "$S/orig.txt"
rm -rf "$S/user"
"$RUNDESK" "$(win build/native/rocketmania.exe)" -userdir="$(win "$S/user")" -fast -drawevery -seed=$GS -script="$(win "$S/port.txt")" > "$S/port.out" 2>&1
"$(dirname "$0")/orig.sh" "$S/orig.txt" $GS 60 '0x4f6850' > "$S/orig.log" 2>&1
ls "$S/user"/Players/save?.dat "$OG_GAME"/Players/save?.dat 2>&1 | head
