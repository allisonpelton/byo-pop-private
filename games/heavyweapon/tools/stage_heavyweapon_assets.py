#!/usr/bin/env python3
"""Copy the game data the Heavy Weapon port needs out of a legally owned Heavy Weapon Deluxe install.

The port never ships PopCap's assets; every build stages them from the user's own copy into the git-ignored
assets/heavyweapon/ folder (heavyweapon.exe finds it from build/native by itself).

    python games/heavyweapon/tools/stage_heavyweapon_assets.py ["A:/SteamLibrary/steamapps/common/Heavy Weapon Deluxe"]
        [--out assets/heavyweapon]

The game's files are loose (data/*.xml, Images, Sounds, Music, Fonts, properties/partner.xml). Names are staged in
lower case, so case-sensitive file systems (Emscripten MEMFS/IDBFS, Linux) see the names the game asks for. The
executable (Heavy Weapon Deluxe.exe, which holds the game's image) is copied for the game's texts only and never run
(src/GameText.h). Not copied: Flash.ocx, bass.dll, the DRM page (drm.swf, drm.xml, drmss.jpg), the readme, the
licence and *.sig files.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Heavy Weapon Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Heavy Weapon Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Heavy Weapon Deluxe",
]
FOLDERS = ["data", "fonts", "images", "music", "sounds", "properties"]
EXECUTABLES = ["heavy weapon deluxe.exe", "heavyweapon.exe"]
SKIP_SUFFIXES = (".sig",)
REQUIRED = ["data/craft.xml", "data/waves.xml", "images/atomictank.png", "music/atomictank.mo3",
            "properties/partner.xml"]


def find_install(explicit):
    for candidate in ([explicit] if explicit else DEFAULT_INSTALLS):
        if candidate and os.path.isfile(os.path.join(candidate, "data", "craft.xml")):
            return candidate
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("install", nargs="?", help="Heavy Weapon Deluxe install folder (contains data/craft.xml)")
    parser.add_argument("--out", default=os.path.join(ROOT, "assets", "heavyweapon"))
    args = parser.parse_args()
    source = find_install(args.install)
    if not source:
        sys.exit("Could not find a Heavy Weapon Deluxe install (data/craft.xml). Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    count = 0
    for entry in sorted(os.listdir(source)):
        top = os.path.join(source, entry)
        if os.path.isfile(top) and entry.lower() in EXECUTABLES:
            target = os.path.join(out, entry.lower())
            os.makedirs(out, exist_ok=True)
            with open(top, "rb") as source_file, open(target, "wb") as target_file:
                target_file.write(source_file.read())
            count += 1
            continue
        if not os.path.isdir(top) or entry.lower() not in FOLDERS:
            continue
        for folder, _dirs, files in os.walk(top):
            for name in files:
                if name.lower().endswith(SKIP_SUFFIXES):
                    continue
                path = os.path.join(folder, name)
                relative = os.path.relpath(path, source).replace("\\", "/").lower()
                target = os.path.join(out, relative)
                os.makedirs(os.path.dirname(target), exist_ok=True)
                with open(path, "rb") as source_file, open(target, "wb") as target_file:
                    target_file.write(source_file.read())
                count += 1
    missing = [name for name in REQUIRED if not os.path.isfile(os.path.join(out, name))]
    if not any(os.path.isfile(os.path.join(out, name)) for name in EXECUTABLES):
        missing.append(" or ".join(EXECUTABLES))
    if missing:
        sys.exit("Staged %d files but required files are missing: %s" % (count, missing))
    print("Staged %d files from %s -> %s" % (count, source, out))


if __name__ == "__main__":
    main()
