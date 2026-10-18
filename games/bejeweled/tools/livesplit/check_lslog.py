#!/usr/bin/env python3
"""Checks lsprobe's reads of the LiveSplit block against what the game wrote
(games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15, test 3).

  check_lslog.py <probe log> <game log> <game stderr>

  <probe log>   lsprobe -log=<file>: "found addr=...", one line per poll ("snap" with the block's fields)
  <game log>    the game's -livesplitlog=<file>: one line per block write, "u=<update>" and the fields, with
                self and the clock masked
  <game stderr> the game's SDL_Log output: "livesplit: block at 0x<address>"

Checks: the probe found the block at the address the game logged; every snapshot it accepted is one the
game wrote, field for field (found by its seq), or the untouched block before the game's first write (seq
0, everything 0); self is the found address once active; the clock (qpc) is set, never runs backwards and
is not later than the probe's own read; the snapshots' seqs grow. Prints the numbers of snapshots, retried
reads, skipped polls and writes the probe never saw. Exit status 1 on any mismatch.
"""

import re
import sys

COMPARED = ("seq", "active", "updates", "flags", "board", "go", "lc", "over", "deaths", "tick", "state", "level", "dlevel",
            "dscore", "score", "blc", "gotick", "dsmax", "rules")


def fields(words):
    return dict(w.split("=", 1) for w in words if "=" in w)


def main(argv):
    if len(argv) != 4:
        sys.stderr.write(__doc__)
        return 2
    probe_path, game_path, stderr_path = argv[1:]
    game = {}
    with open(game_path, encoding="latin-1") as f:
        for line in f:
            words = line.split()
            if words and words[0].startswith("u="):
                g = fields(words[1:])
                game[int(g["seq"])] = g
    logged = None
    with open(stderr_path, encoding="latin-1") as f:
        for line in f:
            m = re.search(r"livesplit: block at 0x([0-9a-fA-F]+)", line)
            if m:
                logged = int(m.group(1), 16)

    problems = []
    found = None
    snaps = retries = skips = 0
    last_seq = -1
    last_qpc = 0
    exit_line = ""
    with open(probe_path, encoding="latin-1") as f:
        for number, line in enumerate(f, 1):
            words = line.split()
            if not words:
                continue
            if words[0] == "found":
                found = int(fields(words[1:])["addr"], 16)
            elif words[0] == "retry":
                retries += 1
            elif words[0] == "skip":
                skips += 1
            elif words[0] == "exit":
                exit_line = line.strip()
            elif words[0] == "snap":
                snaps += 1
                p = fields(words[1:])
                seq = int(p["seq"])
                where = "line %d (seq %d)" % (number, seq)
                if seq <= last_seq:
                    problems.append("%s: seq does not grow (%d before)" % (where, last_seq))
                last_seq = seq
                if seq == 0:
                    zero = all(re.fullmatch(r"[0/,]*|0+", p[k]) for k in COMPARED)
                    if not zero or int(p["self"], 16) != 0:
                        problems.append("%s: the block before the first write is not all zero" % where)
                    continue
                g = game.get(seq)
                if g is None:
                    problems.append("%s: the game logged no write with this seq" % where)
                    continue
                for k in COMPARED:
                    if p[k] != g[k]:
                        problems.append("%s: %s=%s, the game wrote %s" % (where, k, p[k], g[k]))
                self_address = int(p["self"], 16)
                if self_address != found:
                    problems.append("%s: self %x, found at %x" % (where, self_address, found or 0))
                qpc, now, frequency = int(p["qpc"]), int(p["now"]), int(p["qpcf"])
                if frequency <= 0 or qpc <= 0 or qpc < last_qpc or qpc > now:
                    problems.append("%s: clock qpc=%d (before %d), probe now=%d, frequency %d" % (where, qpc, last_qpc, now, frequency))
                last_qpc = qpc
    if found is None:
        problems.append("the probe never found the block")
    elif found != logged:
        problems.append("found at %s, the game logged %s" % (hex(found), hex(logged) if logged is not None else "no address"))
    for p in problems[:20]:
        print("DIFF  " + p)
    if len(problems) > 20:
        print("DIFF  ... %d more" % (len(problems) - 20))
    print("%s: block at %s (the game logged %s); %d snapshots match the game's writes, %d reads retried, %d polls"
          " skipped, of %d writes; %s" % ("FAIL" if problems else "ok", hex(found) if found is not None else "-",
                                         hex(logged) if logged is not None else "-", snaps - len(problems), retries, skips,
                                         len(game), exit_line))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
