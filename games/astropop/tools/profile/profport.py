"""The port's cycle profile on the scenario (cpuprof2.py: by module, thread, caller and function), in 3D or 2D on the visible desktop, 22:00 to 08:00.

    python -I profport.py <out-dir> <seconds> [3d|2d]

Runs from the worktree root with the real clock (the port's own frame pacing), SDL's silent audio driver and the scenario's input; the profile
is `<out-dir>/prof.txt`. The sampler suspends every thread a millisecond and so costs the game cycles of its own: the shares are a guide, the
totals are perf.py's.
"""
import os
import shutil
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
out, seconds = os.path.abspath(sys.argv[1]), sys.argv[2]
os.environ['PERF_3D'] = '1' if (sys.argv[3] if len(sys.argv) > 3 else '3d') == '3d' else '0'
os.environ['PERF_CLOCK'] = 'real'
sys.path.insert(0, here)
import perf   # noqa: E402

perf.night_only()
shutil.rmtree(out, ignore_errors=True)
os.makedirs(out)
user = perf.port_user(out)
script = os.path.join(out, 'script.txt')
perf.port_script(script, os.path.join(out, 'mark.png'))
cmd = [sys.executable, '-I', os.path.join(here, 'cpuprof2.py'), os.path.join(out, 'prof.txt'), seconds,
       'SDL_AUDIODRIVER=dummy', 'BYOPOP_TEST_RUN=1', '--', perf.port_exe(), '-userdir=' + user, '-seed=%d' % perf.SEED,
       '-realinput=' + script]
print(subprocess.run(cmd, cwd=os.getcwd()).returncode)
