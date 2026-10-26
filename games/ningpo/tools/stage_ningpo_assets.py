#!/usr/bin/env python3
"""Copy the game data the Ning Po port needs out of a legally owned Ning Po Mahjong Deluxe install.

The port never ships PopCap's or Pyrogon's data (ningpo.pyz holds the art, sounds, music and layouts) or the
game's texts in the exe (read at start-up from the user's own Ningpo.exe, see games/ningpo/src/GameText.h);
every build stages them from the user's own copy into the git-ignored assets/ningpo/ folder (the native exe finds
it next to itself or up to two levels up).

    python games/ningpo/tools/stage_ningpo_assets.py ["C:/Program Files/PopCap Games/NingPo MahJong Deluxe"] [--out assets/ningpo]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"C:\Program Files\PopCap Games\NingPo MahJong Deluxe",
    r"C:\Program Files (x86)\PopCap Games\NingPo MahJong Deluxe",
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "re", "_installers",
                 "WinNingPo_setup", "Data", "Filez"),
]

# The files the game reads at runtime. Ningpo.exe is read for the game's texts only, never run. Not copied: the
# readme, the license and the player's own data (the preferences and best times).
FILES = ["ningpo.pyz", "Ningpo.exe"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and all(os.path.isfile(os.path.join(c, f)) for f in FILES):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Ning Po Mahjong Deluxe install folder (Ningpo.exe, ningpo.pyz)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "ningpo"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Ning Po Mahjong Deluxe install (Ningpo.exe, ningpo.pyz). Pass its folder.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    for f in FILES:
        shutil.copy2(os.path.join(src, f), os.path.join(out, f))
    print(f"staged {len(FILES)} files from {os.path.abspath(src)} into {out}")


if __name__ == "__main__":
    main()
