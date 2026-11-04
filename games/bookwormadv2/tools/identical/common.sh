#!/bin/bash
# Shared settings of the identical-play scripts (games/bookwormadv2/docs/IDENTICAL_PLAY.md). Source it, do not run it.
#   OG_STAGE   where the original is staged (default C:/byo-og/bwa2; a short path, outside the repository)
#   OUT        where a case's files go (default build/identical)
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 1
OG_STAGE="${OG_STAGE:-C:/byo-og/bwa2}"
OG_GAME="$OG_STAGE/game"
OUT="${OUT:-build/identical}"
TOOLS=games/bookwormadv2/tools/identical
RUNDESK="build/native/rundesk.exe"
ORIGRUN="build/origrun-bin/origrun.exe"
PORT_EXE="${PORT_EXE:-build/native/bookwormadv2.exe}"
GAMEDIR="${GAMEDIR:-assets/bookwormadv2}"
# the app global (gSexyAppBase) and its update count at +0x484 (GetUpdateCount, 0x437e20)
COUNTER='[0x787170]+0x484'
win() { cygpath -w "$1"; }
