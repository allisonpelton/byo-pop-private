#!/bin/bash
# modal_test.sh <mahjongjapan|mahjongchina> <menu|open|control|controlfine> [K] [pairs file]: the check that a modal dialog (the mod menu) holds the
# game (MahjongApp::ModalOpen/ModalClose, docs in the game's notes, "The game under a modal dialog"). A new player, a board, the first solver pair
# clicked (its removal animates), then
#   control      no menu: the state at three game times, and the picture at each (build/modal/<game>-control)
#   controlfine  no menu: a picture at every update around two game times, to find the frames' phase
#   menu K       the menu opened at update 3470 and closed at 4600 + K; states and pictures after the close (K only moves the close)
#   open K       the menu opened at update 3466 + K and closed 1130 updates later: the game's first frame after the close is at one of the five
#                phases of the control's frames, and at the control's phase its picture must be the control's, pixel for pixel
# Runs -fast on the virtual clock (so a menu open for 1130 updates is 5.6 s of game time, with no wait); run it under build/native/rundesk.exe.
# Compare with pngdiff.py (two pictures) and pngmatch.py (one picture against every picture of controlfine); the states have "timer" and "score".
# The pairs file is the solver's, "x1 y1 x2 y2" a line (tools/check.sh writes it under build/logs/check/<game>/ as sol385.txt, sol1.txt).
cd "$(git rev-parse --show-toplevel)" || exit 1
G="$1"; V="$2"; K="${3:-0}"
d="build/modal/$G-$V$K"
if [ "$V" = control ]; then d="build/modal/$G-control"; fi
rm -rf "$d"; mkdir -p "$d/user"
if [ "$G" = mahjongjapan ]; then
	P="1400 click 400 270"; OK="2600 click 715 573"; SOL="${4:-build/logs/check/mahjongjapan/sol385.txt}"
else
	P="1400 click 400 262"; OK="2600 click 725 565"; SOL="${4:-build/logs/check/mahjongchina/sol1.txt}"
fi
read -r x1 y1 x2 y2 < "$SOL"
{
	printf '%s\n' "$P" "1720 char T" "1725 char e" "1730 char s" "1735 char t" "1760 key RETURN" "$OK" "3000 click 400 407" \
		"3320 state" "3400 click $x1 $y1" "3460 click $x2 $y2"
	if [ "$V" = controlfine ]; then
		for u in 3466 3467 3468 3469 3470 3471 3472 3473 3474 3475 3476 3486 3487 3488 3489 3490 3491 3492 3493 3494 3495 3496; do
			echo "$u screenshot $d/c$u.png"
		done
	elif [ "$V" = open ]; then
		O=$((3466 + K)); C=$((O + 1130))
		printf '%s\n' "$O modmenu open" "$((O + 5)) state" "$((C - 100)) state" "$((C - 99)) screenshot $d/menu.png" "$C modmenu close" \
			"$((C + 1)) state" "$((C + 2)) screenshot $d/a0.png" "$((C + 20)) state" "$((C + 21)) screenshot $d/a1.png"
	elif [ "$V" = menu ]; then
		C=$((4600 + K))
		printf '%s\n' "3470 modmenu open" "3475 state" "4500 state" "4501 screenshot $d/menu.png" "$C modmenu close" \
			"$((C + 1)) state" "$((C + 2)) screenshot $d/a0.png" "$((C + 20)) state" "$((C + 21)) screenshot $d/a1.png" \
			"$((C + 900)) state" "$((C + 901)) screenshot $d/a2.png"
	else
		printf '%s\n' "3471 state" "3472 screenshot $d/a0.png" "3490 state" "3491 screenshot $d/a1.png" "4370 state" "4371 screenshot $d/a2.png" \
			"4601 state" "4602 screenshot $d/late0.png"
	fi
	echo "6000 quit"
} > "$d/script.txt"
timeout 120 "build/native/$G.exe" -gamedir="assets/$G" -userdir="$d/user" -fast -logos -script="$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
echo "$G $V$K exit $?"
