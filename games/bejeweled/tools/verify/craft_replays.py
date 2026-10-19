#!/usr/bin/env python3
"""Replays and saves broken or stretched on purpose, for the headless verifier's tests
(games/bejeweled/tools/verify/run_tests.sh) and the oracle's stage-4 cases (oracle_cases.sh). Not part of the
original game. Reads and writes .bjr files with games/bejeweled/tools/bjr.py.

  craft_replays.py copy <replay.bjr> <out.bjr> <kind>
      a copy of a real replay that no player may start: cut (half of it), abi (another engine's), rlid (an RLID
      that is not RULE's SHA-256), diffzero (RULE's normal points base and per-level step 0: SetupLevel's
      mLevelPoints, Board::Update's divisor, 0), diffnan (RULE's normal multiplier base NaN); the last two with
      the RLID of their RULE, so only the RULE check refuses them
  craft_replays.py level <save.dat> <out.dat> <level>
      a v1 save with mLevel set (a vortex save at level 10000: SetupLevel's values leave the int range)
  craft_replays.py synthetic <dir>
      small well-formed replays (seed 1, no move, END tick 400) with RULEs and contents to refuse, into <dir>:
      rule-*.bjr (diff.txt values a level cannot play with, a field out of range, a field at its default, an
      unknown field), limits-*.bjr (segments, events at one tick, SAVEs where the game never makes them), and
      fuzz-*.bjr (extreme diff.txt values the check takes: points and multipliers far past the int range);
      prints the names
  craft_replays.py check <in.bjr> <out.bjr> <tick> [<hash>]
      the replay with a CHECK at <tick>, the board's first settle, of hash 0 (the fuzz files' second pass: the
      player then prints the LOGIC hash it computed there) or <hash> (16 hex digits; the third pass, which then
      plays on to END and prints END's values against the file's)
"""

import hashlib
import os
import random
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import bjr  # noqa: E402

# Bejeweled Deluxe 1.87's data\diff.txt (GameRules' DiffValues order): normal points base (int), per level,
# multiplier base, per level; timed the same; time drain base, per level, accel base, per level
ORIGINAL_DIFF = [500, 150.0, 1.0, 0.5, 250, 10.0, 2.0, 0.5, 1.0, 2.0, 0.01, 0.005]
DIFF_SIZE = 4 + 3 * 8 + 4 + 7 * 8


def diff_bytes(values):
    return (struct.pack("<i", values[0]) + struct.pack("<3d", *values[1:4]) + struct.pack("<i", values[4]) +
            struct.pack("<7d", *values[5:12]))


def rule_bytes(fields=b"", diff=None):
    """GameRules::Canonical(): u16 version 1, the fields (u16 id, u8 kind, value), u16 0, the 12 diff.txt values"""
    return struct.pack("<H", 1) + fields + struct.pack("<H", 0) + diff_bytes(ORIGINAL_DIFF if diff is None else diff)


def with_rule(replay, rule):
    replay["rules"] = rule
    replay["rulesId"] = hashlib.sha256(rule).hexdigest()
    return replay


def minimal(mode=0, events=None, end_tick=400, segments=None):
    seg = {"mode": mode, "rng": {"kind": "seed", "seed": 1}, "userSeeded": False, "showInstructions": False, "timesPlayed": 5,
           "pointsAscent": 0, "events": events or []}
    replay = {"format": 1, "flags": 0, "engineAbi": 1, "segments": segments or [seg],
              "end": {"reason": 0, "endTick": end_tick, "score": 0, "level": 0, "gems": 0, "logicHash": 0, "boardDraws": 0}}
    return with_rule(replay, rule_bytes())


def copy(src, out, kind):
    data = bytearray(open(src, "rb").read())
    if kind == "cut":
        data = data[:len(data) // 2]                    # a replay cut short
    elif kind == "abi":
        data[8:12] = struct.pack("<I", 99)              # another engine's ABI
    elif kind == "rlid":
        at = data.index(b"RLID") + 8                    # an RLID that is not the SHA-256 of RULE
        data[at] ^= 0xFF
    elif kind in ("diffzero", "diffnan"):
        replay = bjr.read(bytes(data))
        rule = bytearray(replay["rules"])
        at = len(rule) - DIFF_SIZE
        if kind == "diffzero":
            rule[at:at + 12] = struct.pack("<id", 0, 0.0)
        else:
            rule[at + 12:at + 20] = struct.pack("<d", float("nan"))
        data = bjr.write(with_rule(replay, bytes(rule)))
    else:
        raise SystemExit("craft_replays.py: unknown copy kind " + kind)
    open(out, "wb").write(bytes(data))


def level(src, out, value):
    data = bytearray(open(src, "rb").read())
    data[5:9] = struct.pack("<i", value)                # version (i32), mTimedMode (u8), mLevel (i32)
    open(out, "wb").write(bytes(data))


def synthetic(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    files = {}

    def diff_with(**changes):
        names = ["npb", "nppl", "nmb", "nmpl", "tpb", "tppl", "tmb", "tmpl", "db", "dpl", "dab", "dapl"]
        values = list(ORIGINAL_DIFF)
        for name, value in changes.items():
            values[names.index(name)] = value
        return values

    nan, inf = float("nan"), float("inf")
    # RULEs the player refuses: diff.txt values (a level of no points, not finite) and fields
    for name, diff in (("diff-zero", diff_with(npb=0, nppl=0.0)), ("diff-base0", diff_with(npb=0)),
                       ("diff-nan", diff_with(nmb=nan)), ("diff-inf", diff_with(tmb=inf)),
                       ("diff-negstep", diff_with(tppl=-10.0)), ("diff-tiny", diff_with(nmb=1e-9)),
                       ("diff-drainnan", diff_with(dapl=nan))):
        files["rule-" + name] = with_rule(minimal(), rule_bytes(diff=diff))
    files["rule-minrun9"] = with_rule(minimal(), rule_bytes(struct.pack("<HBi", 48, 1, 9)))         # match.minRun 9
    files["rule-default-field"] = with_rule(minimal(), rule_bytes(struct.pack("<HBi", 48, 1, 3)))   # minRun 3 spelled out
    files["rule-unknown-field"] = with_rule(minimal(), rule_bytes(struct.pack("<HBi", 999, 1, 3)))
    files["rule-hint-nan"] = with_rule(minimal(), rule_bytes(struct.pack("<HBd", 116, 3, nan)))     # scoring.hintPenalty
    # Contents past the bounds on what a file may make the player do
    files["limits-segments"] = minimal(segments=[minimal()["segments"][0]] * (bjr.MAX_SEGMENTS + 1))
    files["limits-one-tick"] = minimal(events=[(5, "DESELECT", [])] * (bjr.MAX_EVENTS_PER_TICK + 1))
    files["limits-save-early"] = minimal(events=[(5, "SAVE", [1, 0]), (6, "HOVER", [1, 1])])
    files["limits-saves"] = minimal(events=[(5, "SAVE", [1, i]) for i in range(5)])
    # Extreme diff.txt values the check takes: SetupLevel's products far past the int range, inf drain rates
    rnd = random.Random(4)
    for i in range(12):
        values = [rnd.choice([1, 500, 2147483647]), rnd.choice([0.0, 150.0, 1e300]), rnd.choice([1.0, 1e10, 1e300]),
                  rnd.choice([0.0, 0.5, 1e300]), rnd.choice([1, 250, 2147483647]), rnd.choice([0.0, 10.0, 1e300]),
                  rnd.choice([1.0, 1e10, 1e300]), rnd.choice([0.0, 0.5, 1e300])]
        values += [rnd.choice([0.0, 1.0, -1e300, 1e300, 5e-324]) for _ in range(4)]
        files["fuzz-%02d" % i] = with_rule(minimal(mode=i % 2), rule_bytes(diff=values))
    for name in sorted(files):
        with open(os.path.join(out_dir, name + ".bjr"), "wb") as f:
            f.write(bjr.write(files[name]))
        print(name)


def add_check(src, out, tick, value=0):
    replay = bjr.read(open(src, "rb").read())
    events = replay["segments"][-1]["events"]
    events.append((tick, "CHECK", [value - (1 << 64) if value >= (1 << 63) else value]))
    events.sort(key=lambda e: e[0])
    with open(out, "wb") as f:
        f.write(bjr.write(replay))


def main(argv):
    if len(argv) >= 5 and argv[1] == "copy":
        copy(argv[2], argv[3], argv[4])
    elif len(argv) == 5 and argv[1] == "level":
        level(argv[2], argv[3], int(argv[4]))
    elif len(argv) == 3 and argv[1] == "synthetic":
        synthetic(argv[2])
    elif len(argv) in (5, 6) and argv[1] == "check":
        add_check(argv[2], argv[3], int(argv[4]), int(argv[5], 16) if len(argv) == 6 else 0)
    else:
        sys.stderr.write(__doc__)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
