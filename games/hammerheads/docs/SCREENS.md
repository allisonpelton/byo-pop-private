# Hammer Heads: every screen, listed

One row for each screen or dialog the game can show (from `games/hammerheads/src` and the export's dialog ids), checked by
`tools/check.sh screens` against the newest shotdiff run, `build/shotreview/hammerheads/screens` (the original in Windows Sandbox
against the port, 800 by 600, the port with `3d off` as the original's software 2D). The shots come from the scenarios in
`tools/idplay/scenarios/` (`scr_*` for this pass; `classic1`, `classic_nd10`, `classic_nd22`, `classic_sp25`, `tough1`,
`marathon1`, `classic_save1`, `classic_save22`, `menus` and `howto` from the earlier passes, their shots named `classic1_uN`,
`nd10_uN` and so on by the update they were taken at). The causes a Note names by letter are in `IDENTICAL_PLAY.md`, "The screens
pass":

- **A** the removed "More Games" button (the row below the menu's logo closes the gap) and the removed word "Deluxe" are masked
  in `shotdiff.json`, each with its reason;
- **B** the edges of a picture the game stretches or turns (a gnome's soft glow copy, a medal's rays, the zoom wipe's hammer,
  the lettering of a scaled caption) differ by a rim of pixels, the framework's blit against the original's;
- **C** an animation driven by a counter (the light behind a gnome, the question mark, the shop sign, a floater, the Marathon
  clock) is seen at another age than in the original, the sandbox timeline of a long scenario sitting at another update than
  the port's;
- **D** the how-to-play movie runs on the original's wall clock;
- **E** particles and sprites in motion, which differ along their edges.

| Screen | Reached by | Shots | Note |
| --- | --- | --- | --- |
| Loading screen with "Click to Play" (`LoadingScreen`) | launch | classic1_u1000 | Deluxe masked (A) |
| Name box, first launch (`CreateUserDialog`: "New user") | launch with no player | name_empty, name_typed, nd10_u1500 | |
| Name box of another player ("(Create a New User)" row) | the player list | name_other | |
| Player list (`UserDialog`), one and two players | "If this is not you, click here" | players_one, players_two, players_after_rename, players_after_no, players_after_delete | |
| Rename box | Rename on the list | rename_box, rename_box_after_error | |
| Delete user, "Are You Sure?" | Delete on the list | delete_sure | |
| Error box: the name is needed, the name is taken, the new name is taken (`DIALOG_ERROR`) | OK on an empty box, on a taken name, on a taken new name | error_name_needed, error_name_taken, error_rename_taken | A |
| Main menu, welcome bubble | after the name box or the list | menu_welcome, menu_open, menu_two_players, menu_after_delete, menu_after_no, menu_after_leave, menu_after_trophies | A; Marathon shows locked or open as the player's restart points say (`HasFewRestartPoints`) |
| Main menu after a Marathon game | Main Menu on the game over box | menu_after_marathon | explained: C (the original's menu shows the Marathon plate's chains and the cursor's hover at another moment); A |
| Quit question | Quit on the menu | quit_box | |
| How to play (Flash movie) | Classic Bash with the help screen on | howto_u1200, howto_u3000, howto_u6000, nd10_u2500 | explained: D, the movie is compared by its frames in `IDENTICAL_PLAY.md` |
| Options box and its Credits box | Options on the menu | menus_u2250, menus_u2750, menus_u3250 | explained: the Full Screen tick is the port run's setting against the sandbox's, and the highlight of the button under the cursor is a scenario effect |
| High Scores | High Scores on the menu | menus_u4250, menus_u4500 | |
| Trophy Room from the menu, closed shelf | Trophy Room on the menu | menus_u5250, menus_u5500 | |
| Level introduction, Classic levels 2 to 25, with the gnome, shop, boss and king pages (the Shiny Shop at level 6, Irony at 13, Snappy at 15, King Globus at 25) | the restart point of the level, or the play | intro_level02, intro_level03, intro_level04, intro_level05, intro_level07, intro_level08, intro_level09, intro_level10, intro_level11, intro_level12, intro_level14, intro_level16, intro_level17, intro_level18, intro_level20, intro_level22, intro_level24, intro_level06, intro_level13, intro_level15, intro_level19, intro_level21, intro_level23, intro_level25, intro_restart, classic_intro, nd10_u3000, nd22_u3000, sp25_u3000, tough1_u3000 | explained: C for the turning light behind the gnome and the sign of the shop page (level 6, 15 and 25), found by matching the port's shot at the age of the original's (the shop page matches at the port's age 1240 for the original's shot taken at 1495) |
| Ready? box and its Go | Go on the introduction | ready_level11, tough_ready, tough1_u3000 | |
| Board, Classic, Tough Cookie and Marathon, hearts bar and the coin hint (the "Don't miss!" hint over the board at classic1_u3750) | the play | ready_level01, ready_level03, ready_level04, ready_level05, ready_level02, board_level01, board_level11, board_level25, classic_board, board_restart, board_continued, board_level02, board_level05, board_level13, board_level20, tough_board, marathon_board, nd10_u4000, nd22_u5500, tough1_u3500, marathon1_u4000, classic1_u3250, classic1_u3500, classic1_u3750 | explained: B for the glow around a gnome and a gnome's rim, C for the glow's level and the gnomes' pose, E for the particles |
| Game menu | the Menu button on the board | game_menu | |
| Leave game question | Main Menu on the game menu | leave_game | |
| Wave cleared box (Classic) | the end of a wave | nd10_u10500, nd22_u13500, classic1_u6000, classic1_u8750 | |
| Wave cleared box (Marathon) | the end of a wave | marathon_wave_cleared, marathon1_u6000 | explained: C, the original's Marathon clock stands at 0:20 where the port's has run to 0:34 and the box is up, so the first pair shows different moments of one game |
| Level completed box | the end of a level | nd10_u18500, save22_u22000, tough1_u9500 | explained: C (the figures count up at another age) |
| Marathon tutorial boxes (Marathon, Pick up, Don't miss) | Marathon Bash | marathon_box, marathon1_u3000 | |
| Shop (the Shiny Shop) | the shop sign on a board | marathon1_u21500, wipe_to_shop_16 | explained: C and B for the shop's scaled sign |
| Challenge screen (Gold Bonus), the rounds and its finished box | the bonus gold gnome | nd10_u24500, nd10_u25000, nd10_u25500, nd10_u26000, nd22_u23000, nd22_u24500 | |
| Trophy Room in a game, with Back to Game | a trophy won, or the trophy button | ready_level06, trophyroom_level11, nd10_u17000, marathon1_u17500 | explained: B for the Back button's lettering |
| Trophy Room from the menu with eleven trophies won | Trophy Room on the menu | trophy_room | explained: B, the lettering of "Back to Main Menu" and the trophies' edges |
| Trophy award page ("Click to Continue") and its close-up | a trophy won | award_level06, award_level11, nd10_u16500, nd22_u3500, sp25_u3500, marathon1_u16000 | explained: B, the medal and its rays are scaled and turned |
| Trophy close-ups of each trophy won | a trophy on the shelf | trophy_page_place00, trophy_page_place02, trophy_page_place03, trophy_page_place05, trophy_page_place07, trophy_page_place08, trophy_page_place10, trophy_page_place11, trophy_page_place13, trophy_page_place18, trophy_page_place19 | explained: B, the medal and its rays are scaled and turned; the other ten of the twenty-one are not won by the player used |
| Camera floater ("Snapshot Saved!") | the camera trophy of a level | nd10_u19000 | explained: B and C |
| Tough Cookie notice ("A real challenge") | Classic won | tough_notice, sp25_u31500, sp25_u33000 | A |
| Restart points (resurrect screen), 2 buttons | Classic Bash with a restart point | classic_points, tough_points | |
| Restart points, 5 and 6 buttons | Classic Bash with four or five restart points | points_5, points_6, resurrect_classic | |
| Restart points, 3 and 4 buttons | Classic Bash | | none: laid out by the same code as 5 and 6 (`ResurrectScreenWidget::Resize`, three to a row), the two layouts of the sandbox shots are 2, 5 and 6 |
| Restart points of Marathon Bash | Marathon Bash | | unreachable: Marathon has no restart points (its button starts the game at once) | |
| Continue box ("Continue?": a saved game) | a mode's button with a saved game | continue_box, save22_u10500 | A |
| Game over box, Classic | the last heart lost | over_level02, over_level03, over_level05, over_level10, over_level12, over_level14, over_level22, over_level25, classic_over, nd10_u19500 | explained: C for the turning light and the pulsing question mark over the next level's gnome (the original's shows it at another age of the box), B for the rays' edges |
| Game over box, Tough Cookie | the last heart lost | tough_over | |
| Game over box, Marathon | the last heart lost | marathon_over | explained: C, the original is still playing at 0:47 where the port's game is over at 0:55 |
| Game won page (the king down) | level 25 finished | sp25_u30000, sp25_u30500 | explained: E, the confetti |
| Zoom wipe, Hammer: to the Trophy Room and to the High Scores | the menu's buttons | wipe_to_trophies_01, wipe_to_trophies_02, wipe_to_trophies_06, wipe_to_trophies_09, wipe_to_trophies_12, wipe_to_scores_01, wipe_to_scores_06, wipe_to_scores_12 | explained: B, the hammer is scaled |
| Zoom wipe, Hammer: from the Trophy Room and from the High Scores | Back to Main Menu | wipe_from_trophies_05, wipe_from_trophies_12, wipe_from_scores_05, wipe_from_scores_06, wipe_from_scores_12 | explained: B; the High Scores page now stays under the first frames as the original's does (fixed in this pass) |
| Zoom wipe, Hammer: the loading screen to the menu, a game to the menu, the menu to a game | Click to Play, Leave game, a mode's button | wipe_loading_05, wipe_loading_12, wipe_leave_game_05, wipe_leave_game_12, wipe_to_game_05, wipe_to_game_12 | explained: B |
| Zoom wipe, Level Up (the next level) | a level finished | wipe_next_level_01, wipe_next_level_05, wipe_next_level_08, wipe_next_level_12, wipe_next_level_17 | explained: B, and C for the photo floater that comes in over the first frames |
| Zoom wipe, Shop (to and from) | the shop sign | wipe_to_shop_05, wipe_to_shop_16, wipe_from_shop_05, wipe_from_shop_15 | explained: B |
| Quit with a game running | Quit | | none: Quit has no second question (the saved game is written and the program ends) | |
| Options notices ("Not Supported", "Warning", "No Windowed Mode") | the Hardware Acceleration and Full Screen ticks | | none: they depend on the machine's display (the card's support, the windowed modes), not on the game's state | |
| Error box, crash report | the program's own crash | | none: the original's crash is its own (a null read when the shop's gnome is deleted before a strike, `GAME_NOTES.md`); the port tests it | |
