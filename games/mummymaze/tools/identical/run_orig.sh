#!/bin/bash
# run_orig.sh <worktree> <script> <log prefix> <timeout s>: the original once for each of the four watch sets, one at a time.
# Run it under rundesk: build/native/rundesk.exe bash games/mummymaze/tools/identical/run_orig.sh ...
R="$1"; S="$2"; P="$3"; T="${4:-30}"
for set in ${SETS:-A B C D E F}; do
	w=()
	while read -r a b; do w+=("$a" "$b"); done < <(python -I "$R/games/mummymaze/tools/identical/identical.py" watches $set)
	# the play count: the original shows its register nag once it reaches ten, and each quit adds one (docs/IDENTICAL_PLAY.md)
	reg add "HKLM\Software\WOW6432Node\PopCap\MummyMaze" //v TimesPlayed //t REG_DWORD //d 0 //f > /dev/null
	# a fresh copy each time: the original saves its game when it quits, and the next run would be asked to continue it
	rm -rf "C:/byo-og/${RUN:-p1a}" "$R/build/origrun/${RUN:-p1a}"
	"$R/build/origrun-bin/origrun.exe" --game mummymaze --run "${RUN:-p1a}" --stage-root C:/byo-og --script "$S" --clock virtual \
		--cursor scripted --seed "${SEED:-1234}" --timeout "$T" "${w[@]}" > "$P.$set.log" 2>&1
done
