# Rocket Mania Deluxe: mod plan

The port is first the original game, unmodified and working like its exe, with nothing added for later mods (CLAUDE.md, "Port
first, unmodified"). The mod phase waits until every PopCap game works in the browser, and no mod is made without the user's
approval. This file maps the 29 standard mods of `docs/MOD_ROADMAP.md` ("Standard mods for every game") onto this game, with a
reason for each one that does not fit, so the phase can start from it. Mods would use the shared mod system
(`docs/MOD_MENU.md`) and be configurable, never an overwrite.

The game, as the Board's code shows it: a field of 6 columns by 9 rows of pipe tiles that are turned (a click, or a drag to the
next column in arcade mode) until a pipe runs from a fuse on the left to a rocket launcher on the right, which then burns and
launches. Arcade mode drops tiles from above until a column is full, classic mode runs against the night (the dawn is the
timer) and levels need a number of rockets, strategy mode has bombs and, by the menu's hint, a limited number of matches. Each mode has
three difficulties. Randomness is the Board's own twister plus the framework's global one, and the tick count, so a seed and
the clicks of each update are the whole input.

Fits as it is: 1 LiveSplit (classic mode could split at each level; to be checked against the Board's level code), 2 widescreen
(the sky, city and ground pictures are drawn behind a fixed field, so the extra width can show more of them), 3 high-res
textures, 4 replays, 5 seeded RNG, 7 replay checker and 9 daily board (all three follow from 4 and 5, the update loop being
deterministic once the seed is fixed), 8 timer and input display, 13 game stats (rockets launched per round, the longest
chain, the largest launch the Board already tracks), 14 speed control (the burn, the fall and the night each run on update
counts), 16 reduced motion (the earthquake shake of a big launch, the flashes and the fireworks), 17 other controls
(keyboard-only turning and moving of a tile, touch for the phone browser), 18 larger text and high contrast, 19 rule settings
(night length, fall interval, rockets per level, special-piece weights, all of which are tables or constants in the generator),
20 a relaxed variant (no night timer, no full-column loss), 24 translations, 27 pixel-perfect scaling, 28 smooth frames, 29
jukebox (the game loads four music files, assembled from segments for the menu and the game).

Fits with a note: 6 bot scoring (the bot must find rotations that connect a pipe, which is a search over a 6 by 9 field, and in
arcade mode it plays in real time), 10 undo (a turned tile can be turned back and a cancelled burn can be lit again, but a launch,
a drop and the bombs cannot be undone, so it covers the turns between launches only), 11 hints or a solver
(the same search as the bot), 12 level select (classic mode has levels; arcade and strategy have no levels to choose),
15 colour-blind help (a tile's state is shown by its colour variant, and the four rockets differ only by hue), 21 custom
levels (levels are numbers fed to the generator, not drawn layouts, so the format would be a table of per-level settings),
23 developer leftovers (the exe has debug strings and a flag in the app object that gates something at the Board's start; to
be searched once the port is complete), 25 local achievements (to be checked against the Steam version of the game).

Skipped: 22 randomizer (the levels are not distinct designs to shuffle, and 19 already randomises their settings), 26 two
players (one shared field and real-time play leave no turns to take, and a split field would be a different game).

Ideas beyond the 29 are left for the mod phase itself: replacement pictures for the menu's background and the tile art are
covered by 3, a separate high score list for each mod set comes with the shared mod system.

Recorded at the user's request (2026-10-26), for the mod phase and not started: a mod that takes out what players have called
racist in the game's portrayal of Chinese people and culture. The user is not Chinese and does not know what exactly draws
the complaints, and this port has not been audited for it either, so the first step is a real look at what the game shows,
ideally with the help of Chinese players, before anything is changed. What to go through, by what the port already knows the
game contains: the art (the skyline, the roofs and lanterns, the talking head and its speech bubble, the menu picture), the
lettering (the menu and dialog fonts are brush-style display faces), the words the head and the dialogs say, and the music.
As the mod policy says it would be data: replacement pictures and fonts (idea 3), another set of the head's lines, and a
switch that turns each on, off by default as an example mod or offered as a feature mod; a replacement made from the game's own
art or text stays on the user's machine and is never committed, so it would ship as instructions or as art made from scratch.
