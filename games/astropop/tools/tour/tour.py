"""Phase two of the identical-play pass: writes the port's and the original's scripts for one tour of screens.

    python games/astropop/tools/tour/tour.py <tour.txt> <out dir> [--delta-orig N] [--delta-port N] [--shot-delta-orig N]

<tour.txt> holds lines "<update> <command> [args]" (a # starts a comment):
    click x y | rclick x y | move x y | down x y | up x y     game pixels (640x480)
    key <Return|Escape|Space|Tab|Backspace>                    one key
    type <text>                                                one character after another
    focus lose | focus regain
    shot <name>                                                a screenshot named <name>.png
    quit

The port's script (<out>/port.txt) is a -realinput file: real SDL events through HandleEvent, a screenshot written to
<out>/port/<name>.png at that update. The original's (<out>/orig.txt) is an origrun script with "at <update> ..." lines
(origrun's update counter is the same as the port's mUpdateCount: phase one gave both the same clicks at the same updates).
The shots are named alike, so shotdiff pairs them. --delta-orig and --delta-port move one side's lines by that many updates
(measured with a tour of a moving picture: tour_calibrate.txt).
"""
import argparse
import os

VK = {'Return': 13, 'Escape': 27, 'Space': 32, 'Tab': 9, 'Backspace': 8}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('tour')
    ap.add_argument('out')
    ap.add_argument('--delta-orig', type=int, default=0)
    ap.add_argument('--delta-port', type=int, default=0)
    ap.add_argument('--move-delta-port', type=int, default=-1, help="the port's mouse moves only: what an input does starts one update earlier in the original than in the port at the same stamp (hover_timing.txt: the ramp of a lit panel; the credits' scroll), so the port's input is stamped one earlier")
    ap.add_argument('--click-delta-port', type=int, default=-1, help="the port's clicks, presses, releases and keys only (the same offset as --move-delta-port)")
    ap.add_argument('--shot-delta-orig', type=int, default=0, help="the original's shots only (a plain shot at N shows an earlier frame: see docs/ORIGRUN.md)")
    a = ap.parse_args()
    os.makedirs(os.path.join(a.out, 'port'), exist_ok=True)
    absout = os.path.abspath(a.out).replace(os.sep, '/')
    port, orig = [], []
    last = 0
    for ln in open(a.tour):
        ln = ln.split('#')[0].strip()
        if not ln:
            continue
        u, cmd, *rest = ln.split(None, 2)
        arg = rest[0] if rest else ''
        u = int(u)
        last = max(last, u)
        pu, ou = u + a.delta_port, u + a.delta_orig
        if cmd == 'shot':
            port.append('%d screenshot %s/port/%s.png' % (pu, absout, arg))
            orig.append('at %d shot %s' % (ou + a.shot_delta_orig, arg))
        elif cmd in ('click', 'move', 'down', 'up'):
            port.append('%d %s game %s' % (pu + (a.move_delta_port if cmd == 'move' else a.click_delta_port), cmd, arg))
            orig.append('at %d %s %s' % (ou, cmd, arg))
        elif cmd == 'rclick':
            port.append('%d click game %s right' % (pu + a.click_delta_port, arg))
            orig.append('at %d rclick %s' % (ou, arg))
        elif cmd == 'key':
            port.append('%d key %s' % (pu + a.click_delta_port, arg))
            orig.append('at %d key %d' % (ou, VK[arg]))
        elif cmd == 'type':
            port.append('%d type %s' % (pu + a.click_delta_port, arg))
            orig.append('at %d text %s' % (ou, arg))
        elif cmd == 'focus':
            port.append('%d focus %s' % (pu, 'lost' if arg == 'lose' else 'gained'))
            orig.append('at %d focus %s' % (ou, arg))
        elif cmd == 'quit':
            pass
        else:
            raise SystemExit('unknown tour command: ' + ln)
    port.append('%d quit' % (last + 20 + a.delta_port))
    orig.append('at %d move 1 1' % (last + 20 + a.delta_orig))
    orig.append('quit')
    with open(os.path.join(a.out, 'port.txt'), 'w') as f:
        f.write('\n'.join(port) + '\n')
    with open(os.path.join(a.out, 'orig.txt'), 'w') as f:
        f.write('\n'.join(orig) + '\n')


if __name__ == '__main__':
    main()
