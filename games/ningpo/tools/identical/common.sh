#!/bin/bash
# Shared settings of Ning Po's identical-play scripts (games/ningpo/docs/IDENTICAL_PLAY.md). Source it, do not run it.
#   OG_STAGE   where the original is staged (default C:/byo-og/np; a short path, outside the repository)
#   OUT        where a case's files go (default build/identical)
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 1
OG_STAGE="${OG_STAGE:-C:/byo-og/np}"
OG_GAME="$OG_STAGE/game"
OUT="${OUT:-build/identical}"
TOOLS=games/ningpo/tools/identical
RUNDESK="build/native/rundesk.exe"
ORIGRUN="build/origrun-bin/origrun.exe"
REGKEY='HKCU\Software\Pyrogon\NingPo'
export MSYS_NO_PATHCONV=1  # reg.exe takes /v and /f, which MSYS would turn into paths
win() { cygpath -w "$1"; }
