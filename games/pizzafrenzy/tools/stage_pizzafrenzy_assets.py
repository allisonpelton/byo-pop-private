#!/usr/bin/env python3
"""Copy the game data the Pizza Frenzy port needs out of a legally owned Pizza Frenzy install.

The port never ships PopCap's or Sprout's data: PizzaFrenzy.saf holds the pictures, fonts, screen layouts, cities,
levels, stories and sounds, splash/ the splash screen's picture and properties/ the publisher's settings, and the
port reads the game's texts (its string table and the texts in its code) from the player's PizzaFrenzy.exe, which it
never runs. Every build stages them from the user's own copy
into the git-ignored assets/pizzafrenzy/ folder (the native exe finds it next to itself or up to two levels up). The
install is only read.

    python -I games/pizzafrenzy/tools/stage_pizzafrenzy_assets.py ["<install>"] [--out assets/pizzafrenzy]
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Pizza Frenzy",
    r"C:\Program Files (x86)\Steam\steamapps\common\Pizza Frenzy",
    r"C:\Program Files\PopCap Games\Pizza Frenzy",
    r"C:\Program Files (x86)\PopCap Games\Pizza Frenzy",
]

# The archive, the exe (read for its texts) and the loose files the game reads next to them. Not copied: the Flash
# files and the publisher's screens, the readme and the license.
ARCHIVE = "PizzaFrenzy.saf"
FILES = ["PizzaFrenzy.exe"]
FOLDERS = ["splash", "properties"]
SKIP = {"thumbs.db"}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, ARCHIVE)):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Pizza Frenzy install folder (PizzaFrenzy.saf, splash/)")
    ap.add_argument("--out", default=os.path.join(HERE, "..", "..", "..", "assets", "pizzafrenzy"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Pizza Frenzy install (PizzaFrenzy.saf). Pass its folder.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    shutil.copy2(os.path.join(src, ARCHIVE), os.path.join(out, ARCHIVE))
    count = 1
    for name in FILES:
        if os.path.isfile(os.path.join(src, name)):
            shutil.copy2(os.path.join(src, name), os.path.join(out, name))
            count += 1
    for folder in FOLDERS:
        sdir = os.path.join(src, folder)
        if not os.path.isdir(sdir):
            continue
        ddir = os.path.join(out, folder)
        os.makedirs(ddir, exist_ok=True)
        for name in sorted(os.listdir(sdir)):
            if name.lower() in SKIP or not os.path.isfile(os.path.join(sdir, name)):
                continue
            shutil.copy2(os.path.join(sdir, name), os.path.join(ddir, name))
            count += 1
    print(f"staged {count} files from {os.path.abspath(src)} into {out}")


if __name__ == "__main__":
    main()
