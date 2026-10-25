#!/usr/bin/env bash
# Regression oracle (games/bejeweled/docs/CODE_MODS_DESIGN.md 9.1): runs the frozen baseline exes and the
# candidate exe over every oracle case, each run in a fresh directory, then compares the outputs with
# compare.py. Paths below are from the repository root; relative arguments are from the current directory.
#
#   games/bejeweled/tools/oracle/compare.sh [--baseline <exe>] [--hooks-baseline <exe> | --no-hooks-baseline]
#       [--candidate <exe>] [--candidate-args "<switches>"] [--out <dir>] [--compare-only]
#       [--jobs <n>] [--no-cache] [--cache <dir>] [--variants <variant>,...]
#
# --candidate-args adds switches to every candidate run, e.g. "-forcevariant" (CODE_MODS_DESIGN.md 9.1: the
# generalised paths at default rules must match the baselines too). Every run reads the game files in
# assets/bejeweled/.
#
# --variants runs several candidates in one go, each into its own <out> and compared on its own (a report
# each, printed one after the other), sharing the baseline runs: "default" (build/native/bejeweled.exe into
# build/oracle), "forcevariant" (the same exe with -forcevariant, into build/oracle-forcevariant) and
# "novariants" (build/native-novariants/bejeweled.exe from tools/build.sh novariants, into
# build/oracle-novariants). "--variants default,forcevariant,novariants" runs all three oracles. Not with
# --candidate, --candidate-args or --out. The exit status is 0 only if every variant matches.
#
# Speed: --jobs <n> (1 to 3, default 3: CLAUDE.md allows 3 game processes at a time) runs up to n games at a
# time, the variants' in one pool (n + 2 jobs, so that preparing runs and taking them from the cache overlap
# with the games). A run starts once the runs it reads are done (a from= user dir, or a savegame, script,
# replay or mods folder in <out>); cases.txt, report.txt and the run directories do not depend on n, and
# --jobs 1 runs everything one at a time in the order below. The baseline sides' runs are
# cached in build/oracle-cache (--cache <dir>), because the frozen exes give the same outputs for the same
# inputs: a run is taken from the cache when its key matches, the sha256 of the exe and its DLLs (all of
# them, as exes.txt lists them) and of rundesk.exe, the run's command line, the prepared run directory
# (script, user dir with the day fields of hiscores*.dat masked, savegame, replay), the mods folder it
# reads, the staged game files (assets/bejeweled), the run logic below (prepare_run, exec_run), the
# environment variables SDL, the sanitizer runtime and the game read (SDL_*, ASAN_*, LSAN_*, UBSAN_*, BEJ_*,
# BYOPOP_*) and the machine (GPU, driver and display mode, DPI, Windows build, CPU: screenshots come from the
# GPU). The run directory then holds cached.txt (when and where the run was made). A baseline run that starts
# from the user dir of a cached run made on another day gets that user dir's hiscores days moved to today
# first (the game only keeps "today's" score on the day it was saved), and compare.py expects a cached run's
# day fields that many days behind. A run that did not exit with 0, that changed the real mods folder or that
# ran after it changed is not cached; entries unused for 30 days are dropped. --no-cache runs everything now
# and leaves the cache alone. The candidate is never cached. Only the draw count of the "Seconds =" log line
# depends on the machine's speed (compare.py, DRAWS): with several runs at a time, a slower run can draw an
# unchanged screen a few more times; the cache entry of a baseline run whose draw count differed is dropped
# after the comparison, so the next oracle makes that run again.
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
# numbers (compare.py; runs from the cache are allowed for).
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
# Stage 4 (the headless verifier, games/bejeweled/tools/verify/oracle_cases.sh, sourced at the end of define_cases):
# every watched replay played again by the native and the wasm verifier (the same REPLAY and TRACE lines as the
# game), copies no player can start, the verifier's tests and its difftest (the plain and the -forcevariant board in
# lockstep; these two start in the background as the cases are defined), the web build under Node on the playtests,
# the stage-0 scripts and bot runs, a seeded game, a recording and two watches (the candidate's LOGIC hashes and
# TRACE), and two games whose (int) casts leave the int range, recorded natively and by the web build, each
# recording watched and verified on both.
# Stage 5 (allow invalid moves, the noMoves rule, the v2 save) plays the rule mods of mods/stage5 (compare.py
# "variant"): kept swaps, "continue", the other noMoves branches, v2 saves resumed and refused both ways, their
# recordings and playbacks. A candidate built without the variant paths (BEJ_NO_VARIANTS: its -dumprules text
# says "variants = off") plays no rule: the stage-5 cases are left out, and stage 1's and 2's rule-mod cases
# expect what such a build writes (expect.txt's "<case>@novariants" facts: a v1 save and the RNG sidecar).
# Stage 6 (undo) plays the rule mods of mods/stage6 the same way (compare.py "variant" with "undos", "redo" and
# "rerolled"): every way to undo, the limit, restoreRng off, timed mode with and without inTimed, a resume (no
# snapshots in a save), a recording and its playback. A BEJ_NO_VARIANTS candidate leaves them out too.
# Stage 7 (board sizes and shapes) plays the rule mods of mods/stage7 the same way: the autoplay bot on boards
# smaller, wider and larger than 8x8 and on a diamond, a mod whose levels change shape at each vortex (saved on
# the second shape, resumed, recorded and played back). A BEJ_NO_VARIANTS candidate leaves them out too.
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
CAND_ARGS_TEXT=""
OUT_ARG="$ROOT/build/oracle"
COMPARE_ONLY=0
JOBS=3
USE_CACHE=1
CACHE="$ROOT/build/oracle-cache"
VARIANTS=""
SINGLE_ARGS=0  # --candidate, --candidate-args or --out given
usage() { awk 'NR > 1 { if (!/^#/) exit; sub(/^# ?/, ""); print }' "$0"; }
ARGS=("$@")  # for the re-run under rundesk below: the loop consumes them
while [ $# -gt 0 ]; do
	case "$1" in
		--baseline) BASE_EXE="$2"; shift 2 ;;
		--hooks-baseline) HOOKS_EXE="$2"; shift 2 ;;
		--no-hooks-baseline) HOOKS_EXE=""; shift ;;
		--candidate) CAND_EXE="$2"; SINGLE_ARGS=1; shift 2 ;;
		--candidate-args) CAND_ARGS_TEXT="$2"; SINGLE_ARGS=1; shift 2 ;;
		--out) OUT_ARG="$2"; SINGLE_ARGS=1; shift 2 ;;
		--compare-only) COMPARE_ONLY=1; shift ;;
		--jobs) JOBS="$2"; shift 2 ;;
		--no-cache) USE_CACHE=0; shift ;;
		--cache) CACHE="$2"; shift 2 ;;
		--variants) VARIANTS="$2"; shift 2 ;;
		-h|--help) usage; exit 0 ;;
		*) usage; exit 2 ;;
	esac
done
case "$JOBS" in
	1|2|3) ;;
	*) echo "compare.sh: --jobs takes 1 to 3 (CLAUDE.md: at most 3 game processes at a time)" >&2; exit 2 ;;
esac
if [ -n "$VARIANTS" ] && [ $SINGLE_ARGS -eq 1 ]; then
	echo "compare.sh: --variants names the candidates and output folders: no --candidate, --candidate-args or --out with it" >&2
	exit 2
fi
if [ "${BASH_VERSINFO[0]}" -lt 5 ] || { [ "${BASH_VERSINFO[0]}" -eq 5 ] && [ "${BASH_VERSINFO[1]}" -lt 1 ]; }; then
	JOBS=1  # wait -n -p needs bash 5.1
fi

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

# The variants: V_NAME, V_OUT (absolute), V_EXE (the candidate), V_ARGS (its switches)
NV=0
declare -a V_NAME V_OUT V_EXE V_ARGS V_HOOKS V_VARIANTS
add_variant() {
	mkdir -p "$2" || exit 2
	V_NAME[NV]=$1
	V_OUT[NV]="$(cd "$2" && pwd)"
	V_EXE[NV]=$3
	V_ARGS[NV]=$4
	NV=$((NV + 1))
}
if [ -n "$VARIANTS" ]; then
	IFS=, read -r -a VLIST <<< "$VARIANTS"
	for v in "${VLIST[@]}"; do
		case " ${V_NAME[*]:-} " in *" $v "*) echo "compare.sh: variant $v twice" >&2; exit 2 ;; esac
		case "$v" in
			default) add_variant default "$ROOT/build/oracle" "$ROOT/build/native/bejeweled.exe" "" ;;
			forcevariant) add_variant forcevariant "$ROOT/build/oracle-forcevariant" "$ROOT/build/native/bejeweled.exe" "-forcevariant" ;;
			novariants) add_variant novariants "$ROOT/build/oracle-novariants" "$ROOT/build/native-novariants/bejeweled.exe" "" ;;
			*) echo "compare.sh: unknown variant \"$v\" (default, forcevariant, novariants)" >&2; exit 2 ;;
		esac
	done
else
	add_variant candidate "$OUT_ARG" "$CAND_EXE" "$CAND_ARGS_TEXT"
fi

if [ $COMPARE_ONLY -eq 1 ]; then
	status=0
	for ((v = 0; v < NV; v++)); do
		"$PY" "$COMPARE_PY" compare "$(winpath "${V_OUT[v]}")" "$EXPECT" || status=$?
	done
	exit $status
fi

SIDES=(baseline)
declare -A EXE HOOKS EXEID
EXE[baseline]=$BASE_EXE
if [ -n "$HOOKS_EXE" ]; then
	SIDES+=(hooks-baseline)
	EXE[hooks-baseline]=$HOOKS_EXE
fi
SIDES+=(candidate)

# exe_info <exe>: X_EXE (absolute), X_HOOKS, X_SHA, X_DLLS (exes.txt: the DLLs next to it, 16 hex digits of
# each sha256) and X_ID (the cache key's: the full sha256s)
exe_info() {
	local exe=$1 dll sha
	[ -f "$exe" ] || return 1
	X_EXE="$(cd "$(dirname "$exe")" && pwd)/$(basename "$exe")"  # absolute: every case runs in its own directory
	# An exe has the stage-0 hooks (save command, -tracehash, -autoplay, rclick) if it knows -tracehash
	X_HOOKS=no
	grep -q -a "tracehash" "$X_EXE" && X_HOOKS=yes
	X_SHA=$(sha256sum "$X_EXE" | cut -d' ' -f1)
	X_DLLS=""
	X_ID="$X_SHA"
	for dll in "$(dirname "$X_EXE")"/*.dll; do
		[ -f "$dll" ] || continue
		sha=$(sha256sum "$dll" | cut -d' ' -f1)
		X_DLLS="$X_DLLS $(basename "$dll"):${sha:0:16}"
		X_ID="$X_ID $(basename "$dll"):$sha"
	done
	X_DLLS=${X_DLLS# }
}
declare -A SIDE_LINES
for side in "${SIDES[@]}"; do
	[ "$side" = candidate ] && continue
	exe_info "${EXE[$side]}" || { echo "missing $side exe: ${EXE[$side]}"; exit 2; }
	EXE[$side]=$X_EXE
	HOOKS[$side]=$X_HOOKS
	EXEID[$side]=$X_ID
	SIDE_LINES[$side]=$(printf '%s.exe=%s\n%s.hooks=%s\n%s.sha256=%s\n%s.dlls=%s' "$side" "$(winpath "$X_EXE")" "$side" "$X_HOOKS" \
		"$side" "$X_SHA" "$side" "$X_DLLS")
done
for ((v = 0; v < NV; v++)); do
	exe_info "${V_EXE[v]}" || { echo "missing candidate exe: ${V_EXE[v]}"; exit 2; }
	[ "$X_HOOKS" = yes ] || { echo "the candidate has no stage-0 hooks: ${V_EXE[v]}"; exit 2; }
	V_EXE[v]=$X_EXE
	V_HOOKS[v]=$X_HOOKS
	# A candidate built with BEJ_NO_VARIANTS plays no rule (its -dumprules text says so): the stage-5 cases are left
	# out (they test the variant paths, which it does not have)
	V_VARIANTS[v]=yes
	grep -q -a "variants = off (a BEJ_NO_VARIANTS build" "$X_EXE" && V_VARIANTS[v]=no
	SIDE_LINES[candidate.$v]=$(printf 'candidate.exe=%s\ncandidate.hooks=%s\ncandidate.sha256=%s\ncandidate.dlls=%s\ncandidate.variants=%s' \
		"$(winpath "$X_EXE")" "$X_HOOKS" "$X_SHA" "$X_DLLS" "${V_VARIANTS[v]}")
done
if [ -n "$HOOKS_EXE" ] && [ "${HOOKS[hooks-baseline]}" != yes ]; then
	echo "the hooks baseline has no stage-0 hooks: ${EXE[hooks-baseline]}"
	exit 2
fi

# set_variant <index>: the globals the case definitions and the runs of that variant use
set_variant() {
	VIDX=$1
	OUT=${V_OUT[VIDX]}
	C="$OUT/candidate"
	EXE[candidate]=${V_EXE[VIDX]}
	HOOKS[candidate]=${V_HOOKS[VIDX]}
	CAND_VARIANTS=${V_VARIANTS[VIDX]}
	read -r -a CAND_ARGS <<< "${V_ARGS[VIDX]}"
	VTAG=""
	[ $NV -eq 1 ] || VTAG="[${V_NAME[VIDX]}]"
}

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

# one <glob>: the one file the glob names (the run stops if it names none or more)
one() {
	local hits=( $1 )
	if [ ${#hits[@]} -ne 1 ] || [ ! -f "${hits[0]}" ]; then
		echo "compare.sh: $1 names ${#hits[@]} files" >&2
		return 2
	fi
	printf '%s' "${hits[0]}"
}

# run <side> <case> <seed> <script or -> [mods=examples] [modsdir=<dir>] [from=<case>] [savegame=<file>]
#     [saveas=<name>] [strip] [nohash] [dumprules] [fakereplays] [replay=<file or glob>] [arg=<switch>]...
#   modsdir=<dir>    use the mods in <dir> (all enabled, in their priority order; no mods.json)
#   dumprules        afterwards, the same exe with -dumprules into dumprules.txt
#   from=<case>      start from a copy of that case's user dir (same side) instead of a fresh one
#   savegame=<file>  copy <file> in as user/savegame.dat (nothing if it does not exist)
#   saveas=<name>    ... as user/<name> instead (a gameplay mod's scoped save, savegame-<key>.dat)
#   strip            comment out the script's "save" lines even for an exe that has the command
#   nohash           no -tracehash
#   fakereplays      ten old replays in user/replays first, one of them pinned (compare.py fake-replays)
#   replay=<file>    watch a copy of <file> (replay.bjr in the run directory): -replay=replay.bjr -replayquit;
#                    a glob names the one file it matches once the run that writes it is done
# The case definitions below only queue the runs (run, enqueue); schedule runs them (do_run), each once the
# runs it reads are done. A frozen reference exe (baseline, hooks-baseline) that crashes at exit (exit 139,
# or an access violation 0xC0000005 as 3221225477, -1073741819 or its low byte 5) is run once more from
# scratch: those builds predate the audio shutdown fix (af758b9) and can crash after writing all their
# outputs. The second run is the one compared; retried.txt keeps the first exit status for compare.py's
# report. The candidate gets no retry.

# prepare_run <side> <case> <seed> <script or -> [options]: a fresh run directory with the run's inputs, and
# R_DIR, R_SIDE, R_NAME, R_EXE, R_ARGS (the command line), R_MODSDIR, R_MODS, R_DUMPRULES, R_FAKE for exec_run
prepare_run() {
	local side=$1 name=$2 seed=$3 script=$4
	shift 4
	local hooks=${HOOKS[$side]} dir="$OUT/$side/$name" modsdir=mods from="" savegame="" saveas=savegame.dat strip=no replay="" opt
	local args=() hash=(-tracehash=hash.txt)
	R_DIR=$dir R_SIDE=$side R_NAME=$name R_EXE=${EXE[$side]} R_MODS=none R_DUMPRULES=no R_FAKE=no
	[ "$side" = candidate ] && args+=("${CAND_ARGS[@]}")
	for opt in "$@"; do
		case "$opt" in
			mods=examples) R_MODS=examples; modsdir="$MODS_W" ;;
			modsdir=*) modsdir="$(winpath "${opt#modsdir=}")" ;;
			dumprules) R_DUMPRULES=yes ;;
			from=*) from="${opt#from=}" ;;
			savegame=*) savegame="${opt#savegame=}" ;;
			saveas=*) saveas="${opt#saveas=}" ;;
			strip) strip=yes ;;
			nohash) hash=() ;;
			fakereplays) R_FAKE=yes ;;
			replay=*) replay="${opt#replay=}" ;;
			arg=*) args+=("${opt#arg=}") ;;
		esac
	done
	R_MODSDIR=$modsdir

	rm -rf "$dir"
	mkdir -p "$dir/shots" "$dir/snaps" "$dir/mods"
	if [ -n "$from" ]; then
		cp -r "$OUT/$side/$from/user" "$dir/user"
		# the user dir of a run from the cache, made on another day: the hiscores days a run of today would
		# have left (HighScoreWidget::LoadHighScores keeps "today's" score only on the day it was saved)
		if [ -f "$OUT/$side/$from/cached.txt" ]; then
			local days
			days=$(awk -F= '$1 == "made-day" { m = $2 } $1 == "restored-day" { r = $2 } END { print r - m }' \
				"$OUT/$side/$from/cached.txt")
			[ "$days" = 0 ] || "$PY" "$COMPARE_PY" shift-hiscores-days "$(winpath "$dir/user")" "$days"
		fi
	else
		# as build/native/playtest/run.sh: windowed
		mkdir -p "$dir/user"
		printf '; test settings\n\n[SOFTWARE\Steam\Bejeweled]\nScreenMode=dword:0\n' > "$dir/user/registry.ini"
	fi
	if [ "$R_MODS" = examples ]; then
		printf '%s\n' "$MODS_JSON" > "$dir/user/mods.json"
	fi
	if [ -n "$savegame" ] && [ -f "$savegame" ]; then
		cp "$savegame" "$dir/user/$saveas"
	fi
	if [ "$R_FAKE" = yes ]; then
		"$PY" "$COMPARE_PY" fake-replays "$(winpath "$dir/user/replays")"
	fi
	if [ -n "$replay" ]; then
		case "$replay" in *[*?]*) replay=$(one "$replay") || return 2 ;; esac
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
	R_ARGS=(-gamedir="$ASSETS_W" -userdir=user -modsdir="$modsdir" -seed="$seed" -fast -trace "${hash[@]}" "${args[@]}")
}

# exec_run: the prepared run. Separate files: stderr is block-buffered when redirected, so in one file its
# chunks would split stdout lines. exit 124: killed after ORACLE_TIMEOUT seconds (default 600; a -fast run
# takes seconds)
exec_run() {
	local listing="$OUT/gen/mods-listing.$R_SIDE.$R_NAME"
	[ "$R_MODS" != examples ] || mods_listing > "$listing.before.txt"
	(cd "$R_DIR" && timeout "${ORACLE_TIMEOUT:-600}" "$R_EXE" "${R_ARGS[@]}" > stdout.txt 2> stderr.txt; echo $? > exit.txt)
	if [ "$R_DUMPRULES" = yes ]; then
		(cd "$R_DIR" && timeout 60 "$R_EXE" -gamedir="$ASSETS_W" -userdir=user -modsdir="$R_MODSDIR" -dumprules \
			> dumprules.txt 2> dumprules-stderr.txt)
	fi
	if [ "$R_MODS" = examples ]; then
		mods_listing > "$listing.after.txt"
		cmp -s "$listing.before.txt" "$listing.after.txt" ||
			diff "$listing.before.txt" "$listing.after.txt" > "$R_DIR/modsdir-changed.txt"
	fi
	echo "$VTAG  $R_SIDE $R_NAME: exit $(tr -d '\r\n' < "$R_DIR/exit.txt")"
}

# The cache (see the top): cache_key prints the key of the prepared run, cache_restore <key> replaces the
# prepared run directory with the cached run, cache_store <key> keeps the finished one
daynum() { local d; d=$(date '+%Y %j'); set -- $d; echo $((($1 - 1900) * 366 + 10#$2 - 1)); }  # GetDayNumber
cache_key() {
	local modsdir
	case "$R_MODSDIR" in
		mods) modsdir="the run's own (empty) mods/" ;;
		"$MODS_W") modsdir="games/bejeweled/mods $MODS_HASH" ;;
		*) modsdir=$("$PY" "$COMPARE_PY" tree-hash "$R_MODSDIR" | tr -d '\r') || return 1 ;;
	esac
	# the command line as it is: the game logs the game and mods folders it was given
	{
		printf '%s\n' "$CACHE_STATIC" "exe $R_EXE ${EXEID[$R_SIDE]}" "modsdir $modsdir" "examples-check $R_MODS" \
			"dumprules $R_DUMPRULES" "fakereplays $R_FAKE"
		printf 'arg %s\n' "${R_ARGS[@]}"
	} | "$PY" "$COMPARE_PY" cache-key "$(winpath "$R_DIR")" | tr -d '\r'
}
cache_restore() {
	local entry="$CACHE/$1"
	[ -f "$entry/meta.txt" ] && [ -d "$entry/run" ] || return 1
	rm -rf "$R_DIR.cached"
	if ! cp -rp "$entry/run" "$R_DIR.cached"; then
		rm -rf "$R_DIR.cached"
		return 1
	fi
	rm -rf "$R_DIR"
	mv "$R_DIR.cached" "$R_DIR" || return 1
	{ cat "$entry/meta.txt"; printf 'restored=%s\nrestored-day=%s\n' "$(date '+%F %T')" "$(daynum)"; } > "$R_DIR/cached.txt"
	touch "$entry/used"
}
cache_store() {
	local tmp="$CACHE/.tmp-$1-$BASHPID"
	rm -rf "$tmp"
	if mkdir -p "$tmp" && cp -rp "$R_DIR" "$tmp/run" &&
		printf 'key=%s\nside=%s\ncase=%s\nmade=%s\nmade-day=%s\nmade-in=%s\n' "$1" "$R_SIDE" "$R_NAME" "$(date '+%F %T')" \
			"$(daynum)" "$(winpath "$R_DIR")" > "$tmp/meta.txt" && touch "$tmp/used" &&
		mv -T "$tmp" "$CACHE/$1" 2> /dev/null; then
		return 0
	fi
	rm -rf "$tmp"  # an entry of the same key appeared meanwhile, or the copy failed
}

# Game slots: the scheduler keeps a few more jobs going than --jobs, so that preparing a run, restoring one from
# the cache and storing one overlap with the games, but a game only starts with one of the --jobs slots
# (directories <SLOTS>/<n>: mkdir takes one or fails). Not needed when the jobs run one at a time.
SLOT=""
game_slot_take() {
	local k
	[ $JOBS -gt 1 ] || return 0
	while :; do
		for ((k = 0; k < JOBS; k++)); do
			if mkdir "$SLOTS/$k" 2> /dev/null; then
				SLOT=$k
				return 0
			fi
		done
		sleep 0.1
	done
}
game_slot_give() {
	[ -z "$SLOT" ] || rmdir "$SLOTS/$SLOT"
	SLOT=""
}

# do_run <run arguments>: one run (run's arguments), from the cache when it can
do_run() {
	local side=$1 name=$2 key="" code day=""
	prepare_run "$@" || return 2
	if [ $USE_CACHE -eq 1 ] && [ "$side" != candidate ]; then
		key=$(cache_key) || key=""
		if [ -n "$key" ] && cache_restore "$key"; then
			printf '%s %s\n' "$side/$name" "$key" >> "$OUT/gen/cache-keys.txt"  # for the DRAWS drop below
			echo "$VTAG  $side $name: exit $(tr -d '\r\n' < "$R_DIR/exit.txt") (cached)"
			return 0
		fi
		# The key masks the hiscores day fields, so a run is kept only if it starts and ends on one day, from a
		# user dir of that day (not from a parent run made before midnight)
		day=$(daynum)
		if compgen -G "$R_DIR/user/hiscores*.dat" > /dev/null &&
			! "$PY" "$COMPARE_PY" hiscores-day "$(winpath "$R_DIR/user")" "$day"; then
			key=""
		fi
	fi
	game_slot_take
	exec_run
	game_slot_give
	if [ "$side" != candidate ]; then
		code=$(tr -d '\r\n' < "$R_DIR/exit.txt")
		case "$code" in
			139|5|3221225477|-1073741819)
				echo "$VTAG  $side $name: exit $code, a crash at exit (frozen exe without af758b9): run once more"
				prepare_run "$@" || return 2
				game_slot_take
				exec_run
				game_slot_give
				printf '%s\n' "$code" > "$R_DIR/retried.txt"
				;;
		esac
	fi
	# Not kept either: a run that changed the real mods folder (its modsdir-changed.txt would come back from the
	# cache in every later oracle), or one made after that folder changed under this oracle (its key holds the
	# folder's hash from the start)
	if [ -n "$key" ] && [ "$(tr -d '\r\n' < "$R_DIR/exit.txt")" = 0 ] && [ ! -e "$R_DIR/modsdir-changed.txt" ] &&
		[ "$(daynum)" = "$day" ] &&
		{ [ "$R_MODS" != examples ] || [ "$("$PY" "$COMPARE_PY" tree-hash "$MODS_W" | tr -d '\r')" = "$MODS_HASH" ]; }; then
		cache_store "$key"
		printf '%s %s\n' "$side/$name" "$key" >> "$OUT/gen/cache-keys.txt"
	fi
	return 0
}

# The job queue. enqueue <id> <command...> queues a job of the current variant with the dependencies in DEPS
# (job numbers); run (the case definitions' run) and the other jobs below queue through it. Ids: <side>/<case>,
# gen/<file in gen/>.
NJOBS=0
DEPS=""
declare -a J_V J_ID J_CMD J_DEPS J_STATE  # J_STATE: 0 queued, 1 running, 2 done
declare -A J_KEY                         # "<variant>|<id>" -> job number
enqueue() {
	local id=$1 cmd
	shift
	printf -v cmd '%q ' "$@"
	J_V[NJOBS]=$VIDX
	J_ID[NJOBS]=$id
	J_CMD[NJOBS]=$cmd
	J_DEPS[NJOBS]=$DEPS
	J_STATE[NJOBS]=0
	J_KEY["$VIDX|$id"]=$NJOBS
	NJOBS=$((NJOBS + 1))
	DEPS=""
}
dep_id() { local j=${J_KEY["$VIDX|$1"]:-}; [ -z "$j" ] || DEPS="$DEPS $j"; }
# path_dep <path>: a path in <out> depends on the job that writes it (<out>/<side>/<case>/..., <out>/gen/<file>)
path_dep() {
	local rel=${1#"$OUT/"}
	[ "$rel" != "$1" ] || return 0
	local rest=${rel#*/}
	dep_id "${rel%%/*}/${rest%%/*}"
}
run() {
	local side=$1 name=$2 opt j
	DEPS=""
	path_dep "$4"
	for opt in "${@:5}"; do
		case "$opt" in
			from=*) dep_id "$side/${opt#from=}" ;;
			savegame=*|replay=*|modsdir=*) path_dep "${opt#*=}" ;;
		esac
	done
	# a later variant's baseline run waits for the first variant's: it then comes from the cache
	if [ "$side" != candidate ] && [ $USE_CACHE -eq 1 ] && [ "$VIDX" -gt 0 ]; then
		j=${J_KEY["0|$side/$name"]:-}
		[ -z "$j" ] || DEPS="$DEPS $j"
	fi
	enqueue "$side/$name" do_run "$@"
}

# Jobs that are no game run
cli_job() {  # -gameseed values main.cpp must refuse
	mkdir -p "$C/gameseed_cli"
	: > "$C/gameseed_cli/cli.txt"
	local bad
	game_slot_take
	for bad in -gameseed=0 -gameseed=4294967296 -gameseed=99999999999999999999 -gameseed=-5 -gameseed=+5 -gameseed=12a \
			-gameseed=0x10 -gameseed= -gameseed; do
		(cd "$C/gameseed_cli" && timeout 60 "${EXE[candidate]}" -gamedir="$ASSETS_W" -userdir=user -modsdir=mods \
			"${CAND_ARGS[@]}" "$bad" > stdout.txt 2> stderr.txt
		 printf '%s\t%s\t%s\n' "$bad" "$?" "$(head -n 1 stderr.txt | tr -d '\r')" >> cli.txt)
	done
	game_slot_give
	rm -f "$C/gameseed_cli/stdout.txt" "$C/gameseed_cli/stderr.txt"
	echo "$VTAG  candidate gameseed_cli: $(wc -l < "$C/gameseed_cli/cli.txt") runs"
}
lint_job() {  # rule 1 of games/bejeweled/docs/CODE_MODS_DESIGN.md 6.1 on the sources
	mkdir -p "$C/rng_rules"
	"$PY" "$(winpath "$GAME/tools/check_rng_use.py")" > "$C/rng_rules/check.txt" 2>&1
	echo $? > "$C/rng_rules/exit.txt"
	echo "$VTAG  candidate rng_rules: exit $(cat "$C/rng_rules/exit.txt")"
}
layout_job() {  # the LiveSplit script's field table against the game's block (check_asl_offsets.py)
	mkdir -p "$C/livesplit_layout"
	local probe="$ROOT/build/native/lsprobe.exe"
	if [ -x "$probe" ]; then
		"$probe" -dumplayout > "$C/livesplit_layout/layout.txt"
		"$PY" "$(winpath "$GAME/tools/livesplit/check_asl_offsets.py")" "$(winpath "$GAME/tools/livesplit/BejeweledDeluxe-byopop.asl")" \
			"$(winpath "$C/livesplit_layout/layout.txt")" "$(winpath "${EXE[candidate]}")" > "$C/livesplit_layout/check.txt" 2>&1
		echo $? > "$C/livesplit_layout/exit.txt"
	else
		echo "$probe is missing (cmake --build build/native --target lsprobe)" > "$C/livesplit_layout/check.txt"
		echo 1 > "$C/livesplit_layout/exit.txt"
	fi
	echo "$VTAG  candidate livesplit_layout: exit $(cat "$C/livesplit_layout/exit.txt")"
}
corrupt_job() {  # corrupt_job <kind>: a copy of rec_pause's sidecar broken on purpose (compare.py corrupt-replay)
	"$PY" "$COMPARE_PY" corrupt-replay "$(winpath "$C/rec_pause/user/savegame.bjr")" "$(winpath "$OUT/gen/corrupt_$1.bjr")" "$1" \
		> "$OUT/gen/corrupt_$1.txt"
}
engine_job() {  # engine_job <n>: rec_main's sidecar as engine n would have written it (compare.py corrupt-replay engine<n>)
	"$PY" "$COMPARE_PY" corrupt-replay "$(winpath "$C/rec_main/user/savegame.bjr")" "$(winpath "$OUT/gen/engine$1_sidecar.bjr")" \
		"engine$1" > "$OUT/gen/engine$1_sidecar.txt"
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

# define_cases: the cases of the current variant (set_variant), queued in this order
define_cases() {
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
	# since stage 2 any rule gives the board its own RNG, so they play exactly like the per-game-seed reference.
	# Since stage 5 their boards are variant boards (a variant line in each settle, the v2 save, no RNG sidecar); a
	# BEJ_NO_VARIANTS candidate plays them as before (v1 and the sidecar): its own facts, expect.txt's <case>@novariants.
	# A fixture leaves the set when its stage makes it play: s1-diamond went to mods/stage7/dirt-diamond in stage 7
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
	enqueue candidate/gameseed_cli cli_job
	printf '%s\t%s\t%s\t\n' rng_rules lint "games/bejeweled/tools/check_rng_use.py: no other randomness in the game code, Board draws only through Board::Rand" >> "$OUT/cases.txt"
	enqueue candidate/rng_rules lint_job
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
	# playback <case> <description> <flags> <file or glob> [script=<file>] [options]: watch.txt, or the script given
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
		"$C/rec_main/user/replays/*-timed-*.bjr"
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
		"$C/rec_nomoves_normal/user/replays/*-normal-*.bjr"
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
		"$C/rec_autoplay_normal/user/replays/*.bjr"
	playback watch_autoplay_timed "rec_autoplay_timed's replay: the bot's timed game to time up on level 10" "trace=rec_autoplay_timed" \
		"$C/rec_autoplay_timed/user/replays/*.bjr"
	playback watch_decline "rec_decline's replay of the declined resume: rec_main's normal game, END saved" "trace=rec_main" \
		"$C/rec_decline/user/replays/*-normal-*.bjr"
	playback watch_seed_quit_resume "rec_seed_quit_resume's sidecar: a fresh replay with the loaded save (LDSV) and RNGS seed $GS" \
		"trace=rec_seed_quit_resume" "$C/rec_seed_quit_resume/user/savegame.bjr"
	playback watch_pergame_timed "rec_pergame's timed game: a per-game seed, time up" "trace=rec_pergame" \
		"$C/rec_pergame/user/replays/*-timed-*.bjr"
	playback watch_pergame_sidecar "rec_pergame's sidecar: a per-game seed, the RNG the save was made with" "trace=rec_pergame" \
		"$C/rec_pergame/user/savegame.bjr"
	playback watch_seed_normal "rec_seed_normal's replay: seed $GS (user), to no moves" "trace=rec_seed_normal" \
		"$C/rec_seed_normal/user/replays/*.bjr"
	playback watch_seed_abandoned "rec_seed_nomatch_timed's replay: seed $GS (user), a swap without a match, then Timed (END abandoned)" \
		"trace=rec_seed_nomatch_timed" "$C/rec_seed_nomatch_timed/user/replays/*-normal-*.bjr"
	playback watch_seed_timed "rec_seed_nomatch_timed's sidecar: the timed game on seed $GS, its match, the save at Normal" \
		"trace=rec_seed_nomatch_timed" "$C/rec_seed_nomatch_timed/user/savegame.bjr"
	playback watch_seed_highscores "rec_seed_highscores' sidecar: seed $GS, an outside draw that is no XRAND" "trace=rec_seed_highscores" \
		"$C/rec_seed_highscores/user/savegame.bjr"
	playback watch_seed_quit "rec_seed_quit's sidecar: seed $GS to the quit, its RNGS the board's RNG at the save" "trace=rec_seed_quit" \
		"$C/rec_seed_quit/user/savegame.bjr"
	playback watch_seed_quit_bot "rec_seed_quit.bot's replay: seed $GS quit, resumed with the sidecar's RNG, played to game over" \
		"trace=rec_seed_quit,rec_seed_quit.bot" "$C/rec_seed_quit.bot/user/replays/*.bjr"
	playback watch_keyed_hint "rec_keyed_hint's sidecar: the hint costs the RULE's 25, not the session's 50" "trace=rec_keyed_hint" \
		"$C/rec_keyed_hint/user/savegame*.bjr"
	# Replays broken on purpose (compare.py corrupt-replay, from rec_pause's sidecar): the player stops at the fault
	for kind in check select unpause end; do
		dep_id candidate/rec_pause
		enqueue "gen/corrupt_$kind.bjr" corrupt_job "$kind"
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
		"trace=rec_mode_switch_match" "$C/rec_mode_switch_match/user/replays/*-timed-*.bjr"
	playback watch_mode_switch_match "rec_mode_switch_match's sidecar: the normal game to the quit" "trace=rec_mode_switch_match" \
		"$C/rec_mode_switch_match/user/savegame.bjr"
	playback watch_seed_quit_highscores "rec_seed_quit's sidecar (seed $GS, user) with Options -> High Scores mid-watch: the player's hiscores.dat, no -seeded table" \
		"trace=rec_seed_quit;user=hiscores.dat,registry.ini" "$C/rec_seed_quit/user/savegame.bjr" script="$S3/watch_highscores.txt"

	# Stage 5 raised the engine ABI to 2, stage 7 to 3 (a replay of the original rules from engine 1 still plays). A game of the
	# original rules that engine 1 recorded and saved is resumed as before: ReplayRecorder::OnLoad continues its sidecar,
	# as Replay::CheckPlayable plays it. A sidecar the resume cannot continue (here one a later engine wrote) is kept as
	# a finished replay first, END saved (Board::DeleteSaveSidecar), so its moves are not lost; the new replay holds the
	# loaded save (LDSV). rec_main's sidecar with its engine ABI changed (compare.py corrupt-replay engine<n>) stands in.
	for n in 1 4; do
		dep_id candidate/rec_main
		enqueue "gen/engine${n}_sidecar.bjr" engine_job "$n"
	done
	recording rec_load_engine1 "playtest_load.txt in a copy of rec_main's user dir whose sidecar engine 1 wrote: the original rules' replay goes on, two segments as in rec_load" \
		load 1 "$PT/playtest_load.txt" from=rec_main savegame="$OUT/gen/engine1_sidecar.bjr" saveas=savegame.bjr
	recording rec_load_engine4 "the same with a sidecar of engine 4, which this build cannot continue: a fresh replay of the loaded save (LDSV); the sidecar's game is kept in replays/ (END saved)" \
		load 1 "$PT/playtest_load.txt" from=rec_main savegame="$OUT/gen/engine4_sidecar.bjr" saveas=savegame.bjr
	playback watch_main_engine1 "rec_main's sidecar as engine 1 wrote it: an original-rules replay of engine 1 plays" "trace=rec_main" \
		"$OUT/gen/engine1_sidecar.bjr"
	playback watch_load_engine1 "rec_load_engine1's sidecar: engine 1's segment and this engine's in one replay" "trace=rec_main,rec_load_engine1" \
		"$C/rec_load_engine1/user/savegame.bjr"

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
	enqueue candidate/livesplit_layout layout_job
	# Stage 2b with stage 3: a watch with the livesplit mod against the same watch with its inert twin. The playback
	# board (Board::mPlaybackOnly) carries the replay flag (0x20) and is never a run: no GO is counted on it
	# (compare.py "livesplit": a replay board expects none), while its level clears and game over are, as on any board.
	lscase watch_main_timed "rec_main's timed game watched (-replay): a playback board, flag 0x20, no GO" 1 "$S3/watch.txt" 	replay="$C/rec_main/user/replays/*-timed-*.bjr"

	# Stage 5: allow invalid moves, the noMoves rule and the v2 save (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.1,
	# 4.10, 6.6, 7.2). The rule mods of mods/stage5 are played: s5-free (moves.allowInvalid, a penalty of 10, noMoves
	# "continue" in both modes), its twin under edited rules (same id and version: the same save file name, another
	# RulesId), s5-free0 (s5-free without the penalty), s5-strict (kept swaps that are no moves, noMoves "fallOut" in
	# normal and "gameOver" in timed mode) and s5-gold (gold.enabled alone). compare.py "variant" checks every settle
	# step of these runs (one move each, a kept move one swap of neighbours), the "continue" boards, the v2 saves, the
	# refusals, TimesPlayed and the warnings; the recordings must be inert and play back. A BEJ_NO_VARIANTS candidate
	# plays no rule: it leaves this block out.
	if [ "$CAND_VARIANTS" = yes ]; then
		S5="$PT/stage5"
		M5="$GAME/tools/oracle/mods/stage5"
		FREE_SAVE=savegame-899ba83c   # s5-free@1.0's score key (FNV-1a of "s5-free@1.0;", CODE_MODS_DESIGN.md 3.7)
		# variant <case> <description> <flags> <seed> <script or -> [options]: a candidate run with -codeccheck
		variant() {
			local name=$1 desc=$2 flags=$3
			shift 3
			printf '%s\t%s\t%s\t%s\n' "$name" variant "$desc" "$flags" >> "$OUT/cases.txt"
			run candidate "$name" "$@" arg=-codeccheck=codec.txt
		}
		FREEBOT=(arg=-autoplaycap=20000 arg=-autoplaylog=autoplay.txt)
		variant free_quit "-gameseed=$GS, mods/stage5/free: a match, a kept swap (10 off), the quit: a v2 save with MTRand($GS) and the counts" \
			"kept=1;v2=last;boards=user;gameseed=$GS" 1 "$S5/free_quit.txt" modsdir="$M5/free" arg=-gameseed=$GS
		variant free_through "free_quit's moves and free_quit.resume's in one game: two kept swaps" \
			"kept=2;v2=last;boards=user;gameseed=$GS" 1 "$S5/free_through.txt" modsdir="$M5/free" arg=-gameseed=$GS
		cross free_quit.resume samestream "free_quit's user dir, no -gameseed: Yes loads the v2 save (MTRand($GS) and the counts as saved); a match, a kept swap, a match" \
			"first=free_quit;ref=free_through" 1 "$S5/free_quit_resume.txt" from=free_quit modsdir="$M5/free" arg=-codeccheck=codec.txt
		variant free_quit.edited "free_quit's user dir with s5-free under edited rules (another RulesId, the same file name): the v2 save is refused" \
			"log=not loaded: a save of other rules;nosave" 1 "$S0/load_resume.txt" from=free_quit modsdir="$M5/edited"
		variant v1_refused "main's v1 save as s5-free's save file: a variant board takes only the v2 save" \
			"log=not loaded: a v1 save;nosave" 1 "$S0/load_resume.txt" modsdir="$M5/free" savegame="$C/main/user/savegame.dat" \
			saveas="$FREE_SAVE.dat"
		all v2_refused load "free_quit's v2 save as savegame.dat without mods: the original version check refuses it, as in every build" \
			1 "$S0/load_resume.txt" savegame="$C/free_quit/user/$FREE_SAVE.dat"
		variant kept_timed "-gameseed=$GS, mods/stage5/free: a kept swap first is a move: the seed is used up, Timed asks (Yes saves v2) and is on a per-game seed" \
			"kept=2;v2=last;boards=user,pergame;gameseed=$GS;rngseed=1" 1 "$S5/kept_timed.txt" modsdir="$M5/free" arg=-gameseed=$GS
		variant strict_timed "-gameseed=$GS, mods/stage5/strict: a kept swap is no move: no question, the timed board on the seed again, nothing saved" \
			"kept=2;countsasmove=0;nosave;boards=user,user;gameseed=$GS" 1 "$S5/strict_timed.txt" modsdir="$M5/strict" arg=-gameseed=$GS
		# Kept swaps that take the board's last move (MTRand($GS)'s first board, in both modes): the fall-out in normal
		# mode, after which the game is still new and the seed still pending; NO MOVES and the game over in timed mode,
		# which uses the seed up and counts a game played. The first of them breaks the tutorial's move: its arrows go
		variant strict_last "-gameseed=$GS, mods/stage5/strict: kept swaps take the last move: a fall-out (normal), no question at Timed, the same in timed mode: game over (TimesPlayed 1), play again unseeded" \
			"kept=6;countsasmove=0;nosave;timesplayed=1;boards=user,user,pergame;gameseed=$GS;rngseed=1" 1 "$S5/strict_last.txt" \
			modsdir="$M5/strict" arg=-gameseed=$GS
		# Hint on a "continue" board without a move: no penalty, no sparkle, no idle glow (s5-free0 keeps the score)
		variant free_hint "-gameseed=$GS, mods/stage5/free0: a match (10), kept swaps to a board without a move (\"continue\"): Hint there costs nothing, no idle glow" \
			"kept=6;continue=1;v2=last;boards=user;gameseed=$GS" 1 "$S5/free_hint.txt" modsdir="$M5/free0" arg=-gameseed=$GS
		# gold.enabled alone: the gold rule is not played yet, but its derived noMoves.normal "fallOut" is, and
		# mods-status.json says so
		variant gold_status "mods/stage5/gold (gold.enabled alone): a variant board; mods-status.json says its noMoves.normal \"fallOut\" is played" \
			"nosave;warn=the gold rule is not played yet, but with it noMoves.normal defaults to" 1 "$PT/stage2/seed_start.txt" modsdir="$M5/gold"
		variant free_auto "-autoplay=1, mods/stage5/free, 20000 updates: the bot's random swaps are kept, boards without a move go on (\"continue\")" \
			"kept=20;continue=2;v2" 1 - modsdir="$M5/free" arg=-autoplay=1 "${FREEBOT[@]}"
		variant free_auto_timed "-autoplay=2 -autoplaytimed, mods/stage5/free, 20000 updates: \"continue\" in timed mode" \
			"kept=5;continue=2;v2" 1 - modsdir="$M5/free" arg=-autoplay=2 arg=-autoplaytimed "${FREEBOT[@]}"
		variant strict_auto "-autoplay=1, mods/stage5/strict, 20000 updates: kept swaps that are no moves, a fall-out in normal mode (\"fallOut\")" \
			"kept=20;countsasmove=0;v2" 1 - modsdir="$M5/strict" arg=-autoplay=1 "${FREEBOT[@]}"
		variant strict_auto_timed "-autoplay=2 -autoplaytimed, mods/stage5/strict: kept swaps that are no moves; game over at no moves in timed mode (\"gameOver\", after a match)" \
			"kept=5;countsasmove=0" 1 - modsdir="$M5/strict" arg=-autoplay=2 arg=-autoplaytimed arg=-autoplaylog=autoplay.txt
		# Recorded (mods/stage5/*-replays: the rule mod and stage 3's replays feature mod): inert, and played back
		recording rec_free_quit "free_quit recorded: a kept swap is a move like any (SELECT, TARGET); the sidecar next to the v2 save" \
			free_quit 1 "$S5/free_quit.txt" modsdir="$M5/free-replays" arg=-gameseed=$GS
		recording rec_free_quit.resume "free_quit.resume in a copy of rec_free_quit's user dir: the sidecar's replay goes on (two segments, a v2 LOAD)" \
			free_quit.resume 1 "$S5/free_quit_resume.txt" from=rec_free_quit modsdir="$M5/free-replays"
		recording rec_free_ldsv "free_quit.resume in a copy of free_quit's user dir (no sidecar): a fresh replay that holds the v2 save (LDSV)" \
			free_quit.resume 1 "$S5/free_quit_resume.txt" from=free_quit modsdir="$M5/free-replays"
		recording rec_strict_last "strict_last recorded: the normal game kept at the mode switch (END abandoned), the timed one ended by no moves" \
			strict_last 1 "$S5/strict_last.txt" modsdir="$M5/strict-replays" arg=-gameseed=$GS
		recording rec_free_hint "free_hint recorded: the hint ShowHint does not give is no HINT" free_hint 1 "$S5/free_hint.txt" \
			modsdir="$M5/free0-replays" arg=-gameseed=$GS
		recording rec_free_auto "free_auto recorded: kept swaps and \"continue\" boards, saved at the cap" free_auto 1 - \
			modsdir="$M5/free-replays" arg=-autoplay=1 "${FREEBOT[@]}"
		recording rec_strict_auto_timed "strict_auto_timed recorded: a finished replay, END no moves" strict_auto_timed 1 - \
			modsdir="$M5/strict-replays" arg=-autoplay=2 arg=-autoplaytimed arg=-autoplaylog=autoplay.txt
		playback watch_free_quit "rec_free_quit's sidecar: a kept swap, END saved" "trace=rec_free_quit" "$C/rec_free_quit/user/$FREE_SAVE.bjr"
		playback watch_free_resume "rec_free_quit.resume's sidecar: the v2 save LOADed by the second segment's board" \
			"trace=rec_free_quit,rec_free_quit.resume" "$C/rec_free_quit.resume/user/$FREE_SAVE.bjr"
		playback watch_free_ldsv "rec_free_ldsv's sidecar: a fresh replay whose LDSV is a v2 save" "trace=rec_free_ldsv" \
			"$C/rec_free_ldsv/user/$FREE_SAVE.bjr"
		playback watch_strict_last_normal "rec_strict_last's normal game: kept swaps, the tutorial gone, the fall-out at no moves, END abandoned" \
			"trace=rec_strict_last" "$C/rec_strict_last/user/replays/*-normal-*.bjr"
		playback watch_strict_last_timed "rec_strict_last's timed game: game over at no moves after a kept swap" "trace=rec_strict_last" \
			"$C/rec_strict_last/user/replays/*-timed-*.bjr"
		playback watch_free_hint "rec_free_hint's sidecar: kept swaps to a board without a move, no HINT, END saved" "trace=rec_free_hint" \
			"$C/rec_free_hint/user/savegame-*.bjr"
		playback watch_free_auto "rec_free_auto's sidecar: 20000 updates of kept swaps and \"continue\" boards" "trace=rec_free_auto" \
			"$C/rec_free_auto/user/$FREE_SAVE.bjr"
		playback watch_strict_auto_timed "rec_strict_auto_timed's replay: kept swaps, game over at no moves (after a match)" \
			"trace=rec_strict_auto_timed" "$C/rec_strict_auto_timed/user/replays/*.bjr"

		# Stage 6: undo (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.12, 6.7). The rule mods of mods/stage6: s6-undo (depth
		# 3, max 4, penalty 5, not in timed mode), s6-reroll (restoreRng false) and s6-timed (inTimed). compare.py "variant"
		# checks that each undo brings back the board the move was made from, counts the undos and whether the same move
		# again refills the same way; -codeccheck round-trips the UNDO snapshot in every update of play.
		S6="$PT/stage6"
		M6="$GAME/tools/oracle/mods/stage6"
		variant undo_main "-gameseed=$GS, mods/stage6/undo: three moves; undos by z, u, Backspace and the red orb, the same move again after them; the fifth undo refused (max 4); the quit: a v2 save with 4 undos" \
			"undos=4;redo=2;v2=last" 1 "$S6/undo_main.txt" modsdir="$M6/undo" arg=-gameseed=$GS
		variant undo_reroll "-gameseed=$GS, mods/stage6/reroll: a move, z, the same move again: another refill (restoreRng false)" \
			"undos=1;rerolled=1;v2=last" 1 "$S6/undo_reroll.txt" modsdir="$M6/reroll" arg=-gameseed=$GS
		variant undo_reroll.resume "undo_reroll's user dir: Yes resumes its v2 save (1 undo); z is refused, no snapshot survives a save" \
			"v2=last" 1 "$S6/undo_reroll_resume.txt" from=undo_reroll modsdir="$M6/reroll"
		variant undo_timed "-gameseed=$GS, mods/stage6/timed: the timed board, a move, z (inTimed), the same move again: the same refill" \
			"undos=1;redo=1" 1 "$S6/undo_timed.txt" modsdir="$M6/timed" arg=-gameseed=$GS
		variant undo_timed_off "-gameseed=$GS, mods/stage6/undo in timed mode (inTimed false): z pauses as any key, no undo" \
			"v2=last" 1 "$S6/undo_timed_off.txt" modsdir="$M6/undo" arg=-gameseed=$GS
		# Recorded (mods/stage6/undo-replays: s6-undo and stage 3's replays feature mod) and played back: the UNDO events
		recording rec_undo_main "undo_main recorded: UNDO events, the refused ones not recorded; the sidecar next to the v2 save" \
			undo_main 1 "$S6/undo_main.txt" modsdir="$M6/undo-replays" arg=-gameseed=$GS
		playback watch_undo_main "rec_undo_main's sidecar: four UNDOs played back, each undo's board and the same refills" \
			"trace=rec_undo_main" "$C/rec_undo_main/user/savegame-*.bjr"

		# Stage 7: board sizes and shapes (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.3, 5). The rule mods of mods/stage7:
		# s7-6x6 and s7-10x7 (smaller, wider: unscaled and scaled), s7-16x16 (the largest, scale 406/816), s7-diamond (a
		# 9x9 mask of 41 cells) and s7-levels (a holed 8x8 and a 7x7 by turns, 5000 points a match: a level a match).
		# The autoplay bot plays each one (its clicks scaled like the trace's); compare.py "variant" checks every
		# settle step, and -codeccheck the snapshot round trip in every update. s7-levels changes shape at each vortex,
		# is saved on the 7x7 and resumed there (the background rebuilt for the loaded level), recorded and played back.
		S7="$PT/stage7"
		M7="$GAME/tools/oracle/mods/stage7"
		SHAPEBOT=(arg=-autoplaycap=20000 arg=-autoplaylog=autoplay.txt)
		variant shape_6x6_auto "-autoplay=1, mods/stage7/6x6: a 6x6 board, unscaled and centred, to game over at no moves" \
			"shapes=6x6:0" 1 - modsdir="$M7/6x6" arg=-autoplay=1 "${SHAPEBOT[@]}"
		variant shape_6x6_auto_timed "-autoplay=2 -autoplaytimed, mods/stage7/6x6, 20000 updates: fall-outs and refills, stale selections" \
			"shapes=6x6:0" 1 - modsdir="$M7/6x6" arg=-autoplay=2 arg=-autoplaytimed "${SHAPEBOT[@]}"
		variant shape_10x7_auto "-autoplay=1, mods/stage7/10x7, 20000 updates: a 10x7 board, scaled to the window's width" \
			"shapes=10x7:0;v2" 1 - modsdir="$M7/10x7" arg=-autoplay=1 "${SHAPEBOT[@]}"
		variant shape_16x16_auto "-autoplay=1, mods/stage7/16x16, 20000 updates: the largest board, at half size" \
			"shapes=16x16:0;v2" 1 - modsdir="$M7/16x16" arg=-autoplay=1 "${SHAPEBOT[@]}"
		variant shape_diamond_auto "-autoplay=1, mods/stage7/diamond: a diamond of 41 cells, gems falling through the holes, to game over" \
			"shapes=9x9:40" 1 - modsdir="$M7/diamond" arg=-autoplay=1 "${SHAPEBOT[@]}"
		variant shape_diamond_auto_timed "-autoplay=2 -autoplaytimed, mods/stage7/diamond, 20000 updates: fall-outs refill the diamond" \
			"shapes=9x9:40" 1 - modsdir="$M7/diamond" arg=-autoplay=2 arg=-autoplaytimed "${SHAPEBOT[@]}"
		variant shape_dirt_diamond_auto_timed "-autoplay=2 -autoplaytimed, mods/stage7/dirt-diamond (stage 1's s1-diamond): a hole inside the diamond, dirt cells played as holes until stage 7b" \
			"shapes=9x9:27" 1 - modsdir="$M7/dirt-diamond" arg=-autoplay=2 arg=-autoplaytimed "${SHAPEBOT[@]}"
		variant shape_levels_auto "-autoplay=1, mods/stage7/levels, 20000 updates: the holed 8x8 and the 7x7 by turns, a vortex at every level, 5000 points a match (the fast score roll)" \
			"shapes=8x8:10,7x7:0;v2" 1 - modsdir="$M7/levels" arg=-autoplay=1 "${SHAPEBOT[@]}"
		variant shape_levels_auto_timed "-autoplay=2 -autoplaytimed, mods/stage7/levels: the shapes by turns in timed mode" \
			"shapes=8x8:10,7x7:0" 1 - modsdir="$M7/levels" arg=-autoplay=2 arg=-autoplaytimed "${SHAPEBOT[@]}"
		variant shape_levels_quit "-gameseed=$GS, mods/stage7/levels: a match ends level 1 on the holed 8x8; the vortex, the wipe to the 7x7; the quit: a v2 save of level 2's shape" \
			"shapes=8x8:10,7x7:0;v2=last" 1 "$S7/shape_levels_quit.txt" modsdir="$M7/levels" arg=-gameseed=$GS
		variant shape_levels_quit.resume "shape_levels_quit's user dir: Yes resumes on the 7x7 (its background rebuilt); a match, the vortex back to the holed 8x8" \
			"shapes=8x8:10,7x7:0;v2=last" 1 "$S7/shape_levels_resume.txt" from=shape_levels_quit modsdir="$M7/levels"
		# Recorded (mods/stage7/levels-replays: s7-levels and stage 3's replays feature mod) and played back
		recording rec_shape_levels_auto "shape_levels_auto recorded: the shape changes in a replay, saved at the cap" \
			shape_levels_auto 1 - modsdir="$M7/levels-replays" arg=-autoplay=1 "${SHAPEBOT[@]}"
		recording rec_shape_levels_quit "shape_levels_quit recorded: the sidecar next to the v2 save of the 7x7" \
			shape_levels_quit 1 "$S7/shape_levels_quit.txt" modsdir="$M7/levels-replays" arg=-gameseed=$GS
		recording rec_shape_levels_quit.resume "shape_levels_quit.resume in a copy of rec_shape_levels_quit's user dir: the sidecar goes on (a v2 LOAD of the 7x7)" \
			shape_levels_quit.resume 1 "$S7/shape_levels_resume.txt" from=rec_shape_levels_quit modsdir="$M7/levels-replays"
		playback watch_shape_levels_auto "rec_shape_levels_auto's sidecar: every level's shape and refills again" \
			"trace=rec_shape_levels_auto" "$C/rec_shape_levels_auto/user/savegame-*.bjr"
		playback watch_shape_levels_resume "rec_shape_levels_quit.resume's sidecar: the second segment LOADs the 7x7's v2 save, then back to the holed 8x8" \
			"trace=rec_shape_levels_quit,rec_shape_levels_quit.resume" "$C/rec_shape_levels_quit.resume/user/savegame-*.bjr"
	else
		echo "$VTAG  (the candidate has no variant paths: the stage-5, 6 and 7 cases are left out)"
	fi

	# Stage 4: the headless verifier and the web build under Node (games/bejeweled/tools/verify/oracle_cases.sh): the
	# drift guard on every replay above, the verifier's tests and difftest, LOGIC hashes of the web build
	. "$GAME/tools/verify/oracle_cases.sh"
}

# The cache's part of every key that is the same for all runs (see the top)
if [ $USE_CACHE -eq 1 ]; then
	mkdir -p "$CACHE" || exit 2
	CACHE="$(cd "$CACHE" && pwd)"
	find "$CACHE" -mindepth 1 -maxdepth 1 -name '.tmp-*' -mmin +120 -exec rm -rf {} + 2> /dev/null
	find "$CACHE" -mindepth 2 -maxdepth 2 -name used -mtime +30 2> /dev/null | while read -r u; do rm -rf "$(dirname "$u")"; done
	MACHINE=$(powershell.exe -NoProfile -NonInteractive -Command '$ErrorActionPreference = "SilentlyContinue"
		Get-CimInstance Win32_VideoController | ForEach-Object { "gpu " + $_.Name + " driver " + $_.DriverVersion + " " + $_.CurrentHorizontalResolution + "x" + $_.CurrentVerticalResolution }
		"os " + (Get-CimInstance Win32_OperatingSystem).Version + "." + (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion").UBR
		"cpu " + (Get-CimInstance Win32_Processor | Select-Object -First 1).Name
		"dpi " + (Get-ItemProperty "HKCU:\Control Panel\Desktop\WindowMetrics").AppliedDPI' 2> /dev/null | tr -d '\r')
	case "$MACHINE" in
		*"gpu "*) ;;
		*) echo "compare.sh: cannot tell the GPU and display mode (powershell): every run is made now, the cache is left alone"
		   USE_CACHE=0 ;;
	esac
fi
if [ $USE_CACHE -eq 1 ]; then
	MODS_HASH=$("$PY" "$COMPARE_PY" tree-hash "$MODS_W" | tr -d '\r')
	CACHE_STATIC="oracle run cache 1
logic $(declare -f prepare_run exec_run mods_listing one | sha256sum | cut -d' ' -f1)
rundesk $(sha256sum "$RUNDESK" 2> /dev/null | cut -d' ' -f1)
assets $("$PY" "$COMPARE_PY" tree-hash "$ASSETS_W" | tr -d '\r')
visible ${ORACLE_VISIBLE:-}
env $(env | LC_ALL=C grep -E '^(SDL|ASAN|LSAN|UBSAN|BEJ|BYOPOP)_' | LC_ALL=C sort | tr '\n' ' ')
$MACHINE"
fi

# Each variant: a fresh <out>, exes.txt, cases.txt and its queued runs
NRUNS=0
for ((v = 0; v < NV; v++)); do
	set_variant $v
	rm -rf "$OUT/baseline" "$OUT/hooks-baseline" "$OUT/candidate" "$OUT/gen" "$OUT/emptymods"
	mkdir -p "$OUT/gen"
	for side in "${SIDES[@]}"; do mkdir -p "$OUT/$side"; done
	{
		for side in "${SIDES[@]}"; do
			if [ "$side" = candidate ]; then printf '%s\n' "${SIDE_LINES[candidate.$v]}"; else printf '%s\n' "${SIDE_LINES[$side]}"; fi
		done
		printf 'sides=%s\n' "${SIDES[*]}"
		printf 'candidate.args=%s\n' "${CAND_ARGS[*]}"
	} > "$OUT/exes.txt"
	: > "$OUT/cases.txt"
	[ $NV -eq 1 ] || echo "variant ${V_NAME[v]}: $OUT"
	for side in "${SIDES[@]}"; do echo "$VTAG$side: ${EXE[$side]} (stage-0 hooks: ${HOOKS[$side]})"; done
	before=$NJOBS
	define_cases
	for ((j = before; j < NJOBS; j++)); do
		case "${J_CMD[j]}" in do_run\ *) NRUNS=$((NRUNS + 1)) ;; esac
	done
done

# schedule: the queued jobs, each once the jobs it depends on are done, the lowest job number first (so --jobs 1
# runs them in the order of the definitions); with --jobs n > 1, n + 2 jobs at a time and n game slots
declare -A PIDJOB
FAILED=()
INFLIGHT=$JOBS
[ $JOBS -eq 1 ] || INFLIGHT=$((JOBS + 2))
SLOTS="${V_OUT[0]}/gen/slots"
rm -rf "$SLOTS"
mkdir -p "$SLOTS"
schedule() {
	local running=0 finished=0 low=0 i k d ready pid status
	while [ $finished -lt $NJOBS ]; do
		while [ $running -lt $INFLIGHT ]; do
			i=-1
			for ((k = low; k < NJOBS; k++)); do
				[ "${J_STATE[k]}" = 0 ] || continue
				ready=1
				for d in ${J_DEPS[k]}; do
					[ "${J_STATE[d]}" = 2 ] || { ready=0; break; }
				done
				if [ $ready -eq 1 ]; then
					i=$k
					break
				fi
			done
			[ $i -ge 0 ] || break
			J_STATE[i]=1
			( set_variant "${J_V[i]}"; eval "${J_CMD[i]}" ) &
			PIDJOB[$!]=$i
			running=$((running + 1))
			while [ $low -lt $NJOBS ] && [ "${J_STATE[low]}" != 0 ]; do low=$((low + 1)); done
		done
		if [ $running -eq 0 ]; then
			echo "compare.sh: queued jobs wait for jobs that never run" >&2  # dependencies are queued first: never
			return 1
		fi
		pid=""
		if [ $JOBS -eq 1 ]; then
			pid=$!
			wait "$pid"
		else
			wait -n -p pid
		fi
		status=$?
		[ -n "$pid" ] && [ -n "${PIDJOB[$pid]:-}" ] || continue
		i=${PIDJOB[$pid]}
		unset "PIDJOB[$pid]"
		J_STATE[i]=2
		running=$((running - 1))
		finished=$((finished + 1))
		[ $status -eq 0 ] || FAILED+=("${J_ID[i]}${V_NAME[${J_V[i]}]:+ (${V_NAME[${J_V[i]}]})}: status $status")
	done
}
# Stopped: stop the jobs (and the games under them) too
trap 'trap - INT TERM; kill 0 2> /dev/null; exit 130' INT TERM
schedule
SCHEDULED=$?
rm -rf "$SLOTS"

HITS=0
for ((v = 0; v < NV; v++)); do
	for side in baseline hooks-baseline; do
		[ -d "${V_OUT[v]}/$side" ] || continue
		HITS=$((HITS + $(find "${V_OUT[v]}/$side" -mindepth 2 -maxdepth 2 -name cached.txt | wc -l)))
	done
done
RUNTIME=$SECONDS

echo
status=0
if [ $NV -eq 1 ]; then
	"$PY" "$COMPARE_PY" compare "$(winpath "$OUT")" "$EXPECT" || status=$?
else
	# the comparisons at the same time (no game runs), their reports one after the other
	declare -a CMP_PID
	for ((v = 0; v < NV; v++)); do
		"$PY" "$COMPARE_PY" compare "$(winpath "${V_OUT[v]}")" "$EXPECT" > "${V_OUT[v]}/gen/compare.out" 2>&1 &
		CMP_PID[v]=$!
	done
	for ((v = 0; v < NV; v++)); do
		wait "${CMP_PID[v]}" || status=1
		echo "==== variant ${V_NAME[v]} (${V_OUT[v]}/report.txt)"
		cat "${V_OUT[v]}/gen/compare.out"
		echo
	done
	for ((v = 0; v < NV; v++)); do
		echo "variant ${V_NAME[v]}: $(tail -n 1 "${V_OUT[v]}/gen/compare.out" | tr -d '\r')"
	done
fi
# DRAWS (compare.py): the cache entry of a baseline run whose draw count differed is dropped, so that a draw the
# machine's speed added stays in no later oracle; the next one makes that run again
if [ $USE_CACHE -eq 1 ]; then
	DROPPED=0
	for ((v = 0; v < NV; v++)); do
		noted="${V_OUT[v]}/gen/draws-noted.txt"
		[ -s "$noted" ] && [ -f "${V_OUT[v]}/gen/cache-keys.txt" ] || continue
		while read -r run key; do
			[[ $key =~ ^[0-9a-f]{64}$ ]] && grep -qxF "$run" "$noted" && [ -d "$CACHE/$key" ] || continue
			rm -rf "${CACHE:?}/$key"
			DROPPED=$((DROPPED + 1))
		done < "${V_OUT[v]}/gen/cache-keys.txt"
	done
	[ $DROPPED -eq 0 ] ||
		echo "compare.sh: $DROPPED cache entries dropped, of baseline runs whose draw count differed (DRAWS): the next oracle makes them again"
fi
if [ $USE_CACHE -eq 1 ]; then
	echo "compare.sh: $NRUNS runs, $HITS of them from the cache ($CACHE), in $RUNTIME s with --jobs $JOBS"
else
	echo "compare.sh: $NRUNS runs (no cache) in $RUNTIME s with --jobs $JOBS"
fi
if [ ${#FAILED[@]} -gt 0 ] || [ $SCHEDULED -ne 0 ]; then
	printf 'compare.sh: a job could not run: %s\n' "${FAILED[@]}"
	exit 2
fi
exit $status
