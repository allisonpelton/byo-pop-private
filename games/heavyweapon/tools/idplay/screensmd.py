"""Writes games/heavyweapon/docs/SCREENS.md (docs/SHOTDIFF.md, "Every screen, listed"): every screen and box the game can
show, from our export and the port's source, with the shots of the idplay tours that compare it. The rows are below; each
names its shots by patterns over the shot names of the merged `screens` run (`idplay.py screens`, every tour's pairs in one
shotdiff run), a pattern taking a shot's 3D twin (<shot>_3d, `idplay.py shots3d`) too, or by the screen the port's `state`
gives at the shot (port/states.tsv, port3d/states.tsv), which sorts the play tours' periodic shots by level and screen.

    python games/heavyweapon/tools/idplay/screensmd.py [--check]

--check writes nothing and exits 1 when a shot of the run is in no row or a row with no reason has no shot."""
import argparse
import fnmatch
import glob
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..'))
OUT_ROOT = os.path.join(ROOT, 'build', 'logs', 'idplay')
MERGED = os.path.join(OUT_ROOT, '_screens')
SCREENS_MD = os.path.join(ROOT, 'games', 'heavyweapon', 'docs', 'SCREENS.md')
PERIODIC_TOURS = ('shots_campaign', 'shots_levels3', 'shots_levels11', 'shots_survival', 'shots_boxes')


def periodic(name):
    """a play tour's shot taken every so many frames, or before a play tip's OK: sorted by its state, not its name"""
    tour = next((tour for tour in PERIODIC_TOURS if name.startswith(tour + '_')), None)
    return tour is not None


def state_is(screen=None, mode='campaign', level=None, test=None):
    """a row's selector of periodic shots by the port's state at the shot"""
    def select(name, state):
        if not periodic(name) or state is None:
            return False
        if screen is not None and state.get('screen') not in ((screen,) if isinstance(screen, str) else screen):
            return False
        if mode is not None and state.get('mode') != mode:
            return False
        if level is not None and state.get('level') != level:
            return False
        return test is None or test(state)
    return select


def tip(name, state):
    return periodic(name) and '_tip_' in name


def in_level(level):
    return state_is('game', level=level, test=lambda state: state.get('dialog', -1) == -1)


# (screen, reached by, shots, note): shots are patterns (a shot's _3d twin matches too unless the pattern ends in "!2D",
# which takes the 2D shot alone) or selectors of periodic shots; a row with no shots says why in its note
SMOKE = ("explained: translucent smoke drawn over a fire, whose blend the 2005 framework's software blitters and SDL "
         'round apart (IDENTICAL_PLAY.md, "Oddities": translucent pixels on the 2D screen)')
LEVEL_NOTES = {13: SMOKE + ', at most 72 levels in shot shots_levels11_09100',
               14: SMOKE + ', at most 97 levels over the boss in shot shots_levels11_14800'}

ROWS = [
    ('TitleScreen: the intro movie (intro.swf, played by the shared SWF player in the port, by the Flash control in the '
     'original), ended by a click', 'launch', ['menus_intro_*'],
     'explained: the original\'s Flash control plays the movie on the wall clock and the port one movie frame every 100 / '
     'frame-rate updates (a listed deviation), so a shot catches another frame of it; the first frame and one late frame '
     'are shot'),
    ('TitleScreen: the title with its loading bar full and "click here to play"', 'the intro\'s end',
     ['menus_title_early', 'menus_title', 'fs_title'],
     'explained: the click that ends the intro is taken by the port in the update it is given and read by the original '
     'before the next, so the title\'s fade and the glow of "click here to play" run an update ahead in the port (its '
     'picture after update 151 is the original\'s after 152; IDENTICAL_PLAY.md, "Tours and results")'),
    ('TitleScreen: the loading bar while it fills', 'launch', [],
     'none: both games load before the update every run starts from (the original\'s loading on the main thread in the '
     'comparison, the port\'s in one go, a listed deviation), so no update draws the bar part full'),
    ('UserNameDialog (0x1b): the first run\'s name box, empty and typed', 'the title\'s click with no player',
     ['menus_namebox', 'menus_name_typed', 'fs_namebox',
      state_is('menu', mode=None, test=lambda state: state.get('dialog') == 0x1b)], ''),
    ('MessageDialog (0x1f): an empty name refused', 'the name box\'s OK with no name', ['cov_refused'], ''),
    ('MainMenu: the main menu, the button descriptions under the pointer and the player\'s welcome', 'the name box; '
     'every screen\'s way back', ['menus_mainmenu*', 'cov_mainmenu*', 'dlg_mainmenu_*', 'fs_mainmenu*', 'opt_mainmenu',
                                       state_is('menu', mode=None, test=lambda state: state.get('dialog', -1) == -1)], ''),
    ('YesNoDialog (0x20): the main menu\'s quit question, one of ten quotes at random', 'QUIT',
     ['menus_quitbox', 'dlg_quit*'], 'ten in a row, five quotes among them'),
    ('MessageDialog: survival locked', 'SURVIVAL before three levels are passed', ['cov_locked', 'dlg_locked'], ''),
    ('OptionsDialog (7): the sliders, the four boxes, HELP, CREDITS and DONE', 'OPTIONS, from the main menu or the '
     'status bar\'s box', ['menus_options*', 'cov_options', 'cov_dragged', 'cov_hints_off', 'opt_options',
                           'opt_fullscreen_*', 'opt_3d_answered*', 'ingame_options'],
     'a slider dragged and clicked, each box ticked and unticked'),
    ('OptionsDialog: the custom cursors box ticked', 'the options\' custom cursors box', ['cov_cursors_off'],
     'explained: the tour starts with the game\'s own cursors off, so the click turns them on and the original draws its '
     'hand into the frame from then on; the port\'s `screenshot` always leaves the drawn cursor out'),
    ('OptionsDialog in full screen: the full screen box', 'OPTIONS in a game started full screen', ['fs_options'],
     'explained: on rundesk\'s headless desktop the original cannot open its full-screen mode and falls back to a '
     'window, so its box is unticked where the port\'s, full screen, is ticked; every other full-screen shot matches'),
    ('MessageDialog: hardware acceleration not supported', 'the options\' 3D box ticked where there is no 3D',
     ['opt_3d_box!2D'], 'the sandbox\'s DirectDraw has no 3D, and the port\'s 2D run uses SDL\'s software renderer '
     '(the tour\'s PORT_ENVIRONMENT), whose answer is the same'),
    ('YesNoDialog: turn hardware acceleration off?', 'the options\' 3D box unticked where 3D is recommended',
     ['opt_3d_box_3d'], 'the 3D runs: the original on the visible desktop, the port on its hardware renderer'),
    ('YesNoDialog: hardware acceleration not recommended', 'the options\' 3D box ticked where 3D is supported but not '
     'recommended', [], 'rare: the original asks its 3D driver check (DDInterface), which passes on this machine\'s card; '
     'the port answers both questions from one flag (its renderer is accelerated or not), so it never shows the box'),
    ('MessageDialog: full screen cannot be turned off', 'the options\' full screen box unticked when the app is forced '
     'to full screen', [], 'rare: PopCap\'s framework forces full screen when the desktop cannot show the window (too '
     'few colours or too small a screen), which no test machine has; the port\'s framework never forces it'),
    ('CreditsScreen', 'the options\' CREDITS', ['menus_credits_*'], 'as it starts and scrolled'),
    ('HelpScreen: the four pages turning, the box that hides it next time', 'HELP from the main menu or the options; '
     'before the first campaign game and the first survival game',
     ['menus_help', 'menus_options_help', 'cov_help_*', 'fs_help', state_is('help', mode=None)], ''),
    ('HighScoresScreen: the mission and survival lists, a new score\'s line blinking', 'HIGH SCORES; the end of a game',
     ['menus_highscores*', 'dlg_highscores', 'dlg_highscores_survival', 'boxes_highscores*',
      state_is('highscores', mode=None)], ''),
    ('YesNoDialog: reset the scores (mission and survival), and the high scores after it', 'the high scores\' RESET',
     ['dlg_reset_mission', 'dlg_reset_survival', 'dlg_highscores_back', 'dlg_highscores_survival_back'],
     'explained: the box waits in an update loop of its own (Dialog::WaitForResult), which a test script\'s click starts '
     'inside an update in the port and between updates in the original, so the fire\'s bubbles stand an update off '
     'while the box is up and after it (the port\'s picture an update later, then an update earlier, is the nearest; '
     'what remains is the fire\'s known rounding), while the state, both generators included, agrees at every frame '
     '(IDENTICAL_PLAY.md, "The screen pass")'),
    ('UserListDialog (0x15): the players, one picked', 'the main menu\'s welcome', ['menus_userlist', 'cov_userlist',
                                                                                   'cov_picked', 'dlg_userlist*'], ''),
    ('UserNameDialog: a new player from the list, a rename', 'the list\'s first line, RENAME',
     ['cov_newplayer', 'dlg_newplayer*', 'dlg_rename'], ''),
    ('MessageDialog: the name is taken', 'a new player named as one there is', ['dlg_taken'], ''),
    ('YesNoDialog: delete the player?', 'the list\'s DELETE', ['dlg_delete*'], 'answered NO, then YES'),
    ('ContinueDialog (0x3b): continue the campaign or begin a new one', 'CAMPAIGN with a campaign saved',
     ['boxes_continue'], ''),
    ('MapScreen: the map, its zoom and the briefing', 'a campaign\'s start; ADVANCE after a level\'s armory',
     ['ingame_map', 'ingame_into_level', 'fs_map', 'fs_zoom', 'cov_into_level', state_is('map')],
     "explained: in 3D the zoom's stretched map is filtered by each program's renderer (Direct3D 7 in the original, "
     "SDL's in the port), which round its blurred edges apart by up to 72 levels on a few hundred pixels; in 2D it "
     'matches'),
    ('Armory: the weapons and their upgrades', 'a level\'s results; CONTINUE in the continue box',
     ['boxes_armory*', state_is('armory')], ''),
    ('MessageDialog: points left to spend', 'the armory\'s GO with points left', ['boxes_points'], ''),
    ('Banner: the level\'s name as it starts', 'every level', [state_is('game', test=lambda state: state.get('timer', 9999) < 300)],
     'the periodic shots that fall in a level\'s first 300 updates'),
] + [
    ('Board: level %d, its enemies, pickups and boss, the status bar' % level,
     'the campaign; `level %d`' % level,
     (['ingame_level1', 'ingame_resumed', 'ingame_continued', 'ingame_after_no', 'cov_resumed', 'cov_end', 'fs_level1',
       'fs_resumed', 'boxes_resumed'] if level == 1 else []) + [in_level(level)], LEVEL_NOTES.get(level, ''))
    for level in range(1, 20)
] + [
    ('NukeBlast: the nuke', 'the right button in a level', ['cov_nuke'], ''),
    ('PlayTipDialog (0x2c): the auto-fire, nuke, friendly and mega laser tips', 'the first time each comes up in a game',
     ['cov_tip*', tip, state_is('game', mode=None, test=lambda state: state.get('dialog') == 0x2c)], 'each shot the update before the bot\'s click on its OK'),
    ('QuitDialog (3): the status bar\'s MENU box', 'MENU in a level', ['ingame_menubox*', 'boxes_menubox*', 'fs_menubox'], ''),
    ('YesNoDialog: retreat to the main menu? (a tank left to lose: the tall box)', 'the MENU box\'s MAIN MENU',
     ['ingame_confirm'], ''),
    ('YesNoDialog: retreat to the main menu? (the last tank lost: the short box)', 'the MENU box\'s MAIN MENU while the '
     'last tank\'s wait runs', ['boxes_confirm_short'], ''),
    ('PauseScreen: paused by the space bar', 'SPACE in a level', ['cov_paused'], ''),
    ('PauseScreen: the focus lost and regained ("click anywhere to continue")', 'another window taking the focus',
     ['ingame_focus_*', 'cov_focus_lost'], ''),
    ('ResultsScreen: a level\'s results', 'a level\'s end', [state_is('results')], ''),
    ('CutsceneScreen: the middle and the final movies (middle.swf, final.swf)', 'after level 10\'s and level 19\'s '
     'results', [state_is('none')],
     'explained: the movies play on the original\'s wall clock (the Flash control) and on the port\'s update count, so a '
     'shot catches another frame'),
    ('GameOverDialog (0x28): the campaign\'s and survival\'s game over', 'the last tank lost',
     ['boxes_gameover', state_is('gameover', mode=None)], ''),
    ('Board: survival, its rising waves and the status bar\'s time', 'SURVIVAL once unlocked',
     [state_is('game', mode='survival')], ''),
    ('Windows message boxes: a fatal error, a missing animation, an old players file, a bad high-score file', 'a '
     'damaged install or save', [], 'rare: the original shows them only for damaged files; origrun answers a box unseen, '
     'and the port\'s test runs show none (a listed deviation)'),
    ('The registration and trial screens', 'the unregistered game', [],
     'none: the port keeps the registered path only (registration removed, docs/COORDINATION.md)'),
]


def shot_names():
    """the shots the merged run compares: those on both sides"""
    orig = {os.path.splitext(leaf)[0] for leaf in os.listdir(os.path.join(MERGED, 'orig'))}
    port = {os.path.splitext(leaf)[0] for leaf in os.listdir(os.path.join(MERGED, 'port'))}
    return sorted(orig & port)


def shot_states():
    states = {}
    for path in glob.glob(os.path.join(OUT_ROOT, '*', 'port*', 'states.tsv')):
        for line in open(path, encoding='utf-8'):
            name, _, state = line.rstrip('\n').partition('\t')
            try:
                states[name] = json.loads(state)
            except ValueError:
                pass
    return states


def matches(pattern, name):
    if pattern.endswith('!2D'):
        return fnmatch.fnmatchcase(name, pattern[:-3])
    return fnmatch.fnmatchcase(name, pattern) or (name.endswith('_3d') and fnmatch.fnmatchcase(name[:-3], pattern))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    names = shot_names()
    states = shot_states()
    used = set()
    rows = []
    problems = []
    for screen, reached, selectors, note in ROWS:
        taken = []
        for name in names:
            for selector in selectors:
                hit = matches(selector, name) if isinstance(selector, str) else \
                    selector(name[:-3] if name.endswith('_3d') else name, states.get(name))
                if hit:
                    taken.append(name)
                    break
        used.update(taken)
        if not taken and not note.split(':')[0] in ('rare', 'none', 'unreachable'):
            problems.append('no shot for: ' + screen)
        two = [name for name in taken if not name.endswith('_3d')]
        three = [name for name in taken if name.endswith('_3d')]
        cell = ' '.join(two) + (' 3D: ' + ' '.join(three) if three else '')
        rows.append('| %s | %s | %s | %s |' % (screen, reached, cell.strip(), note))
    unused = [name for name in names if name not in used]
    for name in unused:
        problems.append('in no row: %s (%s)' % (name, states.get(name)))
    for problem in problems:
        print('screensmd: ' + problem)
    if args.check:
        return 1 if problems else 0
    with open(SCREENS_MD, 'w', encoding='utf-8', newline='\n') as f:
        f.write(HEAD)
        f.write('| Screen | Reached by | Shots | Note |\n|---|---|---|---|\n')
        f.write('\n'.join(rows) + '\n')
    print('screensmd: %d rows, %d shots, %d in no row' % (len(rows), len(names), len(unused)))
    return 1 if problems else 0


HEAD = '''# Heavy Weapon: every screen, listed

Every screen and box the game can show (`docs/SHOTDIFF.md`, "Every screen, listed"), from our export and the port's
source, with the shots that compare it. The shots are the identical-play tours' (`tools/idplay/idplay.py`, its scenarios
in `tools/idplay/scenarios/`): the original in Windows Sandbox under origrun and the port replaying its input at the
updates the original read it, a shot at the update each of the original's landed on (the `shots` step), and in 3D the
original on the visible desktop under tools/visrun.py with the port on its hardware renderer (the `shots3d` step; those
shots end in `_3d`). `fs_` shots are full screen (`shots_fullscreen`). `idplay.py screens` merges every tour's pairs
into the shotdiff run `screens`, which tools/screencheck.py reads. The play tours' periodic shots (named
`<tour>_<frame>`) are sorted into the rows by the screen, mode and level the port's `state` gives at each
(`port/states.tsv`). This file is written by `tools/idplay/screensmd.py`; the pass is in `IDENTICAL_PLAY.md`, "The
screen pass".

'''

if __name__ == '__main__':
    sys.exit(main())
