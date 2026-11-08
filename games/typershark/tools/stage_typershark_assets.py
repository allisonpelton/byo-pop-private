#!/usr/bin/env python3
"""Copy the game data the Typer Shark port needs out of a legally owned Typer Shark! Deluxe install.

The port never ships PopCap's assets (images, fonts, sounds, music, level and word files) or the game's texts (read at
start-up from the user's own WinTS.exe, see games/typershark/src/GameText.h); every build stages them from the user's own
copy into the git-ignored assets/typershark/ folder (the native exe finds it next to itself or up to two levels up).

    python games/typershark/tools/stage_typershark_assets.py ["A:/SteamLibrary/steamapps/common/Typer Shark Deluxe"] [--out assets/typershark]
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Typer Shark Deluxe",
    r"E:\SteamLibrary\steamapps\common\Typer Shark Deluxe",
    r"C:\Program Files\PopCap Games\Typer Shark Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Typer Shark Deluxe",
]

# Folders and files the game reads at runtime. WinTS.exe (the launcher with the game appended) is read for the game's
# texts only, never run. Not copied: Flash.ocx, drm.swf, drm.xml and drmss.jpg (the launcher's screens), bass.dll
# (the port plays music itself), the uninstaller, readme/licence, the properties signature, and the player's own data.
DIRS = ["images", "data", "sounds", "music", "properties"]
FILES = ["WinTS.exe"]
REQUIRED = ["wints.exe", "data/twords.txt", "data/easy-cfg.xml", "images/titlescreen.jpg", "music/wordshar.mo3",
            "properties/partner.xml"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "WinTS.exe")) and os.path.isdir(os.path.join(c, "images")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Typer Shark! Deluxe install folder (contains WinTS.exe)")
    ap.add_argument("--out", default=os.path.join(ROOT, "assets", "typershark"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Typer Shark! Deluxe install (WinTS.exe, images/). Pass its folder as an argument.")
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
                if low.endswith(".sig"):
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
