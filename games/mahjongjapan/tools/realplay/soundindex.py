"""Finds the sequences in a sound log of the real-input tour (BYOPOP_SOUNDLOG, docs/WEB_PLAYER.md, "The soundboard"):

    python games/mahjongjapan/tools/realplay/soundindex.py <sound.log> [--gap <updates>] [--res <Res.h>] [--top <n>]

A sequence is a run of plays with less than --gap updates (default 200: one second) between one and the next. Runs
that play the same sounds in the same order are one kind; each kind prints how many times the log has it, how long it
lasts, and the first and last update of its first run, which is what tools/soundpreset.py takes
(`python tools/soundpreset.py <log> <first> <last> --ms 5`). A stop is written -<id>. With --res (the game's src/Res.h)
the ids are named. Both Mahjong Escapes' sounds are named the same way: SOUND_SELECT, SOUND_MATCH, SOUND_DEAL ..."""
import re
import sys


def main():
    args = sys.argv[1:]
    path, gap, res, top = None, 200, None, 60
    while args:
        a = args.pop(0)
        if a == '--gap':
            gap = int(args.pop(0))
        elif a == '--res':
            res = args.pop(0)
        elif a == '--top':
            top = int(args.pop(0))
        else:
            path = a
    if path is None:
        sys.exit(__doc__)
    names = {}
    if res:
        for line in open(res):
            m = re.match(r'\s*SOUND_(\w+)\s*=\s*(0x[0-9a-fA-F]+|\d+)', line)
            if m:
                names[int(m.group(2), 0)] = m.group(1).lower()

    def name(i):
        return names.get(i, str(i))

    events = []
    for line in open(path):
        m = re.match(r'(\d+) (play|stop) (\d+) #(\d+)', line)
        if m:
            events.append((int(m.group(1)), m.group(2), int(m.group(3))))
    runs = []
    for e in events:
        if runs and e[0] - runs[-1][-1][0] < gap:
            runs[-1].append(e)
        else:
            runs.append([e])
    kinds = {}
    for r in runs:
        key = tuple(('-' if e[1] == 'stop' else '') + name(e[2]) for e in r)
        kinds.setdefault(key, []).append(r)
    rows = sorted(kinds.items(), key=lambda kv: (-len(kv[1]), len(kv[0])))
    print('%d plays and stops in %d runs of %d kinds (gap %d updates)' % (len(events), len(runs), len(kinds), gap))
    for key, rs in rows[:top]:
        r = rs[0]
        text = ' '.join(key) if len(key) <= 14 else ' '.join(key[:12]) + ' ... (%d events)' % len(key)
        print('x%-4d %6d updates  %d..%d  %s' % (len(rs), r[-1][0] - r[0][0], r[0][0], r[-1][0], text))


if __name__ == '__main__':
    main()
