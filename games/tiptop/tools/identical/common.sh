#!/bin/bash
# Shared settings of the identical-play scripts (games/tiptop/docs/IDENTICAL_PLAY.md). Source it, do not run it.
#   OG_STAGE   where the original is staged (default C:/byo-og/tt; a short path, outside the repository)
#   OUT        where a case's files go (default build/identical-tiptop)
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 1
OG_STAGE="${OG_STAGE:-C:/byo-og/tt}"
OG_GAME="$OG_STAGE/game"
OUT="${OUT:-build/identical-tiptop}"
TOOLS=games/tiptop/tools/identical
RUNDESK="build/native/rundesk.exe"
ORIGRUN="build/origrun-bin/origrun.exe"
PORT_EXE="${PORT_EXE:-build/native/tiptop.exe}"
win() { cygpath -w "$1"; }
