#!/usr/bin/env bash
# The 3D screen wipe of one seed, frame by frame (GAME_NOTES section 10): the original on the desktop (22:00 to 08:00 only; visrun3d.py, its
# window at the top right, pictures read off the screen while the game is held), the port's script from the updates its steps landed at, the
# port under rundesk (-fast -drawevery, Is3D 1) with a shot of every update, then wipematch.py and tools/shotdiff on the best matches.
#   games/astropop/tools/origcmp/wipe.sh <seed> [every [count]]        files in build/logs/ap/wipe/s<seed>/
# The seed picks the kind: 1 the edge sweeping to the right (kind 2), 2 pixels at random (0), 3 the edge to the left (1), 15 rows (3).
cd "$(dirname "$0")/../../../.." || exit 2
SEED=$1; D=build/logs/ap/wipe/s$SEED
T=games/astropop/tools/origcmp
mkdir -p build/logs/ap/wipe
python -I $T/wipeshots.py tour $D ${2:-2} ${3:-45} || exit 2
rm -rf $D/orig
wait_room() { for i in $(seq 1 240); do python tools/gamecount.py --limit 6 > /dev/null && return; sleep 15; done; }
wait_room
VISRUN3D_GRAB=screen python -I $T/wipeshots.py orig $D $SEED --pos 2600,60 || exit 2
python -I $T/wipeshots.py port $D > /dev/null || exit 2
rm -rf $D/user-all; mkdir -p $D/user-all
printf '[SOFTWARE\\PopCap\\AstroPop]\nIs3D=dword:1\nScreenMode=dword:0\n' > $D/user-all/registry.ini
wait_room
build/native/rundesk.exe "$(cygpath -w "$PWD/build/native/astropop.exe")" -fast -drawevery -seed=$SEED -userdir="$(cygpath -w "$PWD/$D/user-all")" -realinput="$(cygpath -w "$PWD/$D/port-all.txt")" > $D/port-all.log 2>&1
echo "seed $SEED: port exit $?, $(ls $D/port-all | wc -l) port shots, $(ls $D/orig | wc -l) original files"
python -I $T/wipematch.py $D --pairs $D/pairs > $D/match.txt 2>&1
tail -1 $D/match.txt
# the boxes that still differ, judged by shotdiff: the menu column (the port has no More Games button) and the window's rounded corners are ignored
cat > $D/shotdiff.json <<'EOF'
{"client": [640, 480], "orig": {"crop": [0, 0, 640, 480]}, "port": {"offset": [0, 0]}, "maxShift": 0,
 "ignore": [{"rect": [430, 100, 210, 360], "label": "the menu column: the port has no More Games button"},
            {"rect": [0, 472, 8, 8], "label": "window corner"}, {"rect": [632, 472, 8, 8], "label": "window corner"}]}
EOF
python tools/shotdiff/shotdiff.py --game astropop --orig $D/pairs/orig --port $D/pairs/port --run wipe_s$SEED --config $D/shotdiff.json | tail -1
