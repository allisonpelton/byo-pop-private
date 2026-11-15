#!/bin/bash
# run.sh <name> <steps> [timeout s]: one of the original's runs for the coverage check (docs/COVERAGE.md), an origrun
# --coverage run on the host, on the virtual clock with the scripted cursor and the seed and hover patches
# (tools/origrun/patches/bookwormadv.txt, as tools/pilot/seedpatch.py makes them). <steps> is a list of steps in the
# pilot's form (tools/pilot/gen.py: at, wait, click, text, key, focus, quit), timed by the game's own update counter;
# its shots are left out (a host run takes none). The hits go to build/coverage/bookwormadv/hits/<name>.txt, the log
# beside them.
#
# The start state: the game's players are in C:\ProgramData\PopCap Games\WinBAD\users and its settings in
# HKCU\Software\PopCap\WinBAD. backup/ (build/coverage/bookwormadv/backup, made once by hand: the users folder and a
# reg export) is put back after every run. PROFILES="<a.bwa> <b.bwa>..." are the players the run starts with (default:
# the backup's Tester.bwa alone, the pilot's start profile); PROFILE=<x.bwa> is a save played as Tester's (one of
# play.py's start profiles); LastUser is Tester.
#   CLOCK=real     the wall clock (the sound code runs only on it)
#   RUN=<id>       origrun's run folder (default cov1; the copy is staged under E:\byo-og\<id>)
set -u
ROOT="$(git rev-parse --show-toplevel)"; cd "$ROOT" || exit 1
NAME=$1; STEPS=$2; T=${3:-300}
C=build/coverage/bookwormadv; H=$C/hits; B=$C/backup
mkdir -p "$H" "$C/scripts"
U="/c/ProgramData/PopCap Games/WinBAD/users"
[ -f "$B/winbad.reg" ] && [ -f "$B/users/Tester.bwa" ] || { echo "no backup in $B"; exit 1; }
RUN=${RUN:-cov1}
CLK="--clock virtual"; [ "${CLOCK:-virtual}" = real ] && CLK="--clock real"

python -I games/bookwormadv/tools/pilot/gen.py "$STEPS" "$C/scripts/$NAME.orig.txt" "$C/scripts/$NAME.port.txt" "$C/scripts" > /dev/null || exit 1
# On rundesk's desktop the game's window never gets the focus, and the game takes no click until it has had it.
{ echo "at 1000 focus regain"; grep -vE '^(waitshot|shot|burst|record) |^at [0-9]+ (shot|burst) ' "$C/scripts/$NAME.orig.txt"; } > "$C/scripts/$NAME.txt"

restore() {
	rm -rf "$H/$NAME.users"; cp -r "$U" "$H/$NAME.users"   # what the run left, for a look afterwards
	rm -f "${U:?}"/*.bwa; cp "$B"/users/*.bwa "$U/"
	reg import "$(cygpath -w "$B/winbad.reg")" > /dev/null 2>&1
}
trap restore EXIT
rm -f "${U:?}"/*.bwa
for p in ${PROFILES:-$B/users/Tester.bwa}; do cp "$p" "$U/"; done
[ -n "${PROFILE:-}" ] && cp "$PROFILE" "$U/Tester.bwa"
reg add 'HKCU\Software\PopCap\WinBAD' //v LastUser //t REG_SZ //d Tester //f > /dev/null
# The original asks whether to check for updates (dialog 24, from LoadingThreadCompleted 0x4481a0) when its last query
# is old, and holds its title behind the question; the port leaves the update check out. The last query is now.
reg add 'HKCU\Software\PopCap\WinBAD' //v LastVerCheckQueryTime //t REG_DWORD //d "$(date +%s)" //f > /dev/null
reg add 'HKCU\Software\PopCap\WinBAD' //v InProgress //t REG_DWORD //d 0 //f > /dev/null

until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 5; done
R=build/origrun/$RUN; rm -f "$R/coverage.txt"
build/origrun-bin/origrun.exe --game bookwormadv --run "$RUN" --stage-root 'E:\byo-og' \
	--script "$(cygpath -w "$PWD/$C/scripts/$NAME.txt")" $CLK --cursor scripted --counter "[0x8259e4]+0x484" --update-ms 10 \
	--seed 12345 --coverage "$(cygpath -w "$PWD/$C/functions.txt")" --timeout "$T" > "$H/$NAME.log" 2>&1
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; tail -5 "$H/$NAME.log"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1); $(grep -E 'RESULT|SOUND LEAK' "$H/$NAME.log" | tail -2 | tr '\n' ' ')"
