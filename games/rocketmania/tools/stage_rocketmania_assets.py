#!/usr/bin/env python3
"""Copy the game data the Rocket Mania Deluxe port needs out of a legally owned Rocket Mania Deluxe install.

The port never ships PopCap's assets (images, fonts, sounds, music) or the game's texts (read at start-up from
the user's own RocketMania.exe, see games/rocketmania/src/GameText.h); every build stages them from the user's own
copy into the git-ignored assets/rocketmania/ folder (the native exe finds it next to itself or up to two levels up).

    python games/rocketmania/tools/stage_rocketmania_assets.py ["<install folder>"] [--out assets/rocketmania]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Rocket Mania Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Rocket Mania Deluxe",
    r"C:\Program Files\Steam\steamapps\common\Rocket Mania Deluxe",
]

# Folders and files the game reads at runtime. RocketMania.exe (PopCap's wrapper with the game inside) is read for the
# game's texts only, never run. Not copied: bass.dll and Flash.ocx (the port plays music itself), drm.swf, drm.xml
# and drmss.jpg (the wrapper's), license, readme and version, the player's own data (Players/), and the music
# cache cached_*.wav (the port joins the pieces itself, src/AppMusic.cpp).
DIRS = ["images", "data", "sounds", "music", "properties"]
FILES = ["RocketMania.exe"]
REQUIRED = ["rocketmania.exe", "music/tubetricks game over loop one v01.ogg", "images/loadinggraphic.jpg",
            "data/arial8.txt", "properties/partner.xml"]
SKIP_PREFIX = ("cached_",)


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "RocketMania.exe")) and os.path.isdir(os.path.join(c, "images")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Rocket Mania Deluxe install folder (contains RocketMania.exe)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "rocketmania"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Rocket Mania Deluxe install (RocketMania.exe, images/). Pass its folder as an argument.")
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
                if low.startswith(SKIP_PREFIX) or low.endswith(".sig"):
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
