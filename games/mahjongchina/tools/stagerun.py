#!/usr/bin/env python3
"""Scripts that play Dynasty Adventure stage n from a saved player whose progress is n - 1, for the original under
origrun (wall clock, milliseconds) and for the port (updates of 5 ms), taking the same shots at points where the
screen stands still: the map, the stage's start tip, the board, each timed tip, the board half way, the level's end
and the map after it (for stage 52 the ending's screens after that).

    python games/mahjongchina/tools/stagerun.py <n> <solver dir> <port shot dir> <outdir>

The solver dir holds s<n>.txt and e<n>.txt from `mahjongchina_dealtest --stage <n>`. The scripts go to
<outdir>/orig<n>.txt and <outdir>/port<n>.txt. A player for the stage comes from any port player through
`resign.py --progress <n - 1>` (signed for the install folder for the port, for C:\\run\\game\\ for the sandbox).
"""
import os
import pathlib
import sys

n, sol, pdir, outdir = int(sys.argv[1]), sys.argv[2], pathlib.Path(sys.argv[3]).resolve().as_posix(), sys.argv[4]
# the OK of each tip (adventure.py's places): the start tips by stage, the timed ones by id
START_TIP = {1: (400, 407, 0), 2: (312, 433, 0), 4: (192, 314, 300), 8: (327, 255, 300), 13: (392, 341, 300)}
TIMED_TIP = {1: (187, 330), 7: (566, 440), 5: (400, 245)}
O = ["waitshot splash 30000", "wait 7000"]
P = []
t = 1400


def both_click(x, y, ow, pw):
    global t
    O.extend(["move %d %d" % (x, y), "wait 150", "click %d %d" % (x, y), "wait %d" % ow])
    P.append("%d click %d %d" % (t, x, y))
    t += pw


def both_shot(name):
    O.append("shot " + name)
    P.append("%d screenshot %s/%s.png" % (t, pdir, name))


def both_wait(ow, pw):
    global t
    O.append("wait %d" % ow)
    t += pw


both_click(400, 262, 3500, 700)   # Dynasty Adventure: the map
both_shot("s%d_map" % n)
both_click(725, 565, 2500, 400)   # the map's PLAY
if n in START_TIP:
    x, y, d = START_TIP[n]
    both_wait(d * 5 + 1000, d + 200)
    both_shot("s%d_tip" % n)
    both_click(x, y, 1500, 300)
both_shot("s%d_board" % n)
moves = [l.split() for l in open(os.path.join(sol, "s%d.txt" % n)) if l[0].isdigit()]
tips, special, ntiles = {}, set(), 144
for l in open(os.path.join(sol, "e%d.txt" % n)):
    p = l.split()
    if len(p) > 1 and p[1] == "tiles,":
        ntiles = int(p[0])
    if p[0] == "special":
        special.add(int(p[1]))
    if p[0] == "tip" and ((p[1] == "1" and n == 1) or (p[1] == "7" and n == 2) or p[1] == "5"):
        tips.setdefault(int(p[2]), []).append(int(p[1]))
half = len(moves) // 2
for i, m in enumerate(moves):
    for tip in tips.get(i, []):
        both_wait(1500, 300)
        both_shot("s%d_tip%d" % (n, tip))
        both_click(*TIMED_TIP[tip], 800, 100)
    if i == half:
        both_wait(1000, 200)
        both_shot("s%d_half" % n)
    both_click(int(m[0]), int(m[1]), 300, 60)
    both_click(int(m[2]), int(m[3]), 2600 if i in special else 600, 650 if i in special else 120)
rem = ntiles - 2 * len(moves)
both_wait(15000 + 300 * rem, 3000 + 60 * rem)   # the tiles left fly off one by one
both_shot("s%d_done" % n)
both_click(397, 400, 4000, 800)   # the level's end OK
both_shot("s%d_after" % n)
if n == 52:
    for k in range(1, 4):
        both_wait(3000, 600)
        both_shot("s%d_end%d" % (n, k))
O.append("quit")
P.append("%d quit" % (t + 50))
open(os.path.join(outdir, "orig%d.txt" % n), "w", newline="\n").write("\n".join(O) + "\n")
open(os.path.join(outdir, "port%d.txt" % n), "w", newline="\n").write("\n".join(P) + "\n")
wall = sum(int(l.split()[1]) for l in O if l.startswith("wait ")) // 1000
print("stage %d: %d pairs of %d tiles; the port ends at update %d, the original after about %d s" % (n, len(moves), ntiles, t, wall))
