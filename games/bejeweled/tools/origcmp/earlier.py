"""For each pair shotdiff found different whose original shot is "or-earlier" (origrun could not prove the frame was
drawn after its update), checks whether the original matches one of the port's shots of the updates before it
(toport.py's <name>~k.png), at shotdiff's tolerance. Prints one line per pair and a summary.

    python games/bejeweled/tools/origcmp/earlier.py <origrun run dir> <port shots dir> <shotdiff summary.json>
"""
import json
import sys

from PIL import Image, ImageChops


def differs(a, b, tol=8):
    d = ImageChops.difference(a, b).convert("RGB")
    return sum(1 for px in d.getdata() if max(px) > tol)


def main():
    run, port, summ = sys.argv[1:4]
    kinds = {}
    for ln in open(run + "/shots/landed.tsv"):
        p = ln.rstrip("\n").split("\t")
        if len(p) >= 3:
            kinds[p[0]] = p[2]
    s = json.load(open(summ))
    ok = bad = 0
    for p in s["pairs"]:
        if p["status"] == "match" or p.get("kind") == "clip":
            continue
        n = p["name"][:-4]
        if kinds.get(n, "drawn") == "drawn":
            continue
        o = Image.open("%s/shots/%s.png" % (run, n)).convert("RGB")
        if o.size != (640, 480):
            o = o.crop((3, 26, 643, 506))
        best = None
        for k in range(1, 4):
            try:
                q = Image.open("%s/%s~%d.png" % (port, n, k)).convert("RGB")
            except OSError:
                continue
            c = differs(o, q)
            if best is None or c < best[1]:
                best = (k, c)
        if best and best[1] == 0:
            ok += 1
            print("%s: matches the port %d update(s) earlier" % (n, best[0]))
        else:
            bad += 1
            print("%s: no earlier port frame matches (best %s)" % (n, best))
    print("earlier.py: %d explained by an older frame, %d not" % (ok, bad))


if __name__ == "__main__":
    main()
