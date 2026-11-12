#!/usr/bin/env python3
"""Copy the game data the BookWorm port needs out of a legally owned BookWorm Deluxe install.

The port never ships PopCap's assets (images, fonts, sounds, music, and the word lists and definitions,
which are game data too) or the game's texts (read at start-up from the user's own Bookworm.exe, see
games/bookworm/src/GameText.h); every build stages them from the user's own copy into the git-ignored
assets/bookworm/ folder (the native exe finds it next to itself or up to two levels up).

    python games/bookworm/tools/stage_bookworm_assets.py ["C:/Program Files/PopCap Games/BookWorm Deluxe"] [--out assets/bookworm]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"C:\Program Files\PopCap Games\BookWorm Deluxe",
    r"C:\Program Files (x86)\PopCap Games\BookWorm Deluxe",
    r"A:\SteamLibrary\steamapps\common\BookWorm Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\BookWorm Deluxe",
]

# Folders and files the game reads at runtime. Bookworm.exe (the CD-protect launcher with the game appended, or
# the PopCap Game Collection's, which is the game itself) is read for the game's texts only, never run. Not copied: hw.dat, BASS, the uninstaller,
# readme/licence/register.tpl, the original's runtime WAV cache (sounds/cached_*.wav), the properties
# signature, and the player's own data (players/, classic.dat, action.dat).
DIRS = ["images", "fonts", "sounds", "music", "properties"]
FILES = ["wordlist.txt", "bonus_word_dictionary.txt", "popdefs.txt", "Bookworm.exe"]
REQUIRED = ["wordlist.txt", "popdefs.txt", "bonus_word_dictionary.txt", "bookworm.exe", "music/bookworm1.mo3",
            "fonts/lockergnome18.txt", "properties/partner.xml"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "wordlist.txt")) and os.path.isdir(os.path.join(c, "images")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="BookWorm Deluxe install folder (contains wordlist.txt)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "bookworm"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a BookWorm Deluxe install (wordlist.txt, images/). Pass its folder as an argument.")
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
                if low.endswith(".sig") or (d == "sounds" and low.startswith("cached_")):
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
