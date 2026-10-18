#!/usr/bin/env python3
"""Regression oracle, comparison side (docs/CODE_MODS_DESIGN.md 9.1). tools/oracle/compare.sh runs the
baseline exes and the candidate exe over the oracle cases into build/oracle/<side>/<case>/ and then
calls "compare.py compare build/oracle tools/oracle/expect.txt". The other commands prepare scripts for
compare.sh or maintain expect.txt.

  compare.py strip-saves <script> <out>     copy of a -script file with its "save" lines commented out
                                            (for an exe without the save command; line numbers kept)
  compare.py quit-cuts <script> <outdir>    for each "save snaps/<name>.dat" line, <outdir>/<name>.txt:
                                            the script up to that command, then "quit" at its update;
                                            prints "<name> <update>" per save
  compare.py savestate <savegame.dat>...    decodes the header and state of v1 savegames
  compare.py compare <oracle dir> [<expect file>]
                                            compares every baseline side with candidate/ for every case
                                            in <oracle dir>/cases.txt, and checks the candidate against
                                            the expect file; exit status 1 on any difference
  compare.py expect-update <oracle dir> <expect file>
                                            rewrites the expect file's derived facts from the candidate
                                            runs (keeps its comments and "row" lines) and prints the diff.
                                            Only after checking that the change in behaviour is intended.

Sides (exes.txt): "baseline" (the build before stage 0, no test hooks), "hooks-baseline" (the frozen
stage-0 build) and "candidate". Each baseline side is compared with the candidate.

What "identical" means, per case and baseline side:
  stdout.txt  (TRACE) and stderr.txt (SDL_Log) line for line, after masking the wall-clock numbers only:
              "Seconds = <s>" and "Resource Loading Time: <ms>" (the rest of the "Seconds =" line, the
              update and draw counts, is compared). They are separate files because stderr is
              block-buffered when redirected: in one file its chunks would split stdout lines.
  file tree   the whole run directory, files and directories (an empty directory counts), the same on both
              sides; then every file byte-identical, except:
                shots/*.png   pixel-identical (byte-identical files are not decoded)
                hiscores*.dat equal, or equal once the two day-number fields are masked, and only if each
                              file's two fields agree and the two sides' days are 1 or 2 apart (a run
                              across midnight or a year end): each of the two mode blocks starts with an
                              int32 day (HighScoreWidget::GetDayNumber, tm_year * 366 + tm_yday), at offset
                              0 and at offset 4 + the size of mode 0's six records (each record: u16 name
                              length n, n name bytes, int32 score, int32 checksum = 10 + n bytes)
              Not in the tree comparison: script.txt (input), stdout/stderr/exit (compared above) and, when
              the baseline has no hooks (or for a .plain case), the hook outputs: hash.txt, autoplay.txt and
              the files in snaps/.
  exit.txt    the exit status; besides, every candidate run must exit with 0
  hash.txt    every candidate run but the .plain ones has one (-tracehash), compared with a hooks
              baseline's; lines "<update> <board state> <logic hash> <full hash>"
  mods folder a run with the real mods/ folder (the examples case) must leave it unchanged
              (compare.sh writes modsdir-changed.txt otherwise)
Cross-checks (cases.txt kinds; the case name gives the related case), all on candidate runs:
  plain       <case>.plain: the candidate run without -tracehash (no stage-0 switch at all) must be
              identical to every baseline's <case>, hook outputs aside
  quitcut     <case>.quit.<name>: snaps/<name>.dat, written by the save command, must equal the
              savegame.dat a real shutdown writes when the same script quits at that update, on every side
  nosave      <case>.nosave: the run of a save script with its save lines commented out must be identical
              to the run with them, hash.txt included: save changes nothing
  replay      <case>.replay: a -script run of the input that -autoplay recorded must be identical to the
              autoplay run (exit, logs, hash.txt, user dir): the bot acts only through script input
Expectations (the expect file, checked against the candidate for the script, load, quitcut and autoplay
cases): what each case is about must still happen, even when every side changes together (a change to
assets/ or mods/, a script that no longer reaches its scenario). Facts derived from the outputs:
  exit <status>, mods <the "mods:" log line>, user <user dir files>,
  settle <update> <mode level score board tick>   every TRACE settle header (scripts and loads), in order
  settles <count> / last-settle <...>             the same for autoplay runs
  states <update>:<state> ...                     hash.txt: each update the board enters game over, game
                                                  over done, vortex, level wipe or fall out
  snap <file> <decoded>, save <file> <decoded>|none
                                                  save snapshots, and the savegame*.dat the quit left
  end <the bot's last note>                       autoplay: how the game ended
and hand-written ones, kept by expect-update:
  row <settle update> <row> <8 colours>           that TRACE row line at that settle
"""

import difflib
import os
import re
import struct
import sys
import zlib

STATE_NAMES = ["falling", "playing", "swapping", "swapping back", "clearing", "game over", "game over done",
               "vortex", "level wipe", "fall out"]
EVENT_STATES = {5: "game_over", 6: "game_over_done", 7: "vortex", 8: "level_wipe", 9: "fall_out"}
HOOK_FILES = ("hash.txt", "autoplay.txt")
RUN_FILES = ("script.txt", "stdout.txt", "stderr.txt", "exit.txt", "modsdir-changed.txt")
WALL_CLOCK = [(re.compile(r"Seconds = [0-9.e+-]+"), "Seconds = *"),
              (re.compile(r"Resource Loading Time: [0-9]+"), "Resource Loading Time: *")]
EXPECT_KINDS = ("script", "load", "quitcut", "autoplay")
AUTO_KEYS = ("exit", "mods", "user", "settle", "settles", "last-settle", "states", "snap", "save", "end")


# ---------------------------------------------------------------------------------------------------
# Scripts

def parse_script(path):
    """(line index, update, command, args, raw line) per line; update and command are None for a
    comment or blank line."""
    with open(path, encoding="latin-1") as f:
        lines = f.read().splitlines()
    out = []
    for i, line in enumerate(lines):
        text = line.split("#", 1)[0].strip()
        parts = text.split(None, 2)
        if len(parts) >= 2 and re.fullmatch(r"-?\d+", parts[0]):
            out.append((i, int(parts[0]), parts[1], parts[2] if len(parts) > 2 else "", line))
        else:
            out.append((i, None, None, None, line))
    return out


def strip_saves(src, dst):
    with open(dst, "w", encoding="latin-1", newline="\n") as f:
        for _, update, cmd, _, line in parse_script(src):
            f.write(("# [oracle: no save command in this exe] " + line) if cmd == "save" else line)
            f.write("\n")


def quit_cuts(src, outdir):
    entries = parse_script(src)
    os.makedirs(outdir, exist_ok=True)
    for i, update, cmd, args, _ in entries:
        if cmd != "save":
            continue
        name = os.path.splitext(os.path.basename(args))[0]
        # RunScript runs the commands in update order, file order within an update (stable sort)
        kept = [e for e in entries if e[1] is not None and e[2] != "save" and
                (e[1] < update or (e[1] == update and e[0] < i))]
        with open(os.path.join(outdir, name + ".txt"), "w", encoding="latin-1", newline="\n") as f:
            f.write("# %s cut off at update %d: the quit writes the save that \"save %s\" snapshots\n"
                    % (os.path.basename(src), update, args))
            for e in kept:
                f.write(e[4] + "\n")
            f.write("%d quit\n" % update)
        print(name, update)


def save_state(path):
    """Header and state of a v1 savegame.dat (Board::WriteSaveGame), as a short text."""
    try:
        with open(path, "rb") as f:
            b = f.read()
        version, timed, level, display_level = struct.unpack_from("<iBii", b, 0)
        o = 13
        cells = 0
        for _ in range(64):
            present = b[o]
            o += 1
            if present:
                o += 0x78
                cells += 1
        state, = struct.unpack_from("<i", b, o)
        _, is_new, bar, seconds, idle, score = struct.unpack_from("<BBiiii", b, o + 4)
        name = STATE_NAMES[state] if 0 <= state < len(STATE_NAMES) else "?"
        return ("v%d %s level %d (shown %d), %d gems, state %d (%s), score %d, play seconds %d, %d bytes"
                % (version, "timed" if timed else "normal", level, display_level, cells, state, name, score,
                   seconds, len(b)))
    except (OSError, struct.error, IndexError) as e:
        return "unreadable (%s)" % e


# ---------------------------------------------------------------------------------------------------
# PNG (enough for stb_image_write's 8-bit RGB/RGBA output; anything else falls back to PIL)

def png_pixels(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    pos = 8
    idat = b""
    width = height = depth = ctype = interlace = None
    while pos < len(data):
        length, kind = struct.unpack_from(">I4s", data, pos)
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
    if depth != 8 or ctype not in (2, 6) or interlace != 0:
        from PIL import Image  # rare formats only
        img = Image.open(path).convert("RGBA")
        return img.size[0], img.size[1], img.tobytes()
    bpp = 3 if ctype == 2 else 4
    raw = zlib.decompress(idat)
    stride = width * bpp
    out = bytearray()
    prev = bytearray(stride)
    p = 0
    for _ in range(height):
        ftype = raw[p]
        line = bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0
            b = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if ftype == 1:
                line[x] = (line[x] + a) & 255
            elif ftype == 2:
                line[x] = (line[x] + b) & 255
            elif ftype == 3:
                line[x] = (line[x] + ((a + b) >> 1)) & 255
            elif ftype == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        if bpp == 3:
            for x in range(width):
                out += line[x * 3:x * 3 + 3] + b"\xff"
        else:
            out += line
        prev = line
    return width, height, bytes(out)


def compare_png(a, b):
    """None if pixel-identical, else a description."""
    with open(a, "rb") as f:
        da = f.read()
    with open(b, "rb") as f:
        db = f.read()
    if da == db:
        return None
    wa, ha, pa = png_pixels(a)
    wb, hb, pb = png_pixels(b)
    if (wa, ha) != (wb, hb):
        return "size %dx%d vs %dx%d" % (wa, ha, wb, hb)
    if pa == pb:
        return None
    count = 0
    x0, y0, x1, y1 = wa, ha, -1, -1
    for i in range(0, len(pa), 4):
        if pa[i:i + 4] != pb[i:i + 4]:
            count += 1
            x, y = (i // 4) % wa, (i // 4) // wa
            x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x), max(y1, y)
    return "%d pixels differ in (%d,%d)-(%d,%d)" % (count, x0, y0, x1, y1)


# ---------------------------------------------------------------------------------------------------
# hiscores.dat day fields

def hiscores_day_offsets(data):
    """Offsets of the two int32 day numbers (see the module comment), or None if it does not parse."""
    offsets = []
    pos = 0
    try:
        for _ in range(2):
            offsets.append(pos)
            pos += 4
            for _ in range(6):  # today's record + 5 table records
                n, = struct.unpack_from("<H", data, pos)
                pos += 2 + n + 8
        return offsets if pos == len(data) else None
    except struct.error:
        return None


def compare_hiscores(da, db):
    """None if equal, ('masked', text) if equal once a midnight rollover is masked, else a description."""
    if da == db:
        return None
    oa, ob = hiscores_day_offsets(da), hiscores_day_offsets(db)
    if oa is None or oa != ob or len(da) != len(db):
        return first_byte_diff(da, db)
    days_a = [struct.unpack_from("<i", da, o)[0] for o in oa]
    days_b = [struct.unpack_from("<i", db, o)[0] for o in ob]
    ma, mb = bytearray(da), bytearray(db)
    for o in oa:
        ma[o:o + 4] = mb[o:o + 4] = b"\0\0\0\0"
    if ma != mb:
        return "after masking the day fields: " + first_byte_diff(bytes(ma), bytes(mb))
    if days_a[0] != days_a[1] or days_b[0] != days_b[1]:
        return "day fields disagree within a file: baseline %s, candidate %s" % (days_a, days_b)
    if abs(days_a[0] - days_b[0]) not in (1, 2):
        return "day %d vs %d: not a midnight rollover" % (days_a[0], days_b[0])
    return ("masked", "day %d vs %d" % (days_a[0], days_b[0]))


def first_byte_diff(da, db):
    n = min(len(da), len(db))
    for i in range(n):
        if da[i] != db[i]:
            return "first difference at byte %d (0x%02x vs 0x%02x), sizes %d / %d" % (i, da[i], db[i], len(da), len(db))
    return "sizes %d / %d" % (len(da), len(db))


# ---------------------------------------------------------------------------------------------------
# Files

def read_bytes(path):
    with open(path, "rb") as f:
        return f.read()


def read_lines(path):
    text = read_bytes(path).decode("latin-1").replace("\r\n", "\n")
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    return lines


def mask_wall_clock(line):
    for pattern, replacement in WALL_CLOCK:
        line = pattern.sub(replacement, line)
    return line


def first_list_diff(a, b, what):
    for i in range(min(len(a), len(b))):
        if a[i] != b[i]:
            return "%s line %d differs:\n        baseline:  %s\n        candidate: %s" % (what, i + 1, a[i], b[i])
    if len(a) != len(b):
        extra = a[len(b)] if len(a) > len(b) else b[len(a)]
        return "%s: %d vs %d lines (first extra: %s)" % (what, len(a), len(b), extra)
    return None


def list_tree(root, keep):
    """Sorted relative paths under root, directories with a trailing '/', for which keep(path) is true."""
    out = []
    if os.path.isdir(root):
        for dirpath, dirs, files in os.walk(root):
            rel = os.path.relpath(dirpath, root).replace("\\", "/")
            prefix = "" if rel == "." else rel + "/"
            out += [prefix + d + "/" for d in dirs]
            out += [prefix + f for f in files]
    return sorted(p for p in out if keep(p))


def tree_filter(hook_outputs):
    """The paths compare_case compares as a tree: everything but the run files, and without the hook
    outputs unless both sides have them."""
    def keep(path):
        if path in RUN_FILES:
            return False
        if not hook_outputs and (path in HOOK_FILES or (path.startswith("snaps/") and path != "snaps/")):
            return False
        return True
    return keep


# ---------------------------------------------------------------------------------------------------
# Comparison

class Report:
    def __init__(self):
        self.lines = []
        self.failures = 0
        self.notes = 0

    def out(self, text=""):
        self.lines.append(text)
        print(text)

    def fail(self, text):
        self.failures += 1
        self.out("    DIFF  " + text)

    def note(self, text):
        self.notes += 1
        self.out("    note  " + text)


def compare_exit(r, a, b, label):
    ea, eb = os.path.join(a, "exit.txt"), os.path.join(b, "exit.txt")
    if not (os.path.exists(ea) and os.path.exists(eb)):
        r.fail("%sexit.txt missing: %s / %s" % (label, os.path.exists(ea), os.path.exists(eb)))
        return
    sa, sb = read_bytes(ea).decode("latin-1").strip(), read_bytes(eb).decode("latin-1").strip()
    if sa != sb:
        r.fail("%sexit status %s / %s" % (label, sa, sb))


def compare_streams(r, a, b, label):
    """stdout.txt and stderr.txt; returns the number of stdout lines."""
    count = 0
    for stream in ("stdout.txt", "stderr.txt"):
        pa, pb = os.path.join(a, stream), os.path.join(b, stream)
        if not (os.path.exists(pa) and os.path.exists(pb)):
            r.fail("%s%s missing: %s / %s" % (label, stream, os.path.exists(pa), os.path.exists(pb)))
            continue
        la = [mask_wall_clock(line) for line in read_lines(pa)]
        lb = [mask_wall_clock(line) for line in read_lines(pb)]
        diff = first_list_diff(la, lb, label + stream)
        if diff:
            r.fail(diff)
        if stream == "stdout.txt":
            count = len(lb)
    return count


def compare_tree(r, a, b, label, keep):
    """The file trees under a and b (paths for which keep() is true): the same list, then every file.
    Returns {top-level entry: number of files compared}."""
    fa, fb = list_tree(a, keep), list_tree(b, keep)
    if fa != fb:
        only_a = [p for p in fa if p not in set(fb)]
        only_b = [p for p in fb if p not in set(fa)]
        r.fail("%sfile tree: baseline only %s, candidate only %s" % (label, only_a, only_b))
    counts = {}
    for name in sorted(set(fa) & set(fb)):
        if name.endswith("/"):
            continue
        top = name.split("/", 1)[0] if "/" in name else name
        counts[top] = counts.get(top, 0) + 1
        pa, pb = os.path.join(a, name), os.path.join(b, name)
        if name.lower().endswith(".png"):
            diff = compare_png(pa, pb)
        else:
            da, db = read_bytes(pa), read_bytes(pb)
            if da == db:
                diff = None
            elif os.path.basename(name).lower().startswith("hiscores"):
                diff = compare_hiscores(da, db)
                if isinstance(diff, tuple):
                    r.note("%s%s: equal once the day-number fields are masked (%s: runs on both sides of midnight)"
                           % (label, name, diff[1]))
                    diff = None
            elif name.endswith(".txt"):
                la, lb = da.decode("latin-1").splitlines(), db.decode("latin-1").splitlines()
                diff = (first_list_diff(la, lb, "") or first_byte_diff(da, db)).lstrip()
            else:
                diff = first_byte_diff(da, db)
        if diff:
            r.fail("%s%s: %s" % (label, name, diff))
    return counts


def compare_case(r, side, a, b, hook_outputs):
    """Baseline side run a against candidate run b. hook_outputs: compare hash.txt, autoplay.txt and
    snaps/ too (both exes have the hooks and the candidate ran with them)."""
    if not os.path.isdir(a) or not os.path.isdir(b):
        r.fail("vs %s: run directory missing: baseline %s, candidate %s" % (side, os.path.isdir(a), os.path.isdir(b)))
        return
    label = "vs %s: " % side
    compare_exit(r, a, b, label)
    lines = compare_streams(r, a, b, label)
    counts = compare_tree(r, a, b, label, tree_filter(hook_outputs))
    summary = ["%d stdout lines" % lines, "%d user files" % counts.get("user", 0),
               "%d screenshots" % counts.get("shots", 0)]
    if hook_outputs:
        if counts.get("snaps"):
            summary.append("%d snapshots" % counts["snaps"])
        if os.path.exists(os.path.join(b, "hash.txt")):
            summary.append("%d hashes" % len(read_lines(os.path.join(b, "hash.txt"))))
        if counts.get("autoplay.txt"):
            summary.append("autoplay.txt")
    else:
        summary.append("hook outputs not compared")
    r.out("    " + label + ", ".join(summary))


def same_run(r, a, b, label, with_shots=True):
    """Cross-check: two candidate runs must match (exit, logs, user dir, screenshots, hash.txt)."""
    before = r.failures
    label += ": "
    compare_exit(r, a, b, label)
    compare_streams(r, a, b, label)
    tops = ("user/", "shots/") if with_shots else ("user/",)
    compare_tree(r, a, b, label, lambda p: p == "hash.txt" or p.startswith(tops))
    for p in (a, b):
        if not os.path.exists(os.path.join(p, "hash.txt")):
            r.fail("%shash.txt missing in %s" % (label, p))
    return r.failures == before


def read_kv(path):
    info = {}
    if os.path.exists(path):
        with open(path, encoding="latin-1") as f:
            for line in f:
                key, _, value = line.rstrip("\n").partition("=")
                info[key] = value
    return info


def read_cases(oracle):
    cases = []
    with open(os.path.join(oracle, "cases.txt"), encoding="latin-1") as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) >= 3:
                cases.append((parts[0], parts[1], parts[2], parts[3] if len(parts) > 3 else ""))
    return cases


# ---------------------------------------------------------------------------------------------------
# Expectations

def savegames(run):
    """The savegame*.dat files in a run's user dir (savegame-<RulesId>.dat for a gameplay mod)."""
    return list_tree(os.path.join(run, "user"), lambda p: re.fullmatch(r"savegame[^/]*\.dat", p) is not None)


def derive_facts(run, kind):
    """The facts of a candidate run, as "key value" lines (see the module comment)."""
    facts = []
    exit_path = os.path.join(run, "exit.txt")
    facts.append("exit " + (read_bytes(exit_path).decode("latin-1").strip() if os.path.exists(exit_path) else "missing"))
    stderr = read_lines(os.path.join(run, "stderr.txt")) if os.path.exists(os.path.join(run, "stderr.txt")) else []
    mods = [line.split("mods: ", 1)[1] for line in stderr if line.startswith("INFO: mods: ")]
    facts.append("mods " + (mods[0] if mods else "(no mods line)"))
    user = list_tree(os.path.join(run, "user"), lambda p: True)
    facts.append("user " + (" ".join(user) if user else "(empty)"))

    stdout = read_lines(os.path.join(run, "stdout.txt")) if os.path.exists(os.path.join(run, "stdout.txt")) else []
    settles = []
    for line in stdout:
        m = re.match(r"TRACE update (\d+): (.*)$", line)
        if m:
            settles.append("%s %s" % (m.group(1), m.group(2)))
    if kind == "autoplay":
        facts.append("settles %d" % len(settles))
        facts.append("last-settle " + (settles[-1] if settles else "none"))
    else:
        facts += ["settle " + s for s in settles]

    hash_path = os.path.join(run, "hash.txt")
    if os.path.exists(hash_path):
        events = []
        last = None
        for line in read_lines(hash_path):
            parts = line.split()
            if len(parts) < 2:
                continue
            state = int(parts[1])
            if state != last and state in EVENT_STATES:
                events.append("%s:%s" % (parts[0], EVENT_STATES[state]))
            last = state
        facts.append("states " + (" ".join(events) if events else "none"))
    else:
        facts.append("states (no hash.txt)")

    for snap in list_tree(os.path.join(run, "snaps"), lambda p: not p.endswith("/")):
        facts.append("snap %s %s" % (snap, save_state(os.path.join(run, "snaps", snap))))
    saves = savegames(run)
    facts += ["save %s %s" % (s, save_state(os.path.join(run, "user", s))) for s in saves] or ["save none"]

    if kind == "autoplay":
        notes = []
        if os.path.exists(os.path.join(run, "autoplay.txt")):
            notes = [line for line in read_lines(os.path.join(run, "autoplay.txt")) if line.startswith("# update")]
        facts.append("end " + (notes[-1][2:] if notes else "(no notes)"))
    return facts


def read_expect(path):
    """({case: [(key, value)]}, header comment lines)."""
    expect, header = {}, []
    if path and os.path.exists(path):
        seen_data = False
        with open(path, encoding="latin-1") as f:
            for line in f:
                line = line.rstrip("\r\n")
                if not line.strip() or line.startswith("#"):
                    if not seen_data and line.startswith("#"):
                        header.append(line)
                    continue
                seen_data = True
                parts = line.split(None, 2)
                if len(parts) >= 2:
                    expect.setdefault(parts[0], []).append((parts[1], parts[2] if len(parts) > 2 else ""))
    return expect, header


def check_row(stdout, value):
    """A hand-written "row <settle update> <row> <colours...>" fact."""
    parts = value.split()
    if len(parts) != 10:
        return "malformed row fact: %s" % value
    header = "TRACE update %s:" % parts[0]
    want = "TRACE   row %s: %s" % (parts[1], " ".join(parts[2:]))
    for i, line in enumerate(stdout):
        if line.startswith(header):
            for row in stdout[i + 1:i + 9]:
                if row.startswith("TRACE   row %s:" % parts[1]):
                    return None if row == want else "row fact: at %s got \"%s\", expected \"%s\"" % (header, row, want)
    return "row fact: no settle at update %s" % parts[0]


def check_expect(r, run, kind, entries):
    if not entries:
        r.fail("expect: no facts for this case in the expect file (compare.py expect-update)")
        return
    want = ["%s %s" % (k, v) for k, v in entries if k in AUTO_KEYS]
    got = derive_facts(run, kind)
    before = r.failures
    if want != got:
        for line in difflib.unified_diff(want, got, "expected", "candidate", lineterm="", n=0):
            if not line.startswith(("---", "+++", "@@")):
                r.out("          " + line)
        r.fail("expect: the candidate's facts differ from the expect file (lines above: - expected, + candidate)")
    stdout = read_lines(os.path.join(run, "stdout.txt")) if os.path.exists(os.path.join(run, "stdout.txt")) else []
    rows = [v for k, v in entries if k == "row"]
    for value in rows:
        diff = check_row(stdout, value)
        if diff:
            r.fail("expect: " + diff)
    others = [k for k, _ in entries if k not in AUTO_KEYS and k != "row"]
    if others:
        r.fail("expect: unknown keys %s" % others)
    if r.failures == before:
        r.out("    expect: %d facts%s hold" % (len(want), (" + %d rows" % len(rows)) if rows else ""))


def compare(oracle, expect_path=None):
    info = read_kv(os.path.join(oracle, "exes.txt"))
    sides = info.get("sides", "baseline candidate").split()
    base_sides = [s for s in sides if s != "candidate"]
    hooks = {s: info.get(s + ".hooks") == "yes" for s in sides}
    cand = os.path.join(oracle, "candidate")
    cases = read_cases(oracle)
    expect, _ = read_expect(expect_path)

    r = Report()
    for side in sides:
        r.out("%-15s %s (stage-0 hooks: %s)" % (side + ":", info.get(side + ".exe", "?"), info.get(side + ".hooks", "?")))
        r.out("%-15s sha256 %s, DLLs %s" % ("", info.get(side + ".sha256", "?"), info.get(side + ".dlls", "?")))
    for side in base_sides:
        if info.get(side + ".sha256") and info.get(side + ".sha256") == info.get("candidate.sha256"):
            r.note("%s and candidate are the same binary: comparing them only shows that the runs repeat" % side)
    if not any(hooks[s] for s in base_sides):
        r.note("no baseline has the stage-0 hooks: hash.txt, snaps/ and autoplay.txt are compared with nothing")
    if expect_path and not os.path.exists(expect_path):
        r.fail("expect file %s missing (compare.py expect-update)" % expect_path)

    for name, kind, desc, flags in cases:
        r.out("")
        r.out("[%s] %s: %s" % (kind, name, desc))
        before = r.failures
        run = os.path.join(cand, name)

        if kind == "plain":
            parent = name[:-len(".plain")]
            for side in base_sides:
                compare_case(r, side, os.path.join(oracle, side, parent), run, False)
        elif kind == "nosave":
            parent = name[:-len(".nosave")]
            if same_run(r, os.path.join(cand, parent), run, "cross " + parent):
                r.out("    candidate %s with and without its save lines: exit, logs, hash.txt, user dir and"
                      " screenshots identical: save changes nothing" % parent)
        else:
            for side in base_sides:
                if flags == "hooksonly" and not hooks[side]:
                    r.out("    vs %s: not run (no stage-0 hooks; %s)" % (side, "the replay case covers it" if kind == "autoplay"
                                                                       else "the script uses a stage-0 command"))
                    continue
                compare_case(r, side, os.path.join(oracle, side, name), run, hooks[side])
            if kind == "quitcut":
                parent, _, snap = name.split(".", 2)
                snapshot = os.path.join(cand, parent, "snaps", snap + ".dat")
                saves = [(side, os.path.join(oracle, side, name, "user", "savegame.dat")) for side in sides]
                have = [os.path.exists(snapshot)] + [os.path.exists(p) for _, p in saves]
                if not any(have):
                    r.out("    no game in progress: no snapshot and no shutdown save (as expected together)")
                elif not all(have):
                    r.fail("snapshot / quit save of %s present: %s" % (", ".join(sides), have))
                else:
                    ds = read_bytes(snapshot)
                    for side, p in saves:
                        if read_bytes(p) != ds:
                            r.fail("snapshot %s.dat != %s shutdown save: %s" % (snap, side, first_byte_diff(ds, read_bytes(p))))
                    r.out("    snapshot %s.dat == shutdown save of every exe: %s" % (snap, save_state(snapshot)))
            elif kind == "replay":
                source = name[:-len(".replay")]
                if same_run(r, os.path.join(cand, source), run, "cross " + source, with_shots=False):
                    r.out("    candidate %s and its recorded input replayed as a script: exit, logs, hash.txt and"
                          " user dir identical: the bot acts only through script input" % source)

        for side in sides:
            changed = os.path.join(oracle, side, name, "modsdir-changed.txt")
            if os.path.exists(changed):
                r.fail("%s: the run changed the mods folder: %s" % (side, read_bytes(changed).decode("latin-1")[:300]))
        exit_path = os.path.join(run, "exit.txt")
        if os.path.isdir(run) and (not os.path.exists(exit_path) or read_bytes(exit_path).strip() != b"0"):
            r.fail("the candidate run did not exit with status 0")
        # every candidate run but the plain ones has -tracehash
        if os.path.exists(os.path.join(run, "hash.txt")) != (kind != "plain"):
            r.fail("the candidate run %s hash.txt" % ("has a" if kind == "plain" else "has no"))
        if kind in EXPECT_KINDS and expect_path:
            check_expect(r, run, kind, expect.get(name, []))
        if kind in ("script", "load", "autoplay"):
            for s in list_tree(os.path.join(run, "snaps"), lambda p: not p.endswith("/")):
                r.out("    snapshot %s: %s" % (s, save_state(os.path.join(run, "snaps", s))))
            for s in savegames(run):
                r.out("    %s: %s" % (s, save_state(os.path.join(run, "user", s))))
        if kind == "autoplay" and os.path.exists(os.path.join(run, "autoplay.txt")):
            notes = [line for line in read_lines(os.path.join(run, "autoplay.txt")) if line.startswith("# update")]
            if notes:
                r.out("    bot's last note: %s" % notes[-1][2:])
        r.out("    -> %s" % ("identical" if r.failures == before else "DIFFERENT"))

    r.out("")
    r.out("%d cases, %d differences, %d notes: %s" % (len(cases), r.failures, r.notes,
                                                      "ALL IDENTICAL" if r.failures == 0 else "DIFFERENCES FOUND"))
    with open(os.path.join(oracle, "report.txt"), "w", encoding="latin-1", newline="\n") as f:
        f.write("\n".join(r.lines) + "\n")
    return 0 if r.failures == 0 else 1


DEFAULT_EXPECT_HEADER = [
    "# Oracle expectations (tools/oracle/compare.py, \"Expectations\"): facts each case's candidate run must",
    "# show, whatever the baselines do. \"<case> <key> <value>\" per line. Derived facts are rewritten by",
    "# \"compare.py expect-update build/oracle tools/oracle/expect.txt\" (only after checking that the change",
    "# is intended); \"row <settle update> <row> <colours>\" lines are hand-written and kept. Game state only:",
    "# update numbers, scores, gem colour indices, decoded save headers; no asset data.",
]


def expect_update(oracle, path):
    old, header = read_expect(path)
    lines = list(header or DEFAULT_EXPECT_HEADER)
    for name, kind, _, _ in read_cases(oracle):
        if kind not in EXPECT_KINDS:
            continue
        lines.append("")
        for fact in derive_facts(os.path.join(oracle, "candidate", name), kind):
            lines.append("%s %s" % (name, fact))
        for key, value in old.get(name, []):
            if key == "row":
                lines.append("%s row %s" % (name, value))
    before = []
    if os.path.exists(path):
        before = read_lines(path)
    with open(path, "w", encoding="latin-1", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    changed = [line for line in difflib.unified_diff(before, lines, "before", "after", lineterm="", n=0)]
    for line in changed:
        print(line)
    print("%s: %d lines, %s" % (path, len(lines), "changed" if changed else "unchanged"))
    return 0


def main(argv):
    if len(argv) == 4 and argv[1] == "strip-saves":
        strip_saves(argv[2], argv[3])
        return 0
    if len(argv) == 4 and argv[1] == "quit-cuts":
        quit_cuts(argv[2], argv[3])
        return 0
    if len(argv) >= 3 and argv[1] == "savestate":
        for p in argv[2:]:
            print("%s: %s" % (p, save_state(p)))
        return 0
    if len(argv) in (3, 4) and argv[1] == "compare":
        return compare(argv[2], argv[3] if len(argv) == 4 else None)
    if len(argv) == 4 and argv[1] == "expect-update":
        return expect_update(argv[2], argv[3])
    sys.stderr.write(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
