#!/usr/bin/env python3
"""The BookWorm auto splitter's decisions, re-implemented and checked against the real script.

An independent re-implementation of games/bookworm/tools/livesplit/BookWormDeluxe-byopop.asl and of the ASL
phase rules it runs under (per poll: update; while running, reset or else split; when not running, start; at
the end the exit action), applied to a game's LiveSplit block as a log shows it:
  - a game log (the game's -livesplitlog: one line per block write, so every update is a poll)
  - a probe log (lsprobe.py: one line per poll of a real polling loop against the running game)
It prints the timer events and, with --asltest, replays the same polls through the real script compiled by
asltest.exe (asltest/asltest.cs), whose events must be identical.

  lsdecide.py decide <log> [--category C] [--segments "A|B|C"] [--set id=0|1]... [--attach N]
                     [--asltest <exe> --asl <script>]
  lsdecide.py run <runs dir> --asltest <exe> --asl <script>
      every scenario below on the logs of <runs dir>/<run>/block.log (run_tests.sh makes them); exit status 1
      on any difference from the expected events or from the script

Events: "start <poll> board=<b>", "split <index> <poll> board=<b>", "reset <cause> <poll> board=<b>" (cause:
the setting that allowed it: resetFire, resetCountdown, resetLeft, resetReplaced, resetQuit or resets;
"exit" for the exit action's reset when the game process ends), "messagebox <poll>". Polls are numbered from
the log's first poll. A scenario's expected events leave out the poll numbers.
"""
import os
import re
import subprocess
import sys

FLAG_VANILLA, FLAG_GAME_OVER = 0x04, 0x40
KEYS = {"seq": "seq", "active": "active", "updates": "updates", "board": "boardSerial", "tick": "tick", "state": "state",
        "level": "level", "dlevel": "displayLevel", "dscore": "displayScore", "score": "score", "blc": "boardLevelClears",
        "gotick": "goTick", "smax": "scoreMax", "mode": "mode", "skill": "skill"}
PAIRS = {"go": ("goCount", "goSerial"), "lc": ("levelClears", "levelClearSerial"), "over": ("gameOvers", "gameOverSerial")}
DEATHS = ("deathsFire", "deathsCountdown", "deathsNewGame", "deathsOther")
MODES = ["Classic", "Action"]
SETTINGS = {"modeGuard": True, "vanillaOnly": False, "resets": True, "resetFire": True, "resetCountdown": True,
            "resetLeft": True, "resetReplaced": True, "resetQuit": True, "log": True}


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
        elif key == "hex":
            f["hex"] = value
    return f


def read_log(path):
    """[(t ms, fields or None for a torn read)]. Game log: "u=<update> <fields> hex=..." (t = 10 ms per update);
    probe log: "t=<ms> ok <fields> hex=..." or "t=<ms> torn" """
    polls = []
    with open(path, encoding="latin-1") as f:
        for line in f:
            words = line.split()
            if not words:
                continue
            if words[0].startswith("u="):
                polls.append((int(words[0][2:]) * 10, parse_fields(words[1:])))
            elif words[0].startswith("t="):
                t = int(words[0][2:])
                polls.append((t, None if words[1] == "torn" else parse_fields(words[2:])))
    return polls


def parse_splits(segments):
    """(targets, isScore) as the script's ParseSplits"""
    targets, is_score, levels = [], [], 0
    for name in segments:
        name = name.strip()
        if name.startswith("-"):
            name = name[1:].strip()
        if name.startswith("{") and name.find("}") > 0:
            name = name[name.find("}") + 1:].strip()
        digits, scale = name.replace(",", ""), 1
        if digits.endswith(("K", "k")):
            scale, digits = 1000, digits[:-1]
        if 0 < len(digits) <= 12 and digits.isdigit() and int(digits) * scale >= 1000:
            targets.append(int(digits) * scale)
            is_score.append(True)
        else:
            levels += 1
            targets.append(levels)
            is_score.append(False)
    return targets, is_score


def category_index(category, names, base):
    found = -1
    for i, n in enumerate(names):
        if n.lower() in category.lower():
            found = i + base if found == -1 else -2
    return found if found >= 0 else -1


def decide(polls, category="", segments=(), settings=None, attach=0):
    s = dict(SETTINGS)
    s.update(settings or {})
    targets, is_score = parse_splits(segments)
    events = []
    phase = "NotRunning"
    split_index = 0
    run_serial = 0
    pending = ""
    current = {}
    baseline = True
    seen_active = warned = False
    attach_t = polls[attach][0] if attach < len(polls) else 0

    def reached(cur):
        if run_serial == 0 or cur.get("boardSerial") != run_serial or split_index >= len(targets):
            return False
        if is_score[split_index]:
            return cur["scoreMax"] >= targets[split_index]
        return cur["boardLevelClears"] >= targets[split_index]

    cat_mode = category_index(category, MODES, 0)
    for i in range(attach, len(polls)):
        t, f = polls[i]
        old = dict(current)
        if f is None:
            continue
        if f["active"] == 0:
            if not seen_active and not warned and t - attach_t > 10000:
                warned = True
                events.append("messagebox %d" % i)
            baseline = True
            continue
        seen_active = True
        current = f
        if baseline:
            baseline = False
            continue
        cur = current
        if phase == "Running":
            # reset
            do_reset = False
            if s["resets"] and run_serial != 0:
                cause = pending
                if cur["gameOvers"] != old["gameOvers"] and cur["gameOverSerial"] == run_serial:
                    if cur["deathsFire"] != old["deathsFire"]:
                        cause = "resetFire"
                    elif cur["deathsCountdown"] != old["deathsCountdown"]:
                        cause = "resetCountdown"
                    elif cur["deathsNewGame"] != old["deathsNewGame"]:
                        cause = "resetLeft"
                    else:
                        cause = "resets"
                elif old["boardSerial"] == run_serial and cur["boardSerial"] != run_serial and not old["flags"] & FLAG_GAME_OVER:
                    cause = "resetQuit" if cur["boardSerial"] == 0 else "resetReplaced"
                allowed = bool(cause) and s[cause]
                hold = allowed and reached(cur)
                pending = cause if hold else ""
                do_reset = allowed and not hold
            if do_reset:
                events.append("reset %s %d board=%d" % (cause, i, cur["boardSerial"]))
                phase, split_index, run_serial, pending = "NotRunning", 0, 0, ""
            elif reached(cur):
                events.append("split %d %d board=%d" % (split_index, i, cur["boardSerial"]))
                split_index += 1
                if split_index >= len(targets):
                    phase = "Ended"
        if phase == "NotRunning":
            start = cur["goCount"] != old["goCount"] and cur["goSerial"] == cur["boardSerial"]
            if start and s["vanillaOnly"] and not cur["flags"] & FLAG_VANILLA:
                start = False
            if start and s["modeGuard"] and cat_mode >= 0 and cat_mode != cur["mode"]:
                start = False
            if start:
                events.append("start %d board=%d" % (i, cur["boardSerial"]))
                phase, split_index, run_serial, pending = "Running", 0, cur["boardSerial"], ""
    if phase == "Running" and run_serial != 0 and s["resets"] and s["resetQuit"]:
        events.append("reset exit %d board=%d" % (len(polls), run_serial))
    return events


def write_polls(polls, out):
    with open(out, "w") as f:
        for t, fields in polls:
            f.write("torn %d\n" % t if fields is None else "ok %d %s\n" % (t, fields["hex"]))


def asltest(exe, asl, polls_file, category, segments, settings, attach):
    args = [exe, asl, polls_file, "-category=" + category] + ["-segment=" + s for s in segments]
    args += ["-set=%s=%d" % (k, 1 if v else 0) for k, v in (settings or {}).items()]
    if attach:
        args.append("-attach=%d" % attach)
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError("asltest failed: " + r.stdout + r.stderr)
    return [line for line in r.stdout.splitlines() if line.strip()]


def normalise(events):
    """The events without poll numbers: "start board=1; split 0; reset resetFire" """
    out = []
    for e in events:
        w = e.split()
        if w[0] == "start":
            out.append("start " + w[2])
        elif w[0] == "split":
            out.append("split " + w[1])
        elif w[0] == "reset":
            out.append("reset " + w[1])
        else:
            out.append(w[0])
    return "; ".join(out)


def levels(n):
    return ["Level %d" % (i + 1) for i in range(n)]


# (name, run, category, segments, settings, attach, expected). The runs are run_tests.sh's (see there).
SCENARIOS = [
    # Classic seed 1 clears 7 levels and dies in the 8th: the killing word takes the score past the level's
    # mark (it is no level clear, the fire tile has already burned through), so the 8th level has no split
    ("classic_10_levels", "bot_classic", "Classic - 10 Levels", levels(10), {}, 0,
     "start board=1; split 0; split 1; split 2; split 3; split 4; split 5; split 6; reset resetFire"),
    ("classic_3_levels_ended", "bot_classic", "Classic - 3 Levels", levels(3), {}, 0,
     "start board=1; split 0; split 1; split 2"),
    # The run goes on past the game over it does not reset at; the bot quits after it (the exit action resets)
    ("classic_fire_reset_off", "bot_classic", "Classic - 10 Levels", levels(10), {"resetFire": False}, 0,
     "start board=1; split 0; split 1; split 2; split 3; split 4; split 5; split 6; reset exit"),
    ("classic_resets_off", "bot_classic", "Classic - 10 Levels", levels(10), {"resets": False}, 0,
     "start board=1; split 0; split 1; split 2; split 3; split 4; split 5; split 6"),
    ("classic_mode_guard", "bot_classic", "Action - 10 Levels", levels(10), {}, 0, ""),
    ("classic_guard_off", "bot_classic", "Action - 10 Levels", levels(10), {"modeGuard": False}, 0,
     "start board=1; split 0; split 1; split 2; split 3; split 4; split 5; split 6; reset resetFire"),
    ("classic_no_category", "bot_classic", "", levels(10), {}, 0,
     "start board=1; split 0; split 1; split 2; split 3; split 4; split 5; split 6; reset resetFire"),
    ("classic_score", "bot_classic", "Classic - 50K Points", ["10K", "25,000", "50K"], {}, 0,
     "start board=1; split 0; split 1; split 2"),
    # 58000 is reached by the killing word only (54430 -> 58970): its split comes with the game over, which
    # then does not reset a finished run; 60000 is not reached, so the game over resets
    ("classic_score_killing_word", "bot_classic", "Classic - 58K Points", ["10K", "25K", "58000"], {}, 0,
     "start board=1; split 0; split 1; split 2"),
    ("classic_score_not_reached", "bot_classic", "Classic - 60K Points", ["10K", "25K", "60000"], {}, 0,
     "start board=1; split 0; split 1; reset resetFire"),
    # "1M" is no number the script reads: a third level, which ends the run
    ("classic_mixed", "bot_classic", "Classic", ["Level 1", "10000", "Level 2", "1M"], {}, 0,
     "start board=1; split 0; split 1; split 2; split 3"),
    ("classic_attach_mid_game", "bot_classic", "Classic - 10 Levels", levels(10), {}, 2000, ""),
    # Action seed 1 clears 13 levels; the countdown ends the game in the 14th
    ("action_10_levels", "bot_action", "Action - 10 Levels", levels(10), {}, 0,
     "start board=1; split 0; split 1; split 2; split 3; split 4; split 5; split 6; split 7; split 8; split 9"),
    ("action_15_levels", "bot_action", "Action - 15 Levels", levels(15), {}, 0,
     "start board=1; split 0; split 1; split 2; split 3; split 4; split 5; split 6; split 7; split 8; split 9; "
     "split 10; split 11; split 12; reset resetCountdown"),
    ("action_countdown_reset_off", "bot_action", "Action - 15 Levels", levels(15), {"resetCountdown": False}, 0,
     "start board=1; split 0; split 1; split 2; split 3; split 4; split 5; split 6; split 7; split 8; split 9; "
     "split 10; split 11; split 12; reset exit"),
    ("action_mode_guard", "bot_action", "Classic - 10 Levels", levels(10), {}, 0, ""),
    # One process, a Classic game (3 level clears) and then an Action game (6): boards 1 and 2
    ("both_any_mode", "bot_both", "BookWorm - 10 Levels", levels(10), {}, 0,
     "start board=1; split 0; split 1; split 2; reset resetFire; "
     "start board=2; split 0; split 1; split 2; split 3; split 4; split 5; reset resetCountdown"),
    ("both_classic_only", "bot_both", "Classic - 10 Levels", levels(10), {}, 0,
     "start board=1; split 0; split 1; split 2; reset resetFire"),
    ("both_action_only", "bot_both", "Action - 10 Levels", levels(10), {}, 0,
     "start board=2; split 0; split 1; split 2; split 3; split 4; split 5; reset resetCountdown"),
    # A gameplay mod (a score key) is on: the run starts unless the script is set to vanilla games only. The bot
    # quits at update 3000, after 3 level clears; the board is gone in the game's last block write
    ("modded_runs", "bot_modded", "Classic - 10 Levels", levels(10), {}, 0,
     "start board=1; split 0; split 1; split 2; reset resetQuit"),
    ("modded_vanilla_only", "bot_modded", "Classic - 10 Levels", levels(10), {"vanillaOnly": True}, 0, ""),
    # LEAVE GAME in the 2nd level, then a new game (the saved one declined) in the same process, which quits.
    # The game's quit is a block write with no board, then the exit action: resetQuit at the write, or "exit"
    # when the run's board is already gone
    ("leave_game", "leave", "Classic - 10 Levels", levels(10), {}, 0,
     "start board=1; split 0; reset resetLeft; start board=2; reset resetQuit"),
    ("leave_game_reset_off", "leave", "Classic - 10 Levels", levels(10), {"resetLeft": False}, 0,
     "start board=1; split 0; reset exit"),
    ("quit_after_level", "quit", "Classic - 10 Levels", levels(10), {}, 0, "start board=1; split 0; reset resetQuit"),
    ("quit_reset_off", "quit", "Classic - 10 Levels", levels(10), {"resetQuit": False}, 0, "start board=1; split 0"),
    # A saved game continued: never a start
    ("resume_no_start", "resume", "Classic - 10 Levels", levels(10), {}, 0, ""),
    ("feature_off", "off:probe", "Classic - 10 Levels", levels(10), {}, 0, "messagebox"),
    # The probe's real polling sees the same events as the game's every-update log (None: the game log's)
    ("probe_matches_game", "probe_classic:probe", "Classic - 10 Levels", levels(10), {}, 0, None),
    ("probe_matches_game_score", "probe_classic:probe", "Classic", ["1000", "2000", "4000"], {}, 0, None),
]


def run_all(runs, exe, asl):
    failures = 0
    cache = {}
    for name, run, category, segments, settings, attach, expected in SCENARIOS:
        run, _, kind = run.partition(":")
        log = os.path.join(runs, run, "probe.log" if kind == "probe" else "block.log")
        if expected is None:
            expected = normalise(decide(read_log(os.path.join(runs, run, "block.log")), category, segments, settings, attach))
        run = run + (":probe" if kind == "probe" else "")
        if not os.path.isfile(log):
            print("%-30s MISSING %s" % (name, log))
            failures += 1
            continue
        if run not in cache:
            polls = read_log(log)
            polls_file = log + ".polls.txt"
            write_polls(polls, polls_file)
            cache[run] = (polls, polls_file)
        polls, polls_file = cache[run]
        mine = decide(polls, category, segments, settings, attach)
        real = asltest(exe, asl, polls_file, category, segments, settings, attach)
        ok_expected = normalise(mine) == expected
        ok_script = mine == real
        status = "ok" if ok_expected and ok_script else "FAIL"
        if status != "ok":
            failures += 1
        print("%-30s %s  %s" % (name, status, normalise(mine) or "(no events)"))
        if not ok_expected:
            print("    expected: %s" % (expected or "(no events)"))
        if not ok_script:
            print("    lsdecide: %s\n    asltest:  %s" % (" | ".join(mine), " | ".join(real)))
    print("%d scenarios, %d failed" % (len(SCENARIOS), failures))
    return 1 if failures else 0


def main(argv):
    if len(argv) >= 3 and argv[1] == "run":
        exe = argv[argv.index("--asltest") + 1]
        asl = argv[argv.index("--asl") + 1]
        return run_all(argv[2], exe, asl)
    if len(argv) >= 3 and argv[1] == "decide":
        category, segments, settings, attach, exe, asl = "", [], {}, 0, None, None
        i = 3
        while i < len(argv):
            a = argv[i]
            if a == "--category":
                category = argv[i + 1]
            elif a == "--segments":
                segments = argv[i + 1].split("|")
            elif a == "--set":
                k, v = argv[i + 1].split("=")
                settings[k] = v == "1"
            elif a == "--attach":
                attach = int(argv[i + 1])
            elif a == "--asltest":
                exe = argv[i + 1]
            elif a == "--asl":
                asl = argv[i + 1]
            i += 2
        polls = read_log(argv[2])
        for e in decide(polls, category, segments, settings, attach):
            print(e)
        if exe:
            polls_file = argv[2] + ".polls.txt"
            write_polls(polls, polls_file)
            print("asltest:")
            for e in asltest(exe, asl, polls_file, category, segments, settings, attach):
                print(e)
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
