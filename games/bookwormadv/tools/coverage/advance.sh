#!/bin/bash
# advance.sh <minutes>: the port's bot (tools/movedriver.py, -fast) plays the adventure on from the start profile, and
# every save of the player's file is kept as build/coverage/bookwormadv/profiles/adv/<book>.<chapter>.<stage>.bwa: start
# profiles for the original's coverage runs at later chapters.
# GAME_ARGS: more options for the game (the screen pass's run: -livesplitlog and a mods folder with the livesplit mod).
set -u
ROOT="$(git rev-parse --show-toplevel)"; cd "$ROOT" || exit 1
B=build/coverage/bookwormadv; O=$B/port/advance; P=$B/profiles/adv
rm -rf "$O"; mkdir -p "$O/user/users" "$P"
printf '[SOFTWARE\PopCap\WinBAD]\nLastUser=sz:Tester\nTotalBooks=dword:3\nScreenMode=dword:0\n' > "$O/user/registry.ini"
cp "${START:-$B/backup/users/Tester.bwa}" "$O/user/users/Tester.bwa"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 5; done
w() { cygpath -w "$PWD/$1"; }
build/native/rundesk.exe python -I "$(w games/bookwormadv/tools/movedriver.py)" "$(w "$O/md")" - $(( $1 * 60 )) "goto adventure" -- \
	"$(w build/native/bookwormadv.exe)" -userdir="$(w "$O/user")" -seed=12345 ${GAME_ARGS:-} &
F="$O/user/users/Tester.bwa"; last=""
while kill -0 $! 2> /dev/null; do
	sleep 0.2
	m=$(stat -c %Y "$F" 2> /dev/null); [ "$m" = "$last" ] && continue; last=$m
	lv=$(python -I games/bookwormadv/tools/coverage/bwa.py dump "$F" CurrentBook CurrentChapter CurrentStage 2> /dev/null | awk '{printf "%s.", $3}')
	[ -n "$lv" ] && cp "$F" "$P/${lv%.}.bwa"
done
tail -1 "$O/md/movedriver.log"; ls "$P" | tr '\n' ' '
