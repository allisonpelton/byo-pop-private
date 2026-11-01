#!/usr/bin/env python3
"""Copy the game data the Bonnie's Bookstore port needs out of a legally owned Bonnie's Bookstore Deluxe install.

The port never ships PopCap's assets (images, sounds, music, the pak, the dictionary and the texts) or the data inside
the exe (read at start-up from the user's own BonniesBookstore.exe); every build stages them from the user's own copy
into the git-ignored assets/bonnies/ folder (the native exe finds it next to itself or up to two levels up).

    python games/bonnies/tools/stage_bonnies_assets.py ["<Bonnie's Bookstore Deluxe folder>"] [--out assets/bonnies]
"""
import argparse
import os
import shutil
import sys

DEFAULT_INSTALLS = [
    r"C:\Program Files\PopCap Games\Bonnie's Bookstore Deluxe",
    r"C:\Program Files (x86)\PopCap Games\Bonnie's Bookstore Deluxe",
    r"A:\SteamLibrary\steamapps\common\Bonnie's Bookstore Deluxe",
    r"C:\Program Files (x86)\Steam\steamapps\common\Bonnie's Bookstore Deluxe",
]

# What the game reads at run time. BonniesBookstore.exe is read for the game's data only, never run.
# SavedGames/General_Preferences.cfg is the preferences the install comes with (the game reads the player's own first).
# Not copied: the DRM wrapper's files (drm/), the DLLs (SDL, SDL_image, FMOD and their libraries: the port has its own),
# the uninstaller, properties/ (the distributor's settings, which the game does not read), the readme and licence.
DIRS = ["Data", "Graphics", "Sound"]
FILES = ["BonniesBookstore.exe", "SavedGames/General_Preferences.cfg"]
REQUIRED = ["BonniesBookstore.exe", "Data/Bonnie.pak", "Data/Language.txt", "Data/Dictionary.txt", "Sound/music.oxm"]
SKIP = {"thumbs.db"}


def find_install(explicit):
    candidates = [explicit] if explicit else DEFAULT_INSTALLS
    for c in candidates:
        if c and os.path.isfile(os.path.join(c, "BonniesBookstore.exe")) and \
                os.path.isfile(os.path.join(c, "Data", "Bonnie.pak")):
            return c
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("install", nargs="?", help="Bonnie's Bookstore Deluxe install folder (contains BonniesBookstore.exe)")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..",
                                                  "assets", "bonnies"))
    args = ap.parse_args()

    src = find_install(args.install)
    if not src:
        sys.exit("Could not find a Bonnie's Bookstore Deluxe install (BonniesBookstore.exe, Data/Bonnie.pak). "
                 "Pass its folder as an argument.")
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)

    # The names keep their case: the port looks every path up case-insensitively (Platform::VFS)
    count = 0
    for d in DIRS:
        sd = os.path.join(src, d)
        if not os.path.isdir(sd):
            print(f"warning: {d} missing in {src}")
            continue
        for root, _, files in os.walk(sd):
            rel = os.path.join(d, os.path.relpath(root, sd))
            os.makedirs(os.path.join(out, rel), exist_ok=True)
            for f in files:
                if f.lower() in SKIP:
                    continue
                shutil.copy2(os.path.join(root, f), os.path.join(out, rel, f))
                count += 1
    for f in FILES:
        p = os.path.join(src, f)
        if os.path.isfile(p):
            os.makedirs(os.path.dirname(os.path.join(out, f)), exist_ok=True)
            shutil.copy2(p, os.path.join(out, f))
            count += 1

    missing = [r for r in REQUIRED if not os.path.isfile(os.path.join(out, r))]
    if missing:
        sys.exit(f"Staged {count} files but required files are missing: {missing}")
    print(f"Staged {count} files from {src} -> {out}")


if __name__ == "__main__":
    main()
