#!/usr/bin/env python3
"""Phase one of the identical-play pass (docs/COORDINATION.md; games/heavyweapon/docs/IDENTICAL_PLAY.md): the original and the
port play the same input, and the game's state is compared update by update.

    idplay.py prepare                     the original's copy made ready (see GAME_NOTES.md, "The original under origrun")
    idplay.py record <scenario>           the port's bot plays the scenario: its input, by update, into input.txt
    idplay.py original <scenario>         the original plays input.txt under origrun, its state read after every update
    idplay.py port <scenario>             the port plays the same input at the updates the original took it, with its trace
    idplay.py compare <scenario>          the two compared
    idplay.py run <scenario>              all four (record only when there is no input.txt yet, or with --record)

Phase two (the looks) runs the same games again:

    idplay.py shots <scenario> [--reuse]  the original inside Windows Sandbox with the scenario's shots (SHOTS, SHOT_EVERY),
                                          the port with its own at the updates they landed on, then tools/shotdiff
    idplay.py sound <scenario> [--reuse]  the original on the host with the logging BASS, the port with its sound log, then
                                          soundcmp.py on the two
    idplay.py web <scenario>              the web build under Node plays the last shot run's script, and shotdiff compares
                                          its shots with the native port's
    idplay.py shots3d <scenario> [--reuse]  the original in 3D on the visible desktop (tools/visrun.py) plays the sandbox
                                          run's script and takes the same shots (<shot>_3d); the port replays in 3D on its
                                          hardware renderer, then shotdiff as <scenario>_3d (after the sandbox run)
    idplay.py preview <scenario> [--3d]   the port alone takes the scenario's shots at their frames, with a contact sheet,
                                          for writing a scenario
    idplay.py screens [<scenario>...]     every tour's pairs (2D and 3D) in one shotdiff run, `screens`, which
                                          tools/screencheck.py reads; screensmd.py then writes docs/SCREENS.md from it

--coverage <functions.txt> (with original, run or sound) has origrun record which of the listed functions the original's run
reaches (docs/COVERAGE.md), into the scenario's folder as orig_coverage.txt.

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
level<n>, see profile_users) and SEED (both games' seed, 1 when not given). A shot scenario adds SHOTS ((frame, name)),
SHOT_EVERY ((first, step): a shot named <scenario>_<frame> every step frames), SHOT_BEFORE_CLICKS ({'x y': tag}: a shot
the frame before each recorded click at x y, for a box the bot clicks away), FULL_SCREEN (both games start in full
screen) and PORT_ENVIRONMENT (the port's environment in the 2D runs, such as SDL's software renderer).
"""

import argparse
import glob
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
COVERAGE = None   # --coverage: the functions origrun watches, or None
RUN = 'heavyweapon'
COPY = os.path.join(ROOT, 'build', 'origrun', RUN, 'game')
ORIGINAL_EXE = os.path.join(COPY, 'HeavyWeaponGame.exe')
PATCHES = os.path.join(ROOT, 'tools', 'origrun', 'patches', 'heavyweapon.txt')
BASS_PROXY = os.path.join(ROOT, 'build', 'bassproxy-hw', 'bass.dll')
BASS_DLL = os.path.join(COPY, 'bass.dll')
BASS_REAL = os.path.join(COPY, 'bass_real.dll')
PORT_EXE = os.path.join(ROOT, 'build', 'native', 'heavyweapon.exe')
WEB_JS = os.path.join(ROOT, 'build', 'web', 'heavyweapon.js')
NODE = 'E:/retools/emsdk/node/24.19.0_64bit/node.exe'
WASMRUN = os.path.join(ROOT, 'games', 'bejeweled', 'tools', 'verify', 'wasmrun.js')
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
UNDRAWN_UPDATES = '0x56b580'            # the app's count of updates since its last draw (+0x3e0; 0x451307 adds, 0x45136d clears)
SHOT_LAG = 1                         # the port's screenshot at update N + SHOT_LAG shows what the original's at N does

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
    if name in ('click', 'rclick', 'move', 'down', 'up') or port_command in ('focus lose', 'focus regain'):
        return port_command
    if name == 'char':
        return 'text ' + (' ' if rest == 'SPACE' else rest)
    if name in ('key', 'keydown', 'keyup'):
        return '%s %d' % (name, int(rest) if rest.isdigit() else KEY_CODES[rest])
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


def weak_bosses_data():
    """data/bosses.xml with every boss, and every part of one, at 1 point of armor, so that the bot beats each boss and the
    game goes on to the boss's death, the results, the cutscenes and the campaign's end"""
    text = open(os.path.join(GAMEDIR, 'data', 'bosses.xml'), encoding='latin-1').read()
    return re.sub(r'(\barmor=")\d+(")', r'\g<1>1\g<2>', text)


def data_overrides(scenario):
    """{file of data/: its text} for a scenario that changes the game's data (SHORT_LEVELS, WEAK_BOSSES); both games read the
    same files, the port from a copy of its game folder, the original from its own copy's data, put back after the run"""
    overrides = {}
    length = getattr(scenario, 'SHORT_LEVELS', None)
    if length is not None:
        overrides['levels.xml'] = short_levels_data(length)
    if getattr(scenario, 'WEAK_BOSSES', False):
        overrides['bosses.xml'] = weak_bosses_data()
    return overrides


def game_dir(scenario):
    """the port's game folder: the staged assets, or for a scenario that changes the game's data a copy with its changes"""
    overrides = data_overrides(scenario)
    if not overrides:
        return GAMEDIR
    key = []
    if getattr(scenario, 'SHORT_LEVELS', None) is not None:
        key.append('short%d' % scenario.SHORT_LEVELS)
    if getattr(scenario, 'WEAK_BOSSES', False):
        key.append('weak')
    folder = os.path.join(OUT_ROOT, '_' + '_'.join(key))
    if not os.path.exists(os.path.join(folder, 'data', 'done.txt')):
        shutil.rmtree(folder, ignore_errors=True)
        shutil.copytree(GAMEDIR, folder)
        for leaf, text in overrides.items():
            open(os.path.join(folder, 'data', leaf), 'w', encoding='latin-1', newline='').write(text)
        open(os.path.join(folder, 'data', 'done.txt'), 'w').write('made by idplay.py\n')
    return folder


def make_user_folder(user_folder, users, shots, full_screen=False, three_d=False):
    """a port run's user folder, made afresh: the players file `users`, if any, and for a shot run the registry's settings
    the original's sandbox.reg gives it (a FULL_SCREEN scenario's in full screen, its record run too; a 3D replay's in 3D)"""
    shutil.rmtree(user_folder, ignore_errors=True)
    os.makedirs(user_folder, exist_ok=True)
    if users:
        shutil.copy(users, os.path.join(user_folder, 'users.dat'))
    if shots or full_screen:
        with open(os.path.join(user_folder, 'registry.ini'), 'w', newline='\n') as f:
            f.write('[SOFTWARE\\PopCap\\HeavyWeapon]\nScreenMode=dword:%d\nIs3D=dword:%d\nCustomCursors=dword:0\n' %
                    (1 if full_screen else 0, 1 if three_d else 0) + ('LastUser=dword:0\n' if users else ''))


def scenario_environment(scenario):
    """the port's environment for a scenario: PORT_ENVIRONMENT, e.g. SDL's software renderer for a scenario that ticks the
    options' hardware acceleration, so the port's answer is the sandbox's (no 3D: the "not supported" box). The 2D runs only:
    a 3D replay (shots3d) keeps the hardware renderer"""
    return dict(getattr(scenario, 'PORT_ENVIRONMENT', {}))


def run_port(script_lines, folder, user_folder, users=None, timeout=900, seed=1, gamedir=GAMEDIR, shots=False, environment=None,
             full_screen=False, three_d=False):
    """the port under rundesk with -fast and -seed=N, from an empty user folder (or one with the players file `users`); a
    shot run (`shots`) draws every update at the game's own size (-drawevery -renderscale=1, docs/SHOTDIFF.md), in a window and
    in software and without the drawn cursor, as the original in the sandbox"""
    script = os.path.join(folder, 'script.txt')
    open(script, 'w', newline='\n').write('\n'.join(script_lines) + '\n')
    make_user_folder(user_folder, users, shots, full_screen, three_d)
    wait_for_room()
    command = '"%s" -gamedir="%s" -userdir="%s" -fast%s -seed=%d -script="%s" > "%s" 2>&1' % (
        PORT_EXE.replace('\\', '/'), gamedir.replace('\\', '/'), user_folder.replace('\\', '/'),
        ' -drawevery -renderscale=1' if shots else '', seed, script.replace('\\', '/'),
        os.path.join(folder, 'stdout.txt').replace('\\', '/'))
    result = subprocess.run([RUNDESK, 'bash', '-c', command], timeout=timeout, env=dict(os.environ, **(environment or {})))
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
    if scenario.BOT_FROM is not None:   # a tour of the menus has no bot
        lines += ['%d botrecord %s %d %s' % (anchor + scenario.BOT_FROM, record_file.replace('\\', '/'), WAIT_PERIOD,
                                             ' '.join(str(residue) for residue in residues))]
    else:
        open(record_file, 'w').close()
    lines += ['%d quit' % (anchor + scenario.END + MARGIN)]
    profile = getattr(scenario, 'PROFILE', None)
    run_port(lines, folder, os.path.join(folder, 'record-user'), profile_users(profile) if profile else None,
             seed=getattr(scenario, 'SEED', 1), gamedir=game_dir(scenario), environment=scenario_environment(scenario),
             full_screen=getattr(scenario, 'FULL_SCREEN', False))
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
    return sorted(inputs, key=lambda entry: entry[0])     # a scenario's late OPENING clicks come after the bot's in the file


def shot_list(name, scenario):
    """[(frame, shot name)]: the scenario's SHOTS, and with SHOT_EVERY = (first, step) a shot every step frames from first to
    the last frame compared"""
    shots = list(getattr(scenario, 'SHOTS', []))
    every = getattr(scenario, 'SHOT_EVERY', None)
    if every:
        first, step = every
        shots += [(frame, '%s_%05d' % (name, frame)) for frame in range(first, scenario_end(name) + 1, step)]
    # SHOT_BEFORE_CLICKS {'x y': tag}: a shot the frame before each recorded click at x y (a play tip's or a box's OK that the
    # bot clicks), so a box that is up only a few updates is shot too
    before = getattr(scenario, 'SHOT_BEFORE_CLICKS', {})
    if before and os.path.exists(os.path.join(out_dir(name), 'input.txt')):
        for frame, command in read_inputs(name):
            verb, _, place = command.partition(' ')
            if verb == 'click' and place in before:
                shots.append((frame - 1, '%s_%s_%05d' % (name, before[place], frame)))
    return sorted(shots)


def sandbox_presets(folder, profile, full_screen=False):
    """the registry the sandbox's copy starts from (docs/ORIGRUN.md, "Saves and the registry"): the window (or full screen),
    software drawing, the system's cursor (the cursor the game draws itself is in the original's capture but not in the port's
    screenshot) and, with a players file, its first player; under HKCU, where --reg-map popcap sends the game's HKLM key"""
    lines = ['Windows Registry Editor Version 5.00', '', '[%s]' % REGISTRY_KEY.replace('HKCU', 'HKEY_CURRENT_USER'),
             '"ScreenMode"=dword:%08x' % (1 if full_screen else 0), '"Is3D"=dword:00000000', '"CustomCursors"=dword:00000000']
    if profile:
        lines.append('"LastUser"=dword:00000000')
    path = os.path.join(folder, 'sandbox.reg')
    open(path, 'w', encoding='utf-16', newline='\r\n').write('\n'.join(lines) + '\n')
    return path


def original(name, sandbox=False, sound=False):
    """the original plays input.txt under origrun, its state peeked after every update; a phase-two run (`sandbox`) is inside
    Windows Sandbox, the only place origrun takes a shot (docs/ORIGRUN.md, "The host-capture ban"), and takes the scenario's
    shots there, into orig/ with the updates they landed on (landed.tsv). A sound run (`sound`, on the host) has the logging
    BASS beside the copy's exe (BASS_PROXY, built from games/bigmoney/tools/bassproxy, which passes every call on), and keeps
    its log and origrun's sound log (every DirectSound buffer's creation, play and stop) as orig_bass.log and orig_sound.log"""
    scenario = load_scenario(name)
    prepare()
    folder = out_dir(name)
    script = original_script(name, scenario, sandbox)
    profile = getattr(scenario, 'PROFILE', None)
    overrides = data_overrides(scenario)
    command = [ORIGRUN, '--game', RUN, '--run', RUN, '--exe', win_path(ORIGINAL_EXE), '--seed', str(getattr(scenario, 'SEED', 1)),
               '--clock', 'virtual', '--cursor', 'scripted', '--keys', 'scripted', '--reg-map', 'popcap', '--counter', '0x56b5b4',
               '--update-ms', '10', '--timeout', '3000' if sandbox else '1800', '--script', script]
    return run_original(name, scenario, folder, command, profile, overrides, sandbox, sound)


def original_script(name, scenario, sandbox, path=None):
    """the original's script (original-script.txt): the intro's click, input.txt's input at the updates before which the
    original is to read it, a peek after every update and, for a sandbox run, the shots; the 3D run (shots3d) writes it to a
    `path` of its own, so a sandbox run waiting for the sandbox keeps its file"""
    folder = out_dir(name)
    inputs = read_inputs(name)
    by_update = {}
    for frame, command in inputs:
        by_update.setdefault(ORIGINAL_ANCHOR + frame - 1, []).append(original_command(command))
    shots = {}
    if sandbox:
        for frame, shot_name in shot_list(name, scenario):
            shots.setdefault(ORIGINAL_ANCHOR + frame, []).append(shot_name)
    # shots of the intro, before the click that ends it (a SHOTS frame below INTRO_CLICK_UPDATE - ORIGINAL_ANCHOR)
    lines = ['focus regain'] + ['at %d shot %s' % (update, shot_name) for update in sorted(shots)
                                if update < INTRO_CLICK_UPDATE for shot_name in shots[update]]
    lines += ['at %d move %d %d' % ((INTRO_CLICK_UPDATE,) + INTRO_POINT),
              'at %d click %d %d' % ((INTRO_CLICK_UPDATE,) + INTRO_POINT)]
    for update in range(ORIGINAL_ANCHOR - 2, ORIGINAL_ANCHOR + scenario_end(name) + 1):
        lines.extend(peek_lines(update))
        for command in by_update.get(update, []):
            lines.append('at %d %s' % (update, command))
        for shot_name in shots.get(update, []):
            lines.append('at %d shot %s' % (update, shot_name))
    lines.append('quit')
    script = path or os.path.join(folder, 'original-script.txt')
    open(script, 'w', newline='\n').write('\n'.join(lines) + '\n')
    return script


def run_original(name, scenario, folder, command, profile, overrides, sandbox, sound):
    """origrun with the original's script (`original`), and its log, shots and landed updates gathered"""
    run_folder = os.path.join(ROOT, 'build', 'origrun', RUN)
    if COVERAGE:
        command += ['--coverage', win_path(COVERAGE)]
    if sandbox:
        # each shot shows exactly the update it is labelled with: the count of updates not yet drawn serves as both the
        # pending draw and the update state (zero once the update is drawn), as Typer Shark's does
        # the machine has one sandbox, which other sessions' runs take too: queue for it up to four hours
        command += ['--sandbox', '--sandbox-wait', '14400',
                    '--reg', win_path(sandbox_presets(folder, profile, getattr(scenario, 'FULL_SCREEN', False))),
                    '--pending-draw', UNDRAWN_UPDATES,
                    '--update-state', UNDRAWN_UPDATES]
        shutil.rmtree(os.path.join(run_folder, 'shots'), ignore_errors=True)
    if sound:
        if not os.path.exists(BASS_PROXY):
            sys.exit('idplay: no %s: build it as games/bigmoney/tools/bassproxy/bassproxy.c says' % BASS_PROXY)
        if not os.path.exists(BASS_REAL):
            os.replace(BASS_DLL, BASS_REAL)
        shutil.copy(BASS_PROXY, BASS_DLL)
    reset_original_state(profile)
    wait_for_room()
    for leaf, text in overrides.items():
        open(os.path.join(COPY, 'data', leaf), 'w', encoding='latin-1', newline='').write(text)
    if COVERAGE and os.path.exists(os.path.join(run_folder, 'coverage.txt')):
        os.remove(os.path.join(run_folder, 'coverage.txt'))
    # the stand-in adds to its log run after run: this run's alone
    if sound and os.path.exists(os.path.join(run_folder, 'logs', 'sound.log')):
        os.remove(os.path.join(run_folder, 'logs', 'sound.log'))
    try:
        with open(os.path.join(folder, 'original.log'), 'w') as f:
            result = subprocess.run(command, stdout=f, stderr=subprocess.STDOUT)
    finally:
        for leaf in overrides:
            shutil.copy(os.path.join(GAMEDIR, 'data', leaf), os.path.join(COPY, 'data', leaf))
    if sandbox:
        # the peeks are logged inside the sandbox, in a file of the run's folder; the shots are in its shots/
        with open(os.path.join(folder, 'original.log'), 'a', encoding='utf-8') as out:
            out.write(open(os.path.join(run_folder, 'logs', 'origrun-sandbox.log'), encoding='utf-8', errors='replace').read())
        original_shots = os.path.join(folder, 'orig')
        shutil.rmtree(original_shots, ignore_errors=True)
        if os.path.isdir(os.path.join(run_folder, 'shots')):
            shutil.copytree(os.path.join(run_folder, 'shots'), original_shots)
    if sound:
        os.replace(BASS_REAL, BASS_DLL)
        shutil.copy(os.path.join(run_folder, 'logs', 'sound.log'), os.path.join(folder, 'orig_sound.log'))
        shutil.move(os.path.join(COPY, 'basslog.txt'), os.path.join(folder, 'orig_bass.log'))
    if COVERAGE and os.path.exists(os.path.join(run_folder, 'coverage.txt')):
        shutil.copy(os.path.join(run_folder, 'coverage.txt'), os.path.join(folder, 'orig_coverage.txt'))
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
    hold_line = re.compile(r'at (\d+): counter reached (\d+) -> (?!peek|shot)(.*)$')
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
        # a focus change is no input, and leaves the last-input count as it was: read before the next update
        if before >= 0 and not command.startswith('focus '):
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


def read_landed(name, side='orig'):
    """{shot name: the update the original's shot landed on}, from the sandbox run's landed.tsv (its name, its update and how
    sure that update is, docs/ORIGRUN.md, "Timing to the game's updates"), or with side 'orig3d' the 3D run's (shots3d)"""
    landed = {}
    path = os.path.join(out_dir(name), side, 'landed.tsv')
    for line in open(path, encoding='utf-8', errors='replace'):
        fields = line.rstrip('\n').split('\t')
        if len(fields) >= 2 and fields[1].isdigit():
            landed[fields[0]] = int(fields[1])
    return landed


def port(name, shots=False, environment=None, three_d=False):
    """the port replays the input at the updates the original read it before, with its trace; a shot run (`shots`) also takes
    each of the original's shots at the update it landed on, SHOT_LAG later (the port's `screenshot` draws the state the
    update before it left), into port/; with `three_d` in 3D, at the 3D run's updates (orig3d/landed.tsv), into port3d/,
    its trace into trace3d.txt"""
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
    trace = os.path.join(folder, 'trace3d.txt' if three_d else 'trace.txt')
    lines = ['1 trace ' + trace.replace('\\', '/'), '%d click %d %d' % ((PORT_INTRO_CLICK_UPDATE,) + INTRO_POINT)]
    # a 3D run (visrun) may end the intro an update or more later than the sandbox run, and every update after it with it:
    # its input comes at the same updates, so the port's replay maps the updates from the 3D run's anchor instead
    shift = anchor3d(name) - original_anchor if three_d else 0
    if shift:
        log('note: the 3D run\'s anchor is %d update(s) after the sandbox run\'s' % shift)
    late = 0
    for (frame, command), (_, _, read_before) in zip(inputs, taken):
        if read_before != original_anchor + frame:
            late += 1
        lines.append('%d %s' % (anchor + read_before - original_anchor - shift, command))
    if shots:
        port_shots = os.path.join(folder, 'port3d' if three_d else 'port')
        shutil.rmtree(port_shots, ignore_errors=True)
        os.makedirs(port_shots)
        for shot_name, update in sorted(read_landed(name, 'orig3d' if three_d else 'orig').items(), key=lambda item: item[1]):
            at = anchor + update - original_anchor - shift + SHOT_LAG
            lines.append('%d screenshot %s/%s.png' % (at, port_shots.replace('\\', '/'), shot_name))
            lines.append('%d state' % at)   # the screen it shows, for SCREENS.md
    lines.append('%d quit' % (anchor + scenario_end(name) + MARGIN))
    # an update's shot before its input: the original's shot at N is drawn before it reads the input it takes before
    # update N + 1, and the port's screenshot draws when its line is read, so an input read before it would show (a tip
    # closed, a letter typed)
    lines.sort(key=lambda line: (int(line.split(' ', 1)[0]), 0 if line.split(' ', 2)[1] in ('screenshot', 'state') else 1))
    profile = getattr(scenario, 'PROFILE', None)
    run_port(lines, folder, os.path.join(folder, 'port-user'), profile_users(profile) if profile else None,
             seed=getattr(scenario, 'SEED', 1), gamedir=game_dir(scenario), shots=shots,
             environment=dict({} if three_d else scenario_environment(scenario), **(environment or {})),
             full_screen=getattr(scenario, 'FULL_SCREEN', False), three_d=three_d)
    if shots:
        # each shot's `state` into states.tsv beside the shots, which SCREENS.md sorts by: the script's only `state` lines are
        # the shots', in the shots' order (the screenshot's own line goes to stderr, so the two streams are not paired)
        shot_names = [os.path.splitext(os.path.basename(line.split(' ', 2)[2]))[0] for line in lines
                      if line.split(' ', 2)[1] == 'screenshot']
        states = [line[line.index('STATE {') + len('STATE '):].strip() for line in
                  open(os.path.join(folder, 'stdout.txt'), encoding='utf-8', errors='replace') if 'STATE {' in line]
        if len(states) != len(shot_names):
            log('note: %d states for %d shots; no states.tsv' % (len(states), len(shot_names)))
        else:
            with open(os.path.join(port_shots, 'states.tsv'), 'w', newline='\n') as f:
                for shot_name, state in zip(shot_names, states):
                    f.write('%s\t%s\n' % (shot_name, state))
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


def shots(name, reuse=False):
    """phase two: the original's run inside Windows Sandbox with its shots (or, with `reuse`, the last one's), the port's run
    with its own at the updates the original's landed on, the comparison of their states (a shot draws the screen once more
    in both, so the games must still agree) and tools/shotdiff on the two folders (docs/SHOTDIFF.md)"""
    if not reuse:
        original(name, sandbox=True)
    port(name, shots=True)
    agree = compare(name)
    folder = out_dir(name)
    log('%s: %d shots of the original, %d of the port' % (name, len(glob.glob(os.path.join(folder, 'orig', '*.png'))),
                                                         len(glob.glob(os.path.join(folder, 'port', '*.png')))))
    subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'heavyweapon', '--orig',
                     os.path.join(folder, 'orig'), '--port', os.path.join(folder, 'port'), '--run', name], cwd=ROOT)
    return agree


VISRUN_CONFIG = os.path.join(HERE, '..', 'visrun', 'heavyweapon.toml')
STATE_PEEK_EVERY = 100                  # the 3D run's state is logged every this many updates (and at every shot)
COPY3D = os.path.join(ROOT, 'build', 'origrun', RUN + '3d', 'game')   # the 3D runs' own copy: a sandbox run uses COPY live


def shots3d(name, reuse=False):
    """phase two in 3D: the original in its Direct3D mode on the visible desktop (tools/visrun.py, docs/VISRUN.md; neither the
    sandbox nor rundesk runs it in 3D) plays the tour's script for the original (original-script.txt, written by its sandbox
    run) without the peeks, on the virtual clock, from a copy of its own (COPY3D), and takes the same shots, each named
    <shot>_3d, into orig3d/ with the updates they landed on (with `reuse`, the last such run's); the port replays the same
    input in 3D, at the updates the sandbox run read it before, and shoots at the 3D run's updates into port3d/; the state at
    each shot (score, level, lives) is compared with the port's trace; then tools/shotdiff on the two folders as the run
    <tour>_3d. The port's part needs the tour's sandbox run to have ended"""
    scenario = load_scenario(name)
    folder = out_dir(name)
    out = os.path.join(folder, 'orig3d')
    if not reuse:
        shots3d_original(name, scenario, folder, out)
    if not os.path.exists(os.path.join(folder, 'orig', 'landed.tsv')):
        log('%s: the 3D run is done; replay the port with `shots3d %s --reuse` once the sandbox run has ended' % (name, name))
        return True
    rows = [line.rstrip('\n').split('\t') for line in open(os.path.join(out, 'steps.tsv'), encoding='utf-8')]
    port(name, shots=True, three_d=True)
    # the state at each shot against the port's trace, at the update the shot shows
    anchor = port_anchor()
    trace = {record['u']: record for record in read_trace(os.path.join(folder, 'trace3d.txt'))}
    differ = []
    for row in rows:
        if len(row) < 6 or row[2] != 'shot':
            continue
        values = dict(item.split('=', 1) for item in row[5].split() if '=' in item)
        record = trace.get(anchor + int(row[1]) - anchor3d(name))
        if record is None:
            continue
        for key in ('score', 'level', 'lives'):
            if values.get(key, '-') != '-' and int(values[key]) != record.get(key, int(values[key])):
                differ.append(row[3].split()[0])
                log('%s: %s is %s in the original and %s in the port' % (row[3].split()[0], key, values[key], record.get(key)))
                break
    log('%s: %d shots in 3D, %d whose state differs' % (name, len(glob.glob(os.path.join(out, '*.png'))), len(differ)))
    # The original's 3D game parts from its 2D one in a level, the tank's X first (IDENTICAL_PLAY.md, "Oddities"), before
    # the score shows it: every shot after the last peek of the 3D run that agrees with the sandbox run (the tank's X to
    # its last bit) is left out by `screens` too, with those whose state differs (port3d/state-differs.txt)
    records, _ = parse_original(name)
    original_anchor = min(update for update, record in records.items() if record['loaded'])
    agreed = None
    for row in rows:
        if len(row) < 6 or row[2] != 'peek' or not row[1].isdigit():
            continue
        record = records.get(int(row[1]) - anchor3d(name) + original_anchor)
        if record is None:
            continue
        values = dict(item.split('=', 1) for item in row[5].split() if '=' in item)
        if any(values.get(key, '-') != '-' and key in record and
               (int(values[key]) & 0xffffffff) != (record[key] & 0xffffffff)
               for key in ('score', 'level', 'lives', 'killed', 'tankxh', 'ltime')):
            log('%s: the 3D game parts from the sandbox run\'s after update %s' % (name, agreed))
            for shot in rows:
                if (len(shot) >= 6 and shot[2] == 'shot' and shot[1].isdigit() and int(shot[1]) > (agreed or 0) and
                        shot[3].split()[0] not in differ):
                    differ.append(shot[3].split()[0])
            break
        agreed = int(row[1])
    with open(os.path.join(folder, 'port3d', 'state-differs.txt'), 'w', newline='\n') as listing:
        listing.write(''.join(shot + '\n' for shot in differ))
    subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'heavyweapon', '--orig',
                     out, '--port', os.path.join(folder, 'port3d'), '--run', name + '_3d'], cwd=ROOT)
    return True     # a parted state is the original's 3D game, listed and left out; the shots are judged in `screens`


def anchor3d(name):
    """the 3D run's anchor: the first update its peeks find the loading done after (orig3d/anchor.txt)"""
    path = os.path.join(out_dir(name), 'orig3d', 'anchor.txt')
    return int(open(path).read()) if os.path.exists(path) else ORIGINAL_ANCHOR


def shots3d_original(name, scenario, folder, out):
    """the original's part of shots3d: visrun on the visible desktop, its shots and steps.tsv into `out`, landed.tsv beside"""
    if not os.path.exists(os.path.join(folder, 'input.txt')):
        record(name)
    os.makedirs(out, exist_ok=True)
    script = original_script(name, scenario, sandbox=True, path=os.path.join(out, 'original-script.txt'))   # as the sandbox's
    steps = []
    last_peek = -STATE_PEEK_EVERY
    for line in open(script, encoding='utf-8'):
        line = line.strip()
        words = line.split()
        if len(words) >= 3 and words[0] == 'at' and words[2] == 'peek':
            update = int(words[1])
            if update - last_peek >= STATE_PEEK_EVERY or update <= ORIGINAL_ANCHOR + 4:   # every one about the anchor
                steps.append('at %d peek' % update)
                last_peek = update
            continue
        if len(words) == 4 and words[0] == 'at' and words[2] == 'shot':
            line = 'at %s shot %s_3d' % (words[1], words[3])
        steps.append(line)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    steps_file = os.path.join(out, 'steps.txt')
    open(steps_file, 'w', newline='\n').write('\n'.join(steps) + '\n')
    profile = getattr(scenario, 'PROFILE', None)
    # the 3D runs' copy: the prepared copy (its exe clean, beside origrun's patched one) without its saves, made once
    prepare()
    if not os.path.exists(os.path.join(COPY3D, 'HeavyWeaponGame.exe')):
        shutil.copytree(COPY, COPY3D, ignore=shutil.ignore_patterns('userdata', 'logs', '*.log'))
    shutil.rmtree(os.path.join(COPY3D, 'userdata'), ignore_errors=True)
    if profile:
        os.makedirs(os.path.join(COPY3D, 'userdata'))
        shutil.copy(profile_users(profile), os.path.join(COPY3D, 'userdata', 'users.dat'))
    config = open(VISRUN_CONFIG, encoding='utf-8').read()
    exe3d = "source_exe = '%s'" % os.path.join(COPY3D, 'HeavyWeaponGame.exe')
    config = re.sub(r'(?m)^source_exe = "[^"]*"', lambda match: exe3d, config)
    if profile:
        config = config.replace('CustomCursors = 0 }', 'CustomCursors = 0, LastUser = 0 }')
    config_file = os.path.join(out, 'heavyweapon.toml')
    open(config_file, 'w', encoding='utf-8', newline='\n').write(config)
    overrides = data_overrides(scenario)
    for leaf, text in overrides.items():
        open(os.path.join(COPY3D, 'data', leaf), 'w', encoding='latin-1', newline='').write(text)
    try:
        with open(os.path.join(folder, 'original3d.log'), 'w') as f:
            result = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'visrun.py'), config_file, steps_file,
                                     '--out', out, '--3d', 'on', '--seed', str(getattr(scenario, 'SEED', 1)), '--timeout', '1800',
                                     '--quiet', '30'], stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    finally:
        for leaf in overrides:
            shutil.copy(os.path.join(GAMEDIR, 'data', leaf), os.path.join(COPY3D, 'data', leaf))
    visrun_log = open(os.path.join(out, 'visrun.log'), encoding='utf-8', errors='replace').read() \
        if os.path.exists(os.path.join(out, 'visrun.log')) else ''
    flags = re.findall(r'3D flag (\S+)', visrun_log)
    log('%s: the original ran in 3D on the desktop (visrun %d, 3D flags seen %s)' % (name, result.returncode, flags))
    if result.returncode != 0 or not flags or any(flag != '1' for flag in flags):
        sys.exit('idplay: the 3D run did not run in 3D to its end: %s' % os.path.join(out, 'visrun.log'))
    rows = [line.rstrip('\n').split('\t') for line in open(os.path.join(out, 'steps.tsv'), encoding='utf-8')]
    loaded = [int(row[1]) for row in rows if len(row) >= 6 and row[2] == 'peek' and
              any(item.startswith('loaded=') and item[7:].lstrip('-').isdigit() and int(item[7:]) & 0xff
                  for item in row[5].split())]
    if not loaded:
        sys.exit('idplay: the 3D run\'s peeks never found the loading done: %s' % os.path.join(out, 'steps.tsv'))
    open(os.path.join(out, 'anchor.txt'), 'w').write('%d\n' % min(loaded))
    log('%s: the 3D run\'s anchor is update %d (the sandbox run\'s is %d)' % (name, min(loaded), ORIGINAL_ANCHOR))
    with open(os.path.join(out, 'landed.tsv'), 'w', newline='\n') as f:
        for row in rows:
            if len(row) >= 4 and row[2] == 'shot':
                f.write('%s\t%s\n' % (row[3].split()[0], row[1]))
    # inputs: posted after the update they were wanted at, as in the sandbox run (the virtual clock is the same)
    late = [row for row in rows if len(row) >= 4 and row[2] in ('move', 'click', 'rclick', 'down', 'up', 'key', 'text')
            and row[0] and row[0] != row[1]]
    if late:
        log('note: %d inputs of the 3D run were posted after another update than asked: %s' % (len(late), late[:3]))


def preview(name, three_d=False):
    """a tour's shots in the port alone, for writing a scenario: the recorded input at the frames it was given and each shot at
    its frame (the original's landing is not known), each followed by a `state` line, into preview/ (with `three_d`, the
    port in 3D on the hardware renderer, into preview3d/), and a contact sheet of them (sheet.png, the names in sheet.txt)"""
    scenario = load_scenario(name)
    folder = out_dir(name)
    anchor = port_anchor()
    shots_folder = os.path.join(folder, 'preview3d' if three_d else 'preview')
    shutil.rmtree(shots_folder, ignore_errors=True)
    os.makedirs(shots_folder)
    lines = ['%d click %d %d' % ((PORT_INTRO_CLICK_UPDATE,) + INTRO_POINT)]
    lines += ['%d %s' % (anchor + frame, command) for frame, command in read_inputs(name)]
    names = []
    for frame, shot_name in shot_list(name, scenario):
        names.append(shot_name)
        lines += ['%d screenshot %s/%s.png' % (anchor + frame + SHOT_LAG, shots_folder.replace('\\', '/'), shot_name),
                  '%d state' % (anchor + frame + SHOT_LAG)]
    lines.append('%d quit' % (anchor + scenario_end(name) + MARGIN))
    lines.sort(key=lambda line: (int(line.split(' ', 1)[0]), 0 if line.split(' ', 2)[1] in ('screenshot', 'state') else 1))
    profile = getattr(scenario, 'PROFILE', None)
    run_port(lines, shots_folder, os.path.join(shots_folder, 'user'), profile_users(profile) if profile else None,
             seed=getattr(scenario, 'SEED', 1), gamedir=game_dir(scenario), shots=True,
             environment={} if three_d else scenario_environment(scenario), full_screen=getattr(scenario, 'FULL_SCREEN', False),
             three_d=three_d)
    states = [line.strip() for line in open(os.path.join(shots_folder, 'stdout.txt'), encoding='utf-8', errors='replace')
              if line.startswith('STATE')]
    from PIL import Image, ImageDraw
    columns, width, height = 5, 256, 192
    taken = [shot_name for shot_name in names if os.path.exists(os.path.join(shots_folder, shot_name + '.png'))]
    sheet = Image.new('RGB', (columns * width, max(1, (len(taken) + columns - 1) // columns) * height))
    draw = ImageDraw.Draw(sheet)
    for index, shot_name in enumerate(taken):
        picture = Image.open(os.path.join(shots_folder, shot_name + '.png')).convert('RGB').resize((width, height))
        x, y = (index % columns) * width, (index // columns) * height
        sheet.paste(picture, (x, y))
        draw.rectangle((x, y, x + 7 * len(shot_name), y + 11), fill=(0, 0, 0))
        draw.text((x + 1, y), shot_name, fill=(255, 255, 0))
    sheet.save(os.path.join(shots_folder, 'sheet.png'))
    with open(os.path.join(shots_folder, 'sheet.txt'), 'w', newline='\n') as f:
        for shot_name, state in zip(names, states):
            f.write('%s %s\n' % (shot_name, state))
    log('%s: %d preview shots of %d, sheet in %s' % (name, len(taken), len(names), os.path.join(shots_folder, 'sheet.png')))


def screens(names):
    """every tour's pairs in one shotdiff run, `screens`, which tools/screencheck.py reads for SCREENS.md (docs/SHOTDIFF.md,
    "Every screen, listed"): the named tours', or every tour's with both orig/ and port/ shots, and their 3D pairs
    (orig3d/ and port3d/, each named <shot>_3d) where the tour has them, but for those whose state differs
    (port3d/state-differs.txt). The shot names are the tours' own, which do not
    repeat across tours"""
    if not names:
        names = sorted(entry for entry in os.listdir(OUT_ROOT) if not entry.startswith('_') and
                       os.path.isdir(os.path.join(OUT_ROOT, entry, 'orig')) and os.path.isdir(os.path.join(OUT_ROOT, entry, 'port')))
    merged = os.path.join(OUT_ROOT, '_screens')
    shutil.rmtree(merged, ignore_errors=True)
    for side in ('orig', 'port'):
        os.makedirs(os.path.join(merged, side))
    seen = {}
    for name in names:
        for side, source in (('orig', 'orig'), ('port', 'port'), ('orig', 'orig3d'), ('port', 'port3d')):
            if source.endswith('3d') and not os.path.isdir(os.path.join(OUT_ROOT, name, 'port3d')):
                continue
            parted = set()
            if source.endswith('3d') and os.path.exists(os.path.join(OUT_ROOT, name, 'port3d', 'state-differs.txt')):
                parted = set(open(os.path.join(OUT_ROOT, name, 'port3d', 'state-differs.txt')).read().split())
            # and every 3D shot inside a level: there the original's 3D game parts from the first shots fired, the shells,
            # smoke and bullets standing elsewhere before its peeks show it (IDENTICAL_PLAY.md, "Oddities")
            states = os.path.join(OUT_ROOT, name, 'port3d', 'states.tsv')
            if source.endswith('3d') and os.path.exists(states):
                for line in open(states, encoding='utf-8'):
                    shot, _, state = line.rstrip('\n').partition('\t')
                    if '"screen":"game"' in state:
                        parted.add(shot)
            for path in glob.glob(os.path.join(OUT_ROOT, name, source, '*.png')):
                leaf = os.path.basename(path)
                if os.path.splitext(leaf)[0] in parted:
                    continue                # a 3D shot whose games parted compares nothing
                if seen.setdefault((side, leaf), name) != name:
                    sys.exit('idplay: %s is a shot of both %s and %s' % (leaf, seen[(side, leaf)], name))
                shutil.copy(path, os.path.join(merged, side, leaf))
    log('screens: %d tours, %d shots of the original, %d of the port' % (
        len(names), len(os.listdir(os.path.join(merged, 'orig'))), len(os.listdir(os.path.join(merged, 'port')))))
    return subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'heavyweapon',
                            '--orig', os.path.join(merged, 'orig'), '--port', os.path.join(merged, 'port'), '--run', 'screens'],
                           cwd=ROOT)


def web(name):
    """phase two's web build: the last shot run's script played by the web build under Node (wasmrun.js: the same switches as
    the native run, a copy of the run's folder as its file system) from the same user folder, its shots into web/shots/, and
    shotdiff with the native port's shots as the reference. The two are one program on two back ends, so what differs is the
    drawing, not the play"""
    scenario = load_scenario(name)
    folder = out_dir(name)
    case = os.path.join(folder, 'web')
    shutil.rmtree(case, ignore_errors=True)
    os.makedirs(os.path.join(case, 'shots'))
    profile = getattr(scenario, 'PROFILE', None)
    make_user_folder(os.path.join(case, 'port-user'), profile_users(profile) if profile else None, True)
    script = open(os.path.join(folder, 'script.txt'), encoding='utf-8').read()
    script = re.sub(r'(screenshot) \S*/([^/\s]+\.png)', r'\1 shots/\2', script)
    script = re.sub(r'(trace) \S+', r'\1 web-trace.txt', script)
    open(os.path.join(case, 'script.txt'), 'w', newline='\n').write(script)
    wait_for_room()
    command = [NODE, WASMRUN, 'game', '--timeout', '3000', WEB_JS, '-gamedir=' + game_dir(scenario).replace('\\', '/'),
               '-userdir=port-user', '-fast', '-drawevery', '-renderscale=1', '-seed=%d' % getattr(scenario, 'SEED', 1),
               '-script=script.txt']
    with open(os.path.join(case, 'web.out'), 'w') as out:
        status = subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=case)
    taken = glob.glob(os.path.join(case, 'shots', '*.png'))
    log('%s: the web build exited with %d and took %d shots' % (name, status, len(taken)))
    def trace_lines(path):
        return [line.rstrip('\r') for line in open(path, encoding='utf-8', newline='').read().split('\n') if line.strip()]
    web_trace = os.path.join(case, 'web-trace.txt')
    native_lines = trace_lines(os.path.join(folder, 'trace.txt'))
    web_lines = trace_lines(web_trace) if os.path.exists(web_trace) else []
    # the native trace has CRLF line ends; the web build's ends where the game quit, its buffer unflushed, so its last line
    # may be cut off and a line or two missing: its whole lines are compared, and the cut one with the start of the native's
    whole = len(web_lines) - 1
    same_trace = (web_lines and whole >= len(native_lines) - 3 and len(web_lines) <= len(native_lines) and
                  web_lines[:whole] == native_lines[:whole] and native_lines[whole].startswith(web_lines[-1]))
    log('%s: the web build\'s trace is %s the native port\'s (%d updates of %d)' % (
        name, 'the same as' if same_trace else 'NOT the same as', whole, len(native_lines) - 1))
    subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'heavyweapon', '--orig',
                     os.path.join(folder, 'port'), '--port', os.path.join(case, 'shots'), '--run', name + '_web'], cwd=ROOT)
    return status == 0 and same_trace


def sound(name, reuse=False):
    """phase two's sound: the original on the host with the logging BASS (or, with `reuse`, its last such run), the port with
    its sound log (BYOPOP_SOUNDLOG), the comparison of their states, then of their sounds (soundcmp.py)"""
    if not reuse:
        original(name, sound=True)
    folder = out_dir(name)
    port(name, environment={'BYOPOP_SOUNDLOG': os.path.join(folder, 'port_sound.log').replace('\\', '/')})
    agree = compare(name)
    result = subprocess.call([sys.executable, os.path.join(HERE, 'soundcmp.py'), folder])
    return agree and result == 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('step', choices=['prepare', 'record', 'original', 'port', 'compare', 'run', 'shots', 'sound', 'web',
                                         'preview', 'screens', 'shots3d'])
    parser.add_argument('--reuse', action='store_true', help='with shots: the original\'s last sandbox run and its shots')
    parser.add_argument('scenario', nargs='?')
    parser.add_argument('more', nargs='*', help='with screens: more tours')
    parser.add_argument('--record', action='store_true', help='with run: record the input again')
    parser.add_argument('--3d', dest='three_d', action='store_true', help='with preview: the port in 3D')
    parser.add_argument('--coverage', help='with original, run or sound: the functions for origrun to watch (docs/COVERAGE.md)')
    args = parser.parse_args()
    global COVERAGE
    COVERAGE = os.path.abspath(args.coverage) if args.coverage else None
    if args.step == 'prepare':
        prepare()
        return
    if args.step == 'screens':
        sys.exit(screens(([args.scenario] if args.scenario else []) + args.more))
    if not args.scenario:
        sys.exit('idplay: which scenario?')
    if args.step in ('record', 'run', 'shots', 'sound', 'preview') and (args.record or args.step == 'record' or
                                                              not os.path.exists(os.path.join(out_dir(args.scenario), 'input.txt'))):
        record(args.scenario)
    if args.step == 'shots':
        sys.exit(0 if shots(args.scenario, args.reuse) else 1)
    if args.step == 'sound':
        sys.exit(0 if sound(args.scenario, args.reuse) else 1)
    if args.step == 'web':
        sys.exit(0 if web(args.scenario) else 1)
    if args.step == 'preview':
        preview(args.scenario, args.three_d)
        return
    if args.step == 'shots3d':
        sys.exit(0 if shots3d(args.scenario, args.reuse) else 1)
    if args.step in ('original', 'run'):
        original(args.scenario)
    if args.step in ('port', 'run'):
        port(args.scenario)
    if args.step in ('compare', 'run'):
        sys.exit(0 if compare(args.scenario) else 1)


if __name__ == '__main__':
    main()
