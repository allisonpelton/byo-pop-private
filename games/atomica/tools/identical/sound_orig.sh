#!/bin/bash
# sound_orig.sh <scenario.txt> <name> [timeout s]: the original on a scenario on the host (rundesk, virtual clock, no pictures) with origrun's
# recording stand-in (--sound wav) and games/atomica/tools/fmodproxy as its fmod.dll (build it first:
#   cd games/atomica/tools/fmodproxy && i686-w64-mingw32-clang -O2 -shared fmodproxy.c fmodproxy.def -o ../../../../build/fmod_proxy_atomica.dll).
# Leaves build/snd/<name>/fmodlog.txt (every FMOD call with the game's update counter) and orig_sound.log (the DirectSound stream).
cd "$(dirname "$0")/../../../.."
# the host registry keeps the game's hints and options between runs (ShowTips off hides the hint boxes the port shows): the key goes first
# (PopCap's own values, which the user allows to be changed)
powershell -NoProfile -Command "reg delete 'HKLM\SOFTWARE\WOW6432Node\PopCap\Atomica' /f" > /dev/null 2>&1
SC=$1; N=$2; T=${3:-200}; R=build/origrun/snd-$N; D=build/snd/$N
mkdir -p "$D"
rm -rf "$R"
build/origrun-bin/origrun.exe --game atomica --run "snd-$N" --copy-only > /dev/null || exit 1
mv "$R/game/fmod.dll" "$R/game/fmod_real.dll"; cp build/fmod_proxy_atomica.dll "$R/game/fmod.dll"
python - "$SC" "$D/orig.txt" <<'PY'
import sys
out=[]
end=0
for l in open(sys.argv[1]):
    l=l.strip()
    if not l or l.startswith('#'): continue
    w=l.split()
    if w[0]=='end': end=int(w[1]); continue
    if w[1]=='shot': continue
    out.append('at %s %s'%(w[0],' '.join(w[1:])))
out.append('at %d move 1 1'%end); out.append('quit')
open(sys.argv[2],'w',newline='\n').write('\n'.join(out)+'\n')
PY
win() { cygpath -w "$1"; }
build/native/rundesk.exe "$(win "$PWD/build/origrun-bin/origrun.exe")" --exe "$(win "$PWD/$R/game/Atomica.exe")" --run "snd-$N" --script "$(win "$PWD/$D/orig.txt")" \
	--clock virtual --cursor scripted --counter 0x4d0048 --update-ms 10 --seed 1 --patches tools/origrun/patches/atomica.txt --sound wav --timeout "$T" > "$D/run.log" 2>&1
cp "$R/logs/sound.log" "$D/orig_sound.log" 2>/dev/null; cp "$R/game/fmodlog.txt" "$D/fmodlog.txt" 2>/dev/null
grep -E "RESULT|SOUND LEAK" "$D/run.log" | tail -4; wc -l "$D/fmodlog.txt" "$D/orig_sound.log"
