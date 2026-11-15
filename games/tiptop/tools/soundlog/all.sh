#!/bin/bash
# The sound logs of every sequence TipTop plays: the menus' tour, the options' sliders, and a whole game of each mode by
# the bot (the strategy mode three times: the bot loses it early). Logs: build/soundlog/<name>/sound.log (run.sh).
# Run it with run_in_background; it prints one line a run.
cd "$(dirname "$0")/../../../.." || exit 1
R=games/tiptop/tools/soundlog/run.sh
bash $R tour1 tour 1
bash $R options options
bash $R regular play regular --seed 1
bash $R strategy play strategy --seed 1
bash $R strategy2 play strategy --seed 2 --botseed 8
bash $R strategy3 play strategy --seed 3 --botseed 9
bash $R speed play speed --seed 1
bash $R timed play timed --seed 1
# more games for the sounds those did not reach (the bot's luck decides how far a game goes)
bash $R regular2 play regular --seed 2
bash $R regular3 play regular --seed 3
bash $R regular4 play regular --seed 4
bash $R timed2 play timed --seed 2
bash $R timed3 play timed --seed 3
bash $R strategy4 play strategy --seed 4 --botseed 10
bash $R strategy5 play strategy --seed 5 --botseed 11
echo "all done"
