"""play.py <base steps> <out steps> <until> [profile] [max steps]: steps the original can play, made by the port's bot.
The base steps lead to a screen (the map, a mini-game); then, one step at a time, the port runs the steps so far
(port.sh, -fast, from the start each time, so every run replays the one before exactly) and its board log (-boardlog,
GameApp's LogBoardForTests) says what is on the screen, and the next step is what tools/realrun.py's `chapter` does
there: a dialog's first button, the tutorial's tile or an arrow's hot spot, a lost turn's grid, the board's best word
spelled and Attack (Scramble without one), the treasure room's treasures and Continue, Enter on the map, and a click
at 400,150 to move a conversation on. Each step is timed by the game's updates, so the original, given the same
steps, sees the same screens.

--resume after the arguments goes on from a base that is an earlier play.py's steps, all of whose log is taken as
answered.

<until>: "chapter" ends once the battles have given way to another screen (the treasure room, the map); a number ends
after that many seconds of game time. The steps are written to <out steps> after every step, so a run cut short still leaves them.

The original deals the same boards only with tools/origrun/patches/bookwormadv.txt's ChapterLoadScreen patch (no hint
drawn while a chapter loads) and the port's native load, under 50 updates, so neither draws while a chapter loads."""
import os
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))


def pt(t):
    return tuple(int(v) for v in t.split(',')) if ',' in t else None


def parse(path):
    """The board log as dicts, as realrun.py's log_lines reads it."""
    out = []
    if not os.path.exists(path):
        return out
    for line in open(path, encoding='utf-8', errors='replace'):
        f = line.split()
        if len(f) < 3:
            continue
        kind, n = f[0].rstrip(':'), int(f[1])
        d = {'kind': kind, 'n': n}
        if kind == 'screen':
            d['name'] = f[2]
        elif kind == 'treasure':
            d.update(cont=pt(f[3]), enabled=f[4] == '1')
        elif kind == 'talk':
            d['on'] = f[2] == '1'
        elif kind == 'tutorial':
            d['at'] = next((pt(t) for t in f[3:] if ',' in t), None)
        elif kind == 'hint':
            d['spots'] = [pt(t) for t in f[2:] if ',' in t]
        elif kind == 'lostturn':
            d['at'] = pt(f[2])
        elif kind == 'dialog':
            d.update(id=int(f[2]), ok=pt(f[4]))
        elif kind == 'board':
            k = f.index('attack')
            d.update(word=f[2], tiles=[pt(t) for t in f[3:k]], attack=pt(f[k + 1]), scramble=pt(f[k + 3]),
                     words=[], letters=[])
            if 'words' in f and f[f.index('words') + 1] != '-':
                d['words'] = f[f.index('words') + 1].split(',')
            if 'letters' in f and f[f.index('letters') + 1] != '-':
                d['letters'] = [(e.split('@')[0], pt(e.split('@')[1])) for e in f[f.index('letters') + 1].split(';')]
        elif kind == 'wordmaster':
            k = f.index('submit') if 'submit' in f else f.index('rows')
            d.update(word=f[2], tiles=[pt(t) for t in f[3:k]], attack=pt(f[k + 1]) if f[k] == 'submit' else None,
                     scramble=None)
        out.append(d)
    return out


def spell(word, letters):
    """The tiles that spell word from letters [(letter(s), (x, y))], the first that fits at each place (realrun.py's)."""
    used, order, i = set(), [], 0
    while i < len(word):
        for t, (l, xy) in enumerate(letters):
            if t not in used and word.startswith(l, i):
                used.add(t)
                order.append(xy)
                i += len(l)
                break
        else:
            return None
    return order


def run(steps, profile, name):
    path = os.path.join(ROOT, 'build', 'coverage', 'bookwormadv', 'scripts', name + '.steps.txt')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    # (gen.py's quit comes 500 ms after the last step: the log is what the screen showed then)
    open(path, 'w', newline='\n').write('\n'.join(steps + ['quit']) + '\n')
    cmd = ['bash', 'games/bookwormadv/tools/coverage/port.sh', name, path] + ([profile] if profile else [])
    subprocess.run(cmd, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return parse(os.path.join(ROOT, 'build', 'coverage', 'bookwormadv', 'port', name, 'board.log'))


def main():
    base, out, until = sys.argv[1], sys.argv[2], sys.argv[3]
    profile = sys.argv[4] if len(sys.argv) > 4 and sys.argv[4] != '-' else None
    most = int(sys.argv[5]) if len(sys.argv) > 5 else 400
    name = 'play_' + os.path.splitext(os.path.basename(out))[0]
    steps = [l.rstrip('\n') for l in open(base) if l.split('#', 1)[0].strip() and l.split()[0] != 'quit']
    t = 0       # game time of the steps, in ms (gen.py's reckoning)
    for s in steps:
        f = s.split('#', 1)[0].split()
        t = max(t, int(f[1])) if f[0] == 'at' else t + int(f[1]) if f[0] == 'wait' else t + 50 if f[0] == 'click' else t
    used, seen, picks, words, idle = 0, False, 0, [], 0
    if '--resume' in sys.argv:
        # the base is an earlier play.py's steps: what its log shows has been answered
        used = max([l['n'] for l in run(steps, profile, name)] or [0])
    for k in range(most):
        log = run(steps, profile, name)
        screens = [l['name'] for l in log if l['kind'] == 'screen']
        scr = screens[-1] if screens else 'none'
        fresh = [l for l in log if l['n'] > used]
        seen = 'battle' in screens or 'minigame' in screens     # (one may come and go between two steps)
        if seen and scr not in ('battle', 'minigame', 'none', 'loading') and until == 'chapter':
            print('step %d: the chapter is over: %s' % (k, scr))
            break
        if until.isdigit() and t >= int(until) * 1000:
            print('step %d: %d s of game time' % (k, t // 1000))
            break
        talk = [l['on'] for l in log if l['kind'] == 'talk']
        act = next((l for l in fresh if l['kind'] == 'dialog' or l['kind'] == 'tutorial' and l['at'] or
                    l['kind'] == 'hint' and l['spots'] or l['kind'] == 'lostturn' and scr == 'battle'), None)
        board = ([l for l in fresh if l['kind'] in ('board', 'wordmaster')] or [None])[-1]
        last = ([l for l in log if l['kind'] in ('board', 'lostturn')] or [None])[-1]
        if act is None and board is None and scr == 'battle' and last and last['kind'] == 'lostturn':
            act = last      # a lost turn still up (it is logged once a round): its grid again
        new = []
        if act:
            used = act['n']
            xy = {'dialog': act.get('ok'), 'tutorial': act.get('at'), 'lostturn': act.get('at'),
                  'hint': (act.get('spots') or [None])[0]}[act['kind']]
            new = ['click %d %d   # %s' % (xy + (act['kind'],)), 'wait %d' % (1500 if act['kind'] in ('dialog', 'lostturn') else 1000)]
        elif board and scr in ('battle', 'minigame'):
            used = board['n']
            if scr == 'minigame' and board['kind'] == 'board':
                # a mini-game wants a word it has not had yet (Letter Rip deals one word to take others from)
                fresh_word = next((w for w in board['words'] if w not in words and spell(w, board['letters'])), None)
                board['word'] = fresh_word or '-'
                board['tiles'] = spell(fresh_word, board['letters']) if fresh_word else []
            if board['word'] != '-':
                words.append(board['word'])
                for x, y in board['tiles']:
                    new += ['click %d %d' % (x, y), 'wait 300']
                new += ['wait 400']
                if board['attack']:
                    new += ['click %d %d   # Attack: %s' % (board['attack'] + (board['word'],))]
            elif board['scramble']:
                new += ['click %d %d   # Scramble' % board['scramble']]
            new += ['wait 2500']
        elif scr == 'treasure' and not seen:
            tl = ([l for l in log if l['kind'] == 'treasure'] or [None])[-1]
            if tl and tl['enabled']:
                new = ['click %d %d   # Continue' % tl['cont'], 'wait 1500']
            elif tl:
                new = ['click 188 %d   # a treasure' % (113, 209, 300, 402)[picks % 4], 'wait 1500']
                picks += 1
            else:
                new = ['wait 1500']
            used = max([l['n'] for l in log] or [used])
        elif scr == 'map' and not seen:
            new = ['click 400 575   # Enter!', 'wait 3000']
            used = max([l['n'] for l in log] or [used])
        elif talk and talk[-1]:
            new = ['click 400 150   # the conversation on', 'wait 3000']
            used = max([l['n'] for l in log] or [used])
        else:
            # nothing asks for a click: realrun clicks the conversation's place every 3 s
            idle += 1
            new = ['click 400 150', 'wait 3000'] if idle % 2 == 0 else ['wait 1500']
        if any(s.startswith('click') for s in new):
            new = ['wait 500'] + new    # not before the moment the log was read
        for s in new:
            f = s.split('#', 1)[0].split()
            t += int(f[1]) if f[0] == 'wait' else 50
        steps += new
        print('step %d: %s %s' % (k, scr, ' | '.join(s for s in new if not s.startswith('wait'))[:120]))
        sys.stdout.flush()
        open(out, 'w', newline='\n').write('\n'.join(steps + ['wait 2000', 'quit']) + '\n')
    open(out, 'w', newline='\n').write('\n'.join(steps + ['wait 2000', 'quit']) + '\n')
    print('%d words: %s' % (len(words), ' '.join(words)))


main()
