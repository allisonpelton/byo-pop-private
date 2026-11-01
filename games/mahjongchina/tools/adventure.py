"""Writes a -script for a whole escape adventure from a new player, from the solver's files.
python adventure.py <solver dir with s<n>.txt/e<n>.txt> <out dir> <last stage> [extra.txt]
MJ_ADV_FIRST=<n> starts at stage n from a saved player whose progress is n - 1 (resign.py --progress), with no name
to type: the solver finds no solution for stage 11, so the adventure is played as stages 1-10 and 12-52.
extra.txt lines: "<stage> <after move|start|end> <update offset> <command...>" add clicks (tips found by looking)."""
import sys, os
sol, out, last = sys.argv[1], sys.argv[2], int(sys.argv[3])
extra = {}
if len(sys.argv) > 4:
    for l in open(sys.argv[4]):
        p = l.split(None, 3)
        if len(p) == 4 and not l.startswith('#'):
            extra.setdefault((int(p[0]), p[1]), []).append((int(p[2]), p[3].strip()))
# China's tips: the OK of each, at its box's place (GameDialogs.cpp's gTipPos) plus the box's own offset
START_TIP = {1: (400, 407, 0), 2: (312, 433, 0), 4: (192, 314, 300), 8: (327, 255, 300), 13: (392, 341, 300)}
TIMED_TIP = {1: (187, 330), 7: (566, 440), 5: (400, 245)}  # ids 1 and 7 come only on stages 1 and 2
lines = ["1400 click 400 262", "1720 char T", "1725 char e", "1730 char s", "1735 char t", "1760 key RETURN"]
first = int(os.environ.get("MJ_ADV_FIRST", "1"))
t = 2600
if first > 1:
    lines = ["1400 click 400 262"]
    t = 2100
def emit(cmd):
    lines.append(f"{t} {cmd}")
def extras(stage, where):
    global t
    for off, cmd in extra.get((stage, where), []):
        t += off
        emit(cmd)
for st in range(first, last + 1):
    emit("click 725 565")  # the map's PLAY
    t += 400
    if os.environ.get("MJ_ADV_SHOTS"):
        t += 800; emit(f"screenshot {out}/start{st:02d}.png"); t += 100
    if st in START_TIP:
        x, y, d = START_TIP[st]
        t += d; emit("click %d %d" % (x, y)); t += 200
    extras(st, 'start')
    t += 200
    moves = [l.split() for l in open(os.path.join(sol, f's{st}.txt')) if l[0].isdigit()]
    tips = {}
    ntiles = 144
    special = set()
    for l in open(os.path.join(sol, f'e{st}.txt')):
        p = l.split()
        if len(p) > 1 and p[1] == 'tiles,':
            ntiles = int(p[0])
        if p[0] == 'special':
            special.add(int(p[1]))
        if p[0] == 'tip' and ((p[1] == '1' and st == 1) or (p[1] == '7' and st == 2) or p[1] == '5'):
            tips.setdefault(int(p[2]), []).append(int(p[1]))
    for i, m in enumerate(moves):
        for tip in tips.get(i, []):
            t += 250; emit("click %d %d" % TIMED_TIP[tip]); t += 100
        emit(f"click {m[0]} {m[1]}"); t += 60
        emit(f"click {m[2]} {m[3]}"); t += 250
        if i in special:
            t += 400  # the key's, wild tile's or changing tile's flights ignore clicks (0x40c030)
        extras(st, str(i + 1))
    t += 3000 + 60 * ntiles  # the tiles left fly off one by one at the end
    emit(f"screenshot {out}/done{st:02d}.png"); t += 100
    emit("click 397 400")  # the level's end OK
    t += 100
    extras(st, 'end')
    t += 1500
    emit(f"screenshot {out}/map{st:02d}.png"); t += 100
t += 500
emit(f"screenshot {out}/last.png"); t += 100
emit("quit")
print("\n".join(lines))
