# cmpsave.py <saved game A> <saved game B>: the bytes at which two saved games differ, leaving out the words that are heap
# pointers in both (the original saves some raw pointers; a word that is in 0x04000000..0x30000000 in both files and
# differs is a pointer). Prints the number of differing bytes left and the first runs, as offsets.
import struct
import sys
a = open(sys.argv[1], 'rb').read()
b = open(sys.argv[2], 'rb').read()
if len(a) != len(b):
    print("sizes differ: %d %d" % (len(a), len(b)))
n = min(len(a), len(b))
diff = set(i for i in range(n) if a[i] != b[i])
explained = set()
for o in range(0, n - 3):
    if any((o + k) in diff for k in range(4)):
        x = struct.unpack_from('<I', a, o)[0]
        y = struct.unpack_from('<I', b, o)[0]
        if 0x04000000 <= x < 0x30000000 and 0x04000000 <= y < 0x30000000:
            explained.update(range(o, o + 4))
left = sorted(diff - explained)
print("%d bytes differ, %d after leaving out pointers" % (len(diff), len(left)))
runs = []
for i in left:
    if runs and i - runs[-1][1] <= 8:
        runs[-1][1] = i
    else:
        runs.append([i, i])
print(runs[:30])
sys.exit(1 if left else 0)
