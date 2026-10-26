#!/usr/bin/env python3
"""Copy the game data the Seven Seas port needs out of a legally owned Seven Seas Deluxe install.

The port never ships PopCap's assets (images, fonts, sounds, music) or the game's texts (read at start-up from
the user's own WinSS.exe, see games/sevenseas/src/GameText.h); every build stages them from the user's own copy
into the git-ignored assets/sevenseas/ folder (the native exe finds it next to itself or up to two levels up).

    python games/sevenseas/tools/stage_sevenseas_assets.py ["C:/Program Files/PopCap Games/Seven Seas Deluxe"] [--out assets/sevenseas]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"C:\Program Files\PopCap Games\Seven Seas Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Seven Seas Deluxe",
]

# Folders and files the game reads at runtime. WinSS.exe is read for the game's texts only, never run.
# Not copied: fmod.dll (the port plays music itself), the uninstaller, readme/licence, the internet score
# template scoreul.tpl, and the player's own data (data/savegame.dat, hiscores.dat).
DIRS = ["images", "data", "sounds", "music"]
FILES = ["WinSS.exe"]
REQUIRED = ["winss.exe", "music/game.it", "images/title.jpg", "data/font1.txt", "data/anims.txt"]
SKIP = {"savegame.dat", "hiscores.dat"}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "WinSS.exe")) and os.path.isdir(os.path.join(c, "images")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Seven Seas Deluxe install folder (contains WinSS.exe)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "sevenseas"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Seven Seas Deluxe install (WinSS.exe, images/). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)

    # The engine resolves paths case-insensitively; staged names are lowercase so case-sensitive file
    # systems (Emscripten MEMFS/IDBFS, Linux) see the names the game code asks for.
    count = 0
    for d in DIRS:
        sd = os.path.join(src, d)
        if not os.path.isdir(sd):
            print(f"warning: {sd} missing")
            continue
        for root, _, files in os.walk(sd):
            rel = os.path.relpath(root, src).lower()
            os.makedirs(os.path.join(out, rel), exist_ok=True)
            for f in files:
                low = f.lower()
                if low in SKIP:
                    continue
                shutil.copy2(os.path.join(root, f), os.path.join(out, rel, low))
                count += 1
    for f in FILES:
        p = os.path.join(src, f)
        if os.path.isfile(p):
            shutil.copy2(p, os.path.join(out, f.lower()))
            count += 1

    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files from {src} -> {out}")


if __name__ == "__main__":
    main()
