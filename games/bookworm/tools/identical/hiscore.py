# hiscore.py <port's registry.ini> <orig.sh log>: the four high-score tables (Classic, Action, Best, Longest) the port
# left in its emulated registry against those the original left in HKCU (orig.sh's "scores:" lines). Both are
# REG_MULTI_SZ lists of strings: the port's file holds them as hex, reg query shows them with "\0" between the strings.
# Exits 1 if a table differs or is missing from one side.
import re
import sys

TABLES = ("Classic", "Action", "Best", "Longest")
port, orig = {}, {}
section = None
for ln in open(sys.argv[1], encoding="utf-8", errors="replace"):
    ln = ln.strip()
    if ln.startswith("["):
        section = ln
    elif section is not None and section.endswith("\\Scores]") and "=hex:" in ln:
        k, v = ln.split("=hex:", 1)
        port[k] = [s.decode("latin-1") for s in bytes.fromhex(v).split(b"\0") if s]
for ln in open(sys.argv[2], encoding="utf-8", errors="replace"):
    m = re.match(r"scores:\s+(\w+)\s+REG_MULTI_SZ\s+(.*)$", ln.rstrip("\r\n"))
    if m:
        orig[m.group(1)] = [s for s in m.group(2).split("\\0") if s]
bad = 0
for t in TABLES:
    if t not in port or t not in orig:
        print("%-10s missing (port %s, original %s)" % (t, t in port, t in orig))
        bad += 1
    elif port[t] != orig[t]:
        print("%-10s DIFFERS: original %s, port %s" % (t, orig[t][:6], port[t][:6]))
        bad += 1
    else:
        print("%-10s same: %s" % (t, " ".join(port[t][:3])))
sys.exit(1 if bad else 0)
