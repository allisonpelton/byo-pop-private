#!/bin/bash
# run_orig.sh <worktree> <script> <log prefix> <timeout s>: the original once for each of the four watch sets, one at a time.
# Run it under rundesk: build/native/rundesk.exe bash games/mummymaze/tools/identical/run_orig.sh ...
R="$1"; S="$2"; P="$3"; T="${4:-30}"
for set in A B C D; do
	w=()
	while read -r a b; do w+=("$a" "$b"); done < <(python -I "$R/games/mummymaze/tools/identical/identical.py" watches $set)
	"$R/build/origrun-bin/origrun.exe" --game mummymaze --run p1a --stage-root C:/byo-og --script "$S" --clock virtual \
		--cursor scripted --seed "${SEED:-1234}" --timeout "$T" "${w[@]}" > "$P.$set.log" 2>&1
done
