"""Compares two folders of game files by path (case ignored), size and SHA-1: the Collection check's file comparison
(README.md, "The PopCap Game Collection version").   python cmpfiles.py <folder a> <folder b>"""
import hashlib
import os
import sys


def walk(root):
    out = {}
    for folder, _, files in os.walk(root):
        for name in files:
            path = os.path.join(folder, name)
            key = '/'.join(os.path.relpath(path, root).split(os.sep)).lower()
            out[key] = (os.path.getsize(path), hashlib.sha1(open(path, 'rb').read()).hexdigest()[:10])
    return out


a, b = walk(sys.argv[1]), walk(sys.argv[2])
print(len(a), 'files in a,', len(b), 'in b,', sum(1 for k in a if k in b and a[k] == b[k]), 'the same')
print('only a:', sorted(set(a) - set(b)))
print('only b:', sorted(set(b) - set(a)))
for k in sorted(set(a) & set(b)):
    if a[k] != b[k]:
        print('differ:', k, a[k][0], b[k][0])
