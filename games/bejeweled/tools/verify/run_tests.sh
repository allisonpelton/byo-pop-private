#!/usr/bin/env bash
# The headless verifier's own tests (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9): what needs no game run. The
# oracle (games/bejeweled/tools/oracle/compare.sh) runs it as its case "verify_tests"; the replays themselves
# are checked there, against the game (games/bejeweled/tools/verify/oracle_cases.sh).
#
#   games/bejeweled/tools/verify/run_tests.sh [--native <bejeweled_verify.exe>] [--wasm <bejeweled_verify.wasm>]
#       [--node <node.exe>] [--seeds <n>] [--out <dir>]
#
# Defaults: build/native/bejeweled_verify.exe, build/web/bejeweled_verify.wasm, emsdk's Node, 10 seeds, a
# temporary folder under build/. Checks, each printed as "ok ..." or "VIOLATION ...", then a summary line;
# exit status 0 only without a violation:
#   rulesid     the RulesId of fixed mod.json texts, native and wasm, against the vectors of
#               games/bejeweled/tools/rulestest (the default rules, a tunables-only mod), and alike on both: a
#               mod.json that is not JSON, one over 1 MB, one of more than 32 match.shapes (refused as it is
#               parsed), and 32 shapes of 16 cells on 64 levels of 16x16 (Validate's fill minimum at its largest)
#   unplayable  files no replay player can start (empty, garbage, a header alone; craft_replays.py synthetic:
#               RULEs with diff.txt values that make a level of no points or are not finite, a field out of range,
#               at its default or unknown, a NaN tunable; more than MAX_SEGMENTS segments, more than
#               MAX_EVENTS_PER_TICK events at one tick, a SAVE before the last tick, five SAVEs): both verifiers
#               say "cannot play" with the same words and JSON, exit status 1 (the oracle adds broken copies of
#               real replays, played by the game too)
#   fuzz        RULEs with extreme diff.txt values the check takes (craft_replays.py synthetic: points and
#               multipliers to 1e300, the int range's end, drains to +-1e300 and the smallest double, so
#               SetupLevel's (int) casts and the drain leave the int range), played in three passes (to the first
#               settle, a CHECK there, then on to END): native and wasm print the same lines, LOGIC hashes included
#   difftest    the plain and the -forcevariant board on seeds 1..n, both modes, native and wasm: every game
#               equal, and each game's line (its length, end state, score and last LOGIC hash) the same on
#               both: native and wasm play the board alike; bv_difftest's own argument checks (the export takes
#               them as they come): seed 0, a range backwards or too long, ticks and modes out of range
#   usage       wrong command lines: exit status 2
# Neither verifier opens a window or plays a sound, so nothing here needs rundesk.
set -u

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
GAME="$ROOT/games/bejeweled"
winpath() { if command -v cygpath > /dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi; }
NATIVE="$ROOT/build/native/bejeweled_verify.exe"
WASM="$ROOT/build/web/bejeweled_verify.wasm"
NODE="${NODE:-}"
PYTHON="${PYTHON:-python}"
SEEDS=10
OUT=""
while [ $# -gt 0 ]; do
	case "$1" in
		--native) NATIVE="$2"; shift 2 ;;
		--wasm) WASM="$2"; shift 2 ;;
		--node) NODE="$2"; shift 2 ;;
		--seeds) SEEDS="$2"; shift 2 ;;
		--out) OUT="$2"; shift 2 ;;
		*) sed -n '2,31p' "$0"; exit 2 ;;
	esac
done
if [ -z "$NODE" ]; then
	for n in "${EMSDK_ROOT:-E:/retools/emsdk}"/node/*/node.exe /e/retools/emsdk/node/*/node.exe; do
		[ -x "$n" ] && NODE="$n"
	done
	[ -n "$NODE" ] || NODE="$(command -v node || true)"
fi
RUNNER="$(winpath "$GAME/tools/verify/wasmrun.js")"
WASM_W="$(winpath "$WASM")"
[ -n "$OUT" ] || OUT="$ROOT/build/verify-tests"
rm -rf "$OUT"
mkdir -p "$OUT"

CHECKS=0
FAILS=0
ok() { CHECKS=$((CHECKS + 1)); echo "ok $*"; }
bad() { CHECKS=$((CHECKS + 1)); FAILS=$((FAILS + 1)); echo "VIOLATION $*"; }
for f in "$NATIVE" "$WASM" "$NODE"; do
	if [ ! -f "$f" ]; then
		echo "VIOLATION missing: $f"
		echo "verify tests: 0 checks, 1 failure"
		exit 1
	fi
done
# The native verifier with LF line ends (its stdout is in text mode), keeping its exit status
nat() {
	"$NATIVE" "$@" > "$OUT/native.out"
	local status=$?
	tr -d '\r' < "$OUT/native.out"
	rm -f "$OUT/native.out"
	return $status
}
wsm() { "$NODE" "$RUNNER" "$@"; }

# rulesid: the vectors of games/bejeweled/tools/rulestest (TestVectors)
rulesid_case() {
	local name=$1 json=$2 want=$3 dir="$OUT/rulesid-$1"
	mkdir -p "$dir"
	printf '%s\n' "$json" > "$dir/mod.json"
	nat rulesid "$dir/mod.json" > "$dir/native.txt"
	wsm rulesid "$WASM_W" "$(winpath "$dir/mod.json")" > "$dir/wasm.txt"
	if ! cmp -s "$dir/native.txt" "$dir/wasm.txt"; then
		bad "rulesid $name: native and wasm differ: $(head -c 200 "$dir/native.txt") / $(head -c 200 "$dir/wasm.txt")"
	elif ! grep -q "\"rulesId\":\"$want\"" "$dir/native.txt"; then
		bad "rulesid $name: $(head -c 200 "$dir/native.txt"), expected $want"
	else
		ok "rulesid $name: $want, native and wasm"
	fi
}
rulesid_case default '{}' 0e0943ce3dc5f1735c47f27c4dd38871daec36ba999f20926bb1a8ec3f5032b3
rulesid_case example-relaxed '{"tunables": {"scoring": {"hintPenalty": 10}}}' 3af35ac3966e672b703bb6ef43acfa840ffc5763d987b4c55df9c84a04060b73
mkdir -p "$OUT/rulesid-notjson"
printf '{"rules": ' > "$OUT/rulesid-notjson/mod.json"
nat rulesid "$OUT/rulesid-notjson/mod.json" > "$OUT/rulesid-notjson/native.txt"
wsm rulesid "$WASM_W" "$(winpath "$OUT/rulesid-notjson/mod.json")" > "$OUT/rulesid-notjson/wasm.txt"
if cmp -s "$OUT/rulesid-notjson/native.txt" "$OUT/rulesid-notjson/wasm.txt" && grep -q '^{"error":"not JSON' "$OUT/rulesid-notjson/native.txt"; then
	ok "rulesid: a mod.json that is not JSON is refused alike: $(cat "$OUT/rulesid-notjson/native.txt")"
else
	bad "rulesid notjson: $(cat "$OUT/rulesid-notjson/native.txt") / $(cat "$OUT/rulesid-notjson/wasm.txt")"
fi
# The same answer on both, containing <text>, for mod.json texts that cost the most: over 1 MB, more shapes than
# a rule set may have, and the largest fill minimum Validate computes (32 shapes of 16 cells, 64 levels of 16x16)
rulesid_alike() {
	local name=$1 want=$2 dir="$OUT/rulesid-$1" start
	start=$(date +%s%N)
	nat rulesid "$dir/mod.json" > "$dir/native.txt"
	wsm rulesid "$WASM_W" "$(winpath "$dir/mod.json")" > "$dir/wasm.txt"
	local ms=$((($(date +%s%N) - start) / 1000000))
	if cmp -s "$dir/native.txt" "$dir/wasm.txt" && grep -q "$want" "$dir/native.txt"; then
		ok "rulesid $name: alike, $(head -c 160 "$dir/native.txt") ($ms ms for both)"
	else
		bad "rulesid $name: $(head -c 200 "$dir/native.txt") / $(head -c 200 "$dir/wasm.txt")"
	fi
}
mkdir -p "$OUT/rulesid-huge" "$OUT/rulesid-shapes" "$OUT/rulesid-fill"
"$PYTHON" - "$(winpath "$OUT")" <<'EOF'
import json, random, sys
out = sys.argv[1]
open(out + "/rulesid-huge/mod.json", "w").write('{"tunables": {"x": "' + "x" * (1024 * 1024) + '"}}')
random.seed(7)
def shapes(n):
    d = {}
    for i in range(n):
        cells = set()
        while len(cells) < 16:
            cells.add((random.randrange(16), random.randrange(16)))
        d["s%d" % i] = {"cells": [list(c) for c in sorted(cells)], "rotations": True, "mirror": True}
    return d
levels = [{"mask": ["".join("#" if (x * 7 + y * 3 + i) % 11 else "." for x in range(16)) for y in range(16)]} for i in range(64)]
json.dump({"rules": {"match": {"shapes": shapes(1000)}}}, open(out + "/rulesid-shapes/mod.json", "w"))
json.dump({"rules": {"board": {"gemColors": 12, "levels": levels}, "match": {"shapes": shapes(32)}}}, open(out + "/rulesid-fill/mod.json", "w"))
EOF
rulesid_alike huge '"error":"more than 1048576 bytes'
rulesid_alike shapes 'more than the 32 shapes'
rulesid_alike fill 'the fewest colours'

# unplayable: both verifiers, the same words, result "unplayable", exit status 1
unplayable_case() {
	local name=$1 dir="$OUT/unplayable-$1"
	local nexit wexit
	nat "$dir/replay.bjr" > "$dir/native.txt"
	nexit=$?
	wsm verify "$WASM_W" "$(winpath "$dir/replay.bjr")" > "$dir/wasm.txt"
	wexit=$?
	if ! cmp -s "$dir/native.txt" "$dir/wasm.txt"; then
		bad "unplayable $name: native and wasm differ: $(head -n 1 "$dir/native.txt") / $(head -n 1 "$dir/wasm.txt")"
	elif [ "$nexit" != 1 ] || [ "$wexit" != 1 ] || ! grep -q '"result":"unplayable"' "$dir/native.txt" || \
			! grep -q '^REPLAY replay.bjr: cannot play: ' "$dir/native.txt"; then
		bad "unplayable $name: exit $nexit/$wexit: $(head -c 300 "$dir/native.txt")"
	else
		ok "unplayable $name: $(head -n 1 "$dir/native.txt")"
	fi
}
for name in empty garbage header; do
	mkdir -p "$OUT/unplayable-$name"
done
: > "$OUT/unplayable-empty/replay.bjr"
printf 'not a replay' > "$OUT/unplayable-garbage/replay.bjr"
printf 'BJRP\001\000\000\000\001\000\000\000' > "$OUT/unplayable-header/replay.bjr"
for name in empty garbage header; do
	unplayable_case "$name"
done
# Well-formed files with what the player refuses: RULEs a game never records, contents past the bounds
mkdir -p "$OUT/synthetic"
SYNTH=$("$PYTHON" "$(winpath "$GAME/tools/verify/craft_replays.py")" synthetic "$(winpath "$OUT/synthetic")" | tr -d '\r')
for name in $SYNTH; do
	case "$name" in
		rule-*|limits-*)
			mkdir -p "$OUT/unplayable-$name"
			cp "$OUT/synthetic/$name.bjr" "$OUT/unplayable-$name/replay.bjr"
			unplayable_case "$name"
			;;
	esac
done

# fuzz: extreme diff.txt values the RULE check takes, in three passes (to the first settle; a CHECK of hash 0
# there, which prints the LOGIC hash; that hash, so the board plays on to END): native and wasm alike each time
fuzz_pass() {  # fuzz_pass <name> <file>: both verifiers with -trace; the native lines into $FUZZ_LINES
	local dir="$OUT/fuzz-$1"
	(cd "$dir" && "$NATIVE" "$2" -trace | tr -d '\r' | sed "s/$2/replay.bjr/" > native.txt)
	(cd "$dir" && "$NODE" "$RUNNER" verify "$WASM_W" "$2" -trace > wasm.txt)
	FUZZ_LINES=$(cat "$dir/native.txt")
	cmp -s "$dir/native.txt" "$dir/wasm.txt"
}
for name in $SYNTH; do
	case "$name" in fuzz-*) ;; *) continue ;; esac
	dir="$OUT/fuzz-$name"
	mkdir -p "$dir"
	cp "$OUT/synthetic/$name.bjr" "$dir/pass1.bjr"
	verdict="ok"
	if ! fuzz_pass "$name" pass1.bjr; then
		verdict="pass 1 differs"
	else
		settle=$(printf '%s\n' "$FUZZ_LINES" | sed -n 's/.*settled into play at tick \([0-9]*\),.*/\1/p' | head -n 1)
		if [ -z "$settle" ]; then
			verdict="pass 1 found no settle: $(printf '%s\n' "$FUZZ_LINES" | grep -m 1 'REPLAY end')"
		else
			"$PYTHON" "$(winpath "$GAME/tools/verify/craft_replays.py")" check "$(winpath "$dir/pass1.bjr")" "$(winpath "$dir/pass2.bjr")" "$settle"
			if ! fuzz_pass "$name" pass2.bjr; then
				verdict="pass 2 differs"
			else
				hash=$(printf '%s\n' "$FUZZ_LINES" | sed -n 's/.*: LOGIC hash \([0-9a-f]*\), the replay.*/\1/p' | head -n 1)
				"$PYTHON" "$(winpath "$GAME/tools/verify/craft_replays.py")" check "$(winpath "$dir/pass1.bjr")" "$(winpath "$dir/pass3.bjr")" "$settle" "$hash"
				fuzz_pass "$name" pass3.bjr || verdict="pass 3 differs"
			fi
		fi
	fi
	if [ "$verdict" = ok ]; then
		ok "fuzz $name: native and wasm alike: settle at tick $settle, LOGIC $hash, $(printf '%s\n' "$FUZZ_LINES" | grep -m 1 'REPLAY end' | cut -c 1-110)"
	else
		bad "fuzz $name: $verdict: $(diff "$dir/native.txt" "$dir/wasm.txt" | head -n 3 | tr '\n' ' ')"
	fi
done

# difftest: native and wasm, every game equal and each game's line the same on both
mkdir -p "$OUT/difftest"
nat difftest --seeds "1..$SEEDS" -v > "$OUT/difftest/native.txt"
nexit=$?
wsm difftest "$WASM_W" --seeds "1..$SEEDS" -v > "$OUT/difftest/wasm.txt"
wexit=$?
if [ "$nexit" != 0 ] || [ "$wexit" != 0 ]; then
	bad "difftest 1..$SEEDS: exit $nexit (native) / $wexit (wasm): $(grep -m 3 DIFFERS "$OUT/difftest/native.txt" "$OUT/difftest/wasm.txt")"
elif ! cmp -s "$OUT/difftest/native.txt" "$OUT/difftest/wasm.txt"; then
	bad "difftest 1..$SEEDS: native and wasm differ: $(diff "$OUT/difftest/native.txt" "$OUT/difftest/wasm.txt" | head -n 3 | tr '\n' ' ')"
else
	ok "difftest 1..$SEEDS: $(grep -c ': equal$' "$OUT/difftest/native.txt") games equal, the same on native and wasm; $(grep '^difftest normal:' "$OUT/difftest/native.txt" | sed 's/^difftest //;s/;.*//'), $(grep '^difftest timed:' "$OUT/difftest/native.txt" | sed 's/^difftest //;s/;.*//')"
fi
# bv_difftest checks what it is given (the export takes it as it comes; the command lines check before): each
# call below returns "difftest: <why>" at once, plays nothing
cat > "$OUT/difftest/export.js" <<'EOF'
const fs = require("fs");
const aModule = new WebAssembly.Module(fs.readFileSync(process.argv[2]));
let aMemory = null;
const anImports = {};
for (const i of WebAssembly.Module.imports(aModule)) {
	anImports[i.module] = anImports[i.module] || {};
	anImports[i.module][i.name] = () => 0;
}
const ex = new WebAssembly.Instance(aModule, anImports).exports;
aMemory = ex.memory;
ex._initialize();
for (const [aFirst, aLast, aTicks, aModes] of [[5, 1, 0, 3], [0, 3, 0, 3], [1, 100001, 0, 3], [1, 1, -5, 3], [1, 1, 8640001, 3], [1, 1, 0, 0], [1, 1, 0, 4]]) {
	const aPtr = ex.bv_difftest(aFirst >>> 0, aLast >>> 0, aTicks, aModes);
	const aHeap = new Uint8Array(aMemory.buffer);
	let anEnd = aPtr;
	while (aHeap[anEnd] !== 0)
		anEnd++;
	process.stdout.write(aFirst + " " + aLast + " " + aTicks + " " + aModes + ": " + Buffer.from(aHeap.subarray(aPtr, anEnd)).toString());
}
EOF
"$NODE" "$(winpath "$OUT/difftest/export.js")" "$WASM_W" > "$OUT/difftest/export.txt" 2>&1
if [ "$(grep -c ': difftest: ' "$OUT/difftest/export.txt")" = 7 ] && [ "$(wc -l < "$OUT/difftest/export.txt")" = 7 ]; then
	ok "bv_difftest refuses what it does not take: $(sed 's/^[^:]*: difftest: //' "$OUT/difftest/export.txt" | tr '\n' ';' | sed 's/;$//;s/;/; /g')"
else
	bad "bv_difftest arguments: $(tr '\n' '|' < "$OUT/difftest/export.txt" | head -c 400)"
fi

# usage
usage_case() {
	"$NATIVE" "$@" > /dev/null 2>&1
	local e=$?
	local shown="${*//$OUT/<out>}"
	if [ "$e" = 2 ]; then ok "usage: bejeweled_verify $shown -> exit 2"; else bad "usage: bejeweled_verify $shown -> exit $e, expected 2"; fi
}
usage_case
usage_case difftest
usage_case difftest --seeds 5..1
usage_case difftest --seeds 0..3
usage_case "$OUT/no-such-file.bjr"
usage_case "$OUT/unplayable-empty/replay.bjr" -nosuchswitch

echo "verify tests: $CHECKS checks, $FAILS failures"
[ "$FAILS" -eq 0 ]
