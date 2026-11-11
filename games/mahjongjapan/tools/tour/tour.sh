#!/usr/bin/env bash
# A screen tour of one Mahjong port (both games: the menu and its animations, the player name box, the map, options,
# help, the classic list, the quit question, a board, the escape stage): the same script and clock for every exe given,
# 20 screenshots, so that two builds of a game can be compared byte for byte. -fast runs on the virtual clock.
#   build/native/rundesk.exe bash games/mahjongjapan/tools/tour/tour.sh <japan|china> <exe> <out dir>
# No click reaches a link: the clicks are the adventure's button, the map's PLAY and a tip's OK.
set -u
game="$1"; exe="$2"; out="$3"
case "$game" in
	japan) name=mahjongjapan; adv="400 270"; play="715 573"; ok="400 407" ;;
	china) name=mahjongchina; adv="400 262"; play="725 565"; ok="400 407" ;;
esac
rm -rf "$out"; mkdir -p "$out/shots" "$out/user"
S="$out/shots"
t=0; n=0
at() { t=$((t + $1)); shift; echo "$t $*"; }          # at <ms after the last line> <command>
shot() { n=$((n + 1)); at "$1" screenshot "$(printf '%s/%02d_%s.png' "$S" $n "$2")"; }
{
	shot 300 menu_a; shot 600 menu_b; shot 600 menu_c
	at 100 move $adv; shot 100 menu_hover
	at 100 click $adv; shot 400 name_dialog
	at 100 char T; at 5 char e; at 5 char s; at 5 char t; shot 85 name_typed
	at 50 key LEFT; at 10 key BACK; at 10 char x; shot 30 name_edit
	at 50 key RETURN; shot 1250 map
	at 100 goto menu; at 900 goto options; shot 900 options
	at 100 goto menu; at 900 goto help; shot 900 help
	at 100 goto menu; at 900 goto classic; shot 900 classic
	at 100 goto menu; shot 900 menu_again
	at 100 key ESCAPE; shot 600 menu_escape
	at 100 key ESCAPE; shot 300 menu_after_escape
	at 100 goto map; shot 900 map2
	at 100 click $play; at 400 click $ok; shot 300 board
	at 100 move 300 300; at 100 move 420 330; shot 100 board_hover
	at 100 key ESCAPE; shot 400 board_escape
	at 100 goto escape; shot 900 escape
	at 100 goto map; shot 900 map3
	at 100 quit
} > "$out/script.txt"
timeout 300 "$exe" -gamedir=assets/$name -userdir="$out/user" -fast -logos -script="$out/script.txt" \
	> "$out/stdout.txt" 2> "$out/stderr.txt"
echo "$name: $(ls "$S" | wc -l) shots"
