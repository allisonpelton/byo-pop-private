#!/bin/bash
# sound_cmp.sh <scenario.txt> <name> [limit update]: sound_orig.sh on the scenario, the port on the same input with BYOPOP_SOUNDLOG, and
# soundcmp.py on the two logs up to the update limit (default 4400: the original stops updating at a point that varies from run to run,
# IDENTICAL_PLAY.md, Oddities, so what it plays after is not the game's).
cd "$(dirname "$0")/../../../.."
SC=$1; N=$2; LIM=${3:-4400}; D=build/snd/$N
bash games/atomica/tools/identical/sound_orig.sh "$SC" "$N" 300 | tail -2
python - "$SC" "$D/port.txt" <<'PY'
import sys
out=[]
for l in open(sys.argv[1]):
    w=l.split()
    if not w or l.startswith('#'): continue
    if w[0]=='end': out.append('%d quit'%(int(w[1])+1)); continue
    if w[1]=='shot': continue
    if w[1]=='text':
        for ch in ' '.join(w[2:]): out.append('%d char %s'%(int(w[0])+1,'SPACE' if ch==' ' else ch))
        continue
    out.append('%d %s'%(int(w[0])+1,' '.join(w[1:])))
open(sys.argv[2],'w',newline='\n').write('\n'.join(out)+'\n')
PY
BYOPOP_SOUNDLOG="$PWD/$D/port_sound.log" bash games/atomica/tools/run_script.sh "$PWD/$D/port.txt" "$PWD/$D/user" 300 -seed=1 -drawevery > /dev/null 2>&1
python games/atomica/tools/identical/soundcmp.py "$D/port_sound.log" "$D/orig_sound.log" "$LIM"
