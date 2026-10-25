#!/usr/bin/env python3
"""The auto splitter's decisions, re-implemented (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15, test 4).

An independent re-implementation of games/bejeweled/tools/livesplit/BejeweledDeluxe-byopop.asl and of the
ASL phase rules it runs under, applied to a game's LiveSplit block as a log shows it:
  - a game log (the game's -livesplitlog: one line per block write, so every update is a poll: exact ticks)
  - a probe log (lsprobe's -log: one line per poll of a real polling loop: snap, same or skip)
It prints the timer events, checks them against the scenario's expected events, and with --asltest also
replays the same polls through the real script compiled by asltest.exe, whose events must be identical.

  lsdecide.py decide <log> [--category C] [--segments "A|B|C"] [--set id=0|1]... [--attach COND]
                     [--manual COND]
      the events of one replay. COND: "field<op>value[&...]" on the block's fields (tick>=200,
      board>0&go>=1), the first poll that meets it
  lsdecide.py polls <log> <out> --layout <file>
      the poll file asltest.exe reads (--layout: the output of "lsprobe -dumplayout")
  lsdecide.py run <runs dir> [--asltest <exe> --asl <script>] --layout <file>
      every scenario below on the game and probe logs of <runs dir>/<run>/ (ls.txt, probe.txt), as
      games/bejeweled/tools/livesplit/run_tests.sh made them; exit status 1 on any mismatch

Events: "start <poll> board=<b>", "split <index> <poll> board=<b>", "reset <cause> <poll> board=<b>"
(cause: the setting that allowed it, resetNoMoves, resetTimeUp, resetNewGame, resetReplaced, resetQuit or
resets; "exit" for the reset the script's exit action makes when the game process ends),
"manualstart <poll> board=<b>", "messagebox <poll>". Polls are numbered from the log's first poll.
"""

import os
import re
import subprocess
import sys

# ---------------------------------------------------------------------------------------------------
# Logs

KEYS = {"seq": "seq", "active": "active", "updates": "updates", "board": "boardSerial", "tick": "tick", "state": "state",
        "level": "level", "dlevel": "displayLevel", "dscore": "displayScore", "score": "score", "blc": "boardLevelClears",
        "gotick": "goTick", "dsmax": "displayScoreMax"}
PAIRS = {"go": ("goCount", "goSerial"), "lc": ("levelClears", "levelClearSerial"), "over": ("gameOvers", "gameOverSerial")}
DEATHS = ("deathsNoMoves", "deathsTimeUp", "deathsNewGame", "deathsOther")


def parse_fields(words):
    """The block's fields from "key=value" words (LiveSplitBlock.h's FormatLiveSplitBlock)"""
    f = {}
    for w in words:
        key, _, value = w.partition("=")
        if key in KEYS:
            f[KEYS[key]] = int(value)
        elif key in PAIRS:
            a, b = value.split("/")
            f[PAIRS[key][0]], f[PAIRS[key][1]] = int(a), int(b)
        elif key == "deaths":
            for name, v in zip(DEATHS, value.split(",")):
                f[name] = int(v)
        elif key == "flags":
            f["flags"] = int(value, 16)
        elif key == "rules":
            f["rulesId"] = value
        elif key == "self":
            f["self"] = None if value == "*" else int(value, 16)
        elif key == "qpc":
            f["qpc"] = None if value == "*" else int(value)
        elif key == "qpcf":
            f["qpcFrequency"] = None if value == "*" else int(value)
    return f


def read_log(path, reads=None):
    """[(t ms, fields or None for a torn read)], kind: "game" or "probe". For a probe log, reads (a list) gets
    each poll's QueryPerformanceCounter at the read, or None"""
    polls = []
    kind = None
    last = None
    with open(path, encoding="latin-1") as f:
        for line in f:
            words = line.split()
            if not words:
                continue
            if words[0].startswith("u="):
                kind = "game"
                polls.append((int(words[0][2:]) * 10, parse_fields(words[1:])))
            elif words[0] in ("snap", "same", "skip"):
                kind = "probe"
                t = int(words[1].split("=")[1])
                now = int(words[2].split("=")[1]) if len(words) > 2 and words[2].startswith("now=") else None
                if words[0] == "snap":
                    last = parse_fields(words[3:])
                    polls.append((t, last))
                elif words[0] == "same" and last is not None:
                    polls.append((t, last))
                elif words[0] == "skip":
                    polls.append((t, None))
                if reads is not None and len(reads) < len(polls):
                    reads.append(now)
    return polls, kind


def lags(polls, reads, events):
    """For a probe log: per start, split or reset event, how long before the poll that saw it the game made it,
    in ms: at least since this snapshot's write, at most since the previous poll's snapshot's write (both
    stamped with the game's QPC, the same clock as the probe's)"""
    out = []
    for e in events:
        w = e.split()
        if w[0] not in ("start", "manualstart", "split", "reset") or w[1] == "exit" or w[0] == "manualstart":
            continue
        i = int(w[2] if w[0] in ("split", "reset") else w[1])
        snap, now = polls[i][1], reads[i]
        before = next((polls[j][1] for j in range(i - 1, -1, -1) if polls[j][1] is not None), None)
        if snap is None or now is None or not snap.get("qpcFrequency") or before is None or not before.get("qpc"):
            continue
        ms = 1000.0 / snap["qpcFrequency"]
        out.append("%s %.1f..%.1f" % (w[0], (now - snap["qpc"]) * ms, (now - before["qpc"]) * ms))
    return out


def condition(text):
    """"tick>=200&board>0" as a test on a poll's fields (the log's key names)"""
    tests = []
    for part in text.split("&"):
        m = re.fullmatch(r"(\w+)(>=|<=|==|!=|>|<)(-?\d+)", part.strip())
        if not m:
            raise ValueError("bad condition %r" % text)
        key = KEYS.get(m.group(1), m.group(1))
        if m.group(1) in PAIRS:
            key = PAIRS[m.group(1)][0]
        tests.append((key, m.group(2), int(m.group(3))))
    ops = {">=": lambda a, b: a >= b, "<=": lambda a, b: a <= b, "==": lambda a, b: a == b, "!=": lambda a, b: a != b,
           ">": lambda a, b: a > b, "<": lambda a, b: a < b}
    return lambda f: f is not None and all(ops[op](f[k], v) for k, op, v in tests)


def skip_torn(polls, attach):
    """The script attaches at a consistent read (asltest's FindBlock needs the block there)"""
    while attach < len(polls) and polls[attach][1] is None:
        attach += 1
    return attach


def first_poll(polls, cond, start=0):
    test = condition(cond)
    for i in range(start, len(polls)):
        if test(polls[i][1]):
            return i
    raise ValueError("no poll meets %s" % cond)


# ---------------------------------------------------------------------------------------------------
# The script, re-implemented

SETTINGS = {"modeGuard": (True, None), "vanillaOnly": (False, None), "resets": (True, None),
            "resetNoMoves": (True, "resets"), "resetTimeUp": (True, "resets"), "resetNewGame": (True, "resets"),
            "resetReplaced": (True, "resets"), "resetQuit": (True, "resets"), "log": (False, None)}
FLAG_TIMED, FLAG_VANILLA, FLAG_REPLAY, FLAG_GAME_OVER = 0x01, 0x04, 0x20, 0x40
FIELDS = ("self", "seq", "active", "updates", "flags", "boardSerial", "goCount", "goSerial", "levelClears", "levelClearSerial",
          "gameOvers", "gameOverSerial") + DEATHS + ("tick", "state", "level", "displayLevel", "displayScore", "score",
                                                    "boardLevelClears", "goTick", "displayScoreMax", "qpc", "qpcFrequency")


def parse_splits(segments):
    """[(is score, target)]: a number of at least 1000 ("10000", "25,000", "25K") is a score; anything else the
    next level (target: how many level clears it needs). A "-" or "{Section}" prefix is dropped."""
    out = []
    levels = 0
    for name in segments:
        name = name.strip()
        if name.startswith("-"):
            name = name[1:].strip()
        if name.startswith("{") and name.find("}") > 0:
            name = name[name.find("}") + 1:].strip()
        digits = name.replace(",", "")
        scale = 1
        if digits.endswith(("K", "k")):
            scale, digits = 1000, digits[:-1]
        if 0 < len(digits) <= 12 and all("0" <= c <= "9" for c in digits) and int(digits) * scale >= 1000:
            out.append((True, int(digits) * scale))
        else:
            levels += 1
            out.append((False, levels))
    return out


def decide(polls, category="", segments=(), settings=None, attach=0, manual=None):
    values = {k: v[0] for k, v in SETTINGS.items()}
    values.update(settings or {})

    def setting(name):
        while name is not None:
            if not values[name]:
                return False
            name = SETTINGS[name][1]
        return True

    events = []
    phase, index = "NotRunning", -1
    splits = []
    run_serial = 0
    pending = None  # a game over's reset held back for a split in the same poll
    attach = skip_torn(polls, attach)
    current = {k: 0 for k in FIELDS}
    baseline, seen_active, warned = True, False, False
    attach_tick = polls[attach][0] if attach < len(polls) else 0

    def board():
        return "board=%d" % current["boardSerial"]

    def reached():
        """The run's current segment is reached in this snapshot of the run's board"""
        if run_serial == 0 or current["boardSerial"] != run_serial or not 0 <= index < len(splits):
            return False
        is_score, target = splits[index]
        return (current["displayScoreMax"] if is_score else current["boardLevelClears"]) >= target

    def start_timer(label, i):
        nonlocal phase, index, splits, run_serial, pending
        if phase != "NotRunning":
            return
        phase, index = "Running", 0
        splits = parse_splits(segments)  # onStart
        run_serial = current["boardSerial"]
        pending = None
        events.append("%s %d %s" % (label, i, board()))

    def reset_timer(text):
        nonlocal phase, index, run_serial, pending
        if phase == "NotRunning":
            return
        phase, index = "NotRunning", -1
        events.append(text)
        run_serial, pending = 0, None  # onReset

    for i in range(attach, len(polls)):
        t, snap = polls[i]
        old = dict(current)
        # update
        updated = False
        if snap is not None:
            if snap["active"] == 0:
                if not seen_active and not warned and t - attach_tick > 10000:
                    warned = True
                    events.append("messagebox %d" % i)
                baseline = True
            else:
                seen_active = True
                for k in FIELDS:
                    current[k] = snap[k] if snap.get(k) is not None else 0
                if baseline:
                    baseline = False
                else:
                    updated = True
        if updated:
            if phase in ("Running", "Paused"):
                cause = None
                if setting("resets") and run_serial != 0:
                    # a new game over or board change, else one held back in an earlier poll
                    cause, pending = pending, None
                    if current["gameOvers"] != old["gameOvers"] and current["gameOverSerial"] == run_serial:
                        cause = ("resetNoMoves" if current["deathsNoMoves"] != old["deathsNoMoves"] else
                                 "resetTimeUp" if current["deathsTimeUp"] != old["deathsTimeUp"] else
                                 "resetNewGame" if current["deathsNewGame"] != old["deathsNewGame"] else "resets")
                    elif (old["boardSerial"] == run_serial and current["boardSerial"] != run_serial and
                          not old["flags"] & FLAG_GAME_OVER):
                        cause = "resetQuit" if current["boardSerial"] == 0 else "resetReplaced"
                    if cause is not None and not setting(cause):
                        cause = None
                    # the segment was reached by the game over (the same poll saw both): split first
                    # (LiveSplit's automatic split is always on here)
                    if cause is not None and reached():
                        cause, pending = None, cause
                if cause is not None:
                    reset_timer("reset %s %d %s" % (cause, i, board()))
                elif reached():
                    index += 1
                    events.append("split %d %d %s" % (index - 1, i, board()))
                    if index >= len(splits):
                        phase = "Ended"
            if phase == "NotRunning":
                category_lower = category.lower()
                timed = bool(current["flags"] & FLAG_TIMED)
                if (current["goCount"] != old["goCount"] and current["goSerial"] == current["boardSerial"] and
                        not current["flags"] & FLAG_REPLAY and
                        not (setting("vanillaOnly") and not current["flags"] & FLAG_VANILLA) and
                        not (setting("modeGuard") and (("timetrial" in category_lower and not timed) or
                                                       ("normal" in category_lower and timed)))):
                    start_timer("start", i)
        if manual is not None and i == manual:
            start_timer("manualstart", i)
    # the game process ended: exit
    if phase in ("Running", "Paused") and run_serial != 0 and setting("resets") and setting("resetQuit"):
        reset_timer("reset exit %d board=%d" % (len(polls), run_serial))
    return events


# ---------------------------------------------------------------------------------------------------
# Poll files for asltest

def read_layout(path):
    layout = {"fields": {}}
    with open(path, encoding="latin-1") as f:
        for line in f:
            words = line.split()
            if not words:
                continue
            if words[0] == "magic":
                layout["magic"] = bytes(int(x, 16) for x in words[1:])
            elif words[0] in ("gameId", "size"):
                layout[words[0]] = int(words[1], 16)
            elif words[0] == "layoutVersion":
                layout[words[0]] = int(words[1])
            elif words[0] == "field":
                layout["fields"][words[1]] = (int(words[2], 16), words[3])
    return layout


def block_bytes(layout, f):
    """The block a poll read, as the game laid it out; self is 1 for "the block's own address" (asltest puts its
    fake address there) when the log masks it"""
    b = bytearray(layout["size"])
    b[0:len(layout["magic"])] = layout["magic"]
    values = dict(f)
    values["gameId"], values["layoutVersion"], values["size"] = layout["gameId"], layout["layoutVersion"], layout["size"]
    if values.get("self") is None:
        values["self"] = 1 if f["active"] else 0
    for name, (offset, kind) in layout["fields"].items():
        if name == "rulesId":
            raw = bytes.fromhex(values.get("rulesId", "00" * 32))
            b[offset:offset + 32] = raw
            continue
        v = values.get(name)
        v = 0 if v is None else v
        size = {"u16": 2, "u32": 4, "i32": 4, "u64": 8}[kind]
        b[offset:offset + size] = (v & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
    return bytes(b)


def write_polls(polls, layout, out):
    with open(out, "w", encoding="latin-1", newline="\n") as f:
        for t, snap in polls:
            if snap is None:
                f.write("torn %d\n" % t)
            else:
                f.write("ok %d %s\n" % (t, block_bytes(layout, snap).hex()))


def asltest(exe, asl, polls_file, category, segments, settings, attach, manual):
    args = [exe, asl, polls_file, "-category=" + category]
    args += ["-segment=" + s for s in segments]
    args += ["-set=%s=%d" % (k, 1 if v else 0) for k, v in sorted((settings or {}).items())]
    if attach:
        args.append("-attach=%d" % attach)
    if manual is not None:
        args.append("-manualstart=%d" % manual)
    p = subprocess.run(args, capture_output=True, text=True)
    events = [line for line in p.stdout.splitlines() if line.strip()]
    return p.returncode, events, p.stderr.strip()


# ---------------------------------------------------------------------------------------------------
# Scenarios (run_tests.sh makes the runs: games with the livesplit mod, their -livesplitlog ls.txt and
# lsprobe's probe.txt)

def levels(n):
    return ["Level %d" % i for i in range(1, n + 1)]


SCORE = "Timetrial - Score Attack"
# expected: the events without poll numbers; "reset quit" is the game's quit, which the script sees as the
# board going away (resetQuit) or, if it never polled that, as the process ending (exit)
SCENARIOS = [
    # Normal: a bot game on a chosen seed, three wormholes, then no moves
    ("normal_5_levels", "bot_normal", "Normal", levels(5), {}, None, None,
     "start board=2; split 0; split 1; split 2; reset resetNoMoves"),
    ("normal_3_levels_ended", "bot_normal", "Normal", levels(3), {}, None, None,
     "start board=2; split 0; split 1; split 2"),
    ("normal_mode_guard", "bot_normal", "Timetrial", levels(5), {}, None, None, ""),
    ("normal_vanilla_only", "bot_normal", "Normal", levels(5), {"vanillaOnly": True}, None, None, ""),
    ("normal_attach_mid_game", "bot_normal", "Normal", levels(5), {}, "board>0&tick>=1000", None, ""),
    ("normal_no_moves_reset_off", "bot_normal", "Normal", levels(5), {"resetNoMoves": False}, None, None,
     "start board=2; split 0; split 1; split 2; reset quit"),
    # Timetrial: a bot game to level 10 (49500 points), time up
    ("timetrial_5_levels", "bot_timed", "Timetrial", levels(5), {}, None, None,
     "start board=3; split 0; split 1; split 2; split 3; split 4"),
    ("timetrial_10_levels", "bot_timed", "Timetrial", levels(10), {}, None, None,
     "start board=3; " + "; ".join("split %d" % i for i in range(9)) + "; reset resetTimeUp"),
    ("score_10k", "bot_timed", SCORE, ["10000"], {}, None, None, "start board=3; split 0"),
    ("score_50k", "bot_timed", SCORE, ["10000", "25,000", "50K"], {}, None, None,
     "start board=3; split 0; split 1; reset resetTimeUp"),
    ("score_timed_guard", "bot_timed", "Normal", levels(5), {}, None, None, ""),
    # Scripted games
    ("level_clear_quit", "level", "Normal", levels(5), {}, None, None, "start board=2; split 0; reset quit"),
    ("level_resets_off", "level", "Normal", levels(5), {"resets": False}, None, None, "start board=2; split 0"),
    ("level_vanilla_only", "level", "Normal", levels(5), {"vanillaOnly": True}, None, None, "start board=2; split 0; reset quit"),
    ("new_game", "newgame", "Normal", levels(5), {}, None, None,
     "start board=2; reset resetNewGame; start board=3; reset resetNewGame; start board=4; reset quit"),
    ("mode_switch_timetrial", "mode_switch", "Timetrial", levels(5), {}, None, None, "start board=3; reset resetReplaced"),
    ("no_moves_play_again", "nomoves", "Normal", levels(5), {}, None, None,
     "start board=2; reset resetNoMoves; start board=3; reset quit"),
    ("no_moves_reset_off", "nomoves", "Normal", levels(5), {"resetNoMoves": False}, None, None,
     "start board=2; reset quit"),
    ("main_normal", "main", "Normal", levels(5), {}, None, None, "start board=2; reset resetReplaced"),
    ("main_timetrial", "main", "Timetrial", levels(5), {}, None, None,
     "start board=3; reset resetTimeUp; start board=4; reset quit"),
    ("timed_fallout", "timed_fallout", "Timetrial", levels(5), {}, None, None, "start board=3; split 0; reset quit"),
    ("resume_no_start", "load", "Normal", levels(5), {}, None, None, ""),
    ("resume_manual_start", "load", "Normal", levels(5), {}, None, "board>0&tick>=100",
     "manualstart board=2; reset quit"),
    ("feature_off", "off", "Normal", levels(5), {}, None, None, "messagebox"),
    # LiveSplit stalls (a ninth item: the polls from the first that meets the first condition up to the first
    # after it that meets the second are lost), so one poll sees a segment reached and the game over: the
    # segment was reached by the game over, so it splits first, and the reset follows in the next poll
    ("score_final_at_time_up", "bot_timed", SCORE, ["10000", "25,000", "49,500"], {}, None, None,
     "start board=3; split 0; split 1; split 2", ("dsmax>=49500", "over>=1")),
    ("score_split_then_time_up", "bot_timed", SCORE, ["10000", "25,000", "49,500", "50K"], {}, None, None,
     "start board=3; split 0; split 1; split 2; reset resetTimeUp", ("dsmax>=49500", "over>=1")),
    ("level_split_then_no_moves", "bot_normal", "Normal", levels(4), {}, None, None,
     "start board=2; split 0; split 1; split 2; reset resetNoMoves", ("blc>=3", "over>=1")),
]


# Runs without the mod: the game writes no -livesplitlog, only the probe's log exists
NO_GAME_LOG = {"off"}
# Runs that play in real time, polled like the script (run_tests.sh "rt"): their probe logs show the lag
REAL_TIME = {"level", "newgame", "mode_switch", "off"}


def normalise(events):
    """Events without poll numbers, "reset resetQuit"/"reset exit" as "reset quit", a split without its board"""
    out = []
    for e in events:
        w = e.split()
        if w[0] == "messagebox":
            out.append("messagebox")
        elif w[0] in ("start", "manualstart"):
            out.append("%s %s" % (w[0], w[2]))
        elif w[0] == "split":
            out.append("split %s" % w[1])
        elif w[0] == "reset":
            out.append("reset quit" if w[1] in ("resetQuit", "exit") else "reset %s" % w[1])
    return out


def stall(polls, reads, cond_from, cond_to):
    """The polls without those from the first that meets cond_from up to the first after it that meets
    cond_to, as if LiveSplit had stalled: that poll then sees both"""
    a = first_poll(polls, cond_from)
    b = first_poll(polls, cond_to, a)
    return polls[:a] + polls[b:], reads[:a] + reads[b:]


def run_all(runs, asltest_exe, asl, layout_path):
    layout = read_layout(layout_path) if layout_path else None
    failures = 0
    for scenario in SCENARIOS:
        name, run, category, segments, settings, attach_cond, manual_cond, expected = scenario[:8]
        stalled = scenario[8] if len(scenario) > 8 else None
        want = [e.strip() for e in expected.split(";") if e.strip()]
        for log_name in ("ls.txt", "probe.txt"):
            if log_name == "ls.txt" and run in NO_GAME_LOG:
                continue
            path = os.path.join(runs, run, log_name)
            label = "%s (%s %s)" % (name, run, "game log" if log_name == "ls.txt" else "probe log")
            if not os.path.exists(path):
                print("FAIL %s: %s missing" % (label, path))
                failures += 1
                continue
            reads = []
            polls, _ = read_log(path, reads)
            if stalled:
                polls, reads = stall(polls, reads, *stalled)
            attach = skip_torn(polls, first_poll(polls, attach_cond) if attach_cond else 0)
            manual = first_poll(polls, manual_cond, attach) if manual_cond else None
            events = decide(polls, category, segments, settings, attach, manual)
            got = normalise(events)
            problems = []
            if got != want:
                problems.append("expected [%s], lsdecide [%s]" % ("; ".join(want), "; ".join(got)))
            if asltest_exe:
                polls_file = os.path.join(runs, run, log_name.replace(".txt", "") + (".%s" % name if stalled else "") +
                                          ".polls.txt")
                if not os.path.exists(polls_file):
                    write_polls(polls, layout, polls_file)
                code, script_events, err = asltest(asltest_exe, asl, polls_file, category, segments, settings, attach, manual)
                if code != 0:
                    problems.append("asltest exit %d: %s" % (code, err))
                elif script_events != events:
                    problems.append("asltest [%s] != lsdecide [%s]" % ("; ".join(script_events), "; ".join(events)))
            status = "FAIL" if problems else "ok  "
            print("%s %s: %s%s" % (status, label, "; ".join(events) or "(no events)",
                                   "" if not asltest_exe or problems else "  [asltest: the same]"))
            for p in problems:
                print("       " + p)
            lag = lags(polls, reads, events) if log_name == "probe.txt" and run in REAL_TIME else []
            if lag:
                print("       lag in ms (the game's event to the poll that saw it): " + ", ".join(lag))
            failures += bool(problems)
    replays = sum(1 if s[1] in NO_GAME_LOG else 2 for s in SCENARIOS)
    print("%d scenario replays (%d scenarios on game and probe logs), %d failed" % (replays, len(SCENARIOS), failures))
    return 1 if failures else 0


def main(argv):
    import argparse
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    d = sub.add_parser("decide")
    d.add_argument("log")
    d.add_argument("--category", default="")
    d.add_argument("--segments", default="")
    d.add_argument("--set", action="append", default=[])
    d.add_argument("--attach")
    d.add_argument("--manual")
    p = sub.add_parser("polls")
    p.add_argument("log")
    p.add_argument("out")
    p.add_argument("--layout", required=True)
    r = sub.add_parser("run")
    r.add_argument("runs")
    r.add_argument("--asltest")
    r.add_argument("--asl")
    r.add_argument("--layout")
    a = ap.parse_args(argv[1:])
    if a.cmd == "decide":
        polls, _ = read_log(a.log)
        settings = {k: v == "1" for k, v in (s.split("=") for s in a.set)}
        attach = first_poll(polls, a.attach) if a.attach else 0
        manual = first_poll(polls, a.manual, attach) if a.manual else None
        for e in decide(polls, a.category, [s for s in a.segments.split("|") if s], settings, attach, manual):
            print(e)
        return 0
    if a.cmd == "polls":
        polls, _ = read_log(a.log)
        write_polls(polls, read_layout(a.layout), a.out)
        return 0
    if a.asltest and not (a.asl and a.layout):
        ap.error("--asltest needs --asl and --layout")
    return run_all(a.runs, a.asltest, a.asl, a.layout)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
