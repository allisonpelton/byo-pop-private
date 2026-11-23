#!/bin/bash
# realinput.sh [mode ...]: Bejeweled 3's native real-input runs (CLAUDE.md "Real input before the final merge", docs/REAL_INPUT.md). For each mode
# (classic zen lightning quest butterflies poker icestorm diamondmine; none given: all) tools/playbymove/playbymove.py starts the port under
# rundesk with a fresh user folder and plays it by the `move` command: the new player's name typed as keys, PLAY, the mode's ball and (Quest) the
# quest's button on the map clicked as a hand does, then the bot's swaps as clicks, until the state says "over". Twice: windowed (the user
# folder's registry.ini has ScreenMode 0) and full screen (a new player's own start: the port opens full screen when no setting says
# otherwise). A third display, "alt", is the full screen start with Alt+Enter pressed at once (keys through SDL's queue, as a hand), so the
# game is played in the window it switches to; it runs for Classic only, after the others. REALINPUT_DISPLAYS="win", "full" or "alt" picks.
# One summary line a run:
#   <mode> <window|full screen>: <moves> moves, <end>
# Exit 0 when every run reached its end. Each run's folder is build/realinput/<mode>-<win|full> (the driver's transcript, the game's log, end.png
# and the user folder); delete build/realinput when the results are noted. One game at a time (tools/gamecount.py --limit first).
# The test settings that keep a long mode short come after the real-input start, as `steps` and a quest's own size: Zen has no end, so it is
# played for ZEN_STEPS driver steps (400, a step is a move or a wait), and "limit" is its end; a mode is given STEPS steps (3000) at most, and a run that stops on that is
# not an end for the others.
cd "$(dirname "$0")/../../.." || exit 2
W="$(pwd -W)"
MODES="${*:-classic zen lightning quest butterflies poker icestorm diamondmine}"
DISPLAYS="${REALINPUT_DISPLAYS:-win full alt}"
STEPS="${STEPS:-3000}"
ZEN_STEPS="${ZEN_STEPS:-400}"
QUEST_INDEX="${QUEST_INDEX:-3}"       # Stratamax: 20 moves, a loss (the map comes back over the board)
NOMOVE="RGBYPORW GBYPORWG BYPORWGB YPORWGBY PORWGBYP ORWGBYPO RWGBYPOR WGBYPORW"   # a board with no move (the suite's): the game ends at once
declare -A ATMOVE=([classic]="40 board $NOMOVE" [icestorm]="60 idle")   # mode: what is run after that many moves, to shorten it
fails=0
for mode in $MODES; do
	for display in $DISPLAYS; do
		[ "$display" = alt ] && [ "$mode" != classic ] && continue
		D="build/realinput/$mode-$display"
		rm -rf "$D"; mkdir -p "$D/user"
		if [ "$display" = win ]; then   # windowed: the setting the window mode's own switch saves
			printf '%s\n' '[SOFTWARE\SteamPopCap\Bejeweled3]' 'ScreenMode=dword:0' > "$D/user/registry.ini"
		fi
		F=(--first "seed 1" --first "unlock all")
		if [ "$mode" = quest ]; then F+=(--first "play quest $QUEST_INDEX"); else F+=(--first "play $mode"); fi
		[ -n "${ATMOVE[$mode]:-}" ] && F+=(--first "atmove ${ATMOVE[$mode]}")
		if [ "$display" = alt ]; then
			F+=(--first "keydown Left Alt" --first "key Return" --first "keyup Left Alt" --first "wait 30")
		fi
		steps="$STEPS"; [ "$mode" = zen ] && steps="$ZEN_STEPS"
		until python -I tools/gamecount.py --limit > /dev/null 2>&1; do sleep 20; done
		python tools/playbymove/playbymove.py --dir "$W/$D" --steps "$steps" --nulls 3 --timeout 180 --shot "$W/$D/end.png" "${F[@]}" \
			"$W/build/native/bejeweled3.exe" "-gamedir=$W/assets/bejeweled3" "-userdir=$W/$D/user" > "$D/summary.txt" 2>&1
		code=$?
		last="$(tail -n 1 "$D/summary.txt")"
		moves="$(echo "$last" | sed -n 's/.*: \([0-9]*\) moves, stop \([^;]*\);.*/\1/p')"
		stop="$(echo "$last" | sed -n 's/.*moves, stop \([^;]*\);.*/\1/p')"
		label=window; [ "$display" = full ] && label="full screen"; [ "$display" = alt ] && label="full screen, Alt+Enter to the window"
		ok=yes; [ "$code" -eq 0 ] || ok=no
		# Zen ends on the step limit; every other mode must stop on "over"
		if [ "$mode" != zen ] && [ "$stop" != over ]; then ok=no; fi
		[ "$ok" = yes ] || fails=$((fails + 1))
		echo "$mode $label: ${moves:-?} moves, stop ${stop:-?}$([ "$ok" = yes ] || echo ' (FAILED)')"
	done
done
exit $((fails > 0))
