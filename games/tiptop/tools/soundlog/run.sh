#!/bin/bash
# Records BYOPOP_SOUNDLOG (the framework's sound log, read by tools/soundpreset.py) of a TipTop run driven by realplay.py:
#   run.sh <name> play <mode> [--steps N] [--seed N] [--botseed N]     a whole game by the bot's `move`, from the title
#   run.sh <name> tour <N>                                              phase two's tour N (menus, options, credits...)
#   run.sh <name> options                                               the options' two sliders dragged (options.py)
# The log is build/soundlog/<name>/sound.log, the run's own files beside it. One game process, at the shared limit.
cd "$(dirname "$0")/../../../.." || exit 1
name=$1; shift
out="build/soundlog/$name"
mkdir -p "$out"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 3; done
script=games/tiptop/tools/realplay.py
if [ "$1" = options ]; then script=games/tiptop/tools/soundlog/options.py; shift; fi
BYOPOP_SOUNDLOG="$(pwd -W)/$out/sound.log" python $script "$@" --out "$out" > "$out/result.txt" 2>&1
echo "$name: exit $? $(tail -1 "$out/result.txt" | cut -c1-300)"
