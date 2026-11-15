#!/bin/bash
# Shared settings of the identical-play scripts (games/dynomite/docs/IDENTICAL_PLAY.md). Source it, do not run it.
#   OG_STAGE   where the original is staged (default C:/byo-og/dyn; a short path, outside the repository)
#   OUT        where a case's files go (default build/identical)
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 1
OG_STAGE="${OG_STAGE:-C:/byo-og/dyn}"
OG_GAME="$OG_STAGE/game"
OUT="${OUT:-build/identical}"
TOOLS=games/dynomite/tools/identical
RUNDESK="build/native/rundesk.exe"
ORIGRUN="build/origrun-bin/origrun.exe"
win() { cygpath -w "$1"; }
# The original lets one copy run at a time on the host: SexyApp's Init makes a mutex named after the game, and a second
# copy exits at once (code 0) while it is held. So every run of the original on the host (orig.sh, sound.sh) takes this
# lock first, then waits for any copy still running; runs in Windows Sandbox (tour.sh) are on another machine and need
# none. The lock goes when the script exits; one whose owner is gone is taken over.
HOSTLOCK="$ROOT/build/origrun/host.lock"
hostlock() {
	mkdir -p "$(dirname "$HOSTLOCK")"
	while ! mkdir "$HOSTLOCK" 2> /dev/null; do
		local p; p=$(cat "$HOSTLOCK/pid" 2> /dev/null)
		if [ -n "$p" ] && ! kill -0 "$p" 2> /dev/null; then rm -rf "$HOSTLOCK"; continue; fi
		sleep 5
	done
	echo $$ > "$HOSTLOCK/pid"
	trap 'rm -rf "$HOSTLOCK"' EXIT
	while tasklist //FI "IMAGENAME eq DynomiteGame.exe" //NH 2> /dev/null | grep -q DynomiteGame; do sleep 5; done
}
