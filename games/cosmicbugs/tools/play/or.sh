# or.sh <run> <script> [extra origrun args]: the original (a copy under build/origrun/<run>/game, windowed, under
# rundesk through origrun) with the seed, score, lives and fill watched (W='--watch ...' replaces them; SAVE=<cb.dat>
# starts it with that save; PROXY=1 logs its BASS calls), output in build/origrun/<run>.out. pair.sh adds the patches and the virtual clock.
R=$1; S=$2; shift 2
WATCH=(--watch 0x460a64 --watch 0x46e5e8 --watch 0x460040 --watch "[0x473138]+0x3b8")
[ -n "$W" ] && read -ra WATCH <<< "$W"
cd "$(dirname "$0")/../../../.."
G=build/origrun/$R/game
[ -d "$G" ] || build/origrun-bin/origrun.exe --game cosmicbugs --run $R --copy-only > /dev/null
rm -f $G/cb.dat; [ -n "$SAVE" ] && cp "$SAVE" $G/cb.dat
# PROXY=1: BASS's calls logged to $G/basslog.txt by games/cosmicbugs/tools/bassproxy (built into build/bassproxy-cb)
if [ -n "$PROXY" ]; then
	[ -f $G/bass_real.dll ] || mv $G/bass.dll $G/bass_real.dll
	cp build/bassproxy-cb/bass.dll $G/bass.dll
elif [ -f $G/bass_real.dll ]; then
	mv -f $G/bass_real.dll $G/bass.dll
fi
printf '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n' > $G/cp.cfg
timeout 900 build/origrun-bin/origrun.exe --exe "$(cygpath -wa $G/CosmicBugs.exe)" --run $R --script "$S" --cursor scripted \
	--counter 0x460a1c --update-ms 1 \
	"${WATCH[@]}" --timeout 600 "$@" > build/origrun/$R.out 2>&1
echo "origrun exit $?"
