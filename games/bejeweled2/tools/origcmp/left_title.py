"""Exit 0 when the original's run left the title: the first two shots of the recording are not the same picture.
    left_title.py <events.json> <shots dir>"""
import json
import os
import sys

ev = json.load(open(sys.argv[1]))
names = [c.split()[1] for _, c, *_ in ev['events'] if c.startswith('shot ')][:2]
pics = [os.path.join(sys.argv[2], n + '.png') for n in names]
if len(pics) < 2 or not all(os.path.exists(p) for p in pics):
    sys.exit(0)
sys.exit(1 if open(pics[0], 'rb').read() == open(pics[1], 'rb').read() else 0)
