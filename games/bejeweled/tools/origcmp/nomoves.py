"""Writes a version-1 savegame.dat (GAME_NOTES 3.14) whose board has no set and no move, from a save the port wrote,
so that loading it in the original and in the port leads straight to "no more moves", the game over and the name
entry of a normal game. Only the gem colours and the score change; every other field stays as the port saved it.

    python games/bejeweled/tools/origcmp/nomoves.py <savegame.dat in> <savegame.dat out> [score] [--one]

With --one the board has exactly one move, a swap in the top row that makes a set there; whether the refill
leaves a move depends on the RNG, so the seed is searched with the port.

The score stays as saved unless given; one that reaches the level's target starts a level-up instead.
"""
ONE = False
import random
import struct
import sys


def has_set(b):
    for r in range(8):
        for c in range(8):
            v = b[r][c]
            if c <= 5 and b[r][c + 1] == v and b[r][c + 2] == v:
                return True
            if r <= 5 and b[r + 1][c] == v and b[r + 2][c] == v:
                return True
    return False


def has_move(b):
    for r in range(8):
        for c in range(8):
            for dr, dc in ((0, 1), (1, 0)):
                r2, c2 = r + dr, c + dc
                if r2 > 7 or c2 > 7:
                    continue
                b[r][c], b[r2][c2] = b[r2][c2], b[r][c]
                s = has_set(b)
                b[r][c], b[r2][c2] = b[r2][c2], b[r][c]
                if s:
                    return True
    return False


def moves(b):
    out = []
    for r in range(8):
        for c in range(8):
            for dr, dc in ((0, 1), (1, 0)):
                r2, c2 = r + dr, c + dc
                if r2 > 7 or c2 > 7:
                    continue
                b[r][c], b[r2][c2] = b[r2][c2], b[r][c]
                if has_set(b):
                    out.append((r, c, r2, c2))
                b[r][c], b[r2][c2] = b[r2][c2], b[r][c]
    return out


def one_move(b):
    """The board changed in one cell of row 0 so that it has exactly one move, which makes a set in row 0 (the
    refill is then three gems at the top), or None."""
    for c in range(8):
        keep = b[0][c]
        for v in range(7):
            b[0][c] = v
            if not has_set(b):
                m = moves(b)
                if len(m) == 1 and m[0][0] == 0 and m[0][2] == 0:
                    return m[0]
        b[0][c] = keep
    return None


def main():
    global ONE
    ONE = "--one" in sys.argv
    sys.argv = [a for a in sys.argv if a != "--one"]
    src, dst = sys.argv[1], sys.argv[2]
    score = int(sys.argv[3]) if len(sys.argv) > 3 else None   # keep the saved score (a higher one can fill the level bar)
    d = bytearray(open(src, "rb").read())
    assert struct.unpack_from("<i", d, 0)[0] == 1, "not a version-1 save"
    b = None
    seed = 0
    while b is None:   # the first random board (Python's own RNG, seeds 0, 1, ...) with no set and no move
        rnd = random.Random(seed)
        cand = [[rnd.randrange(7) for _ in range(8)] for _ in range(8)]
        if not has_set(cand) and not has_move(cand) and (not ONE or one_move(cand)):
            b = cand
        seed += 1
    off = 13
    for r in range(8):
        for c in range(8):
            assert d[off] == 1, "a cell without a gem"
            struct.pack_into("<i", d, off + 1, b[r][c])
            d[off + 1 + 0x4c] = 0                        # mDying: a resting gem
            struct.pack_into("<i", d, off + 1 + 0x50, 0)  # mDyingFrame
            off += 1 + 0x78
    # the tail: state, timed, isNewGame, barWidth, playSeconds, idleTicks, score, mult, displayScore
    struct.pack_into("<i", d, off, 1)
    if score is not None:
        struct.pack_into("<i", d, off + 6 + 12, score)
        struct.pack_into("<i", d, off + 6 + 16 + 8, score)
    open(dst, "wb").write(d)
    for row in b:
        print(" ".join(str(v) for v in row))
    if ONE:
        r, c, r2, c2 = moves(b)[0]
        print("move: (%d,%d)-(%d,%d)" % (c, r, c2, r2))


if __name__ == "__main__":
    main()
