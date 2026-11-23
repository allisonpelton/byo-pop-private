#!/bin/bash
# Bejeweled 3's check suite (tools/check.sh's suite_bejeweled3 calls it): <out dir> [exe]
#
# Two scripts of test commands beside this file, each run twice. suite_test.txt: the menu, a seeded Classic game played by the
# bot for 150 moves, a board with a Flame Gem, a Star Gem and a Hypercube put on it, then a board with no move and the
# game-over dialog. suite_modes.txt: a seeded Zen game (bot, 150 moves) and a board with no move (Zen has no game over: a
# Hypercube is made), then a Lightning game played by the bot through two Last Hurrahs to its end. The two runs' state lines and
# their screenshots (all but the menu's) must be the same, and the state lines must match the frozen hashes below (update one, with
# the reason in the commit, when a translation changes the game's state).
# Both scripts start with `name Tester` (the welcome dialog's own OK with a typed name), so the end dialogs can open; the hashes
# changed once for that (the extra OK line and the endscreen key) and are frozen again.
set -u
OUT="$1"
EXE="${2:-build/native/bejeweled3.exe}"
HERE="$(cd "$(dirname "$0")" && pwd)"
FROZEN="51086c43ed2a"
FROZEN_MODES="873d262bb577"
SHOTS="classic play specials gameover"  # compared; menu.png is only kept to look at: the menu opens after the
# loading, which runs at the machine's speed, so its dialog and balls stand at a different point of their animation each run
mkdir -p "$OUT"
fail=""

run() {  # <name> [script]: the script (suite_test.txt) into $OUT/<name>; prints the exe's exit status
	local dir="$OUT/$1" e=0 windir
	rm -rf "$dir"; mkdir -p "$dir/user"
	windir="$(cygpath -m "$(cd "$dir" && pwd)")"
	sed "s|@DIR@|$windir|" "$HERE/${2:-suite_test.txt}" > "$dir/script.txt"
	timeout 240 "$EXE" -gamedir=assets/bejeweled3 -userdir="$dir/user" -fast -script="$dir/script.txt" > "$dir/stdout.txt" 2>&1 || e=$?
	grep -oE '(OK|ERROR|STATE) .*' "$dir/stdout.txt" > "$dir/states.txt"
	echo "$e"
}

e="$(run game)"; [ "$e" -eq 0 ] || fail="$fail exit($e)"
grep -q '^ERROR' "$OUT/game/states.txt" && fail="$fail command($(grep -m1 '^ERROR' "$OUT/game/states.txt" | cut -c1-80))"
[ "$(grep -c '^STATE ' "$OUT/game/states.txt")" = 3 ] || fail="$fail statelines"
grep '^STATE ' "$OUT/game/states.txt" | tail -n 1 | grep -q '"gameover":1' || fail="$fail nogameover"
grep '^STATE ' "$OUT/game/states.txt" | tail -n 1 | grep -q '"endscreen":1' || fail="$fail noendscreen"
e="$(run rerun)"; [ "$e" -eq 0 ] || fail="$fail rerun-exit($e)"
cmp -s "$OUT/game/states.txt" "$OUT/rerun/states.txt" || fail="$fail rerun-states"
for s in $SHOTS; do
	[ -s "$OUT/game/$s.png" ] || { fail="$fail noshot-$s"; continue; }
	cmp -s "$OUT/game/$s.png" "$OUT/rerun/$s.png" || fail="$fail rerun-$s"
done
e="$(run modes suite_modes.txt)"; [ "$e" -eq 0 ] || fail="$fail modes-exit($e)"
grep -q '^ERROR' "$OUT/modes/states.txt" && fail="$fail modes-command($(grep -m1 '^ERROR' "$OUT/modes/states.txt" | cut -c1-80))"
[ "$(grep -c '^STATE ' "$OUT/modes/states.txt")" = 5 ] || fail="$fail modes-statelines"
grep '^STATE ' "$OUT/modes/states.txt" | sed -n 2p | grep -q '"gameover":0' || fail="$fail zen-gameover"
grep '^STATE ' "$OUT/modes/states.txt" | tail -n 1 | grep -q '"gameover":1' || fail="$fail lightning-nogameover"
grep '^STATE ' "$OUT/modes/states.txt" | tail -n 1 | grep -q '"endscreen":1' || fail="$fail lightning-noendscreen"
e="$(run modesrerun suite_modes.txt)"; [ "$e" -eq 0 ] || fail="$fail modesrerun-exit($e)"
cmp -s "$OUT/modes/states.txt" "$OUT/modesrerun/states.txt" || fail="$fail modesrerun-states"
for s in zen lightning; do
	[ -s "$OUT/modes/$s.png" ] || { fail="$fail noshot-$s"; continue; }
	cmp -s "$OUT/modes/$s.png" "$OUT/modesrerun/$s.png" || fail="$fail modesrerun-$s"
done
# The secret modes: a seeded bot game of Butterflies, one of Poker, one of Ice Storm (its bot stops after 30 moves and the ice
# reaches the top) and one of Diamond Mine (the bot's 150 moves, the dirt of the first six rows cleared by `dig clear` three times so the grid scrolls, the time's end), each to its game over and the end dialog (suite_butterflies.txt, suite_poker.txt, suite_icestorm.txt), each run twice: the state lines (the board, the mode's own keys) and the shots must be the same. Their states are
# not frozen by a hash of their own: the mode's logic is checked by the state lines' contents (a game over, the end dialog) and the rerun.
for m in butterflies poker icestorm diamondmine; do
	e="$(run $m suite_$m.txt)"; [ "$e" -eq 0 ] || fail="$fail $m-exit($e)"
	grep -q '^ERROR' "$OUT/$m/states.txt" && fail="$fail $m-command($(grep -m1 '^ERROR' "$OUT/$m/states.txt" | cut -c1-80))"
	[ "$(grep -c '^STATE ' "$OUT/$m/states.txt")" = 3 ] || fail="$fail $m-statelines"
	grep '^STATE ' "$OUT/$m/states.txt" | sed -n 2p | grep -q '"gameover":0' || fail="$fail $m-early-gameover"
	grep '^STATE ' "$OUT/$m/states.txt" | tail -n 1 | grep -q '"gameover":1' || fail="$fail $m-nogameover"
	grep '^STATE ' "$OUT/$m/states.txt" | tail -n 1 | grep -q '"endscreen":1' || fail="$fail $m-noendscreen"
	e="$(run ${m}rerun suite_$m.txt)"; [ "$e" -eq 0 ] || fail="$fail ${m}rerun-exit($e)"
	cmp -s "$OUT/$m/states.txt" "$OUT/${m}rerun/states.txt" || fail="$fail ${m}rerun-states"
	for s in $m ${m}_end; do
		[ -s "$OUT/$m/$s.png" ] || { fail="$fail noshot-$s"; continue; }
		cmp -s "$OUT/$m/$s.png" "$OUT/${m}rerun/$s.png" || fail="$fail ${m}rerun-$s"
	done
done
# Quest (suite_quest.txt): the quest map (section 0: eight buttons, none done), the quest at index 3 (Stratamax: Move Limit, Clear Gems 120 gems in 20 moves) started by
# a click on its button and played by the bot to its end (a loss: the map comes back over the board), the same quest with 40 gems (`quest 3 Gems=40`: a win, which marks it
# done in the profile), then three more quests marked done (`quest done`) so the second section opens and NEXT shows it. Each run twice: states and shots the same.
e="$(run quest suite_quest.txt)"; [ "$e" -eq 0 ] || fail="$fail quest-exit($e)"
grep -q '^ERROR' "$OUT/quest/states.txt" && fail="$fail quest-command($(grep -m1 '^ERROR' "$OUT/quest/states.txt" | cut -c1-80))"
[ "$(grep -c '^STATE ' "$OUT/quest/states.txt")" = 5 ] || fail="$fail quest-statelines"
qs() { grep '^STATE ' "$OUT/quest/states.txt" | sed -n "$1p"; }
qs 1 | grep -q '"questmenu":1,"questsection":0,.*"questbuttons":8' || fail="$fail quest-map"
qs 2 | grep -q '"goal":"CLEAR GEMS".*"questmenu":0,"movesleft":20' || fail="$fail quest-start"
qs 3 | grep -q '"questmenu":1.*"gameover":1.*"questwon":0' || fail="$fail quest-loss"
qs 3 | grep -q '"quests":"0000000000000000000000000000000000000000"' || fail="$fail quest-loss-marked"
qs 4 | grep -q '"questmenu":1.*"quests":"0001000000000000000000000000000000000000".*"questwon":1' || fail="$fail quest-win"
qs 5 | grep -q '"questsection":1,"quests":"1111' || fail="$fail quest-section"
qs 5 | grep -q '"sectionstate":\[2,1,0,0,0\]' || fail="$fail quest-section-state"
e="$(run questrerun suite_quest.txt)"; [ "$e" -eq 0 ] || fail="$fail questrerun-exit($e)"
cmp -s "$OUT/quest/states.txt" "$OUT/questrerun/states.txt" || fail="$fail questrerun-states"
for s in quest_map quest_play quest_lost quest_won quest_map2; do
	[ -s "$OUT/quest/$s.png" ] || { fail="$fail noshot-$s"; continue; }
	cmp -s "$OUT/quest/$s.png" "$OUT/questrerun/$s.png" || fail="$fail questrerun-$s"
done
# Quest, the second part (suite_quest2.txt): ten quests of quest.cfg started in a row, each played by the bot (the first nine with the setting changes the script names to
# make them short) and each ending in a state line: Avalanche (Filler, won), Balance (won), Alchemy as a Time Limit quest (the first of its repeated Challenge settings wins, as in the exe) with the Clear Gems goal (won), Stratamax as Bad Move Limit (lost
# by swaps that find no match), Time Bomb with the Clear Gems goal (won), Time Bomb II in real time (lost: a bomb ran out), Butterflies (won), Poker (started and played: the quest's
# goal is a score to reach in a number of hands), Ice Storm (won), Gold Rush (won by a treasure). Each run twice: states and shots the same.
e="$(run quest2 suite_quest2.txt)"; [ "$e" -eq 0 ] || fail="$fail quest2-exit($e)"
grep -q '^ERROR' "$OUT/quest2/states.txt" && fail="$fail quest2-command($(grep -m1 '^ERROR' "$OUT/quest2/states.txt" | cut -c1-80))"
[ "$(grep -c '^STATE ' "$OUT/quest2/states.txt")" = 10 ] || fail="$fail quest2-statelines"
q2() { grep '^STATE ' "$OUT/quest2/states.txt" | sed -n "$1p"; }
q2 1 | grep -q '"quest":5,"questwon":1.*"credits":0,"droppermove":5' || fail="$fail quest2-filler"
q2 2 | grep -q '"quest":4,"questwon":1.*"red":2,"blue":2' || fail="$fail quest2-balance"
q2 3 | grep -q '"quest":2,"questwon":1' || fail="$fail quest2-timelimit"
q2 4 | grep -q '"gameover":1.*"quest":3,"questwon":0.*"badallowed":2,"badused":[2-9]' || fail="$fail quest2-badmove"
q2 5 | grep -q '"quest":7,"questwon":1.*"bombstarted":1,"realtime":0' || fail="$fail quest2-bomb"
q2 6 | grep -q '"gameover":1.*"quest":14,"questwon":0.*"realtime":1' || fail="$fail quest2-bombrt"
q2 7 | grep -q '"mode":"butterflies".*"quest":0,"questwon":1' || fail="$fail quest2-butterflies"
q2 8 | grep -q '"mode":"poker".*"gameover":0.*"quest":8,"questwon":0.*"handsleft":[0-9]' || fail="$fail quest2-poker"
q2 9 | grep -q '"mode":"icestorm".*"quest":9,"questwon":1' || fail="$fail quest2-ice"
q2 10 | grep -q '"mode":"diamondmine".*"quest":1,"questwon":1.*"digtreasure":1' || fail="$fail quest2-dig"
e="$(run quest2rerun suite_quest2.txt)"; [ "$e" -eq 0 ] || fail="$fail quest2rerun-exit($e)"
cmp -s "$OUT/quest2/states.txt" "$OUT/quest2rerun/states.txt" || fail="$fail quest2rerun-states"
for s in filler balance nolose badmove bomb bombrt butterflies poker ice dig; do
	[ -s "$OUT/quest2/quest2_$s.png" ] || { fail="$fail noshot-quest2_$s"; continue; }
	cmp -s "$OUT/quest2/quest2_$s.png" "$OUT/quest2rerun/quest2_$s.png" || fail="$fail quest2rerun-$s"
done
# Quest, the third part (suite_quest3.txt): eleven short games, a win and a loss for most goals: Hidden Objects (Buried Treasure: three flames uncover the three objects; Sandstorm
# lost on time), Wall Blast (a flame breaks a wall of nine; Wall Blast II lost on time), Gold Lines (Alchemy won; Alchemy II lost on its moves), Gold Lines Flip (won), Clear Bombs
# (Time Bomb won; Time Bomb II lost: a bomb ran out) and the Poker quest (won on its score; lost on its hands). Each run twice: states and shots the same.
e="$(run quest3 suite_quest3.txt)"; [ "$e" -eq 0 ] || fail="$fail quest3-exit($e)"
grep -q '^ERROR' "$OUT/quest3/states.txt" && fail="$fail quest3-command($(grep -m1 '^ERROR' "$OUT/quest3/states.txt" | cut -c1-80))"
[ "$(grep -c '^STATE ' "$OUT/quest3/states.txt")" = 11 ] || fail="$fail quest3-statelines"
q3() { grep '^STATE ' "$OUT/quest3/states.txt" | sed -n "$1p"; }
q3 1 | grep -q '"quest":6,"questwon":1.*"goal":"HIDDEN OBJECTS".*"hiddenobjects":"6.4.1.3f;1.2.2.3f;5.0.2.2f","hiddenfound":3' || fail="$fail quest3-hidden"
q3 2 | grep -q '"gameover":1.*"quest":11,"questwon":0.*"hiddenfound":0' || fail="$fail quest3-hiddenlost"
q3 3 | grep -q '"quest":17,"questwon":1.*"goal":"WALL BLAST","goalsize":9,"goalcount":9,.*"wallcount":9,"wallleft":0' || fail="$fail quest3-wall"
q3 4 | grep -q '"gameover":1.*"quest":35,"questwon":0.*"wallleft":36' || fail="$fail quest3-walllost"
q3 5 | grep -q '"quest":2,"questwon":1.*"goal":"GOLD LINES".*"squares":10,"gold":[1-9]' || fail="$fail quest3-gold"
q3 6 | grep -q '"gameover":1.*"quest":16,"questwon":0.*"squares":64,"gold":[0-9]' || fail="$fail quest3-goldlost"
q3 7 | grep -q '"quest":26,"questwon":1.*"goal":"GOLD LINES FLIP".*"squares":8,"gold":[8-9]' || fail="$fail quest3-flip"
q3 8 | grep -q '"quest":7,"questwon":1.*"goal":"CLEAR BOMBS".*"bombgoal":1.*"bombstarted":1' || fail="$fail quest3-bombs"
q3 9 | grep -q '"gameover":1.*"quest":14,"questwon":0.*"bombgoal":50' || fail="$fail quest3-bombslost"
q3 10 | grep -q '"mode":"poker".*"quest":8,"questwon":1.*"pokergoal":40' || fail="$fail quest3-poker"
q3 11 | grep -q '"mode":"poker".*"gameover":1.*"quest":8,"questwon":0.*"handsleft":-1,"pokergoal":100000' || fail="$fail quest3-pokerlost"
e="$(run quest3rerun suite_quest3.txt)"; [ "$e" -eq 0 ] || fail="$fail quest3rerun-exit($e)"
cmp -s "$OUT/quest3/states.txt" "$OUT/quest3rerun/states.txt" || fail="$fail quest3rerun-states"
for s in hidden wall gold flip bombs poker; do
	[ -s "$OUT/quest3/quest3_$s.png" ] || { fail="$fail noshot-quest3_$s"; continue; }
	cmp -s "$OUT/quest3/quest3_$s.png" "$OUT/quest3rerun/quest3_$s.png" || fail="$fail quest3rerun-$s"
done
msh="$(sha256sum "$OUT/modes/states.txt" | cut -c1-12)"
[ -z "$FROZEN_MODES" ] || [ "$msh" = "$FROZEN_MODES" ] || fail="$fail MODE STATES CHANGED($msh; frozen $FROZEN_MODES)"
sh="$(sha256sum "$OUT/game/states.txt" | cut -c1-12)"
[ -z "$FROZEN" ] || [ "$sh" = "$FROZEN" ] || fail="$fail STATES CHANGED($sh; frozen $FROZEN)"
if [ -z "$fail" ]; then
	echo "bejeweled3: menu, a seeded Classic game (bot, 150 moves), the specials and the game over; Zen and Lightning games (bot); rerun identical (states and 6 shots); Butterflies, Poker, Ice Storm and Diamond Mine bot games to their game over (rerun identical, states and 6 shots); the quest map and a Quest game lost and won by the bot (rerun identical, states and 5 shots); ten more quests, one for each board, played by the bot (rerun identical, states and 10 shots); eleven games of the goals of the third part (rerun identical, states and 6 shots); states $sh, modes $msh"
	exit 0
fi
echo "bejeweled3: FAILED:$fail"
exit 1
