"""Phase one of the identical-play pass (docs/COORDINATION.md): the port plays a game by its own bot with real input from the
title on, in 2D at 640x480 (what the original does on rundesk's desktop), and this writes down every input with the update at
which it ran, so that the same inputs can be given to the original at the same updates (origcmp/to_origrun.py). The port's
-trace lines (the board's score and level, and the positions in the game's generator and in Rand()'s, as origrun's --watch reads
them in the original) stay in <out>/game.log.

    python games/astropop/tools/origcmp/record.py --out <dir> [--mode classic|survival] [--moves 60] [--seed 1] [--play-at 2500]

Writes <out>/events.json ({"last": the last update, "events": [[update, "input line"], ...]}) and <out>/game.log. The port
runs under rundesk (tools/playbymove's Game does that), held, fast and seeded; it starts once the machine has room for a game
process (at most 3 at a time).

A line sent after a state answer that says update U runs at U + 1, and each `wait n` moves the next line n updates on (the state
line itself runs at the update it reports): every batch is checked against that, so the updates are exact.
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'playbymove'))
import playbymove  # noqa: E402

INPUT_VERBS = ('move', 'click', 'down', 'up', 'key', 'type')

# the flow of the screens, in game pixels (640x480)
TITLE_PLAY = (320, 447)         # "click here to play"
MENU_CLASSIC = (100, 190)       # the main menu's Classic Game
MENU_SURVIVAL = (100, 330)      # ... Survival Game
PILOT_GO_X = (80, 399, 559, 239)  # the pilot selector's GO! of Vector, Sprocket, Turbot and Vixx (y 418)
HELP_CONTINUE = (320, 440)      # the full-screen help's Continue


class Recorder:
    def __init__(self, game):
        self.g = game
        self.u = 0               # the update the last state answer reported
        self.events = []

    def play(self, lines, query=('state',)):
        """Sends the lines (inputs and waits) and then the query lines; returns the answers of the query lines"""
        t = self.u + 1
        events = []
        for l in lines:
            w = l.split()
            if w[0] == 'wait':
                t += int(w[1])
            elif w[0] in INPUT_VERBS:
                events.append([t, l])
        answers = self.g.send(*(list(lines) + list(query)))
        st = json.loads(answers[len(lines)][len('STATE '):])
        if st['appupdates'] != t:
            raise SystemExit('the update model is wrong: expected %d, the game says %d, after %s' % (t, st['appupdates'], lines))
        self.u = t
        self.events += events
        return answers[len(lines):]


def wait_room():
    while subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit', '3'], stdout=subprocess.DEVNULL) != 0:
        time.sleep(15)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'native', 'astropop.exe'))
    ap.add_argument('--out', required=True)
    ap.add_argument('--mode', default='classic')
    ap.add_argument('--moves', type=int, default=60)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--play-at', type=int, default=6000, help='the update of the title click: late enough that the original has loaded (its loading thread takes from 400 to over 2600 updates)')
    ap.add_argument('--max-updates', type=int, default=200000, help='stop at this update, whatever the bot has done')
    ap.add_argument('--registry', default=None,
                    help="the port's registry file at start (none: the port's own defaults, as the original's first run in Windows "
                         "Sandbox has; a file made by astropop.exe -importsaves holds this machine's settings, which the original "
                         "reads from HKLM when it runs on the host)")
    ap.add_argument('--pilot', type=int, default=0, help='the pilot: 0 Vector, 1 Sprocket (2 Turbot and 3 Vixx must have been earned)')
    ap.add_argument('--charge', type=int, default=None,
                    help="the weapon's charge at the start (of 18000): the port's `set charge N`; the original needs the patch list "
                         "tools/origrun/patches/astropop-charge.txt for the same start (run_mode.sh CHARGE=N does both)")
    ap.add_argument('--profile', default=None,
                    help='a profile file (users/data1.usr) the player starts with, as the original does with the same file '
                         '(run_original.sh PROFILE=...): the title then leads to the main menu, no name is typed')
    ap.add_argument('--best', type=int, default=None,
                    help='with --profile: the best level of every pilot, so that a classic game starts at the checkpoint '
                         'after it (4 starts at level 5, 8 at 9 and so on)')
    ap.add_argument('--make-profile', default=None,
                    help='plays the title and the name, opens every pilot (`unlock all`), copies the profile file to this path '
                         'and its registry file (same name, .ini) and stops (no game is played)')
    ap.add_argument('--shot', default=None, help='a screenshot (PNG) when the run stops')
    ap.add_argument('--gap', type=int, default=60, help='updates between the clicks of the flow before the game')
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    userdir = out + '.user'
    shutil.rmtree(userdir, ignore_errors=True)
    os.makedirs(userdir)
    if a.profile:
        os.makedirs(os.path.join(userdir, 'users'))
        shutil.copy(a.profile, os.path.join(userdir, 'users', 'data1.usr'))
        # the player is also named in the registry (Users\LastUser, Users\<name>): the profile's registry file is kept beside it
        if not a.registry and os.path.exists(os.path.splitext(a.profile)[0] + '.ini'):
            a.registry = os.path.splitext(a.profile)[0] + '.ini'
    if a.make_profile:
        a.moves = 0
    if a.registry and os.path.exists(a.registry):
        shutil.copy(a.registry, os.path.join(userdir, 'registry.ini'))
    else:
        # 2D, as the original runs in Windows Sandbox (no Direct3D there): the port's registry says Is3D=0
        with open(os.path.join(userdir, 'registry.ini'), 'w') as f:
            f.write('[SOFTWARE\\PopCap\\AstroPop]\nIs3D=dword:0\n')
    if a.best:
        # the best level of every pilot (the profile's values 8 to 11, kept in the registry): a classic game then asks
        # where to start, and the run takes the checkpoint (the level after the last multiple of four)
        reg = os.path.join(userdir, 'registry.ini')
        lines = open(reg).read().splitlines()
        users = False
        for i, l in enumerate(lines):
            if l.startswith('['):
                users = 'Users' in l.strip('[]').split(chr(92))[:-1]
            elif users and l.split('=')[0] in ('8', '9', '10', '11'):
                lines[i] = '%s=dword:%d' % (l.split('=')[0], a.best)
        with open(reg, 'w') as f:
            f.write(''.join(x + chr(10) for x in lines))
    shutil.copy(os.path.join(userdir, 'registry.ini'), os.path.join(out, 'registry-start.ini'))
    ns = argparse.Namespace(dir=out, timeout=120, exe=a.exe, realtime=False,
                            game_args=['-seed=%d' % a.seed, '-trace', '-userdir=' + userdir])
    wait_room()
    g = playbymove.Game(ns)
    r = Recorder(g)
    moves = 0
    state = {}
    stop = None
    try:
        g.send('state')
        r.u = 0
        # the title: the port has loaded long before the original (play-at is one at which the original takes the click)
        st = json.loads(g.send('state')[0][len('STATE '):])
        r.u = st['appupdates']
        r.play(['wait %d' % (a.play_at - 1 - r.u)])
        r.play(['click game %d %d' % TITLE_PLAY, 'wait %d' % a.gap])
        if not a.profile:
            r.play(['type T', 'type e', 'type s', 'type t', 'wait 5', 'key Return', 'wait %d' % a.gap])
        if a.make_profile:
            r.play(['unlock all', 'wait 5'])
        r.play(['click game %d %d' % (MENU_CLASSIC if a.mode == 'classic' else MENU_SURVIVAL), 'wait %d' % a.gap])
        ans = r.play(['click game %d 418' % PILOT_GO_X[a.pilot], 'wait %d' % a.gap])
        gates = json.loads(ans[0][len('STATE '):]).get('checkpoint')
        if gates:
            # the box of the two gates: the second one goes on from the checkpoint
            r.play(['click game %d %d' % (gates[1], gates[2]), 'wait %d' % a.gap])
        charged = a.charge is None      # the charge is set with the first move of the game itself (see below)
        r.play(['click game %d %d' % HELP_CONTINUE, 'wait 20'])
        # the bot, as tools/playbymove's loop plays it
        batch, nulls = [], 0
        while True:
            ans = r.play(batch, ('state', 'move'))
            state = json.loads(ans[0][len('STATE '):])
            if state.get('over') is True:
                stop = 'over'
                break
            if moves >= a.moves:
                stop = 'limit'
                break
            m = json.loads(ans[1][len('MOVE '):])
            if m is None:
                nulls += 1
                if nulls >= 2:
                    stop = 'no move'
                    break
                batch = ['wait 30']
                continue
            nulls = 0
            lines = playbymove.move_lines(m)
            if lines:
                moves += 1      # a "wait" move (the board is busy) has no input and does not count
            elif r.u > a.max_updates:
                stop = 'update limit'
                break
            batch = []
            if not charged and m.get('kind') == 'shoot':
                # the charge of the weapon is full from the start of the game in the original (the patch list astropop-charge.txt
                # stores it at the end of the demo and in the constructor); nothing reads it before the first pickup
                batch.append('set charge %d' % a.charge)
                charged = True
            for line in lines:
                if batch and not batch[-1].startswith('wait ') and not line.startswith('wait '):
                    batch.append('wait 1')
                batch.append(line)
            upd = m.get('updates', 1)
            batch.append('wait %d' % (upd if isinstance(upd, int) and upd > 0 else 1))

        with open(os.path.join(out, 'events.json'), 'w') as f:
            json.dump({'last': r.u, 'mode': a.mode, 'events': r.events}, f)
        print('%s: %d moves, stop %s, last update %d, %d inputs; score %s level %s' % (
            a.mode, moves, stop, r.u, len(r.events), state.get('score'), state.get('level')))
    finally:
        g.finish(a.shot)
        if a.make_profile:
            shutil.copy(os.path.join(userdir, 'users', 'data1.usr'), a.make_profile)
            shutil.copy(os.path.join(userdir, 'registry.ini'), os.path.splitext(a.make_profile)[0] + '.ini')


if __name__ == '__main__':
    main()
