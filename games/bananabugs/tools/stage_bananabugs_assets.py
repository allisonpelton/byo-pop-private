#!/usr/bin/env python3
"""Copy the game data the Banana Bugs port needs out of a legally owned Banana Bugs install.

The port never ships PopCap's assets; every build stages them from the user's own copy into the git-ignored
assets/bananabugs/ folder (bananabugs.exe finds it from build/native by itself).

    python -I games/bananabugs/tools/stage_bananabugs_assets.py ["C:/Program Files (x86)/PopCap Games/Banana Bugs"]
        [--out assets/bananabugs]

Main.pak (XOR 0xF7, tools/popcap_pak.py) is unpacked: the game reads its files exactly as it reads loose ones, and
loose files win over the pak's in the original too. The pak holds the pictures and one font; the sounds and music
(`sounds/*.ogg`) and the resource manifest (`properties/resources.xml`, the sounds only) are loose. Names are staged
in lower case, so a case-sensitive file system (Emscripten's MEMFS and IDBFS, Linux) sees the names the game asks for.

Not copied: the 2010 DRM launcher (`BananaBugs.exe`, the encrypted `BananaBugs.dat`, `drm/`, `DynamicContentLib.dll`,
`XLayout.dll`, the `*.xml.bin` files and their signatures), `bass.dll`, the JPEG 2000 codec DLL (`j2k-codec.dll`; the
port decodes with OpenJPEG), the uninstaller, the readme, the licence, install and crash logs, a dgVoodoo set-up the
user may have put beside the game, and the sound editor's peak files (`*.sfk`, `*.sfl`) left in `sounds/`.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import popcap_pak  # noqa: E402

DEFAULT_INSTALLS = [
    r"C:\Program Files (x86)\PopCap Games\Banana Bugs",
    r"C:\Program Files\PopCap Games\Banana Bugs",
]
PAK_XOR = 0xF7
LOOSE_FILES = ["properties/resources.xml", "properties/partner.xml"]
LOOSE_FOLDERS = {"sounds": (".ogg",)}
REQUIRED = ["properties/resources.xml", "sounds/mainmenu.ogg", "sounds/music1.ogg",
            "fonts/bradybunchremastered52.txt"]


def find_install(explicit):
    for candidate in ([explicit] if explicit else DEFAULT_INSTALLS):
        if candidate and os.path.isfile(os.path.join(candidate, "Main.pak")):
            return candidate
    return None


def write(out, name, data):
    path = os.path.join(out, name.replace("\\", "/").lower())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("install", nargs="?", help="Banana Bugs install folder (contains Main.pak)")
    parser.add_argument("--out", default=os.path.join(ROOT, "assets", "bananabugs"))
    args = parser.parse_args()
    source = find_install(args.install)
    if not source:
        sys.exit("Could not find a Banana Bugs install (Main.pak). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(source, "Main.pak"), "rb") as handle:
        pak = handle.read()
    count = 0
    for name, offset, size in popcap_pak.read_records(pak, PAK_XOR):
        write(out, name, bytes(byte ^ PAK_XOR for byte in pak[offset:offset + size]))
        count += 1
    for name in LOOSE_FILES:
        path = os.path.join(source, name)
        if os.path.isfile(path):
            with open(path, "rb") as handle:
                write(out, name, handle.read())
            count += 1
    for folder, extensions in LOOSE_FOLDERS.items():
        for leaf in sorted(os.listdir(os.path.join(source, folder))):
            if os.path.splitext(leaf)[1].lower() not in extensions:
                continue
            with open(os.path.join(source, folder, leaf), "rb") as handle:
                write(out, folder + "/" + leaf, handle.read())
            count += 1
    missing = [name for name in REQUIRED if not os.path.isfile(os.path.join(out, name.lower()))]
    if missing:
        sys.exit("Staged %d files but required files are missing: %s" % (count, missing))
    print("Staged %d files from %s -> %s" % (count, source, out))


if __name__ == "__main__":
    main()
