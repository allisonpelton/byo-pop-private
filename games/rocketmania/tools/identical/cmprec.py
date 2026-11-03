# cmprec.py <saved game A> <saved game B> <first piece record offset> [count]: the saved pieces (165-byte records) of two saved
# games, field offset by field offset: for each offset inside a record, in how many records the two files differ. An offset
# where (nearly) every record differs is a field that is noise (pointers, positions that move on their own); the others
# are listed with the records that differ, to be read against WritePiece's order (BoardSave.cpp).
import sys
a = open(sys.argv[1], 'rb').read()
b = open(sys.argv[2], 'rb').read()
start = int(sys.argv[3])
count = int(sys.argv[4]) if len(sys.argv) > 4 else 60
R = 165
per = {}
for r in range(count):
    o = start + r * R
    if o + R > min(len(a), len(b)):
        break
    for k in range(R):
        if a[o + k] != b[o + k]:
            per.setdefault(k, []).append(r)
for k in sorted(per):
    print("offset %3d: %3d records %s" % (k, len(per[k]), per[k][:12]))
