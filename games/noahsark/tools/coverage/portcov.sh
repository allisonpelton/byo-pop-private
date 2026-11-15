#!/bin/bash
# portcov.sh <function regex> <name>:<mode>:<difficulty>:<cap>:<seed>...: the port's own line coverage of bot games that mkbot.sh
# made. docs/COVERAGE.md counts which functions of the original a run reaches; this says which lines of the port's functions the
# same games run, so that a replay which agrees with the original is known to have gone through them.
#   bash games/noahsark/tools/coverage/portcov.sh 'TriggerNoMoves|AbandonPair' nm_str2:strategy:8:18400:2 nm_puz:puzzle:8:17700:1
# A build of the port with clang's source coverage (-fprofile-instr-generate -fcoverage-mapping) goes to build/native-cov, built
# here before the runs (a few minutes the first time, 40 MB). It plays each game's port script (build/coverage/noahsark/scripts/<name>.port.txt)
# with the arguments mkbot.sh gave it, under rundesk with -fast, and its state log must be the plain build's (the same game). The
# runs' counts are merged, and llvm-cov prints the lines of the functions the regex names with the number of times each ran, to
# build/coverage/noahsark/port/lines.txt; the summary line counts the lines that never ran. Waits for room.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
W="$(pwd -W)"
REGEX="$1"; shift
LL="${LLVM_MINGW_ROOT:-E:/retools/llvm-mingw-20261006-ucrt-x86_64}/bin"
S=build/coverage/noahsark/scripts; P=build/coverage/noahsark/port; mkdir -p "$P"
export PATH="${APPDATA:-}/Python/Python312/Scripts:$PATH"   # cmake and ninja, as tools/build.sh finds them
if [ ! -f build/native-cov/CMakeCache.txt ]; then
	cmake -S . -B build/native-cov -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$W/cmake/llvm-mingw-i686.cmake" \
		-DSDL2_DIR="${SDL2_DIR:-E:/retools/SDL2-2.32.10/i686-w64-mingw32/lib/cmake/SDL2}" \
		-DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping" -DCMAKE_C_FLAGS="-fprofile-instr-generate -fcoverage-mapping" \
		-DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate" > "$P/build.log" 2>&1 || { echo "portcov: the configure failed, see $P/build.log"; exit 1; }
fi
# every time, so that the counts are of the sources as they are now
cmake --build build/native-cov --target noahsark >> "$P/build.log" 2>&1 || { echo "portcov: the build failed, see $P/build.log"; exit 1; }
RAW=()
for run in "$@"; do
	IFS=: read -r N MODE DIFF CAP SEED <<< "$run"
	[ -f "$S/$N.port.txt" ] || { echo "portcov: no $S/$N.port.txt (run mkbot.sh for $N first)"; exit 1; }
	until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
	rm -rf "$P/ud_$N" "$P/$N.profraw"
	LLVM_PROFILE_FILE="$W/$P/$N.profraw" timeout 300 build/native/rundesk.exe "$W/build/native-cov/noahsark.exe" -userdir="$W/$P/ud_$N" -fast \
		-script="$W/$S/$N.port.txt" -seed="$SEED" -autoplay="$SEED" -autoplaymenu -autoplaymode="$MODE" -autoplaydiff="$DIFF" \
		-autoplaycap="$CAP" -statelog="$W/$P/$N.state" > "$P/$N.log" 2>&1
	rm -rf "$P/ud_$N"
	[ -f "$P/$N.profraw" ] || { echo "portcov: $N wrote no counts"; exit 1; }
	if cmp -s <(grep '^update' "$P/$N.state") <(grep '^update' "$S/$N.state"); then echo "$N: the same state log as the plain build"; else echo "$N: STATE LOG DIFFERS from the plain build's"; fi
	RAW+=("$P/$N.profraw")
done
"$LL/llvm-profdata.exe" merge -o "$P/merged.profdata" "${RAW[@]}" || exit 1
"$LL/llvm-cov.exe" show build/native-cov/noahsark.exe -instr-profile="$P/merged.profdata" -name-regex="$REGEX" -Xdemangler="$LL/llvm-cxxfilt.exe" > "$P/lines.txt" 2>&1
echo "$(grep -c '^ *[0-9]*| *[0-9.kM]*[1-9][0-9.kM]*|' "$P/lines.txt") lines ran, $(grep -c '^ *[0-9]*| *0|' "$P/lines.txt") never ran: $P/lines.txt"
grep -n '^ *[0-9]*| *0|' "$P/lines.txt" | cut -c1-150
