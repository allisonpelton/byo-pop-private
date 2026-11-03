#!/bin/bash
# port.sh <out dir> <port arguments...>: the port under rundesk (invisible, silent, low priority) with -fast, on a fresh
# user dir inside the out dir with the settings orig.sh gives the original (KEEP_USER=1 keeps the one a run before
# left; HS=1 puts mkhs.py's low tables in it, SAVE=<file> a saved game as SLOT0.SAV). Its output goes to <out dir>/out.txt.
source "$(dirname "$0")/common.sh"
D="$1"; shift
mkdir -p "$D"
if [ -z "$KEEP_USER" ]; then
	rm -rf "$D/user"; mkdir -p "$D/user"
	printf '%s\n' '[SOFTWARE\PopCap\Dynomite]' CustomCursors=dword:0 InProgress=dword:0 MusicVolume=dword:50 Muted=dword:0 \
		PreferredX=dword:-1 PreferredY=dword:-1 ScreenMode=dword:1 SfxVolume=dword:85 TimesExecuted=dword:5 \
		TimesPlayed=dword:0 WaitForVSync=dword:0 > "$D/user/registry.ini"
	[ -n "$HS" ] && python "$TOOLS/mkhs.py" "$D/user"
	[ -n "$SAVE" ] && cp "$SAVE" "$D/user/SLOT0.SAV"
fi
"$RUNDESK" "$(win build/native/dynomite.exe)" -gamedir=assets/dynomite -userdir="$(win "$D/user")" -fast -drawevery "$@" > "$D/out.txt" 2>&1
echo "exit $?"
