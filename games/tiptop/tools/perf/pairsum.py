"""pairsum.py <out dir>...: summarises pairs.sh's results: per pair the port's and the original's window cycles (G), their threads
(the game's thread, then the others, M), the window's wall seconds and the other game processes at the start and the end."""
import glob
import json
import os
import statistics
import sys


def load(path):
    try:
        with open(path) as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def row(r):
    if not r or not r.get("window"):
        return None
    w = r["window"]
    t = w.get("threads") or {}
    return {"cpu": w.get("cpu_s"), "G": (t.get("total_mcycles") or 0) / 1000.0,
            "threads": [c for c, _ in (t.get("mcycles") or [])], "secs": w.get("seconds"),
            "others": (r.get("others_before"), r.get("others_after"))}


for d in sys.argv[1:]:
    print(d)
    gs = {"port": [], "orig": []}
    for n in range(1, 100):
        rows = {k: row(load(os.path.join(d, "p%d-%s.json" % (n, k)))) for k in gs}
        if not any(rows.values()):
            break
        for k in gs:
            r = rows[k]
            if r:
                gs[k].append(r["G"])
                print("  pair %d %-4s %.2f G  cpu %.2f s  threads(M) %s  others %s" % (n, k, r["G"], r["cpu"] or 0, r["threads"], r["others"]))
    for k in gs:
        if gs[k]:
            print("  %-4s n=%d  min %.2f  median %.2f  mean %.2f  max %.2f G" % (k, len(gs[k]), min(gs[k]), statistics.median(gs[k]), statistics.mean(gs[k]), max(gs[k])))
    if gs["port"] and gs["orig"]:
        print("  port / original: median %.2f, mean %.2f" % (statistics.median(gs["port"]) / statistics.median(gs["orig"]), statistics.mean(gs["port"]) / statistics.mean(gs["orig"])))
