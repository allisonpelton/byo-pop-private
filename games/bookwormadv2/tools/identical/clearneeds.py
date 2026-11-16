#!/usr/bin/env python3
"""clearneeds.py <profile dir>: sets the player's Needs*Dialog flags false (a bool is key, type 2, one byte).

A profile saved at a map after a book's boss still holds NeedsBestiaryDialog (and the like): the game shows the unlock panel at
the main menu and clears the flags then. A comparison case that starts from such a profile would meet the panel before
Adventure, which its script's clicks do not answer, so the case starts from the profile as the game leaves it once the panel
has been shown."""
import glob, os, re, struct, sys

for path in glob.glob(os.path.join(sys.argv[1], "users", "*.bwa")):
    data = bytearray(open(path, "rb").read())
    for key in (b"NeedsBestiaryDialog", b"NeedsMiniGamesDialog", b"NeedsArenaDialog", b"NeedsMPDialog"):
        k = struct.pack("<H", len(key)) + key + struct.pack("<I", 2)
        i = bytes(data).find(k)
        if i >= 0:
            data[i + len(k)] = 0
    open(path, "wb").write(bytes(data))
