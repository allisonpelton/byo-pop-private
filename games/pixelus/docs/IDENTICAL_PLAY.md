# Pixelus Deluxe: identical-play pass, first phase (2026-11-03)

Phase one compares the game's state, update by update, between the original (`tools/origrun`, virtual clock, scripted
cursor, the seed patched in) and the port (`-fast`). It is **not finished**: a saved game carried from one program to the other
and the records table after a puzzle are not compared yet (see "Open"). The screens (phase two) have not been started
either.

## How it was compared

`games/pixelus/tools/identical/case.sh <name> <bot seed> <game seed> <puzzles> 2500 [-autoplayskip]` runs one case:

1. The port's bot plays N puzzles through the mouse (`-autoplayui`); its input is written as an origrun script
   (`-autoplayscript`) and its state, change by change with the update it came at, as a state log (`-statelog`,
   `src/testhooks/StateLog.h`): the screens that exist, moves, time, tiles' places, states and pictures, supply, cursor,
   the game's random-draw count and so on.
2. `toorig.py` turns the script into the original's (2500 updates later: the original's loading screen is longer;
   button clicks hover two updates early), `orig.sh` plays it in a copy of the original with up to four `--watch`
   values per run, and `cmp.py` compares each value's changes with the port's. A value is "same" when every change
   comes with the same value at the same update.
3. The global generator's draws were also compared per update: the original's Mersenne index (0x579d20) against the
   port's draw counter (`rand` in the state log).

Cases played: bot seed 1 / game seed 1 and bot seed 2 / game seed 7, three puzzles each, the tutorial skipped
(`-autoplayskip`), from a first run (user question, new player, help screen, story, island, temple, board, completed
screen). Both match in every value except the two listed under "Oddities". Then the tutorial itself, not skipped: bot
seed 1 / game seed 1 over all nine puzzles and bot seed 2 / game seed 7 over eight, each puzzle's lesson dialog answered
by the bot's click on its OK button (see the lesson's row below and "Open" for what does not match yet).

## Changes this pass made

| Difference | Cause in the original | Port now | Commit |
|---|---|---|---|
| Skipping the tutorial went back to the island | 0x4116xx (SkipTutorial OK) shows the story, not the island | `SkipTutorialOK` shows the story | 3910d0c |
| No help screen for a new player | `UserProfile` ctor (0x439e70) sets the "show help" flag | flag defaults to true | 3910d0c |
| Tile pictures differed from the first board on | the tile constructor (0x42eee0) draws `rand % 5` on the game's own generator (0x579360, seeded in `SexyApp::Init`) for every tile made, in a puzzle's file as in a throw; the port drew with `CrtRand` after the fact at 7 sites | `Grid::NewObject` draws on `PixelusApp::mRand` (the same MT, `SeedMTRand2004`); the 7 sites are gone | 3910d0c |
| The draws after the first screens drifted | other consumers of the two generators that the port lacked: the characters of the temple, story and tutorial (0x42e110, 0x42df40; blink, pose changes, spring), their speech bubbles (0x42c4a0, 0x42c590; draw on the application's own generator, default seed 4357), the grid's wave timer (0x425d40, 0x424b30), the board's sidebar gleam timer (0x407f10, 0x409bd0) | `Character` (new), `Grid::mWaveTime`, `Board::mPulseTime`; `PixelusApp::mAppRand` | 3910d0c |
| Temple click drew 1 + tiles fewer numbers | the click (0x41162d) loads the puzzle into a grid of its own (0x427430) to get its id | `TempleScreen::MouseDown` does the same load and throws the grid away | 3910d0c |
| Help screen drew 2 + 3 fewer numbers | its demo grid (0x40ddd0, 0x40d320) makes a grid and a tile and throws a tile at frames 50, 150 and 250 of 950 | `HelpScreen` draws the same numbers; the demo itself is not drawn (phase two) | 3910d0c |
| Initial cursor on the board | the original does not aim at the board's start | the first `Aim` is gone | 3910d0c |
| Dialog width of the user dialogs | 0x190 | `SizeDialog(..., 0x190)` | 3910d0c |
| The tile maker started at the bottom of the grid | 0x42bd40 starts him above it (side 2) at the first column | `TileMaster` starts at `SIDE_DOWN`, cursor 0 | 5e00f97 |
| Dotted aim line, target and base marks on a fresh board | 0x422ab0 draws those only in control modes 0 and 1; the board's mode (+0x19c) is 2 from its constructor (0x407f10) and nothing else writes it. In mode 2 it draws the cursor on the hovered cell (the red one when a click does nothing) and the arrows | `Board::DrawAim` does the same | 7ff6a91 |
| The player's file (`users/<n>`) was in a layout of the port's own | 0x4391c0, 0x438450, 0x439fb0: version 4, four-byte string lengths, the temples by number (0 tutorial, 1..9) with their three flags and their records (uuid, "maxed" flag, fewest moves, second count), the records of a temple ordered by UUID as a signed number | `UserProfile::Write/Read` produce and read that layout; the bytes match the original's for the same play | b29b14e |
| The tutorial had no lesson | the nine puzzles of the tutorial open a modal bubble (dialog 0x1f, ctor 0x40ef30, made by 0x40f160, answered by 2031) from the step functions 0x4061f0, 0x406310, 0x406470, 0x4066b0, 0x406b10, 0x406e60, 0x407150, 0x407510 and 0x402f80, called every fifth update by the dispatcher 0x407aa0; a click that is not on its OK does nothing while it is up (the board is behind a modal dialog); the board ignores a mouse move that comes before its first update (the grid is made there, 0x409880); the "sticky spots" bubble (0x409bd0) opens once for a profile on the first board that has a start tile | `Board::HintCheck` and the sticky-spots hint in `Board::Update`, `PixelusApp::ShowHint` (the bubble: 250/220/300 wide, 24 inset, text height plus 0x32, the OK button 24 in from both sides), the bubble's place for each puzzle (see "Open"), `MouseMove` before the first update does nothing | this commit |
| After the tutorial's last puzzle the port went straight to the story | the tablet's button (0x419c20) opens a screen of its own for a mode 1 to 5 (class vftable 0x50c8c8, made by 0x432b60 / 0x4326c0, Draw 0x40bd80, button 0x40b760); mode 5 is the tutorial's "you completed the tutorial" with a CONTINUE button, which leads on to the story | `CompletionScreen` (mode 5): picture, header, button at 0x40b710's place; the tablet's button opens it when the puzzle ends the tutorial, its button runs what the tablet's did | this commit |
| Temple and game completion screens missing; the ending went to the island | the tablet's mode (+0xf8: 1 temple, 2 game, 5 tutorial; +0xfc: 3 or 4 when a temple or the game is newly "maxed") is set at the end of 0x41b3c0 from the solved and maxed state of the temple and the game before and after the record, the world-record flag and whether the puzzle had a record; 0x419c20 opens the screen (mode 4 for sub 4 with no mode) or goes to a story (mode 1 opened temple, 2 maxed temple) or the temple; the screen's button 0x40b760 goes to story 3, 1 or 2, or the island; the ending story goes to the main menu (0x433d00) | `PuzzleCompletedScreen::mMode/mSub`, `PixelusApp::ContinueAfterPuzzle/ShowCompletion/ContinueAfterCompletion`, `StoryScreen` ending to the menu | this commit |

With these the global generator's draw count matches the original at every update of three puzzles (temples 198 and
463 draws, boards 159, 85, 79, the help screen's frames), and so do all tile pictures. `tools/check.sh pixelus`'s full-run
hash changed from 2edb2003 to 357cb9be because the pictures now come from the original's sequence (shared commit 6ca7d85,
which also adds the check against `build/baseline-pixelus`), and from 357cb9be to 53a08556 because the bot now answers the
tutorial's bubbles and the sticky-spots bubble, which takes updates, and to 4647b6d5, then 45b76e98, for the completion screens.

## Oddities (not chased)

- **Cursor and side after the first throw.** The original's cursor/side values (`grid+0xb054`, `+0xb058`) at the first
  move are the ones the port shows after its second move (it skips one change), then both agree. The tiles' places, the
  moves and everything else are the same; the values are the aim, which the bot's hover/click pairs set in a different
  order from a player's. See it in `case.sh` output as `cursor`/`side DIFFERS at change 1`.
- **The app generator is 3 draws apart after the story is closed.** The original runs the story's last update before the
  scripted click that closes it; the port closes it before. The difference stays constant afterwards (the characters'
  three draws), the global generator is unaffected.
- Two more users of the original's generators were not matched and did not show in the runs: the main loop's draw
  (0x4943b0) and the display-mode chooser (0x47acd0).

- **Tile pictures change at random in some runs of the original.** The tile's constructor (0x42eee0) never sets the field
  +0x20; 0x42f740 (the tile's update) counts it down and, at 0, draws a new picture different from the old one. A
  fresh object holds whatever the heap had, so in some runs a tile re-draws its picture in its first updates (the
  original's tile pictures then differ from the port's, which has the counter 0, with the same draw counts). The same
  case, bot seed 1 / game seed 1 over nine tutorial puzzles, matched in every value in one run and differed in the
  pictures of tiles from the eighth puzzle on in another, and bot seed 2 / game seed 7 differed from the third puzzle on.
  Nothing writes the field, so there is nothing to port.

## Deviations that remain (each without an effect on the state)

- The speech bubbles' text and picture (temple, story, tutorial) are not drawn; their state (which draws numbers) is kept.
- The story's portrait is a still head; the original renders the animated figure with its pose fades.
- The board's sidebar gleam (the colour panel at +0x1d8) is not drawn; its timer is kept.
- The help screen's demo grid (left of the slides) is not drawn; its draws are kept.

## Compared since the first report

- **The player's file after three scripted puzzles:** `users/1` is byte for byte the original's up to where the original's
  run went on one puzzle longer (it was stopped later); `users/directory.txt` and the puzzle history `users/his<n>` are
  identical. This covers the records (fewest moves, second count, "maxed") a puzzle leaves, and a profile saved by one
  program is read by the other (the port's reader follows 0x439fb0 field by field; it was not run on a file the
  original wrote beyond the one above).
- **Control modes 0 and 1** do not exist in play: the board's mode (+0x19c) is set to 2 in its constructor and no code
  writes it. The keyboard/mouse-aim paths of 0x403370 and 0x408400 for modes 0 and 1 (and the editor's grid flag
  +0xb0a1) are dead, so they are not ported.
- **87bce0a (sidebar text, World Record background):** the sidebar of the board is identical to the original's at the
  frames before, during and after a puzzle's end (shotdiff of the original run in the Sandbox against the port at the
  same updates, runs s2 to s4); no box lies in the sidebar.

## Phase two, changes so far (2026-11-08)

| Difference | Cause in the original | Port now |
| --- | --- | --- |
| Sounds of buttons, start of a puzzle, the help demo's tiles | 0x4170d0 plays nothing on a press; the buttons play their own over and click samples; the demo plays the tile sounds at fixed demo frames | the same (`soundcmp.py` compares the port's sound log with the original's) |
| Skip the tutorial showed the intro story | 0x41cf50 finds the temple the solved count opens (the first of the nine) | `ShowStory` mode 1 does the same |
| Island screen (30% of the picture differed) | 0x4163e0, 0x416950: a panel with the island's text and the page of the temple under the mouse (0x414750, 0x414bf0), hover glow, arrow bounce, lightning, gulls and surf at random draws of the application's generator, sea and smoke cross-fade baked into the background, a new temple comes in 1.1 s after the map opens | `IslandMap`/`IslandPanel` follow it; the island's particle bursts (Rumble, PauseBloom) and the pointer follower are not ported yet |
| Mouse-over sound on a temple | 0x416180 plays sample 16 on entering, 0x4163e0 plays sample 10 after 50 updates on one temple with the pitch set by the temple's index | the same |
| The completed screen came in with a plain swap, and its picture was drawn only on the wanted cells | 0x419e60 blends the screen over the board by the flight's progress, and the picture holds every cell's colour (the brown surround too) | `PuzzleCompletedScreen::Draw` buffers the screen and blends it; `MakeSolvedImage` draws every cell | bf2ec62d |
| God's speech bubble (temple, tutorial cave) was not drawn | 0x42c660, 0x42c930: the bubble picture and a text of the table at 0x53d4f8 (nine to a god), wrapped in a 125 pixel box and centred by its line count, alpha following the bubble's phase | `SpeechBubble::Draw`, called at the end of `DrawCharacter` as 0x42e1f0 does |
| The Cyclops' and Vulcan's bodies bobbed with their heads | 0x42eb7a and 0x42e487 draw the bodies at fixed places, (40, 220) and (10, 224); only the heads move | the same; Claudius' figure (shoulders and head, 0x42ebce) is drawn too |
| Skip the tutorial button, 7 pixels left and 3 up | 0x4106d0 places it at (305, 439) | the same |
| Story portraits | 0x42ccf0: the backdrop is stretched over a 160 by 160 picture, the speaker is drawn on it by the same routine as the temple's (moved by the table at 0x53d660), the picture is scaled into a copy of the frame; the frames are at (10, 14) and (514, 156) | `StoryScreen::DrawSpeech` does the same through `DrawCharacter` |
| The temple's puzzle thumbnails and the cave's click-here text | 0x4138d0, 0x413420, 0x4119e0 | `MakeMiniImage`, `TempleScreen::DrawThumbs` follow them |
| The tutorial board lacked the teacher and showed Reset, Hint and Undo | 0x409880: the tutorial board hides the three buttons (the ninth puzzle keeps them) and creates the teacher, drawn by the same routine as the temple's figure | `Board` hides them and draws `DrawCharacter(mTeacher)` after the tutorial sidebar | f18817b8 |
| The sweep picture's tiles sat 1 pixel off | 0x409880's draws of the gray tile take (x - 1, y - 1) | the same (u2560 from 14.4% to 1.9%) | f18817b8 |
| Temple thumbnails: no glow near the mouse | 0x4119e0, 0x4100f0, 0x410ad0: with the mouse within 64 pixels, each 6 by 6 cell of the shape shows with alpha 255 - d^3 * 255 / 2^18 (d from the cell's middle to the pointer) | `TempleScreen::DrawThumbs` does the same | f18817b8 |
| The puzzle just solved came in over its shape on the temple screen, with a medal on the tutorial's thumbnails | 0x40fb80: a solved picture starts at alpha 255 (raised 2 an update), so it shows at once; 0x4127f6 draws a medal at (x + 62, y + 75) only when the player's current temple index is not 0 | `TempleScreen` shows it at once and draws no medal in the tutorial temple (u2800 from 4.6% to 1.4%) | this pass |
| The tutorial's completed tablet showed the move count and the label | 0x41a66a: with the tutorial flag the tablet draws only its three lines (y 268, 293, 318; 283 and 308 once the tutorial is done) | `PuzzleCompletedScreen::DrawContent` the same | this pass |

The frozen baseline (build/baseline-pixelus) was refreshed on 2026-11-08: the island now draws the application's generator as the
original does (three draws in its constructor, then the lightning, gulls and surf timers), so the state logs differ from the
phase-one baseline only in their `apprand` lines; the game's own state and its draws are identical (the old exe is kept as
`pixelus.phase1.exe`).

## Phase two findings so far (not fixed; found by those shots)

- The original cross-fades between screens (help to island, temple to board); the port swaps them. At the same update the
  original is mid-fade. The board to the completed screen is ported (the screen's picture and tablet fade in over the board
  as the picture flies, alpha following the flight's progress); the shots in the middle of the fade still differ (the
  alpha slope of the original is a little steeper, and its picture is stretched with a blur the port does not draw), and
  the arrived screen differs by about 9% (the medal's size and the sign texts' places).
- The completed picture's sweep: the tiles ahead of the sweep turn yellow in the original and stay blue in the port at the
  same update; a difference of the sweep's timing, not of its colours.
- The story portraits' inner picture (the Cyclops' head in the frame, 3% of the screen at u1700) differs at the head's edges:
  the figure's outline matches pixel for pixel, so the stretch filter of the framework's `DrawImage` into the frame differs from the original's
  (0x458da0 draws with the application's own stretch mode). Not chased; the same stretch is used wherever the original scales an image.
- The completed tablet's title was drawn with a four-way outline; 0x442370 centres it in the widget's width and draws a drop shadow
  one pixel right and down, then the text. Fixed (the title now matches pixel for pixel at u2700).
- The completed tablet's picture (u2700, 7.5%) is blurrier in the port than in the original's shot. Cause found, not a bug:
  0x41b13c sets the Graphics' fast stretch flag (nearest filter) when the application is not 3D accelerated, and the original in
  the Windows Sandbox runs without 3D; the port's emulated 3D path filters the stretch, as the original does on a machine with
  3D (the story portraits' stretch does not set the flag, and differs only at the head's edges, also a filter detail). The port
  now sets the flag by the same test, so a port run without 3D draws like the sandboxed original.
- Shots in the middle of a move (u2300, u2450: the tile maker at another place on the board, tiles in flight or already yellow
  in the sweep) differ because the two programs are at different steps of the same move at the same update count; the
  boards agree at the move's end (the state logs). Not a draw difference; a shot at the end of a move would line them up.
- The Skip the Tutorial log differs by about a pixel at its left cap and edges (resampling of the strip's frame).
- Tile icons and the sliding trail are drawn slightly differently; the tile maker's pose differs by a frame.

## Open (this phase is not complete)

- **Completion screens, not ported:** mode 3 (nothing sets it in 0x41b3c0) and the maxed variants were not played (mode 4 with
  the game "maxed" needs every puzzle at its fewest moves). Their animation (0x40b7d0's three counters, the figure's pose
  and the rays in 0x40bd80) is not drawn, nor its music (0x1a, 0x1b in the constructor 0x40bc40).
- **Compared since:** a temple finished (a port-written profile 11 puzzles into the first temple, two puzzles played) and the
  game finished (a profile one puzzle short of the end, two puzzles played: the tutorial's puzzle, then the last) match
  the original's screens, moves and times; `case.sh` takes `PRESET=<port user folder>` for that and `EXTRA_T=<seconds>` for
  the original's slower start with a big profile. In the game case the aim values (`cursor`, `side`) differ by one change
  five updates apart in one throw, the known aim oddity.
- **The bubble's place.** The original puts each puzzle's bubble in a different place (measured on its screen: the
  corner of puzzles 4 to 9 is a fixed point in `Board::HintCheck`; puzzles 1 to 3 are placed by their lesson cells). The
  rule behind these places is not in our export (0x40f160 gives the same y for all, -1); the click on OK is what the
  comparison needs, and it hits in the original for all nine. A drag of the bubble (`Dialog::MouseDrag`) is not tested.
- **Not ported, presentation only (phase two):** the helper widget's bubbles with arrows (0x40ece0, 0x406050, 0x406140),
  the glowing lesson cells (0x422610), the teacher's figure in the tutorial's sidebar and its texts per step (the
  original shows the cyclops and a sentence for each puzzle; the port the tile icons), the sidebar's RESET, HINT and UNDO
  buttons with their labels.
- The non-expert check of a click on the tile just placed (0x403930, 0x403fb0, called from 0x408400, 0x409160, 0x409bd0)
  was not compared.
- The puzzle currently being played (`his<n>` holds its moves) was only compared after the puzzle's end, not
  continued across programs in mid-puzzle.

## Phase two: real-input play, registration, and where it ends (2026-11-08)

**Real input, natively and on the web.** `games/pixelus/tools/realplay/drive.py` plays the game under rundesk with posted
mouse and key events only, from the launch (the loading screen's link, the main menu, the new-player dialog, the story, the
help screen, the island, the temples and the boards) through the tutorial's nine puzzles and a whole temple (Flora, 12
puzzles), choosing every move with the port's own bot (`move`) and reading the state from the log. The runs, all with 21
puzzles solved in the profile: normal mode, with the focus taken away and given back in the middle of a board (r10, 4352 s);
expert mode (r14); full screen by Alt+Enter after the first board, in both modes, over the tutorial's first five puzzles
(r12, r13). One expert run in full screen (r11) stopped at its fourth puzzle's completed screen when the game did not answer
a command for 10 s; the same run again passed, and nothing in the log of the first shows a fault, so it is listed here as a
stall of a busy machine, not explained; four more runs of it (expert, full screen) all passed, and the driver now waits 60 s for a screenshot or click answer instead of 10 s. `games/pixelus/tools/web/play.js` plays the same on the web with DOM events (a hidden
Browser pane, `?fast=1`, `?scale=2`; the pane cannot enter full screen, so `?scale=2` is tested in its place and Alt+Enter is
not tested there): normal mode with the focus lost and regained on every board (21 solved, 0 errors) and expert mode (21
solved, 0 errors). The game has two modes, normal and expert (the new-player dialog's box); it has no 3D.

**Registration and outbound buttons** are not in the port: the menu's More Games button and the register dialog are left
out (GAME_NOTES.md, section 9). No fixes mod is made: "Fix candidates: none" there, with what was checked.

**Phase two is not complete.** The compared screens still differ where listed above: the cross-fades between the help
screen, the island, the temple and the board; the temple screen's thumbnails; the help screen's two live grids (the demo on
the left, the puzzle on the right); the islands' particle bursts and pointer follower; the tutorial's helper bubbles and
glowing cells; the credits roll; the speeches' portraits (the original renders a 160 by 160 figure at the speaker's offsets);
the tablet's particles. None changes a draw of the game's random numbers or a screen's state. The completed screen's fade is
ported (picture and tablet over the board by the flight's progress); mid-fade shots differ by an alpha slope and the picture's
blur.

## Notes for the repository

`tools/identical/setup.sh` copies `re/pixelus/bin/PixelusGame.exe` (the game made by `tools/extract_game_exe.py` from the
wrapped exe) into `C:/byo-og/pix` and writes `tools/origrun/patches/pixelus.txt` (git-ignored; the two seed patches).
No script added here takes a wrapper apart, so nothing new belongs in `.publishignore`.
