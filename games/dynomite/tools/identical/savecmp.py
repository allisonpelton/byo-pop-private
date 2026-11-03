# savecmp.py <saved game> <saved game>: two SLOTn.SAV files field by field, in the order Dynomite::Save (0x405140) writes
# them, up to the side bar's prompt (FirePrompt); the rest of the file is compared byte by byte. Left out are what differs
# from program to program by design: the picture of the board (two renderers), the tick count at the save and the board's
# start ticks; and the fields the original never sets before it saves them, so that its file holds whatever the memory
# held (the port zeroes them; IDENTICAL_PLAY.md, Oddities): the board's +0x64 and +0x140, the grid's warning and its blink
# count (+0x4688, +0x468c), its blast sums (+0x46d4 to +0x46dc), the launcher's 0x50 bytes at +0x38 and the prompt's +0xc
# and +0x1c. Prints each differing field and, last, a summary line; exits 1 if any differs.
import struct
import sys

SKIP = {'pic', 'tick', 'start', 'board+0x64', 'board+0x140', 'grid.warning', 'grid.blink', 'grid.blastx', 'grid.blasty',
        'grid.blastcount', 'launcher+0x38', 'prompt+0xc', 'prompt+0x1c'}


def layout(d):
    m = []
    p = [0]

    def f(n, size):
        m.append((p[0], p[0] + size, n))
        p[0] += size

    def i(*names):
        for n in names:
            f(n, 4)

    def b(*names):
        for n in names:
            f(n, 1)

    f('mode', 0x14); f('name', 0x14); f('pic', 0x3840)
    i('tick'); b('global'); i('start', 'score', 'difficulty', 'type')
    i('board+0x64', 'board+0x68', 'board+0x6c', 'board+0x70', 'board+0x74'); b('board+0xb0', 'initialized')
    i('board+0x104', 'board+0x108', 'powerup angle', 'board+0x128', 'bonus'); b('board+0x130', 'board+0x131', 'board+0x132')
    i('board+0x134', 'board+0x138', 'board+0x13c', 'board+0x140', 'chain', 'board+0x158', 'fired', 'board+0x160',
      'board+0x1c8', 'board+0x1cc', 'longest', 'babies')
    n = struct.unpack_from('<I', d, p[0])[0]
    i('title length'); f('title', n)
    b('board+0x16c', 'board+0x100'); i('rank'); b('rank changed', 'board+0x80')
    i('grid.ceiling'); f('grid.cells', 0x1770)
    i('grid.rows', 'grid.bottom', 'grid.eggs', 'grid.maxcolor', 'grid.pastline'); b('grid.warning'); i('grid.blink')
    b('grid+0x4690', 'grid+0x4691', 'grid.babies on'); i('grid.baby chance'); b('grid+0x46d0', 'grid.break past line')
    i('grid.blastx', 'grid.blasty', 'grid.blastcount'); b('grid.lone egg')
    while True:
        x = struct.unpack_from('<i', d, p[0])[0]
        f('grid.dynamite', 8)
        if x == -1:
            break
    while d[p[0]]:
        f('grid.baby egg', 13)
    b('grid.baby eggs end')
    i('launcher.angle', 'launcher+0x18', 'launcher+0x1c'); f('launcher+0x20', 8); i('launcher.recoil x', 'launcher.recoil y')
    f('launcher+0x30', 8); f('launcher+0x38', 0x50)
    i('launcher.aim x', 'launcher.aim y', 'launcher.sling x', 'launcher.sling y', 'launcher+0xa5c', 'launcher+0xa60',
      'launcher+0xa64', 'launcher+0xa68', 'launcher+0xa6c', 'launcher+0xa70'); b('launcher.frozen'); i('launcher+0xaac')
    for w in ('loaded', 'held'):
        b('launcher.' + w)
        if d[p[0] - 1]:
            f('launcher.' + w + ' egg', 20)
    i('steggy.bob', 'steggy.drop', 'steggy.bob angle', 'steggy.toss vx', 'steggy.toss vy'); b('steggy+0x28')
    i('steggy+0x2c'); b('steggy.alerting'); i('steggy.blink count'); b('steggy.blink', 'steggy.want egg', 'steggy.drop egg')
    for w in ('next', 'toss'):
        b('steggy.' + w)
        if d[p[0] - 1]:
            f('steggy.' + w + ' egg', 20)
    i('prompt+0xc', 'prompt.pulses'); b('prompt+0x1c'); i('prompt.alpha', 'prompt.fade')
    f('rest', len(d) - p[0])
    return m


a = open(sys.argv[1], 'rb').read()
b = open(sys.argv[2], 'rb').read()
la, lb = layout(a), layout(b)
bad = 0
if [(x[1] - x[0], x[2]) for x in la[:-1]] != [(x[1] - x[0], x[2]) for x in lb[:-1]]:
    print("the two files' layouts differ (dynamite, baby eggs, eggs held or the title)")
    bad += 1
else:
    for (lo, hi, n), (lo2, hi2, _) in zip(la, lb):
        if n in SKIP or a[lo:hi] == b[lo2:hi2]:
            continue
        if n == 'rest' or n == 'grid.cells':
            k = next(j for j in range(min(hi - lo, hi2 - lo2)) if a[lo + j] != b[lo2 + j]) if hi - lo and hi2 - lo2 else 0
            print("%s at 0x%x (+0x%x): %s / %s" % (n, lo + k, k, a[lo + k:lo + k + 8].hex(), b[lo2 + k:lo2 + k + 8].hex()))
        else:
            print("%s at 0x%x: %s / %s" % (n, lo, a[lo:hi].hex(), b[lo2:hi2].hex()))
        bad += 1
print("%d and %d bytes; %s" % (len(a), len(b), "%d fields differ" % bad if bad else
                               "the same apart from the picture, the clock and the original's unset fields"))
sys.exit(1 if bad else 0)
