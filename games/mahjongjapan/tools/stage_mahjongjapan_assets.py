#!/usr/bin/env python3
"""Copy the game files the Mahjong Escape: Ancient Japan port needs out of a legally owned install.

The port never ships the game's data: Resource.dll holds the art, layouts, fonts, sounds, music and most texts as
PE resources, and the exe holds a few more texts and pictures. Both are read at run time from the user's own copy
(the port parses the PE resource directory itself and never runs either file), so staging only copies them into
the git-ignored assets/mahjongjapan/ folder (the native exe finds it next to itself or up to two levels up).

    python games/mahjongjapan/tools/stage_mahjongjapan_assets.py ["<install folder>"] [--out assets/mahjongjapan]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"C:\Program Files (x86)\Popcap Game Collection\Mahjong Escape Ancient Japan",
    r"C:\Program Files (x86)\PopCap Games\Mahjong Escape Ancient Japan",
    r"C:\Program Files\PopCap Games\Mahjong Escape Ancient Japan",
]

# The files the game reads. The exe is read for its resources only, never run; bass.dll is replaced by the
# platform's audio. MenuLogo.bmp and SplashLogo.bmp are the publisher's logos the exe loads by name. Not copied:
# the readme, the license and the player's own data.
FILES = ["MahjongEscapeAJ.exe", "Resource.dll", "MenuLogo.bmp", "SplashLogo.bmp"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and all(os.path.isfile(os.path.join(c, f)) for f in FILES[:2]):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="the game's install folder (MahjongEscapeAJ.exe, Resource.dll)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "mahjongjapan"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Mahjong Escape: Ancient Japan install (MahjongEscapeAJ.exe, Resource.dll). Pass its folder.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    n = 0
    for f in FILES:
        if os.path.isfile(os.path.join(src, f)):
            shutil.copy2(os.path.join(src, f), os.path.join(out, f))
            n += 1
    print(f"staged {n} files from {os.path.abspath(src)} into {out}")


if __name__ == "__main__":
    main()
