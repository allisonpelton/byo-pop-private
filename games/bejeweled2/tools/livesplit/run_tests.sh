#!/usr/bin/env bash
# The autosplit block's tests for Bejeweled 2 (src/LiveSplitObserver.cpp): a short scripted game of each category with the block on
# (drive.py), what it logged, and the proof that the block changes nothing.
#   bash games/bejeweled2/tools/livesplit/run_tests.sh <out dir> [scenario...]
# Every game runs under build/native/rundesk.exe (playbymove.py and drive.py start each one there), one at a time, each waiting for room
# under the machine's game-process limit (tools/gamecount.py). Needs build/native/bejeweled2.exe and assets/bejeweled2. About 10 minutes.
# The block is turned on by a mod folder holding only a manifest with the feature "livesplit" (<out dir>/mods).
#
# <out dir>/<scenario>/block.log is the game's -livesplitlog (a line whenever a field the rules read changes), game.log its "livesplit:"
# lines, driver.txt what the driver sent. The scenarios are drive.py's (classic action hyper endless puzzle puzzle20 foursquare switcharoo
# left cognito). Then "inert": the bot's games in the nine modes (check.sh's bejeweled2 suite's) with the block on; each one's transcript
# and end screen must be the frozen ones (BJ2_PLAYS in tools/check.sh) that the block-off run of the same suite gives.
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir> [scenario...]}"
shift
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd -W)"
cd "$ROOT" || exit 1
W="$(pwd -W)"
EXE="${BJ2_EXE:-build/native/bejeweled2.exe}"  # a copy of the exe (and its DLLs) lets a build go on beside the tests
export BJ2_EXE="$EXE"
SCENARIOS="${*:-classic action hyper endless puzzle puzzle20 foursquare switcharoo left cognito inert}"
status=0

mkdir -p "$OUT/mods/livesplit"
echo '{ "id": "livesplit", "name": "LiveSplit", "version": "1.0", "author": "test", "description": "test", "affectsScores": false, "features": { "livesplit": true } }' > "$OUT/mods/livesplit/mod.json"

room() {  # waits until there is room for one more game process
	local i
	for i in $(seq 1 90); do python tools/gamecount.py --limit 6 > /dev/null && return 0; sleep 10; done
	echo "no room for a game process after 15 minutes" >&2; return 1
}

for s in $SCENARIOS; do
	if [ "$s" = inert ]; then
		BJ2_PLAYS="$(sed -n 's/^declare -A BJ2_PLAYS=(\(.*\))$/\1/p' tools/check.sh)"
		BJ2_STEPS="$(sed -n 's/^declare -A BJ2_STEPS=(\(.*\))$/\1/p' tools/check.sh)"
		for m in classic action puzzle endless twilight hyper cognito finity hidden; do
			room || { status=1; continue; }
			want="$(echo "$BJ2_PLAYS" | sed -n "s/.*\[$m\]=\"\([^\"]*\)\".*/\1/p")"
			steps="$(echo "$BJ2_STEPS" | sed -n "s/.*\[$m\]=\([0-9]*\).*/\1/p")"
			d="$OUT/inert-$m"; rm -rf "$d"; mkdir -p "$d/user"
			python tools/playbymove/playbymove.py --dir "$d" --steps "$steps" --first "unlock all" --first "goto $m" --timeout 120 \
				--shot "$d/end.png" "$EXE" "-gamedir=$W/assets/bejeweled2" "-userdir=$d/user" -seed=1 \
				"-modsdir=$OUT/mods" "-livesplitlog=$d/block.log" > "$d/summary.txt" 2>&1
			t="$(grep -v screenshot "$d/driver.txt" 2>/dev/null | sha256sum | cut -c1-12)"
			p="$(sha256sum "$d/end.png" 2>/dev/null | cut -c1-12)"
			if [ "$t:$p" = "$want" ] && [ -s "$d/block.log" ]; then echo "inert $m: ok (block on, $(wc -l < "$d/block.log") lines)"
			else echo "inert $m: DIFFERS ($t:$p, frozen $want, block.log $(wc -l < "$d/block.log" 2>/dev/null) lines)"; status=1; fi
		done
		continue
	fi
	room || { status=1; continue; }
	d="$OUT/$s"; rm -rf "$d"; mkdir -p "$d/user"
	r="$(python games/bejeweled2/tools/livesplit/drive.py "$s" "$d" "-gamedir=$W/assets/bejeweled2" "-userdir=$d/user" -seed=1 \
		"-modsdir=$OUT/mods" "-livesplitlog=$d/block.log" 2>&1 | tail -n 1)"
	echo "$r; $(grep -ac 'livesplit:' "$d/game.log") event lines, $(wc -l < "$d/block.log" 2>/dev/null) block lines"
	case "$r" in *": ok,"*) ;; *) status=1 ;; esac
done
exit $status
