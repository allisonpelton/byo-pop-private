"""screens_md.py <shotdiff run folder>: writes games/bonnies/docs/SCREENS.md from the run's pairs (every shot named is a
pair of the run; a row whose shots differ gets the cause of each difference, judged by looking at the run's crops)"""
import json
import re
import sys

RUN = sys.argv[1]
summary = json.load(open(RUN + "/summary.json"))
status = {p["name"][:-4]: p["status"] for p in summary["pairs"]}

# the causes of the differences, each judged on the crops of the run (the game's own clock moves these; no one sees a wrong
# picture in them)
CAUSES = [
    (r"(be|mw|aw)_menu\d+|(be|mw|aw)_end\d+|be_letter\d+|aw_letter\d+|(be|mw|aw)_story\d+|(be|mw|aw)_next\d+|mw_board\d+",
     "the screens after a win differ by the clock's animations only: the typewriter's text typed to another point, "
     "\"Click to Continue\" pulsing, the tiles on the board behind a dialog wobbling, glowing and falling"),
    (r"(name|st_name|m_options|z_newname|z_exists2?|z_afteresc)",
     "the status line under a dialog (\"Play the game with no time pressure\", \"Sound and display options...\") is shown for "
     "a time after the click, and the shot falls before or after its end"),
    (r"st_page\d|st_tutorial|m_help\d",
     "the page's arrow pulses between two colours on the clock, and the help's demonstration tiles light up on it"),
    (r"action|actionword|ae_resumed|gx_gb_.*|aw_end\d+|be_load\d+",
     "the action clock's bar and the Submit button shimmer on the clock, a tile fades its highlight, and tiles fall at "
     "another point of their fall"),
]


def cause(names):
    out = []
    for n in names:
        text = next((t for pattern, t in CAUSES if re.fullmatch(pattern, n)), "UNEXPLAINED " + n)
        if text not in out:
            out.append(text)
    return out


rows = []


def levels(prefix, levs):
    return [f"{prefix}{L:02d}" for L in levs]


BE = [4, 8, 12, 16, 20, 25, 29, 33, 37, 41, 45, 50]
BE_END = [L for L in BE if L != 33]
MW = [3, 7, 14, 23, 34, 43]


def row(screen, reached, shots, note=""):
    shots = [s for s in shots]
    for s in shots:
        if s not in status:
            raise SystemExit("not a pair of the run: " + s)
    differ = [s for s in shots if status[s] == "differ"]
    if differ:
        note = (note + " " if note else "") + "Differences explained: " + "; ".join(cause(differ)) + "."
    rows.append((screen, reached, " ".join(shots), note))


def none(screen, reached, note):
    rows.append((screen, reached, "", note))


row("LogoScreen: New Crayon's logo (falling letters, crayon, TM), then PopCap's and the distributor's",
    "launch", ["logo"],
    "Only the first logo frame is a pair: the logos run on the game's clock and a test run skips them, so the whole "
    "picture is masked in shotdiff.json (its reason there); the rest of the logos were looked at once, by hand, in a real-time run.")
row("MenuScreen: the main menu (the swinging sign, Bonnie's waving hand, the greeting bubble)",
    "after the logos; Main Menu in the game's options; after a game",
    ["menu", "m_menu", "m_after", "zz_menu", "l_menu", "ae_menu", "st_menu", "lk_menu", "gx_menu", "be_menu", "mw_menu", "aw_menu", "lk_after"],
    "The version text reads \"Version 1.01\" in the port where the original reads \"Deluxe Version 1.01\" (the user's "
    "request of 2026-11-18, DEVIATIONS.md), and the sign and the hand move on the clock: both are masked in every pair "
    "with the menu, with their reasons in shotdiff.json.")
row("NamePanel: a new player's name", "Classic Game or Action Game with no player; New in the players panel",
    ["name", "st_name", "z_newname"])
row("Message dialog: a profile with that name already exists", "OK in the name panel with a taken name", ["z_exists", "z_exists2"])
row("PlayersPanel: the list of players", "the greeting bubble's [change player]; Escape out of the name panel",
    ["z_players", "z_afteresc", "z_nodelete"])
row("Message dialog: delete this player's profile, Yes or No", "Delete in the players panel", ["z_delete"],
    "No is taken, so that the saved games stay.")
row("GameDialog: choose your difficulty", "a new game", ["difficulty", "st_difficulty"])
row("GameDialog: a game in progress, resume or start a new one (classic and action)",
    "Classic Game or Action Game with a saved game that has a score",
    ["l_resume", "lk_resume", "ae_dialog"] + levels("be_resume", BE) + levels("mw_resume", MW) + ["aw_resume05", "aw_resume20",
    "gx_ga_resume", "gx_gb_resume", "gx_gc_resume"])
row("GameDialog: the starting point (the level chooser), all 50 levels", "New in the resume dialog of a player whose best level is 50; the arrow keys move a level",
    ["l_choose0"] + [f"l_t{L:02d}" for L in range(1, 51)] + ["l_aftercancel"], "Each thumbnail is the chosen level's picture; Cancel returns to the menu.")
row("GameDialog: the starting point with levels locked", "the same, for a player whose best level is 12",
    ["lk_c0", "lk_c1", "lk_c2", "lk_c3", "lk_c13"], "Levels past the best one plus one say LOCKED and have no OK button.")
row("StoryScreen: Bonnie's story, three pages, before level 1", "a new game", ["story", "st_page1", "st_page2", "st_page3"])
row("StoryScreen: a chapter's page (the picture and its typed lines)", "after a won level",
    levels("be_story", BE_END) + levels("mw_next", MW) + ["aw_story20", "aw_next05", "aw_next20"] + levels("be_next", BE_END))
row("GameScreen, classic: a level's first board, with its tutorial balloon where it has one",
    "Classic Game at a chosen level",
    ["board", "tutorial", "st_board", "st_tutorial"] + [f"l_b{L:02d}" for L in range(2, 51)],
    "A level begun by the clock has other letters than the original's (the sparkles draw from the random generator on every "
    "frame), and its tiles wobble and glow on the clock: the tiles are masked in these pairs (shotdiff.json) and are "
    "compared on saved games' loaded boards, where nothing is drawn from the random generator.")
row("GameScreen, classic: a saved game's board and the board after a won level", "Resume",
    levels("be_load", BE) + levels("mw_load", MW) + levels("mw_board", MW) + ["gx_ga_load", "gx_gb_load", "gx_gc_load", "gx_gb_word"])
row("GameScreen, action: the timed board, its bar and Submit", "Action Game", ["action", "actionword", "ae_resumed", "aw_load05", "aw_load20"])
none("OverlayScreen: Paused", "the window loses the focus in an action game",
     "rare: no pair. The port's shot (`paused`, `resumed` in the tour) has no original beside it: origrun takes the original's "
     "focus away, and its window was gone at that step in the sandbox run.")
row("Chapter complete (classic and action)", "the last word of a chapter", levels("mw_end", MW) + ["aw_end05"])
row("BookEndScreen: book complete, then the publisher's letter", "the last word of a book's last chapter (11 of 12 books)",
    levels("be_end", BE_END) + levels("be_letter", BE_END) + ["aw_end20", "aw_letter20"],
    "The end of book 8 (level 33) was not reached: its patched save's last word makes the first wildcard and a copycat tile and the level goes on.")
row("Message dialog: you cannot scramble and have one turn left, resign?", "Resign (the Scramble button) with one turn left",
    ["gx_ga_resign", "gx_gc_resign"])
row("Message dialog: game over (no moves; the turns used up; the clock at nothing)",
    "Yes to resigning; the last turn used; the action clock out", ["gx_ga_over", "gx_gb_over", "gx_gc_over"])
row("HighScoresScreen: the classic and the action table, and the player's summary",
    "after a game over with a score on the list; the options' High Scores",
    ["gx_ga_scores", "gx_ga_scores2", "gx_gb_scores", "gx_gb_scores2", "m_highscores"])
row("CreditsScreen", "the options' Credits", ["m_credits"])
row("HelpScreen: three pages", "How To Play on the main menu", ["m_help1", "m_help2", "m_help3"])
row("OptionsPanel: the game's options on the main menu, and on a board (with Main Menu, How To Play, Back To Game)",
    "Options on the main menu; the Menu button of a board",
    ["m_options"] + [f"be_menu{L:02d}" for L in BE_END] + levels("mw_menu", MW) + ["aw_menu05", "aw_menu20"])
none("Message dialog: time's up, with the high score line", "the action clock runs out in a game whose score reaches the list",
     "rare: no pair of the dialog with the score line. The dialog without it is `gx_gc_over`; with the line it differs only "
     "by that sentence. The original's action clock does not run in a sandbox (its window has no input focus, which "
     "stops the clock), so the clock was never run out there.")
none("InterstitialScreen: the advertisement's kinds", "none",
     "unreachable: `showInterstitial` is called when the game's sequence is in state 0x12, which nothing sets, and from "
     "`OptionsMenu::update` after the quit dialog, whose flag `mQuitDialogUp` nothing sets.")
none("OptionsMenu's quit question", "none", "unreachable: `mQuitDialogUp` is never set, so the options menu never asks it.")
none("OverlayScreen: the loading text", "none", "unreachable: it draws when `mLoadImages[7]` is set, and nothing sets that.")
none("Ending after level 50 (the game's last story page, then the high scores)", "the last word of level 50",
     "rare: level 50's end is a book end (`be_end50`, `be_letter50`); the last page of the story after it was not reached in a "
     "scripted run, which would play a game of a thousand words (the whole story was played natively on 2026-11-08, "
     "`IDENTICAL_PLAY.md`).")

header = (
    "# Bonnie's Bookstore Deluxe: every screen and dialog\n\n"
    "One row for each screen or dialog the game can show (`docs/SHOTDIFF.md`, \"Every screen, listed\"), from `game/Screens.h`, "
    "`game/GameScreen.cpp` and the dialogs they call. The shots are pairs of the newest shotdiff run: the original under "
    "origrun in Windows Sandbox against the port with `-fast -seed=1`, windowed (the game has no 3D mode and one size; its "
    "Full Screen option draws the same 800x600 picture). `tools/screens/` has the scripts that reach them; `IDENTICAL_PLAY.md`, "
    "\"The screen coverage pass\", tells how.\n\n")
out = header + "| Screen | Reached by | Shots | Note |\n|---|---|---|---|\n"
for r in rows:
    out += "| " + " | ".join(c.replace("|", "/") for c in r) + " |\n"
open("games/bonnies/docs/SCREENS.md", "w", newline="\n").write(out)
print(len(rows), "rows;", sum(len(r[2].split()) for r in rows), "shots")
