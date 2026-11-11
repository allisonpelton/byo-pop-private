#!/usr/bin/env python3
"""Shots of the port with the fixes mod off and on at the same updates, to see what each fix changes (games/typershark/mods/fixes).

    python games/typershark/tools/fixshots.py schools [--seed 1] [--defaults]
                                                                    the board's first 3000 updates every 100: schools of background fish
                                                                    (--defaults: the mod's own defaults, `schools` off, so nothing differs)
    python games/typershark/tools/fixshots.py tutor [--lesson N]    a typing tutor lesson played by the typist, a shot every 60 updates
                                                                    (the line placement of a long exercise, the key pictures)

Each run starts the port under rundesk with -fast from the same seed and the same script, once without the mod and once with
build/ts-mods/fixes (copied from games/typershark/mods/fixes by this script) given as -modsdir. The shots go to build/ts-fix/<what>/{off,on}/
and the pairs that differ are listed with the rectangle of the difference: a fix that only draws differently leaves the rest alone.
The menu steps are the ones of tools/phase1 (a name typed, the WPM question's No, Adventure, Easy), at the same updates.
"""
import argparse
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
EXE = os.path.join(ROOT, "build", "native", "typershark.exe")
RUNDESK = os.path.join(ROOT, "build", "native", "rundesk.exe")
FIXTURE = os.path.join(ROOT, "build", "ts-p1", "fixtures", "port-clean")
DEFAULTS = False                                    # --defaults: the mod's own defaults for the "on" run


def make_mods():
    dst = os.path.join(ROOT, "build", "ts-mods")
    shutil.rmtree(os.path.join(dst, "fixes"), ignore_errors=True)
    shutil.copytree(os.path.join(HERE, "..", "mods", "fixes"), os.path.join(dst, "fixes"))
    return dst


def wait_room():
    import time
    while subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gamecount.py"), "--limit", "3"],
                         stdout=subprocess.DEVNULL).returncode != 0:
        time.sleep(15)


def run(what, with_mod, script_lines, seed, typist, out_dir):
    side = "on" if with_mod else "off"
    shots = os.path.join(out_dir, side)
    shutil.rmtree(shots, ignore_errors=True)
    os.makedirs(shots)
    ud = os.path.join(out_dir, "u_" + side)
    shutil.rmtree(ud, ignore_errors=True)
    shutil.copytree(FIXTURE, ud)
    script = os.path.join(out_dir, side + ".txt")
    with open(script, "w", newline="\n") as f:
        for line in script_lines:
            f.write(line.replace("{shots}", shots.replace("\\", "/")) + "\n")
    args = [RUNDESK, EXE, "-fast", "-seed=%d" % seed, "-userdir=" + ud, "-script=" + script, "-typist=" + os.path.join(out_dir, side + ".typist.txt") + ";" + typist]
    if with_mod:
        args.append("-modsdir=" + make_mods())
        # every toggle on: `schools` starts off (the fixes mod's rule of 2026-11-11), so the "on" run turns the mod's toggles on itself;
        # --defaults leaves the mod's settings as mod.json gives them (what a player who changed nothing plays)
        if not DEFAULTS:
            manifest = json.load(open(os.path.join(HERE, "..", "mods", "fixes", "mod.json")))
            values = {s["id"]: True for s in manifest["settings"] if s["type"] == "bool"}
            json.dump({"v": 1, "mods": {"fixes": values}}, open(os.path.join(ud, "mod-settings.json"), "w"))
    wait_room()
    subprocess.call(args, stdout=open(os.path.join(out_dir, side + ".out"), "w"), stderr=subprocess.STDOUT)
    return shots


def compare(off_dir, on_dir):
    from PIL import Image, ImageChops
    n = 0
    for name in sorted(os.listdir(off_dir)):
        a, b = os.path.join(off_dir, name), os.path.join(on_dir, name)
        if not os.path.exists(b):
            print(name, "missing with the mod")
            continue
        ia, ib = Image.open(a).convert("RGB"), Image.open(b).convert("RGB")
        box = ImageChops.difference(ia, ib).getbbox() if ia.size == ib.size else "size"
        if box:
            n += 1
            print("%s differs %s" % (name, box))
    print("%d of %d shots differ" % (n, len(os.listdir(off_dir))))


def menu(lesson_clicks):
    return ["4001 char T", "4002 char e", "4003 char s", "4004 char t", "4005 char e", "4006 char r", "4021 click 347 228",
            "4041 click 392 341"] + lesson_clicks


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("what", choices=["schools", "tutor"])
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--lesson", type=int, default=15, help="the typing tutor's lesson to start (tutor)")
    ap.add_argument("--updates", type=int, default=6000, help="how many updates of the lesson to shoot (tutor)")
    ap.add_argument("--every", type=int, default=60, help="updates between two shots (tutor)")
    ap.add_argument("--defaults", action="store_true", help="the 'on' run keeps the mod's defaults (no mod-settings.json): schools then shows no difference")
    a = ap.parse_args()
    global DEFAULTS
    DEFAULTS = a.defaults
    out_dir = os.path.join(ROOT, "build", "ts-fix", a.what)
    os.makedirs(out_dir, exist_ok=True)
    if a.what == "schools":
        lines = menu(["4071 click 510 70", "4091 click 320 163"]) + ["%d screenshot {shots}/s%05d.png" % (u, u) for u in range(4400, 7400, 100)]
        lines.append("7500 quit")
        typist = "every=4"
    else:
        # the Typing Tutor button, then the Next button until the lesson wanted (as tools/phase1/sc_tutor.py)
        nxt = a.lesson - 1
        lines = menu(["4071 click 510 290"] + ["%d click 612 395" % (4100 + 10 * i) for i in range(nxt)])
        lines += ["%d screenshot {shots}/t%05d.png" % (u, u) for u in range(4200 + 10 * nxt, 4200 + 10 * nxt + a.updates, a.every)]
        lines.append("%d quit" % (4300 + 10 * nxt + a.updates))
        typist = "every=4;start=%d" % (4130 + 10 * nxt)
    off = run(a.what, False, lines, a.seed, typist, out_dir)
    on = run(a.what, True, lines, a.seed, typist, out_dir)
    compare(off, on)


if __name__ == "__main__":
    main()
