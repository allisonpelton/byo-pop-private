#!/usr/bin/env python3
"""Copy the game data the Venice port needs out of a legally owned Venice Deluxe install.

The port never ships PopCap's or Retro64's assets; every build stages them from the user's own copy into the
git-ignored assets/venice/ folder (venice.exe finds it from build/native by itself).

    python games/venice/tools/stage_venice_assets.py ["C:/Program Files (x86)/PopCap Games/Venice Deluxe"]
        [--out assets/venice]

main.pak (XOR 0xF7, tools/popcap_pak.py) is unpacked: the game reads its files exactly as it reads loose ones, and loose
files win over the pak's in the original too. Names are staged in lower case, so case-sensitive file systems
(Emscripten MEMFS/IDBFS, Linux) see the names the game asks for. The loose files are config.xml (the game's rules) and
properties/partner.xml (the distributor's settings). Not copied: the DRM wrapper (Venice.exe, drm/), bass.dll, the
uninstaller, the readme, the licence, install logs and *.sig files.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import popcap_pak  # noqa: E402

DEFAULT_INSTALLS = [
    r"C:\Program Files (x86)\PopCap Games\Venice Deluxe",
    r"A:\SteamLibrary\steamapps\common\Venice Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Venice Deluxe",
]
LOOSE_FILES = ["config.xml", "properties/partner.xml", "developer_logo.png"]
REQUIRED = ["config.xml", "properties/partner.xml", "levels/blank.xml", "gfx/font1.png", "audio/basket.ogg"]


def find_install(explicit):
    for candidate in ([explicit] if explicit else DEFAULT_INSTALLS):
        if candidate and os.path.isfile(os.path.join(candidate, "main.pak")):
            return candidate
    return None


def write(out, name, data):
    path = os.path.join(out, name.replace("\\", "/").lower())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("install", nargs="?", help="Venice Deluxe install folder (contains main.pak)")
    parser.add_argument("--out", default=os.path.join(ROOT, "assets", "venice"))
    args = parser.parse_args()
    source = find_install(args.install)
    if not source:
        sys.exit("Could not find a Venice Deluxe install (main.pak). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    pak = open(os.path.join(source, "main.pak"), "rb").read()
    count = 0
    for name, offset, size in popcap_pak.read_records(pak, 0xF7):
        write(out, name, bytes(byte ^ 0xF7 for byte in pak[offset:offset + size]))
        count += 1
    for name in LOOSE_FILES:
        path = os.path.join(source, name)
        if os.path.isfile(path):
            write(out, name, open(path, "rb").read())
            count += 1
    missing = [name for name in REQUIRED if not os.path.isfile(os.path.join(out, name))]
    if missing:
        sys.exit("Staged %d files but required files are missing: %s" % (count, missing))
    print("Staged %d files from %s -> %s" % (count, source, out))


if __name__ == "__main__":
    main()
