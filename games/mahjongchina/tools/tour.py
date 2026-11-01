#!/usr/bin/env python3
"""The menus, the dialogs, a classic game and the focus, from a saved player, as two scripts that make the same shots:
<outdir>/orig.txt for origrun (milliseconds) and <outdir>/port.txt for the port (updates of 5 ms).

    python games/mahjongchina/tools/tour.py <port shot dir> <outdir>

The main menu, the options, the four pages of instructions, the player list, the classic list and its second page, a
layout chosen and played (its How To Play tip, then the board), losing and regaining the focus, pause and the in-game
menu. No click reaches a link: the PopCap and SpinTop logos at the menu's corners and EXIT are never clicked.
"""
import os
import pathlib
import sys

pdir, outdir = pathlib.Path(sys.argv[1]).resolve().as_posix(), sys.argv[2]
O = ["waitshot splash 30000", "wait 7000"]
P = []
t = 1700


def click(x, y, ms):
    global t
    O.extend(["move %d %d" % (x, y), "wait 150", "click %d %d" % (x, y), "wait %d" % ms])
    P.append("%d click %d %d" % (t, x, y))
    t += ms // 5


def shot(name):
    O.append("shot " + name)
    P.append("%d screenshot %s/%s.png" % (t, pdir, name))


def wait(ms):
    global t
    O.append("wait %d" % ms)
    t += ms // 5


def focus(what, ms):
    global t
    O.extend(["focus " + what, "wait %d" % ms])
    P.append("%d focus %s" % (t, what))
    t += ms // 5


shot("menu")
click(236, 543, 1500); shot("options"); click(404, 441, 1500)
click(408, 543, 1500); shot("ins1")
for k in range(2, 5):
    click(566, 441, 1200); shot("ins%d" % k)
click(403, 443, 1500)
click(400, 496, 1500); shot("players"); click(514, 422, 1500)
click(405, 362, 2500); shot("classic1"); click(665, 548, 1500); shot("classic2")
click(400, 140, 800); shot("classic_sel"); click(488, 569, 3000); shot("cboard")
wait(3000); shot("cboard2")
focus("lose", 1500); shot("unfocused"); focus("regain", 1500); shot("refocused")
click(400, 407, 1500); shot("cboard3")
click(738, 580, 1500); shot("gpause"); click(400, 300, 1500); shot("resumed")
click(738, 545, 1500); shot("gmenu")
O.append("quit")
P.append("%d quit" % (t + 50))
open(os.path.join(outdir, "orig.txt"), "w", newline="\n").write("\n".join(O) + "\n")
open(os.path.join(outdir, "port.txt"), "w", newline="\n").write("\n".join(P) + "\n")
print("port ends at update", t)
