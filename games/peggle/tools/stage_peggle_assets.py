#!/usr/bin/env python3
"""Copy the game data the Peggle Deluxe port needs out of a legally owned Peggle Deluxe install.

The port never ships PopCap's assets; every build stages them from the user's own copy into the git-ignored
assets/peggle/ folder (peggle.exe finds it from build/native by itself).

    python games/peggle/tools/stage_peggle_assets.py ["A:/SteamLibrary/steamapps/common/Peggle Deluxe"]
        [--out assets/peggle]

main.pak (XOR 0xF7, tools/popcap_pak.py) is unpacked: the game reads its files exactly as it reads loose ones, and loose
files win over the pak's in the original too. The names are staged in lower case, so a case-sensitive file system
(Emscripten's MEMFS and IDBFS, Linux) sees the names the game asks for. Loose files are the music (`music/`) and the
distributor's settings (`properties/`).

A Steam copy that a theme was installed into keeps the stock pak beside the themed one as `main.pak.original`
(`theme-installed.txt` says so); that one is staged, so the port starts from the original's own data. Not copied: the
DRM wrapper (`Peggle.exe`, `drm/`), `bass.dll`, the JPEG 2000 codec DLL (`j2k-codec.dll`; the port decodes with
OpenJPEG), the readme, the licence, crash logs and `*.sig` files.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import popcap_pak  # noqa: E402

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Peggle Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Peggle Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Peggle Deluxe",
]
LOOSE_FILES = [
    "music/PeggleBeats.mo3",
    "music/odetojoy.ogg",
    "music/opening.ogg",
    "properties/partner.xml",
    "properties/partner_logo.jpg",
]
REQUIRED = ["properties/resources.xml", "music/peggleBeats.mo3", "levels/level1.dat", "levels/stages.cfg",
            "characters/characters.cfg", "images/levels/bumpera.jp2"]


def find_install(explicit):
    for candidate in ([explicit] if explicit else DEFAULT_INSTALLS):
        if candidate and (os.path.isfile(os.path.join(candidate, "main.pak"))
                          or os.path.isfile(os.path.join(candidate, "main.pak.original"))):
            return candidate
    return None


def write(out, name, data):
    path = os.path.join(out, name.replace("\\", "/").lower())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("install", nargs="?", help="Peggle Deluxe install folder (contains main.pak)")
    parser.add_argument("--out", default=os.path.join(ROOT, "assets", "peggle"))
    args = parser.parse_args()
    source = find_install(args.install)
    if not source:
        sys.exit("Could not find a Peggle Deluxe install (main.pak). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    pak_path = os.path.join(source, "main.pak.original")
    if not os.path.isfile(pak_path):
        pak_path = os.path.join(source, "main.pak")
    with open(pak_path, "rb") as handle:
        pak = handle.read()
    count = 0
    for name, offset, size in popcap_pak.read_records(pak, 0xF7):
        write(out, name, bytes(byte ^ 0xF7 for byte in pak[offset:offset + size]))
        count += 1
    for name in LOOSE_FILES:
        path = os.path.join(source, name)
        if os.path.isfile(path):
            with open(path, "rb") as handle:
                write(out, name, handle.read())
            count += 1
    missing = [name for name in REQUIRED if not os.path.isfile(os.path.join(out, name.lower()))]
    if missing:
        sys.exit("Staged %d files but required files are missing: %s" % (count, missing))
    print("Staged %d files from %s (%s) -> %s" % (count, source, os.path.basename(pak_path), out))


if __name__ == "__main__":
    main()
