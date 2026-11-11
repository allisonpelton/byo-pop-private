#!/usr/bin/env bash
# The sound comparison: what the original and the port play on the same input, by order and state.
#   games/typershark/tools/phase2/sound.sh [name] [updates]
# The input is the whole-adventure recording (rec/rt2: a new player, the menu, Adventure, Easy, the hint, level 1, the map, level 2)
# up to <updates> (14200). The original runs on the host (no shots are needed), on the virtual clock, with origrun's recording
# stand-in (--sound wav: every sample play is a line of logs/sound.log) and Big Money's logging BASS beside the copy's own
# (games/bigmoney/tools/bassproxy: Typer Shark's BassLoader asks for 19 of its 21 functions; the music calls go to
# game/basslog.txt). Its script starts with `focus regain`: a SexyApp original that never had the focus plays no sample. The port
# runs with BYOPOP_SOUNDLOG. Big Money's soundcmp.py reads both sides (the same framework generation: BASS_MusicPlayEx,
# BASS_ChannelSetAttributes) and prints the plays that differ.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
N="${1:-snd}"; UPTO="${2:-14200}"
D="build/ts-p2/$N"; RUN="build/origrun/typershark"; G="$RUN/game"
mkdir -p "$D"
python - "$N" "$UPTO" <<'EOF'
import os, sys
sys.path.insert(0, "games/typershark/tools/phase2")
from p2lib import Tour, ROOT
from tour_game import load_inputs
t = Tour(sys.argv[1])
t.steps = [s for s in load_inputs(os.path.join(ROOT, "build", "ts-p2", "rec", "rt2.in.txt")) if s[0] < int(sys.argv[2])]
t.raw(0, "focus regain", "")
t.write(quit_at=int(sys.argv[2]) + 100)
EOF
# the port
BYOPOP_SOUNDLOG="$R/$D/portsound.log" bash games/typershark/tools/phase2/runport.sh "$N" 1 | tail -1
# the original, with the logging BASS in its copy for this run only
TC="$(ls -d /e/retools/llvm-mingw-*-ucrt-x86_64 | sort | tail -1)"
mkdir -p build/bassproxy-ts
[ -f "$G/bass_real.dll" ] || mv "$G/bass.dll" "$G/bass_real.dll"
# The stand-in may export only what the copy's own BASS has: the game's loader calls every function it finds, and this BASS
# has no BASS_SetConfig (with it exported the game called it, the stand-in had nothing to pass the call to, and the run died).
"$TC/bin/llvm-objdump.exe" -p "$G/bass_real.dll" | grep "BASS_" | awk '{print $NF}' > build/bassproxy-ts/real.txt
while IFS= read -r line; do
	name="${line%% *}"
	case "$name" in BASS_*) grep -qx "$name" build/bassproxy-ts/real.txt || { echo "the copy's BASS has no $name: not exported" >&2; continue; };; esac
	echo "$line"
done < games/bigmoney/tools/bassproxy/bassproxy.def > build/bassproxy-ts/bassproxy.def
"$TC/bin/i686-w64-mingw32-clang.exe" -O2 -shared games/bigmoney/tools/bassproxy/bassproxy.c build/bassproxy-ts/bassproxy.def \
	-o build/bassproxy-ts/bass.dll || { mv -f "$G/bass_real.dll" "$G/bass.dll"; exit 1; }
cp build/bassproxy-ts/bass.dll "$G/bass.dll"
rm -f "$G/basslog.txt" "$RUN/logs/sound.log"
rm -rf "$G/users"; cp -r build/ts-p1/fixtures/users-clean "$G/users"
# with phase 2's patch list: its loading patch makes the copy load in one go on its main thread. Without it the loading
# thread runs on the wall clock while the updates run on the virtual one, and whether the new-player dialog is up by update
# 1000, where the recording types the name, depends on how busy the machine is (two of four runs typed into the title screen).
bash games/typershark/tools/run_original.sh "$R/$D/orig.txt" "$D/orig_sound_run.log" 1 600 --sound wav --patches "$R/build/ts-p2/patches.txt"
grep -E "RESULT|SOUND LEAK|exit=" "$D/orig_sound_run.log" | tail -4 | cut -c1-200
# the two logs of the original, kept beside the port's in the shape soundcmp.py reads (a run folder's logs/ and game/): the
# copy's own are gone with its next run
mkdir -p "$D/orun/logs" "$D/orun/game"
cp "$RUN/logs/sound.log" "$D/orun/logs/sound.log"; cp "$G/basslog.txt" "$D/orun/game/basslog.txt" 2>/dev/null
mv -f "$G/bass_real.dll" "$G/bass.dll"
echo "original: $(grep -c ' play ' "$D/orun/logs/sound.log" 2>/dev/null) play lines, $(wc -l < "$D/orun/game/basslog.txt" 2>/dev/null) BASS calls; port: $(grep -c ' play ' "$D/portsound.log") play lines, $(grep -c ' music ' "$D/portsound.log") music lines"
python games/bigmoney/tools/origcmp/soundcmp.py "$D/orun" "$D" > "$D/soundcmp.txt" 2>&1
tail -25 "$D/soundcmp.txt" | cut -c1-240
echo "sound done"
