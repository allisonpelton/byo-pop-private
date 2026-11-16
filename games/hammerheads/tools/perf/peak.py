#!/usr/bin/env python3
"""perf.py port <label> <args...> with the port's peak private bytes sampled every 2 s (Get-Process) and written to
build/perf/hammerheads/<label>/peak.txt.   python games/hammerheads/tools/perf/peak.py <label> [perf.py port's options]"""
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
label = sys.argv[1]
process = subprocess.Popen([sys.executable, os.path.join(HERE, 'perf.py'), 'port', label] + sys.argv[2:])
peak = 0
while process.poll() is None:
    out = subprocess.run(['powershell', '-NoProfile', '-Command',
                          "(Get-Process hammerheads -ErrorAction SilentlyContinue | Measure-Object PrivateMemorySize64 -Maximum).Maximum"],
                         capture_output=True, text=True).stdout.strip()
    if out:
        peak = max(peak, int(float(out)))
    time.sleep(2)
folder = os.path.join(HERE, '..', '..', '..', '..', 'build', 'perf', 'hammerheads', label)
open(os.path.join(folder, 'peak.txt'), 'w').write('%.1f\n' % (peak / 1048576))
print(label, 'peak private MB', round(peak / 1048576, 1))
