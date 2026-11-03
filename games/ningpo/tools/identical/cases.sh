#!/bin/bash
# cases.sh <case> <script>: writes a case's port script. The mouse cases are built in stages: the port plays the script
# so far to deal (or shuffle) the board, and mouse.py adds the clicks that match pairs on it.
#   cheat7    seed 7: the cheat's matches until no pair is left, End Game, the name, the best times (tools/check.sh's)
#   mouse23   seed 23: mouse matches (with wrong pairs), hints, undos, show moves, pause, shuffle, more matches, the
#             idle hint after 45 s, a new game from the button and matches on its deal
#   layouts   seed 11: every layout in turn: a deal, a hint, two cheat matches, an undo, a shuffle, a new game
#   won39     seed 39: a game won with the cheat on layout 0, the name, the best times, a second game ended with
#             tiles left (End Game), whose entry goes second, and the title
source "$(dirname "$0")/common.sh"
CASE="$1"; S="$2"
TMP="$OUT/cases-tmp"; mkdir -p "$TMP"

stage() {  # <seed>: plays the script so far (with a quit) so that its last board is in $TMP/stage.log
	{ cat "$S"; echo "$(($(tail -1 "$S" | cut -d' ' -f1) + 50)) quit"; } > "$TMP/stage.txt"
	"$TOOLS/port.sh" "$TMP/stage.txt" "$1" "$TMP/stage.log"
}
cheat() {  # <first update> <count>: the cheat's match key every 20 updates
	local u
	for ((u = $1; u < $1 + 20 * $2; u += 20)); do echo "$u char m"; done >> "$S"
}

case "$CASE" in
cheat7)
	{
		printf '%s\n' "150 click 320 425" "350 click 340 440" "500 key BACK" "510 char q" "520 key BACK"
		for ((u = 550; u <= 2030; u += 20)); do echo "$u char m"; done
		printf '%s\n' "2150 click 385 368" "2350 click 270 345"
		for ((u = 2500; u < 2530; u += 2)); do echo "$u key BACK"; done
		printf '%s\n' "2550 char T" "2555 char e" "2560 char s" "2565 char t" "2630 key RETURN" "2820 click 495 435" "3020 quit"
	} > "$S"
	;;
mouse23)
	printf '%s\n' "150 click 320 425" "350 click 340 440" > "$S"
	stage 23
	u=$(python "$TOOLS/mouse.py" "$TMP/stage.log" "$S" 500 60 10 2)
	printf '%s\n' "$((u + 50)) click 50 265" "$((u + 450)) click 50 265" "$((u + 800)) click 60 370" "$((u + 900)) click 60 370" \
		"$((u + 1000)) click 50 300" "$((u + 1100)) click 50 335" "$((u + 1400)) click 50 335" "$((u + 1600)) click 50 405" >> "$S"
	stage 23
	u=$(python "$TOOLS/mouse.py" "$TMP/stage.log" "$S" $((u + 1800)) 60 12)
	# 50 seconds without a match: the hint after 45 (NingPo::tick)
	printf '%s\n' "$((u + 5000)) click 47 115" "$((u + 5070)) click 316 328" "$((u + 5200)) click 340 440" >> "$S"
	stage 23
	u=$(python "$TOOLS/mouse.py" "$TMP/stage.log" "$S" $((u + 5400)) 60 6)
	# back to the title before the quit (a close during a game asks first)
	printf '%s\n' "$((u + 100)) click 47 115" "$((u + 170)) click 316 328" "$((u + 300)) quit" >> "$S"
	;;
layouts)
	printf '%s\n' "150 click 320 425" > "$S"
	u=350
	for ((l = 0; l < 18; l++)); do
		printf '%s\n' "$u click 340 440" "$((u + 150)) click 50 265" >> "$S"
		# the cheat once (Backspace q Backspace toggles it)
		[ $l -gt 0 ] || printf '%s\n' "$((u + 200)) key BACK" "$((u + 210)) char q" "$((u + 220)) key BACK" >> "$S"
		cheat $((u + 250)) 2
		printf '%s\n' "$((u + 330)) click 60 370" "$((u + 400)) click 50 405" "$((u + 550)) click 47 115" \
			"$((u + 620)) click 316 328" "$((u + 700)) click 484 161" >> "$S"
		u=$((u + 800))
	done
	echo "$u quit" >> "$S"
	;;
won39)
	printf '%s\n' "150 click 320 425" "350 click 340 440" "500 key BACK" "510 char q" "520 key BACK" > "$S"
	cheat 550 72
	u=$((550 + 20 * 72 + 300))
	printf '%s\n' "$u click 300 345" >> "$S"
	for ((i = 0; i < 15; i++)); do echo "$((u + 150 + 2 * i)) key BACK"; done >> "$S"
	printf '%s\n' "$((u + 200)) char A" "$((u + 205)) char n" "$((u + 210)) char n" "$((u + 280)) key RETURN" \
		"$((u + 450)) click 495 435" "$((u + 600)) click 320 425" "$((u + 800)) click 340 440" >> "$S"
	u=$((u + 900))
	cheat $((u + 50)) 72  # the cheat is still on (Backspace q Backspace toggles it: NingPo::onChar)
	# this deal runs out of pairs: End Game in the no-moves dialog, then the continue arrow
	u=$((u + 50 + 20 * 72 + 300))
	printf '%s\n' "$u click 385 368" "$((u + 150)) click 270 345" >> "$S"
	u=$((u + 150))
	for ((i = 0; i < 15; i++)); do echo "$((u + 150 + 2 * i)) key BACK"; done >> "$S"
	printf '%s\n' "$((u + 200)) char B" "$((u + 205)) char o" "$((u + 280)) key RETURN" "$((u + 450)) click 495 435" \
		"$((u + 650)) quit" >> "$S"
	;;
*)
	echo "cases.sh: no case $CASE" >&2; exit 1
	;;
esac
