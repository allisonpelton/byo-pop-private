"""cmp.py <tour dir>: the sounds of the screen tour, the original's (orig.sh's sound-orig.log) against the port's
(tools/shots/tour.py's run, port.log with its SOUND lines). The original's log has no names: origrun watches the
sample pointer of voices 0 to 3, so a start is a change to a pointer that is not 0, and a pointer is given the name
of the port's sound at the same place in the order the first time it is seen; after that, it must be the same name.
Times are the game's event time in ms from the first click (the port's update times 10; the original's counter less
the script's mark, which comes 100 ms after the first click). A sound shorter than origrun's watch poll can be missed
in the original's log, and one voice taken over by the next sound shows as one change."""
import re
import sys

VOICES = 4


def orig_starts(path):
    text = open(path, errors='replace').read()
    m = re.search(r'mark: the counter reads (\d+)', text)
    mark, out = int(m.group(1)), []
    for m in re.finditer(r'watch (\d): (\S+) -> (\d+) at update (\d+)', text):
        if m.group(3) != '0' and int(m.group(4)) > 0:  # update 0: before the game's first event (the splash)
            out.append((int(m.group(4)) - mark + 100, int(m.group(1)) - 1, int(m.group(3))))
    return out


def port_starts(path):
    out = []
    for line in open(path, errors='replace'):
        m = re.search(r'SOUND (\d+) (-?\d+) (\S+)', line)
        if m and 0 <= int(m.group(2)) < VOICES:
            out.append((int(m.group(1)) - 1510, int(m.group(2)), m.group(3)))
    return [s for s in out if s[0] >= 0]  # from the first click, as the original's mark counts


def main():
    d = sys.argv[1]
    o, p = orig_starts(d + '/sound-orig.log'), port_starts(d + '/port.log')
    names, bad, worst = {}, 0, 0
    for i in range(len(o)):
        a = o[i]
        b = p[i] if i < len(p) else None
        if b is None:
            print('%3d  orig %s, port none' % (i, a))
            bad += 1
            continue
        name = names.setdefault(a[2], b[2])
        ok = name == b[2] and a[1] == b[1]
        dt = a[0] - b[0]
        worst = max(worst, abs(dt))
        if not ok or abs(dt) > 100:
            bad += 1
        print('%3d  orig %6d v%d %-18s port %6d v%d %-18s %+5d %s' % (i, a[0], a[1], name, b[0], b[1], b[2], dt,
                                                                     '' if ok else 'DIFFERS'))
    if len(p) > len(o):
        # the original's watched voices are all taken from the deal on (IDENTICAL_PLAY.md, "Oddities"): no later start
        print('not compared: %d starts of the port from %d ms on' % (len(p) - len(o), p[len(o)][0]))
    print('orig %d starts, port %d; %d of the first %d differ; the largest time apart %d ms' % (len(o), len(p), bad, len(o), worst))


if __name__ == '__main__':
    main()
