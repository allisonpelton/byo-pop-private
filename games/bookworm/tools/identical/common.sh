#!/bin/bash
# Shared settings of BookWorm's identical-play scripts (games/bookworm/docs/IDENTICAL_PLAY.md). Source it, do not run it.
#   OG_STAGE   where the original is staged (default C:/byo-og/bw; a short path, outside the repository)
#   OUT        where a case's files go (default build/identical)
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 1
OG_STAGE="${OG_STAGE:-C:/byo-og/bw}"
OG_MASTER="$OG_STAGE/master"   # setup.sh's staged copy, never run
OG_GAME="$OG_STAGE/game"       # the copy a run uses, made afresh from the master
REGKEY='HKCU\Software\PopCap\BookWorm'   # the original's settings and players
OUT="${OUT:-build/identical}"
TOOLS=games/bookworm/tools/identical
RUNDESK="build/native/rundesk.exe"
ORIGRUN="build/origrun-bin/origrun.exe"
APP='[0x5347d4]'           # gApp, the BookWormApp the original's WinMain makes
COUNTER="$APP+0x4b8"       # SexyAppBase::mUpdateCount
win() { cygpath -w "$1"; }
export MSYS_NO_PATHCONV=1
