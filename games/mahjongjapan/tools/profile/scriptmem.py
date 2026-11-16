"""The port's memory held by kind at moments of the check's scripted play (perfrun.sh's play, docs/MEMORY.md): the port runs the same
-script as perfrun.sh port, and at each of the seconds given a `memreport` goes through the live command channel (-commands); the
MEMREPORT line it logs is read back (the pictures, sounds and buffers the engine holds, the process's private bytes). Run it under rundesk
from the repository's root, with the script perfscript.py made (build/mem/orig/<game>-port-script.txt):

  build/native/rundesk.exe python -I games/mahjongjapan/tools/profile/scriptmem.py <out dir> <mahjongjapan|mahjongchina> <script> <t1,t2,...>
"""
import json
import os
import subprocess
import sys
import time

out, game, script, times = os.path.abspath(sys.argv[1]), sys.argv[2], os.path.abspath(sys.argv[3]), [float(x) for x in sys.argv[4].split(',')]
root = os.getcwd()
sections = {'mahjongjapan': 'Mahjong Escape Ancient Japan', 'mahjongchina': 'Mahjong Escape'}
os.makedirs(os.path.join(out, 'user'), exist_ok=True)
with open(os.path.join(out, 'user', 'registry.ini'), 'w') as f:
    f.write('; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE' + chr(92) + 'JollyBear' + chr(92) + '%s]\nFullScreen=dword:0\n' % sections[game])
commands = os.path.join(out, 'commands.txt')
open(commands, 'w').close()
log = open(os.path.join(out, 'game.log'), 'w')
cmd = [os.path.join(root, 'build', 'native', game + '.exe'), '-gamedir=' + os.path.join(root, 'assets', game), '-userdir=' + os.path.join(out, 'user'),
       '-logos', '-script=' + script, '-windowscale=1', '-commands=' + commands]
started = time.time()
proc = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT)
rows = []
for t in times:
    while time.time() - started < t and proc.poll() is None:
        time.sleep(0.05)
    if proc.poll() is not None:
        break
    with open(commands, 'a', newline='\n') as f:
        f.write('memreport ' + os.path.join(out, 'memreport-%d.txt' % t).replace(chr(92), '/') + '\n')
        f.write('memimages ' + os.path.join(out, 'images-%d.txt' % t).replace(chr(92), '/') + '\n')
    time.sleep(1.5)
    rows.append(t)
proc.terminate()
proc.wait()
log.close()
for line in open(os.path.join(out, 'game.log'), errors='replace'):
    if 'MEMREPORT' in line:
        j = json.loads(line[line.index('MEMREPORT') + 10:])
        print('private %.1f MB  pictures %.1f (+%.1f bits) %d  sounds %.1f %d  songs %d  others %s' % (
            j['process']['private'] / 1048576, j['images']['pixels'] / 1048576, j['images']['copies'] / 1048576, j['images']['count'],
            j['sounds']['bytes'] / 1048576, j['sounds']['count'], j['music']['count'],
            ' '.join('%s %.1f' % (k, v / 1048576) for k, v in j['others'].items() if v > 100000)))
