#!/usr/bin/env python3
"""Copy the game data the Bejeweled 3 port needs out of a legally owned Bejeweled 3 install.

port: a staging tool. The port never ships PopCap's files (pictures, meshes, effects, fonts, sounds, music) or the
game's texts; every build stages them from the user's own copy into the git-ignored assets/bejeweled3/ folder.

    python games/bejeweled3/tools/stage_bejeweled3_assets.py ["A:/SteamLibrary/steamapps/common/Bejeweled 3"]
        [--out assets/bejeweled3]

main.pak (XOR 0xF7, tools/popcap_pak.py) is unpacked; the original reads its files through PakInterface exactly as
it reads loose ones, and a loose file wins over the pak's. Names are staged in lower case, so case-sensitive file
systems (Emscripten's, Linux) find the names the game asks for in any case. The JPEG 2000 pictures stay .jp2 and
are decoded at run time (platform/openjpeg). The loose folders are properties/ (partner.xml), binaural/ (Zen mode's
SBaGen programs), ambient/ (Zen's ambient loops) and affirmations/ (Zen's texts).

Bejeweled3.exe is not wrapped. It is staged for data only (the game's texts and tables, by address), never run by
the port. Not copied: bass.dll, j2k-codec.dll, steam_api.dll, compat.cfg (FEAST's rules for graphics cards), the
readme, the licence and the *.sig files.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import popcap_pak  # noqa: E402

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Bejeweled 3",
    r"C:\Program Files (x86)\Steam\steamapps\common\Bejeweled 3",
    r"C:\Program Files (x86)\PopCap Games\Bejeweled 3",
]
EXE = "Bejeweled3.exe"
LOOSE_DIRS = ["properties", "binaural", "ambient", "affirmations"]
PAK_KEY = 0xF7
REQUIRED = ["bejeweled3.exe", "properties/resources.xml", "properties/default.xml", "board.p3d"]


def find_install(explicit):
    for candidate in ([explicit] if explicit else DEFAULT_INSTALLS):
        if candidate and os.path.isfile(os.path.join(candidate, "main.pak")):
            return candidate
    return None


def write(out, rel, data):
    path = os.path.join(out, rel.replace("\\", "/").lower())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)
    return path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Bejeweled 3 install folder (holds main.pak)")
    ap.add_argument("--out", default=os.path.join(ROOT, "assets", "bejeweled3"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Bejeweled 3 install (main.pak). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)

    with open(os.path.join(src, "main.pak"), "rb") as f:
        pak = f.read()
    count = 0
    for name, offset, size in popcap_pak.read_records(pak, PAK_KEY):
        write(out, name, bytes(x ^ PAK_KEY for x in pak[offset:offset + size]))
        count += 1
    for folder in LOOSE_DIRS:
        for root, _, files in os.walk(os.path.join(src, folder)):
            for leaf in files:
                if leaf.lower().endswith(".sig"):
                    continue
                path = os.path.join(root, leaf)
                with open(path, "rb") as f:
                    write(out, os.path.relpath(path, src), f.read())
                count += 1
    with open(os.path.join(src, EXE), "rb") as f:
        write(out, EXE, f.read())
    count += 1

    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files from {src} -> {out}")


if __name__ == "__main__":
    main()
