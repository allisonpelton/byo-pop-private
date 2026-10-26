# Stage 4 of the regression oracle: the headless verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9, 9.1
# items 6 and 7, the stage 4 row of 9.2). Sourced by games/bejeweled/tools/oracle/compare.sh at the end of its
# define_cases, once per variant, so it has that script's variables (ROOT, GAME, OUT, C, PT, S0, S3, M3, GS, SEEDBOT,
# EXE, CAND_ARGS, ASSETS_W, MODS_W, MODS_JSON, PY, COMPARE_PY) and functions (run, enqueue, dep_id, path_dep, one,
# game_slot_take, game_slot_give, winpath). Like the other cases it only queues jobs; compare.sh's scheduler runs
# them once the runs they read are done. The two long ones that start no game (the verifier's tests and its
# difftest) start at once, in the background, when the cases are defined: their jobs only wait for them. Cases
# (compare.py):
#   verify_tests       lint: games/bejeweled/tools/verify/run_tests.sh (RulesId vectors, files no player can start,
#                      RULE and diff.txt fuzz native against wasm, a short difftest native against wasm, bv_difftest's
#                      argument checks, command lines)
#   difftest           difftest: "bejeweled_verify difftest --seeds 1..N", N = $ORACLE_DIFFTEST_SEEDS (500), both
#                      modes: the plain and the -forcevariant board in lockstep, LOGIC compared every update (9.1 item 7)
#   <watch>.verify     verifier, the drift guard, for every playback case: the native and the wasm verifier play that
#                      case's replay.bjr with -trace (and the candidate's -forcevariant). Their "REPLAY" and "TRACE"
#                      lines must be the game's (but the TRACE update numbers, the app's count) and each other's, their
#                      JSON result the game's verdict. That is every replay of the corpus and the broken copies: game,
#                      native verifier and wasm verifier agree on TRACE, CHECK and END (9.1 item 6; native-recorded,
#                      wasm-verified)
#   verify_unplayable_<kind>  verifier: copies of rec_pause's sidecar that no player can start (cut short, another
#                      engine ABI, an RLID that is not RULE's SHA-256, RULE's normal points base and step 0, its normal
#                      multiplier base NaN: games/bejeweled/tools/verify/craft_replays.py copy), watched by the game too:
#                      the same "cannot play" line from all three
#   <case>.wasm        wasmlogic: the game's web build (build/web/bejeweled.js) under Node
#                      (games/bejeweled/tools/verify/wasmrun.js) runs the candidate's case with the same switches and
#                      inputs: hash.txt's update, board state and LOGIC columns and stdout.txt (TRACE, REPLAY) must be
#                      the candidate's (the playtests, the stage-0 scripts, the bot, a seeded game, a recording, two
#                      watches, and the two cases below)
# After the stage 4 review, two games whose (int) casts leave the int range, each recorded natively and by the web
# build, and each recording watched by the game and played by both verifiers (native-recorded and wasm-recorded,
# verified on both; no -codeccheck: their saves fail the LDSV save check on purpose, so the watches are verifier
# cases with verdict=ok, not playback cases):
#   overflow, rec_overflow  the timed bot game with a tunables mod (mods/stage4/overflow: setValues 2,000,000,000,
#                      times the multiplier 2): the score goes past the int range, INT_MIN as on x86; the reference and
#                      its recording (mods/stage4/overflow_rec, with the replays mod)
#   highlevel_load, rec_highlevel  save_states' vortex snapshot at level 10000 (craft_replays.py level), resumed:
#                      SetupLevel's mLevelPoints for level 10001 is past the int range; the recording's sidecar holds
#                      the save (LDSV)
# The verifier is the candidate build's (bejeweled_verify.exe next to the candidate exe, else build/native's); the
# wasm side is build/web's (bejeweled_verify.wasm, bejeweled.js) and emsdk's Node ($NODE overrides it). Neither
# verifier opens a window or plays a sound and neither counts as a game run; the web build under Node has no window
# and no audio device, and it takes one of the oracle's game slots, as a game.

V4="$GAME/tools/verify"
V4_WASM_W="$(winpath "$ROOT/build/web/bejeweled_verify.wasm")"
V4_GAMEJS_W="$(winpath "$ROOT/build/web/bejeweled.js")"
V4_RUNNER_W="$(winpath "$V4/wasmrun.js")"
V4_CRAFT_W="$(winpath "$V4/craft_replays.py")"
V4_M4="$GAME/tools/oracle/mods/stage4"
if [ -z "${V4_NODE:-}" ]; then
	V4_NODE="${NODE:-}"
	if [ -z "$V4_NODE" ]; then
		for n in "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe /e/retools/emsdk/node/*/node.exe; do
			[ -f "$n" ] && V4_NODE="$n"
		done
		[ -n "$V4_NODE" ] || V4_NODE="$(command -v node || true)"
	fi
fi

# v4_tools: the current variant's verifier (the one built next to its candidate exe) and its -forcevariant
v4_tools() {
	V4_NATIVE="$(dirname "${EXE[candidate]}")/bejeweled_verify.exe"
	[ -f "$V4_NATIVE" ] || V4_NATIVE="$ROOT/build/native/bejeweled_verify.exe"
	V4_FORCE=()
	local a
	for a in "${CAND_ARGS[@]}"; do
		[ "$a" = -forcevariant ] && V4_FORCE=(-forcevariant)
	done
}
v4_tools
echo "$VTAG  stage 4: verifier $V4_NATIVE, wasm $V4_WASM_W, web build $V4_GAMEJS_W, node $V4_NODE"

# The difftest and the verifier's tests: started now, in the background (no game, minutes of one CPU core); their
# jobs wait for them, at most ORACLE_TIMEOUT seconds (default 600) more than the difftest's own budget
v4_wait_job() {  # v4_wait_job <case>: until <case>/exit.txt is there
	local name=$1 waited=0
	while [ ! -f "$C/$name/exit.txt" ]; do
		if [ $waited -ge $((${ORACLE_TIMEOUT:-600} + 1800)) ]; then
			echo 124 > "$C/$name/exit.txt"
			break
		fi
		sleep 1
		waited=$((waited + 1))
	done
	echo "$VTAG  candidate $name: exit $(tr -d '\r\n' < "$C/$name/exit.txt")"
}
# (both start now, so a partial oracle starts only those its --cases name: case_wanted, compare.sh)
rm -rf "$C/difftest" "$C/verify_tests" "$OUT/gen/verify-tests"
if case_wanted difftest; then
	printf '%s\t%s\t%s\t\n' difftest difftest "bejeweled_verify difftest --seeds 1..${ORACLE_DIFFTEST_SEEDS:-500}: the plain and the -forcevariant board in lockstep, both modes, LOGIC every update" >> "$OUT/cases.txt"
	mkdir -p "$C/difftest"
	(cd "$C/difftest" && "$V4_NATIVE" difftest --seeds "1..${ORACLE_DIFFTEST_SEEDS:-500}" > report.txt 2> stderr.txt; echo $? > exit.txt) &
	enqueue candidate/difftest v4_wait_job difftest
fi
if case_wanted verify_tests; then
	printf '%s\t%s\t%s\t\n' verify_tests lint "games/bejeweled/tools/verify/run_tests.sh: RulesId vectors, unplayable files, RULE fuzz and difftest native = wasm, bv_difftest's arguments, command lines" >> "$OUT/cases.txt"
	mkdir -p "$C/verify_tests"
	(bash "$V4/run_tests.sh" --native "$V4_NATIVE" --wasm "$V4_WASM_W" --node "$V4_NODE" --out "$OUT/gen/verify-tests" > "$C/verify_tests/check.txt" 2>&1
	 echo $? > "$C/verify_tests/exit.txt") &
	enqueue candidate/verify_tests v4_wait_job verify_tests
fi

# v4_verify <case> <watch case> <flags>: both verifiers on that case's replay.bjr, once the watch has run
v4_verify_job() {
	local name=$1 watch=$2 dir="$C/$1"
	v4_tools
	rm -rf "$dir"
	mkdir -p "$dir"
	cp "$C/$watch/replay.bjr" "$dir/replay.bjr"
	(cd "$dir" && "$V4_NATIVE" replay.bjr -trace "${V4_FORCE[@]}" > native.txt 2> native-stderr.txt; echo $? > native-exit.txt)
	(cd "$dir" && "$V4_NODE" "$V4_RUNNER_W" verify "$V4_WASM_W" replay.bjr -trace "${V4_FORCE[@]}" > wasm.txt 2> wasm-stderr.txt
	 echo $? > wasm-exit.txt)
	echo "$VTAG  candidate $name: native exit $(cat "$dir/native-exit.txt"), wasm exit $(cat "$dir/wasm-exit.txt")"
}
v4_verify() {
	local name=$1 watch=$2 flags=$3
	printf '%s\t%s\t%s\t%s\n' "$name" verifier "the native and the wasm verifier play $watch's replay.bjr" "watch=$watch;$flags" >> "$OUT/cases.txt"
	DEPS=""
	dep_id "candidate/$watch"
	enqueue "candidate/$name" v4_verify_job "$name" "$watch"
}
# v4_craft <gen file> <arguments of craft_replays.py>: a file in <out>/gen, once the runs it reads are done
v4_craft_job() {
	"$PY" "$V4_CRAFT_W" "$@" > /dev/null
}
v4_craft() {
	local id=$1
	shift
	DEPS=""
	path_dep "$2"
	enqueue "gen/$id" v4_craft_job "$@"
}

# The drift guard: every playback case (all defined by now) played again by both verifiers
V4_CASES=$(cat "$OUT/cases.txt")
while IFS=$'\t' read -r name kind desc flags; do
	[ "$kind" = playback ] && v4_verify "$name.verify" "$name" "$flags"
done <<< "$V4_CASES"

# Copies no player can start, watched by the game and played by both verifiers
for kind in cut abi rlid diffzero diffnan; do
	v4_craft "unplayable_$kind.bjr" copy "$C/rec_pause/user/savegame.bjr" "$OUT/gen/unplayable_$kind.bjr" "$kind"
	run candidate "verify_unplayable_$kind.game" 1 "$S3/watch.txt" replay="$OUT/gen/unplayable_$kind.bjr"
	v4_verify "verify_unplayable_$kind" "verify_unplayable_$kind.game" "unplayable"
done

# v4_wasm <case> <seed> <script or -> [options]: the web build under Node, as compare.sh's prepare_run makes the
# candidate's run (options: mods=examples, modsdir=<dir>, from=<case> (that case's .wasm run), savegame=<file>,
# replay=<file or glob>, arg=<switch>)
v4_wasm_job() {
	local ref=$1 seed=$2 script=$3
	shift 3
	local dir="$C/$ref.wasm" modsdir=mods mods=none from="" savegame="" replay="" opt
	local args=("${CAND_ARGS[@]}")
	for opt in "$@"; do
		case "$opt" in
			mods=examples) mods=examples; modsdir="$MODS_W" ;;
			modsdir=*) modsdir="$(winpath "${opt#modsdir=}")" ;;
			from=*) from="${opt#from=}" ;;
			savegame=*) savegame="${opt#savegame=}" ;;
			replay=*) replay="${opt#replay=}"; args+=(-replay=replay.bjr -replayquit) ;;
			arg=*) args+=("${opt#arg=}") ;;
		esac
	done
	rm -rf "$dir"
	mkdir -p "$dir/shots" "$dir/snaps" "$dir/mods"
	if [ -n "$from" ]; then
		cp -r "$C/$from.wasm/user" "$dir/user"
	else
		mkdir -p "$dir/user"
		printf '; test settings\n\n[SOFTWARE\Steam\Bejeweled]\nScreenMode=dword:0\n' > "$dir/user/registry.ini"
	fi
	[ "$mods" = examples ] && printf '%s\n' "$MODS_JSON" > "$dir/user/mods.json"
	[ -n "$savegame" ] && cp "$savegame" "$dir/user/savegame.dat"
	if [ -n "$replay" ]; then
		case "$replay" in *[*?]*) replay=$(one "$replay") || return 2 ;; esac
		cp "$replay" "$dir/replay.bjr"
	fi
	if [ "$script" != - ]; then
		cp "$script" "$dir/script.txt"
		args+=("-script=script.txt")
	fi
	# wasmrun's own timeout first (exit 4), at 95% of the oracle's: it flushes stdout and writes out the game's files,
	# hash.txt to the last update, where timeout's kill (exit 124) would leave nothing to see how far a hung run got
	local aTimeout="${ORACLE_TIMEOUT:-600}"
	game_slot_take
	(cd "$dir" && timeout "$aTimeout" "$V4_NODE" "$V4_RUNNER_W" game --timeout "$((aTimeout * 19 / 20))" "$V4_GAMEJS_W" \
		-gamedir="$ASSETS_W" -userdir=user -modsdir="$modsdir" -seed="$seed" -fast -trace -tracehash=hash.txt "${args[@]}" \
		> stdout.txt 2> stderr.txt; echo $? > exit.txt)
	game_slot_give
	echo "$VTAG  candidate $ref.wasm: exit $(cat "$dir/exit.txt")"
}
v4_wasm() {
	local ref=$1 opt
	printf '%s\t%s\t%s\t%s\n' "$ref.wasm" wasmlogic "the web build under Node runs $ref: its LOGIC hashes and TRACE" "ref=$ref" >> "$OUT/cases.txt"
	DEPS=""
	path_dep "$3"
	for opt in "${@:4}"; do
		case "$opt" in
			from=*) dep_id "candidate/${opt#from=}.wasm" ;;
			savegame=*|replay=*|modsdir=*) path_dep "${opt#*=}" ;;
		esac
	done
	enqueue "candidate/$ref.wasm" v4_wasm_job "$@"
}
v4_wasm main 1 "$PT/playtest.txt"
v4_wasm load 1 "$PT/playtest_load.txt" from=main
v4_wasm level 3 "$PT/playtest_level.txt"
v4_wasm mods 3 "$PT/playtest_mods.txt" mods=examples
for s in drag_swaps input_edges rclick falling_select options_highscores pause nomoves_normal timed_fallout mode_switch; do
	v4_wasm "$s" 1 "$S0/$s.txt"
done
v4_wasm timed_fallout_load 1 "$S0/load_resume.txt" from=timed_fallout
v4_wasm mode_switch_load 1 "$S0/load_resume.txt" from=mode_switch
v4_wasm save_states 3 "$S0/save_states.txt"
v4_wasm autoplay_normal 1 - arg=-autoplay=1 arg=-autoplaylog=autoplay.txt
v4_wasm autoplay_timed 1 - arg=-autoplay=2 arg=-autoplaytimed arg=-autoplaylog=autoplay.txt
v4_wasm autoplay_cap 2 - arg=-autoplay=7 arg=-autoplaycap=6000 arg=-autoplaylog=autoplay.txt
v4_wasm seed_normal 1 - "${SEEDBOT[@]}"
v4_wasm rec_main 1 "$PT/playtest.txt" modsdir="$M3" arg=-codeccheck=codec.txt
v4_wasm watch_load 1 "$S3/watch.txt" replay="$C/rec_load/user/savegame.bjr" arg=-codeccheck=codec.txt
v4_wasm watch_autoplay_timed 1 "$S3/watch.txt" replay="$C/watch_autoplay_timed/replay.bjr" arg=-codeccheck=codec.txt

# After the review: (int) casts out of the int range, which x86 and wasm convert alike only with the web build's
# -mno-nontrapping-fptoint (games/bejeweled/CMakeLists.txt). Each game is played natively and by the web build
# (wasmlogic: the same LOGIC hashes), recorded by both, and each recording watched by the game and verified by both
# v4_watch <case> <replay file or glob>: the game watches it (no -codeccheck), a verifier case with verdict=ok
v4_watch() {
	run candidate "$1" 1 "$S3/watch.txt" replay="$2"
	v4_verify "$1.verify" "$1" "verdict=ok"
}
OVERFLOW_BOT=(arg=-autoplay=2 arg=-autoplaytimed arg=-autoplaycap=3000 arg=-autoplaylog=autoplay.txt)
printf '%s\t%s\t%s\t\n' overflow reference "the -autoplay=2 timed game with setValues 2,000,000,000 (mods/stage4/overflow): the score past the int range, INT_MIN" >> "$OUT/cases.txt"
run candidate overflow 1 - modsdir="$V4_M4/overflow" "${OVERFLOW_BOT[@]}"
printf '%s\t%s\t%s\t%s\n' rec_overflow recording "overflow recorded (mods/stage4/overflow_rec: the tunables mod and the replays mod)" "ref=overflow" >> "$OUT/cases.txt"
run candidate rec_overflow 1 - modsdir="$V4_M4/overflow_rec" "${OVERFLOW_BOT[@]}"
v4_wasm overflow 1 - modsdir="$V4_M4/overflow" "${OVERFLOW_BOT[@]}"
v4_wasm rec_overflow 1 - modsdir="$V4_M4/overflow_rec" "${OVERFLOW_BOT[@]}"
v4_watch watch_overflow "$C/rec_overflow/user/replays/*.bjr"
v4_watch watch_overflow_web "$C/rec_overflow.wasm/user/replays/*.bjr"
v4_craft s5_vortex_level10000.dat level "$OUT/candidate/save_states/snaps/s5_vortex.dat" "$OUT/gen/s5_vortex_level10000.dat" 10000
HIGHLEVEL=(savegame="$OUT/gen/s5_vortex_level10000.dat")
printf '%s\t%s\t%s\t\n' highlevel_load reference "load_resume.txt with save_states' vortex snapshot at level 10000: level 10001's points past the int range" >> "$OUT/cases.txt"
run candidate highlevel_load 1 "$S0/load_resume.txt" "${HIGHLEVEL[@]}"
printf '%s\t%s\t%s\t%s\n' rec_highlevel recording "highlevel_load recorded: a fresh replay that holds the loaded save (LDSV)" "ref=highlevel_load" >> "$OUT/cases.txt"
run candidate rec_highlevel 1 "$S0/load_resume.txt" "${HIGHLEVEL[@]}" modsdir="$M3"
v4_wasm highlevel_load 1 "$S0/load_resume.txt" "${HIGHLEVEL[@]}"
v4_wasm rec_highlevel 1 "$S0/load_resume.txt" "${HIGHLEVEL[@]}" modsdir="$M3"
v4_watch watch_highlevel "$C/rec_highlevel/user/savegame.bjr"
v4_watch watch_highlevel_web "$C/rec_highlevel.wasm/user/savegame.bjr"
