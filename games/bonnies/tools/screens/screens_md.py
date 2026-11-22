"""writes games/bonnies/docs/SCREENS.md"""
BE = [4, 8, 12, 16, 20, 25, 29, 33, 37, 41, 45, 50]
MW = [3, 7, 14, 23, 34, 43]


def be(levels, kinds):
    return " ".join(f"be_{k}{L:02d}" for L in levels for k in kinds)


def mw(kinds):
    return " ".join(f"mw_{k}{L:02d}" for L in MW for k in kinds)


rows = []


def row(screen, reached, shots, note):
    rows.append((screen, reached, shots, note))


row("LogoScreen (state 1): the New Crayon logo, its letters, crayon and TM, then PopCap's logo",
    "launch", "logo",
    "The logos run on the game's clock and the port's shot is not at the same frame as origrun's first drawn one, so the "
    "whole picture is masked (the reason is in shotdiff.json); `lg00` to `lg40` is a real-time series of both.")
row("MenuScreen (state 2): the main menu, its swinging sign, Bonnie's waving hand and the greeting bubble",
    "after the logos; `goto menu`", "menu mainmenu endmenu ae_menu lk_menu st_menu",
    "The version text reads \"Version 1.01\" in the port where the original reads \"Deluxe Version 1.01\" (the user's "
    "request of 2026-11-18, DEVIATIONS.md): that strip is masked, with its reason, in every pair with the menu.")
row("NamePanel: a new player's name", "Classic Game with no player, or New in the players panel", "name st_name z_newname", "")
row("Message dialog: a profile with that name already exists", "OK in the name panel with a taken name", "z_exists",
    "The name panel after it (`z_exists2`) is the same panel with the typed name.")
row("PlayersPanel: the list of players", "the greeting bubble's [change player]", "z_players z_afteresc z_nodelete", "")
row("Message dialog: delete this player's profile, Yes or No", "Delete in the players panel", "z_delete",
    "Yes is not taken: it deletes the player's saved games.")
row("GameDialog: choose your difficulty", "a new game", "difficulty st_difficulty", "")
row("GameDialog: a game in progress, resume or start a new one (classic and action)",
    "Classic Game or Action Game with a saved game that has a score",
    "l_resume lk_resume ae_dialog " + be(BE, ["resume"]) + " " + mw(["resume"]) +
    " aw_resume05 aw_resume20 gx_ga_resume gx_gb_resume gx_gc_resume", "")
row("GameDialog: the starting point (the level chooser), all 50 levels unlocked",
    "New in the resume dialog, a player whose best level is 50; the arrow keys move by a level",
    "l_choose0 " + " ".join(f"l_t{L:02d}" for L in range(1, 51)) + " l_aftercancel",
    "Each thumbnail is a picture of the level; Cancel returns to the menu.")
row("GameDialog: the starting point with levels locked", "the same, for a player whose best level is 12",
    "lk_c0 lk_c1 lk_c2 lk_c3 lk_c13 lk_after", "Levels past the best one plus one show LOCKED and have no OK button.")
row("StoryScreen: the three pages of Bonnie's story before level 1", "a new classic or action game",
    "story st_page1 st_page2 st_page3", "")
row("StoryScreen: a chapter's page (the picture and its typed lines) before the next level", "after a won level",
    mw(["next"]) + " " + be(BE, ["story"]) + " aw_next05 aw_story20", "The ending's pages after level 50 are `be_story50`, `be_next50`.")
row("GameScreen, classic: a level's board, with its first tutorial balloon where it has one",
    "Classic Game, a chosen level; ESC leaves a board and the options' Main Menu returns",
    "board tutorial word played cake tale st_board st_tutorial " + " ".join(f"l_b{L:02d}" for L in range(2, 51)),
    "The letters of a level begun by the clock cannot match (the sparkles draw from rand on every frame), so the tile area "
    "is masked in those pairs; the boards of saved games, which both sides load, are compared whole (`be_load`, `mw_load`, "
    "`gx_*_load`).")
row("GameScreen, classic, a saved game's board loaded, and the board after a won level",
    "Resume in the resume dialog", be(BE, ["load"]) + " " + mw(["load"]) + " " + mw(["board"]) +
    " gx_ga_load gx_gb_load gx_gc_load " + be(BE, ["next"]), "")
row("GameScreen, action: the timed board, its bar and Submit", "Action Game", "action actionword ae_resumed aw_load05 aw_load20", "")
row("OverlayScreen: Paused", "the window loses the focus in the action game", "paused resumed", "")
row("Chapter complete summary (classic and action)", "the last word of a chapter", mw(["end"]) + " aw_end05",
    "The score counts up, so each shot is taken once the count has stopped.")
row("BookEndScreen: book complete, and the publisher's letter", "the last chapter of each of the 12 books",
    be(BE, ["end", "letter"]) + " aw_end20 aw_letter20", "")
row("Message dialog: you cannot scramble any more and have one turn left, resign?",
    "Resign (the Scramble button) with one turn left", "gx_ga_resign gx_gc_resign", "")
row("Message dialog: game over (no moves left; the turns used up; time's up), with or without the high score line",
    "Yes to resigning; the last turn used; the action clock at nothing", "gx_ga_over gx_gb_over gx_gc_over timeup",
    "The three texts are 0x4a4, 0x4a5 and 0x4a6; the line about the high score list is added when the score enters it.")
row("HighScoresScreen: classic and action tables, a player's summary",
    "game over with a score on the list; the game options' High Scores",
    "highscores gx_ga_scores gx_ga_scores2 gx_gb_scores", "")
row("CreditsScreen", "the game options' Credits", "credits", "")
row("HelpScreen: three pages", "the game options' How To Play; the main menu's How To Play", "help1 help2 help3", "")
row("OptionsMenu: the game's options, in a game and from the main menu", "the Menu button, ESC; Options on the main menu",
    "gameoptions options " + be(BE, ["menu"]) + " " + mw(["menu"]) + " aw_menu05 aw_menu20", "")
row("Main menu after a finished game", "the end of a game", "endmenu gx_ga_menu gx_gb_menu gx_gc_menu", "")
row("Ending after level 50", "the last word of the last chapter", "be_end50 be_letter50 be_story50 be_next50",
    "The high score entry that follows a won game is `be_next50`'s successor and is not shot.")
row("InterstitialScreen: the advertisement's four kinds", "none", "",
    "unreachable: `showInterstitial` is called when the game's sequence is in state 0x12, which nothing sets, and from "
    "`OptionsMenu::update` after the quit dialog, whose flag `mQuitDialogUp` nothing sets (the port has no other writer).")
row("OptionsMenu's quit question", "none", "",
    "unreachable: `mQuitDialogUp` is never set, so the options menu never asks it.")
row("OverlayScreen: the loading text", "none", "",
    "unreachable: it draws when `mLoadImages[7]` is set, and nothing sets that.")

header = (
    "# Bonnie's Bookstore Deluxe: every screen and dialog\n\n"
    "One row for each screen or dialog the game can show (`docs/SHOTDIFF.md`, \"Every screen, listed\"), taken from "
    "`game/Screens.h`, `game/GameScreen.cpp` and the dialogs they call, and checked against the audit of 2026-11-21. The "
    "shots are pairs of the newest shotdiff run: the original under origrun in Windows Sandbox against the port with "
    "`-fast -seed=1`, windowed and 2D (the game has no 3D mode and no second size; its Full Screen option draws the same "
    "800x600 picture).\n\n")
out = header + "| Screen | Reached by | Shots | Note |\n|---|---|---|---|\n"
for r in rows:
    out += "| " + " | ".join(c.replace("|", "/") for c in r) + " |\n"
open('games/bonnies/docs/SCREENS.md', 'w', newline='\n').write(out)
print(len(rows), 'rows')
