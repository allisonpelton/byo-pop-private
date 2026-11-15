#!/bin/bash
# powerups.sh <name> <seed> <end ms> <vk>...: a coverage run of the original with the developers' test keys held in turn (addkeys.py, 8000
# updates each from update 12000): a power-up made while a key is held is of that kind (0..9, A..D: kinds 0..13), so one run meets the
# effects the table's chances rarely give. A fresh profile, Start Game, the instructions' Play, then the random play of gen.py.
# The runs are run.sh's.
cd "$(dirname "$0")/../../../.."
N=$1; SEED=$2; END=$3; shift 3
mkdir -p build/coverage/cosmicbugs
P=build/coverage/cosmicbugs/prefix-$N.txt
printf '3000 click 175 313\n4000 click 400 563\n' > $P
python games/cosmicbugs/tools/play/gen.py $SEED $END $N $P
python -I games/cosmicbugs/tools/coverage/hover.py build/origrun/$N.txt 2
python -I games/cosmicbugs/tools/coverage/addkeys.py build/origrun/$N.txt 12000 8000 "$@"
bash games/cosmicbugs/tools/coverage/run.sh $N build/origrun/$N.txt 900
