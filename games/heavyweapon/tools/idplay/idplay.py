#!/usr/bin/env python3
"""Phase one of the identical-play pass (docs/COORDINATION.md; games/heavyweapon/docs/IDENTICAL_PLAY.md): the original and the
port play the same input, and the game's state is compared update by update.

    idplay.py prepare                     the original's copy made ready (see GAME_NOTES.md, "The original under origrun")
    idplay.py record <scenario>           the port's bot plays the scenario: its input, by update, into input.txt
    idplay.py original <scenario>         the original plays input.txt under origrun, its state read after every update
    idplay.py port <scenario>             the port plays the same input at the updates the original took it, with its trace
    idplay.py compare <scenario>          the two compared
    idplay.py run <scenario>              all four (record only when there is no input.txt yet, or with --record)

Both games are counted from the anchor: the update whose end first finds the loading done (the app's +0x91c, the title told).
The intro is ended by a click at update 50 in both, and nothing before the anchor draws a random number; the anchor is
update 51 in the port and 52 in the original. Frame f is the anchor's update plus f. An input the port's script gives at
the start of frame f's update is posted to the original at its wait after frame f - 1 ("at"). The original waits, and
reads its messages, after five updates in eight (WAITS: a frame every 16 ms of origrun's virtual clock, an update every
10 ms), so the record run gives the bot's input, and the opening's, only at a frame that follows one of those waits
(botrecord's period and residues, read_frame), and the original reads every input before the same update as the port.
The update each input was read before is measured all the same, from the WidgetManager's last-input count at the peeks
after the hold (parse_original), the port's replay gives each input there, and `port` counts those that came later.
`compare` also checks the replay's trace against the record run's: the same trace means the games compared are the bot's
own, not a replay that has drifted from what its input was chosen for.

A record run ends the frames compared GAME_OVER_MARGIN after the bot's game is over (end.txt), or at the scenario's END.

A peek the original takes at a wait inside an update (the sound code waits as a sound starts) shows the update half done;
the comparison counts such a one-peek difference apart, since it agrees again at the next peek.

The original's copy: build/origrun/heavyweapon/game, with the seed, loading, mouse and refresh-rate patches
(tools/origrun/patches/heavyweapon.txt, written by `prepare`), on the virtual clock with the scripted cursor and keys, and its
HKLM settings mapped to HKCU.

A scenario is a python file under scenarios/ with END (the last frame compared), OPENING (a list of (frame, port command):
the title's play, the name typed, the menu's button, each moved on to the next frame the original reads input before),
BOT_FROM (the frame the bot starts on) and optionally PROFILE (a players file the port makes first: unlocked or
level<n>, see profile_users) and SEED (both games' seed, 1 when not given).
"""

import argparse
import importlib.util
import io
import os
import re
import shutil
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
HERE = os.path.dirname(os.path.abspath(__file__))
OUT_ROOT = os.path.join(ROOT, 'build', 'logs', 'idplay')
ORIGRUN = os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')
RUN = 'heavyweapon'
COPY = os.path.join(ROOT, 'build', 'origrun', RUN, 'game')
ORIGINAL_EXE = os.path.join(COPY, 'HeavyWeaponGame.exe')
PATCHES = os.path.join(ROOT, 'tools', 'origrun', 'patches', 'heavyweapon.txt')
PORT_EXE = os.path.join(ROOT, 'build', 'native', 'heavyweapon.exe')
RUNDESK = os.path.join(ROOT, 'build', 'native', 'rundesk.exe')
GAMEDIR = os.path.join(ROOT, 'assets', 'heavyweapon')
REGISTRY_KEY = r'HKCU\SOFTWARE\PopCap\HeavyWeapon'   # the original's HKLM key, mapped by origrun's --reg-map popcap

INTRO_CLICK_UPDATE = 50                 # the original's click that ends the intro (it takes the focus first)
# The port's comes an update later: the port takes a script click in the update it is given, the original reads a posted
# one at its wait after it, before the next. The two anchors, and so the app's update counts at every frame, are then
# the same, and the game reads that count: the Eye's and the final boss's retargets, the head's bombs, the worm's sand.
PORT_INTRO_CLICK_UPDATE = INTRO_CLICK_UPDATE + 1
INTRO_POINT = (320, 240)
ORIGINAL_ANCHOR = 52                    # the update after which the original's loading is done (51 at 60 Hz); the
                                        # port's is measured (port_anchor), 52 too
# The original under origrun draws a frame every 16 ms of its virtual clock and updates every 10 ms, so it waits (and reads
# its messages) after five updates in eight: these, counted from its anchor. An input posted at such a wait is read before
# the next update, so the bot is asked, and the opening's input given, only before the update after one of them.
WAIT_PERIOD = 8
WAITS = (0, 2, 4, 5, 7)
LAST_BITS = 1 << 12                    # a tank X that differs by no more units in its last place (see compare)
MARGIN = 2                              # updates run after the last frame before the games are closed
GAME_OVER_MARGIN = 500                  # frames compared after the bot's game is over

# The original's patches (va, expected bytes, replacement); {seed} is origrun's --seed as four little-endian bytes
PATCH_LINES = [
    'seed HeavyWeaponGame.exe va:0x45cb5f 8b3d68705200ffd7 b8{seed}89c790 SexyAppBase::Init (0x45c970) seeds Rand() and '
    'mRandSeed with GetTickCount(): the run seed instead, as the port\'s -seed=N; edi then holds the seed instead of '
    'GetTickCount\'s address',
    'crtseed HeavyWeaponGame.exe va:0x45cbd8 ffd7 89f8 the same Init then calls srand(GetTickCount()) through edi: srand(seed) '
    'instead, as the port\'s CrtSRand(mRandSeed)',
    'test HeavyWeaponGame.exe va:0x44ccb3 566a006810cc4400c6867d04000001e84ff90a0083c40c '
    'c6867d0400000156e850ffffff83c40490909090909090 StartLoadingThread (0x44cc90) runs the loading stub (0x44cc10) on the main '
    'thread instead of _beginthread, as the port does, so the loading ends at the same update in every run',
    'mousein HeavyWeaponGame.exe va:0x4506ce ff1568725200 83c4088bc790 the window procedure\'s WM_TIMER (0x44ff50, on the '
    'wall clock) asks WindowFromPoint whether the window is under the cursor, which on origrun\'s hidden desktop it '
    'sometimes is not, and then calls the WidgetManager\'s MouseExit at a random update (a level\'s board waits for the '
    'next mouse move): the game\'s own window instead (the two pushed words dropped), so a cursor inside the client '
    'area is in, as in the port\'s scripted runs',
    'refresh HeavyWeaponGame.exe va:0x44d779 8b4878 6a6459 the refresh rate the game paces its '
    'UpdateF by (mSyncRefreshRate, app +0x530, set at 0x44d770 from DDInterface +0x78: 60 on origrun\'s desktop) is 100, '
    'as the port\'s framework has it: one UpdateF(1.0) after each update. At 60 an UpdateF(1.667) comes after every '
    'one or two updates, so the enemies stand at other places between them (docs/IDENTICAL_PLAY.md)',
    'enemyplace HeavyWeaponGame.exe va:0x4315e2 8b485c8bc6894e505f5e5b5dc20400909090909090909090909090909090 '
    '8b485c894e50895e08895e0c895e10895e148bc65f5e5b5dc20400909090 the Enemy constructor (0x431550) leaves mX and mY '
    '(+0x8, +0x10) unset, and the robot reads its own before it sets them, to place its arms and launchers: they start at '
    '0, as in the port (its last stores moved into the padding after it, with ebx still 0)',
    'robotfoot HeavyWeaponGame.exe va:0x4436eb 899ea0000000899ea4000000c6464801899ea8000000899eac000000899ef0000000 '
    '8dbe98000000891f895f04895f08895f0cc6464801895f10895f14895f5890909090 the robot\'s constructor (0x443210) leaves '
    'its foot\'s y (+0x98) unset, and a stray value of 410 or more has it stamp, a sound and a shake, on its first '
    'update: 0, as in the port (its last stores made through edi, which is restored after them, to make room)',
    'headtarget HeavyWeaponGame.exe va:0x43f41d c786a8000000f40100008bc65e5b64890d000000008be55dc204009090909090909090 '
    '8d9698000000891a895a04c74210f40100008bc65e5b64890d000000008be55dc20400 the head\'s constructor (0x43f310) leaves '
    'its target x (+0x98) unset, and its first update eases toward it: 0, as in the port (the last store made through '
    'edx, free there, to make room)',
]

TRACED_ITEMS = 3


def traced_items(prefix, head, x_offset):
    """(key, expression) for the high words of mX and mY of a board list's first TRACED_ITEMS items: b1x, b1y, b2x...;
    for the projectiles also each one's vtable (p1s...), which `parse_original` makes 1 for the lightning"""
    rows = []
    node_address = '[[0x56bac0]+0x%x]+4' % head          # what it holds is the first node
    for index in range(1, TRACED_ITEMS + 1):
        item = '[%s]+0' % node_address                     # what it holds is that node's item
        rows.append(('%s%dx' % (prefix, index), '[%s]+0x%x' % (item, x_offset + 4)))
        rows.append(('%s%dy' % (prefix, index), '[%s]+0x%x' % (item, x_offset + 0xc)))
        if prefix == 'p':
            rows.append(('%s%ds' % (prefix, index), '[%s]+0x0' % item))
        node_address = '[%s]+4' % node_address
    return rows


# The tank's lightning (StaticStrike, vtable 0x531118) keeps its path in its own arrays and never sets the projectile's mX
# and mY, which hold what the heap had there, in the original as in the port, and nothing reads them: its place counts
# as 0 and its p<n>s is 1, as the port's trace writes it
UNPLACED_VTABLE = 0x531118


# What is read from the original's memory after every update, and the port's trace name (TestCommands.cpp, TraceFrame).
# The app is the static object at 0x56b1a0, its board pointer at +0x920 (0x56bac0). A read through a null pointer is '-', 0.
EXPRESSIONS = [
    ('u', '0x56b5b4'),                  # SexyAppBase::mUpdateCount (app +0x414)
    ('loaded', '0x56babc'),             # mLoadingCompleteShown (app +0x91c, one byte; the three after it are padding)
    ('fidx', '0x5a4258'),               # Rand()'s generator (0x5a3898): its index (+0x9c0) and first word
    ('f0', '0x5a3898'),
    ('gidx', '0x56cff8'),               # the game's own generator (app +0x1498): its index and first word
    ('g0', '0x56c638'),
    ('board', '0x56bac0'),              # mBoard (only whether there is one is compared)
    ('score', '0x56bafc'),              # mScore (app +0x95c)
    ('level', '0x56bb00'),              # mLevel
    ('lives', '0x56bb04'),              # mLives
    ('nukes', '0x56bb08'),              # mPowerUps[POWERUP_NUKES] (app +0x968)
    ('ltime', '[0x56bac0]+0x17c'),      # Board::mLevelTime
    ('tankxl', '[0x56bac0]+0x90'),      # Board::mTankX, a double: its low and high words
    ('tankxh', '[0x56bac0]+0x94'),
    ('tankdead', '[0x56bac0]+0x80'),    # Board::mTankDeadTime
    ('killed', '[0x56bac0]+0x328'),     # Board::mEnemiesDestroyed
    ('spawned', '[0x56bac0]+0x32c'),    # Board::mEnemiesSpawned
    ('enemies', '[0x56bac0]+0x200'),    # Board::mEnemies' count (the list at +0x1f4, its count at +0xc)
    ('boss', '[0x56bac0]+0x2e4'),       # Board::mBoss (only whether there is one)
    ('bosshpl', '[[0x56bac0]+0x2e4]+0x58'),     # the boss's mHealth, a double
    ('bosshph', '[[0x56bac0]+0x2e4]+0x5c'),
    ('mousex', '[0x56bac0]+0x60'),      # Board::mMouseX, mMouseY and mMouseDown (+0x128, its low byte)
    ('mousey', '[0x56bac0]+0x64'),
    ('mousedown', '[0x56bac0]+0x128'),
    ('bullets', '[0x56bac0]+0x1f0'),    # the counts (+0xc) of the board's lists: mBullets +0x1e4, mExplosions +0x204,
    ('explosions', '[0x56bac0]+0x210'), # mProjectiles +0x214, mCraters +0x224, mTankShots +0x234, mShellCasings +0x284,
    ('projectiles', '[0x56bac0]+0x220'),  # mSparks +0x2a4 and the particles' (mParticles +0x2e8, its list +0x4)
    ('craters', '[0x56bac0]+0x230'),
    ('tankshots', '[0x56bac0]+0x240'),
    ('casings', '[0x56bac0]+0x290'),
    ('sparks', '[0x56bac0]+0x2b0'),
    ('particles', '[0x56bac0]+0x2f8'),
] + [
    # the first TRACED_ITEMS of the board's bullets, enemies and projectiles, each by the high words of its mX and mY
    # (doubles: a place to a few millionths of a pixel); a node holds its item at +0 and the next node at +4, from the
    # list's empty head node, and a list with fewer reads 0 for the rest
    (key, expression) for prefix, head, x_offset in (('b', 0x1e4, 0x0), ('e', 0x1f4, 0x8), ('p', 0x214, 0x10))
    for key, expression in traced_items(prefix, head, x_offset)
] + [
    # not compared: the WidgetManager's (app +0x2dc) update count (+0x4) and the count its last input was read at (+0x64,
    # mLastInputUpdateCnt), which give the update before which the original read each input
    ('wmupd', '[0x56b47c]+0x4'),
    ('wminput', '[0x56b47c]+0x64'),
    ('mousein', '0x56b5f0'),            # not compared: SexyAppBase's mMouseIn (app +0x450, its low byte), which stays 1
]
POINTERS = ('board', 'boss')            # compared as 0 or 1
COMPARED = [name for name, _ in EXPRESSIONS if name not in ('u', 'wmupd', 'wminput', 'mousein')]
ITEM_KEYS = {key for prefix in 'bep' for key, _ in traced_items(prefix, 0, 0)}

# origrun reads a script line into 1024 characters and ends a peek's log line once it passes about 920, so an update's
# peek goes in several lines (the rest after the first): `parse_original` joins those with the same counter
SCRIPT_LINE_BUDGET = 1000
LOG_LINE_BUDGET = 880
LOGGED_VALUE_LENGTH = 13                # " <expr>=" and a value of up to 11 characters, beyond the expression


def peek_lines(update):
    """the script's `at <update> peek ...` lines: the state's main values in the first, the items' places after it"""
    groups = [[expression for key, expression in EXPRESSIONS if key not in ITEM_KEYS]]
    current = []
    for key, expression in EXPRESSIONS:
        if key not in ITEM_KEYS:
            continue
        script_length = len('at %d peek' % update) + sum(len(e) + 1 for e in current + [expression])
        log_length = sum(len(e) + LOGGED_VALUE_LENGTH for e in current + [expression])
        if current and (script_length > SCRIPT_LINE_BUDGET or log_length > LOG_LINE_BUDGET):
            groups.append(current)
            current = []
        current.append(expression)
    if current:
        groups.append(current)
    return ['at %d peek %s' % (update, ' '.join(group)) for group in groups]

# the port's script commands and the original's (origrun) for the same input
KEY_CODES = {'RETURN': 13, 'BACK': 8, 'ESCAPE': 27, 'SPACE': 32}


def log(text):
    print('idplay: ' + text, flush=True)


def win_path(path):
    return os.path.abspath(path).replace('/', '\\')


def load_scenario(name):
    path = os.path.join(HERE, 'scenarios', name + '.py')
    spec = importlib.util.spec_from_file_location('scenario_' + name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def out_dir(name):
    path = os.path.join(OUT_ROOT, name)
    os.makedirs(path, exist_ok=True)
    return path


def wait_for_room():
    """CLAUDE.md: at most 6 game processes on the machine"""
    for _ in range(240):
        if subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit', '6'],
                          capture_output=True).returncode == 0:
            return
        time.sleep(15)
    sys.exit('idplay: no room for another game process after an hour')


def original_command(port_command):
    """the origrun script command for a port script command"""
    name, _, rest = port_command.partition(' ')
    if name in ('click', 'move', 'down', 'up'):
        return port_command
    if name == 'char':
        return 'text ' + (' ' if rest == 'SPACE' else rest)
    if name == 'key':
        return 'key %d' % KEY_CODES[rest]
    sys.exit('idplay: no original command for "%s"' % port_command)


# ---------------------------------------------------------------- the original's copy

def prepare():
    if not os.path.exists(ORIGINAL_EXE):
        sys.exit('idplay: no %s: stage and prepare the copy as GAME_NOTES.md says ("The original under origrun")' % ORIGINAL_EXE)
    text = '\n'.join(PATCH_LINES) + '\n'
    if not os.path.exists(PATCHES) or open(PATCHES, encoding='utf-8').read() != text:
        os.makedirs(os.path.dirname(PATCHES), exist_ok=True)
        open(PATCHES, 'w', encoding='utf-8', newline='\n').write(text)
        log('wrote ' + PATCHES)
    reencode_pictures()


def reencode_pictures():
    """The Steam copy's pictures include 1- and 4-bit paletted PNGs, which the original's PNG reader overruns its buffer on
    (it crashes in the loading at the fourth picture). Steam's launcher rewrites a copy's pictures on its first run; the
    copy is never run through it, so these are stored again as 8-bit paletted PNGs with the same palette and transparency:
    every decoded pixel stays the same (checked)."""
    from PIL import Image
    count = 0
    for folder, _, files in os.walk(COPY):
        for leaf in files:
            path = os.path.join(folder, leaf)
            if not leaf.lower().endswith('.png'):
                continue
            with open(path, 'rb') as f:
                header = f.read(33)
            if header[24] >= 8:
                continue
            picture = Image.open(path)
            picture.load()
            before = picture.convert('RGBA').tobytes()
            options = {'bits': 8}
            if 'transparency' in picture.info:
                options['transparency'] = picture.info['transparency']
            data = io.BytesIO()
            picture.save(data, 'PNG', **options)
            data = data.getvalue()
            if data[24] != 8 or Image.open(io.BytesIO(data)).convert('RGBA').tobytes() != before:
                sys.exit('idplay: could not store %s again unchanged' % path)
            open(path, 'wb').write(data)
            count += 1
    if count:
        log('stored %d sub-byte PNGs of the copy as 8-bit ones' % count)


def reset_original_state(profile):
    """a first run's state: no players file and only the window setting, or the scenario's profile"""
    shutil.rmtree(os.path.join(COPY, 'userdata'), ignore_errors=True)
    subprocess.run(['reg', 'delete', REGISTRY_KEY, '/f'], capture_output=True)
    subprocess.run(['reg', 'add', REGISTRY_KEY, '/v', 'ScreenMode', '/t', 'REG_DWORD', '/d', '0', '/f'], capture_output=True)
    if profile:
        os.makedirs(os.path.join(COPY, 'userdata'), exist_ok=True)
        shutil.copy(profile_users(profile), os.path.join(COPY, 'userdata', 'users.dat'))
        subprocess.run(['reg', 'add', REGISTRY_KEY, '/v', 'LastUser', '/t', 'REG_DWORD', '/d', '0', '/f'], capture_output=True)


# ---------------------------------------------------------------- the port

def short_levels_data(length):
    """data/levels.xml with every level's length set to length (the boss's warning comes 600 before it), so the bot meets
    each level's boss: the port reads it from a copy of its game folder, the original from its own copy for the run"""
    text = open(os.path.join(GAMEDIR, 'data', 'levels.xml'), encoding='latin-1').read()
    return re.sub(r'(<Level\b[^>]*\blength=")\d+(")', r'\g<1>%d\g<2>' % length, text)


def game_dir(scenario):
    """the port's game folder: the staged assets, or for a scenario with SHORT_LEVELS a copy with its short levels"""
    length = getattr(scenario, 'SHORT_LEVELS', None)
    if length is None:
        return GAMEDIR
    folder = os.path.join(OUT_ROOT, '_short%d' % length)
    levels = os.path.join(folder, 'data', 'levels.xml')
    if not os.path.exists(levels):
        shutil.rmtree(folder, ignore_errors=True)
        shutil.copytree(GAMEDIR, folder)
        open(levels, 'w', encoding='latin-1', newline='').write(short_levels_data(length))
    return folder


def run_port(script_lines, folder, user_folder, users=None, timeout=900, seed=1, gamedir=GAMEDIR):
    """the port under rundesk with -fast and -seed=N, from an empty user folder (or one with the players file `users`)"""
    script = os.path.join(folder, 'script.txt')
    open(script, 'w', newline='\n').write('\n'.join(script_lines) + '\n')
    shutil.rmtree(user_folder, ignore_errors=True)
    os.makedirs(user_folder, exist_ok=True)
    if users:
        shutil.copy(users, os.path.join(user_folder, 'users.dat'))
    wait_for_room()
    command = '"%s" -gamedir="%s" -userdir="%s" -fast -seed=%d -script="%s" > "%s" 2>&1' % (
        PORT_EXE.replace('\\', '/'), gamedir.replace('\\', '/'), user_folder.replace('\\', '/'), seed, script.replace('\\', '/'),
        os.path.join(folder, 'stdout.txt').replace('\\', '/'))
    result = subprocess.run([RUNDESK, 'bash', '-c', command], timeout=timeout)
    errors = [line for line in open(os.path.join(folder, 'stdout.txt'), encoding='utf-8', errors='replace') if line.startswith('ERROR')]
    if errors:
        sys.exit('idplay: the port\'s script failed: ' + errors[0].strip())
    return result.returncode


def port_anchor():
    """the port's anchor update with the intro click at PORT_INTRO_CLICK_UPDATE (found with a short traced run, made again
    when the port's exe is newer than its trace)"""
    folder = out_dir('_anchor')
    trace = os.path.join(folder, 'trace.txt')
    if not os.path.exists(trace) or os.path.getmtime(trace) < os.path.getmtime(PORT_EXE):
        run_port(['1 trace ' + trace.replace('\\', '/'), '%d click %d %d' % ((PORT_INTRO_CLICK_UPDATE,) + INTRO_POINT),
                  '%d quit' % (PORT_INTRO_CLICK_UPDATE + 100)], folder, os.path.join(folder, 'user'))
    for record in read_trace(trace):
        if record['loaded']:
            return record['u']
    sys.exit('idplay: the port never finished loading')


def read_trace(path):
    records = []
    for line in open(path, encoding='utf-8'):
        record = {}
        for item in line.split():
            key, _, value = item.partition('=')
            record[key] = int(value)
        records.append(record)
    return records


def profile_users(name):
    """a players file made by the port, which both games start from (the port writes the original's format): a player named
    Test, then for `unlocked` every mode and level open (`unlock all`), for `level<n>` the campaign saved before level n
    (`set progress n`), as after passing the level before it"""
    folder = out_dir('_profile_' + name)
    users = os.path.join(folder, 'user', 'users.dat')
    if os.path.exists(users):
        return users
    if name == 'unlocked':
        profile_command = 'unlock all'
    elif re.fullmatch(r'level\d+', name):
        profile_command = 'set progress ' + name[len('level'):]
    else:
        sys.exit('idplay: no profile "%s" (unlocked, level<n>)' % name)
    anchor = port_anchor()
    lines = ['%d click %d %d' % ((PORT_INTRO_CLICK_UPDATE,) + INTRO_POINT)]
    lines += ['%d %s' % (anchor + frame, command) for frame, command in PROFILE_OPENING]
    lines += ['%d %s' % (anchor + 300, profile_command), '%d quit' % (anchor + 320)]
    run_port(lines, folder, os.path.join(folder, 'user'))
    if not os.path.exists(users):
        sys.exit('idplay: the port wrote no players file for the profile')
    return users


# the title's play and the name typed, for a profile made by the port (frames after the anchor)
PROFILE_OPENING = [(60, 'move 320 419'), (60, 'click 320 419'), (100, 'char T'), (103, 'char e'), (106, 'char s'),
                   (109, 'char t'), (120, 'key RETURN')]


# ---------------------------------------------------------------- the steps

def read_frame(frame):
    """the first frame from frame on whose input the original reads before that frame's update: the frame after a wait"""
    while (frame - 1) % WAIT_PERIOD not in WAITS:
        frame += 1
    return frame


def record(name):
    scenario = load_scenario(name)
    folder = out_dir(name)
    anchor = port_anchor()
    record_file = os.path.join(folder, 'botrecord.txt')
    opening = [(read_frame(frame), command) for frame, command in scenario.OPENING]
    residues = sorted((anchor + 1 + wait) % WAIT_PERIOD for wait in WAITS)
    record_trace = os.path.join(folder, 'record-trace.txt')
    lines = ['1 trace ' + record_trace.replace('\\', '/'), '%d click %d %d' % ((PORT_INTRO_CLICK_UPDATE,) + INTRO_POINT)]
    lines += ['%d %s' % (anchor + frame, command) for frame, command in opening]
    lines += ['%d botrecord %s %d %s' % (anchor + scenario.BOT_FROM, record_file.replace('\\', '/'), WAIT_PERIOD,
                                         ' '.join(str(residue) for residue in residues))]
    lines += ['%d quit' % (anchor + scenario.END + MARGIN)]
    profile = getattr(scenario, 'PROFILE', None)
    run_port(lines, folder, os.path.join(folder, 'record-user'), profile_users(profile) if profile else None,
             seed=getattr(scenario, 'SEED', 1), gamedir=game_dir(scenario))
    # the game is over once no tank is left and the last one's wait has run out (the game-over box is up): the frames
    # compared end a little after that, or at the scenario's END
    end = scenario.END
    for trace_record in read_trace(record_trace):
        frame = trace_record['u'] - anchor
        if frame > 0 and trace_record['board'] and trace_record['lives'] == 0 and trace_record['tankdead'] == 1:
            end = min(end, frame + GAME_OVER_MARGIN)
            break
    inputs = list(opening)
    for line in open(record_file, encoding='utf-8'):
        update, command = line.split(' ', 1)
        if int(update) - anchor <= end:
            inputs.append((int(update) - anchor, command.strip()))
    with open(os.path.join(folder, 'input.txt'), 'w', newline='\n') as f:
        for frame, command in inputs:
            f.write('%d %s\n' % (frame, command))
    with open(os.path.join(folder, 'end.txt'), 'w', newline='\n') as f:
        f.write('%d\n' % end)
    log('%s: %d inputs recorded (port anchor %d), frames to %d' % (name, len(inputs), anchor, end))


def scenario_end(name):
    """the last frame compared: the bot's game over and a margin, or the scenario's END (record writes it)"""
    path = os.path.join(out_dir(name), 'end.txt')
    if os.path.exists(path):
        return int(open(path).read())
    return load_scenario(name).END


def read_inputs(name):
    inputs = []
    for line in open(os.path.join(out_dir(name), 'input.txt'), encoding='utf-8'):
        frame, command = line.split(' ', 1)
        inputs.append((int(frame), command.strip()))
    return inputs


def original(name):
    scenario = load_scenario(name)
    prepare()
    folder = out_dir(name)
    inputs = read_inputs(name)
    by_update = {}
    for frame, command in inputs:
        by_update.setdefault(ORIGINAL_ANCHOR + frame - 1, []).append(original_command(command))
    lines = ['focus regain', 'at %d move %d %d' % ((INTRO_CLICK_UPDATE,) + INTRO_POINT),
             'at %d click %d %d' % ((INTRO_CLICK_UPDATE,) + INTRO_POINT)]
    for update in range(ORIGINAL_ANCHOR - 2, ORIGINAL_ANCHOR + scenario_end(name) + 1):
        lines.extend(peek_lines(update))
        for command in by_update.get(update, []):
            lines.append('at %d %s' % (update, command))
    lines.append('quit')
    script = os.path.join(folder, 'original-script.txt')
    open(script, 'w', newline='\n').write('\n'.join(lines) + '\n')
    reset_original_state(getattr(scenario, 'PROFILE', None))
    wait_for_room()
    levels = os.path.join(COPY, 'data', 'levels.xml')
    length = getattr(scenario, 'SHORT_LEVELS', None)
    if length is not None:
        open(levels, 'w', encoding='latin-1', newline='').write(short_levels_data(length))
    try:
        with open(os.path.join(folder, 'original.log'), 'w') as f:
            result = subprocess.run([ORIGRUN, '--game', RUN, '--run', RUN, '--exe', win_path(ORIGINAL_EXE),
                                     '--seed', str(getattr(scenario, 'SEED', 1)), '--clock', 'virtual', '--cursor', 'scripted', '--keys', 'scripted', '--reg-map',
                                     'popcap', '--counter', '0x56b5b4', '--update-ms', '10', '--timeout', '1800', '--script',
                                     script], stdout=f, stderr=subprocess.STDOUT)
    finally:
        if length is not None:
            shutil.copy(os.path.join(GAMEDIR, 'data', 'levels.xml'), levels)
    text = open(os.path.join(folder, 'original.log'), encoding='utf-8', errors='replace').read()
    exit_line = re.search(r'game exited \(its process\'s exit code (\S+)\)', text)
    log('%s: the original ran (origrun %d, the game\'s exit code %s)' % (name, result.returncode,
                                                                        exit_line.group(1) if exit_line else 'not seen'))


def parse_original(name):
    """{update: values} from the peeks, and the input holds in order: [(command, the update the hold came after, the
    update the original read it before)]. A hold's input is read when the game next reads its messages, which can be
    an update later than the hold's counter says (a hold at a wait between two updates with no message check between
    them), so the update is found from the WidgetManager's counts: the input's read sets its last-input count to the
    WidgetManager's update count of that moment, which also counts each drawn frame (UpdateF), and the read came after
    the last peek whose count is no higher. One never seen read is taken as read before the next update."""
    records = {}
    holds = []                          # [command, the counter at the hold, the index of the peek before it]
    peeks = []                          # (update, the WidgetManager's count, its last input's count), in the log's order
    peek_line = re.compile(r'peek u=(\d+) (.*)$')
    hold_line = re.compile(r'at (\d+): counter reached (\d+) -> (?!peek)(.*)$')
    names = {expression: key for key, expression in EXPRESSIONS}
    for line in open(os.path.join(out_dir(name), 'original.log'), encoding='utf-8', errors='replace'):
        match = hold_line.search(line)
        if match:
            holds.append((match.group(3).strip(), int(match.group(2)), len(peeks) - 1))
            continue
        match = peek_line.search(line)
        if not match:
            continue
        record = {}
        for item in match.group(2).split(' '):
            expression, _, value = item.rpartition('=')
            if expression in names:
                record[names[expression]] = 0 if value == '-' else int(value)
        update = int(match.group(1))
        if 'wmupd' not in record:
            # an item line, after its update's first: it joins that update's record
            if update in records:
                records[update].update(record)
            continue
        record['loaded'] &= 0xff
        if 'mousedown' in record:
            record['mousedown'] &= 0xff
        for key in POINTERS:
            record[key] = 1 if record[key] else 0
        records[update] = record
        peeks.append((update, record['wmupd'], record['wminput']))
    for record in records.values():
        for index in range(1, TRACED_ITEMS + 1):
            kind = 'p%ds' % index
            if kind not in record:
                continue
            record[kind] = 1 if (record[kind] & 0xffffffff) == UNPLACED_VTABLE else 0
            if record[kind]:
                record['p%dx' % index] = record['p%dy' % index] = 0
    taken = []
    for command, reached, before in holds:
        read_before = reached + 1
        if before >= 0:
            held_count = peeks[before][1]
            # the first peek after the hold whose last input is no older than the hold has the read's count
            read_count = next((peeks[k][2] for k in range(before + 1, len(peeks)) if peeks[k][2] >= held_count), None)
            if read_count is not None:
                k = before
                while k + 1 < len(peeks) and peeks[k + 1][1] <= read_count:
                    k += 1
                read_before = peeks[k][0] + 1
        taken.append((command, reached, read_before))
    return records, taken


def port(name):
    scenario = load_scenario(name)
    folder = out_dir(name)
    inputs = read_inputs(name)
    records, holds = parse_original(name)
    anchor = port_anchor()
    original_anchor = min(update for update, record in records.items() if record['loaded'])
    if original_anchor != ORIGINAL_ANCHOR:
        log('note: the original\'s anchor is %d, not %d' % (original_anchor, ORIGINAL_ANCHOR))
    expected = [original_command(command) for _, command in inputs]
    taken = holds[2:]                   # after the intro's move and click
    if [hold[0] for hold in taken] != expected[:len(taken)]:
        sys.exit('idplay: the original\'s log holds other input than input.txt')
    trace = os.path.join(folder, 'trace.txt')
    lines = ['1 trace ' + trace.replace('\\', '/'), '%d click %d %d' % ((PORT_INTRO_CLICK_UPDATE,) + INTRO_POINT)]
    late = 0
    for (frame, command), (_, _, read_before) in zip(inputs, taken):
        if read_before != original_anchor + frame:
            late += 1
        lines.append('%d %s' % (anchor + read_before - original_anchor, command))
    lines.append('%d quit' % (anchor + scenario_end(name) + MARGIN))
    profile = getattr(scenario, 'PROFILE', None)
    run_port(lines, folder, os.path.join(folder, 'port-user'), profile_users(profile) if profile else None,
             seed=getattr(scenario, 'SEED', 1), gamedir=game_dir(scenario))
    log('%s: the port replayed %d inputs (%d taken an update later by the original)' % (name, len(taken), late))


def bot_game_left(name, trace, port_anchor_update, end):
    """the first frame at which the port's replay leaves the game the bot played in the record run (the original took an
    input at another update than the bot gave it), or None when it is that game throughout"""
    path = os.path.join(out_dir(name), 'record-trace.txt')
    if not os.path.exists(path):
        return -1
    recorded = {record['u']: record for record in read_trace(path)}
    record_anchor = min(update for update, record in recorded.items() if record['loaded'])
    for frame in range(0, end + 1):
        mine = trace.get(port_anchor_update + frame)
        theirs = recorded.get(record_anchor + frame)
        if mine is None or theirs is None:
            break
        if any(mine[key] != theirs[key] for key in mine if key != 'u'):
            return frame
    return None


def compare(name):
    end = scenario_end(name)
    records, _ = parse_original(name)
    trace = {record['u']: record for record in read_trace(os.path.join(out_dir(name), 'trace.txt'))}
    original_anchor = min(update for update, record in records.items() if record['loaded'])
    port_anchor_update = min(update for update, record in trace.items() if record['loaded'])
    left = bot_game_left(name, trace, port_anchor_update, end)
    peeks = []
    for update in sorted(records):
        frame = update - original_anchor
        if frame < 0 or frame > end:
            continue
        mine = trace.get(port_anchor_update + frame)
        if mine is None:
            continue
        theirs = records[update]
        mine = dict(mine)
        for key in POINTERS:
            mine[key] = 1 if mine[key] else 0
        peeks.append((frame, update, [key for key in COMPARED if theirs.get(key) != mine.get(key)], theirs, mine))
    # A peek the original took at a wait inside an update (the sound code waits as a sound starts) shows that update
    # half done, so it differs from the port's state after the whole update and agrees again at the next peek. Such a
    # one-peek difference is counted apart; a real split never agrees again, since the random draws part.
    # The tank's X can also differ in its last bits for a stretch of updates: the gun's angle, which sets the tank's
    # speed near the cursor, comes from the x87's fpatan in the original, whose result keeps 64 bits until it is scaled
    # to degrees, and from a double atan2 in the port. Those differences shrink as the tank closes on the cursor and go
    # once it stops or drives at full speed (IDENTICAL_PLAY.md, "Oddities"); they are counted apart too.
    compared = len(peeks)
    first = None
    differing = 0
    inside = 0
    last_bits = 0
    widest = 0
    for index, (frame, update, diffs, theirs, mine) in enumerate(peeks):
        if not diffs:
            continue
        if diffs == ['tankxl']:
            units = (theirs['tankxl'] - mine['tankxl']) & 0xffffffff
            units = min(units, 0x100000000 - units)
            if units <= LAST_BITS:
                last_bits += 1
                widest = max(widest, units)
                continue
        if index + 1 < len(peeks) and not peeks[index + 1][2]:
            inside += 1
            continue
        differing += 1
        if first is None:
            first = (frame, update, diffs, theirs, mine)
    report = os.path.join(out_dir(name), 'compare.txt')
    with open(report, 'w') as f:
        f.write('%s: %d frames compared (the original\'s anchor %d, the port\'s %d), %d differ, %d more taken inside an '
                'update, %d more in the tank X\'s last bits (at most %d units)\n' % (
                    name, compared, original_anchor, port_anchor_update, differing, inside, last_bits, widest))
        if first:
            frame, update, diffs, theirs, mine = first
            f.write('first difference at frame %d (original update %d): %s\n' % (frame, update, ', '.join(diffs)))
            for key in COMPARED:
                f.write('  %-9s original %-12d port %-12d%s\n' % (key, theirs.get(key, 0), mine.get(key, 0),
                                                                  '  <--' if key in diffs else ''))
    last = max((update - original_anchor for update in records if update >= original_anchor), default=0)
    summary = ('%s: %d frames compared up to frame %d, %d differ (%d more taken inside an update, %d in the tank X\'s '
               'last bits, at most %d units)' % (name, compared, last, differing, inside, last_bits, widest))
    if first:
        summary += '; the first at frame %d (%s)' % (first[0], ', '.join(first[2]))
    else:
        compared_records = [records[update] for update in sorted(records) if 0 <= update - original_anchor <= end]
        final = compared_records[-1]
        over = final['board'] and final['lives'] == 0 and final['tankdead'] == 1
        summary += '; at the end: level %d, score %d, %s' % (
            max(record['level'] for record in compared_records) + 1, max(record['score'] for record in compared_records),
            'the game over' if over else 'lives %d' % final['lives'])
    if left is None:
        summary += '; the bot\'s own game throughout'
    elif left >= 0:
        summary += '; the replay leaves the bot\'s game at frame %d' % left
    log(summary)
    return differing == 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('step', choices=['prepare', 'record', 'original', 'port', 'compare', 'run'])
    parser.add_argument('scenario', nargs='?')
    parser.add_argument('--record', action='store_true', help='with run: record the input again')
    args = parser.parse_args()
    if args.step == 'prepare':
        prepare()
        return
    if not args.scenario:
        sys.exit('idplay: which scenario?')
    if args.step in ('record', 'run') and (args.record or args.step == 'record' or
                                            not os.path.exists(os.path.join(out_dir(args.scenario), 'input.txt'))):
        record(args.scenario)
    if args.step in ('original', 'run'):
        original(args.scenario)
    if args.step in ('port', 'run'):
        port(args.scenario)
    if args.step in ('compare', 'run'):
        sys.exit(0 if compare(args.scenario) else 1)


if __name__ == '__main__':
    main()
