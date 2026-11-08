"""sndcmp.py <origrun sound.log> <port sound log> [--from N] [--to N] [--slack N]: the sounds of an original run and a port
run lined up (IDENTICAL_PLAY.md, phase two). The original's log is origrun's DirectSound stand-in's (logs/sound.log: each
buffer's create with its size and format, its plays and stops, in ms of the virtual clock, 33 ms an update); the port's is
-soundlog's (each start and stop with its frame and the sound's length in ms).

A sound is known by its length: a buffer of the original holds a whole sound of up to 2 s, and a longer one streams
through a 2 s buffer (88200 bytes), so both sides call a sound of more than 2 s "stream". Each start of the original is
matched with a start of the port of the same length at most --slack updates away (2 by default), in order; the starts
left over on either side are printed, then one line: how many matched and the largest gap in updates.
"""
import re
import sys

UPDATE_MS = 33.0


def orig_events(path):
    sizes, out = {}, []
    for line in open(path, encoding='utf-8', errors='replace'):
        m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)\s+fmt=(\d+)Hz/(\d+)bit/(\d+)ch', line)
        if m:
            t, buf, size, rate, bits, ch = m.groups()
            per_ms = int(rate) * int(bits) // 8 * int(ch) / 1000.0
            sizes[buf] = int(size) / per_ms if per_ms > 0 else 0.0
            continue
        m = re.match(r'\s*([\d.]+)\s+buffer (\d+) (play|stop)\s*(\w*)', line)
        if m:
            t, buf, what, how = m.groups()
            ms = sizes.get(buf, 0.0)
            if ms <= 0.0:
                continue  # the primary buffer
            out.append((float(t) / UPDATE_MS, what, 'stream' if ms >= 2000.0 else '%.0f' % ms, how))
    return out


def port_events(path):
    out = []
    for line in open(path):
        f = line.split()
        if len(f) >= 3 and f[1] in ('play', 'stop'):
            ms = float(f[2])
            out.append((float(f[0]), f[1], 'stream' if ms > 2000.0 else '%.0f' % ms, f[3] if len(f) > 3 else ''))
    return out


def main():
    args = sys.argv[1:]
    opt = {'--from': 0.0, '--to': 1e12, '--slack': 2.0}
    for k in list(opt):
        if k in args:
            i = args.index(k)
            opt[k] = float(args[i + 1])
            del args[i:i + 2]
    o = [e for e in orig_events(args[0]) if e[1] == 'play' and opt['--from'] <= e[0] <= opt['--to']]
    p = [e for e in port_events(args[1]) if e[1] == 'play' and opt['--from'] <= e[0] <= opt['--to']]
    used, matched, worst, left = set(), 0, 0.0, []
    for e in o:
        best = None
        for j, q in enumerate(p):
            if j in used or q[2] != e[2] or abs(q[0] - e[0]) > opt['--slack']:
                continue
            if best is None or abs(q[0] - e[0]) < abs(p[best][0] - e[0]):
                best = j
        if best is None:
            left.append(('original', e))
        else:
            used.add(best)
            matched += 1
            worst = max(worst, abs(p[best][0] - e[0]))
    left += [('port', q) for j, q in enumerate(p) if j not in used]
    for side, e in sorted(left, key=lambda x: x[1][0]):
        print('%-8s only: update %.1f %s %s %s' % (side, e[0], e[1], e[2], e[3]))
    print('%d of %d original starts matched (port %d), largest gap %.1f updates, %d left over'
          % (matched, len(o), len(p), worst, len(left)))


if __name__ == '__main__':
    main()
