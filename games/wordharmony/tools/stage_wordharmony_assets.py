#!/usr/bin/env python3
"""Copy the game data the Word Harmony port needs out of a legally owned Word Harmony install.

The port never ships PopCap's or Sprout's data: WHArchive.saf holds the pictures, fonts, screen layouts, levels and the
word lists, and res/ the sounds, the music's samples and the splash. Every build stages them from the user's own copy
into the git-ignored assets/wordharmony/ folder (the native exe finds it next to itself or up to two levels up).

    python games/wordharmony/tools/stage_wordharmony_assets.py ["C:/Program Files/PopCap Games/Word Harmony"] [--out assets/wordharmony]
"""
import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main_checkout():
    """The main checkout of the repository (a worktree has no re/_installers of its own)."""
    try:
        common = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=HERE, text=True).strip()
        return os.path.dirname(os.path.abspath(os.path.join(HERE, common)))
    except (OSError, subprocess.CalledProcessError):
        return os.path.join(HERE, "..", "..", "..")


DEFAULT_INSTALLS = [
    r"C:\Program Files\PopCap Games\Word Harmony",
    r"C:\Program Files (x86)\PopCap Games\Word Harmony",
    os.path.join(HERE, "..", "..", "..", "re", "_installers", "WordHarmonySetup", "files", "WordHarmony"),
    os.path.join(main_checkout(), "re", "_installers", "WordHarmonySetup", "files", "WordHarmony"),
]

# The archive, and the loose folders the game reads next to it. Not copied: the DRM wrapper and its Flash files, the
# uninstaller, the readme and license, partner.xml and Windows' Thumbs.db files.
ARCHIVE = "WHArchive.saf"
FOLDERS = ["res/sounds", "res/music", "res/splash", "res/buttons"]
SKIP = {"thumbs.db"}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, ARCHIVE)):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Word Harmony install folder (WHArchive.saf, res/)")
    ap.add_argument("--out", default=os.path.join(HERE, "..", "..", "..", "assets", "wordharmony"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Word Harmony install (WHArchive.saf). Pass its folder.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    shutil.copy2(os.path.join(src, ARCHIVE), os.path.join(out, ARCHIVE))
    count = 1
    for folder in FOLDERS:
        sdir = os.path.join(src, folder)
        if not os.path.isdir(sdir):
            continue
        ddir = os.path.join(out, folder)
        os.makedirs(ddir, exist_ok=True)
        for name in sorted(os.listdir(sdir)):
            if name.lower() in SKIP or not os.path.isfile(os.path.join(sdir, name)):
                continue
            shutil.copy2(os.path.join(sdir, name), os.path.join(ddir, name))
            count += 1
    print(f"staged {count} files from {os.path.abspath(src)} into {out}")


if __name__ == "__main__":
    main()
