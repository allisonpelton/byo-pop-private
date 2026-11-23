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
#
# PILOT3D=on: both sides in 3D mode, captures in build/scratch/pilot/{orig3d,port3d}. The original gets --3d on and the
# host's GPU (--sandbox-vgpu: the user allowed it for these 3D runs only, 2026-11-01), and dgVoodoo2 in its copy when
# PILOT_DGVOODOO names its folder (origrun --dgvoodoo; on WARP, without vGPU, it crashed the game at start); the port Is3D 1.
# PILOT_SOUND=on: the original's BASS calls logged to build/scratch/pilot/<steps>.basslog.txt by tools/bassproxy (build
# it to build/bassproxy-bwa/bass.dll; it gives BASS the "no sound" device, the sandbox having none), its sounds by
# origrun's stand-in to <steps>.sound.log, and the port's to <steps>.portsound.log (BYOPOP_SOUNDLOG); compare them
# with soundcmp.py.
# PILOT_PROFILE=<x.bwa>: a save played as Tester's on both sides, in place of the pilot's Tester.bwa (the screen
# coverage pass's parts start from the port's own saves, tools/coverage/advance.sh, some changed with
# tools/coverage/bwa.py). PILOT_USERS="<a.bwa> ...": more players' files in both users folders. PILOT_TIMEOUT=<s>:
# the original's wall-clock cap (240 by default; the port gets 60 s more). PILOT_WATCH=<expr>: an origrun --watch in place of
# the startup load's two (a fourth stops them all), whose length then comes from the part's last run (0x85a938, the
# generator's index, with the port's -randlog in PILOT_PORT_ARGS: the random draws side by side).
# PILOT_PORT_ARGS: more options for the port. PILOT_FULL=on: both sides start in full screen (the registry's ScreenMode 1);
# the part names its shots x_full itself. In the sandbox the 2D original does not take full screen, with or without
# --sandbox-vgpu (2026-11-22): it opens a window and writes ScreenMode 0 back, so its x_full shots are windowed.
set -u
W=$(cd "$(dirname "$0")/../../../.." && pwd)
T=$W/games/bookwormadv/tools/pilot
P=$W/build/scratch/pilot
name=$1; which=${2:-both}
EXE=$(cat "$P/exe.txt")
SEED=12345
SUF=; MODE3D=(--3d off)
FULL=0; [ "${PILOT_FULL:-off}" = on ] && FULL=1
# --watch-max: a load on the host runs thousands of updates, the held screen's timer changing at each
WATCH23=(--watch "[0x8259e4]+0x4f9" --watch "[0x8259e4]+0x919" --watch-max 100000)
[ -n "${PILOT_WATCH:-}" ] && WATCH23=(--watch "$PILOT_WATCH" --watch-max 100000)
if [ "${PILOT3D:-off}" = on ]; then
	SUF=3d; MODE3D=(--3d on --sandbox-vgpu)
	[ -n "${PILOT_DGVOODOO:-}" ] && MODE3D+=(--dgvoodoo "$(cygpath -w "$PILOT_DGVOODOO")")
fi
PO=$P/orig$SUF; PP=$P/port$SUF; N=$name$SUF; SC=$P/scripts$SUF
# A chapter load's screen draws a hint, one Rand(), every 50 updates, so the next board follows how long the load took,
# which on a busy machine is anything from 5 to 260 updates. The pilot holds the hints on both sides (seedpatch.py
# --holdhints, the port's -holdloadhints) and its steps are made so; PILOT_HINTS=on leaves them drawn, for the one pair
# that shows a load's hint change (hints.txt). A part's 3D steps are tools/pilot/3d/<steps>.txt where it has them.
HOLD=-holdloadhints; HOLDP=--holdhints; [ "${PILOT_HINTS:-off}" = on ] && HOLD= && HOLDP=
ST=$T/$name.txt; [ -n "$SUF" ] && [ -f "$T/3d/$name.txt" ] && ST=$T/3d/$name.txt
mkdir -p "$PO" "$PP" "$SC"
python "$T/gen.py" "$ST" "$SC/$name.orig.txt" "$SC/$name.port.txt" "$PP" || exit 1

if [ "$which" != port ]; then
	# The script is timed to the original's own updates (gen.py): SexyAppBase's mUpdateCount, at +0x484 in the app
	# (0x442e60 reads it), whose pointer is the global at 0x8259e4.
	# A shot inside the sandbox shows exactly the update it is labelled with when origrun can see that the game has
	# drawn it: mHasPendingDraw at +0x45a and mUpdateAppState at +0x488 (Process, 0x549b10; DrawDirtyStuff, 0x5446f0), and
	# whether that draw was skipped, mLastDrawWasEmpty at +0x459.
	run=pilot-$N
	R=$W/build/origrun/$run
	rm -rf "${R:?}"
	if [ "${PILOT_HOST:-off}" = on ]; then
		# On the host through origrun's in-game frame reader (docs/ORIGRUN.md), off the sandbox's queue: the copy staged
		# from the install on a short path, the pilot's patches (origrun's list, which seedpatch.py's match; without the
		# ChapterLoadScreen patch when the hints are drawn), the virtual clock, the script's stills only (hostscript.py).
		# The game reads the user's own WinBAD key and users folder: backed up, set to the start state and put back
		# (winbad.py), one run at a time, never beside a 3D run (the lock winbad.lock, which vis3d.sh takes too).
		if [ -n "$HOLDP" ]; then cp "$W/tools/origrun/patches/bookwormadv.txt" "$P/patches-$N.txt"
		else grep -v "va:0x42efa0" "$W/tools/origrun/patches/bookwormadv.txt" > "$P/patches-$N.txt"; fi
		python "$T/hostscript.py" "$SC/$name.orig.txt" "$SC/$name.host.txt" || exit 1
		sed "s/^ScreenMode=dword:0/ScreenMode=dword:${FULL}/" "$P/portuser0/registry.ini" > "$P/start.ini"
		python "$T/inireg.py" "$P/start.ini" "$P/start.reg"
		H=$P/hostsaves; rm -rf "$H"; mkdir -p "$H"
		cp "$P/backup/users/Allison.bwa" "$P/portuser0/users/Tester.bwa" "$H/"
		[ -n "${PILOT_PROFILE:-}" ] && cp "$PILOT_PROFILE" "$H/Tester.bwa"
		for f in ${PILOT_USERS:-}; do cp "$f" "$H/"; done
		until mkdir "$P/winbad.lock" 2> /dev/null; do sleep 15; done
		python "$T/winbad.py" stage "$(cygpath -w "$P/hostbackup")" "$(cygpath -w "$P/start.reg")" $(for f in "$H"/*; do cygpath -w "$f"; done) || { rmdir "$P/winbad.lock"; exit 1; }
		"$W/build/origrun-bin/origrun.exe" --game bookwormadv --stage-root 'E:\byo-og' --run "$run" --script "$(cygpath -w "$SC/$name.host.txt")" \
			--patches "$(cygpath -w "$P/patches-$N.txt")" --seed $SEED --clock virtual --3d off --counter "[0x8259e4]+0x484" --pending-draw "[0x8259e4]+0x45a" --update-state "[0x8259e4]+0x488" --draw-empty "[0x8259e4]+0x459" --watch "[[0x8259e4]+0x83c]+0xb0" "${WATCH23[@]}" --update-ms 10 --timeout ${PILOT_TIMEOUT:-240} \
			--reader-surface "[[0x8259e4]+0x36c]+0x68" --reader-waits 54a024 ${PILOT_READER_TRIAL:+--reader-trial} > "$P/$N.orig.log" 2>&1
		python "$T/winbad.py" restore "$(cygpath -w "$P/hostbackup")"; rmdir "$P/winbad.lock"
		grep -i "RESULT\|refused\|timeout\|error" "$P/$N.orig.log" | tail -6
		OLOG=$R/logs/origrun.log
	else
		# a sandbox run's exe lies inside its run folder: the prepared copy (exe.txt) is copied there for each run
		mkdir -p "$R"
		cp -r "$(dirname "$(cygpath -u "$EXE")")" "$R/game"
		GEXE=$R/game/$(basename "$(cygpath -u "$EXE")")
		# the same seeds on both sides: the copy's clock reads in SexyAppBase::Init become constants (seedpatch.py)
		python "$T/seedpatch.py" "$GEXE" $SEED $HOLDP | tee "$P/$name.seedpatch.log"
		if [ "${PILOT_SOUND:-off}" = on ]; then
			mv "$R/game/bass.dll" "$R/game/bass_real.dll" && cp "$W/build/bassproxy-bwa/bass.dll" "$R/game/bass.dll" || exit 1
		fi
		# The original runs inside Windows Sandbox (docs/ORIGRUN.md, "Windows Sandbox"), where every run is a first run, so
		# the start state goes in as files and registry presets: Allison.bwa (the user's) and the pilot's Tester.bwa in
		# the game's users folder on the sandbox's C:, and the port's start values (portuser0/registry.ini: LastUser
		# Tester, Is3D 0) as a .reg file, which the game reads there as SYSTEM's HKCU.
		U="$R/sandbox-files/C/ProgramData/PopCap Games/WinBAD/users"
		mkdir -p "$U"
		cp "$P/backup/users/Allison.bwa" "$P/portuser0/users/Tester.bwa" "$U/"
		[ -n "${PILOT_PROFILE:-}" ] && cp "$PILOT_PROFILE" "$U/Tester.bwa"
		for f in ${PILOT_USERS:-}; do cp "$f" "$U/"; done
		sed "s/^ScreenMode=dword:0/ScreenMode=dword:${FULL}/" "$P/portuser0/registry.ini" > "$P/start.ini"
		python "$T/inireg.py" "$P/start.ini" "$P/start.reg"
		"$W/build/origrun-bin/origrun.exe" --sandbox --exe "$(cygpath -w "$GEXE")" --run "$run" --script "$(cygpath -w "$SC/$name.orig.txt")" \
			--reg "$(cygpath -w "$P/start.reg")" "${MODE3D[@]}" --3d-key "Software\PopCap\WinBAD" --3d-flag "[[0x8259e4]+0x36c]+0x38" --counter "[0x8259e4]+0x484" --pending-draw "[0x8259e4]+0x45a" --update-state "[0x8259e4]+0x488" --draw-empty "[0x8259e4]+0x459" --watch "[[0x8259e4]+0x83c]+0xb0" "${WATCH23[@]}" --update-ms 10 --timeout ${PILOT_TIMEOUT:-240} > "$P/$N.orig.log" 2>&1
		grep -i "RESULT\|timeout\|error\|msgbox\|dialog" "$P/$N.orig.log" | tail -5
		OLOG=$R/logs/origrun-sandbox.log
	fi
	# How many updates each chapter load's screen stayed up in the original, for the port's -chapterloadupdates: the
	# original loads on a thread, so the load's length in updates, and with it the hint draws (one Rand() every 50
	# updates), follow the machine's speed. origrun watches the screen's mTimer (app+0x83c, +0xb0); loads.py counts.
	python "$T/loads.py" "$OLOG" > "$P/$N.loads"
	echo "chapter loads (updates): $(cat "$P/$N.loads")"
	# and the startup load's, for -startloadupdates (the thread's start, app+0x4f9, to mLoadingDone, +0x919)
	[ -z "${PILOT_WATCH:-}" ] && python "$T/loads.py" "$OLOG" --start > "$P/$N.startload"
	echo "startup load (updates): $(cat "$P/$N.startload")"
	# the update at each of the script's marks (gen.py's `loaded` steps), from which the port's lines after a load count
	python "$T/anchors.py" marks "$OLOG" > "$P/$N.marks"
	[ -s "$P/$N.marks" ] && echo "marks: $(cat "$P/$N.marks")"
	[ "${PILOT_SOUND:-off}" = on ] && [ -f "$R/logs/sound.log" ] && cp "$R/logs/sound.log" "$P/$N.sound.log"
	[ -f "$R/game/basslog.txt" ] && cp "$R/game/basslog.txt" "$P/$N.basslog.txt" && echo "BASS calls: $(grep -vc '^#' "$P/$N.basslog.txt") lines"
	S=$W/build/origrun/$run/shots
	python - "$S" "$PO" "$ST" <<'EOF'
import os, shutil, sys, json
src, dst, steps = sys.argv[1:4]
clips = {}
for line in open(steps):
    w = line.split("#", 1)[0].split()
    if w and w[0] == "title": clips["title"] = int(w[2])
    if w and w[0] == "clip": clips[w[1]] = int(w[3])
# an earlier run's pictures of these steps go first, so a run that took none leaves none behind
names = [l.split()[1] for l in open(steps) if l.split()[:1] in (["shot"], ["clip"])]
names += list(clips) + [os.path.basename(steps)[:-4] + "_focus"]
for f in os.listdir(dst):
    stem = f[:-4] if f.endswith(".png") else f
    if any(stem == n or stem.startswith(n + "_") for n in names):
        p = os.path.join(dst, f)
        shutil.rmtree(p) if os.path.isdir(p) else os.remove(p)
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
	d=$P/portrun/$N; mkdir -p "$d"
	user() { # the port's start state in $1
		rm -rf "$1"; bash "$T/reset.sh" "$1"
		[ -n "$SUF" ] && sed -i 's/^Is3D=dword:0/Is3D=dword:1/' "$1/registry.ini"
		sed -i "s/^ScreenMode=dword:0/ScreenMode=dword:${FULL}/" "$1/registry.ini"
		[ -n "${PILOT_PROFILE:-}" ] && cp "$PILOT_PROFILE" "$1/users/Tester.bwa"
		for f in ${PILOT_USERS:-}; do cp "$f" "$1/users/"; done
	}
	user "$d/user"
	[ "${PILOT_SOUND:-off}" = on ] && export BYOPOP_SOUNDLOG="$(cygpath -w "$P/$N.portsound.log")"
	LOADS="-chapterloadupdates=$(cat "$P/$N.loads" 2>/dev/null) -startloadupdates=$(cat "$P/$N.startload" 2>/dev/null)"
	# The lines after a load count from the end of the port's own load (anchors.py), as the original's count from its
	# mark: the two can start a load a few updates apart (after the arena's treasure room), its length the original's.
	# One -fast pass, without the screenshots, for each load finds where it ends, the loads before it placed so.
	PS=$SC/$name.port.txt
	if grep -q '^@' "$PS"; then
		cp "$P/$N.marks" "$P/$N.portmarks"
		K=$(grep -o '^@[0-9]*' "$PS" | sort -u | wc -l)
		for k in $(seq 1 $K); do
			python "$T/anchors.py" fill "$PS" "$P/$N.portmarks" "$d/pass.txt" || exit 1
			grep -v ' screenshot' "$d/pass.txt" > "$d/pass2.txt"
			user "$d/passuser"; rm -f "$d/pass.log"
			timeout $((${PILOT_TIMEOUT:-240} + 60)) "$W/build/native/rundesk.exe" "$W/build/native/bookwormadv.exe" -userdir="$d/passuser" -seed=$SEED -fast -drawevery $LOADS -popcaplogo -loadanchors $HOLD ${PILOT_PORT_ARGS:-} \
				-script="$d/pass2.txt" -boardlog="$d/pass.log" > "$d/pass.out" 2>&1
			python "$T/anchors.py" ends "$d/pass.log" $k "$P/$N.portmarks" "$PS" || exit 1
		done
		echo "the port's load ends (less one): $(cat "$P/$N.portmarks")"
		python "$T/anchors.py" fill "$PS" "$P/$N.portmarks" "$SC/$name.portrun.txt" || exit 1
		PS=$SC/$name.portrun.txt
	fi
	timeout $((${PILOT_TIMEOUT:-240} + 60)) "$W/build/native/rundesk.exe" "$W/build/native/bookwormadv.exe" -userdir="$d/user" -seed=$SEED -drawevery -renderscale=1 $LOADS -popcaplogo -loadanchors $HOLD ${PILOT_PORT_ARGS:-} \
		-script="$PS" > "$d/stdout.txt" 2>&1
	echo "port exit $?"; grep "script\|FATAL" "$d/stdout.txt" | tail -5
	python - "$PP" "$ST" <<'EOF'
import os, sys, json
dst, steps = sys.argv[1:3]
for line in open(steps):
    w = line.split("#", 1)[0].split()
    if w and w[0] in ("title", "clip"):
        c, iv = ("title", w[2]) if w[0] == "title" else (w[1], w[3])
        json.dump({"interval_ms": int(iv)}, open(os.path.join(dst, c, "clip.json"), "w"))
EOF
fi
