#!/usr/bin/env python3
"""Writes tools/autosplit/bigmoney-scenarios.json.gz (gzipped JSON), the fixture tools/autosplittest.js replays through the page's
shared decider (web/autosplit.js, Big Money's row): every scenario of games/bigmoney/tools/livesplit/lsdecide.py on the game and
probe logs games/bigmoney/tools/livesplit/run_tests.sh made, with the events lsdecide.py decides for each.

The fields are named as the shared block's (death0 to death3 for Big Money's four kinds), and only those the decisions read are
kept; runs of polls with the same fields are written once with their count, so the poll numbers the events carry stay the same,
and a poll whose block is inactive keeps its time, which the "ask for the mod" message box counts.

  python tools/autosplit/bigmoney_fixture.py [<runs dir>]     (default: build/logs/check/livesplit-bigmoney)
"""
import gzip
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "games", "bigmoney", "tools", "livesplit"))
import lsdecide as ls  # noqa: E402

# (the shared block's name, lsdecide.py's)
KEEP = (("active", "active"), ("flags", "flags"), ("boardSerial", "boardSerial"), ("goCount", "goCount"), ("goSerial", "goSerial"),
        ("gameOvers", "gameOvers"), ("gameOverSerial", "gameOverSerial"), ("death0", "deathsOverflow"), ("death1", "deathsNoMoves"),
        ("death2", "deathsNewGame"), ("death3", "deathsOther"), ("boardLevelClears", "boardLevelClears"),
        ("displayScoreMax", "displayScoreMax"), ("mode", "mode"), ("skill", "skill"))


def pack(polls):
    out = []
    for t, snap in polls:
        if snap is None:
            out.append(None)
            continue
        f = [snap.get(k) or 0 for _, k in KEEP]
        if not f[0]:
            out.append({"t": t, "f": f})
        elif out and isinstance(out[-1], list) and out[-1][1] == f:
            out[-1][0] += 1
        else:
            out.append([1, f])
    return out


def main():
    runs = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "logs", "check", "livesplit-bigmoney")
    logs, cases = {}, []
    for name, run, category, segments, settings, attach, expected in ls.SCENARIOS:
        run, _, kind = run.partition(":")
        log_name = "probe.log" if kind == "probe" else "block.log"
        polls = ls.read_log(os.path.join(runs, run, log_name))
        if expected is None:
            expected = ls.normalise(ls.decide(ls.read_log(os.path.join(runs, run, "block.log")), category, segments, settings, attach))
        events = ls.decide(polls, category, segments, settings, attach)
        if ls.normalise(events) != expected:
            sys.exit("%s on %s/%s: lsdecide decides [%s], the scenario expects [%s]" % (name, run, log_name, "; ".join(events), expected))
        key = run + "/" + log_name
        if key not in logs:
            logs[key] = pack(polls)
        cases.append({"name": name, "log": key, "category": category, "segments": segments, "settings": settings,
                      "attach": attach, "manual": None, "events": events})
    out = os.path.join(HERE, "bigmoney-scenarios.json.gz")
    text = json.dumps({"fields": [k for k, _ in KEEP], "cases": cases, "logs": logs}, separators=(",", ":")) + "\n"
    with gzip.GzipFile(out, "wb", 9, mtime=0) as f:
        f.write(text.encode("utf-8"))
    print("%d cases on %d logs: %s (%d bytes)" % (len(cases), len(logs), out, os.path.getsize(out)))


if __name__ == "__main__":
    main()
