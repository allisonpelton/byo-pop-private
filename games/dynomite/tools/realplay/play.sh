#!/bin/bash
# play.sh <steps file> <out dir> [game args...]: the port played with real input only (CLAUDE.md, "Done means
# identical"), natively: Bejeweled Twist's driver (games/bejeweledtwist/tools/realplay/drive.py, its steps documented
# there) posts the mouse and keys to the game's own window on rundesk's private desktop, on the real clock, and its
# "play" step sends the input of each MOVE the port's own bot gives (docs/TEST_COMMANDS.md). The game's -script only
# asks "state" and "move" every 100 updates (once a second: the driver takes about half a second a move, and more often it falls behind on stale moves); it gives no input. The user folder is a fresh one with the settings
# identical/port.sh writes (ScreenMode 1, windowed; FULL=1 makes it full screen) and mkhs.py's low tables, so every
# game ends in the name dialog. The driver's log is <out dir>/drive.log, the game's output stdout.txt.
source "$(dirname "$0")/../identical/common.sh"
STEPS=$1; OUT=$2; shift 2
rm -rf "$OUT"; mkdir -p "$OUT/user"
MODE=1; [ -n "$FULL" ] && MODE=0
printf '%s\n' '[SOFTWARE\PopCap\Dynomite]' CustomCursors=dword:0 InProgress=dword:0 MusicVolume=dword:50 Muted=dword:0 \
	PreferredX=dword:-1 PreferredY=dword:-1 ScreenMode=dword:$MODE SfxVolume=dword:85 TimesExecuted=dword:5 \
	TimesPlayed=dword:0 WaitForVSync=dword:0 > "$OUT/user/registry.ini"
python "$TOOLS/mkhs.py" "$OUT/user"
python - "$OUT/ask.txt" <<'PY'
import sys
with open(sys.argv[1], 'w') as f:
    for u in range(300, 400000, 100):
        f.write('%d state\n%d move\n' % (u, u))
PY
"$RUNDESK" python games/bejeweledtwist/tools/realplay/drive.py --size 640x480 "$STEPS" "$OUT" -- \
	"$(win build/native/dynomite.exe)" -gamedir=assets/dynomite -userdir="$(win "$OUT/user")" -script="$(win "$OUT/ask.txt")" "$@"
echo "exit $?"
grep -E "play:|move 1 |no window|error" "$OUT/drive.log" | tail -20
