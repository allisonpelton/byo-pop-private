# Dynomite Deluxe: mod ideas

The port is first the original game, unmodified and working like its exe; it adds nothing for later mods, not even
hooks (the user's rule of 2026-10-26). Mods wait for the game's mod phase, which comes only after every PopCap game
works in the browser, and each one needs the user's approval. In that phase the session goes through the 29 standard
mods of `docs/MOD_ROADMAP.md` ("Standard mods for every game") and makes each that fits; this file maps them onto the
game and gives the reason for the one that does not fit, and then lists the ideas that are particular to this game. None
is made. A mod is a folder with a `mod.json` (`docs/MOD_MENU.md`), with settings read from its `"features"`.

Two things make Dynomite easy to mod. Stomped Puzzle is played in turns: the eggs come down only after a number of
shots, so undo, a solver and a bot are natural there (Endless and the fossil challenge's panic kind push the eggs down
on a clock, and Time Trial has its own). And its puzzle packs are data: the game already lists every `STOMPED\*.EGG` beside its own `DEFAULT.EGG`, and the format is
known (`GAME_NOTES.md` section 4: a pack's name, the rounds each skill skips and its picture, then per round the width,
the launcher's place, the shots per drop, the baby eggs' chance, its name, the first eggs' colours and the grid).

## The 29 standard mods, mapped

| # | Standard mod | For this game |
|---|---|---|
| 1 | LiveSplit | Fits: a split at each round of a Stomped pack, each fossil fragment and era, and the end of a Time Trial, from the shared tagged memory block. |
| 2 | Widescreen | Fits: the board is 490 pixels wide beside a 150-pixel side bar; the extra space could hold the next eggs, the chain and the game's stats, or a wider jungle backdrop. |
| 3 | High-res textures | Fits: `@2x` images made locally from the player's files; the eggs are a sheet with masks multiplied in at load, which the upscaler must keep. |
| 4 | Replays | Fits: the seed plus the per-update input, through the same update loop. The game draws from three generators (the board's lagged Fibonacci one, the framework's Mersenne Twister and the C library's `rand` for a baby egg's shake), all of which must be seeded from the replay. |
| 5 | Seeded RNG | Fits: the seed shown at game over and settable from the options, the command line or the URL. |
| 6 | Bot scoring | Fits: a bot that tries every aim, runs the flying egg as the aim path already does to find the landing cell, and scores the match; for tuning curated mods. |
| 7 | Replay checker | Fits once 4 is: a saved replay replayed without a window to confirm its score. |
| 8 | Timer and input display | Fits: a timer for a whole pack or fossil challenge and per round, and the aim's angle and the shots, for speedruns in the browser. |
| 9 | Daily board | Fits: Endless and Time Trial with one seed for everyone on a date and their own high scores; a daily Stomped round picked from the packs. |
| 10 | Undo | Fits, in Stomped Puzzle and the fossil challenge's kinds without a clock: the board before each shot, kept in memory as the saved game writes it (the boards' and their parts' save and load already hold the whole state). |
| 11 | Hints or a solver | Fits: the best shot by the bot of 6, shown as the aim path's landing cell; the game already draws that cell. |
| 12 | Level select | Fits: start any round of a pack (the pack already starts each skill at its own round) or any fossil era, with separate scores. |
| 13 | Game stats | Fits: the game already counts the longest chain and the babies hatched; with shots, eggs broken, sticks of dynamite and power-ups, at game over and as lifetime totals. |
| 14 | Speed control | Fits: a factor on the update clock for Endless's whirleys, the Time Trial's clock and the ceiling; a slower game for practice. |
| 15 | Colour-blind help | Fits: the eggs differ only by colour; a symbol in each colour's spots, or another palette for the egg sheet. |
| 16 | Reduced motion | Fits: no thunderbolt flash (the board turns white while one strikes), no shaking baby eggs, and fewer confetti, sparks and shell pieces when eggs break. |
| 17 | Other controls | Fits: the game aims only with the mouse; keys to turn the aim and fire, a gamepad's stick, and touch (drag to aim, release to fire) for phone browsers. |
| 18 | Larger text and high contrast | Fits: the fonts are bitmap fonts, so larger text needs a second image set or a vector font; the side bar's tiny font, Inchy's bubble and the saved games' names matter most. |
| 19 | Rule settings | Fits: the shots per drop, the baby eggs' chance and life, the number of colours, the chain bonuses, the power-ups' times, the skill's score factor and the ceiling's speed as settings. |
| 20 | Relaxed variant | Fits: no ceiling coming down (or no game over) in Endless and Stomped, and Time Trial without its clock. |
| 21 | Custom levels | Fits best of all: a mod carries `.EGG` packs, which the pack list already reads; a documented format and a pack editor (below) make them. |
| 22 | Randomizer | Fits: a pack's rounds in a shuffled order, or rounds generated from the grid's own random rows (the game adds two each time it pushes the eggs down). |
| 23 | Developer leftovers | Fits if there are any: none was met while porting; a search of the key handlers and the exe's strings would list cheats or debug keys for an example mod. |
| 24 | Translations | Fits: most texts are string properties by id, and the texts read from the exe can already be replaced by a `GT_<address>` property; translations made locally, never committed. |
| 25 | Local achievements | Does not fit as such: the game has no Steam achievements (its Steam copy runs inside PopCap's own wrapper, with no Steamworks). The museum's fossils are its own collection and are already saved. |
| 26 | Two players | Fits: two players taking shots in turn on one board, each with a score and a chain of their own, or both playing the same Stomped round and comparing. |
| 27 | Pixel-perfect scaling and scanlines | Fits: a 640x480 game. |
| 28 | Smooth frames on high-refresh screens | Fits: frames between updates with the flying eggs, the falling eggs and the effects interpolated, without changing the update clock, so replays stay exact. |
| 29 | Jukebox | Fits, though small: the game has two music modules; the player chooses which plays where, or adds modules of their own. |

## Ideas particular to this game

| Idea | What it does | How |
|---|---|---|
| Pack editor | Makes and edits Stomped packs: rounds, widths, the launcher's place, shots per drop, the first eggs and the grid | A tool (native or a web page) that writes the `.EGG` format of `GAME_NOTES.md`; the base of 21 |
| Smaller saved games | A Stomped saved game keeps the pack's name and round instead of a copy of the whole pack (about 267 KB each now) | Matters in the browser, where eight slots take about 2.9 MB of the origin's storage; the original format stays readable |
| Longer aim path | The aim path shows every bounce up to the landing cell, or a shorter path for a harder game | A setting on how far the launcher's path (0x402d90) runs the flying egg |
| Museum viewer | Browse the fossils found, with their eras, outside a game | The museum of the fossil mode, opened from the main menu |
| Endless with fossils | Endless's whirleys with fragments buried in the eggs, so a long game also fills the museum | Combines the Panic and Fossil modes' parts; a curated mod with its own high scores |
