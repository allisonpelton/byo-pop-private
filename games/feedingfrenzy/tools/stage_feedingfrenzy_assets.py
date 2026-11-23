#!/usr/bin/env python3
"""Copy the game data the Feeding Frenzy port needs out of a legally owned Feeding Frenzy Deluxe install.

The port never ships PopCap's or Sprout's data: FFArchive.saf holds the pictures, fonts, screen layouts, stages and
fish, resources/ the loose sounds and music, properties/ the publisher's settings, and the port reads the game's texts
(the texts in its code and its string table) from the player's FeedingFrenzy.exe, which it never runs. Every build
stages them from the user's own copy into the git-ignored assets/feedingfrenzy/ folder (the native exe finds it next to
itself or up to two levels up). The install is only read.

    python -I games/feedingfrenzy/tools/stage_feedingfrenzy_assets.py ["<install>"] [--out assets/feedingfrenzy]
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

DEFAULT_INSTALLS = [
    r"C:\Program Files (x86)\Popcap Game Collection\Feeding Frenzy Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Feeding Frenzy",
    r"C:\Program Files\PopCap Games\Feeding Frenzy",
]

# The archive, the exe (read for its texts) and the loose files the game reads next to them. Not copied: the screen
# saver, its installer, the readme and the license.
ARCHIVE = "FFArchive.saf"
FILES = ["FeedingFrenzy.exe"]
FOLDERS = ["resources", "properties"]
SKIP = {"thumbs.db"}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, ARCHIVE)):
            return c
    return None


def copy_tree(src, dst):
    """Every file under src, at the same place under dst; the number copied."""
    count = 0
    for root, _, names in os.walk(src):
        rel = os.path.relpath(root, src)
        os.makedirs(os.path.join(dst, rel), exist_ok=True)
        for name in sorted(names):
            if name.lower() in SKIP:
                continue
            shutil.copy2(os.path.join(root, name), os.path.join(dst, rel, name))
            count += 1
    return count


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Feeding Frenzy install folder (FFArchive.saf, resources/)")
    ap.add_argument("--out", default=os.path.join(HERE, "..", "..", "..", "assets", "feedingfrenzy"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Feeding Frenzy install (FFArchive.saf). Pass its folder.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    shutil.copy2(os.path.join(src, ARCHIVE), os.path.join(out, ARCHIVE))
    count = 1
    for name in FILES:
        if os.path.isfile(os.path.join(src, name)):
            shutil.copy2(os.path.join(src, name), os.path.join(out, name))
            count += 1
    for folder in FOLDERS:
        if os.path.isdir(os.path.join(src, folder)):
            count += copy_tree(os.path.join(src, folder), os.path.join(out, folder))
    print(f"staged {count} files from {os.path.abspath(src)} into {out}")


if __name__ == "__main__":
    main()
