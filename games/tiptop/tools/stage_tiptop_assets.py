#!/usr/bin/env python3
"""Copy the game data the TipTop port needs out of a legally owned TipTop Deluxe install.

The port never ships PopCap's data (TipTop.dat holds the art, sounds, music, configuration and texts) or the
game's texts in the exe (read at start-up from the user's own TipTop.exe, see games/tiptop/src/GameText.h);
every build stages them from the user's own copy into the git-ignored assets/tiptop/ folder (the native exe finds
it next to itself or up to two levels up).

    python games/tiptop/tools/stage_tiptop_assets.py ["C:/Program Files/PopCap Games/TipTop Deluxe"] [--out assets/tiptop]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"C:\Program Files\PopCap Games\TipTop Deluxe",
    r"C:\Program Files (x86)\PopCap Games\TipTop Deluxe",
]

# The files the game reads at runtime. TipTop.exe is read for the game's texts only, never run. Not copied: the
# uninstaller, the readme and the player's own data (Preferences.dat, the saved games).
FILES = ["TipTop.dat", "TipTop.exe"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and all(os.path.isfile(os.path.join(c, f)) for f in FILES):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="TipTop Deluxe install folder (contains TipTop.exe and TipTop.dat)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "tiptop"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a TipTop Deluxe install (TipTop.exe, TipTop.dat). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    for f in FILES:
        shutil.copy2(os.path.join(src, f), os.path.join(out, f))
    print(f"staged {len(FILES)} files from {src} into {out}")


if __name__ == "__main__":
    main()
