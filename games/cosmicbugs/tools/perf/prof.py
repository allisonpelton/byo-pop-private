#!/usr/bin/env python
"""prof.py: where the Cosmic Bugs port spends its processor time (README.md, "Performance"), by cpuprof.py's cycle-weighted EIP samples.

  build/native/rundesk.exe python games/cosmicbugs/tools/perf/prof.py title|board OUTDIR [--exe build/native/cosmicbugs.exe] [--seconds N]

Real time, windowed, a fresh user directory. `title` idles on the title for the whole run; `board` goes to the first board with the
test channel's `goto game` after 3 s (the logo's two seconds are over) and plays no input, as perf.py's board window does. A `quit`
ends the run, and cpuprof.py's profile (modules, threads, functions) lands in OUTDIR/prof.txt.
"""
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()).resolve()
CONFIG = '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n'


def main():
    args = sys.argv[1:]
    kind, out = args[0], Path(args[1]).resolve()
    exe = ROOT / "build/native/cosmicbugs.exe"
    seconds = 25.0
    i = 2
    while i < len(args):
        if args[i] == "--exe":
            exe = Path(args[i + 1]).resolve()
        elif args[i] == "--seconds":
            seconds = float(args[i + 1])
        i += 2
    (out / "user").mkdir(parents=True, exist_ok=True)
    (out / "user" / "cp.cfg").write_text(CONFIG)
    commands = out / "commands.txt"
    commands.write_text("")
    (out / "realinput.txt").write_text("")
    cmd = [sys.executable, str(HERE / "cpuprof.py"), str(out / "prof.txt"), str(seconds + 3), "--", str(exe),
           "-gamedir=" + str(ROOT / "assets/cosmicbugs"), "-userdir=" + str(out / "user"), "-commands=" + str(commands),
           "-realinput=" + str(out / "realinput.txt")]
    p = subprocess.Popen(cmd, cwd=str(out))
    t0 = time.time()
    if kind == "board":
        time.sleep(3.0)
        with open(commands, "a", newline="\n") as f:
            f.write("goto game\n")
    while time.time() - t0 < seconds and p.poll() is None:
        time.sleep(0.2)
    if p.poll() is None:
        with open(commands, "a", newline="\n") as f:
            f.write("quit\n")
    p.wait(timeout=180)
    print(Path(out / "prof.txt").read_text()[:6000])


main()
