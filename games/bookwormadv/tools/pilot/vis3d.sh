#!/bin/bash
# vis3d.sh <steps name>: one part of the screenshot pilot in 3D mode. The original runs on the visible desktop through
# tools/visrun.py (Windows Sandbox cannot give it 3D; docs/VISRUN.md): staged by origrun on a short path (the game dies
# under a worktree's long one), with tools/origrun/patches/bookwormadv.txt (the seeds, the hover, the trial's end and
# the ChapterLoadScreen patch: no hint drawn while a chapter loads, so the boards do not follow the desktop's load
# lengths), on origrun's virtual clock, its shots in the sandbox's shape. Then the port under rundesk in 3D mode with
# the original's load lengths and -holdloadhints (run.sh with PILOT3D=on). The steps are tools/pilot/3d/<steps>.txt
# where the part's own do not fit 3D. PILOT_PROFILE and PILOT_USERS as for run.sh: the config made here seeds the
# game's users folder with them (save_seed), and visrun backs up and restores the user's own key and folder.
set -u
W=$(cd "$(dirname "$0")/../../../.." && pwd)
T=$W/games/bookwormadv/tools/pilot
P=$W/build/scratch/pilot
name=$1
ST=$T/$name.txt; [ -f "$T/3d/$name.txt" ] && ST=$T/3d/$name.txt
mkdir -p "$P/v3d" "$P/orig3d" "$P/port3d" "$P/scripts3d" "$P/visrun"
python "$T/gen.py" "$ST" "$P/scripts3d/$name.orig.txt" "$P/scripts3d/$name.port.txt" "$P/port3d" > /dev/null || exit 1
w() { cygpath -w "$1"; }
U='C:\ProgramData\PopCap Games\WinBAD\users'
seed="['$(w "${PILOT_PROFILE:-$P/portuser0/users/Tester.bwa}")', '$U\\Tester.bwa']"
for f in ${PILOT_USERS:-}; do seed="$seed, ['$(w "$f")', '$U\\$(basename "$f")']"; done
cat > "$P/visrun/$name.toml" <<EOF
# made by games/bookwormadv/tools/pilot/vis3d.sh for the part $name; addresses as in run.sh
name = "bookwormadv-pilot"
game = "bookwormadv"
exe = "BookwormAdventures.exe"
patches = "tools/origrun/patches/bookwormadv.txt"
stage_root = 'E:\byo-og'
work_root = 'E:\byo-og\vr'
title = "Bookworm Adventures"
client = [800, 600]
clock = "virtual"
update_ms = 10
counter = "[0x8259e4]+0x484"
drawn = ["[0x8259e4]+0x45a:1 == 0"]
flag3d = "[[0x8259e4]+0x36c]+0x38:1"
reg_key = 'Software\PopCap\WinBAD'
reg_preset = '$(w "$P/start.reg")'
reg_3d_value = "Is3D"
reg_values = { ScreenMode = 0 }
save_dirs = ['$U']
save_seed = [$seed]
watch = ["[[0x8259e4]+0x83c]+0xb0", "[0x8259e4]+0x4f9", "[0x8259e4]+0x919"]
EOF
until python "$W/tools/gamecount.py" --limit > /dev/null; do sleep 5; done
rm -rf "$P/v3d/$name"
# never beside a host run of the 2D original (run.sh with PILOT_HOST=on), which uses the same key and users folder
until mkdir "$P/winbad.lock" 2> /dev/null; do sleep 15; done
(cd "$W" && python -I tools/visrun.py "$(w "$P/visrun/$name.toml")" "$(w "$P/scripts3d/$name.orig.txt")" --out "$(w "$P/v3d/$name")" \
	--3d on --seed 12345 --shape sandbox) > "$P/v3d/$name.out" 2>&1
rc=$?
rmdir "$P/winbad.lock"
echo "visrun exit $rc: $(tail -n 1 "$P/v3d/$name.out")"
python "$T/loads.py" "$P/v3d/$name/watch.log" > "$P/${name}3d.loads"
python "$T/anchors.py" marks "$P/v3d/$name/visrun.log" > "$P/${name}3d.marks"
python "$T/loads.py" "$P/v3d/$name/watch.log" --start > "$P/${name}3d.startload"
echo "chapter loads (updates): $(cat "$P/${name}3d.loads"); startup load: $(cat "$P/${name}3d.startload")"
# the shots and clips into orig3d (an earlier run's of the same names replaced)
(cd "$P/v3d/$name" && for f in *; do
	case $f in visrun.log|steps.tsv|watch.log|*.toml) continue ;; esac
	rm -rf "$P/orig3d/$f"; cp -r "$f" "$P/orig3d/"
done)
PILOT3D=on bash "$T/run.sh" "$name" port
