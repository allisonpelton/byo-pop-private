#!/usr/bin/env bash
# Regression oracle (docs/CODE_MODS_DESIGN.md 9.1): runs the frozen baseline exes and the candidate exe
# over every oracle case, each run in a fresh directory, then compares the outputs with compare.py.
#
#   tools/oracle/compare.sh [--baseline <exe>] [--hooks-baseline <exe> | --no-hooks-baseline]
#                           [--candidate <exe>] [--out <dir>] [--compare-only]
#
# Three sides, each run in <out>/<side>/<case>/:
#   baseline        build/baseline/bejeweled.exe: the build before stage 0, the identity proof for the
#                   plain outputs (logs, user dir, screenshots). It has no test hooks: it gets the scripts
#                   with their "save" lines commented out, skips -autoplay runs and hooks-only cases.
#   hooks-baseline  build/baseline-stage0/bejeweled.exe: the frozen stage-0 build, the reference for the
#                   hook outputs too (hash.txt, snaps/, autoplay.txt). --no-hooks-baseline skips it.
#   candidate       build/native/bejeweled.exe
# A run directory holds script.txt, stdout.txt (TRACE), stderr.txt (SDL_Log), exit.txt, user/ (the user
# dir), shots/, snaps/ ("save" command), mods/ (an empty -modsdir; the examples case uses the real mods/
# and leaves modsdir-changed.txt if that folder changed), hash.txt (-tracehash) and autoplay.txt
# (-autoplaylog). exes.txt records each exe's sha256 and its DLLs'. The comparison, plus the candidate's
# check against tools/oracle/expect.txt, is printed and written to <out>/report.txt; the exit status is 0
# only if everything matches. Keep <out> under build/ (git-ignored): it holds user dirs and screenshots
# made from the game files. Run all exes on the same day: hiscores.dat stores day numbers (compare.py).
#
# The scripts: tools/oracle/scripts/playtest*.txt (the original playtests, in the old run.sh/runm.sh
# order and seeds) and tools/oracle/scripts/stage0/*.txt.
set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BASE_EXE="$ROOT/build/baseline/bejeweled.exe"
HOOKS_EXE="$ROOT/build/baseline-stage0/bejeweled.exe"
CAND_EXE="$ROOT/build/native/bejeweled.exe"
OUT="$ROOT/build/oracle"
COMPARE_ONLY=0
while [ $# -gt 0 ]; do
	case "$1" in
		--baseline) BASE_EXE="$2"; shift 2 ;;
		--hooks-baseline) HOOKS_EXE="$2"; shift 2 ;;
		--no-hooks-baseline) HOOKS_EXE=""; shift ;;
		--candidate) CAND_EXE="$2"; shift 2 ;;
		--out) OUT="$(mkdir -p "$2" && cd "$2" && pwd)"; shift 2 ;;
		--compare-only) COMPARE_ONLY=1; shift ;;
		*) sed -n '2,24p' "$0"; exit 2 ;;
	esac
done

winpath() { if command -v cygpath > /dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi; }
PY="${PYTHON:-python}"
COMPARE_PY="$(winpath "$ROOT/tools/oracle/compare.py")"
EXPECT="$(winpath "$ROOT/tools/oracle/expect.txt")"
PT="$ROOT/tools/oracle/scripts"
S0="$PT/stage0"
ASSETS_W="$(winpath "$ROOT/assets")"
MODS="$ROOT/mods"
MODS_W="$(winpath "$MODS")"

if [ $COMPARE_ONLY -eq 1 ]; then
	"$PY" "$COMPARE_PY" compare "$(winpath "$OUT")" "$EXPECT"
	exit $?
fi

SIDES=(baseline)
declare -A EXE HOOKS
EXE[baseline]=$BASE_EXE
if [ -n "$HOOKS_EXE" ]; then
	SIDES+=(hooks-baseline)
	EXE[hooks-baseline]=$HOOKS_EXE
fi
SIDES+=(candidate)
EXE[candidate]=$CAND_EXE

mkdir -p "$OUT"
: > "$OUT/exes.tmp"
for side in "${SIDES[@]}"; do
	exe=${EXE[$side]}
	[ -f "$exe" ] || { echo "missing $side exe: $exe"; exit 2; }
	# absolute: every case runs in its own directory
	exe="$(cd "$(dirname "$exe")" && pwd)/$(basename "$exe")"
	EXE[$side]=$exe
	# An exe has the stage-0 hooks (save command, -tracehash, -autoplay, rclick) if it knows -tracehash
	HOOKS[$side]=no
	grep -q -a "tracehash" "$exe" && HOOKS[$side]=yes
	dlls=""
	for dll in "$(dirname "$exe")"/*.dll; do
		[ -f "$dll" ] && dlls="$dlls $(basename "$dll"):$(sha256sum "$dll" | cut -c1-16)"
	done
	printf '%s.exe=%s\n%s.hooks=%s\n%s.sha256=%s\n%s.dlls=%s\n' "$side" "$(winpath "$exe")" "$side" "${HOOKS[$side]}" \
		"$side" "$(sha256sum "$exe" | cut -d' ' -f1)" "$side" "${dlls# }" >> "$OUT/exes.tmp"
done
[ "${HOOKS[candidate]}" = yes ] || { echo "the candidate has no stage-0 hooks"; exit 2; }
if [ -n "$HOOKS_EXE" ] && [ "${HOOKS[hooks-baseline]}" != yes ]; then
	echo "the hooks baseline has no stage-0 hooks: ${EXE[hooks-baseline]}"
	exit 2
fi

rm -rf "$OUT/baseline" "$OUT/hooks-baseline" "$OUT/candidate" "$OUT/gen" "$OUT/emptymods"
mkdir -p "$OUT/gen"
for side in "${SIDES[@]}"; do mkdir -p "$OUT/$side"; done
mv "$OUT/exes.tmp" "$OUT/exes.txt"
printf 'sides=%s\n' "${SIDES[*]}" >> "$OUT/exes.txt"
: > "$OUT/cases.txt"

# The examples case loads exactly the six example mods in runm.sh's order: every other mod in mods/
# (a later addition such as hd4x-textures) is listed as disabled, so it cannot change the case.
MOD_ORDER=(true-gem mines-trilliant example-voice-lines example-relaxed example-crystal-gems bejeweled2-audio)
MODS_JSON='{ "order": ['
for m in "${MOD_ORDER[@]}"; do MODS_JSON="$MODS_JSON\"$m\", "; done
MODS_JSON="${MODS_JSON%, }], \"disabled\": ["
first=1
for d in "$MODS"/*/; do
	m=$(basename "$d")
	case " ${MOD_ORDER[*]} " in *" $m "*) continue ;; esac
	[ $first -eq 1 ] || MODS_JSON="$MODS_JSON, "
	MODS_JSON="$MODS_JSON\"$m\""
	first=0
done
MODS_JSON="$MODS_JSON] }"

mods_listing() { (cd "$MODS" && find . -printf '%P\t%y\t%s\t%T@\n' | LC_ALL=C sort); }

# run <side> <case> <seed> <script or -> [mods=examples] [from=<case>] [savegame=<file>] [strip] [nohash] [arg=<switch>]...
#   from=<case>      start from a copy of that case's user dir (same side) instead of a fresh one
#   savegame=<file>  copy <file> in as user/savegame.dat (nothing if it does not exist)
#   strip            comment out the script's "save" lines even for an exe that has the command
#   nohash           no -tracehash
run() {
	local side=$1 name=$2 seed=$3 script=$4
	shift 4
	local exe=${EXE[$side]} hooks=${HOOKS[$side]}
	local dir="$OUT/$side/$name" modsdir=mods mods=none from="" savegame="" strip=no
	local args=() hash=(-tracehash=hash.txt)
	for opt in "$@"; do
		case "$opt" in
			mods=examples) mods=examples; modsdir="$MODS_W" ;;
			from=*) from="${opt#from=}" ;;
			savegame=*) savegame="${opt#savegame=}" ;;
			strip) strip=yes ;;
			nohash) hash=() ;;
			arg=*) args+=("${opt#arg=}") ;;
		esac
	done

	rm -rf "$dir"
	mkdir -p "$dir/shots" "$dir/snaps" "$dir/mods"
	if [ -n "$from" ]; then
		cp -r "$OUT/$side/$from/user" "$dir/user"
	else
		# as build/native/playtest/run.sh: windowed
		mkdir -p "$dir/user"
		printf '; test settings\n\n[SOFTWARE\Steam\Bejeweled]\nScreenMode=dword:0\n' > "$dir/user/registry.ini"
	fi
	if [ "$mods" = examples ]; then
		printf '%s\n' "$MODS_JSON" > "$dir/user/mods.json"
		mods_listing > "$OUT/gen/mods-before.txt"
	fi
	if [ -n "$savegame" ] && [ -f "$savegame" ]; then
		cp "$savegame" "$dir/user/savegame.dat"
	fi
	if [ "$script" != - ]; then
		if [ "$hooks" = yes ] && [ "$strip" = no ]; then
			cp "$script" "$dir/script.txt"
		else
			"$PY" "$COMPARE_PY" strip-saves "$(winpath "$script")" "$(winpath "$dir/script.txt")"
		fi
		args+=("-script=script.txt")
	fi

	# Separate files: stderr is block-buffered when redirected, so in one file its chunks would split
	# stdout lines. exit 124: killed after ORACLE_TIMEOUT seconds (default 600; a -fast run takes seconds)
	(cd "$dir" && timeout "${ORACLE_TIMEOUT:-600}" "$exe" -gamedir="$ASSETS_W" -userdir=user -modsdir="$modsdir" \
		-seed="$seed" -fast -trace "${hash[@]}" "${args[@]}" > stdout.txt 2> stderr.txt; echo $? > exit.txt)
	if [ "$mods" = examples ]; then
		mods_listing > "$OUT/gen/mods-after.txt"
		cmp -s "$OUT/gen/mods-before.txt" "$OUT/gen/mods-after.txt" ||
			diff "$OUT/gen/mods-before.txt" "$OUT/gen/mods-after.txt" > "$dir/modsdir-changed.txt"
	fi
	echo "  $side $name: exit $(cat "$dir/exit.txt")"
}

# all <case> <kind> <description> <seed> <script> [hooksonly] [options]: the same run with every exe
# (hooksonly: only the exes with the stage-0 hooks, for a script that uses a stage-0 command)
all() {
	local name=$1 kind=$2 desc=$3 flags=""
	shift 3
	local seed=$1 script=$2
	shift 2
	[ "${1:-}" = hooksonly ] && { flags=hooksonly; shift; }
	printf '%s\t%s\t%s\t%s\n' "$name" "$kind" "$desc" "$flags" >> "$OUT/cases.txt"
	for side in "${SIDES[@]}"; do
		[ "$flags" = hooksonly ] && [ "${HOOKS[$side]}" != yes ] && continue
		run "$side" "$name" "$seed" "$script" "$@"
	done
}

# plain <case> <seed> <script> [options]: the candidate without any stage-0 switch, compared with the
# baselines' runs of <case>
plain() {
	local name=$1
	shift
	printf '%s\t%s\t%s\t\n' "$name.plain" plain "the candidate without -tracehash against the baselines' $name" >> "$OUT/cases.txt"
	run candidate "$name.plain" "$@" nohash
}

# autoplay <case> <description> <seed> <switches>...: a bot run on every exe with the hooks, then the
# candidate's recorded input replayed as a -script by every exe
autoplay() {
	local name=$1 desc=$2 seed=$3
	shift 3
	local args=()
	for a in "$@"; do args+=("arg=$a"); done
	printf '%s\t%s\t%s\thooksonly\n' "$name" autoplay "$desc" >> "$OUT/cases.txt"
	for side in "${SIDES[@]}"; do
		[ "${HOOKS[$side]}" = yes ] && run "$side" "$name" "$seed" - "${args[@]}" arg=-autoplaylog=autoplay.txt
	done
	all "$name.replay" replay "$name, its recorded input as a -script" "$seed" "$OUT/candidate/$name/autoplay.txt"
}

for side in "${SIDES[@]}"; do echo "$side: ${EXE[$side]} (stage-0 hooks: ${HOOKS[$side]})"; done

# The existing oracle (run.sh / runm.sh order and seeds)
all main script "playtest.txt: normal game, swaps, hint, options, timed game, pause, time up, high score" 1 "$PT/playtest.txt"
all load load "playtest_load.txt in a copy of main's user dir: loads the game the mode switch saved" 1 "$PT/playtest_load.txt" from=main
all level script "playtest_level.txt: four moves finish level 1, vortex, wipe, level 2" 3 "$PT/playtest_level.txt"
all mods script "playtest_mods.txt: the six example mods (TRUE first, others disabled), cascade, hint" 3 "$PT/playtest_mods.txt" mods=examples
# ... and the candidate without -tracehash, as a user would run it
plain main 1 "$PT/playtest.txt"
plain load 1 "$PT/playtest_load.txt" from=main.plain
plain level 3 "$PT/playtest_level.txt"
plain mods 3 "$PT/playtest_mods.txt" mods=examples

# Stage 0 scripts
all drag_swaps script "drags in four directions, off every edge, below the threshold, tie, illegal" 1 "$S0/drag_swaps.txt"
all input_edges script "click zone at major 88/89, 21/22 and minor 26/27; drags of 15/16 px" 1 "$S0/input_edges.txt"
all rclick script "right button: deselects, unpauses (rclick, a stage-0 script command)" 1 "$S0/rclick.txt" hooksonly
all falling_select script "selection while falling, clearing and swapping back; a dying selected gem" 1 "$S0/falling_select.txt"
all options_highscores script "Options -> High Scores mid-game (HighScoreWidget Rand()%9), then a refill" 1 "$S0/options_highscores.txt"
all pause script "user pause while playing, mid-swap and falling; input delay; play seconds" 1 "$S0/pause.txt"
all nomoves_normal script "normal mode: no moves -> game over, name dialog (Cancel), play again" 1 "$S0/nomoves_normal.txt"
all timed_fallout script "timed: no-moves fall-out, stale selection swapped back into the grid, timed level 2" 1 "$S0/timed_fallout.txt"
all timed_fallout_load load "load_resume.txt in a copy of timed_fallout's user dir: the timed level-2 save" 1 "$S0/load_resume.txt" from=timed_fallout
all mode_switch script "timed game -> Normal: No, then Yes saves the timed game" 1 "$S0/mode_switch.txt"
all mode_switch_load load "load_resume.txt in a copy of mode_switch's user dir: loads the timed game" 1 "$S0/load_resume.txt" from=mode_switch

# Save snapshots: the script with its save lines, the same without them (candidate), each snapshot
# against a shutdown at that update, and each snapshot loaded
all save_states script "save snapshots while clearing, falling, swapping, swapping back, vortex, wipe" 3 "$S0/save_states.txt"
printf '%s\t%s\t%s\t\n' save_states.nosave nosave "save_states with its save lines commented out" >> "$OUT/cases.txt"
run candidate save_states.nosave 3 "$S0/save_states.txt" strip
SNAPS=$("$PY" "$COMPARE_PY" quit-cuts "$(winpath "$S0/save_states.txt")" "$(winpath "$OUT/gen/save_states")" | tr -d '\r')
while read -r snap update; do
	[ -n "$snap" ] || continue
	all "save_states.quit.$snap" quitcut "save_states snapshot $snap: quit at update $update" 3 "$OUT/gen/save_states/$snap.txt"
done <<< "$SNAPS"
while read -r snap update; do
	[ -n "$snap" ] || continue
	printf '%s\t%s\t%s\t\n' "save_states.load.$snap" load "load_resume.txt with the $snap save (update $update)" >> "$OUT/cases.txt"
	for side in "${SIDES[@]}"; do
		# an exe without the save command loads the save of its quit at that update instead
		save="$OUT/$side/save_states/snaps/$snap.dat"
		[ "${HOOKS[$side]}" = yes ] || save="$OUT/$side/save_states.quit.$snap/user/savegame.dat"
		run "$side" "save_states.load.$snap" 1 "$S0/load_resume.txt" savegame="$save"
	done
done <<< "$SNAPS"

# Autoplay
autoplay autoplay_normal "-autoplay=1: normal game to no moves (level 2), quit 100 updates into the high-score screen" 1 -autoplay=1
autoplay autoplay_timed "-autoplay=2 -autoplaytimed: timed game to time up (level 10): fall-outs, stale selections" 1 -autoplay=2 -autoplaytimed
autoplay autoplay_cap "-autoplay=7 -autoplaycap=6000, seed 2: quits at the cap, mid-game (savegame.dat)" 2 -autoplay=7 -autoplaycap=6000

echo
"$PY" "$COMPARE_PY" compare "$(winpath "$OUT")" "$EXPECT"
