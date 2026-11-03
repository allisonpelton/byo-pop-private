"""Turn the port bot's log (-autoplaylog, with -autoplaymenu) into an origrun script that plays the same clicks in the
original, and compare the original's watched values with the port's -statelog.

    python games/noahsark/tools/origrun_replay.py script <menu.txt> <bot log> <out script> <last update>
    python games/noahsark/tools/origrun_replay.py compare <origrun log> <port statelog>

script: the menu part ("at <N> click ..." lines, given as plain "<N> click ..." lines as the port's -script takes them) is
copied; every "click at update U: x y" of the bot becomes "at U+1 click x y" (the original's update counter reads one
more than the port's at the same state); the script ends with a wait and quit.
compare: the original's "watch 1..4" lines (board state, score, level, mode: origrun --watch of [[0x4d125c]+0x408]+0x310,
[0x4d125c]+0x474, +0x43c, +0x444) and the port's "update U: state a score b level c mode d" lines give the sequence of
(update, state, score, level, mode) in each (the row of the start, update 0 or 1, is skipped); they are compared with the original's updates taken one lower.
"""
import re
import sys


def script(menu, bot, out, last):
    lines = []
    for ln in open(menu):
        m = re.match(r'(\d+) (.*)', ln.strip())
        if m:
            lines.append('at %s %s' % (m.group(1), m.group(2)))
    for ln in open(bot):
        m = re.match(r'click at update (\d+): (\d+) (\d+)', ln)
        if m:
            lines.append('at %d click %s %s' % (int(m.group(1)) + 1, m.group(2), m.group(3)))
        m = re.match(r'text at update (\d+): (\S+)', ln)
        if m:
            lines.append('at %d text %s' % (int(m.group(1)) + 1, m.group(2)))
    lines.append('at %d move 1 1' % last)
    lines.append('quit')
    open(out, 'w').write('\n'.join(lines) + '\n')


def orig_series(path):
    cur = {1: None, 2: 0, 3: 0, 4: 1}
    rows = []
    for ln in open(path):
        m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', ln)
        if not m:
            continue
        w, v, u = int(m.group(1)), m.group(3), int(m.group(4))
        cur[w] = -1 if v == '-' else int(v)
        rows.append((u, cur[1], cur[2], cur[3], cur[4]))
    # one row per update (the last values at that update)
    byu = {}
    for r in rows:
        byu[r[0]] = r
    return [byu[u] for u in sorted(byu)]


def port_series(path):
    rows = []
    for ln in open(path):
        m = re.match(r'update (\d+): state (-?\d+) score (-?\d+) level (-?\d+) mode (-?\d+)', ln)
        if m:
            rows.append(tuple(int(g) for g in m.groups()))
    return rows


def compare(orig, port):
    o = orig_series(orig)
    p = port_series(port)
    # the original's board state is "-" (-1 here) before a board exists; the port logs -1 too
    o = [(x[0] - 1,) + tuple(x[1:]) for x in o if x[0] > 1]
    # both ways: a change the port makes at another update than the original is a difference too (the port logs
    # one row per update with a change, the original may log the watches in separate rows, so rows are
    # merged per update and the values are the last ones seen)
    def last_by_update(rows):
        d = {}
        for r in rows:
            d[r[0]] = r[1:]
        return d
    od = last_by_update(o)
    pd = last_by_update(p)
    bad = 0
    for u in sorted(set(od) | set(pd)):
        if u <= 1 or u > max(od):
            continue
        if od.get(u) != pd.get(u):
            bad += 1
            if bad <= 15:
                print('update', u, 'orig', od.get(u), 'port', pd.get(u))
    print('compared %d changes of the original and %d of the port: %d different' % (len(od), len(pd), bad))
    return bad


if __name__ == '__main__':
    if sys.argv[1] == 'script':
        script(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]))
    else:
        sys.exit(1 if compare(sys.argv[2], sys.argv[3]) else 0)
