"""index.py: a table of contents for TipTop's sound logs (BYOPOP_SOUNDLOG, the framework's lines; run.sh records them).

    python games/tiptop/tools/soundlog/index.py <sound.log>... [--gap N] [--min N]

For each log: every sound with how often it played and when it first did, then the bursts: plays that follow each other by
at most --gap updates (30 by default, 0.3 s at TipTop's 10 ms update), as "u<first>-<last>: name +<updates from the first>".
Bursts of at least --min plays (2 by default) are the sequences tools/soundpreset.py makes a preset of:
    python tools/soundpreset.py <sound.log> <first> <last> --files "TipTop.dat|tiptop:" --name <name> --when <text>
A sound that plays on its own (a mouse over, one match) is in the first table only.
"""
import argparse
import re
import sys


def read(path):
    names, plays = {}, []
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"(\d+) load (\d+) (.+?) frames=(\d+) rate=(\d+)", line)
        if m:
            names[int(m.group(2))] = re.sub(r"\.[^./]*$", "", m.group(3).rsplit("/", 1)[-1])
            continue
        m = re.match(r"(\d+) play (\d+) #(\d+)", line)
        if m:
            plays.append((int(m.group(1)), int(m.group(2))))
    return names, plays


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logs", nargs="+")
    ap.add_argument("--gap", type=int, default=30)
    ap.add_argument("--min", type=int, default=2)
    a = ap.parse_args()
    for path in a.logs:
        names, plays = read(path)
        print("## %s\n" % path)
        count, first = {}, {}
        for u, sid in plays:
            count[sid] = count.get(sid, 0) + 1
            first.setdefault(sid, u)
        print("%d plays of %d sounds, last at update %d. Sound: plays, first at update.\n" %
              (len(plays), len(count), plays[-1][0] if plays else 0))
        print(", ".join("%s: %d, u%d" % (names.get(s, str(s)), count[s], first[s]) for s in sorted(count, key=lambda s: first[s])))
        absent = sorted(names[s] for s in names if s not in count)
        print("\nNever played: %s\n" % (", ".join(absent) if absent else "none"))
        bursts, cur = [], []
        for p in plays:
            if cur and p[0] - cur[-1][0] > a.gap:
                bursts.append(cur)
                cur = []
            cur.append(p)
        if cur:
            bursts.append(cur)
        for b in bursts:
            if len(b) >= a.min:
                print("- u%d-%d: %s" % (b[0][0], b[-1][0], ", ".join("%s +%d" % (names.get(s, str(s)), u - b[0][0]) for u, s in b)))
        print()


if __name__ == "__main__":
    main()
