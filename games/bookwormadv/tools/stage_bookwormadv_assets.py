#!/usr/bin/env python3
"""Copy the game data the Bookworm Adventures port needs out of a legally owned Bookworm Adventures Deluxe install.

The port never ships PopCap's assets (images, animations, scripts, fonts, sounds, music) or the game's texts; every
build stages them from the user's own copy into the git-ignored assets/bookwormadv/ folder (the native exe finds it
next to itself or up to two levels up).

    python games/bookwormadv/tools/stage_bookwormadv_assets.py ["C:/Program Files (x86)/PopCap Games/Bookworm Adventures Deluxe"]
        [--out assets/bookwormadv] [--no-png]

main.pak (XOR 0xF7, tools/popcap_pak.py) is unpacked; the game reads its files exactly as it reads loose ones, and
loose files win over the pak's in the original too. Names are staged in lower case, so case-sensitive file systems
(Emscripten MEMFS/IDBFS, Linux) see the names the game asks for. The 42 JPEG 2000 images (.jp2) are also written as
.png next to themselves (Pillow), because the framework's image loader has no JPEG 2000 decoder; --no-png skips that.
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import popcap_pak  # noqa: E402

DEFAULT_INSTALLS = [
    r"C:\Program Files (x86)\PopCap Games\Bookworm Adventures Deluxe",
    r"C:\Program Files\PopCap Games\Bookworm Adventures Deluxe",
]
EXE = "BookwormAdventures.exe"

# Loose files the game reads besides main.pak. The exe is read for data only, never run. Not copied: drm/ (the
# DRM client's UI), Flash.ocx, bass.dll, j2k-codec.dll (the port has its own audio and image code), the
# uninstaller, readme, licence and install logs.
LOOSE_DIRS = ["images", "properties"]
LOOSE_FILES = [EXE]
REQUIRED = ["bookwormadventures.exe", "properties/resources.xml", "scripts/main.luc", "data/compressed.txt"]


def find_install(explicit):
    for c in ([explicit] if explicit else DEFAULT_INSTALLS):
        if c and os.path.isfile(os.path.join(c, "main.pak")):
            return c
    return None


def write(out, rel, data):
    path = os.path.join(out, rel.lower())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)
    return path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Bookworm Adventures Deluxe install folder (contains main.pak)")
    ap.add_argument("--out", default=os.path.join(ROOT, "assets", "bookwormadv"))
    ap.add_argument("--no-png", action="store_true", help="do not write .png copies of the .jp2 images")
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Bookworm Adventures Deluxe install (main.pak). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)

    pak = open(os.path.join(src, "main.pak"), "rb").read()
    count = 0
    jp2 = []
    for name, offset, size in popcap_pak.read_records(pak, 0xF7):
        data = bytes(x ^ 0xF7 for x in pak[offset:offset + size])
        path = write(out, name, data)
        count += 1
        if path.endswith(".jp2"):
            jp2.append(path)
    for d in LOOSE_DIRS:
        for root, _, files in os.walk(os.path.join(src, d)):
            for f in files:
                if f.lower().endswith(".sig"):
                    continue
                rel = os.path.relpath(os.path.join(root, f), src)
                write(out, rel, open(os.path.join(root, f), "rb").read())
                count += 1
    for f in LOOSE_FILES:
        shutil.copy2(os.path.join(src, f), os.path.join(out, f.lower()))
        count += 1

    converted = 0
    if not args.no_png and jp2:
        from PIL import Image
        for path in jp2:
            png = path[:-4] + ".png"
            with Image.open(path) as im:
                im.save(png)
            converted += 1

    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files ({converted} JPEG 2000 images also as PNG) from {src} -> {out}")


if __name__ == "__main__":
    main()
