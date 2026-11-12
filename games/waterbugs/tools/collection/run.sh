#!/usr/bin/env bash
# The port on the PopCap Game Collection's files, against the same port on the files it was staged from (GAME_NOTES.md, "The PopCap
# Game Collection version"). Needs build/native/waterbugs.exe, waterbugs_run.exe and rundesk, and assets/waterbugs staged.
#   games/waterbugs/tools/collection/run.sh <the Collection's WaterBugsComponent folder>
# Stages the Collection's files into build/wb-coll/assets/waterbugs with the game's own stager and compares the two trees byte for
# byte (cmpfiles.py), then plays the suite's scripted games (tools/check.sh's waterbugs_run, waterbugs_loss and waterbugs_window) on
# each set of files: the headless runner's state lines and the pixels of its three screenshots (as check.sh hashes them), and the
# windowed exe's state after the beam, must come out the same. One game process at a time, invisible and silent under rundesk, with
# -fast; each waits for room among the machine's 3 game processes first.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
if [ -z "${BYOPOP_TEST_RUN:-}" ]; then exec "$ROOT/build/native/rundesk.exe" bash "$ROOT/games/waterbugs/tools/collection/run.sh" "$@"; fi
SRC="$1"
OUT=build/wb-coll/run
python -I games/waterbugs/tools/stage_waterbugs_assets.py "$SRC" --out build/wb-coll/assets/waterbugs || exit 1
python -I games/waterbugs/tools/collection/cmpfiles.py assets/waterbugs build/wb-coll/assets/waterbugs

waitroom() { until python tools/gamecount.py --limit 3 > /dev/null; do sleep 20; done; }
hash() {  # <png>: check.sh's wb_hash
	python -I -c "import hashlib,sys;from PIL import Image;i=Image.open(sys.argv[1]).convert('RGB');i.paste((0,0,0),(560,240,690,520)) if 'scores' in sys.argv[1] else i.paste((0,0,0),(405,220,425,235)) if 'beam' in sys.argv[1] else 0;print(hashlib.sha256(i.tobytes()).hexdigest()[:12])" "$1" | tr -d '\r'
}
run() {  # <key> <assets folder>: the three plays on one set of files, a summary line in $OUT/<key>.txt
	local k="$1" A="$2" d="$OUT/$1" u g l w
	rm -rf "$d"; mkdir -p "$d/game" "$d/loss" "$d/win/user"
	waitroom
	printf '%s\n' "3000 click 400 303" "8000 click 75 260" "14000 click 400 465" "20000 state" "20000 shot start" \
		"24000 click 300 300" "32000 state" "32000 shot beam" "32010 end" > "$d/game/script.txt"
	timeout 300 build/native/waterbugs_run.exe "$A" "$d/game" "$d/game/script.txt" > "$d/game/stdout.txt" 2> "$d/game/stderr.txt"
	g="$(grep -E '^[0-9]+ ms: score' "$d/game/stdout.txt" | tr -d '\r' | paste -sd ';' -)"
	waitroom
	printf '%s\n' "3000 click 400 303" "8000 click 75 260" "14000 click 400 465" "20000 lives 1" "21000 die" "24000 click 400 300" \
		"30000 state" "30000 shot scores" "30010 end" > "$d/loss/script.txt"
	timeout 300 build/native/waterbugs_run.exe "$A" "$d/loss" "$d/loss/script.txt" > "$d/loss/stdout.txt" 2> "$d/loss/stderr.txt"
	l="$(grep -E '^[0-9]+ ms: score' "$d/loss/stdout.txt" | tr -d '\r')"
	waitroom
	u="$(cygpath -ma "$d/win")"
	printf '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n' > "$d/win/user/cp.cfg"
	printf '%s\n' "300 down 400 303" "303 up 400 303" "800 down 75 260" "803 up 75 260" "1400 down 400 465" "1403 up 400 465" \
		"2400 down 300 300" "2403 up 300 300" "3200 state" "3200 screenshot $u/beam.png" "3210 quit" > "$d/win/script.txt"
	timeout 300 build/native/waterbugs.exe -gamedir="$A" -userdir="$u/user" -fast -script="$u/script.txt" \
		> "$d/win/stdout.txt" 2> "$d/win/stderr.txt"
	w="$(cat "$d/win/stdout.txt" "$d/win/stderr.txt" | tr -d '\r' | sed -n 's/^INFO: state: //p' | head -n 1)"
	echo "game: $g | loss: $l | window: $w | shots: $(for f in game/start game/beam loss/scores; do hash "$d/$f.png"; done | paste -sd' ' -)" | tee "$d.txt"
}
run staged assets/waterbugs
run coll build/wb-coll/assets/waterbugs
if cmp -s "$OUT/staged.txt" "$OUT/coll.txt"; then echo "the Collection's files play the scripted games as the staged copy: the same states and the same pixels"
else echo "DIFFERENT"; diff "$OUT/staged.txt" "$OUT/coll.txt"; exit 1; fi
