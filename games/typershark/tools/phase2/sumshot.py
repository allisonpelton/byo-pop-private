#!/usr/bin/env python3
"""A short summary of a shotdiff run: python sumshot.py <run id> [--all]  (the pairs that differ, with their boxes; --all lists matches too)."""
import json, os, sys
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
run = sys.argv[1]
s = json.load(open(os.path.join(ROOT, "build", "shotreview", "typershark", run, "summary.json")))
c = s["counts"]
print(run, {k: c[k] for k in c if k in ("pairs", "match", "differ", "faint", "error", "items", "visible_boxes", "faint_boxes", "shifted_boxes")})
for p in s["pairs"]:
    if p["status"] == "match" and "--all" not in sys.argv:
        continue
    bx = " ".join(f"[{b['x']},{b['y']} {b['w']}x{b['h']} v{b['visible_pixels']}]" for b in p.get("boxes", [])[:6])
    fb = len(p.get("faint_boxes", []))
    print(f"{p['name']:28s} {p['status']:6s} {p.get('diff_pct', 0):6.2f}% shift={p.get('shift')} {bx}" + (f" faint={fb}" if fb else ""))
for w in s.get("warnings", []):
    print("WARN", w)
def short(names):
    names = names or []
    return names if len(names) <= 10 else "%d shots (the other side's were pruned or never taken)" % len(names)


print("only orig:", short(s.get("orig_only")), "only port:", short(s.get("port_only")))
