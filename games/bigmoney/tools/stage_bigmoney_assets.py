#!/usr/bin/env python3
"""Copy the game data the Big Money port needs out of a legally owned Big Money Deluxe install.

The port never ships PopCap's assets; every build stages them from the user's own copy into the
git-ignored assets/bigmoney/ folder (the native exe finds it next to itself or up to two levels up).

    python games/bigmoney/tools/stage_bigmoney_assets.py ["A:/SteamLibrary/steamapps/common/Big Money Deluxe"] [--out assets/bigmoney]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Big Money Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Big Money Deluxe",
    r"C:\Program Files\Steam\steamapps\common\Big Money Deluxe",
]

# Directories the game reads at runtime. cached/ (the original's WAV cache), drm.*, the DRM/Flash/BASS
# binaries and the player's own saves (savegame.dat, hiscores.dat) are not copied.
DIRS = ["images", "data", "sounds", "music", "properties"]
REQUIRED = ["images/mrmoneybags.jpg", "data/bigmoneyfont1.txt", "properties/default.xml",
            "music/QualityTime_BigMoney.it"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "images", "mrmoneybags.jpg")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Big Money Deluxe install folder (contains images/mrmoneybags.jpg)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "assets", "bigmoney"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Big Money Deluxe install (images/mrmoneybags.jpg). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)

    count = 0
    for d in DIRS:
        sd = os.path.join(src, d)
        if not os.path.isdir(sd):
            print(f"warning: {sd} missing")
            continue
        for root, _, files in os.walk(sd):
            rel = os.path.relpath(root, src)
            os.makedirs(os.path.join(out, rel), exist_ok=True)
            for f in files:
                if f.endswith(".sig"):
                    continue
                # The engine resolves paths case-insensitively, but stage lowercase-extension copies so
                # case-sensitive filesystems (Emscripten MEMFS, Linux) see consistent names.
                name = f[:f.rfind(".")] + f[f.rfind("."):].lower() if "." in f else f
                shutil.copy2(os.path.join(root, f), os.path.join(out, rel, name))
                count += 1

    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files from {src} -> {out}")


if __name__ == "__main__":
    main()
