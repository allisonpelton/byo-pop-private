#!/usr/bin/env bash
# The fixes mod (games/bejeweled/mods/fixes; docs/MOD_ROADMAP.md "The fixes mod"; GAME_NOTES.md "Fix candidates") on the native
# build: each fix is played with the mod off and on, under rundesk with -fast (CLAUDE.md), and the two runs must differ the way the
# fix says, in nothing else. Prints "fixes_test: ok" or what failed.
#   games/bejeweled/tools/fixes_test.sh [<exe>]     (default build/native/bejeweled.exe)
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
EXE="${1:-$ROOT/build/native/bejeweled.exe}"
DIR="$ROOT/build/fixestest"
cd "$ROOT" && rm -rf "$DIR"; mkdir -p "$DIR/mods-off" "$DIR/mods-on"
cp -r "$ROOT/games/bejeweled/mods/fixes" "$DIR/mods-on/fixes"
cd "$DIR"
# run <script> <out> <seed> <save or -> <mods dir> [game args]: a fresh user folder, the save in it as savegame.dat
run() {
	rm -rf u && mkdir u && printf '[SOFTWARE\Steam\Bejeweled]\nScreenMode=dword:0\n' > u/registry.ini
	[ "$4" != "-" ] && cp "$4" u/savegame.dat
	timeout 240 "$ROOT/build/native/rundesk.exe" "$(cygpath -w "$EXE")" -gamedir="$(cygpath -w "$ROOT/assets/bejeweled")" \
		-userdir="$(cygpath -w u)" -modsdir="$(cygpath -w "$5")" -seed="$3" -fast -script="$(cygpath -w "$1")" "${@:6}" > "$2" 2>&1
}
fail=""

# a timed game with one move played, saved; then its board turned into one with exactly one move (a swap in the top row)
printf '5 goto timed\n400 click 227 295\n410 click 227 244\n900 save snap.dat\n901 quit\n' > s-snap.txt
run s-snap.txt o-snap.txt 3 - mods-off
python "$ROOT/games/bejeweled/tools/origcmp/nomoves.py" snap.dat one.dat 100 --one > /dev/null || fail="$fail; no save made"

# fixes.selection: continue the saved game, make its one move (the refill leaves no move: Timed mode's fall-out), select a gem at
# the start of the fall-out, then click a gem of the refilled board. The original has kept the selection and the click is lost
cat > s-sel.txt <<'EOS'
5 goto timed
300 click 330 200
310 goto menu
320 click 329 40
340 click 380 40
432 click 380 40
433 state
900 state
901 click 431 244
930 state
1100 quit
EOS
run s-sel.txt o-sel-off.txt 1 one.dat mods-off
run s-sel.txt o-sel-on.txt 1 one.dat mods-on
sel() { grep -E '^STATE' "$1" | sed -n "$2p" | sed -n 's/.*"phase":"\([a-z ]*\)".*/\1/p;'; }
grep -E '^STATE' o-sel-off.txt | sed -n 1p | grep -q '"phase":"fall out".*"selected":\[3,0\]' || fail="$fail; selection: no fall-out with the gem selected"
grep -E '^STATE' o-sel-off.txt | sed -n 3p | grep -q '"selected"' && fail="$fail; selection (off): the click selected a gem"
grep -E '^STATE' o-sel-on.txt  | sed -n 3p | grep -q '"selected":\[4,4\]' || fail="$fail; selection (on): the click did not select the gem"
grep -E '^STATE' o-sel-off.txt | sed -n 2p | grep -q '"selected":\[3,0\]' || fail="$fail; selection (off): no stale selection after the refill"
# nothing else differs: the boards after the refill are the same
[ "$(grep -E '^STATE' o-sel-off.txt | sed -n 2p | sed 's/.*"board"//')" = "$(grep -E '^STATE' o-sel-on.txt | sed -n 2p | sed 's/.*"board"//')" ] || fail="$fail; selection: the refilled boards differ"

# fixes.resumeLook: a timed game at level 2 saved; loaded again its background must be the level's (the run before the save)
printf '5 goto timed\n400 click 227 295\n410 click 227 244\n450 set score 700\n1700 state\n1701 screenshot before.png\n1710 save lvl2.dat\n1720 quit\n' > s-lvl.txt
run s-lvl.txt o-lvl.txt 3 - mods-off
grep -E '^STATE' o-lvl.txt | grep -q '"level":2,' || fail="$fail; resumeLook: no game at level 2 to save"
printf '5 goto timed\n300 click 330 200\n310 goto menu\n340 screenshot after.png\n350 quit\n' > s-load.txt
mkdir -p off on
run s-load.txt o-load-off.txt 3 lvl2.dat mods-off && mv after.png off/after.png
run s-load.txt o-load-on.txt 3 lvl2.dat mods-on && mv after.png on/after.png
python - "$DIR" <<'EOP' || fail="$fail; resumeLook: pictures"
import sys
from PIL import Image
d = sys.argv[1]
def px(p): return Image.open(p).convert("RGB")
def redness(img):
    # the mean of red minus green over the board: the level's tint (level 1 is grey-brown, level 2 red) shows through between
    # the gems; the gems are the same in the pictures
    t = n = 0
    for y in range(20, 440, 2):
        for x in range(206, 612, 2):
            p = img.getpixel((x, y))
            t += p[0] - p[1]
            n += 1
    return t / n
before, off, on = (redness(px(d + "/" + f)) for f in ("before.png", "off/after.png", "on/after.png"))
print("resumeLook: board redness: before the save %.2f, loaded with the fix off %.2f, on %.2f" % (before, off, on))
sys.exit(0 if (abs(off - before) > 1.5 and abs(on - before) < abs(off - before) / 3) else 1)
EOP
[ -z "$fail" ] && echo "fixes_test: ok" || { echo "fixes_test: FAILED$fail (see $DIR)"; exit 1; }
