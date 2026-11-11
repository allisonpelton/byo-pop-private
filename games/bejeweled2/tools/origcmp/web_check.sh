#!/bin/bash
# Plays a recording of origcmp/record.py (a 2D, 640 by 480 run) again without a driver, natively and as the web build under Node
# (tools/verify/wasmrun.js game), and compares the three -trace lists: the recording's, the native replay's and the wasm's.
#   [SEED=<the recording's -seed, default 1>] [PROFILE=<make_profile.py's folder>] web_check.sh <recording dir> <tag> [build/web/bejeweled2.js]
# Writes build/logs/bej2/web/<tag>.* and prints one line: IDENTICAL, or the first line at which the wasm differs from the native replay.
# The 3D game cannot run under Node (no 3D device there), so the 640 by 480 game is what the web build plays in it.
rec=$1; tag=$2; js=${3:-build/web/bejeweled2.js}
cd "$(dirname "$0")/../../../.." || exit 2
ABS=$PWD; W=$ABS/build/logs/bej2/web; mkdir -p $W/node_cwd
NODE=$(ls E:/retools/emsdk/node/*/node.exe | tail -1)
for i in $(seq 1 120); do python tools/gamecount.py --limit 3 > /dev/null && break; sleep 15; done
NS=; grep -q '^nosparkle' $rec/commands.txt && NS=--nosparkle
python games/bejeweled2/tools/origcmp/to_commands.py $rec/events.json $W/$tag.cmd $NS > /dev/null
: > $W/empty.ri
mkuser() {  # <folder>
	rm -rf "$1"; mkdir -p "$1"
	printf '[SOFTWARE\Steam\Bejeweled2]\nScreenMode=dword:0\nHighResolution=dword:0\n' > "$1/registry.ini"
	[ -n "$PROFILE" ] && cp -r "$PROFILE/users" "$1/users"
}
trace() { grep 'watch' "$1" | sed 's/^INFO: //'; }
mkuser $W/$tag.nat.user
SDL_RENDER_DRIVER=software build/native/rundesk.exe $ABS/build/native/bejeweled2.exe -gamedir= -userdir=$W/$tag.nat.user -commands=$W/$tag.cmd -realinput=$W/empty.ri -hold -fast -seed=${SEED:-1} -trace > $W/$tag.nat.log 2>&1
mkuser $W/$tag.wasm.user
(cd $W/node_cwd && "$NODE" $ABS/games/bejeweled/tools/verify/wasmrun.js game --timeout 900 $ABS/$js -gamedir=$ABS/assets/bejeweled2 -userdir=$W/$tag.wasm.user -commands=$W/$tag.cmd -realinput=$W/empty.ri -hold -fast -seed=${SEED:-1} -trace > $W/$tag.wasm.log 2>&1)
trace $W/$tag.nat.log > $W/$tag.nat.p; trace $W/$tag.wasm.log > $W/$tag.wasm.p; trace $rec/game.log > $W/$tag.rec.p
# the replay stops where the recording's last input was, the recording a little later (its quit): compare up to the replay's end
n=$(wc -l < $W/$tag.nat.p)
r="native replay = recording:"
if cmp -s <(head -$((n - 8)) $W/$tag.nat.p) <(head -$((n - 8)) $W/$tag.rec.p); then r="$r yes"; else r="$r NO"; fi
if cmp -s $W/$tag.wasm.p $W/$tag.nat.p; then echo "$tag: IDENTICAL ($n trace lines; $r)"; else
	echo "$tag: DIFFERENT, wasm and native replay ($(wc -l < $W/$tag.wasm.p) and $n lines; $r); first difference:"; diff $W/$tag.wasm.p $W/$tag.nat.p | head -4
fi
