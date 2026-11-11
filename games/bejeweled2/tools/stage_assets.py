#!/usr/bin/env python3
"""Copy the game data the port needs out of a legally owned Bejeweled 2 Deluxe install.

The port never ships PopCap's assets; every build stages them from the user's own copy into the
git-ignored assets/bejeweled2/ folder (bejeweled2.exe finds it from build/native by itself).

    python games/bejeweled2/tools/stage_assets.py ["A:/SteamLibrary/steamapps/common/Bejeweled 2 Deluxe"]
                                                  [--out assets/bejeweled2]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Bejeweled 2 Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Bejeweled 2 Deluxe",
    r"C:\Program Files\Steam\steamapps\common\Bejeweled 2 Deluxe",
]

# Directories the game reads at runtime. drm/ and the DRM/Flash/Steam binaries are not needed.
DIRS = ["images", "data", "edata", "sounds", "music", "properties", "puzzles"]
REQUIRED = ["properties/resources.xml", "properties/default.xml", "music/BeyondNetwork.mo3",
            "puzzles/Galaxy One/galaxy.xml"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "WinBej2.exe")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Bejeweled 2 Deluxe install folder (contains WinBej2.exe)")
    repo = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..")
    ap.add_argument("--out", default=os.path.normpath(os.path.join(repo, "assets", "bejeweled2")))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Bejeweled 2 Deluxe install (WinBej2.exe). Pass its folder as an argument.")
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
                shutil.copy2(os.path.join(root, f), os.path.join(out, rel, f))
                count += 1

    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files from {src} -> {out}")


if __name__ == "__main__":
    main()
