#!/usr/bin/env python3
"""Read Big Money replays (.bmr, games/bigmoney/src/Replay.h).

    python games/bigmoney/tools/bmr.py <file.bmr>...            the start, the end and the event counts
    python games/bigmoney/tools/bmr.py --events <file.bmr>      also every event, one per line

The reader applies the game's limits (16 MB, 4M events or bag draws) and refuses what the game refuses.
"""
import argparse
import json
import struct
import sys

MAX_FILE_SIZE = 16 * 1024 * 1024
MAX_ITEMS = 4000000
OPS = {1: ("CLICK", 2), 2: ("MOVE", 2), 3: ("LEAVE", 0), 4: ("NEXT", 0)}
MODES = ["action", "strategy", "puzzle"]
SKILLS = ["", "easy", "normal", "hard"]
APP_FLAGS = ["showInstructor", "hasClickedGroup", "dangerHintShown", "bagCollectedOnce", "bombSeenOnce"]


class ReplayError(Exception):
    pass


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def take(self, n):
        if self.pos + n > len(self.data):
            raise ReplayError("record cut short")
        out = self.data[self.pos:self.pos + n]
        self.pos += n
        return out

    def u8(self):
        return self.take(1)[0]

    def u16(self):
        return struct.unpack("<H", self.take(2))[0]

    def u32(self):
        return struct.unpack("<I", self.take(4))[0]

    def i32(self):
        return struct.unpack("<i", self.take(4))[0]

    def varint(self):
        value = 0
        for shift in range(0, 35, 7):
            b = self.u8()
            value |= (b & 0x7F) << shift
            if not b & 0x80:
                return value
        raise ReplayError("varint too long")

    def zigzag(self):
        v = self.varint()
        return (v >> 1) ^ -(v & 1)


def read(path):
    data = open(path, "rb").read()
    if len(data) > MAX_FILE_SIZE:
        raise ReplayError("file too large")
    if len(data) < 8 or data[:4] != b"BMRP":
        raise ReplayError("not a Big Money replay")
    fmt, flags = struct.unpack("<HH", data[4:8])
    if fmt != 1:
        raise ReplayError("format %d not supported" % fmt)
    out = {"format": fmt, "flags": flags, "events": [], "bags": []}
    pos = 8
    while pos < len(data):
        if pos + 8 > len(data):
            raise ReplayError("file cut short")
        tag, length = struct.unpack("<4sI", data[pos:pos + 8])
        pos += 8
        if length > len(data) - pos:
            raise ReplayError("file cut short")
        r = Reader(data[pos:pos + length])
        tag = tag.decode("latin-1")
        if tag == "META":
            out["meta"] = data[pos:pos + length].decode("utf-8", "replace")
        elif tag == "GAME":
            out["game"] = {"mode": r.u8(), "skill": r.u8(), "userSeeded": bool(r.u8()), "appFlags": r.u8(),
                           "rngSeed": r.u32(), "gameSeed": r.u32()}
            out["game"]["scoreKey"] = r.take(r.u16()).decode("latin-1")
        elif tag == "EVTS":
            count = r.u32()
            if count > MAX_ITEMS:
                raise ReplayError("too many events")
            tick = 0
            for _ in range(count):
                tick += r.varint()
                op = r.u8()
                if op not in OPS:
                    raise ReplayError("unknown event %d" % op)
                args = [r.zigzag() for _ in range(OPS[op][1])]
                out["events"].append((tick, OPS[op][0], args))
        elif tag == "BAGS":
            count = r.u32()
            if count > MAX_ITEMS:
                raise ReplayError("too many bag draws")
            out["bags"] = [r.u16() for _ in range(count)]
        elif tag == "END ":
            out["end"] = {"tick": r.u32(), "score": r.i32(), "level": r.i32() + 1, "coins": r.i32()}
        pos += length
    if "game" not in out or "end" not in out:
        raise ReplayError("incomplete replay")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("files", nargs="+")
    ap.add_argument("--events", action="store_true", help="print every event")
    args = ap.parse_args()
    status = 0
    for path in args.files:
        try:
            r = read(path)
        except (OSError, ReplayError) as e:
            print("%s: %s" % (path, e))
            status = 1
            continue
        g, e = r["game"], r["end"]
        flags = [n for i, n in enumerate(APP_FLAGS) if g["appFlags"] & (1 << i)]
        counts = {}
        for _, op, _ in r["events"]:
            counts[op] = counts.get(op, 0) + 1
        print("%s: %s %s, %s, rng seed %u, score key %s; app flags %s" % (
            path, MODES[g["mode"]], SKILLS[g["skill"]],
            "seed %u" % g["gameSeed"] if g["userSeeded"] else "unseeded", g["rngSeed"], g["scoreKey"] or "(none)",
            ", ".join(flags) or "none"))
        print("  end: tick %u, score %d, level %d, coins %d; events %s; bag draws %d" % (
            e["tick"], e["score"], e["level"], e["coins"],
            ", ".join("%s %d" % kv for kv in sorted(counts.items())) or "none", len(r["bags"])))
        if "meta" in r:
            try:
                print("  meta:", json.dumps(json.loads(r["meta"])))
            except ValueError:
                print("  meta (not JSON):", r["meta"])
        if args.events:
            for tick, op, a in r["events"]:
                print("  %8u %s %s" % (tick, op, " ".join(str(x) for x in a)))
    return status


if __name__ == "__main__":
    sys.exit(main())
