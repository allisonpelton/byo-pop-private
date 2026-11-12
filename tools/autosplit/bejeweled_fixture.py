#!/usr/bin/env python3
"""Writes tools/autosplit/bejeweled-scenarios.json.gz (gzipped JSON), the fixture tools/autosplittest.js replays through the page's
autosplitter (web/autosplit.js): every scenario of games/bejeweled/tools/livesplit/lsdecide.py on the game and probe
logs games/bejeweled/tools/livesplit/run_tests.sh made, with the events lsdecide.py decides for each.

Only the fields the decisions read are kept, and runs of polls with the same fields are written once with their count
(so the poll numbers, which the events carry, stay the same); a poll whose block is inactive keeps its time, which the
"ask for the mod" message box counts. The attach and manual-start conditions are resolved here to poll numbers.

  python tools/autosplit/bejeweled_fixture.py [<runs dir>]     (default: build/livesplit-test of the main checkout)
"""
import gzip
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "games", "bejeweled", "tools", "livesplit"))
import lsdecide as ls  # noqa: E402

KEEP = ("active", "flags", "boardSerial", "goCount", "goSerial", "gameOvers", "gameOverSerial", "deathsNoMoves", "deathsTimeUp",
        "deathsNewGame", "deathsOther", "boardLevelClears", "displayScoreMax")


def pack(polls):
    out = []
    for t, snap in polls:
        if snap is None:
            out.append(None)
            continue
        f = [snap.get(k) or 0 for k in KEEP]
        if not f[0]:
            out.append({"t": t, "f": f})
        elif out and isinstance(out[-1], list) and out[-1][1] == f:
            out[-1][0] += 1
        else:
            out.append([1, f])
    return out


def main():
    runs = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "livesplit-test")
    if not os.path.isdir(os.path.join(runs, "bot_normal")):
        main_checkout = os.path.normpath(os.path.join(ROOT, "..", "..", ".."))
        runs = os.path.join(main_checkout, "build", "livesplit-test")
    logs, cases = {}, []
    for sc in ls.SCENARIOS:
        name, run, category, segments, settings, attach_cond, manual_cond, expected = sc[:8]
        stalled = sc[8] if len(sc) > 8 else None
        for log_name in ("ls.txt", "probe.txt"):
            if log_name == "ls.txt" and run in ls.NO_GAME_LOG:
                continue
            reads = []
            polls, _ = ls.read_log(os.path.join(runs, run, log_name), reads)
            if stalled:
                polls, reads = ls.stall(polls, reads, *stalled)
            attach = ls.skip_torn(polls, ls.first_poll(polls, attach_cond) if attach_cond else 0)
            manual = ls.first_poll(polls, manual_cond, attach) if manual_cond else None
            events = ls.decide(polls, category, segments, settings, attach, manual)
            key = run + "/" + log_name + ("/" + name if stalled else "")
            if key not in logs:
                logs[key] = pack(polls)
            want = [e.strip() for e in expected.split(";") if e.strip()]
            if ls.normalise(events) != want:
                sys.exit("%s on %s: lsdecide decides [%s], the scenario expects [%s]" % (name, key, "; ".join(events), "; ".join(want)))
            cases.append({"name": name, "log": key, "category": category, "segments": segments, "settings": settings,
                          "attach": attach, "manual": manual, "events": events})
    out = os.path.join(HERE, "bejeweled-scenarios.json.gz")
    text = json.dumps({"fields": KEEP, "cases": cases, "logs": logs}, separators=(",", ":")) + "\n"
    with gzip.GzipFile(out, "wb", 9, mtime=0) as f:
        f.write(text.encode("utf-8"))
    print("%d cases on %d logs: %s (%d bytes)" % (len(cases), len(logs), out, os.path.getsize(out)))


if __name__ == "__main__":
    main()
