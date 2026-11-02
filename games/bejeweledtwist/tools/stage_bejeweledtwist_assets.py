#!/usr/bin/env python3
"""Copy the game data the Bejeweled Twist port needs out of a legally owned Bejeweled Twist install.

The port never ships PopCap's assets (images, meshes, fonts, sounds, music) or the game's texts; every build stages
them from the user's own copy into the git-ignored assets/bejeweledtwist/ folder (the native exe finds it next to
itself or up to two levels up).

    python games/bejeweledtwist/tools/stage_bejeweledtwist_assets.py ["C:/Program Files (x86)/PopCap Games/Bejeweled Twist"]
        [--out assets/bejeweledtwist]

main.pak (XOR 0xF7, tools/popcap_pak.py) is unpacked; the game reads its files exactly as it reads loose ones, and
loose files win over the pak's in the original too. Names are staged in lower case, so case-sensitive file systems
(Emscripten MEMFS/IDBFS, Linux) see the names the game asks for. The game's 2,364 JPEG 2000 images stay .jp2 and are
decoded at run time (platform/openjpeg). The loose files are the 17 .p3d meshes, music/ and properties/.

BejeweledTwist.exe is read for data only (the game's texts, by address), never run. The retail copy's exe is PopCap's
CDProtect launcher with the game appended; the Steam copy's is the 2008 DRM wrapper. Both inner images are carved out
(tools/extract_game_exe.py) and staged as bejeweledtwist.exe. Not copied: compat.cfg (FEAST's GPU rules, which the
port does not need), bass.dll, j2k-codec.dll, the uninstaller, readme, licence, install logs and *.sig files.

properties/ comes from the Steam copy when there is one, whatever install the rest comes from: the port matches the
copy the user plays (docs/PORT_PLAN.md, "Which copy's files"). The retail resources.xml names the 47 Zen visualizer
drops by paths main.pak does not have (images/1200/NonResize/...), so the retail game quits when it first needs one;
Steam's resources.xml names them where they are.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import popcap_pak  # noqa: E402
import extract_game_exe  # noqa: E402

DEFAULT_INSTALLS = [
    r"C:\Program Files (x86)\PopCap Games\Bejeweled Twist",
    r"A:\SteamLibrary\steamapps\common\Bejeweled Twist",
    r"C:\Program Files (x86)\Steam\steamapps\common\Bejeweled Twist",
]
EXE = "BejeweledTwist.exe"
STEAM_INSTALLS = DEFAULT_INSTALLS[1:]
LOOSE_DIRS = ["properties", "music"]
REQUIRED = ["bejeweledtwist.exe", "properties/resources.xml", "properties/config.xml", "crystal1.p3d",
            "music/bejeweledtwist24.mo3"]


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


def inner_exe(data):
    """The game's own PE image: carved out of a wrapper or launcher, or the file itself if it is not wrapped."""
    overlay = extract_game_exe.pe_end(data, 0)
    if not any(data[overlay:overlay + len(m)] == m for m in extract_game_exe.MARKERS):
        return data
    start = data.find(b"MZ", overlay)
    return data[start:start + extract_game_exe.pe_end(data, start)]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Bejeweled Twist install folder (contains main.pak)")
    ap.add_argument("--out", default=os.path.join(ROOT, "assets", "bejeweledtwist"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Bejeweled Twist install (main.pak). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)

    pak = open(os.path.join(src, "main.pak"), "rb").read()
    count = 0
    for name, offset, size in popcap_pak.read_records(pak, 0xF7):
        write(out, name, bytes(x ^ 0xF7 for x in pak[offset:offset + size]))
        count += 1
    props = next((c for c in STEAM_INSTALLS if os.path.isfile(os.path.join(c, "properties", "resources.xml"))), src)
    for d in LOOSE_DIRS:
        base = props if d == "properties" else src
        for root, _, files in os.walk(os.path.join(base, d)):
            for f in files:
                if f.lower().endswith(".sig"):
                    continue
                rel = os.path.relpath(os.path.join(root, f), base)
                write(out, rel, open(os.path.join(root, f), "rb").read())
                count += 1
    for f in os.listdir(src):
        if f.lower().endswith(".p3d"):
            write(out, f, open(os.path.join(src, f), "rb").read())
            count += 1
    write(out, EXE, inner_exe(open(os.path.join(src, EXE), "rb").read()))
    count += 1

    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files from {src} (properties from {props}) -> {out}")


if __name__ == "__main__":
    main()
