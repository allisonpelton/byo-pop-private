#!/usr/bin/env python3
"""Write the word list and definitions of the mod "collins" (games/bookworm/mods/collins) from the player's own
copy of Collins Scrabble Words. Neither is ours to commit or redistribute: the files written are git-ignored
(mods/collins/.gitignore), and the mod does nothing without them.

The words come from one of:
  - a NASPA Zyzzyva folder (installed, or the installer's files unpacked): its data/words/<LEXICON>/ word graph
    (<hash>-<LEXICON>.dwg) is read;
  - a Zyzzyva word graph (.dwg) itself;
  - a text file: a word a line, or a word, a tab or space and its definition.
The definitions come from a text file of the last kind, the words' own or --definitions (an older edition's
definitions serve the words both have). In a text file only words in capitals (or all lower case) count, so
a title line is skipped, and so are lines starting with '#'.

Zyzzyva's .dwg is a plain DAWG of little-endian 32-bit words: word 0 is the node count, the nodes follow from
word 1 (the root's first letter), each with its letter in bits 24-31, bit 23 set where a word ends, bit 22 on a
node's last sibling and its first child's index in bits 0-21 (0: none). Its definitions are in a separate
encrypted file, which this does not read.

Definitions in the Collins lists' style (senses split by " / ", each "(label) gloss [pos inflections]", an
inflected form's "BASE, gloss [pos]") are written in popdefs.txt's: "(n.) Gloss.", "(n., French) Gloss.",
"(v.) Form of BASE: gloss.", the senses one after another. Others are kept as they are.

Kept: words of 3 or more letters A-Z (the game skips shorter ones), but not those with a Q that no U follows:
BookWorm's Q tile always spells "QU". Written: collins-words.txt (one word a line, upper case) and, with
definitions, collins-defs.txt (the word, a space, the definition: popdefs.txt's format).

    python games/bookworm/tools/make_collins_mod.py <Zyzzyva folder | .dwg | words.txt> [--definitions defs.txt]
        [--lexicon CSW24] [--mod-dir games/bookworm/mods/collins]
"""
import argparse
import glob
import os
import re
import struct
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
MOD_DIR = os.path.join(ROOT, "games", "bookworm", "mods", "collins")
WORD_RE = re.compile(r"^[A-Z]{3,}$")
TEXT_WORD_RE = re.compile(r"^([A-Z]+|[a-z]+)$")
SENSE_RE = re.compile(r"^(?:\(([^)]*)\) )?([^\[]*?)\s*(?:\[([a-z]+)[^\]]*\])?\.?$")
FORM_RE = re.compile(r"^([A-Z]{2,}), (.*)$")
POS = {"n": "n.", "v": "v.", "adj": "adj.", "adv": "adv.", "interj": "interj.", "int": "interj.",
       "prep": "prep.", "pron": "pron.", "conj": "conj."}
# a label that reads into the gloss ("short for" ...) rather than naming its origin or register
LEAD_IN_RE = re.compile(r"\b(for|of|to|with|as|in|by)$")


def read_dawg(path):
    data = open(path, "rb").read()
    count = len(data) // 4
    nodes = struct.unpack("<%dI" % count, data[:count * 4])
    if (count < 2) or (nodes[0] != count - 1):
        sys.exit("%s: not a Zyzzyva word graph (node count %d, %d words in the file)" % (path, nodes[0], count))
    words = []
    stack = [(1, "")]
    while stack:
        index, prefix = stack.pop()
        while True:
            node = nodes[index]
            word = prefix + chr(node >> 24)
            if node & 0x800000:
                words.append(word)
            if node & 0x3fffff:
                stack.append((node & 0x3fffff, word))
            if node & 0x400000:
                break
            index += 1
    return {w: None for w in words}


def read_text(path):
    words = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.strip()
        if (not line) or line.startswith("#"):
            continue
        parts = line.split(None, 1)
        if not TEXT_WORD_RE.match(parts[0]):
            continue
        words[parts[0].upper()] = parts[1].strip() if len(parts) > 1 else None
    return words


def sentence(text):
    text = text.strip().rstrip(";,")
    if not text:
        return text
    text = text[0].upper() + text[1:]
    return text if text[-1] in ".!?" else text + "."


def format_definition(definition):
    """The Collins lists' style in popdefs.txt's, or the definition as it is"""
    definition = "".join(c for c in definition if (c >= " ") and (c != "\x7f"))
    if not re.search(r"\[[a-z]+[^\]]*\]", definition):
        return definition
    out = []
    for sense in definition.split(" / "):
        sense = sense.strip()
        m = SENSE_RE.match(sense)
        if m:
            label, gloss, pos = m.group(1), m.group(2), m.group(3)
        else:
            # brackets within the sense: their grammar is dropped
            label, gloss, pos = None, re.sub(r"\s*\[[^\]]*\]", "", sense), None
        form = FORM_RE.match(gloss)
        base = form.group(1) if form else None
        if form:
            gloss = form.group(2)
        if (label is None) and gloss.startswith("("):
            inner = re.match(r"^\(([^)]*)\) (.*)$", gloss)
            if inner:
                label, gloss = inner.group(1), inner.group(2)
        if label and LEAD_IN_RE.search(label):
            gloss, label = label + " " + gloss, None
        if base:
            gloss = "Form of %s: %s" % (base, gloss)
        tags = ", ".join(t for t in (POS.get(pos, pos + ".") if pos else None, label) if t)
        out.append(("(%s) " % tags if tags else "") + sentence(gloss))
    return " ".join(out)


def find_dawg(folder, lexicon):
    found = sorted(glob.glob(os.path.join(folder, "**", "words", lexicon, "*-%s.dwg" % lexicon), recursive=True))
    if not found:
        sys.exit("%s: no data/words/%s/*-%s.dwg in it" % (folder, lexicon, lexicon))
    return found[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("source", help="a Zyzzyva folder, a .dwg word graph or a text word list")
    parser.add_argument("--definitions", help="a text file of words and their definitions")
    parser.add_argument("--lexicon", default="CSW24", help="the Zyzzyva lexicon in a folder (default CSW24)")
    parser.add_argument("--mod-dir", default=MOD_DIR, help="the mod folder to write into")
    args = parser.parse_args()

    source = args.source
    if os.path.isdir(source):
        source = find_dawg(source, args.lexicon)
    words = read_dawg(source) if source.lower().endswith(".dwg") else read_text(source)
    defs_read = 0
    if args.definitions:
        definitions = read_text(args.definitions)
        defs_read = len(definitions)
        for word in words:
            words[word] = definitions.get(word) or words[word]

    kept = {}
    no_u = 0
    for word, definition in words.items():
        if not WORD_RE.match(word):
            continue
        if re.search(r"Q(?!U)", word):
            no_u += 1
            continue
        kept[word] = format_definition(definition) if definition else None

    os.makedirs(args.mod_dir, exist_ok=True)
    with open(os.path.join(args.mod_dir, "collins-words.txt"), "w", newline="\n") as f:
        f.write("".join(w + "\n" for w in sorted(kept)))
    defined = sorted(w for w, d in kept.items() if d)
    defs_path = os.path.join(args.mod_dir, "collins-defs.txt")
    if defined:
        with open(defs_path, "w", encoding="utf-8", newline="\n") as f:
            f.write("".join("%s %s\n" % (w, kept[w]) for w in defined))
    elif os.path.exists(defs_path):
        os.remove(defs_path)
    print("%s: %d words, %d kept, %d of them defined%s; left out: %d with a Q and no U, %d shorter than 3 letters "
          "or not A-Z" % (os.path.basename(source), len(words), len(kept), len(defined),
                          " (%d definitions read)" % defs_read if args.definitions else "", no_u,
                          len(words) - len(kept) - no_u))


if __name__ == "__main__":
    main()
