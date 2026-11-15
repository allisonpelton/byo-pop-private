#!/usr/bin/env python3
"""Flags every mod text longer than the menu takes (platform/ModSettings.cpp cuts it, with a warning the menu shows).

    python -I tools/modtextcheck.py [--over N] [mod.json or folder ...]

The limits are read from platform/ModSettings.cpp itself (kMaxLabel, kMaxHelp, kMaxUnit), so the check follows the
code: a setting's and a section's "label" and a choice's "label" (kMaxLabel), a setting's "help" (kMaxHelp), an int's
or a float's "unit" (kMaxUnit). Lengths are in characters (code points), as ModSettings counts them. A mod's "name"
and "description" have no limit.

Without paths it reads every games/*/mods/*/mod.json and the menu tests' fixture mods (tools/fixtures/mods,
tools/modmenutest/mods). Each text over its limit is one line:
    games/venice/mods/fixes/mod.json: setting "typos": "help" has 412 characters, at most 200
and the last line is the summary. The exit code is 1 when a text is over its limit, 0 when none is.

--over N lists, besides, every help longer than N characters (the texts a lower limit would cut, 200 before
2026-11-14), without failing.
"""

import argparse
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(ROOT, "platform", "ModSettings.cpp")


def limits():
    """kMaxLabel, kMaxHelp and kMaxUnit as ModSettings.cpp defines them"""
    with open(SOURCE, encoding="utf-8", errors="replace") as f:
        text = f.read()
    found = {}
    for name in ("kMaxLabel", "kMaxHelp", "kMaxUnit"):
        m = re.search(r"\b" + name + r"\s*=\s*(\d+)\s*;", text)
        if not m:
            sys.exit("modtextcheck: no %s in %s" % (name, os.path.relpath(SOURCE, ROOT)))
        found[name] = int(m.group(1))
    return found


def manifests(paths):
    if not paths:
        paths = (glob.glob(os.path.join(ROOT, "games", "*", "mods", "*", "mod.json")) +
                 glob.glob(os.path.join(ROOT, "tools", "fixtures", "mods", "*", "mod.json")) +
                 glob.glob(os.path.join(ROOT, "tools", "modmenutest", "mods", "*", "mod.json")))
    out = []
    for p in paths:
        if os.path.isdir(p):
            out += glob.glob(os.path.join(p, "**", "mod.json"), recursive=True)
        else:
            out.append(p)
    return sorted(set(os.path.abspath(p) for p in out))


def texts(manifest):
    """(where, key, text, limit name) for every text a limit applies to"""
    settings = manifest.get("settings")
    if not isinstance(settings, list):
        return
    for i, entry in enumerate(settings):
        if not isinstance(entry, dict):
            continue
        where = 'setting "%s"' % entry["id"] if isinstance(entry.get("id"), str) else "settings[%d]" % i
        if entry.get("type") == "section":
            yield where, "label", entry.get("label"), "kMaxLabel"
            continue
        yield where, "label", entry.get("label"), "kMaxLabel"
        yield where, "help", entry.get("help"), "kMaxHelp"
        if entry.get("type") in ("int", "float"):
            yield where, "unit", entry.get("unit"), "kMaxUnit"
        choices = entry.get("choices")
        if entry.get("type") == "choice" and isinstance(choices, list):
            for choice in choices:
                if isinstance(choice, dict):
                    yield (where + " choice %s" % json.dumps(choice.get("value")), "label", choice.get("label"),
                           "kMaxLabel")


def main(argv=None):
    ap = argparse.ArgumentParser(description="Flags every mod text longer than its limit in platform/ModSettings.cpp.")
    ap.add_argument("--over", type=int, help="also list every help longer than this many characters, without failing")
    ap.add_argument("paths", nargs="*", help="mod.json files or folders (default: every game's mods and the fixtures)")
    args = ap.parse_args(argv)
    limit = limits()
    files = manifests(args.paths)
    over, longer, count = 0, 0, 0
    for path in files:
        name = os.path.relpath(path, ROOT).replace(os.sep, "/")
        try:
            with open(path, encoding="utf-8") as f:
                manifest = json.load(f)
        except (OSError, ValueError) as e:
            print("%s: cannot be read (%s)" % (name, e))
            over += 1
            continue
        if not isinstance(manifest, dict):
            continue
        for where, key, text, limit_name in texts(manifest):
            if not isinstance(text, str):
                continue
            count += 1
            if len(text) > limit[limit_name]:
                print('%s: %s: "%s" has %d characters, at most %d' % (name, where, key, len(text), limit[limit_name]))
                over += 1
            elif args.over is not None and key == "help" and len(text) > args.over:
                print('%s: %s: "%s" has %d characters (over %d)' % (name, where, key, len(text), args.over))
                longer += 1
    summary = "modtextcheck: %d texts in %d mods; %d over the limits (label %d, help %d, unit %d)" % (
        count, len(files), over, limit["kMaxLabel"], limit["kMaxHelp"], limit["kMaxUnit"])
    if args.over is not None:
        summary += "; %d helps over %d" % (longer, args.over)
    print(summary)
    return 1 if over else 0


if __name__ == "__main__":
    sys.exit(main())
