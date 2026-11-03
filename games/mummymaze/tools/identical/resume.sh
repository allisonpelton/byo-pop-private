#!/bin/bash
# resume.sh <name> <seed> <save at> <bot arguments for the continued game...>: a classic game saved by the port and continued by
# both programs. The port plays <save at> updates of a first board with its bot and writes classic.sav; the original gets that
# file in its copy, its Continue dialog is answered by the clicks the port's bot logs, and both play on with the same input.
# Run it under rundesk from the worktree: build/native/rundesk.exe bash games/mummymaze/tools/identical/resume.sh <name> ...
N="$1"; SEED="$2"; AT="$3"; shift 3
R="$(pwd -W)"; T="$R/games/mummymaze/tools/identical"; D="$R/build/phase1"; mkdir -p "$D"
U="$D/$N.user"; rm -rf "$U"; mkdir -p "$U"
if [ -n "$CUSTOM" ]; then	# CUSTOM=<save file>: start from that file instead of the port's own first run
	cp "$CUSTOM" "$U/classic.sav"
else
	"$R/build/native/mummymaze.exe" -userdir="$U" -fast -seed="$SEED" -autoplay="$SEED" -autoplaygames=1 -autoplaysave="$AT" -autoplaycap=100000 \
		-autoplaylog="$D/$N.save.txt" > "$D/$N.save.stdout" 2> "$D/$N.save.stderr"
fi
cp "$U/classic.sav" "$D/$N.classic.sav"
# the port's second run: Continue answered yes, then the bot plays on (its user folder keeps the save; the original's copy gets a copy)
USERDIR="$U" bash "$T/run_port.sh" "$R" "$D/$N.port.txt" -seed="$SEED" -autoplay="$SEED" -autoplaygames=1 -autoplaycontinue "$@"
python -I "$T/identical.py" script "$D/$N.port.txt" "$D/$N.script.txt" --offset "${OFF:-2301}"
LAST=$(grep -o "^at [0-9]*" "$D/$N.script.txt" | tail -1 | cut -d" " -f2)
TO=${TIMEOUT:-$(( (LAST + 3000) / 500 + 30 ))}
RUN=p1r
# stage the copy once with a script that quits at once, put the save in it, then run it where it stands
printf 'quit\n' > "$D/$N.stage.txt"
rm -rf "C:/byo-og/$RUN" "$R/build/origrun/$RUN"
"$R/build/origrun-bin/origrun.exe" --game mummymaze --run "$RUN" --stage-root C:/byo-og --script "$D/$N.stage.txt" --clock virtual --seed "$SEED" --timeout 60 > "$D/$N.stage.log" 2>&1
for set in ${SETS:-A B C D E F}; do
	w=()
	while read -r a b; do w+=("$a" "$b"); done < <(python -I "$T/identical.py" watches $set)
	mkdir -p "C:/byo-og/$RUN/game/data/saves"; cp "$D/$N.classic.sav" "C:/byo-og/$RUN/game/data/saves/classic.sav"
	"$R/build/origrun-bin/origrun.exe" --game mummymaze --run "$RUN" --stage-root C:/byo-og --exe "C:/byo-og/$RUN/game/WinMM.exe" --script "$D/$N.script.txt" --clock virtual \
		--cursor scripted --seed "$SEED" --timeout "$TO" "${w[@]}" > "$D/$N.$set.log" 2>&1
done
python -I "$T/identical.py" compare "$D/$N.port.txt" "${OFF:-2301}" "$D/$N.A.log:A" "$D/$N.B.log:B" "$D/$N.C.log:C" "$D/$N.D.log:D" "$D/$N.E.log:E" "$D/$N.F.log:F"
