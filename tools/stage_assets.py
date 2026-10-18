#!/usr/bin/env python3
"""Copy the game data the port needs out of a legally owned Bejeweled Deluxe install.

The port never ships PopCap's assets; every build stages them from the user's own copy.

    python tools/stage_assets.py ["A:/SteamLibrary/steamapps/common/Bejeweled Deluxe"] [--out assets]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Bejeweled Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Bejeweled Deluxe",
    r"C:\Program Files\Steam\steamapps\common\Bejeweled Deluxe",
]

# Directories the game reads at runtime. drm/ and the DRM/Flash/Steam binaries are not needed.
DIRS = ["images", "data", "sounds", "music", "properties"]
REQUIRED = ["images/gemsheet6.png", "images/_gemsheet6.gif", "data/diff.txt", "properties/default.xml"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "WinBej.exe")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Bejeweled Deluxe install folder (contains WinBej.exe)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(__file__), "..", "assets"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Bejeweled Deluxe install (WinBej.exe). Pass its folder as an argument.")
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
