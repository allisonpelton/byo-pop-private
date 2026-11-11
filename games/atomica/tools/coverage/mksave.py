#!/usr/bin/env python3
"""Crafts the saved games that the coverage check continues in the original (docs/COVERAGE.md): the port plays to a field
(make-saves.sh), and this makes a SAVEGAME.DAT of it with the atoms, the meter and the bomb the run needs, so the original reaches
what the recorded bot games never do (a game ends at level 1): element atoms, a bonus molecule, the level box, the bomb.
The layout is Board::Save / Grid::Save / Atom::Save of the port (the original's own file format).

    python -I games/atomica/tools/coverage/mksave.py dump <SAVEGAME.DAT>
    python -I games/atomica/tools/coverage/mksave.py make <base SAVEGAME.DAT> <elements|molecule|bonus|over|magnet, with -action for the action mode's game> <out SAVEGAME.DAT>
"""
import struct
import sys

BOARD = 4                      # after the mode (a 4-byte int)
PROGRESS = BOARD + 12          # Board::mProgress, the bomb's charge, 0 to 48
SCORE = BOARD + 105
METER = BOARD + 141            # Meter::Save: level, value, max, tick, countdown (4 bytes each), full (1), over tick (4)
BONUS = BOARD + 166            # BonusMolecules::Save: x, round, element, timer (4 each), 25 ints of the shape, has-shape (1)
GRID = BOARD + 283             # Grid::Save: pulse, pulse step, select glow, atom count, the atoms, then the cells' state
ATOMS = GRID + 16
PLAIN = 102                    # a plain atom's record: the element flag and Atom::Save's 101 bytes
ELEMENT = 158                  # an element atom's: and 56 bytes of its own
COLORS = {0xc: (0xff, 0x6e, 0x12), 0xf: (0x17, 0xe9, 0xfc), 0x12: (0xff, 0xac, 0xfe), 0x15: (0xe4, 0xe4, 0xe4), 0x18: (100, 0xff, 100)}
PLAIN_COLORS = ((0xff, 0, 0), (0, 0xff, 0), (0xff, 0xff, 0), (0x96, 0x96, 0xff), (0xff, 0, 0xff), (0xfc, 0x86, 0))   # types 0 to 5


def read(path):
    data = open(path, 'rb').read()
    count = struct.unpack_from('<i', data, GRID + 12)[0]
    atoms = []
    o = ATOMS
    for _ in range(count):
        size = ELEMENT if data[o] else PLAIN
        atoms.append(bytearray(data[o:o + size]))
        o += size
    return bytearray(data), atoms, data[o:]


def field(rec, name):
    offsets = {'pre': 1, 'col': 4, 'row': 8, 'x': 12, 'y': 16, 'placed': 20, 'color': 22, 'type': 46, 'hidden': 59}
    return offsets[name]


def col_row_type(rec):
    return struct.unpack_from('<i', rec, 4)[0], struct.unpack_from('<i', rec, 8)[0], struct.unpack_from('<i', rec, 46)[0]


def new_plain(template, kind, col, row):
    """A plain atom of a type 0 to 5, copied from an atom of the base field (the first plain one), put in a cell."""
    rec = bytearray(template[:PLAIN])
    r, g, b = PLAIN_COLORS[kind]
    struct.pack_into('<iiii', rec, 22, r, g, b, 255)
    struct.pack_into('<i', rec, 46, kind)
    struct.pack_into('<ii', rec, 4, col, row)
    struct.pack_into('<ii', rec, 12, col * 50 + 25, row * 50 + 25)
    rec[1] = 0                                           # not a pre-atom
    return rec


def new_element(template, kind, col, row):
    """An element atom (types 12 to 26) of the plain template's shape: the colour of its element, the 56 bytes of its own."""
    rec = bytearray(template[:PLAIN])
    struct.pack_into('<ii', rec, 4, col, row)
    struct.pack_into('<ii', rec, 12, col * 50 + 25, row * 50 + 25)
    rec[1] = 0
    base = 0xc + (kind - 0xc) // 3 * 3
    r, g, b = COLORS[base]
    struct.pack_into('<iiii', rec, 22, r, g, b, 255)
    struct.pack_into('<i', rec, 46, kind)
    rec[1 + 88] = 1                                      # Atom::mIsElementAtom
    rec[0] = 1                                           # the element flag
    flicker = struct.pack('<5f', 0.0, 8.0, 16.0, 24.0, 4.0)
    step = struct.pack('<5f', 0.1, 0.15, 0.2, 0.25, 0.1)
    rec += struct.pack('<f', 0.0) + flicker + step + struct.pack('<iii', (kind - 0xc) // 3, (kind - 0xc) % 3, 0)
    return rec


def build(base, name):
    data, atoms, tail = read(base)
    mode = 1 if name.endswith('-action') else 0      # a saved game starts with its mode: 0 strategy, 1 action
    name = name.replace('-action', '')
    plain = next(a for a in atoms if col_row_type(a)[2] < 6 and not a[1])      # a settled plain atom to copy
    out = []
    if name == 'elements':
        # one element atom of each kind in a corner, plain atoms between them with no square of one type
        for kind, cell in ((0xc, (0, 0)), (0x10, (8, 0)), (0x14, (0, 7)), (0x16, (8, 7)), (0x18, (4, 0))):
            out.append(new_element(plain, kind, *cell))
        for i, cell in enumerate(((2, 2), (4, 2), (6, 2), (2, 4), (6, 4), (2, 6), (4, 6), (6, 6), (4, 4), (1, 3), (7, 3), (3, 5), (5, 5), (1, 1))):
            out.append(new_plain(plain, i % 5, *cell))
        progress, value = 48.0, None
    elif name == 'molecule':
        # three atoms of type 1 in an L round (4, 4), the fourth far off; one molecule fills the meter
        for cell in ((3, 3), (4, 3), (3, 4)):
            out.append(new_plain(plain, 1, *cell))
        struct.pack_into('<ii', out[0], 60, 2, 5)         # the first of them a biohazard atom (kind 2, picture 5): the molecule destroys it
        out.append(new_plain(plain, 1, 7, 6))
        for i, cell in enumerate(((1, 1), (6, 1), (1, 6), (5, 5), (7, 2), (2, 7), (6, 4), (0, 3))):
            out.append(new_plain(plain, (0, 2, 3, 4, 5)[i % 5], *cell))
        progress, value = 24.0, 'almost'
    elif name == 'magnet':
        # a few atoms of five kinds and a magnet's arrow pointing right (levels 17, 33 and 38 have one: the atoms slide a cell every six updates)
        for i, cell in enumerate(((0, 0), (1, 2), (0, 4), (2, 6), (1, 7), (3, 1), (2, 3), (0, 6), (4, 5), (3, 7), (5, 2), (4, 0))):
            out.append(new_plain(plain, (0, 1, 2, 3, 4)[(i * 2 + i // 5) % 5], *cell))
        progress, value = 0.0, None
    elif name == 'bonus':
        # the plus of the first bonus shape missing one atom: type 0 in the middle, type 2 on its four sides
        for cell in ((4, 3), (3, 4), (5, 4)):
            out.append(new_plain(plain, 2, *cell))
        out.append(new_plain(plain, 0, 4, 4))
        out.append(new_plain(plain, 2, 8, 7))             # the fourth: moved to (4, 5)
        for i, cell in enumerate(((1, 1), (7, 1), (1, 6), (7, 3), (2, 7), (0, 3))):
            out.append(new_plain(plain, (1, 3, 4, 5)[i % 4], *cell))
        progress, value = 0.0, None
    elif name == 'over':
        # a field of 70 atoms with no square of one type and two free cells: one move brings the next round, the field is full and
        # the game is over; the score is a high one, so the high score box comes
        for c in range(9):
            for r in range(8):
                if (c, r) not in ((7, 7), (8, 7)):
                    out.append(new_plain(plain, (c + 2 * r) % 5, c, r))
        progress, value = 0.0, None
    else:
        raise SystemExit('unknown save ' + name)
    body = bytearray(data[:ATOMS])
    struct.pack_into('<i', body, GRID + 12, len(out))
    for rec in out:
        body += rec
    tail = bytearray(tail)
    if name == 'magnet':
        struct.pack_into('<i', tail, 666, 1)               # Grid::mArrowX, after the hints shown, four flags and the bonus tick
    if name == 'molecule':
        struct.pack_into('<i', tail, 0x120 + 29, 1)       # Grid::mCombo after the cell states, the cursor, the colours and one flag: one molecule already made
        tail[0x120 + 29 + 12] = 0                          # and mComboEnded (no molecule in the last move) off, so the move does not start the combo again
    body += tail
    struct.pack_into('<f', body, PROGRESS, progress)
    if value == 'almost':
        mx = struct.unpack_from('<f', body, METER + 8)[0]
        struct.pack_into('<f', body, METER + 4, mx - 0.5)
    if name == 'over':
        struct.pack_into('<i', body, SCORE, 99999)
    if name == 'bonus':
        struct.pack_into('<i', body, BONUS + 4, 3)          # three molecules made: the sign slides in
    struct.pack_into('<i', body, 0, mode)
    return bytes(body)


def dump(path):
    data, atoms, tail = read(path)
    print('mode', struct.unpack_from('<i', data, 0)[0], 'progress', struct.unpack_from('<f', data, PROGRESS)[0],
          'score', struct.unpack_from('<i', data, SCORE)[0])
    print('meter level %d value %.1f max %.1f' % (struct.unpack_from('<i', data, METER)[0], *struct.unpack_from('<f', data, METER + 4)[0:1],
                                                  struct.unpack_from('<f', data, METER + 8)[0]))
    print('bonus x %d round %d element %d' % struct.unpack_from('<iii', data, BONUS))
    print(len(atoms), 'atoms, tail', len(tail), 'bytes')
    for a in atoms:
        print(('E' if a[0] else 'a'), col_row_type(a), 'pre', a[1])


if __name__ == '__main__':
    if sys.argv[1] == 'dump':
        dump(sys.argv[2])
    else:
        open(sys.argv[4], 'wb').write(build(sys.argv[2], sys.argv[3]))
