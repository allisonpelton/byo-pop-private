"""Finds the original's draw counter and tries the PrintWindow shot, on the visible desktop (22:00 to 08:00, as visrun3d.py):

    python -I probe3d.py <out-dir> [--pos x,y]

Starts the staged original in 3D (visrun3d.py's copy, registry and window rules), waits for the title, then reads every dword of the app
object between 0x460 and 0x4c0 about 4,000 times (the game is not held) and prints, for each offset, how many distinct values it took, its
smallest and largest value and how often it read 0, beside the update counter (0x4a0). The draw counter is the one that is 0 or small and
falls back to 0 after each draw: with the game at 100 updates a second and a draw most updates, it reads 0 in most samples and 1 in the
rest. Then it takes three shots with visrun3d.shot (PrintWindow) at the title and saves them in <out-dir>.
"""
import os
import sys
import time
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import visrun3d as v   # noqa: E402


def main():
    a = sys.argv[1:]
    pos = None
    if "--pos" in a:
        i = a.index("--pos")
        pos = tuple(int(x) for x in a[i + 1].split(","))
        del a[i:i + 2]
    out = os.path.abspath(a[0])
    import datetime
    if 8 <= datetime.datetime.now().hour < 22:
        raise SystemExit("probe3d: the visible desktop is for 22:00 to 08:00")
    os.makedirs(v.WORK, exist_ok=True)
    os.makedirs(out, exist_ok=True)
    log = open(os.path.join(out, "probe.log"), "w")
    backup = os.path.join(v.WORK, "registry-backup")
    v.prepare()
    v.stage_registry(backup, True)
    run_id = uuid.uuid4().hex[:12]
    cur = v.Cursor(run_id)
    env = dict(os.environ)
    env.update({"ORIGRUN_DIR": os.path.join(v.WORK, "logs"), "ORIGRUN_SOUND": "stub", "ORIGRUN_REGMAP": "popcap",
                "ORIGRUN_CURSOR": "scripted", "ORIGRUN_CURSOR_ID": run_id})
    os.makedirs(env["ORIGRUN_DIR"], exist_ok=True)
    g = None
    try:
        g = v.Game(os.path.join(v.GAME, "AstroPopGame.exe"), env)
        g.pos = pos
        t0 = time.time()
        while not g.window() and time.time() - t0 < 90 and g.p.poll() is None:
            time.sleep(0.05)
        if not g.window():
            print("no window", file=log, flush=True)
            return
        v.post_input(g, cur, "focus", ["regain"])
        while time.time() - t0 < 120 and g.p.poll() is None:
            a0 = g.rd(v.APP)
            if a0 and g.rd(a0 + 0x50c) == 0x10101:
                break
            time.sleep(0.05)
        time.sleep(1.0)
        app = g.rd(v.APP)
        offs = list(range(0x460, 0x4c0, 4))
        stats = {o: {"vals": set(), "min": None, "max": None, "zero": 0} for o in offs}
        n = 0
        t1 = time.time()
        while n < 4000 and time.time() - t1 < 20:
            for o in offs:
                x = g.rd(app + o)
                st = stats[o]
                st["vals"].add(x)
                st["min"] = x if st["min"] is None else min(st["min"], x)
                st["max"] = x if st["max"] is None else max(st["max"], x)
                st["zero"] += (x == 0)
            n += 1
            v.short_sleep(0.0005)
        for o in offs:
            st = stats[o]
            print("+0x%x  distinct %d  min %d  max %d  zero %.2f" % (o, len(st["vals"]), st["min"], st["max"], st["zero"] / max(n, 1)), file=log)
        print("samples %d in %.1f s; counter now %s" % (n, time.time() - t1, g.counter()), file=log, flush=True)
        for k in range(3):
            c = v.shot(g, out, "probe%d" % k, log)
            print("shot %d at update %s" % (k, c), file=log, flush=True)
            time.sleep(0.2)
    finally:
        if g and g.p.poll() is None:
            hw = g.window()
            if hw:
                v.u32.PostMessageW(hw, 0x0010, 0, 0)
            try:
                g.p.wait(20)
            except Exception:
                g.p.kill()
                g.p.wait(10)
        time.sleep(0.5)
        v.restore_registry(backup)
        print("restored the registry key", file=log, flush=True)


if __name__ == "__main__":
    main()
