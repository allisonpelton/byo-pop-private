#!/usr/bin/env python3
"""Copy the game data the Hammer Heads port needs out of a legally owned Hammer Heads Deluxe install.

The port never ships PopCap's or Nuclide's assets; every build stages them from the user's own copy into the
git-ignored assets/hammerheads/ folder (the native exe finds it next to itself or up to two levels up).

    python games/hammerheads/tools/stage_hammerheads_assets.py ["A:/SteamLibrary/steamapps/common/Hammer Heads Deluxe"] [--out assets/hammerheads]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Hammer Heads Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Hammer Heads Deluxe",
    r"C:\Program Files\Steam\steamapps\common\Hammer Heads Deluxe",
]

# The folders the game reads at runtime. The DRM files (drm.*, vhwb.dat, Flash.ocx), bass.dll, the launcher
# exe and the player's own saves are not copied; properties/*.sig is the DRM's signature of its partner file.
DIRS = ["boards", "gnomedata", "images", "sounds", "fonts", "properties"]
REQUIRED = ["properties/resources.xml", "boards/board001.xml", "boards/classic.csv", "fonts/Arial10.txt"]


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "properties", "resources.xml")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Hammer Heads Deluxe install folder (contains properties/resources.xml)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "assets", "hammerheads"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Hammer Heads Deluxe install (properties/resources.xml). Pass its folder as an argument.")
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
                # The framework resolves paths case-insensitively, but the copies keep lower-case extensions so that
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
