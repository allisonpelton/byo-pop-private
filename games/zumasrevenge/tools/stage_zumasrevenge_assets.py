#!/usr/bin/env python3
"""Copy the game data the Zuma's Revenge port needs out of a legally owned Zuma's Revenge install.

port: a staging tool. The port never ships PopCap's files (pictures, animations, effects, fonts, sounds, music,
levels) or the game's texts; every build stages them from the user's own copy into the git-ignored
assets/zumasrevenge/ folder.

    python games/zumasrevenge/tools/stage_zumasrevenge_assets.py ["A:/SteamLibrary/steamapps/common/Zuma's Revenge"]
        [--out assets/zumasrevenge]

main.pak (XOR 0xF7, tools/popcap_pak.py) is unpacked; the original reads its files through PakInterface exactly as
it reads loose ones, and a loose file wins over the pak's. Names are staged in lower case, so case-sensitive file
systems (Emscripten's, Linux) find the names the game asks for in any case. The JPEG 2000 pictures stay .jp2 and
are decoded at run time (platform/openjpeg). The loose folders are levels/ (the curves, "CURV" .dat files) and
properties/ (partner.xml).

The Steam copy's ZumasRevenge.exe is a wrapper; the game inside it is re/zumasrevenge/bin/ZumasRevengeGame.exe, which
is staged for data only (the game's texts and tables, by address), never run by the port. Not copied: the wrapper and
its drm/ folder, bass.dll, j2k-codec.dll, steam_api.dll, Steam.dll, dbghelp.dll, PopInstallStub.exe, compat.cfg
(FEAST's rules for graphics cards), crash.txt, the readme, the licence and the *.sig files.
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
    r"A:\SteamLibrary\steamapps\common\Zuma's Revenge",
    r"C:\Program Files (x86)\Steam\steamapps\common\Zuma's Revenge",
]
GAME_EXE = os.path.join(ROOT, "re", "zumasrevenge", "bin", "ZumasRevengeGame.exe")
LOOSE_DIRS = ["levels", "properties"]
PAK_KEY = 0xF7
REQUIRED = ["zumasrevengegame.exe", "properties/resources.xml", "properties/partner.xml"]


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
    ap.add_argument("install", nargs="?", help="Zuma's Revenge install folder (holds main.pak)")
    ap.add_argument("--out", default=os.path.join(ROOT, "assets", "zumasrevenge"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Zuma's Revenge install (main.pak). Pass its folder as an argument.")
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
    if os.path.isfile(GAME_EXE):
        shutil.copyfile(GAME_EXE, os.path.join(out, "zumasrevengegame.exe"))
        count += 1
    else:
        print("note: %s is missing; the game exe is not staged" % GAME_EXE)

    missing = [rel for rel in REQUIRED if not os.path.isfile(os.path.join(out, rel))]
    print("staged %d files into %s" % (count, out))
    if missing:
        sys.exit("missing after staging: " + ", ".join(missing))


if __name__ == "__main__":
    main()
