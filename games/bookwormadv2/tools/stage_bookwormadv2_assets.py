#!/usr/bin/env python3
"""Copy the game data the Bookworm Adventures Volume 2 port needs out of a legally owned Bookworm Adventures Volume 2 install.

The port never ships PopCap's assets (images, animations, scripts, fonts, sounds, music) or the game's texts; every
build stages them from the user's own copy into the git-ignored assets/bookwormadv2/ folder (the native exe finds it
next to itself or up to two levels up).

    python games/bookwormadv2/tools/stage_bookwormadv2_assets.py ["A:/SteamLibrary/steamapps/common/Bookworm Adventures Volume 2"]
        [--out assets/bookwormadv2]

main.pak (XOR 0xF7, tools/popcap_pak.py) is unpacked; the game reads its files exactly as it reads loose ones, and
loose files win over the pak's in the original too. Names are staged in lower case, so case-sensitive file systems
(Emscripten MEMFS/IDBFS, Linux) see the names the game asks for; the names whose case the game's files had go to
<out>/filecase.txt, which sexy.GetFilesInDirectory reads so that the scripts see the original's names ("Book4.luc").
--case-only writes that file alone. The game loads its 34 JPEG 2000 images (.jp2)
itself (platform/openjpeg); they are also decoded with Pillow into re/bookwormadv2/jp2ref/ (the same paths, as .png),
the reference the unit suite's jp2test compares that decoder with. PNG copies that earlier staging wrote next to them
are removed, so that the game reads the .jp2 files as in the browser.
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
    r"A:\SteamLibrary\steamapps\common\Bookworm Adventures Volume 2",
    r"C:\Program Files (x86)\Steam\steamapps\common\Bookworm Adventures Volume 2",
]
EXE = "BookwormAdventuresVol2.exe"

# Loose files the game reads besides main.pak. The exe is read for data only, never run. Not copied: drm/ (the
# DRM client's UI), Flash.ocx, bass.dll, j2k-codec.dll (the port has its own audio and image code), the
# uninstaller, readme, licence and install logs. images/upsell (the trailer) is the buy screen's and left out.
LOOSE_DIRS = ["images", "properties", "music", "scripts"]
LOOSE_FILES = [EXE]
REQUIRED = ["bookwormadventuresvol2.exe", "properties/resources.xml", "scripts/main.luc", "data/compressed.txt"]


def find_install(explicit):
    for c in ([explicit] if explicit else DEFAULT_INSTALLS):
        if c and os.path.isfile(os.path.join(c, "main.pak")):
            return c
    return None


CASES = []
CASE_ONLY = False


def write(out, rel, data):
    if rel != rel.lower():
        CASES.append(rel.replace("\\", "/"))
    path = os.path.join(out, rel.lower())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if not CASE_ONLY:
        with open(path, "wb") as f:
            f.write(data)
    return path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Bookworm Adventures Volume 2 install folder (contains main.pak)")
    ap.add_argument("--out", default=os.path.join(ROOT, "assets", "bookwormadv2"))
    ap.add_argument("--case-only", action="store_true", help="write out/filecase.txt only")
    args = ap.parse_args()
    global CASE_ONLY
    CASE_ONLY = args.case_only

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Bookworm Adventures Volume 2 install (main.pak). Pass its folder as an argument.")
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
                if rel.lower().replace("\\", "/").startswith("images/upsell/"):
                    continue
                write(out, rel, open(os.path.join(root, f), "rb").read())
                count += 1
    for f in LOOSE_FILES:
        shutil.copy2(os.path.join(src, f), os.path.join(out, f.lower()))
        count += 1

    converted = 0
    if jp2:
        from PIL import Image
        ref = os.path.join(ROOT, "re", "bookwormadv2", "jp2ref")
        for path in jp2:
            refpng = os.path.join(ref, os.path.relpath(path, out)[:-4] + ".png")
            os.makedirs(os.path.dirname(refpng), exist_ok=True)
            with Image.open(path) as im:
                im.save(refpng)
            converted += 1
            if os.path.isfile(path[:-4] + ".png"):
                os.remove(path[:-4] + ".png")

    with open(os.path.join(out, "filecase.txt"), "w", newline="") as f:
        f.write("".join(c + "\n" for c in sorted(set(CASES))))
    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files ({converted} JPEG 2000 references in re/bookwormadv2/jp2ref) from {src} -> {out}")


if __name__ == "__main__":
    main()
