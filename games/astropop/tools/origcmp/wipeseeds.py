"""Runs visrun3d.py's 3D run of the original once for each seed, until the coverage has reached all four kinds of the screen wipe.

    python -I wipeseeds.py <script.txt> <seed> [<seed> ...]        (22:00 to 08:00, as visrun3d.py)

The wipe's kind is drawn when a board is made (`Board::Board`: AstroRand() % 500000000 % 4, GAME_NOTES section 10): 0 clears pixels at random
(0x426370), 1 moves an edge to the left (0x40b340), 2 to the right (0x40b430), 3 clears rows (0x40b540); the draw (0x40b5e0) and the check
(0x4264a0) belong to every kind. A run with one seed shows one kind, so each run patches the copy's seed (the bytes of origrun's `seed` patch
of tools/origrun/patches/astropop.txt, `mov eax, <seed>` where SexyApp::Init called GetTickCount) before it starts, and the coverage of
the six functions is added up over the runs; it stops at the first run after which all six have been reached. Each run's coverage file is
kept as build/ap-3d/coverage-<seed>.txt and its log lines go to build/ap-3d/seeds.log.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import visrun3d as v

SEED_VA = 0x4dc2be            # `b8 <seed> 90` in the staged copy's AstroPopGame.exe
WIPE = {"0040b340": "left (kind 1)", "0040b430": "right (kind 2)", "0040b540": "rows (kind 3)", "0040b5e0": "draw",
        "00426370": "random (kind 0)", "004264a0": "check"}


def patch_seed(exe, seed):
    d = bytearray(open(exe, "rb").read())
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    nsec, opt = struct.unpack_from("<H", d, pe + 6)[0], struct.unpack_from("<H", d, pe + 20)[0]
    base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
    sec = pe + 24 + opt
    for i in range(nsec):
        _, vsize, vaddr, rsize, rptr = struct.unpack_from("<8sIIII", d, sec + 40 * i)
        if base + vaddr <= SEED_VA < base + vaddr + max(vsize, rsize):
            off = rptr + (SEED_VA - base - vaddr)
            if d[off] != 0xb8 or d[off + 5] != 0x90:
                raise SystemExit("wipeseeds: no `mov eax, <seed>; nop` at the seed patch's place in %s" % exe)
            d[off + 1:off + 5] = struct.pack("<I", seed)
            open(exe, "wb").write(d)
            return
    raise SystemExit("wipeseeds: the seed patch's address is in no section")


def hits(path):
    got = {}
    for line in open(path):
        w = line.split()
        if w and w[0].lower() in WIPE:
            got[w[0].lower()] = int(w[1]) if len(w) > 1 and w[1].isdigit() else 1
    return got


def main():
    script, seeds = sys.argv[1], [int(s, 0) for s in sys.argv[2:]]
    seen = set()
    first = v.prepare
    cur = {}

    def prepare_with_seed():
        first()
        patch_seed(os.path.join(v.GAME, "AstroPopGame.exe"), cur["seed"])
    v.prepare = prepare_with_seed
    summary = open(os.path.join(v.WORK, "seeds.log"), "a")
    for seed in seeds:
        cur["seed"] = seed
        sys.argv = ["visrun3d.py", script]
        v.main()
        cov = os.path.join(v.WORK, "coverage.txt")
        got = hits(cov) if os.path.exists(cov) else {}
        if os.path.exists(cov):
            os.replace(cov, os.path.join(v.WORK, "coverage-%d.txt" % seed))
        seen |= set(got)
        print("seed %d: %s; so far %d of 6" % (seed, ", ".join(WIPE[a] for a in sorted(got)) or "nothing", len(seen)), file=summary, flush=True)
        print("seed %d: %s; so far %d of 6" % (seed, ", ".join(WIPE[a] for a in sorted(got)) or "nothing", len(seen)), flush=True)
        if len(seen) == len(WIPE):
            break
    print("not reached: %s" % (", ".join(WIPE[a] for a in sorted(set(WIPE) - seen)) or "none"), file=summary, flush=True)


if __name__ == "__main__":
    main()
