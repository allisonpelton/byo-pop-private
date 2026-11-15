#!/bin/bash
# port.sh <out dir> <port arguments...>: the port under rundesk (invisible, silent, low priority) with -fast, on a fresh
# user dir inside the out dir with the settings orig.sh gives the original (KEEP_USER=1 keeps the one a run before
# left; HS=1 puts mkhs.py's low tables in it, SAVE=<file> a saved game as SLOT0.SAV; SCREENMODE=0 starts it windowed, as
# origrun runs the original). Its output goes to <out dir>/out.txt. It draws at render scale 1, as the original draws: on a
# desktop scaled 200% the port draws at twice the size, and a shot at 640x480 is that picture box-downsampled, which
# blends the columns of a stretched picture the original samples whole (the tutorial's stomping foot, IDENTICAL_PLAY.md).
source "$(dirname "$0")/common.sh"
D="$1"; shift
mkdir -p "$D"
if [ -z "$KEEP_USER" ]; then
	rm -rf "$D/user"; mkdir -p "$D/user"
	printf '%s\n' '[SOFTWARE\PopCap\Dynomite]' CustomCursors=dword:0 InProgress=dword:0 MusicVolume=dword:50 Muted=dword:0 \
		PreferredX=dword:-1 PreferredY=dword:-1 ScreenMode=dword:${SCREENMODE:-1} SfxVolume=dword:85 TimesExecuted=dword:5 \
		TimesPlayed=dword:0 WaitForVSync=dword:0 > "$D/user/registry.ini"
	[ -n "$HS" ] && python "$TOOLS/mkhs.py" "$D/user"
	[ -n "$SAVE" ] && cp "$SAVE" "$D/user/SLOT0.SAV"
fi
"$RUNDESK" "$(win build/native/dynomite.exe)" -gamedir=assets/dynomite -userdir="$(win "$D/user")" -fast -drawevery -renderscale=1 "$@" > "$D/out.txt" 2>&1
echo "exit $?"
