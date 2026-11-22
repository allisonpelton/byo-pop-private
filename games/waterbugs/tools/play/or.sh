# or.sh <run> <script> [extra origrun args]: the original (a copy under build/origrun/<run>/game, windowed, under rundesk
# through origrun) played by a script; SAVE=<cb.dat> starts it with that save. Output in build/origrun/<run>.out; its shots
# in build/origrun/<run>/shots. pair.sh adds the patches and the virtual clock.
R=$1; S=$2; shift 2
cd "$(dirname "$0")/../../../.."
G=build/origrun/$R/game
[ -d "$G" ] || build/origrun-bin/origrun.exe --game waterbugs --run $R --copy-only > /dev/null
rm -f $G/cb.dat; [ -n "$SAVE" ] && cp "$SAVE" $G/cb.dat
printf '<CP_CONFIG
    fullscreen="0"
    desktopResolution="0"
    wave="1"
    music="1"
/>
' > $G/cp.cfg
timeout 10800 build/origrun-bin/origrun.exe --exe "$(cygpath -wa $G/WaterBugs.exe)" --run $R --script "$S" --cursor scripted --sandbox \
	--counter 0x460a1c --update-ms 1 --input-eip 41f489,41f511 --timeout ${WBTIMEOUT:-600} "$@" > build/origrun/$R.out 2>&1
echo "origrun exit $?"
