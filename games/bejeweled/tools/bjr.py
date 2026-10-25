#!/usr/bin/env python3
"""Reads .bjr replays and sidecars (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.4, 7.3), independently of the
game's reader (games/bejeweled/src/Replay.cpp), for the oracle (games/bejeweled/tools/oracle/compare.py) and
for looking at a replay:

  games/bejeweled/tools/bjr.py <file.bjr>...        a summary of each file
  games/bejeweled/tools/bjr.py -v <file.bjr>...     and every event

read(data) returns a dict, or raises BjrError for anything that is not exactly a well-formed file (the same
rules as Replay::Read: at most MAX_FILE_SIZE bytes, magic, format 1, records inside the file and in their order
(RULE RLID META SESS, then per segment [LDSV] [RNGS] SEGM, then [RNGS] [SVHA] END, each at most once where it
stands), payload sizes, known ops, varints of at most 10 bytes, at most MAX_EVENTS events and at most
MAX_EVENTS_PER_TICK at one tick of a segment, at most MAX_SEGMENTS segments, END reason 0..5, END present and
last). Unknown record tags are skipped wherever they are. META is text decoded from UTF-8 with surrogateescape:
bytes that are not UTF-8 read (the game's reader keeps them too) and write back as they were. write(replay) makes the bytes of such a dict again (write(read(data)) == data for every file the game
writes), for tests that change a replay.
"""

import json
import struct
import sys

OPS = {1: ("SELECT", 2), 2: ("DESELECT", 0), 3: ("TARGET", 2), 4: ("TWIST", 3), 5: ("SLIDE", 5), 6: ("HINT", 0),
       7: ("PAUSE", 1), 8: ("UNPAUSE", 1), 9: ("UNDO", 0), 10: ("NEW_GAME", 0), 11: ("XRAND", 1), 12: ("HOVER", 2),
       13: ("SAVE", 2), 14: ("LOAD", 0), 15: ("CHECK", 1)}
END_REASONS = {0: "abandoned", 1: "no moves", 2: "time up", 3: "new game", 4: "gold time up", 5: "saved"}
MT_WORDS = 624
MAX_FILE_SIZE = 16 * 1024 * 1024
MAX_EVENTS = 4000000
MAX_EVENTS_PER_TICK = 10000
MAX_SEGMENTS = 1000
ENGINE_ABI = 5          # Replay.h ENGINE_ABI, the engine this build plays (verify/run_tests.sh checks they agree)
FIRST_ENGINE_ABI = 1    # Replay.h FIRST_ENGINE_ABI: the oldest engine whose replays of the original rules still play
RNGS_SIZE = 4 + 1 + 1 + MT_WORDS * 4 + 4
END_SIZE = 1 + 4 + 4 + 4 + 4 + 8 + 4


class BjrError(ValueError):
    pass


class Reader:
    def __init__(self, data, pos, end):
        self.data, self.pos, self.end = data, pos, end

    def take(self, n, what):
        if n > self.end - self.pos:
            raise BjrError("truncated in " + what)
        out = self.data[self.pos:self.pos + n]
        self.pos += n
        return out

    def u8(self, what):
        return self.take(1, what)[0]

    def u32(self, what):
        return struct.unpack("<I", self.take(4, what))[0]

    def i32(self, what):
        return struct.unpack("<i", self.take(4, what))[0]

    def u64(self, what):
        return struct.unpack("<Q", self.take(8, what))[0]

    def varint(self, what):
        value = 0
        for i in range(10):
            byte = self.u8(what)
            if i == 9 and byte > 1:
                raise BjrError("varint over 64 bits in " + what)
            value |= (byte & 0x7f) << (7 * i)
            if not byte & 0x80:
                return value
        raise BjrError("varint over 10 bytes in " + what)


def unzigzag(value):
    return (value >> 1) ^ -(value & 1)


def read_rng(r):
    seed = r.u32("RNGS")
    user = r.u8("RNGS")
    ranked = r.u8("RNGS")
    mt = struct.unpack("<%dI" % MT_WORDS, r.take(MT_WORDS * 4, "RNGS"))
    mti = r.i32("RNGS")
    return {"seed": seed, "userSeeded": bool(user), "ranked": bool(ranked), "mt": mt, "mti": mti}


def tag_text(tag):
    """A tag for a message: its 4 bytes, a question mark for each that is not printable ASCII (Replay::TagName)"""
    return "".join(chr(b) if 32 <= b < 127 else "?" for b in tag)


def read_segment(r, budget):
    seg = {"mode": r.u8("SEGM")}
    kind = r.u8("SEGM")
    if kind == 0:
        seg["rng"] = {"kind": "seed", "seed": r.u32("SEGM")}
    elif kind == 1:
        mt = struct.unpack("<%dI" % MT_WORDS, r.take(MT_WORDS * 4, "SEGM"))
        seg["rng"] = {"kind": "state", "mt": mt, "mti": r.i32("SEGM")}
    else:
        raise BjrError("SEGM: unknown RNG start kind %d" % kind)
    seg["userSeeded"] = bool(r.u8("SEGM"))
    seg["showInstructions"] = bool(r.u8("SEGM"))
    seg["timesPlayed"] = r.i32("SEGM")
    seg["pointsAscent"] = r.i32("SEGM")
    count = r.u32("SEGM")
    if count > (r.end - r.pos) // 2:
        raise BjrError("SEGM: %d events cannot fit in %d bytes" % (count, r.end - r.pos))
    if count > budget:
        raise BjrError("SEGM: %d events, more than the %d a replay may have" % (count, MAX_EVENTS))
    events, tick, tick_start = [], 0, 0
    for i in range(count):
        delta = r.varint("event")
        tick += delta
        if tick > 0xffffffff:
            raise BjrError("SEGM: event %d: tick past 2^32" % i)
        if delta:
            tick_start = i
        if i - tick_start >= MAX_EVENTS_PER_TICK:
            raise BjrError("SEGM: event %d: more than %d events at one tick" % (i, MAX_EVENTS_PER_TICK))
        op = r.u8("event")
        if op not in OPS:
            raise BjrError("SEGM: event %d: unknown op %d" % (i, op))
        args = [unzigzag(r.varint("event")) for _ in range(OPS[op][1])]
        events.append((tick, OPS[op][0], args))
    if r.pos != r.end:
        raise BjrError("SEGM: data after the last event")
    seg["events"] = events
    return seg


HEADER_TAGS = (b"RULE", b"RLID", b"META", b"SESS")


def read(data):
    if len(data) > MAX_FILE_SIZE:
        raise BjrError("%d bytes, more than the %d a replay may have" % (len(data), MAX_FILE_SIZE))
    if len(data) < 12 or data[:4] != b"BJRP":
        raise BjrError("not a replay (no BJRP magic)" if len(data) >= 12 else "shorter than the header")
    fmt, flags, abi = struct.unpack("<HHI", data[4:12])
    if fmt != 1:
        raise BjrError("format %d" % fmt)
    out = {"format": fmt, "flags": flags, "engineAbi": abi, "segments": [], "end": None}
    pending = {}
    header_at, past_header, budget = 0, False, MAX_EVENTS
    pos = 12
    while pos < len(data):
        if out["end"] is not None:
            raise BjrError("data after END")
        if len(data) - pos < 8:
            raise BjrError("truncated record header at byte %d" % pos)
        tag = data[pos:pos + 4]
        length = struct.unpack("<I", data[pos + 4:pos + 8])[0]
        start = pos + 8
        pos = start
        if length > len(data) - pos:
            raise BjrError("%s: %d bytes, %d left" % (tag_text(tag), length, len(data) - pos))
        r = Reader(data, pos, pos + length)
        payload = data[pos:pos + length]
        pos += length
        # The record order (Replay::Read): a record out of place or repeated is refused
        misplaced = None
        if tag in HEADER_TAGS:
            index = HEADER_TAGS.index(tag) + 1
            if past_header or index <= header_at:
                misplaced = "RULE, RLID, META and SESS come first, each once, in that order"
            header_at = index
        elif tag in (b"LDSV", b"RNGS", b"SEGM", b"SVHA", b"END "):
            past_header = True
            if tag == b"LDSV" and ("loadSave" in pending or "loadRng" in pending or "saveHash" in out):
                misplaced = "an LDSV comes once before the RNGS and the SEGM it goes with"
            elif tag == b"RNGS" and ("loadRng" in pending or "saveHash" in out):
                misplaced = "an RNGS comes once before a SEGM, or after the last one before SVHA"
            elif tag == b"SEGM" and "saveHash" in out:
                misplaced = "a SEGM after SVHA"
            elif tag == b"SVHA" and ("saveHash" in out or "loadSave" in pending):
                misplaced = "SVHA comes once, after the last SEGM (an LDSV needs a SEGM after it)"
            elif tag == b"END " and "loadSave" in pending:
                misplaced = "an LDSV with no SEGM after it"
        else:
            continue  # a later format addition: skipped wherever it is
        if misplaced:
            raise BjrError("%s at byte %d: out of place (%s)" % (tag_text(tag), start - 8, misplaced))
        if tag == b"RULE":
            out["rules"] = payload
        elif tag == b"RLID":
            if length != 32:
                raise BjrError("RLID: %d bytes, not 32" % length)
            out["rulesId"] = payload.hex()
        elif tag == b"META":
            out["meta"] = payload.decode("utf-8", "surrogateescape")
        elif tag == b"SESS":
            out["session"] = payload
        elif tag == b"LDSV":
            pending["loadSave"] = payload
        elif tag == b"RNGS":
            if length != RNGS_SIZE:
                raise BjrError("RNGS: %d bytes, not %d" % (length, RNGS_SIZE))
            pending["loadRng"] = read_rng(r)
        elif tag == b"SEGM":
            if len(out["segments"]) >= MAX_SEGMENTS:
                raise BjrError("SEGM at byte %d: more than the %d segments a replay may have" % (start - 8, MAX_SEGMENTS))
            seg = read_segment(r, budget)
            budget -= len(seg["events"])
            seg.update(pending)
            pending = {}
            out["segments"].append(seg)
        elif tag == b"SVHA":
            if length != 4:
                raise BjrError("SVHA: %d bytes, not 4" % length)
            out["saveHash"] = r.u32("SVHA")
        elif tag == b"END ":
            if length != END_SIZE:
                raise BjrError("END: %d bytes, not %d" % (length, END_SIZE))
            reason = r.u8("END")
            if reason not in END_REASONS:
                raise BjrError("END: reason %d (0..5)" % reason)
            out["end"] = {"reason": reason, "reasonName": END_REASONS[reason], "endTick": r.u32("END"),
                          "score": r.i32("END"), "level": r.i32("END"), "gems": r.i32("END"), "logicHash": r.u64("END"),
                          "boardDraws": r.u32("END")}
    if out["end"] is None:
        raise BjrError("no END record (truncated?)")
    if "loadRng" in pending:
        out["trailingRng"] = pending["loadRng"]
    return out


def zigzag(value):
    return (value << 1) ^ (value >> 63) if value < 0 else value << 1


def put_varint(out, value):
    while True:
        byte = value & 0x7f
        value >>= 7
        if value:
            out.append(byte | 0x80)
        else:
            out.append(byte)
            return


def rng_payload(rng):
    return (struct.pack("<IBB", rng["seed"], int(rng["userSeeded"]), int(rng["ranked"])) +
            struct.pack("<%dI" % MT_WORDS, *rng["mt"]) + struct.pack("<i", rng["mti"]))


def write(replay):
    """The bytes of a replay as read() returns it (for tests that change one; the game's writer is Replay::Write)"""
    names = {name: op for op, (name, _) in OPS.items()}
    out = bytearray(b"BJRP" + struct.pack("<HHI", replay["format"], replay["flags"], replay["engineAbi"]))

    def record(tag, payload):
        out.extend(tag.encode("latin-1") + struct.pack("<I", len(payload)) + bytes(payload))

    if "rules" in replay:
        record("RULE", replay["rules"])
    if "rulesId" in replay:
        record("RLID", bytes.fromhex(replay["rulesId"]))
    if "meta" in replay:
        record("META", replay["meta"].encode("utf-8", "surrogateescape"))
    if "session" in replay:
        record("SESS", replay["session"])
    for seg in replay["segments"]:
        if "loadSave" in seg:
            record("LDSV", seg["loadSave"])
        if "loadRng" in seg:
            record("RNGS", rng_payload(seg["loadRng"]))
        payload = bytearray(struct.pack("<B", seg["mode"]))
        if seg["rng"]["kind"] == "seed":
            payload += struct.pack("<BI", 0, seg["rng"]["seed"])
        else:
            payload += struct.pack("<B", 1) + struct.pack("<%dI" % MT_WORDS, *seg["rng"]["mt"]) + struct.pack("<i", seg["rng"]["mti"])
        payload += struct.pack("<BBiiI", int(seg["userSeeded"]), int(seg["showInstructions"]), seg["timesPlayed"],
                               seg["pointsAscent"], len(seg["events"]))
        tick = 0
        for event_tick, op, args in seg["events"]:
            put_varint(payload, event_tick - tick)
            tick = event_tick
            payload.append(names[op])
            for a in args:
                put_varint(payload, zigzag(a))
        record("SEGM", payload)
    if "trailingRng" in replay:
        record("RNGS", rng_payload(replay["trailingRng"]))
    if "saveHash" in replay:
        record("SVHA", struct.pack("<I", replay["saveHash"]))
    end = replay["end"]
    record("END ", struct.pack("<BIiiiQI", end["reason"], end["endTick"], end["score"], end["level"], end["gems"], end["logicHash"],
                               end["boardDraws"]))
    return bytes(out)


def save_hash(data):
    """The u32 a sidecar and a SAVE event keep of a save: the first 4 bytes of its SHA-256, little-endian"""
    import hashlib
    return struct.unpack("<I", hashlib.sha256(data).digest()[:4])[0]


def count_ops(replay):
    counts = {}
    for seg in replay["segments"]:
        for _, op, _ in seg["events"]:
            counts[op] = counts.get(op, 0) + 1
    return counts


def moves(replay):
    return sum(1 for seg in replay["segments"] for _, op, args in seg["events"] if op == "TARGET" and args[0] >= 0)


def rng_text(rng):
    """A SEGM's RNG start ("kind") or an RNGS record (seed 0: the global stream)"""
    if rng.get("kind") == "state":
        return "global stream state"
    if rng.get("kind") == "seed":
        return "seed %d" % rng["seed"]
    if rng["seed"] == 0:
        return "global stream"
    return "seed %d%s" % (rng["seed"], " user" if rng["userSeeded"] else "")


def summary(replay):
    """One deterministic line (no dates): what the oracle's expect file keeps of a replay or sidecar"""
    end = replay["end"]
    parts = ["end %s tick %d score %d level %d gems %d draws %d" % (end["reasonName"], end["endTick"], end["score"], end["level"],
                                                                   end["gems"], end["boardDraws"])]
    for i, seg in enumerate(replay["segments"]):
        counts = {}
        for _, op, _ in seg["events"]:
            counts[op] = counts.get(op, 0) + 1
        text = "seg%d %s %s%s" % (i, "timed" if seg["mode"] else "normal", rng_text(seg["rng"]), " user" if seg["userSeeded"] else "")
        if "loadSave" in seg:
            text += " +save"
        if "loadRng" in seg:
            text += " load(%s)" % rng_text(seg["loadRng"])
        text += " " + ",".join("%s%d" % (op, counts[op]) for op in sorted(counts))
        parts.append(text)
    if "trailingRng" in replay:
        parts.append("rng " + rng_text(replay["trailingRng"]))
    flags = [name for bit, name in ((1, "hover"), (2, "ranked"), (4, "multi")) if replay["flags"] & bit]
    if flags:
        parts.append("flags " + "+".join(flags))
    return "; ".join(parts)


def main(argv):
    # Whatever a file holds (a META that is not UTF-8) prints, also to a redirected stdout in a narrow code page
    try:
        sys.stdout.reconfigure(errors="backslashreplace")
    except AttributeError:
        pass
    verbose = "-v" in argv
    files = [a for a in argv[1:] if a != "-v"]
    if not files:
        sys.stderr.write(__doc__)
        return 2
    status = 0
    for path in files:
        with open(path, "rb") as f:
            data = f.read()
        try:
            replay = read(data)
        except BjrError as e:
            print("%s: not a well-formed replay: %s" % (path, e))
            status = 1
            continue
        print("%s: %d bytes, abi %d, %s" % (path, len(data), replay["engineAbi"], summary(replay)))
        if "meta" in replay:
            print("  meta " + replay["meta"])
        if "rulesId" in replay:
            print("  rulesId " + replay["rulesId"])
        if "saveHash" in replay:
            print("  saveHash %08x" % replay["saveHash"])
        if verbose:
            for i, seg in enumerate(replay["segments"]):
                print("  segment %d: %s" % (i, {k: v for k, v in seg.items() if k not in ("events", "rng", "loadRng", "loadSave")}))
                for tick, op, args in seg["events"]:
                    print("    %7d %s %s" % (tick, op, " ".join("%x" % (a & 0xffffffffffffffff) if op == "CHECK" else str(a) for a in args)))
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv))
