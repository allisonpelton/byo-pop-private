#!/usr/bin/env python3
"""Copy the game data the Dynomite port needs out of a legally owned Dynomite Deluxe install.

The port never ships PopCap's assets (images, fonts, sounds, music, the level pack, the texts of
properties/default.xml) or the texts inside the exe (read at start-up from the user's own Dynomite.exe);
every build stages them from the user's own copy into the git-ignored assets/dynomite/ folder (the native
exe finds it next to itself or up to two levels up).

    python games/dynomite/tools/stage_dynomite_assets.py ["A:/SteamLibrary/steamapps/common/Dynomite Deluxe"] [--out assets/dynomite]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Dynomite Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Dynomite Deluxe",
    r"C:\Program Files\PopCap Games\Dynomite Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Dynomite Deluxe",
]

# Folders and files the game reads at runtime. Dynomite.exe is read for the game's texts only, never run.
# properties/partner.xml is the distributor's settings (the Steam copy's says NoReg: the game is registered); it is
# optional. Not copied: the DRM wrapper's files (drm.swf, drm.xml, drmss.jpg, Flash.ocx), partner.xml's signature,
# bass.dll (the port plays the music itself), the readme and licence, and the player's own data
# (the .HS high-score files and saved games).
DIRS = ["images", "data", "sounds", "music", "stomped"]
FILES = ["Dynomite.exe", "DEFAULT.EGG", "properties/default.xml", "properties/partner.xml"]
REQUIRED = ["dynomite.exe", "default.egg", "properties/default.xml", "images/welcomescreen.gif",
            "data/tinyfont.txt", "music/amazonas.it"]
SKIP = {"thumbs.db", "readme.txt"}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "Dynomite.exe")) and os.path.isdir(os.path.join(c, "Images")):
            return c
    return None


def find_dir(src, name):
    # The install's folders are capitalised (Images, Stomped); match them case-insensitively.
    for e in os.listdir(src):
        if e.lower() == name and os.path.isdir(os.path.join(src, e)):
            return os.path.join(src, e)
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Dynomite Deluxe install folder (contains Dynomite.exe)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "dynomite"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Dynomite Deluxe install (Dynomite.exe, Images/). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)

    # The engine resolves paths case-insensitively; staged names are lowercase so case-sensitive file
    # systems (Emscripten MEMFS/IDBFS, Linux) see the names the game code asks for.
    count = 0
    for d in DIRS:
        sd = find_dir(src, d)
        if not sd:
            print(f"warning: {d} missing in {src}")
            continue
        for root, _, files in os.walk(sd):
            rel = os.path.join(d, os.path.relpath(root, sd)).lower()
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
            os.makedirs(os.path.dirname(os.path.join(out, f.lower())), exist_ok=True)
            shutil.copy2(p, os.path.join(out, f.lower()))
            count += 1

    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files from {src} -> {out}")


if __name__ == "__main__":
    main()
