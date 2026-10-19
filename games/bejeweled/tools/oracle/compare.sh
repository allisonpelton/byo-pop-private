#!/usr/bin/env bash
# Regression oracle (games/bejeweled/docs/CODE_MODS_DESIGN.md 9.1): runs the frozen baseline exes and the
# candidate exe over every oracle case, each run in a fresh directory, then compares the outputs with
# compare.py. Paths below are from the repository root; relative arguments are from the current directory.
#
#   games/bejeweled/tools/oracle/compare.sh [--baseline <exe>] [--hooks-baseline <exe> | --no-hooks-baseline]
#       [--candidate <exe>] [--candidate-args "<switches>"] [--out <dir>] [--compare-only]
#
# --candidate-args adds switches to every candidate run, e.g. "-forcevariant" (CODE_MODS_DESIGN.md 9.1: the
# generalised paths at default rules must match the baselines too). Every run reads the game files in
# assets/bejeweled/.
#
# Three sides, each run in <out>/<side>/<case>/:
#   baseline        build/baseline/bejeweled.exe: the build before stage 0, the identity proof for the
#                   plain outputs (logs, user dir, screenshots). It has no test hooks: it gets the scripts
#                   with their "save" lines commented out, skips -autoplay runs and hooks-only cases.
#   hooks-baseline  build/baseline-stage0/bejeweled.exe: the frozen stage-0 build, the reference for the
#                   hook outputs too (hash.txt, snaps/, autoplay.txt). --no-hooks-baseline skips it.
#   candidate       build/native/bejeweled.exe
# A run directory holds script.txt, stdout.txt (TRACE), stderr.txt (SDL_Log), exit.txt, user/ (the user
# dir), shots/, snaps/ ("save" command), mods/ (an empty -modsdir; the examples case uses the real
# games/bejeweled/mods/ and leaves modsdir-changed.txt if that folder changed), hash.txt (-tracehash) and
# autoplay.txt (-autoplaylog). exes.txt records each exe's sha256 and its DLLs'. The comparison, plus the
# candidate's check against expect.txt (next to this script), is printed and written to <out>/report.txt;
# the exit status is 0 only if everything matches. Keep <out> under build/ (git-ignored): it holds user
# dirs and screenshots made from the game files. Run all exes on the same day: hiscores.dat stores day
# numbers (compare.py).
#
# The scripts (in games/bejeweled/tools/oracle/): scripts/playtest*.txt (the original playtests, in the old
# run.sh/runm.sh order and seeds) and scripts/stage0/*.txt. Stage 1 adds an "inert" case: the candidate
# with the rule mods of mods/stage1 plays exactly like a reference run (rules are read, merged, checked and
# reported, never played) and leaves mods-status.json. Stage 2 (set seed) adds candidate runs with
# -gameseed and the fixtures of mods/stage2 and scripts/stage2, checked against a model of the board RNG
# (compare.py "seeded", "sameboard", "samegrids"), the -gameseed refusals and
# games/bejeweled/tools/check_rng_use.py. Stage 2b (Auto LiveSplit) runs the playtest, stage-0 and autoplay
# cases with games/bejeweled/mods/livesplit and with its inert twin (mods/stage2b/off): identical outputs,
# and the block's -livesplitlog against hash.txt (compare.py "livesplit"); plus check_asl_offsets.py. Stage 3
# (replays) adds candidate runs with the replays feature mod of mods/stage3, each checked against the
# candidate's run of its case without mods (compare.py "recording": recording is inert, 9.1 item 5), and a
# seeded game quit and resumed (scripts/stage3; "samestream", "seeded"). Stage 3b (watching) plays every
# recording back in the game (-replay, compare.py "playback": 9.1 item 6 in the game), and replays broken on
# purpose, which the player must call corrupt. After the stage 3 review: recordings without the hover channel
# and resumed under the other setting, a quit mid-swap, a save that replaces another board's, and Options ->
# High Scores while watching. With both: a watch with the livesplit mod (a playback board is never a run).
#
# The frozen exes (build/baseline, build/baseline-stage0) predate the audio shutdown fix (af758b9) and can
# crash on exit after writing all their outputs: a baseline or hooks-baseline run that exits 139 or with an
# access violation (3221225477 = 0xC0000005, or its 8-bit form 5) is run once more, and report.txt notes it
# (<out>/<side>/<case>/retried.txt). The candidate is never retried.
set -u

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"  # the repository root
GAME="$ROOT/games/bejeweled"
BASE_EXE="$ROOT/build/baseline/bejeweled.exe"
HOOKS_EXE="$ROOT/build/baseline-stage0/bejeweled.exe"
CAND_EXE="$ROOT/build/native/bejeweled.exe"
CAND_ARGS=()
OUT="$ROOT/build/oracle"
COMPARE_ONLY=0
ARGS=("$@")  # for the re-run under rundesk below: the loop consumes them
while [ $# -gt 0 ]; do
	case "$1" in
		--baseline) BASE_EXE="$2"; shift 2 ;;
		--hooks-baseline) HOOKS_EXE="$2"; shift 2 ;;
		--no-hooks-baseline) HOOKS_EXE=""; shift ;;
		--candidate) CAND_EXE="$2"; shift 2 ;;
		--candidate-args) read -r -a CAND_ARGS <<< "$2"; shift 2 ;;
		--out) OUT="$(mkdir -p "$2" && cd "$2" && pwd)"; shift 2 ;;
		--compare-only) COMPARE_ONLY=1; shift ;;
		*) sed -n '2,49p' "$0"; exit 2 ;;
	esac
done

# Every game this script starts runs on a private, invisible desktop (tools/rundesk): test windows are
# never shown and never take focus from the user. Children inherit the desktop, so re-running this script
# under rundesk covers every exe it starts. ORACLE_VISIBLE=1 runs on the user's desktop instead.
RUNDESK="$ROOT/build/native/rundesk.exe"
if [ -z "${BEJ_TEST_DESKTOP:-}" ] && [ -z "${ORACLE_VISIBLE:-}" ]; then
	if [ ! -x "$RUNDESK" ]; then
		echo "compare.sh: $RUNDESK is missing; build it (cmake --build build/native --target rundesk) or set ORACLE_VISIBLE=1" >&2
		exit 2
	fi
	export BEJ_TEST_DESKTOP=1
	exec "$RUNDESK" bash "$0" "${ARGS[@]}"
fi

winpath() { if command -v cygpath > /dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi; }
PY="${PYTHON:-python}"
COMPARE_PY="$(winpath "$GAME/tools/oracle/compare.py")"
EXPECT="$(winpath "$GAME/tools/oracle/expect.txt")"
PT="$GAME/tools/oracle/scripts"
S0="$PT/stage0"
ASSETS_W="$(winpath "$ROOT/assets/bejeweled")"
MODS="$GAME/mods"
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
printf 'candidate.args=%s\n' "${CAND_ARGS[*]}" >> "$OUT/exes.txt"
: > "$OUT/cases.txt"

# The examples case loads exactly the six example mods in runm.sh's order: every other mod in
# games/bejeweled/mods/ (a later addition such as hd4x-textures) is listed as disabled, so it cannot
# change the case.
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

# run <side> <case> <seed> <script or -> [mods=examples] [modsdir=<dir>] [from=<case>] [savegame=<file>] [strip]
#     [nohash] [dumprules] [fakereplays] [replay=<file>] [arg=<switch>]...
#   modsdir=<dir>    use the mods in <dir> (all enabled, in their priority order; no mods.json)
#   dumprules        afterwards, the same exe with -dumprules into dumprules.txt
#   from=<case>      start from a copy of that case's user dir (same side) instead of a fresh one
#   savegame=<file>  copy <file> in as user/savegame.dat (nothing if it does not exist)
#   strip            comment out the script's "save" lines even for an exe that has the command
#   nohash           no -tracehash
#   fakereplays      ten old replays in user/replays first, one of them pinned (compare.py fake-replays)
#   replay=<file>    watch a copy of <file> (replay.bjr in the run directory): -replay=replay.bjr -replayquit
# A frozen reference exe (baseline, hooks-baseline) that crashes at exit (exit 139, or an access violation
# 0xC0000005 as 3221225477, -1073741819 or its low byte 5) is run once more from scratch: those builds predate
# the audio shutdown fix (af758b9) and can crash after writing all their outputs. The second run is the one
# compared; retried.txt keeps the first exit status for compare.py's report. The candidate gets no retry.
run() {
	local side=$1 name=$2
	run_once "$@"
	[ "$side" = candidate ] && return
	local code
	code=$(tr -d '\r\n' < "$OUT/$side/$name/exit.txt")
	case "$code" in
		139|5|3221225477|-1073741819)
			echo "  $side $name: exit $code, a crash at exit (frozen exe without af758b9): run once more"
			run_once "$@"
			printf '%s\n' "$code" > "$OUT/$side/$name/retried.txt"
			;;
	esac
}
run_once() {
	local side=$1 name=$2 seed=$3 script=$4
	shift 4
	local exe=${EXE[$side]} hooks=${HOOKS[$side]}
	local dir="$OUT/$side/$name" modsdir=mods mods=none from="" savegame="" strip=no dumprules=no fakereplays=no replay=""
	local args=() hash=(-tracehash=hash.txt)
	[ "$side" = candidate ] && args+=("${CAND_ARGS[@]}")
	for opt in "$@"; do
		case "$opt" in
			mods=examples) mods=examples; modsdir="$MODS_W" ;;
			modsdir=*) modsdir="$(winpath "${opt#modsdir=}")" ;;
			dumprules) dumprules=yes ;;
			from=*) from="${opt#from=}" ;;
			savegame=*) savegame="${opt#savegame=}" ;;
			strip) strip=yes ;;
			nohash) hash=() ;;
			fakereplays) fakereplays=yes ;;
			replay=*) replay="${opt#replay=}" ;;
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
	if [ "$fakereplays" = yes ]; then
		"$PY" "$COMPARE_PY" fake-replays "$(winpath "$dir/user/replays")"
	fi
	if [ -n "$replay" ]; then
		cp "$replay" "$dir/replay.bjr"
		args+=(-replay=replay.bjr -replayquit)
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
	if [ "$dumprules" = yes ]; then
		(cd "$dir" && timeout 60 "$exe" -gamedir="$ASSETS_W" -userdir=user -modsdir="$modsdir" -dumprules \
			> dumprules.txt 2> dumprules-stderr.txt)
	fi
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

# inert <case> <description> <flags> <seed> <script> [options]: a run of the candidate only (the baselines
# know no rules), which must play exactly like the candidate's run of the case its flags name ("ref=<case>")
inert() {
	local name=$1 desc=$2 flags=$3
	shift 3
	printf '%s\t%s\t%s\t%s\n' "$name" inert "$desc" "$flags" >> "$OUT/cases.txt"
	run candidate "$name" "$@"
}

# seeded <case> <description> <flags> <seed> <script or -> [options]: a run of the candidate only, checked
# against compare.py's model of the board RNG (compare.py, "seeded"; flags "k=v;k=v")
seeded() {
	local name=$1 desc=$2 flags=$3
	shift 3
	printf '%s\t%s\t%s\t%s\n' "$name" seeded "$desc" "$flags" >> "$OUT/cases.txt"
	run candidate "$name" "$@"
}

# cross <case> <kind> <description> <flags> <seed> <script or -> [options]: a candidate run that compare.py
# checks against another candidate run (kinds sameboard, repeat)
cross() {
	local name=$1 kind=$2 desc=$3 flags=$4
	shift 4
	printf '%s\t%s\t%s\t%s\n' "$name" "$kind" "$desc" "$flags" >> "$OUT/cases.txt"
	run candidate "$name" "$@"
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

# Stage 2: set seed. A board owns an MTRand for a seed the player chose (-gameseed), for any rule, and for
# the seeds.perGame feature (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.2). The baselines ignore -gameseed,
# so these are candidate runs, checked against compare.py's model of the board RNG and against each other.
S2="$PT/stage2"
M2="$GAME/tools/oracle/mods/stage2"
GS=4242
SEEDBOT=(arg=-gameseed=$GS arg=-autoplay=1 arg=-autoplaylog=autoplay.txt arg=-autoplayshot=shots/gameover.png)
# per-game seeds: playtest.txt with a seeds.perGame feature mod (no rules): every NewGame draws the board's
# seed from the global stream, the scores stay in the normal table
seeded pergame "playtest.txt with a seeds.perGame feature mod: each game's seed is a global draw in NewGame" \
	"boards=pergame,pergame,pergame;rngseed=1" 1 "$PT/playtest.txt" modsdir="$M2/pergame"
# Stage 1's rule mods: read, merged, checked and reported (mods-status.json, -dumprules), still not played;
# since stage 2 any rule gives the board its own RNG, so they play exactly like the per-game-seed reference
inert rules_inert "playtest.txt with the rule, feature and imageOps mods of games/bejeweled/tools/oracle/mods/stage1: plays exactly like pergame" \
	"ref=pergame;boards=pergame,pergame,pergame;rngseed=1" 1 "$PT/playtest.txt" modsdir="$GAME/tools/oracle/mods/stage1" dumprules
cross rules_inert.repeat repeat "rules_inert again: the same game under the same -seed" "" \
	1 "$PT/playtest.txt" modsdir="$GAME/tools/oracle/mods/stage1"
inert rules_only "playtest.txt with one rule mod and no features: a rule alone gives the board its per-game seed" \
	"ref=pergame;boards=pergame,pergame,pergame;rngseed=1" 1 "$PT/playtest.txt" modsdir="$M2/rules" dumprules
# the seed field alone (features.seeds {"entry": true}; the object form leaves perGame off): every game stays
# on the global stream, exactly like main
inert seeds_entry "playtest.txt with a features.seeds {entry: true} mod: perGame stays off, it plays exactly like main" \
	"ref=main" 1 "$PT/playtest.txt" modsdir="$M2/entry" dumprules
# a seed the player chose, normal mode: the bot plays it to game over (SEED line, the seeded table)
seeded seed_normal "-gameseed=$GS -autoplay=1: the startup boards get the seed, a normal game on it to game over" \
	"boards=user;gameseed=$GS;shot=shots/gameover.png;plainshot=nomoves_normal/shots/n05_high_score_screen.png" \
	1 - "${SEEDBOT[@]}"
cross seed_normal.seed2 sameboard "seed_normal with -seed=2: the same game, only the global stream differs" \
	"ref=seed_normal" 2 - "${SEEDBOT[@]}"
seeded seed_max "-gameseed=4294967295: the largest seed, another board" \
	"boards=user;gameseed=4294967295;differs=seed_normal" 1 "$S2/seed_start.txt" arg=-gameseed=4294967295
seeded seed_resume "-gameseed=31337 in main's user dir: resume the save (the seed stays pending), then a timed game on it" \
	"boards=-,user;gameseed=31337;shot=shots/s05_timed_game_over.png" 1 "$S2/seed_resume.txt" from=main arg=-gameseed=31337
seeded seed_keyed "-gameseed=$GS with a tunables-only mod: original rules, the seeded table hiscores-<key>-seeded.dat" \
	"boards=user;gameseed=$GS" 1 "$S2/seed_keyed.txt" modsdir="$M2/keyed" arg=-gameseed=$GS
# an outside draw (Options -> High Scores mid-game: the tip, GAME_NOTES 3.16) must not reach the stream of a
# board with its own RNG: the same moves with and without the dialog give the same refills
seeded seed_highscores "-gameseed=$GS: Options -> High Scores mid-game (a global draw for the tip), then two refills" \
	"boards=user;gameseed=$GS" 1 "$S2/seed_highscores.txt" arg=-gameseed=$GS
cross seed_highscores.nodialog samegrids "seed_highscores without the dialog: the same settled boards" \
	"ref=seed_highscores;after=720;table=hiscores-seeded.dat" 1 "$S2/seed_highscores_nodialog.txt" arg=-gameseed=$GS
# the seed stays pending until a game on it is played: the first swap that makes a match, or a game over
# (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10, the user's decision of 2026-10-18). Until then a mode
# switch or New Game starts on it again
seeded seed_timed "-gameseed=$GS, Timed before any move: a timed game on the seed to time up (SEED line, seeded table); play again is unseeded" \
	"boards=user,user,-;gameseed=$GS;shot=shots/t03_timed_game_over.png" 1 "$S2/seed_timed.txt" arg=-gameseed=$GS
seeded seed_match_timed "-gameseed=$GS: a match plays the seed, so Timed after it and Normal after that are unseeded" \
	"boards=user,-,-;gameseed=$GS" 1 "$S2/seed_match_timed.txt" arg=-gameseed=$GS
seeded seed_nomatch_timed "-gameseed=$GS: a swap without a match leaves the seed pending: Timed is seeded, and its match uses the seed up" \
	"boards=user,user,-;gameseed=$GS" 1 "$S2/seed_nomatch_timed.txt" arg=-gameseed=$GS
seeded seed_newgame "-gameseed=$GS: New Game on the untouched board starts the seed again; after a match New Game is unseeded" \
	"boards=user,user,-;gameseed=$GS" 1 "$S2/seed_newgame.txt" arg=-gameseed=$GS
# -gameseed values main.cpp must refuse, and rule 1 of games/bejeweled/docs/CODE_MODS_DESIGN.md 6.1 on the
# sources (check_rng_use.py)
printf '%s\t%s\t%s\t\n' gameseed_cli cli "-gameseed=0, out of range, signed, not a number or empty: refused before the game starts" >> "$OUT/cases.txt"
mkdir -p "$OUT/candidate/gameseed_cli"
: > "$OUT/candidate/gameseed_cli/cli.txt"
for bad in -gameseed=0 -gameseed=4294967296 -gameseed=99999999999999999999 -gameseed=-5 -gameseed=+5 -gameseed=12a \
		-gameseed=0x10 -gameseed= -gameseed; do
	(cd "$OUT/candidate/gameseed_cli" && timeout 60 "${EXE[candidate]}" -gamedir="$ASSETS_W" -userdir=user -modsdir=mods \
		"${CAND_ARGS[@]}" "$bad" > stdout.txt 2> stderr.txt
	 printf '%s\t%s\t%s\n' "$bad" "$?" "$(head -n 1 stderr.txt | tr -d '\r')" >> cli.txt)
done
rm -f "$OUT/candidate/gameseed_cli/stdout.txt" "$OUT/candidate/gameseed_cli/stderr.txt"
echo "  candidate gameseed_cli: $(wc -l < "$OUT/candidate/gameseed_cli/cli.txt") runs"
printf '%s\t%s\t%s\t\n' rng_rules lint "games/bejeweled/tools/check_rng_use.py: no other randomness in the game code, Board draws only through Board::Rand" >> "$OUT/cases.txt"
mkdir -p "$OUT/candidate/rng_rules"
"$PY" "$(winpath "$GAME/tools/check_rng_use.py")" > "$OUT/candidate/rng_rules/check.txt" 2>&1
echo $? > "$OUT/candidate/rng_rules/exit.txt"
echo "  candidate rng_rules: exit $(cat "$OUT/candidate/rng_rules/exit.txt")"

# Autoplay
autoplay autoplay_normal "-autoplay=1: normal game to no moves (level 2), quit 100 updates into the high-score screen" 1 -autoplay=1
autoplay autoplay_timed "-autoplay=2 -autoplaytimed: timed game to time up (level 10): fall-outs, stale selections" 1 -autoplay=2 -autoplaytimed
autoplay autoplay_cap "-autoplay=7 -autoplaycap=6000, seed 2: quits at the cap, mid-game (savegame.dat)" 2 -autoplay=7 -autoplaycap=6000

# Stage 3: replays (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.2-6.6, 7.3, 9.1 item 5). With the replays
# feature mod of mods/stage3/replays (the hover channel on, seeds {entry}, perGame off: every game is recorded
# on the global stream, XRAND for the outside draws) each case must play exactly like the candidate's run of
# it without mods, and leave only replays/, the sidecar and mods-status.json more. -codeccheck adds StateCodec's
# round trip after every update (6.6), which must be inert too.
S3="$PT/stage3"
M3="$GAME/tools/oracle/mods/stage3/replays"
# recording <case> <description> <reference case> <seed> <script or -> [options] (a modsdir= among them replaces the
# replays mod)
recording() {
	local name=$1 desc=$2 ref=$3
	shift 3
	printf '%s\t%s\t%s\t%s\n' "$name" recording "$desc" "ref=$ref" >> "$OUT/cases.txt"
	run candidate "$name" "$1" "$2" modsdir="$M3" "${@:3}" arg=-codeccheck=codec.txt
}
recording rec_main "playtest.txt recorded: the timed game a finished replay (time up), the normal game a sidecar (saved)" \
	main 1 "$PT/playtest.txt"
recording rec_load "playtest_load.txt in a copy of rec_main's user dir: the resume continues the sidecar's replay (two segments)" \
	load 1 "$PT/playtest_load.txt" from=rec_main
recording rec_level "playtest_level.txt recorded: level 1, vortex, wipe, level 2" level 3 "$PT/playtest_level.txt"
recording rec_drag_swaps "drag_swaps.txt recorded: TARGET from drags, off the edges" drag_swaps 1 "$S0/drag_swaps.txt"
recording rec_falling_select "falling_select.txt recorded: SELECT while falling, a dying selected gem" falling_select 1 "$S0/falling_select.txt"
recording rec_options_highscores "options_highscores.txt recorded: the dialog's tip draw is an XRAND" options_highscores 1 \
	"$S0/options_highscores.txt"
recording rec_pause "pause.txt recorded: PAUSE/UNPAUSE, the input delay" pause 1 "$S0/pause.txt"
recording rec_nomoves_normal "nomoves_normal.txt recorded: no moves, a finished replay kept (-keep: a high score)" nomoves_normal 1 \
	"$S0/nomoves_normal.txt"
recording rec_timed_fallout "timed_fallout.txt recorded: fall-out, a stale selection swapped back, timed level 2" timed_fallout 1 \
	"$S0/timed_fallout.txt"
recording rec_timed_fallout_load "load_resume.txt in a copy of rec_timed_fallout's user dir: the timed level-2 resume" \
	timed_fallout_load 1 "$S0/load_resume.txt" from=rec_timed_fallout
recording rec_mode_switch "mode_switch.txt recorded: the mode switch saves the timed game (sidecar)" mode_switch 1 "$S0/mode_switch.txt"
recording rec_mode_switch_load "load_resume.txt in a copy of rec_mode_switch's user dir" mode_switch_load 1 "$S0/load_resume.txt" \
	from=rec_mode_switch
recording rec_save_states "save_states.txt recorded: each save snapshot with its sidecar, ending in SAVE(snapshot)" save_states 3 \
	"$S0/save_states.txt"
recording rec_autoplay_normal "the -autoplay=1 game recorded: to no moves on level 2" autoplay_normal 1 - arg=-autoplay=1 \
	arg=-autoplaylog=autoplay.txt
recording rec_autoplay_timed "the -autoplay=2 timed game recorded: to time up on level 10, fall-outs and stale selections" \
	autoplay_timed 1 - arg=-autoplay=2 arg=-autoplaytimed arg=-autoplaylog=autoplay.txt
all decline load "load_no.txt in a copy of main's user dir: No in the load dialog deletes the save" 1 "$S3/load_no.txt" from=main
recording rec_decline "load_no.txt in a copy of rec_main's user dir: the declined game's sidecar becomes a finished replay (END saved)" \
	decline 1 "$S3/load_no.txt" from=rec_main
recording rec_prune "nomoves_normal.txt with ten old replays (one pinned) and keep 3: the newest 3 unpinned by the time they were written stay (not by name), every pinned one" \
	nomoves_normal 1 "$S0/nomoves_normal.txt" fakereplays
# A resumed seeded game keeps its stream (GAME_NOTES 12.3): the sidecar's RNGS record replaces stage 2's
# interim (a resume went on unseeded). seed_highscores.nodialog is the same three moves without the quit.
seeded seed_quit "-gameseed=$GS: a match plays the seed, then the quit saves: savegame.bjr keeps the board's RNG" \
	"boards=user;gameseed=$GS" 1 "$S3/seed_quit.txt" arg=-gameseed=$GS arg=-codeccheck=codec.txt
cross seed_quit.resume samestream "seed_quit's user dir, no -gameseed: Yes resumes on MTRand($GS) as saved; two more moves" \
	"first=seed_quit;ref=seed_highscores.nodialog" 1 "$S3/seed_quit_resume.txt" from=seed_quit arg=-codeccheck=codec.txt
seeded seed_quit.bot "seed_quit's user dir, no -gameseed: Yes, then the bot plays the resumed game to game over: SEED line, the seeded table" \
	"boards=user*;gameseed=$GS;shot=shots/gameover.png" 1 "$S3/load_yes.txt" from=seed_quit arg=-autoplay=1 \
	arg=-autoplaylog=autoplay.txt arg=-autoplayshot=shots/gameover.png
recording rec_seed_quit_resume "seed_quit.resume recorded: a fresh replay that holds the loaded save (LDSV) and the restored RNG" \
	seed_quit.resume 1 "$S3/seed_quit_resume.txt" from=seed_quit

# Stage 3b: recordings of games with their own RNG (a seed the player chose, per-game seeds) and of a tunables mod,
# for the player below. Each must again play exactly like its run without the replays mod.
recording rec_pergame "playtest.txt recorded with per-game seeds (mods/stage3/pergame): replays that start from a seed" pergame 1 \
	"$PT/playtest.txt" modsdir="$GAME/tools/oracle/mods/stage3/pergame"
recording rec_seed_normal "seed_normal recorded: a game on the seed the player chose, to no moves (the bot)" seed_normal 1 - \
	"${SEEDBOT[@]}"
recording rec_seed_nomatch_timed "seed_nomatch_timed recorded: a seeded normal game dropped (END abandoned), the timed game on the seed saved" \
	seed_nomatch_timed 1 "$S2/seed_nomatch_timed.txt" arg=-gameseed=$GS
recording rec_seed_highscores "seed_highscores recorded: an outside draw that does not reach a seeded board (no XRAND)" \
	seed_highscores 1 "$S2/seed_highscores.txt" arg=-gameseed=$GS
recording rec_seed_quit "seed_quit recorded: a seeded game saved at the quit, its sidecar the recording and the RNG" seed_quit 1 \
	"$S3/seed_quit.txt" arg=-gameseed=$GS
recording rec_seed_quit.bot "seed_quit.bot recorded in a copy of rec_seed_quit's user dir: resumed and played to game over, a two-segment replay" \
	seed_quit.bot 1 "$S3/load_yes.txt" from=rec_seed_quit arg=-autoplay=1 arg=-autoplaylog=autoplay.txt \
	arg=-autoplayshot=shots/gameover.png
printf '%s\t%s\t%s\t\n' keyed_hint reference "keyed_hint.txt with a tunables-only mod (hintPenalty 25): a match, a hint, the quit" \
	>> "$OUT/cases.txt"
run candidate keyed_hint 1 "$S3/keyed_hint.txt" modsdir="$M2/keyed"
recording rec_keyed_hint "keyed_hint recorded (mods/stage3/keyed: the tunables mod and the replays mod): a RULE with tunables" \
	keyed_hint 1 "$S3/keyed_hint.txt" modsdir="$GAME/tools/oracle/mods/stage3/keyed"

# Stage 3b: watching (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.8, 9.1 item 6 in the game). The candidate plays a
# copy of each recording above in a fresh user dir without mods (-replay, -codeccheck), compare.py "playback": the
# replay is ok (every CHECK and END), the TRACE settles of each segment are a run of the recording's, and nothing
# is written. Files the oracle has twice (a copy in another case's user dir, the same game recorded again) are
# played once.
C="$OUT/candidate"
# one <glob>: the one file the glob names (compare.sh stops if it names none or more)
one() {
	local hits=( $1 )
	if [ ${#hits[@]} -ne 1 ] || [ ! -f "${hits[0]}" ]; then
		echo "compare.sh: $1 names ${#hits[@]} files" >&2
		exit 2
	fi
	printf '%s' "${hits[0]}"
}
# playback <case> <description> <flags> <file> [script=<file>] [options]: watch.txt, or the script given
playback() {
	local name=$1 desc=$2 flags=$3 file=$4 script="$S3/watch.txt"
	shift 4
	if [ $# -gt 0 ] && [ "${1#script=}" != "$1" ]; then
		script="${1#script=}"
		shift
	fi
	printf '%s\t%s\t%s\t%s\n' "$name" playback "$desc" "$flags" >> "$OUT/cases.txt"
	run candidate "$name" 1 "$script" replay="$file" arg=-codeccheck=codec.txt "$@"
}
playback watch_main_timed "rec_main's timed game: time up, on the global stream (XRAND none)" "trace=rec_main" \
	"$(one "$C/rec_main/user/replays/*-timed-*.bjr")"
playback watch_main_sidecar "rec_main's sidecar: the normal game to the mode switch's save (END saved)" "trace=rec_main" \
	"$C/rec_main/user/savegame.bjr"
playback watch_load "rec_load's sidecar: playtest_load as a two-segment replay (SAVE, then a new board's LOAD of it)" \
	"trace=rec_main,rec_load" "$C/rec_load/user/savegame.bjr"
playback watch_level "rec_level's sidecar: level 1, vortex, wipe, level 2" "trace=rec_level" "$C/rec_level/user/savegame.bjr"
playback watch_drag_swaps "rec_drag_swaps' sidecar: TARGET from drags" "trace=rec_drag_swaps" "$C/rec_drag_swaps/user/savegame.bjr"
playback watch_falling_select "rec_falling_select's sidecar: SELECT while falling, a dying selected gem" "trace=rec_falling_select" \
	"$C/rec_falling_select/user/savegame.bjr"
playback watch_options_highscores "rec_options_highscores' sidecar: the dialog's tip draw replayed as an XRAND" \
	"trace=rec_options_highscores" "$C/rec_options_highscores/user/savegame.bjr"
playback watch_pause "rec_pause's sidecar: PAUSE/UNPAUSE, the input delay" "trace=rec_pause" "$C/rec_pause/user/savegame.bjr"
playback watch_nomoves_normal "rec_nomoves_normal's replay: no moves, game over" "trace=rec_nomoves_normal" \
	"$(one "$C/rec_nomoves_normal/user/replays/*-normal-*.bjr")"
playback watch_timed_fallout "rec_timed_fallout's sidecar: fall-outs, a stale selection, timed level 2" "trace=rec_timed_fallout" \
	"$C/rec_timed_fallout/user/savegame.bjr"
playback watch_timed_fallout_load "rec_timed_fallout_load's sidecar: the timed level-2 game resumed (two segments)" \
	"trace=rec_timed_fallout,rec_timed_fallout_load" "$C/rec_timed_fallout_load/user/savegame.bjr"
playback watch_mode_switch "rec_mode_switch's sidecar: the timed game the mode switch saved" "trace=rec_mode_switch" \
	"$C/rec_mode_switch/user/savegame.bjr"
playback watch_mode_switch_load "rec_mode_switch_load's sidecar: resumed (two segments)" "trace=rec_mode_switch,rec_mode_switch_load" \
	"$C/rec_mode_switch_load/user/savegame.bjr"
playback watch_save_states "rec_save_states' sidecar: to level 2" "trace=rec_save_states" "$C/rec_save_states/user/savegame.bjr"
for snap in s1_clearing s2_falling s3_swapping s4_swapping_back s5_vortex s6_wipe; do
	playback "watch_save_states.$snap" "rec_save_states' snapshot sidecar $snap.bjr: ends in SAVE(snapshot), SaveGameSnapshot again" \
		"trace=rec_save_states" "$C/rec_save_states/snaps/$snap.bjr"
done
playback watch_autoplay_normal "rec_autoplay_normal's replay: the bot's game to no moves on level 2" "trace=rec_autoplay_normal" \
	"$(one "$C/rec_autoplay_normal/user/replays/*.bjr")"
playback watch_autoplay_timed "rec_autoplay_timed's replay: the bot's timed game to time up on level 10" "trace=rec_autoplay_timed" \
	"$(one "$C/rec_autoplay_timed/user/replays/*.bjr")"
playback watch_decline "rec_decline's replay of the declined resume: rec_main's normal game, END saved" "trace=rec_main" \
	"$(one "$C/rec_decline/user/replays/*-normal-*.bjr")"
playback watch_seed_quit_resume "rec_seed_quit_resume's sidecar: a fresh replay with the loaded save (LDSV) and RNGS seed $GS" \
	"trace=rec_seed_quit_resume" "$C/rec_seed_quit_resume/user/savegame.bjr"
playback watch_pergame_timed "rec_pergame's timed game: a per-game seed, time up" "trace=rec_pergame" \
	"$(one "$C/rec_pergame/user/replays/*-timed-*.bjr")"
playback watch_pergame_sidecar "rec_pergame's sidecar: a per-game seed, the RNG the save was made with" "trace=rec_pergame" \
	"$C/rec_pergame/user/savegame.bjr"
playback watch_seed_normal "rec_seed_normal's replay: seed $GS (user), to no moves" "trace=rec_seed_normal" \
	"$(one "$C/rec_seed_normal/user/replays/*.bjr")"
playback watch_seed_abandoned "rec_seed_nomatch_timed's replay: seed $GS (user), a swap without a match, then Timed (END abandoned)" \
	"trace=rec_seed_nomatch_timed" "$(one "$C/rec_seed_nomatch_timed/user/replays/*-normal-*.bjr")"
playback watch_seed_timed "rec_seed_nomatch_timed's sidecar: the timed game on seed $GS, its match, the save at Normal" \
	"trace=rec_seed_nomatch_timed" "$C/rec_seed_nomatch_timed/user/savegame.bjr"
playback watch_seed_highscores "rec_seed_highscores' sidecar: seed $GS, an outside draw that is no XRAND" "trace=rec_seed_highscores" \
	"$C/rec_seed_highscores/user/savegame.bjr"
playback watch_seed_quit "rec_seed_quit's sidecar: seed $GS to the quit, its RNGS the board's RNG at the save" "trace=rec_seed_quit" \
	"$C/rec_seed_quit/user/savegame.bjr"
playback watch_seed_quit_bot "rec_seed_quit.bot's replay: seed $GS quit, resumed with the sidecar's RNG, played to game over" \
	"trace=rec_seed_quit,rec_seed_quit.bot" "$(one "$C/rec_seed_quit.bot/user/replays/*.bjr")"
playback watch_keyed_hint "rec_keyed_hint's sidecar: the hint costs the RULE's 25, not the session's 50" "trace=rec_keyed_hint" \
	"$(one "$C/rec_keyed_hint/user/savegame*.bjr")"
# Replays broken on purpose (compare.py corrupt-replay, from rec_pause's sidecar): the player stops at the fault
for kind in check select unpause end; do
	"$PY" "$COMPARE_PY" corrupt-replay "$(winpath "$C/rec_pause/user/savegame.bjr")" "$(winpath "$OUT/gen/corrupt_$kind.bjr")" "$kind" \
		> "$OUT/gen/corrupt_$kind.txt"
done
playback watch_corrupt_check "rec_pause's sidecar with a CHECK hash changed: the LOGIC hash differs there" "corrupt=LOGIC hash" \
	"$OUT/gen/corrupt_check.bjr"
playback watch_corrupt_select "rec_pause's sidecar with its first SELECT twice: the second fails SELECT's gate" \
	"corrupt=a selection MouseDown would not make" "$OUT/gen/corrupt_select.bjr"
playback watch_corrupt_unpause "rec_pause's sidecar without its last UNPAUSE: the board stays paused" "corrupt=no event unpauses it" \
	"$OUT/gen/corrupt_unpause.bjr"
playback watch_corrupt_end "rec_pause's sidecar with END's score changed: END differs" "corrupt=END differs" "$OUT/gen/corrupt_end.bjr"

# Stage 3, after the review. The hover channel without it (features.replays alone, mods/stage3/nohover) and a game
# resumed under the other setting: the replay's hover flag says every segment has the channel, so a mixed one has
# none, and the player checks no save bytes. A quit in the middle of a swap: the watch's last board, whose SAVE
# dropped the swap, shows no settle after the end. A save that replaces another board's (a mode switch saved the
# timed game, the quit saves the normal one): the timed game's recording is kept as a finished replay first.
# Options -> High Scores while watching a game on a seed the player chose: the player's table, no -seeded one.
M3N="$GAME/tools/oracle/mods/stage3/nohover"
recording rec_main_nohover "playtest.txt recorded without the hover channel (mods/stage3/nohover): no hover flag" main 1 \
	"$PT/playtest.txt" modsdir="$M3N"
recording rec_load_hover_on "playtest_load.txt with the hover channel in a copy of rec_main_nohover's user dir: segment 0 has none, so the replay has no hover flag" \
	load 1 "$PT/playtest_load.txt" from=rec_main_nohover
recording rec_load_hover_off "playtest_load.txt without the hover channel in a copy of rec_main's user dir: segment 1 has none, so the replay has no hover flag" \
	load 1 "$PT/playtest_load.txt" from=rec_main modsdir="$M3N"
all quit_midswap script "Seed 1: a match, then a quit in the middle of the next swap (Shutdown's SaveGame drops the swap)" 1 \
	"$S3/quit_midswap.txt"
recording rec_quit_midswap "quit_midswap.txt recorded: the sidecar ends in the two SAVEs of the quit, the first of them mid-swap" \
	quit_midswap 1 "$S3/quit_midswap.txt"
all mode_switch_match script "mode_switch.txt, then a match on the new normal game and a quit, which saves it over the timed game" 1 \
	"$S3/mode_switch_match.txt"
recording rec_mode_switch_match "mode_switch_match.txt recorded: the timed game's sidecar is kept as a finished replay (END saved) before the quit's save replaces it" \
	mode_switch_match 1 "$S3/mode_switch_match.txt"
playback watch_main_nohover "rec_main_nohover's sidecar: no hover channel, no save bytes checked" "trace=rec_main_nohover" \
	"$C/rec_main_nohover/user/savegame.bjr"
playback watch_load_hover_on "rec_load_hover_on's sidecar: segment 0 without the hover channel, segment 1 with it" \
	"trace=rec_main_nohover,rec_load_hover_on" "$C/rec_load_hover_on/user/savegame.bjr"
playback watch_load_hover_off "rec_load_hover_off's sidecar: segment 0 with the hover channel, segment 1 without it" \
	"trace=rec_main,rec_load_hover_off" "$C/rec_load_hover_off/user/savegame.bjr"
playback watch_quit_midswap "rec_quit_midswap's sidecar: its last SAVEs drop a swap; the board that stays settles no more" \
	"trace=rec_quit_midswap" "$C/rec_quit_midswap/user/savegame.bjr"
playback watch_mode_switch_kept "rec_mode_switch_match's replay of the timed game the quit's save replaced (END saved)" \
	"trace=rec_mode_switch_match" "$(one "$C/rec_mode_switch_match/user/replays/*-timed-*.bjr")"
playback watch_mode_switch_match "rec_mode_switch_match's sidecar: the normal game to the quit" "trace=rec_mode_switch_match" \
	"$C/rec_mode_switch_match/user/savegame.bjr"
playback watch_seed_quit_highscores "rec_seed_quit's sidecar (seed $GS, user) with Options -> High Scores mid-watch: the player's hiscores.dat, no -seeded table" \
	"trace=rec_seed_quit;user=hiscores.dat,registry.ini" "$C/rec_seed_quit/user/savegame.bjr" script="$S3/watch_highscores.txt"

# Stage 2b: Auto LiveSplit (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15). The candidate with the livesplit
# mod (games/bejeweled/mods/livesplit, copied into a mods folder of its own) against the candidate with its
# inert twin (mods/stage2b/off: features.livesplit false). Neither shows mod signs, so the twin is the
# reference for everything: compare.py "livesplit" (TRACE, hash.txt, the user dir but mods-status.json and
# the copied livesplit/ templates, screenshots, snapshots, the logs without their "livesplit:" and "mods:"
# lines) and the block's event log (-livesplitlog) against hash.txt's states.
LSMODS="$OUT/gen/livesplit-mods"
LSOFF="$GAME/tools/oracle/mods/stage2b/off"
rm -rf "$LSMODS"
mkdir -p "$LSMODS"
cp -r "$MODS/livesplit" "$LSMODS/livesplit"
# lscase <case> <description> <seed> <script or -> [options]: <case>.lsoff (the reference) and <case>.ls; a
# "from=<case>" option starts each from the user dir of the same twin of <case>
lscase() {
	local name=$1 desc=$2 seed=$3 script=$4
	shift 4
	local off=() on=()
	for opt in "$@"; do
		case "$opt" in
			from=*) off+=("from=${opt#from=}.lsoff"); on+=("from=${opt#from=}.ls") ;;
			*) off+=("$opt"); on+=("$opt") ;;
		esac
	done
	printf '%s\t%s\t%s\t\n' "$name.lsoff" lsref "$desc, with the inert features.livesplit=false mod (the reference)" >> "$OUT/cases.txt"
	run candidate "$name.lsoff" "$seed" "$script" modsdir="$LSOFF" "${off[@]}"
	printf '%s\t%s\t%s\t%s\n' "$name.ls" livesplit "$desc, with the livesplit mod" "ref=$name.lsoff" >> "$OUT/cases.txt"
	run candidate "$name.ls" "$seed" "$script" modsdir="$LSMODS" arg=-livesplitlog=livesplit.txt "${on[@]}"
}
lscase main "playtest.txt" 1 "$PT/playtest.txt"
lscase load "playtest_load.txt in main's user dir (a resumed board: no GO counted)" 1 "$PT/playtest_load.txt" from=main
lscase level "playtest_level.txt (a level clear)" 3 "$PT/playtest_level.txt"
for s in drag_swaps input_edges rclick falling_select options_highscores pause nomoves_normal timed_fallout mode_switch; do
	lscase "$s" "stage-0 $s.txt" 1 "$S0/$s.txt"
done
lscase timed_fallout_load "load_resume.txt in timed_fallout's user dir" 1 "$S0/load_resume.txt" from=timed_fallout
lscase mode_switch_load "load_resume.txt in mode_switch's user dir" 1 "$S0/load_resume.txt" from=mode_switch
lscase save_states "save_states.txt (snapshots in every state)" 3 "$S0/save_states.txt"
for snap in s5_vortex s6_wipe; do
	lscase "save_states.load.$snap" "load_resume.txt with the $snap save (LoadGame restores the vortex or wipe: no level clear)" \
		1 "$S0/load_resume.txt" savegame="$OUT/candidate/save_states/snaps/$snap.dat"
done
lscase autoplay_normal "-autoplay=1 (normal, no moves)" 1 - arg=-autoplay=1 arg=-autoplaylog=autoplay.txt
lscase autoplay_timed "-autoplay=2 -autoplaytimed (timed, level 10, time up)" 1 - arg=-autoplay=2 arg=-autoplaytimed \
	arg=-autoplaylog=autoplay.txt
# The script reads the block the game writes, and the block is in the exe once (check_asl_offsets.py, with
# lsprobe -dumplayout from build/native)
printf '%s\t%s\t%s\t\n' livesplit_layout lint "games/bejeweled/tools/livesplit/check_asl_offsets.py: the script's magic and field table against the game's layout 1; the magic once, in the candidate's .data" >> "$OUT/cases.txt"
mkdir -p "$OUT/candidate/livesplit_layout"
LSPROBE="$ROOT/build/native/lsprobe.exe"
if [ -x "$LSPROBE" ]; then
	"$LSPROBE" -dumplayout > "$OUT/candidate/livesplit_layout/layout.txt"
	"$PY" "$(winpath "$GAME/tools/livesplit/check_asl_offsets.py")" "$(winpath "$GAME/tools/livesplit/BejeweledDeluxe-byopop.asl")" \
		"$(winpath "$OUT/candidate/livesplit_layout/layout.txt")" "$(winpath "${EXE[candidate]}")" > "$OUT/candidate/livesplit_layout/check.txt" 2>&1
	echo $? > "$OUT/candidate/livesplit_layout/exit.txt"
else
	echo "$LSPROBE is missing (cmake --build build/native --target lsprobe)" > "$OUT/candidate/livesplit_layout/check.txt"
	echo 1 > "$OUT/candidate/livesplit_layout/exit.txt"
fi
echo "  candidate livesplit_layout: exit $(cat "$OUT/candidate/livesplit_layout/exit.txt")"

# Stage 2b with stage 3: a watch with the livesplit mod against the same watch with its inert twin. The playback
# board (Board::mPlaybackOnly) carries the replay flag (0x20) and is never a run: no GO is counted on it
# (compare.py "livesplit": a replay board expects none), while its level clears and game over are, as on any board.
lscase watch_main_timed "rec_main's timed game watched (-replay): a playback board, flag 0x20, no GO" 1 "$S3/watch.txt" 	replay="$(one "$C/rec_main/user/replays/*-timed-*.bjr")"

echo
"$PY" "$COMPARE_PY" compare "$(winpath "$OUT")" "$EXPECT"
