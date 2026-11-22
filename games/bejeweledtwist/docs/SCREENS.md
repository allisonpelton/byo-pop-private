# Every screen, listed (Bejeweled Twist)

Every screen and dialog the game can show, from the export and the source, with the shots that compare it against the
original (docs/SHOTDIFF.md, "Every screen, listed"). The shots come from the screen coverage routes in
`tools/screens/` (gen.py turns one spec into the original's origrun script and the port's `-script`), compared in one
shotdiff run, `screens`. They are 2D and windowed at the 600 art: the original in Windows Sandbox on origrun's virtual
clock, the port under rundesk with `-fast`. 3D and full screen were compared on the visible desktop in phase two's sets
V3 and V4 and in the challenge pass (IDENTICAL_PLAY.md); full screen draws the same 800 by 600 picture scaled.

Why pairs differ, named in the Notes below:
- **Sparkles**: the glints on the logo and the Play button, the sparkles of a mode button or the level's end, the smoke
  of the counter gems, debris, a level's flying gems and fire, the replay's stars: each is placed by the global Rand,
  which on the menu runs two draws apart from the original's (NOTES.md, Oddities, "The menu's global Rand is two draws
  apart at its start") and in a game depends on the heap ("The credits' sparks start their turn from the heap"). Their
  shapes, colours, numbers and timing agree; only where each one is differs.
- **Autoplay slip**: the original's autoplay sometimes moves 15 updates early or late (NOTES.md, "Oddities"); the board,
  scores and timers agree, and the autoplay's selector and the effects of its moves sit elsewhere for a while.
- **Landed early**: origrun takes a shot at the first update at or past its time, and the original's counter can jump
  a few updates while it loads (the run's landed.tsv); the port's shot of the same moment then matches a few updates
  earlier.

A Note says "explained" with the reason for each pair of the run that differs; the pairs it does not name match or
differ only faintly.

| Screen | Reached by | Shots | Note |
| --- | --- | --- | --- |
| LoaderScreen, the loading screen | launch | | none: its device slides in on the loading thread's progress, so where it stands at an update varies from run to run on both sides; compared by eye in phase two's set 1 |
| Logo, the publisher's logo | launch | | none: shown before the loader, on the same loading thread; compared in phase two's set 1 |
| NewUserPanel at first launch: the name box | a fresh player folder | m01_newplayer | explained: sparkles |
| Menu, the main menu | the name typed, RETURN | m02_menu, m06_menu_back, m13_after_credits, y07_menu | explained: sparkles (all four). The Play button's spiral now catches up with the loader's updates and turns at half speed while the mode select has the mouse over Zen (this pass) |
| HighScoresPanel, Classic | menu: High Scores | m03_scores_classic | explained: sparkles |
| HighScoresPanel, Blitz | High Scores: Blitz Scores | m04_scores_blitz | |
| Statistics (the menu's stats panel) | High Scores: Personal Stats | m05_stats | |
| OptionsPanel | menu: Options | m07_options | explained: sparkles |
| The options' graphics dialogs (3D refused, full screen refused) | Options: 3D or full screen on a card that cannot | | rare: they show only when the graphics card refuses 3D or the display refuses the mode, which neither the sandbox nor this machine's card does |
| HelpPanel, sixteen pages | menu: How to Play, then Next | m08_help01, m08_help02, m08_help03, m08_help04, m08_help05, m08_help06, m08_help07, m08_help08, m08_help09, m08_help10, m08_help11, m08_help12, m08_help13, m08_help14, m08_help15, m08_help16 | explained: sparkles (the logo's glints; m08_help02 to 04 and 06 to 16) |
| TwistDialog, the quit question | menu: Quit Game | m09_quit | explained: sparkles |
| Mandala, the mode select | menu: Play | m10_modeselect, c01_modeselect | explained: sparkles (both) |
| The mode select's locked mode text | the mouse over the locked Challenge | m11_locked | explained: sparkles |
| Credits | the mode select's hidden sequence: Classic, Zen, Blitz, Challenge three times | m12_credits_a, m12_credits_b, m12_credits_c, m12_credits_d | explained: sparkles (m12_credits_a) |
| ProfilePanel, the player list | menu: Change User | y01_players, y04_two_players, y06_after_delete | explained: sparkles (all three) |
| NewUserPanel from the list | Change User: New User | y02_newuser | explained: sparkles |
| The same name's question | New User: a name already taken | y03_samename | |
| The delete question | Change User: Delete | y05_delete | |
| Board, Classic: its start and the tutorial's tips | Play: Classic (autoplay) | c02_board_start, c03_tutorial | |
| Board, Classic in play, level ends | autoplay | c04_level1, c05_level_done, c10_level2, c11_level2_done, c12_level3, c13_level3_b | explained: sparkles (all six); autoplay slip (c10 to c13) |
| LevelUpShow and RankBonusWidget, the level's end and the rank bar | autoplay to a level's end | c06_levelup, c07_levelup_b, r01_rankbar, r02_rankbar_full | explained: sparkles (all four) |
| Hyperspace, the jump between levels | autoplay to a level's end | c08_jump, c09_jump_b | explained: sparkles (the flying gems and their fire) |
| Board, Blitz | Play: Blitz (autoplay) | b01_board, b02_board_b, b03_late, b04_last_seconds | explained: sparkles (all four); autoplay slip (b03, b04) |
| Blitz's end: the explosion and the flash | the timer runs out | b05_end, b06_end_b | explained: sparkles (b05_end, the debris) |
| EndStats | after a game's end | b07_endstats, b08_endstats_b, g10_endstats | the average multiplier of 5 / 4 showed x1.2 where the original's C runtime writes x1.3 (fixed this pass) |
| HighScoresPanel after a game | EndStats: Continue | b09_scores, g13_highscores | explained: landed early (g13: the original's shot came 14 updates early; the names' colours match the port's 20 updates earlier) |
| The continue question | Play: Classic with a saved game | g01_continue | |
| The zen-jump offer ("Too many bombs?") | Continue a first Classic game whose bomb count (board +0x21e0) is under 3 | g02_zenjump, z01_offer | explained: sparkles (both: the counters' smoke, the mode button's sparkles) |
| The last-chance tip | a bomb ticks to zero | g03_board, g04_lastchance | explained: sparkles (both: the counters' smoke; at g04 the bombs' glow too, best 6 updates later by under 100 pixels) |
| WheelSpin, the bomb's wheel | the last-chance tip's OK | g05_wheel, g06_wheel_ready | explained: the zero bomb's glow, within 50 of the original's at its edge. In 2D the hole under the standing wheel was not drawn (fixed this pass) |
| GameOverShow: the skulls, the explosion, Game Over | the wheel lost | g07_skulls, g08_explosion, g09_gameover | explained: sparkles (the explosion's debris and fire; at g07 the bomb's glow as on the wheel) |
| ReplayWidget, the end's replay | EndStats: View Replay | g11_replay, g12_replay_b | explained: sparkles (the replay's stars) |
| The zen jump and RETURNING TO GAME | the zen-jump offer: Zen | z02_jump, z03_zen_start, z04_zen_go, z05_returning | |
| Board, Zen | RETURNING TO GAME: OK | z06_zen_board | |
| AwardTelly at a rank-up | Classic by the autoplay from a player one rank point short of rank 2 (profile byte 246) | r03_award, r04_award_rank1, r05_rankup, r06_rankup_b | explained: sparkles (all four: the flying gems burning behind the award) |
| FruitAwardScreen, the fruit's trail and the fruit gem | a saved game whose multiplier's steps are 151 (save bytes 727 to 730), then one match | f01_multiplier, f02_trail, f03_trail_b, f04_fruit, f05_fruit_gem | explained: sparkles (all five: the flame gem's fire, the tube's bubbles, the twist ring's and the multiplier's sparkles); the fruit comes to the same gem |
| The mode-unlock banner | Classic's level 10 unlocks Blitz (LevelUpShow, UnlockMode(3)) | | unreachable: this copy is Steam's (partner.xml's MarketingMode LevelLock), whose mode select unlocks Blitz the first time it opens (Mandala), so level 10 finds Blitz open and no banner shows; a saved game raised to level 10 ended it so on the port, after the coal and bomb tips |
| ChallengeMenu, GoodTube, RankBadge, the challenges' boards, win, bar and restart | Play: Challenge | | none: compared against the original in the challenge pass of 2026-11-11 (IDENTICAL_PLAY.md, "Challenge mode against the original"), five whole challenges in 2D and the 3D load, dialogs, win and bar |
| The pause screen at a focus loss | the window loses the focus in play | | none: while the original is unfocused origrun sees its update counter stand still, so a route's timed shots cannot be lined up there; the mute, the hidden board and the pause text were compared in phase two's set 5 (IDENTICAL_PLAY.md) |
| The original's crash reporter (SexyAppFramework's SEH dialog) | a crash in the original | | unreachable: the framework does not port SEH crash reporting (framework/SexyAppBase.cpp's header); its texts are the only place "Deluxe" appears in the export's strings, so no screen of the port shows it |
| Unlock dialogs (Unlock, Maybe Later) | | | none: registration code, removed from the port with the trial (CLAUDE.md, "Registration and outbound buttons") |
