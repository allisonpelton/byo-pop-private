#!/usr/bin/env python3
"""Write the word list of the mod "bwa2-words" (games/bookworm/mods/bwa2-words) from the player's own Bookworm
Adventures Volume 2. The list is not ours to commit or redistribute: the file written is git-ignored
(mods/bwa2-words/.gitignore), and the mod does nothing without it.

The words are Volume 2's dictionary, data/compressed.txt in its main.pak (tools/popcap_pak.py reads the pak):
wordlist.txt's prefix-compressed format, in lower case. Volume 2 has no definitions (data/compressed_attribs.dat
holds a small number per word, which the game uses for its own purposes and this mod does not need).

Kept: words of 3 or more letters A-Z (BookWorm skips shorter ones), but not those with a Q that no U follows:
BookWorm's Q tile always spells "QU". Written: bwa2-words.txt, one word a line, upper case. Its words of more
than 12 letters count in the game only with the mod "long-words".

    python games/bookworm/tools/make_bwa2_mod.py [<Volume 2's folder | main.pak>] [--mod-dir games/bookworm/mods/bwa2-words]

Without a path, the default Steam folder is tried.
"""
import argparse
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import popcap_pak  # noqa: E402

MOD_DIR = os.path.join(ROOT, "games", "bookworm", "mods", "bwa2-words")
DEFAULT_INSTALL = r"C:\Program Files (x86)\Steam\steamapps\common\Bookworm Adventures Volume 2"
WORDS_ENTRY = "data/compressed.txt"
WORD_RE = re.compile(r"^[A-Z]{3,}$")


def read_entry(pak_path, entry, key=0xF7):
    with open(pak_path, "rb") as f:
        data = f.read()
    for name, offset, size in popcap_pak.read_records(data, key):
        if name.lower() == entry:
            return bytes(x ^ key for x in data[offset:offset + size])
    raise SystemExit(f"{pak_path} has no {entry}: is it Bookworm Adventures Volume 2's main.pak?")


def decompress(text):
    """wordlist.txt's format (BookWormApp::LoadWordList): a number is how many leading letters of the previous
    word the next word keeps, in force for the following lines until the next number"""
    words = []
    word, prev = [], []
    prefix, in_number = 0, False
    for c in text:
        if c.isdigit():
            if not in_number:
                prefix, in_number = 0, True
            prefix = prefix * 10 + int(c)
        elif c in "\r\n":
            if word:
                prev = word + prev[len(word):]
                words.append("".join(word))
                word = []
            in_number = False
        else:
            if prefix > 0 and not word:
                word = prev[:prefix]
            word.append(c)
    return words


def playable(word):
    return WORD_RE.match(word) is not None and all(word[i + 1:i + 2] == "U" for i, c in enumerate(word) if c == "Q")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("source", nargs="?", default=DEFAULT_INSTALL, help="Volume 2's folder or its main.pak")
    ap.add_argument("--mod-dir", default=MOD_DIR)
    args = ap.parse_args()

    pak = os.path.join(args.source, "main.pak") if os.path.isdir(args.source) else args.source
    if not os.path.isfile(pak):
        raise SystemExit(f"no main.pak at {pak}: give Bookworm Adventures Volume 2's folder")
    words = decompress(read_entry(pak, WORDS_ENTRY).decode("latin-1"))
    kept = sorted(set(w.upper() for w in words if playable(w.upper())))
    if not kept:
        raise SystemExit("no words read")

    os.makedirs(args.mod_dir, exist_ok=True)
    out = os.path.join(args.mod_dir, "bwa2-words.txt")
    with open(out, "w", newline="\n") as f:
        f.write("\n".join(kept) + "\n")
    longer = sum(1 for w in kept if len(w) > 12)
    print(f"{out}: {len(kept)} words of {len(words)} ({longer} longer than 12 letters, the longest "
          f"{max(len(w) for w in kept)})")


if __name__ == "__main__":
    main()
