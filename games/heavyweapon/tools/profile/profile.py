#!/usr/bin/env python3
"""Heavy Weapon Deluxe's processor time like for like, the port against the original (docs/COORDINATION.md, "A performance
pass"; games/heavyweapon/docs/GAME_NOTES.md, "Performance").

    python -I games/heavyweapon/tools/profile/profile.py prep <out dir>              the players file and the stretch's input
    python -I games/heavyweapon/tools/profile/profile.py port <out dir> <run dir>    one measured run of the port (real time)
    python -I games/heavyweapon/tools/profile/profile.py orig <out dir> <run dir>    one measured run of the original (origrun, real clock)
    python -I games/heavyweapon/tools/profile/profile.py sum <out dir>               the medians of pairs.sh's runs

The stretch is idplay's level4 scenario (the campaign continued before level 4, the continue box, the armory, the map and then
the port's bot), its input as `idplay.py record level4` wrote it: both games get it at the same update after their loading, the
port at its update count (the anchor, the update after which its loading is done), the original at its update counter (0x56b5b4)
from its own anchor. The bot ran once, in the recording; the measured runs carry only the script's input lines and a `cyclesplit`
or `count` line at each end of the stretch, which is frames FIRST to LAST of level 4's play.

The original runs with idplay's patches, its refresh-rate one too, so that both games play the stretch alike: that patch sets
only the step its UpdateF takes (mSyncRefreshRate, from DDInterface +0x78), one UpdateF(1.0) an update as in the port's
framework. Its drawing is still paced as on a player's 60 Hz screen: WaitForVSync has SexyAppBase::DrawDirtyStuff (0x455b80)
sleep to the display's period (DDInterface +0x7c, which no patch touches) between draws, about 57 draws a second. Without that
patch it would take an UpdateF(1.667) once a drawn frame, its enemies would stand elsewhere from the start, and the bot's
recorded input would lose its tanks at other times. The player has the hints off (prep). An input still lands an update or two
late now and then in a real-time run, so the original's shots need not all hit as the port's do; each run's peeks just outside
the stretch (score, lives, level time) are read beside the port's STATE lines. Counting is the shared one (`-cyclesplit=<file>`
in the port, `--count` in the original; docs/TEST_COMMANDS.md, "The cycle split"), read with tools/cyclesplit.py. pairs.sh
starts a run under pincore, the port under rundesk, the original by origrun.
"""
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'games', 'heavyweapon', 'tools', 'idplay'))
import idplay  # noqa: E402

SCENARIO = 'level4'
FIRST = 1200    # the scenario frame the stretch starts at (label a): level 4 is in play
LAST = 4200     # and ends at (label b), the port's tank still in play (lives 2 to 1 over the stretch)
QUIT = LAST + 100
PERF_RUN = 'heavyweapon-perf'                     # origrun's run folder for these runs
USERS_FIRST_RECORD = 1  # users.dat: a zero byte, then the players' records (HeavyWeaponApp::SaveUsers 0x404c90)
USER_FLAGS = 0x42       # UserRecord::mFlags
FLAG_NO_TIPS = 2        # UserRecord::FLAG_NO_TIPS


def prep(out):
    os.makedirs(out, exist_ok=True)
    scenario = idplay.load_scenario(SCENARIO)
    inputs = os.path.join(idplay.out_dir(SCENARIO), 'input.txt')
    if not os.path.exists(inputs):
        idplay.record(SCENARIO)
    shutil.copyfile(inputs, os.path.join(out, 'input.txt'))
    # The player's hints box unticked (UserRecord::FLAG_NO_TIPS) for both games: in the recording a play tip comes up just
    # before frame 1833 and the bot clicks its OK there; a real-time run of the original can bring the tip an update or two
    # later, the click then misses, and the tip's box stays up with the board stopped under it to the end of the stretch
    users = bytearray(open(idplay.profile_users(scenario.PROFILE), 'rb').read())
    users[USERS_FIRST_RECORD + USER_FLAGS] |= FLAG_NO_TIPS
    open(os.path.join(out, 'users.dat'), 'wb').write(bytes(users))
    anchor = idplay.port_anchor()
    open(os.path.join(out, 'anchor.txt'), 'w').write('%d\n' % anchor)
    text = '\n'.join(idplay.PATCH_LINES) + '\n'
    open(os.path.join(out, 'patches.txt'), 'w', encoding='utf-8', newline='\n').write(text)
    print('inputs', len(load_inputs(out)), 'port anchor', anchor, 'seed', scenario.SEED)


def load_inputs(out):
    inputs = []
    for line in open(os.path.join(out, 'input.txt'), encoding='utf-8'):
        frame, command = line.split(' ', 1)
        inputs.append((int(frame), command.strip()))
    return inputs


def port(out, run):
    os.makedirs(run, exist_ok=True)
    scenario = idplay.load_scenario(SCENARIO)
    anchor = int(open(os.path.join(out, 'anchor.txt')).read())
    user = os.path.join(run, 'user')
    shutil.rmtree(user, ignore_errors=True)
    os.makedirs(user)
    shutil.copyfile(os.path.join(out, 'users.dat'), os.path.join(user, 'users.dat'))
    # windowed, as the original's runs (origrun's ScreenMode 0), the players file's player chosen, and the 2D display the
    # original runs in on rundesk's desktop (its 3D test fails there: Test3D's FailureReason, CreateDevice E_OUTOFMEMORY);
    # CYC_3D=1 leaves Is3D unset, the port's default, its 3D display
    with open(os.path.join(user, 'registry.ini'), 'w', newline='\n') as registry:
        registry.write('[SOFTWARE\\PopCap\\HeavyWeapon]\nScreenMode=dword:0\nLastUser=dword:0\n' +
                       ('' if os.environ.get('CYC_3D') == '1' else 'Is3D=dword:0\n'))
    lines = ['%d click %d %d' % ((idplay.PORT_INTRO_CLICK_UPDATE,) + idplay.INTRO_POINT)]   # the intro's end, as idplay's
    lines += ['%d %s' % (anchor + frame, command) for frame, command in load_inputs(out) if frame <= QUIT]
    # a state line just outside each end, to see the level in play there (two lines, not a log an update)
    lines += ['%d state' % (anchor + FIRST - 1), '%d cyclesplit a' % (anchor + FIRST), '%d cyclesplit b' % (anchor + LAST),
              '%d state' % (anchor + LAST + 1), '%d quit' % (anchor + QUIT)]
    script = os.path.join(run, 'port.txt')
    open(script, 'w', newline='\n').write('\n'.join(lines) + '\n')
    scale = os.environ.get('CYC_WINDOWSCALE', '1')
    command = [idplay.PORT_EXE, '-gamedir=' + idplay.GAMEDIR, '-userdir=' + user, '-seed=%d' % scenario.SEED,
               '-script=' + script, '-cyclesplit=' + os.path.join(run, 'port.split.jsonl')]
    if scale != '0':
        command.append('-windowscale=' + scale)
    with open(os.path.join(run, 'port.out'), 'w') as handle:
        subprocess.call(command, stdout=handle, stderr=subprocess.STDOUT, cwd=ROOT)


def orig(out, run):
    os.makedirs(run, exist_ok=True)
    scenario = idplay.load_scenario(SCENARIO)
    idplay.prepare()
    idplay.reset_original_state(scenario.PROFILE)
    shutil.copyfile(os.path.join(out, 'users.dat'), os.path.join(idplay.COPY, 'userdata', 'users.dat'))   # prep's, hints off
    timed = [(frame, 'at %d %s' % (idplay.ORIGINAL_ANCHOR + frame - 1, idplay.original_command(command)))
             for frame, command in load_inputs(out) if frame <= QUIT]
    timed += [(FIRST, 'at %d count a' % (idplay.ORIGINAL_ANCHOR + FIRST - 1)), (LAST, 'at %d count b' % (idplay.ORIGINAL_ANCHOR + LAST - 1))]
    # the board, score, level, lives and level time just outside each end (two peeks, which hold the game a moment outside the stretch)
    peek = 'peek 0x56bac0 0x56bafc 0x56bb00 0x56bb04 [0x56bac0]+0x17c'
    timed += [(FIRST - 1, 'at %d %s' % (idplay.ORIGINAL_ANCHOR + FIRST - 2, peek)), (LAST + 1, 'at %d %s' % (idplay.ORIGINAL_ANCHOR + LAST, peek))]
    every = int(os.environ.get('CYC_PEEK_EVERY', '0'))   # a diagnostic run only, every N frames: the board's mPaused and mSuspended (+0x324), mIsOver (+0x20), mMouseDown, mTankDeadTime and mLevelTime, the app's dialogs (mDialogMap's count, app +0x2e8) and mMouseIn
    if every:
        timed += [(frame, 'at %d peek [0x56bac0]+0x324 [0x56bac0]+0x20 [0x56bac0]+0x128 [0x56bac0]+0x80 [0x56bac0]+0x17c 0x56b488 0x56b5f0' % (idplay.ORIGINAL_ANCHOR + frame - 1))
                  for frame in range(FIRST, LAST, every)]
    timed.sort(key=lambda entry: entry[0])   # stable: the inputs of one frame keep their order
    lines = ['focus regain', 'at %d move %d %d' % ((idplay.INTRO_CLICK_UPDATE,) + idplay.INTRO_POINT),
             'at %d click %d %d' % ((idplay.INTRO_CLICK_UPDATE,) + idplay.INTRO_POINT)] + [line for _, line in timed]
    lines += ['at %d count end' % (idplay.ORIGINAL_ANCHOR + QUIT - 1), 'quit']
    script = os.path.join(run, 'orig.txt')
    open(script, 'w', newline='\n').write('\n'.join(lines) + '\n')
    counts = os.path.join(ROOT, 'build', 'origrun', PERF_RUN, 'counts.jsonl')
    if os.path.exists(counts):
        os.remove(counts)
    command = [idplay.ORIGRUN, '--game', idplay.RUN, '--run', PERF_RUN, '--exe', idplay.win_path(idplay.ORIGINAL_EXE),
               '--seed', str(scenario.SEED), '--clock', 'real', '--cursor', 'scripted', '--keys', 'scripted',
               '--reg-map', 'popcap', '--counter', '0x56b5b4', '--update-ms', '10', '--timeout', '600',
               '--patches', idplay.win_path(os.path.join(out, 'patches.txt')), '--count', idplay.win_path(os.path.join(HERE, 'count.txt')),
               '--script', idplay.win_path(script)]
    with open(os.path.join(run, 'orig.out'), 'w') as handle:
        subprocess.call(command, stdout=handle, stderr=subprocess.STDOUT, cwd=ROOT)
    if os.path.exists(counts):
        shutil.copyfile(counts, os.path.join(run, 'orig.split.jsonl'))


def cycle_rate(path):
    """the cycle counter's rate, as tools/cyclesplit.py measures it"""
    import re
    text = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'cyclesplit.py'), path, '--from', 'a', '--to', 'b'],
                          capture_output=True, text=True).stdout
    return float(re.search(r'taken as (\d+) MHz', text).group(1)) * 1e6


def summary(out):
    """the medians of the runs pairs.sh made: for each side the process's cycles an update over the stretch, its parts, its
    share of one core, and the total less the audio thread (the port's mixer, SDL's thread; origrun's stand-in DirectSound
    mixes nothing for the original, so its figure holds no mixer) with the share that leaves"""
    import glob
    import statistics
    sides = {'original': glob.glob(os.path.join(out, 'o[0-9]*', 'orig.split.jsonl')),
             'port, window 1x': glob.glob(os.path.join(out, 'p[0-9]*1', 'port.split.jsonl')),
             'port, default window (2x)': glob.glob(os.path.join(out, 'p[0-9]*0', 'port.split.jsonl')),
             'port, 3D display, window 1x': glob.glob(os.path.join(out, 'q[0-9]*1', 'port.split.jsonl')),
             'port, 3D display, default window': glob.glob(os.path.join(out, 'q[0-9]*0', 'port.split.jsonl'))}
    first = next(path for files in sides.values() for path in files)
    hz = cycle_rate(first)
    print('cycle counter %.0f MHz; the stretch is labels a to b; M cycles an update, the share of one core' % (hz / 1e6))
    print('%-28s %4s %8s %8s %8s %8s %8s %8s %8s %8s %8s' % ('', 'runs', 'update', 'draw', 'present', 'driver', 'audio', 'total',
                                                          'share', 'no audio', 'share'))
    for name, files in sides.items():
        rows = []
        for path in sorted(files):
            text = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'cyclesplit.py'), path, '--from', 'a', '--to', 'b',
                                   '--json', '--hz', str(hz)], capture_output=True, text=True).stdout
            d = json.loads(text)[0]
            rows.append([d['update'] / d['updates'] / 1e6, d['draw'] / d['updates'] / 1e6, d['present'] / d['updates'] / 1e6,
                         d['driver'] / d['updates'] / 1e6, d['audio'] / d['updates'] / 1e6, d['process'] / d['updates'] / 1e6,
                         d['process'] / (d['wall_s'] * hz) * 100, (d['process'] - d['audio']) / d['updates'] / 1e6,
                         (d['process'] - d['audio']) / (d['wall_s'] * hz) * 100, d['updates'], d['draw_calls']])
        if not rows:
            continue
        med = [statistics.median(column) for column in zip(*rows)]
        print('%-28s %4d %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %7.1f%% %8.3f %7.1f%%   (%d updates, %d draws)' % (
            name, len(rows), *med[:9], med[9], med[10]))
        print('%-28s runs: %s; no audio: %s' % ('', ' '.join('%.3f' % row[5] for row in rows),
                                                ' '.join('%.3f' % row[7] for row in rows)))


if __name__ == '__main__':
    verb = sys.argv[1] if len(sys.argv) > 1 else ''
    if verb == 'prep':
        prep(sys.argv[2])
    elif verb == 'port':
        port(sys.argv[2], sys.argv[3])
    elif verb == 'orig':
        orig(sys.argv[2], sys.argv[3])
    elif verb == 'sum':
        summary(sys.argv[2])
    else:
        sys.exit(__doc__)
