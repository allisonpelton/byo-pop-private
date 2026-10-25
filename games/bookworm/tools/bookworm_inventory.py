#!/usr/bin/env python3
"""Write games/bookworm/docs/INVENTORY.md: BookWorm's game classes, their vtables and every game function.

Inputs are the Ghidra export (re/bookworm/gh/functions.jsonl) and the RTTI scan (re/bookworm/rtti.classes.json,
tools/rtti_msvc70.py). Slot names come from the framework base classes' own vtables as named in Ghidra
(docs/FRAMEWORK_2004.md; the 2004 framework has no public headers, so its slot names are the ones the
framework map applied). The game code is 0x401000-0x437800; source files are in link (alphabetical) order,
so each module is an address range.

    python games/bookworm/tools/bookworm_inventory.py [--out games/bookworm/docs/INVENTORY.md]
"""
import argparse
import collections
import json
import os

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
GAME_LO, GAME_HI = 0x401000, 0x437800
# (module, first address); a module runs to the next one's start
MODULES = [
    ("BWGameSelector.cpp", 0x401000), ("BigLex.cpp", 0x402df0), ("Board.cpp (with Piece, StatKeeper, WordScore)", 0x402ff0),
    ("BookWormApp.cpp", 0x4100f0), ("ColorizedButton.cpp", 0x41b3e0), ("CreditsDialog.cpp", 0x41bfc0),
    ("Definitionator.cpp", 0x41d8d0), ("DeleteUserDialog.cpp", 0x41ddb0),
    ("Effect.cpp (Effect, Sparkle, FlyingPieceList, EffectMgr, EmberListMgr)", 0x41e880),
    ("FameWidget.cpp", 0x421860), ("GetStringDialog.cpp", 0x428f50), ("HelpWidget.cpp", 0x4297e0),
    ("HintDialog.cpp", 0x42a820), ("ImagelessButton.cpp", 0x42b4c0), ("LabelWidget.cpp", 0x42b730),
    ("LevelUpWidget.cpp", 0x42b980), ("Lex.cpp", 0x42d050), ("Math.cpp", 0x42e330), ("MoneyDialog.cpp", 0x42e3d0),
    ("NewUserDialog.cpp", 0x42e480), ("OptionsDialog.cpp", 0x42eb40), ("OverlayWidget.cpp", 0x430970),
    ("Points.cpp", 0x431950), ("RegisterDialog.cpp", 0x431b50), ("SelectUserDialog.cpp", 0x4325f0),
    ("ShadowFont.cpp", 0x4330c0), ("TitleScreen.cpp", 0x433300), ("WaitDialog.cpp", 0x435e20),
    ("WordBalloon.cpp", 0x436340), ("(end)", GAME_HI),
]
GAME_CLASSES = ["BWGameSelector", "BigLex", "Piece", "StatKeeper", "WordScore", "Board", "BookWormApp",
                "ColorizedButton", "CreditsDialog", "Definitionator", "DeleteUserDialog", "Effect", "Sparkle",
                "FlyingPieceList", "EffectMgr", "EmberListMgr", "FameWidget", "GetStringDialog", "HelpWidget",
                "HintDialog", "ImagelessButton", "LabelWidget", "LevelUpWidget", "Lex", "Math", "MoneyDialog",
                "NewUserDialog", "OptionsDialog", "OverlayWidget", "Points", "RegisterDialog", "SelectUserDialog",
                "ShadowFont", "TitleScreen", "WaitDialog", "WordBalloon"]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=os.path.join(ROOT, "games", "bookworm", "docs", "INVENTORY.md"))
    ap.add_argument("--functions", default=os.path.join(ROOT, "re", "bookworm", "gh", "functions.jsonl"))
    ap.add_argument("--classes", default=os.path.join(ROOT, "re", "bookworm", "rtti.classes.json"))
    ap.add_argument("--slots", default=os.path.join(ROOT, "re", "bookworm", "fw", "slots.json"),
                    help="framework slot names {class: {slot: method}} (from the framework map's vtable tables)")
    args = ap.parse_args()

    funcs = {}
    for line in open(args.functions, encoding="utf-8"):
        f = json.loads(line)
        try:
            funcs[int(f["addr"], 16)] = f
        except ValueError:
            pass
    classes = json.load(open(args.classes, encoding="utf-8"))
    slots = json.load(open(args.slots, encoding="utf-8")) if os.path.isfile(args.slots) else {}

    def primary(cls):
        info = classes.get("Sexy::" + cls)
        return next((v for v in info["vtables"] if v["offset"] == 0), None) if info else None

    def base_slots(cls):
        """Primary vtable of the nearest base class (game or framework) of cls, or []."""
        info = classes.get("Sexy::" + cls)
        for b in (info["bases"] if info else []):
            vt = primary(b.split("::")[-1])
            if vt:
                return [int(x, 16) for x in vt["slots"]]
        return []

    def slot_name(cls, i):
        """Name of slot i in the nearest framework base class of cls (the base's own vtable function)."""
        info = classes.get("Sexy::" + cls)
        if not info:
            return None
        for b in info["bases"]:
            bn = b.split("::")[-1]
            if str(i) in slots.get(bn, {}):
                return slots[bn][str(i)]
        for b in info["bases"]:
            bn = b.split("::")[-1]
            if bn in GAME_CLASSES:
                continue
            vt = primary(bn)
            if vt and i < len(vt["slots"]):
                f = funcs.get(int(vt["slots"][i], 16))
                if f:
                    return f["name"]
        return None

    out = []
    w = out.append
    w("# BookWorm Deluxe: game class and function inventory")
    w("")
    w("Generated by `games/bookworm/tools/bookworm_inventory.py` from the Ghidra export of the carved game PE and")
    w("the RTTI scan; names are the ones applied in Ghidra. Game code is 0x%x-0x%x, in source files linked in" % (GAME_LO, GAME_HI))
    w("alphabetical order. Slot names are the 2004 framework's ([FRAMEWORK_2004.md](../../../docs/FRAMEWORK_2004.md)).")
    w("")
    w("## Classes")
    w("")
    w("| Class | Bases | Vtables (offset: slots) | Overridden or new slots |")
    w("|---|---|---|---|")
    vt_owner = {}
    for c in GAME_CLASSES:
        info = classes.get("Sexy::" + c)
        if not info:
            w("| %s | (no RTTI vtable) | | |" % c)
            continue
        vts = ", ".join("+0x%x: %d" % (v["offset"], len(v["slots"])) for v in sorted(info["vtables"], key=lambda v: v["offset"]))
        own = []
        prim = primary(c)
        if prim:
            base = base_slots(c)
            for i, a in enumerate(prim["slots"]):
                ai = int(a, 16)
                vt_owner.setdefault(ai, []).append("%s slot %d" % (c, i))
                if i >= len(base) or ai != base[i]:
                    nm = slot_name(c, i)
                    own.append("%d `%s`" % (i, nm if nm else "(new virtual)"))
        for v in info["vtables"]:
            if v["offset"]:
                for i, a in enumerate(v["slots"]):
                    vt_owner.setdefault(int(a, 16), []).append("%s +0x%x slot %d" % (c, v["offset"], i))
        w("| %s | %s | %s | %s |" % (c, ", ".join(b.split("::")[-1] for b in info["bases"]), vts, "; ".join(own)))
    w("")
    w("## Functions by module")
    w("")
    total = collections.Counter()
    for (mod, lo), (_, hi) in zip(MODULES, MODULES[1:]):
        fs = sorted(a for a in funcs if lo <= a < hi)
        size = sum(funcs[a]["size"] for a in fs)
        w("### %s (0x%x-0x%x, %d functions, %d bytes)" % (mod, lo, hi, len(fs), size))
        w("")
        w("| Address | Bytes | Name | Vtable slots | Callers |")
        w("|---|---|---|---|---|")
        for a in fs:
            f = funcs[a]
            name = f["full"] if f["ns"] != "Global" else f["name"]
            w("| %08x | %d | %s | %s | %d |" % (a, f["size"], name.replace("|", "\\|"), "; ".join(vt_owner.get(a, [])),
                                                len(f["callers"])))
            total["fns"] += 1
            total["bytes"] += f["size"]
        w("")
    w("Total: %d functions, %d bytes." % (total["fns"], total["bytes"]))
    w("")
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out))
    print("wrote %s (%d functions)" % (args.out, total["fns"]))


if __name__ == "__main__":
    main()
