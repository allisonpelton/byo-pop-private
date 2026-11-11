#!/usr/bin/env python3
"""Record a scenario for phase 2: run the port with its bot (-typist) on top of a setup script and keep what the bot played.

    python rec.py <name> "<typist options>" <last update> [--base U] [--difficulty 0..4] [--abyss] [--tutor] [--next N] [--seed S]

Writes build/ts-p2/rec/<name>.in.txt (the port's script: the menu clicks and the bot's input, `<update> char|click|key ...`, quit last),
<name>.sl (the state log) and <name>.out. tour_game.py takes the .in.txt and the .sl (--input, --sl). The menu flow is the phase-1 one
(scen_menu.py): a new player named Tester, "assess WPM" answered No, then the Adventure button (or the Abyss one, or the typing tutor)
and, for the adventure, the difficulty's button.
"""
import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(HERE, "..", "phase1"))
from tsp import spread_text, port_script  # noqa: E402

ap = argparse.ArgumentParser()
ap.add_argument("name")
ap.add_argument("typist")
ap.add_argument("last", type=int)
ap.add_argument("--base", type=int, default=1000)
ap.add_argument("--difficulty", type=int, default=0)
ap.add_argument("--abyss", action="store_true")
ap.add_argument("--tutor", action="store_true")
ap.add_argument("--next", type=int, default=0)
ap.add_argument("--seed", type=int, default=1)
ap.add_argument("--exe", default=os.path.join(ROOT, "build", "native", "typershark.exe"))
a = ap.parse_args()
B = a.base
steps = spread_text(B, "Tester") + [(B + 20, "click", 347, 228), (B + 40, "click", 392, 341)]
if a.tutor:
    steps += [(B + 70, "click", 510, 290)] + [(B + 100 + 10 * i, "click", 612, 395) for i in range(a.next)]
    start = B + 130 + 10 * a.next
    typist = a.typist + (";" if a.typist else "") + f"start={start}"
elif a.abyss:
    steps += [(B + 70, "click", 510, 165)]
    typist = a.typist
else:
    steps += [(B + 70, "click", 510, 70), (B + 90, "click", 320, 163 + 45 * a.difficulty)]
    typist = a.typist
out = os.path.join(ROOT, "build", "ts-p2", "rec")
os.makedirs(out, exist_ok=True)
setup = os.path.join(out, a.name + ".setup.txt")
open(setup, "w", newline="\n").write(port_script(steps, [f"{a.last + 5} quit"]))
ud = os.path.join(out, "u_" + a.name)
subprocess.call(["rm", "-rf", ud])
subprocess.call(["cp", "-r", os.path.join(ROOT, "build", "ts-p1", "fixtures", "port-clean"), ud])
R = os.popen(f'cd "{ROOT}" && pwd -W').read().strip()
rec = os.path.join(out, a.name + ".typist.txt")
subprocess.call(["bash", os.path.join(HERE, "waitroom.sh")])
subprocess.call([os.path.join(ROOT, "build", "native", "rundesk.exe"), a.exe, "-fast", f"-seed={a.seed}", f"-userdir={ud}",
                 f"-script={setup}", f"-typist={rec};{typist}", f"-statelog={os.path.join(out, a.name + '.sl')}"],
                stdout=open(os.path.join(out, a.name + ".out"), "w"), stderr=subprocess.STDOUT)
lines = [l.rstrip("\n") for l in open(setup)] + [l.rstrip("\n") for l in open(rec) if l.strip()]
q = [l for l in lines if l.split()[1:2] == ["quit"]]
lines = [l for l in lines if l.split()[1:2] != ["quit"]]
lines.sort(key=lambda l: int(l.split()[0]))
lines += q
open(os.path.join(out, a.name + ".in.txt"), "w", newline="\n").write("\n".join(lines) + "\n")
print(f"{a.name}: {len(lines)} lines -> {os.path.join(out, a.name + '.in.txt')}")
