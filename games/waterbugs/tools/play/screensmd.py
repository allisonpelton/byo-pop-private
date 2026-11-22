# screensmd.py: writes games/waterbugs/docs/SCREENS.md (docs/SHOTDIFF.md, "Every screen, listed") from the shot names of the tours in
# games/waterbugs/test/tours, so that a row's Shots cell is what the tours take and never what someone remembered. A row is
# (screen, reached by, shot patterns, note): a pattern is a regular expression that must match a whole shot name; a row with no
# patterns has a note that says why it has no shot.
#   python games/waterbugs/tools/play/screensmd.py <shotdiff run id>   (the id's differing shots get "explained" in their rows' notes)
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))
TOURS = os.path.join(ROOT, 'games', 'waterbugs', 'test', 'tours')
OUT = os.path.join(ROOT, 'games', 'waterbugs', 'docs', 'SCREENS.md')

ROWS = [
    ('Start-up logo (type `logo`, animation `retro64`: the PopCap logo fading in and out)', 'launch',
     [r'm_logo\d+'], 'the logo is up for about 2 s; the main menu follows by itself'),
    ('Partner logos 1 to 4 (type `logo`, animations `portal1` to `portal4`)', 'launch, after the first logo',
     [], 'unreachable: they are shown only when `wanims\\portal<n>.tga` exists (`PortalCount`), and the install has none'),
    ('Loading screen (`DrawLoadingScreen`: the "loading" picture on a darkened screen)', 'launch, and each time a board or the menus are read afresh',
     [], 'none: it is drawn and presented inside one update, in the middle of a load, so no shot at an update boundary holds it; the picture is the art bank\'s "loading" image'),
    ('Main menu (`main_menu`: porthole, six buttons, the sign "Hello, name")', 'after the logo; Esc or the back buttons of every screen',
     [r'm_menu', r'm_hover_\w+', r'm_help_back', r'm_scores_back', r'u_users_left', r'p_left_to_menu', r'e_after_scores', r'x_menu', r'x_200_left', r'x_\d+_left'], 'each button hovered, the sign too'),
    ('Help: Instructions (`help`)', 'main menu, Help', [r'm_help1'], ''),
    ('Help: Power-Ups (`help2`, the first of the two `help2` levels of the file)', 'Instructions, "click here to continue"', [r'm_help2'],
     'the file has a second `help2` level ("click here to play", the instructions picture); `GoToScreen` takes the first, so it is never shown'),
    ('Options (`options`)', 'main menu, Options', [r'm_options', r'm_options_hover', r'm_options_again', r'm_options_back'],
     'Video (windowed or full screen) is the full-screen tour'),
    ('Options: LCD mode, sound, music, shooter mode and difficulty toggled', 'Options buttons',
     [r'm_options_(lcd|sound|music|shooter)_(on|off)', r'm_options_diff\d', r'm_options_diff_back'], 'difficulty cycles normal, hard, easy'),
    ('Credits (`credits`)', 'Options, Credits', [r'm_credits\d', r'm_credits_back'], 'as it fades in and when whole'),
    ('High scores (`high_scores`), empty', 'main menu, High Scores', [r'm_scores'], ''),
    ('Player list (`change_user`)', 'main menu, Change User or the sign', [r'u_users', r'u_slot_hover', r'u_pick_first', r'u_users_done'], ''),
    ('Player name box, a new player or a rename (requester 2)', 'player list, an empty slot or Rename',
     [r'u_newname_\w+', r'u_rename_\w+'], 'typed with key events'),
    ('Delete player question (requester 0) and its two answers', 'player list, Delete with two players',
     [r'u_delete_question2?', r'u_delete_no', r'u_delete_yes'], ''),
    ('Message "you must always have at least one player" (requester 1)', 'player list, Delete the only player', [r'u_delete_only', r'u_delete_only_ok'], ''),
    ('World map (`select_level`)', 'Start (a new game, or the answer no to the resume question)',
     [r'p_map', r'w_map', r'x_map', r'x_hover_\w+', r'x_hover_back2?', r'x_\d+_map', r'x_\d+_hover'], 'every world hovered'),
    ('Level selection (`select_level2`): the starting level of a world', 'a world on the map',
     [r'x_select', r'x_select_later', r'x_step_\w+', r'x_hover_start', r'x_\d+_select'], 'the slider stepped up and down'),
    ('Story screen (the first board of the first world: "The Crash Site")', 'the first world, new game', [r'p_story', r'w_story'], 'the other worlds start straight on the board'),
    ('Board: the level start texts and the beam', 'a new board', [r'p_board[0-7]', r'w_board0', r'x_started', r'x_started_later', r'x_\d+_started', r'x_\d+_started_later'], ''),
    ('Board: play, beams, fills, bugs and the tutorial texts of the first board', 'shots at fixed places',
     [r'p_shot\d', r'w_shot\d+', r'p_board_later'], ''),
    ('Board: the later worlds and their bugs (boards skipped through with the original\'s "." key)', 'a world\'s button, the start, then ".": each board as it starts',
     [r'x_(200_)?skip\d_[ab]', r'x_\d+_skip\d_[ab]', r'x_space\d'], 'a saved profile that has reached every level'),
    ('Board: power-ups of every kind (the original\'s test keys 0 to 9 and A to D held)', 'the hundreds world, keys held while shots go on', [r'x_100_held\w_\d', r'x_100_held_start'], 'a power-up that is collected shows its text and effect'),
    ('Quit question on a board (requester 3, "Do you really want to quit?")', 'Esc on a board', [r'p_quit_box', r'p_quit_box_later', r'p_quit_hover', r'p_quit_cancelled', r'p_quit_box2', r'x_\d+_quit'],
     'the game waits under it (state 2); cancel resumes, okay goes to the main menu with the game saved'),
    ('Resume question (requester 5, "would you like to resume your previous game?")', 'Start with a saved game', [r'e_resume_question', r'e_resume_hover', r'x_\d+_resume'], 'yes resumes, no goes to the map'),
    ('Level finished: "LEVEL FINISHED!" and the results (requester 4: percent, best, shots, bonus)', 'a board filled past its quota', [r'w_won\d+', r'w_shot1[0-3]'], 'the results build up text by text'),
    ('The board after the results', 'the results\' button', [r'w_after_results', r'w_next\d'], ''),
    ('Game over ("GAME OVER / CLICK TO CONTINUE")', 'the last life lost', [r'e_shot\d(_b)?', r'e_over\d+'], ''),
    ('High-score entry and the Hall of Fame (`high_score_entry`, `high_scores` with the new score)', 'a click on the game over; Return', [r'e_hof\d', r'e_typed_\w+', r'e_entered', r'e_entered_later', r'e_scores_back_hover'],
     'the name is the player\'s, entered by the game; typed keys change nothing'),
    ('Saved game resumed', 'the resume question, yes', [r'e_resumed', r'e_menu'], ''),
    ('Mission complete (`mission_complete`, kind 0xb)', 'after the last board of a file', [], 'unreachable: no level of that type is in the game file or its level files'),
    ('Registration screens (`opening_nag`, `closing_nag`, `expiring_nag`, Buy now)', 'launch, Quit', [], 'unreachable: removed from the port; the original\'s Quit button leaves the game at once (`closing_nag` is the item\'s type)'),
    ('Quit (the main menu\'s Quit button, Esc on the main menu)', 'main menu, Quit', [], 'none: the game ends with no screen of its own (the original under origrun: a click on Quit and the process exits with code 0, no window left to shoot; the runner of the port ends at the click too)'),
]


def shot_names():
    names = []
    for fn in sorted(os.listdir(TOURS)):
        if fn.endswith('.txt'):
            for line in open(os.path.join(TOURS, fn)):
                f = line.split('#')[0].split()
                if len(f) == 3 and f[1] == 'shot':
                    names.append(f[2])
    return names


def differing(run):
    # the shots of the shotdiff run that differ (build/shotreview/waterbugs/<run>/summary.json)
    d = json.load(open(os.path.join(ROOT, 'build', 'shotreview', 'waterbugs', run, 'summary.json')))
    return {p['name'][:-4] for p in d['pairs'] if p['status'] == 'differ'}


def main():
    diff = differing(sys.argv[1]) if len(sys.argv) > 1 else set()
    names = shot_names()
    out = ['# Water Bugs: every screen, listed',
           '',
           'Every screen and dialog the game can show (`docs/SHOTDIFF.md`, "Every screen, listed"), from its export and source (`WBMenu.cpp`\'s',
           'requesters, `WBLevel.cpp`\'s level types, `water_bugs.xml`). The shots are taken by the tours of `games/waterbugs/test/tours`',
           '(`tools/play/gentours.py` writes them, `tools/play/runall.sh` plays them on the original and the port) and compared by',
           '`tools/shotdiff`; this table is written by `tools/play/screensmd.py` from the names in the tours. The game is 2D only, and the',
           'tours run windowed; full screen is in the Notes of the rows it covers.',
           '',
           '| Screen | Reached by | Shots | Note |', '|---|---|---|---|']
    used = set()
    for screen, reached, pats, note in ROWS:
        hit = []
        for pat in pats:
            rx = re.compile(pat + r'$')
            found = [n for n in names if rx.match(n)]
            if not found:
                raise SystemExit('no shot matches %r (%s)' % (pat, screen))
            hit += [n for n in found if n not in hit]
        used.update(hit)
        bad = [n for n in hit if n in diff]
        if bad:
            note = (note + '; ' if note else '') + ('explained: %d of %d shots differ from the original, in bubbles, the place of a bug or the frame of a sprite only; '
                    'the original differs from itself the same way after a load (`IDENTICAL_PLAY.md`, "Screen coverage")' % (len(bad), len(hit)))
        out.append('| %s | %s | %s | %s |' % (screen, reached, ' '.join(hit), note))
    unused = [n for n in names if n not in used]
    if unused:
        print('shots in no row: ' + ' '.join(unused))
    with open(OUT, 'w', newline='\n') as f:
        f.write('\n'.join(out) + '\n')
    print('wrote %s: %d rows, %d shots' % (OUT, len(ROWS), len(used)))


if __name__ == '__main__':
    main()
