"""Plays a whole classic game natively with real input, by the port's own bot: the framework's -realinput (SDL's own
event queue, docs/REAL_INPUT.md) fed live through -commands, never -script. Run it under rundesk:

    build/native/rundesk.exe python games/bonnies/tools/realmove.py <out dir> [fullscreen] -- <bonnies.exe> <args...>

It starts the game with -realinput=<out>/empty.txt and -commands=<out>/commands.txt (and BONNIES_STATE for the test
state's buttons), its log in <out>/game.log and its own in <out>/play.log. Each move is the port's: the test command
"move" (game/GameScreen.cpp, testMove: a tutorial balloon's button or word, the best word, or the game's hint) gives
the clicks, which go back as "click game <x> <y>" lines. Around the board it does what a player does: answers dialogs
with their first button that neither leaves the game nor opens anything (realplay.py's DENY), clicks on through the
story's pages, and presses Scramble when the latest words fill no new cell. It starts a classic game from the menu
(the player "classic", Normal), loses and regains the focus once on the first board, and with "fullscreen" goes to
full screen (Alt+Enter) after the first level. It ends when the story is over or the game is lost, with a summary.
"""
import json
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realplay import read_state, allowed  # noqa: E402


def run():
    sep = sys.argv.index('--')
    out = os.path.abspath(sys.argv[1])
    fullscreen = 'fullscreen' in sys.argv[2:sep]
    os.makedirs(out, exist_ok=True)
    empty, commands = os.path.join(out, 'empty.txt'), os.path.join(out, 'commands.txt')
    state_path, game_log = os.path.join(out, 'state.txt'), os.path.join(out, 'game.log')
    for p in (commands, state_path):
        if os.path.exists(p):
            os.remove(p)
    open(empty, 'w').close()
    open(commands, 'w').close()
    env = dict(os.environ, BONNIES_STATE=state_path)
    cmd = sys.argv[sep + 1:] + ['-realinput=' + empty, '-commands=' + commands]
    logf = open(os.path.join(out, 'play.log'), 'w', encoding='utf-8')
    t0 = time.time()

    def log(text):
        logf.write('%.1f %s\n' % (time.time() - t0, text))
        logf.flush()

    with open(game_log, 'w', encoding='utf-8') as gl:
        proc = subprocess.Popen(cmd, env=env, stdout=gl, stderr=subprocess.STDOUT)
    seen = [0]

    def new_lines():
        with open(game_log, encoding='latin-1') as f:
            f.seek(seen[0])
            text = f.read()
        cut = text.rfind('\n') + 1
        seen[0] += len(text[:cut].encode('latin-1'))
        return text[:cut].splitlines()

    def send(line, answer=None, timeout=20):
        """Appends a command and waits for its answer line (OK/ERROR <line>, or one starting with answer)"""
        with open(commands, 'a', encoding='utf-8') as f:
            f.write(line + '\n')
        end = time.time() + timeout
        while time.time() < end:
            for l in new_lines():
                # (the framework's answers come as "INFO: OK <line>", the game's own as they are)
                l = l[6:] if l.startswith('INFO: ') else l
                if l.startswith('OK ' + line) or l.startswith('ERROR ' + line) or (answer and l.startswith(answer)):
                    return l
            if proc.poll() is not None:
                return None
            time.sleep(0.02)
        log('no answer to ' + line)
        return None

    def jstate():
        l = send('state', 'STATE ')
        try:
            return json.loads(l[6:]) if l and l.startswith('STATE ') else None
        except ValueError:
            return None

    def click(x, y):
        send('click game %d %d' % (round(x), round(y)))

    def settle(before):
        """waits for the game to take a move (its score, moves, phase or dialogs change), then for the board to stand"""
        for _ in range(100):
            j = jstate()
            if j is None or [j.get(k) for k in ('level', 'score', 'moves', 'phase', 'modal')] != before:
                break
            time.sleep(0.05)
        board = None
        for _ in range(60):
            j = jstate()
            b = j and j.get('board')
            if b == board:
                break
            board = b
            time.sleep(0.1)

    # the window exists once the state file does
    for _ in range(600):
        if os.path.exists(state_path) or proc.poll() is not None:
            break
        time.sleep(0.1)
    send('focus gained')
    result = {'words': 0, 'levels': [], 'scrambles': 0, 'end': None}
    started = named = seen_game = focus_done = full_done = False
    filled_best, idle, fill_level, last_word, refused, still = -1, 0, 0, '', 0, time.time()
    last_summary = ''
    deadline = time.time() + 4 * 3600
    while proc.poll() is None and time.time() < deadline:
        st = read_state(state_path)
        j = jstate()
        if st is None or j is None:
            time.sleep(0.2)
            continue
        summary = json.dumps([j.get('screen'), j.get('modal'), j.get('level'), j.get('phase'), [b[4] for b in st['buttons']] if st['modal'] else 0])
        if summary != last_summary:
            log('state ' + summary)
            last_summary, still = summary, time.time()
        if j.get('screen') == 'game' and j.get('level') and (not result['levels'] or result['levels'][-1] != j['level']):
            result['levels'].append(j['level'])
            if fullscreen and not full_done and len(result['levels']) == 2:
                log('Alt+Enter: full screen')
                send('keydown Left Alt'); send('key Return'); send('keyup Left Alt')
                full_done = True
                time.sleep(1)
                continue
        if st['modal'] > 0:
            texts = ' / '.join(b[4] for b in st['buttons'])
            if started and not named:
                # the first dialog after Classic Game asks for the player's name (a text box, its OK by Enter)
                time.sleep(0.5)
                log('the name (%s): classic' % texts)
                send('type classic')
                send('key Return')
                named, still = True, time.time()
                time.sleep(0.5)
                continue
            b = next((b for b in st['buttons'] if 'Normal' in b[4]), None) or \
                next((b for b in st['buttons'] if allowed(b[4])), None)
            if b:
                log('dialog (%s): %s' % (texts, b[4]))
                if seen_game and ('turns' in texts.lower() or 'over' in texts.lower()):
                    result['end'] = texts
                click(b[2], b[3])
            elif time.time() - still > 3:
                log('dialog without a button to press (%s): Return' % texts)
                send('key Return')
                still = time.time()
            time.sleep(0.5)
            continue
        if j.get('screen') == 'menu':
            if seen_game:
                result['end'] = result['end'] or 'back at the menu'
                log('the game is over: ' + result['end'])
                break
            b = next((b for b in st['buttons'] if b[4] == 'Classic Game'), None)
            if b and time.time() - still > 1:
                log('menu: Classic Game')
                click(b[2], b[3])
                started = True
                still = time.time()
            time.sleep(0.3)
            continue
        if j.get('screen') != 'game':
            if seen_game and j.get('screen') in ('highscores', 'credits', 'title'):
                result['end'] = result['end'] or ('the story is over: ' + j['screen'])
                log(result['end'])
                break
            if time.time() - still > 4:
                log('before the board (%s): a click on the next arrow' % j.get('screen'))
                click(745, 565)
                still = time.time()
            time.sleep(0.2)
            continue
        seen_game = True
        phase = j.get('phase')
        if phase != 1:
            if time.time() - still > 4:
                log('nothing moves (phase %s): a click on the next arrow' % phase)
                click(745, 565)
                still = time.time()
            time.sleep(0.2)
            continue
        if not focus_done and st['balloon'] and not st['balloon'][0]:
            log('focus lost')
            send('focus lost')
            time.sleep(0.5)
            away = jstate() or {}
            result['paused_away'] = away.get('paused')
            log('while away: paused %s' % away.get('paused'))
            send('focus gained')
            time.sleep(0.5)
            back = jstate() or {}
            result['paused_back'] = back.get('paused')
            log('back: paused %s' % back.get('paused'))
            focus_done = True
            time.sleep(0.5)
            continue
        l = send('move', 'MOVE ')
        m = None
        if l and l.startswith('MOVE {'):
            m = json.loads(l[5:])
        if m and m['kind'] == 'button':
            log('button %d' % m['id'])
            for s in m['input']:
                click(*s['click'])
            still = time.time()
            time.sleep(0.5)
            continue
        if j.get('fill') is not None and not (st['balloon'] and st['balloon'][0]):
            if j['level'] != fill_level:
                fill_level, filled_best = j['level'], -1
            if j['filled'] > filled_best:
                filled_best, idle = j['filled'], 0
            scramble = next((b for b in st['buttons'] if b[1] == 0xcb), None)
            if idle >= (3 if j['fill'] - filled_best <= 3 else 8) and scramble:
                log('scramble: %d cells left, none filled in %d words' % (j['fill'] - j['filled'], idle))
                click(scramble[2], scramble[3])
                result['scrambles'] += 1
                idle = 0
                settle([j.get(k) for k in ('level', 'score', 'moves', 'phase', 'modal')])
                continue
        if not m:
            time.sleep(0.3)
            continue
        # the same word on an unchanged board twice: the game would not take it, so Scramble, as a player would
        if m['word'] == last_word and [j.get(k) for k in ('score', 'moves', 'board')] == refused:
            scramble = next((b for b in st['buttons'] if b[1] == 0xcb), None)
            log('the same word again, refused: ' + ('scramble' if scramble else 'waiting'))
            if scramble:
                click(scramble[2], scramble[3])
                result['scrambles'] += 1
                settle([j.get(k) for k in ('level', 'score', 'moves', 'phase', 'modal')])
                last_word = ''
            else:
                time.sleep(0.5)
            continue
        last_word, refused = m['word'], [j.get(k) for k in ('score', 'moves', 'board')]
        log('%s %s at level %s (score %s, moves %s, filled %s of %s)' % (m['kind'], m['word'], j['level'], j['score'],
                                                                       j['moves'], j['filled'], j['fill']))
        for s in m['input']:
            click(*s['click'])
        result['words'] += 1
        idle += 1
        settle([j.get(k) for k in ('level', 'score', 'moves', 'phase', 'modal')])
    j = jstate() or {}
    send('screenshot')
    result['last'] = {k: j.get(k) for k in ('screen', 'level', 'score', 'moves')}
    log('summary ' + json.dumps(result))
    send('close')
    try:
        proc.wait(30)
    except subprocess.TimeoutExpired:
        proc.kill()
    print('realmove: ' + json.dumps(result))


if __name__ == '__main__':
    run()
