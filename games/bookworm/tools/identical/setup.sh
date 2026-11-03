#!/bin/bash
# Stages the original for the identical-play runs: a copy of the retail install (version 1.03, the one decompiled; the
# Steam copy in tools/origrun/games.tsv wraps a later build) under a short path, with the game carved out of its
# Bookworm.exe (re/bookworm/bin/BookwormGame.exe) next to it, and the patch list that gives the run's seed
# to both of SexyApp::Init's GetTickCount() calls: the one whose value seeds the framework's generator (SRand) and the
# one that seeds the C library's (srand), as the port's -seed=N does. Nothing here runs the game.
source "$(dirname "$0")/common.sh"
[ -f re/bookworm/bin/BookwormGame.exe ] || { echo "re/bookworm/bin/BookwormGame.exe is missing"; exit 1; }
RETAIL="${RETAIL:-C:/Program Files/PopCap Games/BookWorm Deluxe}"
[ -f "$RETAIL/Bookworm.exe" ] || { echo "no retail install at $RETAIL"; exit 1; }
rm -rf "$OG_MASTER"; mkdir -p "$OG_STAGE"
cp -r "$RETAIL" "$OG_MASTER" || exit 1
cp re/bookworm/bin/BookwormGame.exe "$OG_MASTER/BookwormGame.exe" || exit 1
mkdir -p tools/origrun/patches
{
	echo 'seed BookwormGame.exe va:0x48adbe ff156c904f00 b8{seed}90 GetTickCount() in SexyApp::Init seeding SRand takes the run'"'"'s seed'
	echo 'seed BookwormGame.exe va:0x48affe ff156c904f00 b8{seed}90 GetTickCount() in SexyApp::Init seeding srand takes the run'"'"'s seed'
	# The Board constructor never sets mLexPraisePending (+0x358), so the first board of a session starts with heap
	# contents (nonzero in every run seen: one praise draw of the board's generator, with no praise, the first time Lex
	# rests) and a later board with the board before it's (IDENTICAL_PLAY.md, "Oddities"). The port starts it false;
	# this store of the byte at +0x357 becomes a dword store that clears +0x357 to +0x35a, the flag and two of
	# the three bytes of padding after it (cmp.py compares the flag's byte alone).
	echo 'test BookwormGame.exe va:0x40b021 889e57030000 899e57030000 Board::Board clears mLexPraisePending as the port does'
	# The original keeps its settings, players and high scores under HKLM\SOFTWARE\PopCap\BookWorm, which a user who is not
	# an administrator reads but cannot write: it would start as the machine's last player, with the machine's tables, and
	# keep nothing. Each RegOpenKeyEx/RegCreateKeyEx's push of HKEY_LOCAL_MACHINE becomes HKEY_CURRENT_USER, the key orig.sh
	# clears, so it starts as a first run does and keeps what it writes, as the port does in its user folder.
	for a in 0x412f32 0x412fdb 0x4190ef 0x434a86 0x434c16 0x434db3 0x434f21 0x43502f 0x43534d 0x435532 0x4357a7 0x435a2e \
		0x435c23 0x457338 0x483819 0x48384d 0x484330 0x484597 0x4868a1 0x4869e2; do
		echo "test BookwormGame.exe va:$a 6802000080 6801000080 the registry in HKCU, not HKLM"
	done
} > tools/origrun/patches/bookworm.txt
echo "staged in $OG_MASTER"
