#!/bin/bash
# vis3d.sh <steps name>: one part of the screenshot pilot in 3D mode. The original runs on the visible desktop
# (visrun.py: Windows Sandbox cannot give it 3D), then the port under rundesk in 3D mode with the original's load
# lengths (run.sh with PILOT3D=on). The original's copy is staged by hand at a short path (the game dies under a
# worktree's long one): build/origrun/bwapilot's game folder, seedpatch.py applied, without origrun's dsound.dll
# stand-in (VIS_EXE names another). visrun clears its own folder, so each part's shots land in
# build/scratch/pilot/v3d/<steps> and are then copied into orig3d, their landed.tsv lines added to orig3d's.
# PILOT_PROFILE and PILOT_USERS as for run.sh (visrun.py reads them too). One part at a time (the lock
# build/scratch/pilot/vis3d.lock).
set -u
W=$(cd "$(dirname "$0")/../../../.." && pwd)
T=$W/games/bookwormadv/tools/pilot
P=$W/build/scratch/pilot
name=$1
EXE=${VIS_EXE:-E:/byo-og/bwa3d/game/BookwormAdventures.exe}
mkdir -p "$P/v3d" "$P/orig3d" "$P/port3d" "$P/scripts"
# one at a time: visrun backs up and restores the user's WinBAD registry key and users folder
until mkdir "$P/vis3d.lock" 2> /dev/null; do sleep 15; done
trap 'rmdir "$P/vis3d.lock"' EXIT
python "$T/gen.py" "$T/$name.txt" "$P/scripts/$name.orig.txt" "$P/scripts/$name.port.txt" "$P/port3d" > /dev/null || exit 1
until python "$W/tools/gamecount.py" --limit > /dev/null; do sleep 5; done
python "$T/visrun.py" "$EXE" "$P/scripts/$name.orig.txt" "$P/v3d/$name" --3d on
tail -n 2 "$P/v3d/$name.visrun.log"
python "$T/loads.py" "$P/v3d/$name.watch.log" > "$P/${name}3d.loads"
python "$T/loads.py" "$P/v3d/$name.watch.log" --start > "$P/${name}3d.startload"
echo "chapter loads (updates): $(cat "$P/${name}3d.loads"); startup load: $(cat "$P/${name}3d.startload")"
(cd "$P/v3d/$name" && find . -mindepth 1 -maxdepth 1 ! -name landed.tsv -exec cp -r {} "$P/orig3d/" \;)
cat "$P/v3d/$name/landed.tsv" >> "$P/orig3d/landed.tsv"
PILOT3D=on bash "$T/run.sh" "$name" port
