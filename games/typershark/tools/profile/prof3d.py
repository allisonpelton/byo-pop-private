"""A sampled profile of the port on the visible desktop in 3D, over the performance scenario (GAME_NOTES.md section 11): the same launch as
cyc3d.py's port3d (windowed 640x480, render scale 1, silent audio, window shown without the focus) under cpuprof2.py's sampler.

    python -I games/typershark/tools/profile/prof3d.py <out dir> [seconds]     writes <out dir>/prof.txt (by module, thread, caller, function)

The sampler suspends threads every millisecond, so its shares are the profile of a run that costs more than a plain one; cyc3d.py's
counts are the figures. Run it as the machine allows (gamecount.py), a few times: the main thread's spikes (GAME_NOTES section 11) come and go.
"""
import os, shutil, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import perf
out = os.path.abspath(sys.argv[1]); seconds = sys.argv[2] if len(sys.argv) > 2 else '30'
shutil.rmtree(out, ignore_errors=True); os.makedirs(out)
lines, end = perf.scenario(perf.DEFAULT_SCENARIO)
sc = os.path.join(out, 'script.txt')
with open(sc, 'w', newline='\n') as f:
    for n, c, a in lines:
        f.write('%d %s %s\n' % (n, c, ' '.join(a)))
    f.write('%d quit\n' % (end + 150))
exe, gamedir = perf.port_exe()
ud = os.path.join(out, 'ud'); perf.fresh_users(ud)
cmd = [sys.executable, '-I', os.path.join(HERE, 'cpuprof2.py'), os.path.join(out, 'prof.txt'), seconds, 'PROF_VISIBLE=1', 'PROF_EXE=typershark.exe',
       'ORACLE_VISIBLE=1', 'SDL_AUDIODRIVER=dummy', '--', exe, '-gamedir=' + gamedir, '-userdir=' + ud, '-seed=1', '-script=' + sc, '-renderscale=1']
sys.exit(subprocess.call(cmd))
