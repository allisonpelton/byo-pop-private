# mkhs.py <dir>: writes the four modes' high-score files (STOMPED.HS, ENDLESS.HS, FOSSIL.HS, TIMETRIAL.HS) with low
# scores, so that a short scripted game reaches the name dialog and the table in both programs. A file is five rows of
# 0x1c bytes, as HighScores (0x422410) reads them: the game's id (-1), a name of 0x14 bytes and the score. Time Trial's
# scores are minus its times in milliseconds (a shorter time is higher).
import os
import struct
import sys

d = sys.argv[1]
names = [b'Ann', b'Bob', b'Cat', b'Dan', b'Eve']
tables = {
    'STOMPED.HS': [50, 40, 30, 20, 10],
    'ENDLESS.HS': [50, 40, 30, 20, 10],
    'FOSSIL.HS': [50, 40, 30, 20, 10],
    'TIMETRIAL.HS': [-3000000, -3100000, -3200000, -3300000, -3400000],
}
for f, scores in tables.items():
    with open(os.path.join(d, f), 'wb') as out:
        for n, s in zip(names, scores):
            out.write(struct.pack('<i20si', -1, n, s))
