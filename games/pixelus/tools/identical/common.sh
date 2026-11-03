#!/bin/bash
# Shared settings of the identical-play scripts (games/pixelus/docs/IDENTICAL_PLAY.md). Source it, do not run it.
#   OG_STAGE   where the original is staged (default C:/byo-og/pix; a short path, outside the repository)
#   OUT        where a case's files go (default build/identical)
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 1
OG_STAGE="${OG_STAGE:-C:/byo-og/pix}"
OG_GAME="$OG_STAGE/game"
OUT="${OUT:-build/identical}"
TOOLS=games/pixelus/tools/identical
RUNDESK="build/native/rundesk.exe"
ORIGRUN="build/origrun-bin/origrun.exe"
win() { cygpath -w "$1"; }
