#!/usr/bin/env python3
"""Copy the game data the Cosmic Bugs port needs out of a legally owned Cosmic Bugs install.

The port never ships Retro64's or PopCap's data (the art banks, the FLIC animations, the XML data files, the
sounds and music) or the game's texts in the exe (read at start-up from the user's own CosmicBugs.exe); every build
stages them from the user's own copy into the git-ignored assets/cosmicbugs/ folder (the native exe finds it next
to itself or up to two levels up).

    python games/cosmicbugs/tools/stage_cosmicbugs_assets.py ["C:/Program Files (x86)/Popcap Game Collection/Cosmic Bugs"] [--out assets/cosmicbugs]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"C:\Program Files (x86)\Popcap Game Collection\Cosmic Bugs",
    r"C:\Program Files\Popcap Game Collection\Cosmic Bugs",
]

# The files the game reads at runtime. CosmicBugs.exe is read for the game's texts only, never run. Not copied:
# bass.dll (the port plays sound through the platform), the readme, the license, properties/ (the exe never reads
# it), and the player's own data (cb.dat, cp.cfg).
FILES = ["CosmicBugs.exe", "art.bbk", "art.dat", "art8.bbk", "art8.dat"]
# Whole folders, by extension: the animations and data files, the sounds and the music. Never the tools that came
# in animations/ (an exe and a batch file).
FOLDERS = {"animations": (".flc", ".xml"), "audio": (".wav", ".mo3")}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and all(os.path.isfile(os.path.join(c, f)) for f in FILES):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Cosmic Bugs install folder (CosmicBugs.exe, art.bbk...)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "cosmicbugs"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Cosmic Bugs install (CosmicBugs.exe, art.bbk...). Pass its folder.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    n = 0
    for f in FILES:
        shutil.copy2(os.path.join(src, f), os.path.join(out, f))
        n += 1
    for folder, exts in FOLDERS.items():
        os.makedirs(os.path.join(out, folder), exist_ok=True)
        for f in sorted(os.listdir(os.path.join(src, folder))):
            if os.path.splitext(f)[1].lower() in exts:
                shutil.copy2(os.path.join(src, folder, f), os.path.join(out, folder, f))
                n += 1
    print(f"staged {n} files from {os.path.abspath(src)} into {out}")


if __name__ == "__main__":
    main()
