#!/bin/bash
# The native reference for the page's replay (replay3d.js): records one mode's 3D tour with the port's bot, turns it into commands and plays
# them natively (rundesk, -hold -fast -drawevery -seed=909); the shots land in $OUT/<mode>/ref/shots and the commands in $OUT/<mode>/cmd.txt.
#   [OUT=build/visrun/web3d] native_ref.sh <mode>...
cd "$(dirname "$0")/../../../.." || exit 2
OUT=${OUT:-build/visrun/web3d}; ABS=$PWD
for m in "$@"; do
  python tools/gamecount.py --limit 6 > /dev/null || { echo "$m: no room"; continue; }
  MODES=$m OUT=$OUT bash games/bejeweled2/tools/origcmp/record_3d_tours.sh > /dev/null
  V=$OUT/$m/ref; rm -rf $V; mkdir -p $V/cwd/shots $V/user
  python games/bejeweled2/tools/origcmp/to_commands.py $OUT/$m/events.json $OUT/$m/cmd.txt --shots shots > /dev/null
  : > $V/empty.ri
  cp -r build/logs/bej2/profile/users $V/user/users
  printf '[SOFTWARE\Steam\Bejeweled2]\nScreenMode=dword:0\nHighResolution=dword:1\n' > $V/user/registry.ini
  (cd $V/cwd && timeout 1500 $ABS/build/native/rundesk.exe $ABS/build/native/bejeweled2.exe -gamedir= -userdir=$ABS/$V/user -commands=$ABS/$OUT/$m/cmd.txt -realinput=$ABS/$V/empty.ri -hold -fast -drawevery -seed=909 -trace > $ABS/$V/port.log 2>&1)
  echo "$m: $(ls $V/cwd/shots | wc -l) shots, $(wc -l < $OUT/$m/cmd.txt) commands"
done
