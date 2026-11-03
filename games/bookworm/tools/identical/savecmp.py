# savecmp.py <port's file> <original's file> <port's state log>: a saved game (Board::WriteSavedGame) from each program,
# byte for byte, but for the fields the original never initialises, which it writes as the heap left them, and the wall-clock
# time in the game log: the byte
# mUnused1b2 after mBurnedDown, and the sapphire and diamond counters of the game's and the level's StatKeeper
# (StatKeeper.cpp, NAMES.md D39). They are found from the port's own file: its game stats start with the score, words
# and letters its state log ends on, its level stats follow them, and mUnused1b2 is two bytes after the fire meter,
# whose high word the state log also has. Exits 1 if anything else differs, printing where.
import re
import struct
import sys

PAT = re.compile(r"watch (\S+): (\S+) -> (\S+) at update (\d+)")
port, orig = open(sys.argv[1], "rb").read(), open(sys.argv[2], "rb").read()
last = {}
for ln in open(sys.argv[3], encoding="utf-8", errors="replace"):
    m = PAT.search(ln)
    if m and m.group(3) != '-':
        last[m.group(1)] = int(m.group(3))


def stats_end(o):
    o += 12 * 4
    o += 4 + struct.unpack_from("<i", port, o)[0] + 4   # the best word and its score
    o += 4 + struct.unpack_from("<i", port, o)[0] + 4   # the longest word and its score
    return o


game = port.find(struct.pack("<iii", last["score"], last["words"], last["letters"]))
meter = port.rfind(struct.pack("<I", last["firemeter"]), 0, game)
if game < 0 or meter < 0 or len(port) != len(orig):
    print("saved game: cannot place the fields (game stats at %d, fire meter at %d; %d and %d bytes)" %
          (game, meter, len(port), len(orig)))
    sys.exit(1)
level = stats_end(game)
skip = {meter + 6}
for s in (game, level):
    skip.update(range(s + 28, s + 36))
# the game log (Board::mGameLog as a web string, the record's last string, before one bool): its start time, time(NULL),
# is the wall clock; bits 48 to 79 after the string's 8 hex digits of length are characters 8 to 13 (6 bits each)
for n in range(20, len(port) - 5):
    at = len(port) - 1 - n
    if struct.unpack_from("<i", port, at - 4)[0] == n:
        skip.update(range(at + 8 + 8, at + 8 + 14))
        break
else:
    print("saved game: cannot find the game log")
    sys.exit(1)
diff = [i for i in range(len(port)) if port[i] != orig[i] and i not in skip]
garbage = sum(1 for i in skip if port[i] != orig[i])
if diff:
    print("saved game: DIFFERS at %d bytes, from offset %d (port %s, original %s)" %
          (len(diff), diff[0], port[diff[0]:diff[0] + 8].hex(), orig[diff[0]:diff[0] + 8].hex()))
    sys.exit(1)
print("saved game: the same %d bytes but for the original's uninitialised fields and the start time (%d of their bytes differ)" %
      (len(port), garbage))
