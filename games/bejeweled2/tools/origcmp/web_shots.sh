#!/bin/bash
# The web build under Node plays a recording of origcmp/record.py (a 2D, 640 by 480 tour with `shot` lines) again and takes its shots as
# the native port did, for tools/shotdiff against the recording's own (the native port's): the pictures of the web build, which no
# trace shows. The 3D game cannot run under Node (no 3D device there), so the 640 by 480 game is what is compared.
#   [PROFILE=<make_profile.py's folder>] [SEED=909] web_shots.sh <recording dir> <tag> [build/web/bejeweled2.js]
# Writes build/logs/bej2/web/<tag>.* and the shots to build/logs/bej2/web/<tag>.shots; prints the shotdiff summary line.
rec=$1; tag=$2; js=${3:-build/web/bejeweled2.js}
cd "$(dirname "$0")/../../../.." || exit 2
ABS=$PWD; W=$ABS/build/logs/bej2/web; mkdir -p $W
NODE=$(ls E:/retools/emsdk/node/*/node.exe | tail -1)
for i in $(seq 1 120); do python tools/gamecount.py --limit 3 > /dev/null && break; sleep 15; done
rm -rf $W/node_cwd_$tag $W/$tag.shots $W/$tag.wasm.user; mkdir -p $W/node_cwd_$tag/shots $W/$tag.wasm.user
python games/bejeweled2/tools/origcmp/to_commands.py $rec/events.json $W/$tag.cmd --shots shots > /dev/null
: > $W/empty.ri
printf '[SOFTWARE\Steam\Bejeweled2]\nScreenMode=dword:0\nHighResolution=dword:0\n' > $W/$tag.wasm.user/registry.ini
[ -n "$PROFILE" ] && cp -r "$PROFILE/users" $W/$tag.wasm.user/users
(cd $W/node_cwd_$tag && "$NODE" $ABS/games/bejeweled/tools/verify/wasmrun.js game --timeout 900 $ABS/$js -gamedir=$ABS/assets/bejeweled2 -userdir=$W/$tag.wasm.user -commands=$W/$tag.cmd -realinput=$W/empty.ri -hold -fast -drawevery -seed=${SEED:-909} -trace > $W/$tag.wasm.log 2>&1)
mv $W/node_cwd_$tag/shots $W/$tag.shots
echo "$tag: $(ls $W/$tag.shots | wc -l) shots"
python tools/shotdiff/shotdiff.py --game bejeweled2 --orig $rec --port $W/$tag.shots --run web_$tag 2>&1 | tail -1
