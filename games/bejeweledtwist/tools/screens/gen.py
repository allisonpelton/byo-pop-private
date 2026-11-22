"""The screen coverage routes (docs/SCREENS.md): one spec drives the original under origrun and the port, so both
get the same input on the same updates and take their shots on the same updates.

    python games/bejeweledtwist/tools/screens/gen.py <spec> <out dir> <port shots dir>
        -> <out dir>/<name>.orig.txt (origrun's script) and <out dir>/<name>.port.txt (the port's -script)

Updates in a spec count from the anchor: the update on which the main menu reaches its state 2 (Menu +0x1e0, the app's
main menu at +0x864), which the original's script waits for (`waitfor`, `mark`, then `at +k`). The original's loader
runs on its own thread, so the update it ends on varies from run to run, and absolute counts cannot line up; from the
anchor everything is in step. The port reaches the anchor at update 4 and its update k + 5 is the original's +k (the
calibration of 2026-11-21: every original shot +0 to +240 matched the port's at k + 5 within a level).

Spec lines, after an optional `#` comment:
    <k> click <x> <y>        a left click: the button down at k, up at k + 1 (as origrun's `at N click x y 1`)
    <k> move <x> <y>         the mouse moved there
    <k> key <name>           a key pressed: RETURN, ESCAPE, SPACE, BACK, or a decimal virtual-key code
    <k> text <chars>         characters typed (letters and digits)
    <k> focus lose|regain    the window loses or regains the focus
    <k> shot <name>          both sides shoot: <name>.png
    <k> state                the port prints its state (nothing on the original's side)
    <k> port <command...>    a port-only command (test commands to reach a state; never in a compared stretch)
    <k> offset <n>           from here on the port's update k + n is the original's +k (default 5); an autoplay
                             route lines up at 4, since the autoplay's game starts an update sooner on the port
    <k> end                  the last update: the port quits, the original's script ends
"""
import os
import sys

PORT_OFFSET = 5
ANCHOR = 'waitfor [[0xe0b698]+0x864]+0x1e0 == 2 120000'
KEYS = {'RETURN': 13, 'ESCAPE': 27, 'SPACE': 32, 'BACK': 8}


def main():
    spec, out, shots = sys.argv[1], sys.argv[2], sys.argv[3]
    name = os.path.splitext(os.path.basename(spec))[0]
    orig, port, end = [], [], None
    offset = PORT_OFFSET
    for raw in open(spec, encoding='utf-8'):
        line = raw.split('#', 1)[0].strip()
        if not line:
            continue
        w = line.split(None, 2)
        k, cmd, rest = int(w[0]), w[1], (w[2] if len(w) > 2 else '')
        p = k + offset
        if cmd == 'click':
            x, y = rest.split()
            orig.append((k, 'at +%d click %s %s 1' % (k, x, y)))
            port.append((p, '%d down %s %s' % (p, x, y)))
            port.append((p + 1, '%d up %s %s' % (p + 1, x, y)))
        elif cmd == 'move':
            orig.append((k, 'at +%d move %s' % (k, rest)))
            port.append((p, '%d move %s' % (p, rest)))
        elif cmd == 'key':
            vk = KEYS.get(rest, None)
            vk = vk if vk is not None else int(rest)
            orig.append((k, 'at +%d key %d' % (k, vk)))
            port.append((p, '%d key %d' % (p, vk)))
        elif cmd == 'text':
            orig.append((k, 'at +%d text %s' % (k, rest)))
            for c in rest:
                port.append((p, '%d char %s' % (p, c)))
        elif cmd == 'focus':
            orig.append((k, 'at +%d focus %s' % (k, rest)))
            port.append((p, '%d focus %s' % (p, rest)))
        elif cmd == 'shot':
            orig.append((k, 'at +%d shot %s' % (k, rest)))
            port.append((p, '%d screenshot %s/%s.png' % (p, shots, rest)))
        elif cmd == 'state':
            port.append((p, '%d state' % p))
        elif cmd == 'port':
            port.append((p, '%d %s' % (p, rest)))
        elif cmd == 'offset':
            offset = int(rest)
        elif cmd == 'end':
            end = k
        else:
            sys.exit('%s: unknown command %r' % (spec, cmd))
    if end is None:
        sys.exit('%s: no end line' % spec)
    with open(os.path.join(out, name + '.orig.txt'), 'w', newline='\n') as f:
        f.write(ANCHOR + '\nmark\n')
        for _, t in sorted(orig, key=lambda e: e[0]):
            f.write(t + '\n')
        f.write('wait 200\nquit\n')
    with open(os.path.join(out, name + '.port.txt'), 'w', newline='\n') as f:
        for _, t in sorted(port, key=lambda e: e[0]):
            f.write(t + '\n')
        f.write('%d quit\n' % (end + PORT_OFFSET))
    print('%s: %d original lines, %d port lines' % (name, len(orig), len(port)))


main()
