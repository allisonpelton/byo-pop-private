# Cosmic Bugs: every screen and dialog

One row per screen or dialog the game can show, taken from the export (`LevelKind` and `RequesterKind` in `src/CBLevel.h`,
the requesters of `src/CBMenu.cpp`) and the game file's level and menu definitions. Every shot is a pair of the newest
shotdiff run (`build/shotreview/cosmicbugs/`), the original in Windows Sandbox beside the port's headless runner (`CB_555=1`,
the original's 5-5-5 screen), both from the tours in `test/tours/`. A shot's name is its tour, an underscore and the
tour's own name for it (`menus_help` is `help` of `menus.txt`; the player-list tour's shots are `players_players_...`).
The game is 2D only, with a window and a full screen that draw the same 800x600 picture, so each picture is taken once.
How a tour is shot and compared is in `IDENTICAL_PLAY.md`, "Screen coverage pass".

| Screen | Reached by | Shots | Note |
|---|---|---|---|
| Logo screen (`LEVEL_LOGO`: the PopCap logo) | the launch | menus_t0200 menus_t0500 menus_t1000 menus_t2000 | The logo is up for the first two seconds (shots at 0.2, 0.5, 1 and 2 s); the title follows at about 2.2 s. |
| Main menu (`LEVEL_MAIN_MENU`), each button hovered | after the logo; `goto title` | menus_hover_help menus_hover_start menus_hover_options menus_hover_hs menus_hover_instr menus_hover_credits menus_hover_quit menus_hover_user | The player's name ("Hello, ...") is masked: the original takes the Windows user name, the host gives GUEST. |
| Main menu after a player is chosen, a game is left or ended | Change User, Esc from a board, a game over | menus_title_abc menus_esc_title end_title_after end_title_after2 board_late_title board_late_title2 | |
| Help (`LEVEL_HELP`, the Help button's instructions page) | Help on the main menu | menus_help menus_help2 menus_help_back | |
| Instructions (`LEVEL_HELP2`, "click here to play" on a first game) | Instructions on the main menu; Start Game with a new player | menus_instr menus_instructions_play menus_start_again | |
| Credits (`LEVEL_CREDITS`) | Credits on the main menu | menus_credits menus_credits2 | |
| High scores (`LEVEL_HIGH_SCORES`, the Hall of Fame) | High Scores on the main menu | menus_hof menus_hof2 | |
| High-score entry (`LEVEL_HIGH_SCORE_ENTRY`) | a game over with a score in the table | end_hof_fade end_hof end_hof_key | |
| Options (`LEVEL_OPTIONS`): video, shooter mode, sound, music | Options on the main menu | menus_options menus_options_hover menus_options_auto menus_options_manual menus_options_music_off menus_options_back | The game file has no difficulty item (`README.md`, "Fix candidates"). |
| Change User (`LEVEL_CHANGE_USER`) | Change User on the main menu | menus_users menus_users_abc players_players_list players_players_list2 players_players_ab players_players_ab_de players_players_cancelled players_players_deleted players_players_renamed players_players_pick1 players_players_pick2 players_players_pick3 players_players_back | The first row's name is masked (SYSTEM against GUEST). |
| Name requester (`REQUESTER_NAME`), a new player | an empty slot on Change User | menus_newname menus_newname_abc players_players_name players_players_name2 players_players_name3 | |
| Name requester, rename | Rename on Change User | players_players_rename | |
| Delete-player confirmation (`REQUESTER_DELETE_PLAYER`) | Delete with two or more players | players_players_delete | |
| Message requester (`REQUESTER_MESSAGE`): "you must always have at least one player" | Delete with one player | players_players_message | The only message the game file can show. |
| World picker (`LEVEL_SELECT_LEVEL`) | Start Game after the first board is won, or from the resume question's Cancel | end_worlds end_worlds2 end_worlds_hover board_worlds_saved board_worlds_saved_hover | The buttons of worlds not reached show a question mark. |
| Level slider (`LEVEL_SELECT_LEVEL2`) | a world's button | board_slider board_slider_left mission_m_slider0 mission_m_slider | The last world's slider runs 500 to 577. |
| Resume question (`REQUESTER_RESUME`) | Start Game with a game in progress | board_resume_q board_resume_q2 | |
| Saved game resumed (`LEVEL_SAVED_GAME`) | the question's OK | board_resume_yes board_resume_yes2 | |
| Board: intro, play, beams, fill | Start Game, Play, a click | menus_board_0500 menus_board_1500 menus_board_3000 menus_board_ship menus_board_beam menus_board_fill menus_board_turned menus_board_again menus_board_again2 board_slider_board | The enemies' and the power-ups' pictures are the board's own sprites, compared through the random plays of `test/plays` (phase one, state by state). |
| Pause (`LEVEL_PAUSED`) | Enter, or Esc, on a board | menus_enter menus_enter2 menus_paused menus_paused2 board_late_paused | |
| Board end: "level finished", the bonus and the next board's intro | a board's fill reaches its quota | board_end_fill board_end_bonus board_end_next | |
| Game over | the last life lost | end_over_death end_over_fade end_gameover end_gameover2 | |
| Mission complete (`LEVEL_MISSION_COMPLETE`) | the last board (577) won | mission_mission_fade mission_mission mission_mission_after mission_mission_after2 | Explained: the picture's only differences are a few stars' twinkles and the pointer's animation frame, which follow the game's random draws and the time since the level began. The original is patched to win at its first line (`mission.orig.txt`), the port by `win`, so neither is in the other's state. |
| Leave-game confirmation (`REQUESTER_QUIT`, 0x416d70) | none | | unreachable: its menu item type `confirm_quit` is read by the game but no menu of the game file uses it; Esc leaves a paused board without asking. |
| Trial screens (`LEVEL_OPENING_NAG`, `_CLOSING_NAG`, `_EXPIRING_NAG`) | none | | none: the registration screens are removed from the port; the game file's copies are read, never gone to. |
| Floating score texts, power-up popups, the beam's sparks | on a board | | none: parts of the board's picture (see the Board row). |
