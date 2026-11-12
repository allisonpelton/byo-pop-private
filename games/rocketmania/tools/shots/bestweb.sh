#!/bin/bash
# bestweb.sh <tour name> [window]: best.sh for the web build: the web build (build/web/rocketmania.js) under Node
# (games/bejeweled/tools/verify/wasmrun.js) on a tour with a shot at every update of each shot's window, the best frame of each picked
# against the original's (lagscan.py), then shotdiff of those: build/ph2/<tour>web/, run id <tour>webbest. The original's shots (orig.sh)
# must be in build/ph2/<tour>/orig. RM_REG="Name=value,..." and GS as best.sh. Needs build/web (TARGET=rocketmania tools/build.sh web).
source "$(dirname "$0")/../identical/common.sh"
N=$1; W=${2:-1}; D=build/ph2/${N}web; R=$D/run
NODE=$(ls /e/retools/emsdk/node/*/bin/node.exe 2>/dev/null | head -1); [ -n "$NODE" ] || NODE=$(ls /e/retools/emsdk/node/*/node.exe | head -1)
rm -rf "$D"; mkdir -p "$D" "$R/shots" "$R/user"
python games/rocketmania/tools/shots/lagscan.py games/rocketmania/tools/shots/$N.txt build/ph2/$N/orig "$D" --window $W --ahead 0 --prepare > /dev/null
python games/rocketmania/tools/shots/make.py "$D/lag-tour.txt" "$D" web > "$R/script.txt"
if [ -n "$RM_REG" ]; then
	{ echo "; Settings (emulated Windows registry, HKEY_CURRENT_USER)"; echo; echo '[SOFTWARE\PopCap\RocketMania]'
	  for kv in ${RM_REG//,/ }; do echo "${kv%%=*}=dword:${kv#*=}"; done; } > "$R/user/registry.ini"
fi
until python tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done
( cd "$R" && "$ROOT/$RUNDESK" "$NODE" "$(win "$ROOT/games/bejeweled/tools/verify/wasmrun.js")" game "$(win "$ROOT/build/web/rocketmania.js")" \
	-gamedir="$(win "$ROOT/assets/rocketmania")" -userdir=user -fast -drawevery -seed=${GS:-1} -script=script.txt --timeout 1800 > out.txt 2>&1 )
mv "$R/shots" "$D/port"; ls "$D/port" | wc -l; tail -2 "$R/out.txt" | cut -c1-160
python games/rocketmania/tools/shots/lagscan.py games/rocketmania/tools/shots/$N.txt build/ph2/$N/orig "$D" --window $W --ahead 0 --compare --pick "$D/best" > "$D/lag.log"
python tools/shotdiff/shotdiff.py --game rocketmania --orig build/ph2/$N/orig --port "$D/best" --run ${N}webbest | tail -1
python - "$N" <<'PY'
import json, sys
s = json.load(open('build/shotreview/rocketmania/%swebbest/summary.json' % sys.argv[1]))
print(s['counts'])
for p in s['pairs']:
    if p['status'] != 'match':
        print(p['name'], p['status'], [(b['x'], b['y'], b['w'], b['h'], b.get('pixels')) for b in p.get('boxes', [])][:6])
PY
