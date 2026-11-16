#!/bin/bash
# The 2D tours (640 by 480) on the GPU path and on SDL's software renderer (the software screen of the no-GPU fallback): each mode's tour is recorded
# by the port's bot, replayed natively both ways, and shotdiff sets the software shots beside the GPU's (which equal the original's, IDENTICAL_PLAY.md).
#   [REUSE=1: keep the recording] [OUT=build/visrun/soft2d] soft2d.sh <mode>...
cd "$(dirname "$0")/../../../.." || exit 2
OUT=${OUT:-build/visrun/soft2d}; ABS=$PWD
for m in "$@"; do
  python tools/gamecount.py --limit 6 > /dev/null || { echo "$m: no room"; continue; }
  mkdir -p $OUT
  if [ -z "$REUSE" ] || [ ! -f $OUT/$m/cmd.txt ]; then rm -rf $OUT/$m
  LOWRES=1 MODES=$m OUT=$OUT bash -c 'PLAY_AT=500 DRAWEVERY=1 PROFILE=build/logs/bej2/profile bash games/bejeweled2/tools/origcmp/record_port.sh tour:'$m' 40 $OUT/'$m' 909' > $OUT/$m.record.txt 2>&1
  python games/bejeweled2/tools/origcmp/to_commands.py $OUT/$m/events.json $OUT/$m/cmd.txt --shots shots > /dev/null
  fi; rm -rf $OUT/$m/gpu $OUT/$m/software
  for d in gpu software; do
    V=$OUT/$m/$d; mkdir -p $V/cwd/shots $V/user; : > $V/e.ri
    cp -r build/logs/bej2/profile/users $V/user/users
    printf '[SOFTWARE\\Steam\\Bejeweled2]\nScreenMode=dword:0\nHighResolution=dword:0\nIs3D=dword:0\n' > $V/user/registry.ini
    if [ $d = software ]; then export SDL_RENDER_DRIVER=software; else unset SDL_RENDER_DRIVER; fi
    (cd $V/cwd && timeout 1500 $ABS/build/native/rundesk.exe $ABS/build/native/bejeweled2.exe -gamedir= -userdir=$ABS/$V/user -commands=$ABS/$OUT/$m/cmd.txt -realinput=$ABS/$V/e.ri -hold -fast -drawevery -seed=909 -trace > $ABS/$V/port.log 2>&1)
  done
  unset SDL_RENDER_DRIVER
  python tools/shotdiff/shotdiff.py --game bejeweled2 --orig $OUT/$m/gpu/cwd/shots --port $OUT/$m/software/cwd/shots --run soft2d_$m 2>&1 | tail -n 1
done
