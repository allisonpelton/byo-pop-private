#!/usr/bin/env python3
"""mkprofile.py <book> <chapter> <stage> <out dir> [from profile dir]: a profile that stands at that point of the adventure.

Copies a profile (default profiles/bosschapter1: a player with the mini-games unlocked) and sets the player's saved position
(CurrentBook, CurrentChapter, CurrentStage and the Farthest* ones, as the game saves them: key, a number's type 0, a 4-byte
little-endian value). Both programs load it, so a comparison case can start at any chapter without playing up to it."""
import os, shutil, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))


def patch(data, key, value):
    k = struct.pack('<H', len(key)) + key.encode()
    i = data.find(k)
    if i < 0:
        sys.exit('no key ' + key)
    j = i + len(k)
    if data[j:j + 4] != b'\0\0\0\0':
        sys.exit('%s is not a number' % key)
    return data[:j + 4] + struct.pack('<i', value) + data[j + 8:]


def main():
    book, chapter, stage = (int(a) for a in sys.argv[1:4])
    out = os.path.abspath(sys.argv[4])
    src = sys.argv[5] if len(sys.argv) > 5 else os.path.join(HERE, 'profiles', 'bosschapter1')
    if os.path.isdir(out):
        shutil.rmtree(out)
    shutil.copytree(src, out)
    p = os.path.join(out, 'users', 'Lex.bwa')
    d = open(p, 'rb').read()
    for pre in ('Current', 'Farthest'):
        d = patch(d, pre + 'Book', book)
        d = patch(d, pre + 'Chapter', chapter)
        d = patch(d, pre + 'Stage', stage)
    open(p, 'wb').write(d)
    print('profile at book %d chapter %d stage %d: %s' % (book, chapter, stage, out))


main()
