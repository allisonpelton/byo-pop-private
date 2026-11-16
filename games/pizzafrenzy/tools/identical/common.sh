#!/bin/bash
# Shared settings of Pizza Frenzy's identical-play scripts (games/pizzafrenzy/docs/IDENTICAL_PLAY.md). Source it.
#   PF_INSTALL  the installed game (default the Steam copy), read only: setup.sh copies it
#   OG_STAGE    where the original is staged (default E:/byo-og/pf; a short path, outside the repository)
#   OUT         where a case's files go (default build/identical)
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 1
PF_INSTALL="${PF_INSTALL:-A:/SteamLibrary/steamapps/common/Pizza Frenzy}"
OG_STAGE="${OG_STAGE:-E:/byo-og/pf}"
OG_GAME="$OG_STAGE/game"
OUT="${OUT:-build/identical}"
TOOLS=games/pizzafrenzy/tools/identical
RUNDESK="build/native/rundesk.exe"
ORIGRUN="build/origrun-bin/origrun.exe"
PATCHES="${PATCHES:-tools/origrun/patches/pizzafrenzy.txt}"
REGKEY='HKCU\Software\GameHouse\Pizza Frenzy'
# the carved game exe, in the main checkout's re/ (git-ignored)
GAME_EXE="${GAME_EXE:-$(git worktree list | head -1 | cut -d' ' -f1)/re/pizzafrenzy/bin/PizzaFrenzyGame.exe}"
win() { cygpath -w "$1"; }
