# Water Bugs: mods for its mod phase

The port is the original game, unmodified, and adds nothing for later mods, not even hooks (CLAUDE.md, "Port first,
unmodified"). Nothing below is made. In the game's mod phase, which waits until every PopCap game works in the browser,
its session goes through the 29 standard mods of `docs/MOD_ROADMAP.md` ("Standard mods for every game"), under the mod
rules in CLAUDE.md: each is a feature mod, or an example mod for a single setting, and the game plays unchanged with no mod
on. This file maps the 29 onto Water Bugs, numbered as the roadmap numbers them, and says why a mod is skipped where it
is. Everything here would be a mod of the port's own code and data, never another game's files or modes.

What the game is, for the list: a real-time game on a mouse. A ship moves with the pointer and a click fires a pair of
beams that close a stretch of the board; a right click or the space bar switches the beams' aim. Bugs roam the board and
cut a beam they touch; a fill that holds no bug stays, and a board is won at its quota (85% in the first ones, from the
level file's `fillQuota`). The game keeps the player's lives, a score, two records per board (the fewest shots and the best
fill, each with a name) and a Hall of Fame, and has a world map with the levels reached. It runs on a fixed 10 ms update
clock with its own random numbers, so a game is its seed and its input. The screen is 800x600 and the art is drawn by
software blitters into a 16-bit back buffer.

## The user's first six

1. **LiveSplit.** Fits. A board's end (the level finished state, which the results requester follows) is the natural
   split, and a world's last board the longer one.
2. **Widescreen.** Fits poorly. The board is the whole 800x600 screen and the levels are fixed grids (50 by 37 cells), so a
   wider screen can only hold panels beside it (the score, the lives, the board's records, the Hall of Fame) and a wider
   background. Worth making only if the session finds a use for the space.
3. **High-res textures.** Fits. The art is two banks of images read from the player's files; a pack upscaled locally from
   them would be drawn by a renderer that scales, since the game's own blitters are 16 bits at 800x600.
4. **Replays.** Fits. The game's whole state follows from a seed and the pointer and button of each update, and the
   headless runner (`test/gamerun.cpp`) already plays scripted input through the same update loop.
5. **Seeded RNG.** Fits. Enemy places, power-up kinds and random boards (`Level_MakeRandom` takes a seed) all draw on the
   game's random numbers; the seed would be shown on the results requester and set from the options or the URL.
6. **Bot scoring.** Fits. A bot would choose where to fire from the open cells and the bugs' positions, run in the
   headless runner and score on boards finished, shots and lives, for tuning curated mods.

## Speedrunning and competition

7. **Replay checker.** Fits once replays (4) exist: the runner replays a file without a window and reports the score.
8. **Timer and input display.** Fits. The game shows no clock on a board (its time limit is the trial's, unused in the
   registered game), so the timer and the pointer and button display would be new overlays, for browser speedruns.
9. **Daily board.** Fits. The random boards come from a seed (`Level_StartRandom`, `Level_EnemyCounts` giving each its
   bugs), so a date's seed would give everyone the same board, with its own records beside the game's.

## Practice and help

10. **Undo.** Skipped: the game is real time, with no turns to take back.
11. **Hints or a solver.** Fits as a hint overlay: while a beam pair is on its way, tint the part of the board that would
    fill if it closed now, so a player can see which side the bugs are on before committing; a solver would suggest a
    line. The first three boards already carry the game's own hints (`Level_Tutorial`); a setting could bring such
    hints back on any board.
12. **Level select.** Fits. The game keeps two records per board (`gLevelRecords`) and has a world map for the levels
    reached; the mod would let a player start any board alone, with no lives to lose, and keep its scores apart.
13. **Game stats.** Fits. The Hall of Fame shows high scores only; a stats page could show boards finished, shots, bugs
    caught, power-ups taken and lives lost, at the game's end and as lifetime totals.
14. **Speed control.** Fits. The update clock is the game's own 10 ms step, so a slower or faster clock changes the time
    between updates and nothing else, and replays stay exact.

## Accessibility

15. **Colour-blind help.** Fits. The game tells good power-ups from bad ones by blue and red, and the bugs of a level by
    their tints; an outline or a small mark on the bad power-ups, and symbols for the tints, would help.
16. **Reduced motion.** Fits. The bubbles sway as they rise, and fills, shots and arrivals throw sparkles and smoke; a
    switch would hold the bubbles still and thin the bursts.
17. **Other controls.** Fits. Keyboard play (arrow keys to move the ship, a key to fire, one to switch the aim; the game
    already reads keys each frame), a gamepad, touch for phone browsers (a tap fires; the aim switch needs a button on
    screen) and remappable keys.
18. **Larger text and high contrast.** Fits. The menus, the hints and the Hall of Fame use small bitmap fonts; a scaled
    font for the items, the hints and the requesters would help on large screens and for low vision, and a plainer
    panel for contrast.

## Rules and content

19. **Rule settings.** Fits. Lives (the game starts with six), the fill quota, the number and speed of the bugs, the
    beams' speed, how often power-ups come and how bad ones are, beside the game's own three difficulties (`gDifficulty`).
    A mod that changes the quota must also build the first levels' hint from it, since that hint says 85% in the game's
    own words.
20. **A relaxed variant.** Fits. There is no timer on a board, so the variant is no game over: lives that are not lost,
    or a board restarted at no cost; below the easy difficulty, bugs that are slower and beams that are faster at the
    start, for younger players.
21. **Custom levels.** Fits well. A board is an XML file (`wanims/water_level*.xml`, read by `Level_Load`) with its cells
    drawn in text, its enemies and its quota; a documented format would let a mod add boards to the world map.
22. **Randomizer.** Fits. The game file's `random_levels` tag makes a number of random boards from a seed; a shuffled
    order of the shipped boards, or a run of only random boards, each a little faster and with one more bug, ending with
    the lives and with its own score table.
23. **Developer leftovers.** Fits as an example mod. The exe reads the keys 0 to 9 and A to D when a power-up is made and
    gives it that kind (`Powerup_Init`, ungated), and it has a demo mode in which the game plays itself (`gDemo`, which its own code never sets); the
    port took the demo mode, the sweepers power-up and the water mine and the freeze out on 2026-11-11 (`GAME_NOTES.md`,
    "Unneeded code taken out"), so a mod would put them back from the export, with what else the session finds unused in
    the exe and the banks.
24. **Translations.** Fits. The game's texts are read from the exe by address (`GT(0x...)`) and from `local.xml`, so a
    table by address or id could replace them; such tables are made locally and never committed.
25. **Local achievements.** Skipped for now: the exe has no achievement texts that the search found, so there are no names
    to show; the session looks again when it starts the mods.
26. **Two players.** Fits as turns only: players alternate on the same board, passing the mouse when one loses a life
    (the game's rules have one ship, so competing at once does not fit).

## Presentation

27. **Pixel-perfect scaling and a scanline filter.** Fits as an option. The screen is 800x600, not 640x480, so whole-number
    scaling reaches 2x only on large screens; the filter works at any size.
28. **Smooth frames on high-refresh screens.** Fits poorly. The engine redraws only the dirty rectangles of a 16-bit back
    buffer, so a frame between updates would need a full-frame path with the objects at interpolated positions first.
29. **Jukebox.** Fits. The music is five MO3 modules (`audio\cosmo*.mo3`), chosen by the level's number
    (`Level_SetBackground`, `Level_StartSong`); the mod would let the player choose the track or play others on other
    boards.
