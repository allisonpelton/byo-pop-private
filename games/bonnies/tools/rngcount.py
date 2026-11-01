# rngcount.py (run from the repository root, with the Ghidra export in re/bonnies): for each function the port names by address, the original's calls to rand (0x430366) and randomRange
# (0x41f500) and the port's calls to crtRand and randomRange; prints the functions where the counts differ
import re, glob, bisect, collections
addrs = set()
port = collections.Counter()
for f in glob.glob("games/bonnies/engine/*.cpp") + glob.glob("games/bonnies/game/*.cpp"):
    cur = None
    for line in open(f, encoding="utf-8", errors="replace"):
        m = re.match(r"^// (0x4[0-9a-f]{5})", line)
        if m:
            cur = int(m.group(1), 16); addrs.add(cur); continue
        # a comment, or a definition (crtRand's and randomRange's own) rather than a call
        if cur is None or line.lstrip().startswith("//") or re.match(r"^[A-Za-z].*\)\s*$", line):
            continue
        port[(cur, "rand")] += len(re.findall(r"\bcrtRand\(", line))
        port[(cur, "range")] += len(re.findall(r"\brandomRange\(", line))
addrs = sorted(addrs)
orig = collections.Counter()
for line in open("re/bonnies/game_disasm.txt"):
    m = re.match(r"\s+([0-9a-f]+):\s+calll\s+0x(430366|41f500)\b", line)
    if not m:
        continue
    site = int(m.group(1), 16)
    i = bisect.bisect_right(addrs, site) - 1
    fn = addrs[i] if i >= 0 else 0
    orig[(fn, "rand" if m.group(2) == "430366" else "range")] += 1
keys = sorted(k for k in set(orig) | set(port) if orig[k] or port[k])
bad = 0
for k in keys:
    if orig[k] != port[k]:
        bad += 1
        print(f"{k[0]:#x} {k[1]}: original {orig[k]}, port {port[k]}")
print(f"functions with draws: {len({k[0] for k in keys})}; original sites {sum(orig.values())}, port sites {sum(port.values())}; differing {bad}")
