#!/bin/bash
# run.sh <steps name> [orig|port]: one part of the screenshot pilot (docs/SHOTDIFF.md), the original under origrun and
# then the port under rundesk (one game process at a time), from the same start state, with the same steps
# (gen.py). Captures land in build/scratch/pilot/{orig,port}, as shotdiff wants them: stills by name, clips as
# folders of numbered frames with a clip.json.
#
# The start state: the user's own WinBAD registry values (backed up in build/scratch/pilot/backup) with LastUser
# Tester, and their Allison.bwa plus the pilot's Tester.bwa in the original's users folder; the port gets the same
# in a user dir of its own (build/scratch/pilot/portuser0). The original's copy is staged under the main checkout's
# build/origrun/bwapilot: its launcher fails from a worktree's longer path. Both sides get the same random seeds: the
# port -seed=12345, the original's copy patched to the same (seedpatch.py).
#
# Prepared by hand in build/scratch/pilot (git-ignored; the user's own saves): backup/ (reg export of
# HKCU\Software\PopCap\WinBAD, and its users folder), portuser0/ (registry.ini with the same values and LastUser
# Tester; users/ with Allison.bwa and the pilot's Tester.bwa: tests/menu's profile after cheat.lua's _DefaultMP(),
# whose media player list the scripts can draw), exe.txt (the staged copy's exe). Restore the backup afterwards.
set -u
W=$(cd "$(dirname "$0")/../../../.." && pwd)
T=$W/games/bookwormadv/tools/pilot
P=$W/build/scratch/pilot
name=$1; which=${2:-both}
EXE=$(cat "$P/exe.txt")
SEED=12345
mkdir -p "$P/orig" "$P/port" "$P/scripts"
python "$T/gen.py" "$T/$name.txt" "$P/scripts/$name.orig.txt" "$P/scripts/$name.port.txt" "$P/port" || exit 1

if [ "$which" != port ]; then
	# The script is timed to the original's own updates (gen.py): SexyAppBase's mUpdateCount, at +0x484 in the app
	# (0x442e60 reads it), whose pointer is the global at 0x8259e4.
	# A shot inside the sandbox shows exactly the update it is labelled with when origrun can see that the game has
	# drawn it: mHasPendingDraw at +0x45a and mUpdateAppState at +0x488 (Process, 0x549b10; DrawDirtyStuff, 0x5446f0), and
	# whether that draw was skipped, mLastDrawWasEmpty at +0x459.
	run=pilot-$name
	R=$W/build/origrun/$run
	rm -rf "${R:?}"
	# a sandbox run's exe lies inside its run folder: the prepared copy (exe.txt) is copied there for each run
	mkdir -p "$R"
	cp -r "$(dirname "$(cygpath -u "$EXE")")" "$R/game"
	GEXE=$R/game/$(basename "$(cygpath -u "$EXE")")
	# the same seeds on both sides: the copy's clock reads in SexyAppBase::Init become constants (seedpatch.py)
	python "$T/seedpatch.py" "$GEXE" $SEED | tee "$P/$name.seedpatch.log"
	# The original runs inside Windows Sandbox (docs/ORIGRUN.md, "Windows Sandbox"), where every run is a first run, so
	# the start state goes in as files and registry presets: Allison.bwa (the user's) and the pilot's Tester.bwa in
	# the game's users folder on the sandbox's C:, and the port's start values (portuser0/registry.ini: LastUser
	# Tester, Is3D 0) as a .reg file, which the game reads there as SYSTEM's HKCU.
	U="$R/sandbox-files/C/ProgramData/PopCap Games/WinBAD/users"
	mkdir -p "$U"
	cp "$P/backup/users/Allison.bwa" "$P/portuser0/users/Tester.bwa" "$U/"
	python "$T/inireg.py" "$P/portuser0/registry.ini" "$P/start.reg"
	"$W/build/native/rundesk.exe" "$W/build/origrun-bin/origrun.exe" --sandbox --exe "$(cygpath -w "$GEXE")" --run "$run" --script "$(cygpath -w "$P/scripts/$name.orig.txt")" \
		--reg "$(cygpath -w "$P/start.reg")" --3d off --3d-key "Software\PopCap\WinBAD" --3d-flag "[[0x8259e4]+0x36c]+0x38" --counter "[0x8259e4]+0x484" --pending-draw "[0x8259e4]+0x45a" --update-state "[0x8259e4]+0x488" --draw-empty "[0x8259e4]+0x459" --update-ms 10 --timeout 240 > "$P/$name.orig.log" 2>&1
	grep -i "RESULT\|timeout\|error\|msgbox\|dialog" "$P/$name.orig.log" | tail -5
	S=$W/build/origrun/$run/shots
	python - "$S" "$P/orig" "$T/$name.txt" <<'EOF'
import os, shutil, sys, json
src, dst, steps = sys.argv[1:4]
clips = {}
for line in open(steps):
    w = line.split("#", 1)[0].split()
    if w and w[0] == "title": clips["title"] = int(w[2])
    if w and w[0] == "clip": clips[w[1]] = int(w[3])
for f in sorted(os.listdir(src)):
    if os.path.isdir(os.path.join(src, f)):   # a burst: numbered frames and its clip.json already
        shutil.copytree(os.path.join(src, f), os.path.join(dst, f), dirs_exist_ok=True)
        continue
    stem = f[:-4]
    if stem == "_window": continue
    base, _, num = stem.rpartition("_")
    if base in clips and num.isdigit():
        os.makedirs(os.path.join(dst, base), exist_ok=True)
        shutil.copy(os.path.join(src, f), os.path.join(dst, base, "%04d.png" % int(num)))
    else:
        shutil.copy(os.path.join(src, f), os.path.join(dst, f))
for c, iv in clips.items():
    if os.path.isdir(os.path.join(dst, c)):
        json.dump({"interval_ms": iv}, open(os.path.join(dst, c, "clip.json"), "w"))
EOF
fi

if [ "$which" != orig ]; then
	d=$P/portrun/$name; mkdir -p "$d"
	bash "$T/reset.sh" "$d/user"
	timeout 280 "$W/build/native/rundesk.exe" "$W/build/native/bookwormadv.exe" -userdir="$d/user" -seed=$SEED -drawevery \
		-script="$P/scripts/$name.port.txt" > "$d/stdout.txt" 2>&1
	echo "port exit $?"; grep "script\|FATAL" "$d/stdout.txt" | tail -5
	python - "$P/port" "$T/$name.txt" <<'EOF'
import os, sys, json
dst, steps = sys.argv[1:3]
for line in open(steps):
    w = line.split("#", 1)[0].split()
    if w and w[0] in ("title", "clip"):
        c, iv = ("title", w[2]) if w[0] == "title" else (w[1], w[3])
        json.dump({"interval_ms": int(iv)}, open(os.path.join(dst, c, "clip.json"), "w"))
EOF
fi
