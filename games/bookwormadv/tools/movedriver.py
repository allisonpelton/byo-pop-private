"""movedriver.py: Bookworm Adventures played by the port's own bot, the thin loop of docs/TEST_COMMANDS.md. It asks the
game `move` on the live channel (-commands, with an empty -realinput file so its lines are real SDL events), plays the
move's input steps as real events, and asks again, until the screen named is reached or the time is up. Run it under
rundesk, with absolute paths:

    build/native/rundesk.exe python games/bookwormadv/tools/movedriver.py <out dir> <until screen> <seconds> [command ...] -- <exe> [game args...]

<until screen> is a `state` screen (menu, map, treasure, ...) to stop at once it has left it and come back, or "-" to
play until the time is up. The commands ("goto adventure") are sent first, each once its answer to the one
before has come (OK). The out dir gets commands.txt, game.log (the game's log) and movedriver.log (each move and
the screen after it); the last line says what ended the run.
"""
import json
import os
import subprocess
import sys
import time


def main():
    k = sys.argv.index('--')
    out, until, seconds = sys.argv[1], sys.argv[2], float(sys.argv[3])
    game = sys.argv[k + 1:]
    first = sys.argv[4:k]
    os.makedirs(out, exist_ok=True)
    cf, log, me = (os.path.join(out, n) for n in ('commands.txt', 'game.log', 'movedriver.log'))
    for f in (cf, log):
        if os.path.exists(f):
            os.remove(f)
    open(cf + '.empty', 'w').close()
    proc = subprocess.Popen(game + ['-commands=' + cf, '-realinput=' + cf + '.empty', '-fast'],
                            stdout=open(log, 'w'), stderr=subprocess.STDOUT)
    pos = 0
    sent = 0
    mine = open(me, 'w')

    def say(text):
        mine.write(text + '\n')
        mine.flush()

    def send(line, answer):
        """appends a line and waits up to 60 s for the first new log line that starts with answer"""
        nonlocal pos, sent
        with open(cf, 'a', newline='\n') as f:
            f.write(line + '\n')
        sent += 1
        t = time.time()
        while time.time() - t < 60:
            if proc.poll() is not None:
                return None
            with open(log, encoding='utf-8', errors='replace') as f:
                f.seek(pos)
                text = f.read()
            # only whole lines
            cut = text.rfind('\n')
            if cut >= 0:
                for ln in text[:cut].split('\n'):
                    pos += len(ln.encode('utf-8', errors='replace')) + 1
                    ln = ln[6:] if ln.startswith('INFO: ') else ln
                    if ln.startswith(answer):
                        return ln[len(answer):].strip()
            time.sleep(0.02)
        return None

    t0 = time.time()
    left = False
    why = 'time up'
    moves = {}
    nulls_since, shot = None, False
    # wait for the channel
    while not os.path.exists(log) or 'commands: waiting for' not in open(log, errors='replace').read():
        if proc.poll() is not None or time.time() - t0 > 120:
            why = 'the game did not start'
            break
        time.sleep(0.2)
    for c in first:
        say('%s: %s' % (c, send(c, 'OK ' + c) is not None))
    while why == 'time up' and time.time() - t0 < seconds:
        st = send('state', 'STATE ')
        mv = send('move', 'MOVE ')
        if st is None or mv is None:
            why = 'the game ended' if proc.poll() is not None else 'no answer'
            break
        screen = json.loads(st).get('screen')
        if screen != until:
            left = True
        elif left:
            why = 'reached ' + until
            break
        if mv == 'null':
            # a long stretch with no move: a picture of it, once
            if nulls_since is None:
                nulls_since = time.time()
            elif time.time() - nulls_since > 20 and not shot:
                shot = True
                send('screenshot ' + os.path.join(out, 'stuck.png').replace(os.sep, '/'), 'OK screenshot')
                say('stuck: no move for 20 s; state %s' % st)
            time.sleep(0.05)
            continue
        nulls_since, shot = None, False
        m = json.loads(mv)
        moves[m['what']] = moves.get(m['what'], 0) + 1
        say('%s %s %s' % (screen, m['what'], m.get('word', '')))
        for step in m['input']:
            if 'click' in step:
                send('click game %d %d' % tuple(step['click']), 'OK click')
            elif 'move' in step:
                send('move game %d %d' % tuple(step['move']), 'OK move')
            elif 'key' in step:
                send('key %s' % step['key'], 'OK key')
            elif 'wait' in step:
                time.sleep(0.01 * step['wait'])
        time.sleep(0.1)
    say('end: %s after %d s, moves %s' % (why, time.time() - t0, json.dumps(moves, sort_keys=True)))
    if proc.poll() is None:
        with open(cf, 'a') as f:
            f.write('quit\n')
        try:
            proc.wait(30)
        except subprocess.TimeoutExpired:
            proc.kill()
    os.remove(cf + '.empty')


if __name__ == '__main__':
    main()
