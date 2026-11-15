#!/usr/bin/env python3
"""Venice Deluxe's phase-two shots with the original in its 3D mode: the original runs on the visible desktop under tools/visrun.py
(docs/VISRUN.md; neither Windows Sandbox nor rundesk gives it Direct3D, docs/ORIGRUN.md "Hardware acceleration"), the port runs in
its own 3D mode at the updates the original's steps landed on, with the ticks the original ran, and tools/shotdiff compares the two
folders. It is idplay's method (idplay.py) with visrun for the original's side.

    python -I games/venice/tools/idplay/visshots.py prep <scenario>                     the player's profile, the step file and
                                                                                         the run's config, then visrun's --dry-run
    python -I games/venice/tools/idplay/visshots.py original <scenario> [visrun options]  the original's run (--user-window-until HH:MM
                                                                                         outside 22:00 to 08:00; the options are visrun's)
    python -I games/venice/tools/idplay/visshots.py port <scenario>                     the port's run and its shots
    python -I games/venice/tools/idplay/visshots.py compare <scenario>                  the port's trace against the original's peeks (the seed,
                                                                                         the last scene's ticks, the ticks each frame ran)
    python -I games/venice/tools/idplay/visshots.py diff <scenario>                     shotdiff on the two folders

The scenario is an idplay scenario of `scenarios/` (shots_menus3d, ...): its INPUTS and SHOTS become the step file. The original
loads on a thread of real time, so the update its loading ends at varies from run to run: the steps are at START (1500) plus
the scenario's frame, by when the loading is done in every run (it ended at update 1132 in the 3D run, 936 in the 2D one; `port` stops if a run
ended it later), and the step file peeks the game's values at every update, which is how the first update after the loading (the port's frame 0)
and the ticks each update ran are found (idplay.first_running_frame, idplay.tick_schedule; schedule() below corrects the frames where the
original reset its tick base). The port plays that tick schedule (the original's virtual clock gives an update a variable number of 4 ms
ticks) and takes each input and shot at the update the original's step landed on. The original's shots are `--shape sandbox`, as
shotdiff's Venice settings expect them.
"""
import os
import re
import shutil
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..', '..'))
sys.path.insert(0, HERE)
import idplay  # noqa: E402
idplay.MAX_CATCH_UP = 1000  # the most the tick counter steps between two of visrun's updates and still counts as ticks run: the widget gives up the missed ticks
                            # past 100 (GiveUpBehindUpdates, VeniceApp.cpp), but visrun's update is the app's, and the 3D original's scene time and counter both
                            # advance by 224 or 235 in one of them while the batch it logs is 1 to 3 (several widget updates between two peeks); a reset of the
                            # tick base shows as a jump of about 29 million, far over this

BASE_CONFIG = os.path.join(ROOT, 'games', 'venice', 'tools', 'visrun', 'venice.toml')
VISRUN = os.path.join(ROOT, 'tools', 'visrun.py')
START = 1500               # the update by which the loading is done in every run (it ended at update 1132 in the 3D run with a peek at every update, 936 in the 2D one; idplay's start is 3000)
ORIGINAL_TIMEOUT = 600     # seconds one wait for an update may take (visrun's --timeout)


def names(scenario_argument):
    scenario = idplay.load_scenario(scenario_argument)
    name = idplay.scenario_name(scenario_argument)
    prep_dir = os.path.join(idplay.RUN_DIR, name)
    os.makedirs(prep_dir, exist_ok=True)
    return scenario, name, prep_dir


def run_dir(name):
    return os.path.join(ROOT, 'build', 'visrun', 'venice', name)


def write_steps(name, scenario, prep_dir):
    """the step file: every update from 1 to the scenario's last has a peek (the values the config's [values] names), and the inputs
    and shots are at START plus their frame, an input before the peek of its update and a shot after it, as idplay's original script
    has them"""
    if getattr(scenario, 'FOCUS', None):
        sys.exit('a scenario with focus losses is not run on the desktop')
    inputs = {}
    for frame, original_command, _ in scenario.INPUTS:
        if original_command:
            inputs.setdefault(START + frame, []).append(original_command)
    shots = {}
    for frame, shot_name in scenario.SHOTS:
        shots.setdefault(START + frame, []).append(shot_name)
    lines = ['# made by visshots.py from scenarios/%s.py: the steps at update %d plus the scenario\'s frame, a peek at every update' % (name, START)]
    mouse = getattr(scenario, 'MOUSE_AT', None)
    if mouse:
        lines.append('move %d %d' % mouse)
    for update in range(1, START + scenario.END + 1):
        for command in inputs.get(update, []):
            lines.append('at %d %s' % (update, command))
        lines.append('at %d peek' % update)
        for shot_name in shots.get(update, []):
            lines.append('at %d shot %s' % (update, shot_name))
    lines.append('quit')
    path = os.path.join(prep_dir, 'visrun-steps.txt')
    with open(path, 'w') as out:
        out.write('\n'.join(lines) + '\n')
    return path


def write_config(name, scenario, prep_dir):
    """the run's copy of the base config: the profile the scenario starts with, seeded into the original's profile folder"""
    profile = idplay.start_profile(name, scenario)
    base = open(BASE_CONFIG, encoding='utf-8').read()
    if profile:
        seed = "save_seed = [['%s', '%s']]" % (profile, idplay.ORIGINAL_PROFILES)
    else:
        seed = '# no profile: a new player'
    path = os.path.join(prep_dir, 'venice-run.toml')
    with open(path, 'w', encoding='utf-8') as out:
        out.write(base.replace('@SAVE_SEED@', seed))
    return path


def visrun_command(name, config, steps, extra=()):
    return [sys.executable, '-I', VISRUN, config, steps, '--out', run_dir(name), '--seed', idplay.SEED, '--3d', 'on', '--shape', 'sandbox',
            '--timeout', str(ORIGINAL_TIMEOUT)] + list(extra)


def prep(scenario_argument):
    scenario, name, prep_dir = names(scenario_argument)
    if hasattr(scenario, 'PROFILE_COMMANDS'):
        idplay.make_profile(name, scenario)
    steps = write_steps(name, scenario, prep_dir)
    config = write_config(name, scenario, prep_dir)
    print('steps', steps, '\nconfig', config)
    return subprocess.call(visrun_command(name, config, steps, ['--dry-run']), cwd=ROOT)


def original(scenario_argument, extra):
    scenario, name, prep_dir = names(scenario_argument)
    steps = os.path.join(prep_dir, 'visrun-steps.txt')
    config = os.path.join(prep_dir, 'venice-run.toml')
    if not (os.path.exists(steps) and os.path.exists(config)):
        sys.exit('run prep first')
    os.makedirs(run_dir(name), exist_ok=True)
    return subprocess.call(visrun_command(name, config, steps, extra), cwd=ROOT)


def read_steps(path):
    """the rows of visrun's steps.tsv that were asked for at an update: (wanted update, landed update, command, arguments, values),
    the values as {name: signed 32-bit int}, as the port's trace and origrun's peeks print them (visrun prints them unsigned; a null read, '-', is 0)"""
    rows = []
    for line in open(path, encoding='utf-8', errors='replace'):
        fields = line.rstrip('\n').split('\t')
        if len(fields) >= 3 and re.fullmatch(r'-?\d+', fields[0]) and re.fullmatch(r'-?\d+', fields[1]):
            values = {}
            for item in (fields[5] if len(fields) > 5 else '').split(' '):
                if '=' in item:
                    key, value = item.rsplit('=', 1)
                    values[key] = 0 if value == '-' else int(value) - (1 << 32 if int(value) >= 1 << 31 else 0)
            rows.append((int(fields[0]), int(fields[1]), fields[2], fields[3] if len(fields) > 3 else '', values))
    return rows


def original_records(name):
    """idplay's records of the original (update -> its values) from the peeks, and what the steps landed on: the update of each
    input as idplay's holds ((asked update, the command) -> landed) and of each shot (its name -> landed)"""
    records, holds, landed = {}, {}, {}
    for wanted, update, command, arguments, values in read_steps(os.path.join(run_dir(name), 'steps.tsv')):
        if command == 'peek':
            records[update] = values
        elif command == 'shot':
            landed[arguments.split()[0]] = update
        else:
            holds[(wanted, ('%s %s' % (command, arguments)).strip())] = update
    return records, holds, landed


def scene_ticks(bits):
    """the last scene's tick count (the peek's `scene`, float bits)"""
    return struct.unpack('<f', struct.pack('<i', bits))[0]


def schedule(records, first, last):
    """the ticks each frame of the port runs: the original's (idplay.tick_schedule), with the frames where the original reset its tick base
    corrected. The tick counter reads 1 after such a frame (the reset came in the loop, on its last tick), so the ticks that frame ran are
    not in it; what is known is how far the clock moved over that frame and the next (the counter after the next minus the counter before
    this one) and how many of those ticks the next ran (the new last scene's count at the next frame, which is the scene's count at the
    reset frame plus its ticks): the 3D tour's credits click is 22 ticks over the two, one after the scene was inserted, so the frame that
    inserted it ran 21 and the loading screen ended on the same tick as in 2D"""
    ticks = idplay.tick_schedule(records, first, last)
    for frame in range(first + 1, last):
        before, now, after = records.get(frame - 1), records.get(frame), records.get(frame + 1)
        if before is None or now is None or after is None or not before['counter'] or now['counter'] >= before['counter'] - 1000000:
            continue   # not a reset frame (the counter reads the clock, about 29 million, but for a little after a reset)
        if after['tail'] != now['tail'] or not after['counter'] or after['counter'] < before['counter']:
            continue   # the scene changed again, or the next frame was not counting from the clock: the batch rule stands
        second = int(round(scene_ticks(after['scene']) - scene_ticks(now['scene'])))
        total = after['counter'] - before['counter']
        if 0 <= second <= total:
            ticks[frame - first] = total - second
            ticks[frame + 1 - first] = second
    return ticks


def port(scenario_argument):
    scenario, name, prep_dir = names(scenario_argument)
    records, holds, landed = original_records(name)
    first = idplay.first_running_frame(records)
    if first is None:
        sys.exit('the original never ran: no update with the game running in the peeks (the run did not finish?)')
    if first >= START:
        sys.exit('the original ran from update %d, not before START %d: raise START' % (first, START))
    last = first + scenario.END
    if last not in records:
        sys.exit('the original stopped before update %d: the run did not finish' % last)
    ticks = schedule(records, first, last)
    shots_dir = os.path.join(idplay.ROOT, 'build', 'ph2', name, 'port')
    shutil.rmtree(shots_dir, ignore_errors=True)
    os.makedirs(shots_dir)
    idplay.wait_for_slot()
    idplay.run_port(name, scenario, START, first, holds, ticks, os.path.join(prep_dir, 'port-3d.out'), shots_dir=shots_dir.replace('\\', '/'),
                    landed=landed)
    print('the original ran from update %d; %d shots of the port' % (first, len(os.listdir(shots_dir))))


def compare(scenario_argument):
    """the original's values at every update (the peeks) against the port's trace of the run `port` made, as idplay.compare does for
    the sandbox: the random seed, the last scene's tick count and the ticks each frame ran; the first updates that part"""
    scenario, name, prep_dir = names(scenario_argument)
    records, _, _ = original_records(name)
    first = idplay.first_running_frame(records)
    port_records = idplay.parse_port(os.path.join(prep_dir, 'port.trace'))
    last = first + scenario.END
    ticks = schedule(records, first, last)
    fields = ['seed', 'scene']
    different = compared = 0
    for offset in range(0, scenario.END + 1):
        frame = first + offset
        if frame not in records:
            continue
        if offset not in port_records:
            print('frame %d (original %d): missing in the port' % (offset, frame))
            break
        compared += 1
        bad = [field for field in fields if field in records[frame] and records[frame][field] != port_records[offset][field]]
        if port_records[offset]['ticks'] != ticks[offset]:
            bad.append('ticks')
        if bad:
            different += 1
            if different <= 5:
                print('frame %d (original %d): differs in %s' % (offset, frame, ', '.join(bad)))
                for field in bad:
                    original_value = ticks[offset] if field == 'ticks' else records[frame][field]
                    print('    %s: original %d, port %d' % (field, original_value, port_records[offset][field] if field != 'ticks' else port_records[offset]['ticks']))
    print('%s: the original ran from update %d; compared %d frames: %d differ' % (name, first, compared, different))
    return 1 if different else 0


def diff(scenario_argument):
    scenario, name, prep_dir = names(scenario_argument)
    original_shots = os.path.join(idplay.ROOT, 'build', 'ph2', name, 'orig')
    shutil.rmtree(original_shots, ignore_errors=True)
    os.makedirs(original_shots)
    for file_name in os.listdir(run_dir(name)):
        if file_name.endswith('.png'):
            shutil.copy(os.path.join(run_dir(name), file_name), original_shots)
    port_shots = os.path.join(idplay.ROOT, 'build', 'ph2', name, 'port')
    print('%d shots of the original, %d of the port' % (len(os.listdir(original_shots)), len(os.listdir(port_shots))))
    return subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'venice', '--orig', original_shots,
                            '--port', port_shots, '--run', name], cwd=ROOT)


if __name__ == '__main__':
    if len(sys.argv) < 3 or sys.argv[1] not in ('prep', 'original', 'port', 'compare', 'diff'):
        sys.exit(__doc__)
    verb, scenario_argument = sys.argv[1], sys.argv[2]
    if verb == 'original':
        sys.exit(original(scenario_argument, sys.argv[3:]))
    sys.exit({'prep': prep, 'port': port, 'compare': compare, 'diff': diff}[verb](scenario_argument))
