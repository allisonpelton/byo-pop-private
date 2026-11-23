#!/usr/bin/env python3
"""Copy the game data the Talismania port needs out of a legally owned Talismania Deluxe install.

The port never ships PopCap's or Sprout's data: Talismania.saf holds the pictures, fonts, screen layouts, stages, animations
and sounds, music/ the soundtrack, splash/ the splash screen and properties/ the publisher's settings. Every build
stages them from the user's own copy into the git-ignored assets/talismania/ folder (the native exe finds it next
to itself or up to two levels up). The install is only read.

    python -I games/talismania/tools/stage_talismania_assets.py ["<install>"] [--out assets/talismania]
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

DEFAULT_INSTALLS = [
    r"A:\SteamLibrary\steamapps\common\Talismania Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Talismania Deluxe",
    r"C:\Program Files\PopCap Games\Talismania Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Talismania Deluxe",
]

# The archive and the loose folders the game reads next to it. Not copied: the launcher and its DRM screens (drm/,
# drm.swf, drm.xml, drmss.jpg), Flash.ocx, bass.dll, the readme and the license.
ARCHIVE = "Talismania.saf"
FOLDERS = ["music", "splash", "properties"]
SKIP = {"thumbs.db"}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for candidate in candidates:
        if candidate and os.path.isfile(os.path.join(candidate, ARCHIVE)):
            return candidate
    return None


def copy_tree(source, destination):
    count = 0
    os.makedirs(destination, exist_ok=True)
    for name in sorted(os.listdir(source)):
        if name.lower() in SKIP:
            continue
        path = os.path.join(source, name)
        if os.path.isdir(path):
            count += copy_tree(path, os.path.join(destination, name))
        else:
            shutil.copy2(path, os.path.join(destination, name))
            count += 1
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("install", nargs="?", help="Talismania Deluxe install folder (Talismania.saf, music/)")
    parser.add_argument("--out", default=os.path.join(HERE, "..", "..", "..", "assets", "talismania"))
    args = parser.parse_args()

    source = find_install(args.install)
    if not source:
        sys.exit("Could not find a Talismania Deluxe install (Talismania.saf). Pass its folder.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    shutil.copy2(os.path.join(source, ARCHIVE), os.path.join(out, ARCHIVE))
    count = 1
    for folder in FOLDERS:
        folder_path = os.path.join(source, folder)
        if os.path.isdir(folder_path):
            count += copy_tree(folder_path, os.path.join(out, folder))
    print(f"staged {count} files from {os.path.abspath(source)} into {out}")


if __name__ == "__main__":
    main()
