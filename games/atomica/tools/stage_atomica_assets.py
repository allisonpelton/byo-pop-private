#!/usr/bin/env python3
"""Copy the game data the Atomica port needs out of a legally owned Atomica Deluxe install.

The port never ships PopCap's assets (images, fonts, sounds, music) or the game's texts (read at start-up from
the user's own Atomica.exe, see games/atomica/src/GameText.h); every build stages them from the user's own copy
into the git-ignored assets/atomica/ folder (the native exe finds it next to itself or up to two levels up).

    python games/atomica/tools/stage_atomica_assets.py ["C:/Program Files/PopCap Games/Atomica Deluxe"] [--out assets/atomica]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"C:\Program Files\PopCap Games\Atomica Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Atomica Deluxe",
]

# Folders and files the game reads at runtime. Atomica.exe is read for the game's texts only, never run.
# Not copied: fmod.dll (the port plays music itself), the uninstaller, readme/licence, and the
# player's own data (savegame.dat, hs.cfg).
DIRS = ["images", "data", "sounds", "music"]
FILES = ["Atomica.exe"]
REQUIRED = ["atomica.exe", "music/atomica.it", "images/titlescreen.jpg", "data/atomicfont.txt"]
SKIP = {"savegame.dat", "hs.cfg", "thumbs.db"}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "Atomica.exe")) and os.path.isdir(os.path.join(c, "images")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Atomica Deluxe install folder (contains Atomica.exe)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "atomica"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Atomica Deluxe install (Atomica.exe, images/). Pass its folder as an argument.")
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
