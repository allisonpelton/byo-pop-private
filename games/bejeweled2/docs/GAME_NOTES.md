# Bejeweled 2 Deluxe: notes on the port

Bejeweled 2 Deluxe (Bejeweled2Game.exe, `Sexy::WinDMApp : SexyApp`, the global app object `gApp` at 0x67ccd8) is ported
from our own Ghidra export in `re/bejeweled2/` (git-ignored). The PC exe is the authority for behaviour; the Mac build's
symbol names (`re/bejeweled2/mac/Bejeweled_2.symbols.txt`) are used for names. The tool `games/bejeweled2/tools/dec.py`
decompiles a function of the export again (`<hex>`, `-r lo hi [minsize]` for a range, `-l` for a list, `-a` for the
assembly). Source files carry the original addresses in their comments, and every member of a translated class has its
original offset in a comment.

This file collects what the port does differently from the exe and what it found odd. It is written as the port is made;
a section that says "not yet" is still to be done.

## Deviations from the original

Each of these has no visible effect on play unless the entry says so.

- **Registration, trial and online code is left out.** No registration or unlock path, no trial counters (TitleScreen's
  trial logic, the nag screen before play, `DoPregameNag`), no announcement or update check, no "More Games" button
  (MainMenu; the 3D game's picture of the gadgets, `IMAGE_MAIN_GADGETS`, holds the button's pill, so `MainMenu::Draw` leaves the
  rectangle round it, (600,660) to the right and bottom edges of (788,768) in the 1024 layout, out of that picture; the 2D game's has none), no PopCap pages (OptionsDialog's Register, PopCap page and Update Check buttons and the Automatic Update
  checkbox; `mAlwaysUpdate` is no longer set). OptionsDialog's Main Menu button with no board calls Shutdown in the original
  (a branch that cannot be reached); the port has no such branch.
- **The title's Play link makes no click sound.** TitleScreen's ButtonPress (0x5d9b0e) plays the click only while the
  trial counter at +0x308 (the updates before the nag, 100 at the start, counting down in an unregistered game) is 0; in a
  registered game it stays 100 and the original is silent there too. The unregistered copy that origrun runs plays it, which is
  the one `menuclick` that every mode's sound comparison lists as original-only.
- **Loading is done in line.** The original loads backgrounds, resources and the galaxy map on worker threads
  (`WorkerThread`) while the screen keeps drawing; the port runs those tasks in one go on the main thread (the framework runs
  a SexyAppFramework game's loading thread in one go in the same way). `backdrops.lst` is not rewritten, and the reload
  thread of the original runs in line. Board's image preparation (`mStaticImage` and the like) is in line too.
- **No Vista migration** of the old save folders; saved games use "/" as the path separator.
- **No game-state recorder** (GameStateEngine's recording), no write-only app byte (+0xf24), no `OutputDebugString` in
  UpdateBonusPenalty.
- **Dead functions are left out**: 0x59bb4f, 0x59bbc1, 0x59c390, 0x59dd3c, 0x594c71 and 0x5963ba are never called.
- **The 640 by 480 game shrinks its pictures as they load, and keeps no copies.** At 640 by 480 (High Resolution off, or no
  3D acceleration in the original) the original loads every picture that has no small art of its own (`sm_path`) and no
  `nr_` in its name at five eighths of its size: it shrinks the picture once (0x5afda3, every 8 by 8 pixels to 5 by 5, each
  new pixel the sum of the old ones by the part it covers, in 256ths), writes the result as `sm_<path>.tga` into the user's
  folder, and loads that file from then on while it is newer than the picture. The port shrinks at each load with the same
  arithmetic (`CustomResourceManager::ShrinkImage`, through `WinDMApp::GetImage`) and writes no file: loading stays as fast as
  the machine allows. The backdrops, planets and the galaxy's map (`GetCachedImage`) go the same way. Until phase two the port
  loaded the full pictures at 640 by 480, so everything laid out from a picture's size stood wrong there (a dialog's
  buttons most of all) and `AddAlphaSparkle`'s pixel test read other pixels than the original's.
- **Memory leaks of the original are fixed**: Lightning's bolt array, `mPoints` when CheckForHints clears them, the old
  backdrop that the XMAS cheat replaces, and PuzzleStateOverlay's dome image (`mDomeImage`) are freed.
- **Uninitialised state is zeroed**: the ranges of a `Piece` that the original never sets, a ShockWave's grow flag (set
  true, as the original's uninitialised byte happens to be), `PuzzlePod::mIndex` (0 until the overlay's constructor sets it)
  and the tunnel rings' size, and points' x and y in Hyperspace (DoUpdate sets them before they are read). A new player's
  `Profile` counters (+0x7c, +0x80, +0x84, +0x8c, which open the secret modes) are zero as well (see Oddities).
- **The board's memory is zeroed** (`Board::operator new`). The original's constructor leaves some sixty members unset (for
  example `mChainCount`, the swap and hint phases, the slide values), which the first game reads as zero from fresh memory and a
  later game reads as the last board's leftovers; the port always reads zero.
- **A gem colour table copy**: `gOrigGemColors` is copied into `gGemColors` as the original's code does at start.
- **The four colour tables are one block** (`gColorBlock` in BoardEffects.cpp): the lightning, gem, score and original gem
  colours (0x6797d0 to 0x6799cf) are one run of memory in the original, and the gem types past 7 (rock, bomb, hyper cube) index
  past the end of a table into the next, so the port keeps them in the same order and the lookups give the same colours.
- **`mFadingSound` is never set** (the original's code never does either; kept only for the offsets).
- **EffectOverlay.** The destructor frees the text picture (`mTextImage`), which the original leaks. The constructor zeroes
  what the original leaves uninitialised (the text's alpha, scale, stretch, age and hold, the portal and face timers, the wild
  gem's place, the fade) and sets `mHasAlpha`, which the first stub missed. The portal's wait for the backdrop's task
  (FUN_005e3594) is left out, since tasks run in line, and a missing backdrop is skipped. The face's data files (edata) are read
  with `ReadBufferFromFile` and an emulation of the C runtime's text-mode `fgets`/`feof` (CRLF to LF, Ctrl-Z ends, lines of 255
  characters); a line without a space, a polygon colour outside 0 to 9 and a polygon of more than 20 points (the data has at
  most 6) are skipped or clamped, where the original would read past its arrays. `powf`, `sinf` and `cosf` are libm calls
  rounded to float, where the original calls the C runtime's x87 versions: a last-bit difference is possible in the wild gem's
  curve and in the hint arrow's cosine.
- **GalaxyMap.** The map picture (`mMapImage`) that every Setup overwrites without freeing is freed first; `mSelectedPuzzle` and
  `mPuzzleBoard`, uninitialised in the original's constructor, are zeroed. The setup that the original runs on its worker
  thread runs in line (`mSettingUp` is never seen set), and the `Sleep` calls and the wait for the worker in Draw and in the
  destructor are left out.
- **Hyperspace's 3D tunnel wraps its texture by cutting triangles.** The original sets Direct3D's WRAP (u and v past 1) and CLAMP
  (v below 0) mode for the tunnel's triangles. The framework's display takes the same two settings now
  (`DDInterface::SetTextureAddress`, `SetTextureAddress` in Hyperspace.cpp), and with a wrap set draws each triangle as the
  pieces that its whole-number u and v cut it into, each moved into 0..1 (`framework/TriangleWrap.cpp`). A pixel at the edge
  of a piece filters with the picture's own edge, where Direct3D's wrap filters with the opposite edge: at most one pixel of
  a seam, not yet compared with the original's.

- **The port never calls `Done3dTesting`.** The original's `DDInterface::Init` calls it (vtable +0x15c) once its 3D test is
  over, and on a machine without acceleration the app then drops to 640 by 480 (`HighResolution` 0). The port's display decides 3D
  by the SDL renderer it got (accelerated or software) and the app keeps the 1024 by 768 size either way, so a software-only
  machine plays at 1024 by 768 where the original plays at 640 by 480. A test that wants the original's 2D size (the phase-one
  comparison, which runs the original on rundesk's desktop, where it has no 3D) sets `HighResolution=dword:0` in the registry file
  and `SDL_RENDER_DRIVER=software` (`Driver(is3d=False)`).
- **On the web the screen is the display, and never too small** (web only). The original takes the screen's size from
  `GetSystemMetrics` and uses it in three places: the Options dialog refuses the windowed game on a 640 wide screen and
  asks to give up high resolution on one of 1024 or less, and `SwitchScreenMode` drops to 640 by 480 when a high resolution
  game goes windowed there. Natively the port asks SDL for the display's usable bounds. In a browser those are the page's
  inner size (`window.innerWidth`: the frame the game is shown in, which is 640 by 480 as the site's Desktop 95 window
  starts), and the page scales the game's canvas to the room it has, so the web build takes the display's own size
  (`screen.width` and `height`, SDL's desktop mode) and never less than 1280 by 1024: none of the three ever applies there.
- **Four differences from the coverage check, kept** (no effect on play). `BackdropsParser::Parse` has other error and end-of-file
  paths than the original (only a malformed `backdrops.xml` shows it, and the game ships one that parses). `~TitleScreen` leaves
  out the register link (registration code). The `Profile` and `Planet` constructors zero fields the original leaves unset (the
  port's runs are the same on every start). `Board::~Board` deletes the score texts from the front where the original deletes
  them from the back (the order of two `delete`s of independent objects).

## Test commands

The standard commands of `docs/TEST_COMMANDS.md` are in `src/TestCommands.cpp` (`WinDMApp::HandleTestCommand`, reached from
`HandleScriptCommand`); a command that is never given changes nothing. Done: `state` (screen, dialogs and the buttons of the
top one, menu state, mode, level, level index, score, phase, tick, busy, the board as eight strings of gem types, the selected
gem), `goto <mode>` (a latch that `WinDMApp::UpdateFrames` steps once an update through whatever screen is in front, as a
player's clicks would: the title's play link, the first user's name ("Tester"), the welcome and other messages, the menu's
mode button, so it works from launch, from the menu and from a game; `goto menu` from the title clicks the play link, from a
game it runs `DoMainMenu`), `unlock all` (the counters that open the secret modes; given before a user exists it waits for
the goto that creates one), `seed`, `set score`, `lose` (the game's own ending), `win` and
`level <n>` (the level's goal is met, so the board's own update starts the whirlpool; in the puzzle modes `win` is
`PuzzleCompleted`), and `move` (the first of the moves `Board::FindMove` finds, picked by a hash of the board and the score;
the swap as two clicks in game pixels). `move` also answers for the situations around the play: while a goto is on its way, or a
game starts or ends, it asks to wait; with a message open (a one-time hint, the How To Play screen) it answers with the click
on the message's button, once the dialog has stopped sliding; in the puzzle modes (puzzles and Cognito) the move is the first
valid one of the puzzle's own solution file (`FindPuzzleValidMoves`, the moves the hint shows). `goto options`, `goto help` and
`goto highscores` take the same latch to the menu (closing whatever is open the way its own button does) and then open the
options dialog, the How To Play screen or the high scores; the credits are no screen of their own (they roll after the last
level of Classic) and answer an error. `state` also carries what a driver clicks: `widgets` (the middle of the title's play link,
the menu's mode buttons and secret button, the high scores' back button, in game pixels; `[0,0]` before the menu has laid them out),
`secret` (whether the menu's secret button is toggled on) and `user` (whether a player exists; `unlock all` with one also lays the
menu out again, so the secret button shows) and `updates` (the app's update count, which is what origrun's update counter is in the
original, so a recorded input can be given to the original at the same update). `-trace` (with `-commands`) prints a line for every
change of four values of the board, in the form of origrun's `--watch` lines (`watch 1: 0 -> 30 at update 1015`; "-" while there is no
board): the score, the level, `mState` and the index of the board's random generator (`MTRand::mti`; the original's `[[0x67ccd8]+0xe68]`
plus 0x15e20, 0x15e4, 0x170c and 0x15e0). `games/bejeweled2/tools/origcmp/` is the phase-one comparison with the original: `record.py`
plays a mode with real input in true 2D (below) and writes every input with its update, `to_origrun.py` turns that into an origrun
script, `run_original.sh` runs the original on origrun's virtual clock with the same seed and those four watches, and `compare.py`
lines the two lists of changes up. Not yet: `set` for other keys. `tools/playbymove/playbymove.py` plays every mode but the puzzle
ones with `--first "unlock all" --first "goto <mode>"`; `games/bejeweled2/tools/realplay/full.py` is the real-input driver (its header says how it runs; `soak.py` is its first version).

`state` also names, for the options dialog, its Main Menu button (`buttons.mainmenu`) and its three display checkboxes
(`checks`: `fullscreen`, `accel` and `highres`, each the middle of the box and 1 when it is checked). `origcmp/record.py` has two
more modes, `resume` (Classic left by its menu button and loaded again) and `load` (the saved game of the player's folder;
`--hyper` swaps a hyper cube first), and `origcmp/make_save.py` makes such a folder: a player with a Classic game saved at a
level (`level <n>`), flame gems and hyper cubes set in the file. `to_origrun.py --remap X,Y=X2,Y2` gives the original a click at
its own place where the port's 640 by 480 dialog layout is not the original's.

## The user's faults of 2026-11-10 (the local site)

1. **The new-user box was not pre-filled with the site's player.** The game had no site hook. `src/SiteProfiles.cpp` (web only)
   is one now, after Pixelus's: the roster is `Profile::mNames`, a name is fitted by typing it into a box with the dialog's font
   and limits (12 characters, 200 pixels, letters, digits and spaces) and tidied as `CreateUserDialog::GetName` does, and
   players are made, chosen, renamed and deleted with the functions the dialogs end in. `DoCreateUserDialog` types the site's
   player into its box under "fill" and "create", and the four dialogs' ends report the roster. Checked in the Browser pane on
   the test page: under "fill" the box held the site's name and OK made that player; under "create" the title led to the menu
   with the player made and no dialog.
2. **Full screen: "claims desktop resolution is too low for windowed mode".** The screen's size came from SDL's usable bounds,
   which in a browser are the page's inner size: 640 wide in Desktop 95's game window as it starts, so the Options dialog
   answered the unchecked Full Screen box with its message for a 640 wide desktop and checked the box again. See "On the web
   the screen is the display" above. Checked in the pane with a 640 by 480 page: the box now toggles both ways with no message
   and High Resolution stays on. That full screen "doesn't increase the resolution" is the page's: Desktop 95's full screen
   shows the game at the largest whole multiple of its size that fits (`fitFullscreen` in `web/shell.html`), and for a 1024 by
   768 game on a 3840 by 2160 screen that is 2, the same as a window can already have; told to the coordinator.
3. **"Reloading graphics" at many transitions.** `DoMainMenu` reloaded whenever a game in progress was left (IDENTICAL_PLAY.md's
   table of changes); with the screen read as 640 wide, each change of high resolution that the Options dialog asked for
   reloaded as well.
4. **The flame gem's silence and the scores.** The blast's sound was never played: one field declared twice (below). The scores
   are the original's: `properties/config.xml` pays 1000 times the level's multiplier for a gem caught in a blast and 400 times
   it for each gem a hyper cube strikes, 5,500 and 2,400 on levels 10 and 11, and the original paid the same amounts from the
   same saved game (IDENTICAL_PLAY.md).

**Fields declared twice.** The translation named some fields of the board in one file and left them as their offset in
another, as two members: what one side wrote the other never read. Found by listing, for every member with a known offset, the
functions of the export that touch that offset and whose translation does not name the member (`tools/splitfields.py bejeweled2
--export re/bejeweled2/gh/decomp`, which grew out of it and covers every class). What it still lists for this game was read and
is no split: a helper that the original has inline (`PickNextBackdrop` in `UpdateWhirlpool`, the undo list's accessors), the main
menu's fields as its second base sees them, and the update checkbox that is not ported. A second pass looked for the offsets that the export reads as an index (`this[0x5794]`) and found the fourteenth. They are one member each now:
`mExplodeHappened` (+0x15e78: `ExplodeAt` sets it, the end of the update plays `SOUND_BOMB_EXPLODE` once),
`mLastExplodeSoundTick` (+0x15e8c), `mCascadeCount` (+0x15e50: `DoneClearing` counts the clears that follow a move, and the count picks the combo sound, which therefore never rose, and the best cascade of the player's statistics), `mFadeIn` (+0x15fb0), `mStarRot`, `mStarDrift` and `mStarPhase` (+0x15fc0, +0x15fc8,
+0x15fd0), `mSecretPuzzleCount`, `mSecretSolved` and `mSecretRun` (+0x17a4 to +0x17ac), `mSolutionFirstNode` (+0x18f0),
`mSolutionMove` (+0x18fc), `mFinale` (the port's `mHintTutorialShown` was never set) and `mIsPlanetIntro` (+0x191d, which the
constructor does clear).

## Oddities (skipped, not chased)

- **PuzzlePackSelector's double click** calls `ButtonDepress(1000)`.
- **A missing galaxy file** (`GalaxyParser`): the "Unable to open galaxy.xml file" error is overwritten by the empty parse
  that follows it, so a missing file parses to an empty galaxy that has not failed.
- **galaxy.xml's case mismatch**: one file name is "26. green right.bpz" where the file is "26. Green right.bpz" (Windows
  does not care).
- **PuzzleStateOverlay::DrawFront** draws the unlit pods picture on `g` (colorize off) while it sets the alpha on a copy of
  the Graphics, so the alpha does not apply. The port repeats it (the fixes mod's `podFade` mends it, off by default).
- **NewBoard reads freed gems** (0x597efd): it frees the 64 gems of the old board without clearing their pointers and
  then makes the new gems one square at a time, asking HasSet after every colour it draws, so HasSet looks at the
  freed gems of the squares that are not made yet. Their memory holds what the allocator left there. The port clears
  the pointers (those squares read as empty); whether the original's HasSet ever counted a stale gem, and so drew one
  more colour, is for the identical-play comparison of a level change to show.
- **AddAlphaSparkle's pixel test reads the 640 by 480 game's shrunk pictures** (0x5a7bfb): in the 2D game it looks for a solid
  pixel of the picture it is given (up to 100 tries, each a `Rand()`). Solved in phase two: the port shrinks its pictures as
  the original does (the deviations, above), the test finds the same pixels, and the 640 by 480 Endless run that had needed
  `nosparkle` is identical without it (40 moves, 27 levels). The `nosparkle` test command and `NOSPARKLE=1` are left for a
  comparison that wants the sweep out of the way; nothing needs them now.
- **Hyperspace::Update2d's star lines depend on the screen's width** (0x5c7532): each new line takes `Rand() % mWidth`, so a jump
  at 640 by 480 (the original without 3D) draws different numbers than the port's 2D at 1024 by 768 scaled down. No 2D run
  has shown a difference from it so far; the 3D game (both at 1024 by 768) needs no such care.
- **A new player's profile in the original has garbage counters, and the secret mode's "?" button shows on the new player's
  menu.** `Profile`'s constructor (0x5d0716) never sets the four counters at +0x7c, +0x80, +0x84 and +0x8c that `IsModeUnlocked`
  tests (the thresholds are 17 levels, 8, 80 and 280), so a profile made with `new` holds what the heap had: in the 3D tour
  of 2026-11-11 the four read 1280262991, 1752396605, 1330794496 and 1096766031 (`visrun.py`'s `profile` command), which are past
  every threshold, and the 3D menu then shows the secret button (the "?" at (372,531) in the new player's first menu shot, 18 thousand
  pixels). Which counters are past their thresholds depends on the allocator, and no run shows it twice the same way; the port
  zeroes them (a new player has no secret mode), as the 2D tours' menus show it, where a fresh heap read as zero. To see it, run
  `run_tour_visible.sh` (IDENTICAL_PLAY.md, the 3D game) and read the "newuser" pair. Not chased: matching a heap's leftovers
  would take values measured from runs.
- **A click on a dialog reaches the 3D original one update after the port's** (not a bug of either): in the original's 3D loop the
  board resumes its ticks one update after a dialog's button was clicked, where the port resumes at once. The comparison tool
  gives the original those inputs one update earlier (`to_origrun.py --dialog-shift -1`, `run_mode.sh` does it with `HIRES=1`; the Escape key that closes a hint dialog is the exception: it reaches the board's resume at the same update as the port's, and a shifted one resumes it an update early, which moved the carried-over flame gem's pick by one draw of the global generator),
  and the watch lines of the 3D original then fall one update after the port's (its loop) with the same values in the same order.
  The same holds for the click that closes the first game's help screen in the visible-desktop 3D tour (IDENTICAL_PLAY.md, "The 3D game"):
  visrun reads the update each input landed at, and the replay gives that one click (`landed_to_events.py --shift 2127 2130 1`) the later update.
- **The original's pointer is never over its window in the visible-desktop 3D runs, so no gem glows under it.** The Board's hover glow
  (`BoardUpdate.cpp`: a gem under `mMouseX/mMouseY` gains 0.045 a update, the step 0.0625) needs the pointer in the window, and the
  tool posts clicks to the window without moving the real cursor, so the original's `mMouseX/mMouseY` stay at -1,-1 (and the board
  re-reads `mLastMouseX/mLastMouseY` itself when it becomes idle, BoardLogic.cpp:423). The port's replayed pointer stays where its last
  click was, so a shot taken straight after a click had a gem glowing that the original's did not. The replay therefore moves the pointer
  out of the window and sends a leave one update after each click (`landed_to_events.py --leave-after 1`). Not a difference of play: a
  person's cursor over the board glows the same gem in both.
- **A gem or two differ in the loaded 3D board's slide.** In the visible-desktop 3D tour (`run_tour_visible.sh`, with shots every 2
  updates of the whirlpool and the slide added to the recording; the run's events were `t3d_events.json` in the scratch logs), the gem
  at the board's bottom right has its facets in other places in the original's shot than in the port's, in 8 of the 13 shots from
  update 3806 to 3830 (3810 to 3826, a swirl-shaped patch of 100 to 500 pixels), and the shots are identical before, after and at rest. Its cause
  was not found. The difference is small against the board's own motion in the same region (the region changes about ten times as
  much between two shots), so another frame of a gem's own animation is unlikely and a filter on a moving gem (Direct3D 7's against
  Direct3D 11's, at a fractional position) is the likelier cause; neither was tested. Not chased.
- **The original's first run after another one sometimes never creates a board** (seen a few times, in 2D and 3D alike, as a
  state watch that stays at its title value); a second run is as the first should have been. `run_original.sh` runs again
  when a log has no board.
- **The tunnel's triangles are drawn with tx = 1.0**: the original's wrapper passes a byte that stands for "blend" to the
  argument that moves the vertices in x, so every vertex is one pixel to the right. The port repeats it (the fixes mod's `tunnelEdge`
  mends it).

- **A game resumed in the 3D game: the flame gems of the next level change land elsewhere.** Classic for 25 moves, the menu
  button, Main Menu, Classic and "Load Game?" yes, then 25 moves (`record.py --mode resume`, `HIRES=1`): the original and the
  port agree through the load and 22 moves; at the first level change after it the two flame gems that are carried over are
  put on other squares, (0,2) and (2,3) in the original where the port puts them on (6,2) and (4,5), and a later clear
  explodes in one and not the other. The global generator's words and position are equal up to that update and both sides
  draw 67 numbers in it; the original's two picks are the draws one place later than the port's, as if the loaded board had
  made one more of its one-a-update draws (the shooting star's, `Rand() % 6000`) before the whirlpool ended. At 640 by 480,
  where both sides tick on the same update, the same flow is identical through two level changes, generator included, and
  a fresh 3D game is identical through five level changes with flame gems, so it is a matter of when the 3D original's loop
  takes the two dialog clicks of the way out and back in. Tried: the "Load Game?" click 0, 1, 2 and 3 updates later, and the
  Main Menu click with and without its one-update shift. It would take the original's update count at each of those clicks
  from inside its loop (a breakpoint, which origrun does not have).
- **`UpdateStarRots` wraps the wrong angle** (0x596747): after it turns the second star back by 0.004 it tests the first
  star's angle for being negative, which it never is, so the second angle runs on below zero for ever (harmless: the angle is
  only drawn). The port repeats it.
- **The 640 by 480 dialogs** stood wrong until phase two: the port loaded the full pictures there, and a dialog is laid out
  with its pictures' sizes (the yes of "Load Game?" was at (257,222), on the original's Cancel row). With the pictures shrunk
  as the original shrinks them the button is at (257,213), inside the original's, and the resumed game is identical with no
  click moved; whether every dialog matches to the pixel is the screen comparison's to say.
- **Finity's screenshots differ from the original's in small details** (2D, 640 by 480, `record.py --mode tour:finity` against
  `run_tour_original.sh`, `tools/shotdiff`): from the first shot after level 2's clear (1.9 thousand pixels of 307 thousand) to the
  cascades of level 8 and 9, where the whole board is covered by overlapping score popups of millions (up to 60 thousand
  pixels, 18 thousand of them visible). The layout of every popup, shard and bomb counter is the same; what differs is shading
  and the placement of explosion shards and the big digits' semi-transparent interiors by a pixel. The logic is the
  original's: every state, the global generator's index at every update (up to update 8716 compared), the sounds (562 plays) and the
  music (204 events) are identical, and the web build gives the native build's pictures exactly. Endless, which has no bombs,
  rocks or power gems, is identical in all 55 shots, so the difference is in the pictures that only Finity draws (the bomb's
  counter, the rock, the wildcard destroyer) or in the popups' arithmetic at scores of millions (the score popups of the
  other modes, in the same font, match). A lead for whoever takes it up: the shards' jitter and the bombs' shake use the C library's
  `rand()` (`CrtRand`, seeded by origrun's `crtseed` patch), which no watch shows, so a bomb or rock that makes the original call it a
  different number of times or in another order than the port does would move the particles and nothing else; a watch of the
  C library's seed in the original (per-thread data) would tell. Not chased (CLAUDE.md, "Oddities are not chased"); to see it, run the tour
  (`record_port.sh tour:finity 12 <dir> 909` with `PROFILE=`, then `run_tour_original.sh`) and `tools/shotdiff`.

## The fixes mod and the fix candidates

`mods/fixes/mod.json` is the one mod `fixes` (`docs/MOD_ROADMAP.md`, "The fixes mod"): a toggle for each fix, at its default on the
site, all off when a test must play as the original does (`-nomods`, or no mods folder). A fix starts on unless the original's
behaviour looks intended and is not widely known as a bug (the user, 2026-11-11: "Off by default for functionality that seems
intended but where the bug is not well known to be a bug"; `docs/MOD_ROADMAP.md`, "The fixes mod"): `tunnelEdge` starts on, `podFade` off. The port reads a toggle with `FixEnabled("<id>")`
(`Fixes.h`, the mod manager's live value), so the original's behaviour stays in the code as it was and each fix is one branch.
Before it the port loaded no mods at all: `main.cpp` now sets the mods folder (`-modsdir=<dir>`, by default `mods-bejeweled2` beside
the exe, `/bej2persist/mods` on the web) and reads it unless `-nomods` is given; the framework's mod menu (F2) and the site's page
(which installs the shipped `bejeweled2-fixes/` on) do the rest, as for every game that loads mods.

| Toggle (default) | The fault in the original | What the fix does |
| --- | --- | --- |
| `tunnelEdge` (on) | The 3D hyperspace tunnel's triangles are drawn one pixel to the right of their points: the original's drawing routine takes the one that stands for "blend" for the distance to move the vertices in x (`Hyperspace::Draw3dPortal`, 0x5c620e). | The tunnel is drawn with no shift. Three quarters of a tunnel frame's pixels change (591 thousand of 786 thousand at update 1902), since the whole tunnel moves a pixel. |
| `podFade` (off) | `PuzzleStateOverlay::DrawFront` (0x5d2004) sets the alpha of an unlit pod's picture on a copy of the Graphics and draws that picture with the Graphics whose colorizing is off, so the picture never fades: the lit one fades in over an unlit one that stays fully drawn. | The unlit picture is drawn with the copy, so it fades out as the lit one fades in. Over the 250 updates of a pod lighting (puzzle solved) the pod dips darker, the dome's dark backdrop showing through, then ends as the original's does. |

`tools/fixes_check.sh` plays a Hyper tour (the tunnel is on screen from the board's state 10 to the new board's, 310 updates here) and
a Puzzle tour (a solved puzzle lights its pod) in 3D under `-fast -drawevery` with the same seed, with the mod absent (`-nomods`), at its
defaults, all toggles on, all off, and all on but one, and compares the shots and the board's trace: in all of them the trace is the
original's (15,060 lines in the Hyper tour, 2,583 in the Puzzle tour), the shots with every toggle off are the original's, the shots at
the defaults are those of all on in the Hyper tour (`tunnelEdge` is on) and those of all off in the Puzzle tour (`podFade` is off), `tunnelEdge` changes the six
shots along the tunnel (updates 1822 to 2022) and `podFade` the three shots of the pod's fade (1594 to 1674), by 4,000 pixels each, inside
the pod's 83 by 69 box. With every toggle off the port is the original (`tools/check.sh bejeweled2`, with no mods folder, plays as before).

What was checked for more fixes, and left as it is. *Bugs that break play* (controls that stop working, a soft lock, a crash, a lost
save): none in the real-input play-through of every mode (`IDENTICAL_PLAY.md`, "Real input"). *Run-to-run variation* (the machine's
speed, the heap, memory never set): the game's code reads no clock; the one thing that came from the heap, the new player's
secret-mode counters, is zeroed in the port (see Deviations); and the bot's games in the nine modes replay identically from the same seed
(`tools/check.sh bejeweled2`). *Waits that only cost the player time*: the loading is the
port's own (done in line, at the machine's speed); what is left is the original's scenes, which an update of the game follows
(the whirlpool that opens a board, and at a level's end the whirlpool, the tunnel and the new board's show, 38, 310 and 50
updates in the Hyper tour, each ending in a screen the player plays on), so no toggle shortens them. *Other faults of the original that the port repeats* (the oddities below): `UpdateStarRots`
runs a star's angle below zero, which is only drawn, so nothing shows; the star lines of Hyperspace's 2D jump depend on the
screen's width, which is a difference between two sizes and not a fault of either; and the two above are the only ones with
a picture to mend. `podFade` is the judgment call of the two: it mends what the code plainly meant, but it changes how a pod looks
for a moment, and the original's look may be taken as intended, so it starts off (the rule above) and a user who wants the fade
turns it on.

## Coverage check (2026-11-11; `docs/COVERAGE.md`, `games/bejeweled2/coverage.txt`, `games/bejeweled2/tools/coverage/`)

**Status: done for this pass. List one is empty, five faults are fixed, and list two (the port's functions that no run reached) is judged: 52 are left,
each read again against the export (43 faithful, 4 differences, all in dead code) and listed below as a candidate for the phase that takes out code the
game does not need.** The numbers are the game's own code, 0x5936ad to 0x5e3675: 2,095 functions.

**The runs.** The original ran on the host under origrun's `--coverage`, one run at a time (`tools/coverage/run.sh`, `runall.sh`; the lists
of runs are in `build/coverage/bejeweled2/list*.txt`, git-ignored). They are: the port's recorded plays of the modes converted to origrun
scripts (`origcmp/to_origrun.py`: Classic, Action, Endless, Puzzle, Twilight, Hyper, Cognito, Finity, the dialogs and the help tour, the second
tour), the resume and name-entry tours, the five 3D recordings under dgVoodoo2 (`HIRES=1`), the long Twilight and Action plays to their game over (`tools/coverage/record_plays.sh`: the port records them, the original plays them back),
the saved game loaded and its first hyper cube swapped (`c_load`, `c_loadh`, profile `build/logs/bej2/save10`), and scripts of this pass in `tools/coverage/scripts/`: `game_a` (a Classic board with the hint, every cheat word, the focus lost and regained, the
in-game options, the window closed mid-game; also run on the real clock with the silent stand-in as `sound_a`), `opts_a`, `menu_a`, `title_q`,
`menu_c` (the menu's Quit, scores, reset and every way out), `users_a` (create, rename and delete a user, each answered both ways) and `puzzle_a`
(the puzzle from its first puzzle on, then the hint, the undo, the galaxy map: planets hovered and clicked, CLOSE MAP, WARP TO PLANET). Later ones: `puzzle_h` (the tutorial panel closed, then HINT four times: the panel covers the board and takes
the HINT click until "Continue" at (408,266)), `puzzle_u` (one move, then UNDO: `Board::LoadState` is the undo's restore, not the saved game's load),
`load_b` ("Load Game?" answered CANCEL at (320,235), then Classic again and NO at (382,199): `DeleteSavedGame`, `CancelNewGame`; YES is (257,199)),
`menu_d` (the mouse over each mode button, Quit asked and answered YES at (260,182): the shutdown timer) and `cheat_c` (COLORSWAP first, NORMAL
after 2,700 updates). One more thing the first runs got wrong: `run.sh` drops a script's `mark` lines, so a recorded play that ends with an idle
to the game's end (`at 17583 mark`) quit right after its last input and never reached the game over; `record_plays.sh` now turns that last `mark` into
a mouse move off the gems. A Finity, Classic and Hyper play took half an hour or more each to record on a busy machine and would reach the same
game-over functions as Action, so only Twilight and Action are recorded.
The scripts' coordinates were found with `tools/coverage/portshots.sh`, which plays an origrun script in the port (same updates, `-fast`) and
takes its `shot` lines. Things the first runs got wrong, so the next session need not find them again:
- **The recordings are seed 909** (`record_port.sh ... 909`); the lists ran them at seed 1, where the boards are other boards: no move cleared a
  gem, and 25 runs reached none of the clearing, scoring and game-over code. At 909 Classic changes the board's state 206 times.
- A recorded play assumes the original's timing, so `run.sh` keeps each script's `at N` and first waits for the title's loaded flag
  (`[[0x67ccd8]+0xe78]+0x9d >= 0x01000000`; the load progress at +0x90 reads 1.0 at update 35, long before) and makes a `mark`; a run whose
  title loaded later than 30 updates before the first input is made again. The 2D game takes the Play click from update 566 on (the port's
  recordings say 350: `record_plays.sh` passes `PLAY_AT=566`).
- A list file must have LF line endings (a CR ended up in `CLOCK=real`).
- The cheat words that reload every image (NORMAL after COLORSWAP or a colour screen) take no keys while they run: they come last in `game_a`.

**Counts.** Before the sorting the runs reached 914 of the 2,095 functions and 537 of the reached ones had no definition in the port (list one);
now they reach 1,060, the port defines 518, 595 are left out on purpose (`coverage.txt`, each with its reason: the standard library's
instantiations, among them the lightning vector's, compiler-made destructors, the original's event recorder, outbound code, the loading thread,
registration code) and list one is empty. 52 port functions are not reached (list two, below); 1,005 are neither reached nor defined (list
three: exception-handling funclets, `Catch_All` blocks, registration and outbound code, the recorder).

**What it found and fixed.** One Sonnet agent sorted list one in three parts (its detail lines in `build/coverage/bejeweled2/list1_res_*.tsv`);
the gaps were checked against the export by hand:
- `Board::MouseUp` (0x59596f, vtable slot 56) was not translated: the board's "button is down" flag (`mMouseHeld`) stayed set after the first press
  of a game, so `UpdateFalling` called `Board::MouseMove` at every idle update (the cursor back to the pointer). It clears the flag now.
- `WinDMApp::KillDialog` (0x5dceb6, slot 76): when the last dialog is gone the board has the keyboard focus (the port left it to
  `WidgetManager::RemoveBaseModal`'s restore). Translated.
- `PuzzleStateOverlay::ButtonPress` (0x5d1b49): the galaxy map's dome button clicks (`SOUND_CLICK_DIALOG`) when it goes down. Translated.
- `StarField::~StarField` (0x5d8fac) did not delete the nebula image that `Init` loads (one image leaked per STARFIELD/NETWORK toggle). Fixed.
- `Board::~Board` (0x5a5abe) never deleted its `EffectOverlay` (found by reading `~EffectOverlay` 0x5b8cee, which no run reaches because nothing
  deletes the overlay: the original's `Board::~Board` deletes +0x90 and +0x94 and leaves +0x98 out). The overlay, its tables and its text
  picture leaked with every board, one per game started. The port deletes the overlay now (`~EffectOverlay` already deletes the picture).
- Comments only: 30 addresses were put above the port functions that hold them (constructors and destructors, `CurvedVal`, `FancyBackdrop`,
  the resource group functions in `Res.h`, which `resgen.py` now writes with their addresses).
`tools/check.sh bejeweled2` (the bot's games in the nine modes, the frozen baseline) passes after the five changes: none changes a trace.
Differences found and kept (no effect in play, listed for the readability pass): `BackdropsParser::Parse` has other error and end-of-file paths than the
original (only a malformed `backdrops.xml` shows it); `~TitleScreen` leaves out the register link (registration code); the `Profile` and `Planet`
constructors zero fields the original leaves unset; `Board::~Board` deletes the score texts from the front, the original from the back.

**List two, judged** (52 port functions no run reached; the second batch of runs took 17 off the 69: the game over, its animation and the score table,
the hint, `LoadState`, `DeleteSavedGame`, `CancelNewGame`, the shutdown timer, the lightning and `PixelsRotate`). One Sonnet agent read each of them
(and the few more that are only reached through them) against the export: 43 are faithful and four differ, all in code nothing runs: `GetOrMakePieceAt`
marks the board dirty with `MarkDirtyFull` where the original calls slot 0x44 (`MarkDirty`; fixed, dead code), `DoPuzzleRating` and
`FinishPuzzleRatingDialog` are empty stubs (`WinDMAppStubs.cpp`; the rating dialog 0x23 is never created), and `~EffectOverlay` also deletes the
text picture (the leak fix above). The rest, by why no run reaches it; the ones nothing can reach are taken out (the paragraph "Taken out"
after the list), and the others stay.
- **Dead in the original** (no caller, no vtable slot: 15; all but `UpdateLevelTransition` taken out): `GetColAt` 0x594c19, `GetRowAt` 0x594c42, `GetBoardSize` 0x5957b2, `GetOrMakePieceAt`
  0x59597c, `GetOldBoardHash` 0x5963ba, 0x5aef96 (`CurvedVal`), 0x5d3ffd and 0x5d8b5d (resource helpers), `DoPuzzleRating` 0x5dc25b,
  `ShowInstructionsDialog` 0x5df3d7, `ShowFullInstructionsDialog` 0x5df4d9, `ShowColorDepthDialog` 0x5df614, `ConfirmNewGame` 0x5e119e, `Board::NewGame`
  0x5a50a0 (only the dead dialog's button calls it) and `UpdateLevelTransition` 0x5a510a (no code sets board state 11).
- **The high-score name dialog** (8: `HighScoreNameDialog`, 0x5d31e4-0x5d34c8 and 0x5d3382; taken out): its only creator, 0x5dcdf0, has no callers; the score
  table takes the profile's name (`HighScoreWidget::NameEntered`, reached in the Action run).
- **The puzzle pack selector** (10: `PuzzlePackSelector` 0x5d0f09-0x5d13ae, `DoPuzzlePackSelector` 0x5dc1f6, `FinishPuzzlePackSelector` 0x5e15ea): shown only
  when `puzzles/` holds more than one pack folder; the shipped game has one (Galaxy One). Kept.
- **Error paths** (3: `BackdropsParser::Fail` 0x593732, `GalaxyParser::Fail` 0x5be56d and 0x5be07b): only a malformed or missing `backdrops.xml` or `galaxy.xml`.
- **The galaxy's finale** (6: `InitFace` 0x5b9071, the shape interpolation 0x5b592c and 0x5b5960, `powf` 0x5b5229, `GetPuzzleNum` 0x5d0ee6, `~EffectOverlay`
  0x5b8cee): the last planet of the puzzle galaxy; `DoHyperspace` sets `mFinale` when the planet index is the last, and a profile with the whole galaxy
  open and a warp to it would reach them (a puzzle driver with the test commands; not done). `~EffectOverlay` never runs in the original.
- **`GalaxyMap` slot 47** (0x5ba705, a bare return: the map is never the focus widget).
- **Rare states and inputs** (9): `UpdateBoardDropping` 0x59814e (Action, no move left), `ShowNow` 0x59828a (a new game with no main menu around),
  `ShowPlanetCount` 0x59de98 (a planet starting with nothing solved and a pod chosen), `UndoAll` 0x59bb11 (the secret puzzles' "Reset Puzzle"),
  `UndoUntilSolvable` 0x5a96f9 (a puzzle made unsolvable, then HINT), `FinishPuzzleRatingDialog` 0x5e1a45, `SwitchScreenMode` 0x5e0b03 (Alt+Enter and
  the full-screen switch: a run that changes the window, kept for the overnight window), `DrawMouseover` 0x5c8afd (the original's hover run, `menu_d`,
  reaches `PodButton::MouseEnter` and `SelectMode` but never draws the glow; `Update`'s glow loop and `Draw`'s call read the same as the port's, and the
  port draws it; an oddity, not chased) and `WinDMApp::ButtonPress` 0x5dc1cf (the pod dialogs' own `ButtonPress` plays the click and does not call it).
- Not done, for later: the full-screen toggle run, the finale, the secret puzzle's reset, a puzzle made unsolvable. The Collection check (the PopCap Game
  Collection's Bejeweled2DeluxeComponent) came after the performance pass (done: the section "The PopCap Game Collection version" below).

**Taken out** (2026-11-11, in the pass that removes the code the game does not need; no code of the original calls these, no run reaches them and the
port's own code never called them): `Board::GetColAt`, `GetRowAt`, `GetBoardSize`, `GetOrMakePieceAt` and `GetOldBoardHash` (0x594c19, 0x594c42, 0x5957b2,
0x59597c, 0x5963ba), `Board::NewGame` (0x5a50a0: only the yes button of the "New Game?" question called it, and `ConfirmNewGame` 0x5e119e, which asks
that question, has no caller; the question's dialog id 0 and its button branch are gone with them), `CurvedVal::SetInRange` (0x5aef96), the resource
name table and group finder of the exe (0x5d3ffd and 0x5d8b5d: `resgen.py` no longer writes `GetStringIdById` and `ExtractResourcesByName`, nor the
245 names), the three dialog openers `ShowInstructionsDialog` 0x5df3d7, `ShowFullInstructionsDialog` 0x5df4d9 and `ShowColorDepthDialog` 0x5df614 (the dialog
id 13 stays: the galaxy map's "Planet is Locked" message uses it), the high-score name box (`HighScoreNameDialog`, 0x5d31e4-0x5d34c8, with its dialog id
10, its yes-button branch and the port's `HighScoreWidget::SetHighScoreName`, which only that branch called; the registry's `LastScoreName` is still
read and written) and the puzzle rating box's two stubs (`DoPuzzleRating` 0x5dc25b and `FinishPuzzleRatingDialog` 0x5e1a45, with the dialog id 35 and its
two button branches; `WinDMAppStubs.cpp` is gone). 24 functions of the port (two of them inline in headers) and 587 lines in all, 273 of them the generated names. Each has a `none` line in
`coverage.txt`. Proved by the object code (`tools/objsnap.py`, before and after): 44 of the 48 objects that are left are the same instructions, data and sizes; the other four
are the ones that held a removed function (`BoardBase`, `BoardLevel`, `Res`, `WinDMAppDialogs`), each with exactly those functions gone and every other
function the same size, and by the frozen baseline (`tools/check.sh bejeweled2`: the nine modes, rerun and games as frozen, baseline identical).
Left in, though no run reaches it: `UpdateLevelTransition` 0x5a510a (the update's case for board state 11, a state no code sets: it is one case of the
state machine, which the port keeps whole), the puzzle pack selector (the original shows it for any `puzzles/` folder with several packs, and the
port follows the data it is given), the parsers' error paths, the finale, `GalaxyMap` slot 47, and the rare states and inputs above, which a player
can reach.

## Performance (2026-11-11; `docs/COORDINATION.md`, "Performance pass" and "The target")

**Status (2026-11-12).** The processor-time target (the port's at most the original's, like for like, the window at the original's own size) is **met**:
idle, in 2D the port takes 6.6% of a core on SDL's software renderer and 7.5% on a GPU renderer against the original's 9.4%, and in 3D 8.8% (five rounds,
the table "At the game's own size" below). At the player's default window, which is twice the game's size (1,280 by 960; 3D 2,048 by 1,536) and so has no
original to compare with, the GPU paths cost the same (8.5% and 8.6%) and the software renderer 48.5%: the software stretch of every sprite to twice its
size. The first measurement (2026-11-11, the table after the method) set the port's default window against the original's 640 by 480 and so read 93.7% against 12.7% in
2D software: not like for like (the Mahjong session's finding, 2026-11-12). The game's own code is 2.5% of the process on the software path and 0.14 ms an update everywhere;
nothing in it was changed.

**Method.** `games/bejeweled2/tools/profile/` (`perf.py`, `pairs.sh`, `summarize.py`; `statspatch.py` for the counters): the port and the original play the
same stretch, alternating, five pairs, the whole loop on one performance core of its own (`tools/pincore.py`), the medians with their ranges.
*Processor time is the process's cycle count* (`QueryProcessCycleTime`, every thread's `QueryThreadCycleTime` with the exited ones) over the machine's
2.112 GHz clock, never `GetProcessTimes` (charged by the scheduler's 15.6 ms tick: a first pass of this section read 15.2 s where the cycles say 10.8 s
for the same run). The stretches: (1) *the idle board in real time*: the recording's inputs up to the click that dismisses the first dialog of a Classic
game, nothing after, to update 5700, and the last 20 s of it; the original on origrun's real clock (its 12.7% of a core is no spin on a vertical blank, which would take
all of one), the port as a `-script` file, so it has no commands file to poll; both end on an
idle board (`state`: phase idle, score 0, no dialog; the original's board state 1). (2) *the recorded game without waiting* (`scenario/classic909.json`:
seed 909, 36 swaps, 6,421 updates; the 3D scenario's 7,897): the work of the game's code and drawing; the original on origrun's virtual clock counts only
if its final score is the recorded game's (5 of 5 did), and the port plays it as a `-commands` file under `-hold`, whose poll of the file (a `stat` a
pass, test-only) was counted in a build with `statspatch.py` and is taken off the figure (0.1 s of 9.9). (3) *the load*: from the start of the process
to the title taking input. The original's 3D is its DirectDraw and Direct3D 7 under dgVoodoo2 (`--dgvoodoo`, as every 3D comparison of this game runs
it), so its 3D figures hold that wrapper. Sound: origrun's stand-in takes the original's DirectSound, so the original mixes nothing; the port's
mixer is its SDL audio thread (`SDLAudioP2`, silent driver under rundesk) and is counted apart: 0.10 to 0.17 s in the 20 s (about 0.7% of a core).

| Five pairs, 2026-11-11, **the port's window at its default 2x, the original's at 1x: not like for like** | Original, 2D | Port, 2D, GPU renderer | Port, 2D, SDL software renderer | Original, 3D (dgVoodoo2) | Port, 3D |
|---|---|---|---|---|---|
| Idle board, share of one core | 12.7% (10.9 to 13.7) | **8.6%** (8.1 to 10.5; 7.9 less the audio thread) | **93.7%** (91.6 to 96.1) | 61.3% (52.2 to 64.5) | **9.3%** (8.1 to 10.4; 8.7 less the audio thread) |
| The recorded game without waiting, after the title | 7.71 s (6.65 to 9.80) | 0.21 s (0.05 to 0.28) | **9.83 s** (8.94 to 11.48) | not run (it spins on its virtual clock) | 0.61 s (0.56 to 0.83; 7,900 updates) |
| Load to the title | 4.41 s (4.07 to 6.58) | 1.18 s (1.02 to 1.41) | 0.89 s (0.78 to 3.48) | 4.71 s (4.31 to 5.22) | 1.42 s (1.28 to 1.62) |
| Private bytes at the title | 39.9 MB | 88 MB | 59 MB | 144.6 MB | 128 MB |
| Peak private bytes of the idle run | 41.9 MB | 82.3 MB | 103.5 MB | 149.3 MB | 109.4 MB |

So in 2D on a GPU the port takes two thirds of the original's processor time idle and a thirty-seventh of it for the recorded game, and in 3D a seventh of the
original's idle (the original's 3D on dgVoodoo2 is the dearer way to run it). In 2D on the software renderer it takes seven times the original's idle
and 28% more for the recorded game. Memory stays the framework's (about 54 MB in every process before it reads a game file, the pixels kept after upload);
it is not a target. The earlier figures of this section (the first pass of the day) are replaced by these.

**Where the 2D software path's processor time goes** (a build with `statspatch.py`: every SDL draw call counted and timed; the idle board, 61 s, 3,786
drawn frames; `DRAWSTATS=1`). 98% of the process's time is inside those SDL calls (55.0 s of 56.3 s: the game's code and the rest are 2.5%). A frame is
14.5 ms: 64.6 copies of images drawn at full alpha (11.8 ms, 183 µs each, 1.9 Mpixels a frame: a screen and a half at the render scale), 7.2 additive (0.64 ms),
1.4 with alpha (0.35 ms), the copy of the screen to the window (0.85 ms) and the present (0.85 ms); what the 64 are was not itemised (the board's 64 gems
are the obvious part). Two things make a copy that dear. The window is 1,280 by 960 on this machine (the screen copy's destination is 1.23 Mpixels), so
the framework's render scale is 2 and every sprite is stretched to twice its size by SDL's software stretch (a copy of a 29,000-pixel sprite takes
183 µs, about 6 ns a pixel); and a texture is blended when it is drawn, whether or not its image has any transparency. With nothing to wait on the port draws 64 frames a second at that price
(the GPU path is paced by the present's vertical blank: its `Present` takes 16.4 ms and 1.96 s of the 20 s are processor time).
Three changes to the framework's 2D path were tried in a scratch build (one run each, the unpinned machine; the diff is in the coordinator's notes
folder, `bejeweled2_2d_software_prototype.diff`; none is committed):
1. *An image with no transparency, drawn at full alpha, as a copy* (`SDL_SetTextureBlendMode(NONE)` when `!mHasAlpha && !mHasTrans && alpha == 255 && normal mode`):
   the copy takes 87 µs in place of 171 and a frame 7.8 ms in place of 13.2; the picture (a screenshot at update 5000) is the same pixel for pixel.
2. *Rendering at the logical size (`-renderscale=1`) and stretching once at the present with nearest-neighbour*: a frame 2.6 ms, the idle board
   **23.8% of a core** (5.5 times less; the port then draws 93 frames a second). With the linear copy the cost only moves (the final stretch is
   12.5 ms a frame, 55 s in all). This changes the picture (a screenshot at render scale 1 differs from the scale 2 one inside (47,21) to (607,437) of the 640 by 480
   picture: the sprites are drawn at the logical size and doubled), so it is the user's look to decide.
3. What is left to reach the original's 12.7% is drawing less: the original's DirectDraw path redraws only the dirty rectangles and draws at the
   display's rate, where the port redraws the whole picture on every update it has time for; the software path could draw at most 60 frames a second
   (as a player's screen shows them) and skip a frame when nothing was marked dirty.

**Done on this branch after the table (2026-11-11, evening): two shared commits, to be merged with the other games' checks.** Change 1 is in
the framework as `e19af272` (the blend mode of an image with no transparency, at the draw call; the idle-board screenshot at update 5000 is pixel for
pixel the same; the port's idle share 95.7% of a core before, 76.7% after, five alternating pairs on a machine with other games running meanwhile).
A review found that images in forced mode (`SetImageMode`, which the board's static background is) say nothing of their texels, so `fec8a8a5` leaves
them blended; measured again with three builds turn about (machine 50 to 90% busy): share of a core 82.6% none, 79.4% with `e19af272`, 87.8% with
`fec8a8a5`, frames a second 44, 73 and 53. The software path has no pace for its draws, so a cheaper frame shows as more frames a second at the same share (the
first figures above were that); the board's own forced background was two thirds of `e19af272`'s saving, and about 10% a frame is left. An exact test (the
alpha bytes read at the texture's upload, no flags) would bring the rest back; not done. The commands file's poll is cheap on Windows as `fa73221e` (0.11 s of processor time over the recorded game's 643
calls before, 0.015 s after). Change 3 goes into the presenter (the Mahjong session's window-surface presenter); change 2 (render scale 1 and a
nearest-neighbour present, which changes the look) waits for the user, and only if the first and third leave the software path over the target.
The table above is the measurement before these: repeat it after the merge.

**Two more shared commits (2026-11-12, night): the exact opaque test and the software renderer's pace.** `7413ba61`: `PrepareTexture` notes at each
upload whether every alpha byte of the image is 255 (not for the HD textures or the screen image), and `DrawTexturedQuad` sets the blend mode off from
that, for every image, forced-mode ones included, so the board's background gets its saving back with no flags to trust; the conditions are as
before (alpha 255, normal mode, 2D, scale 1). `5eac4e4a`: SDL's software renderer is held by no vertical blank (it even reports
`SDL_RENDERER_PRESENTVSYNC` when asked, and presents with a blit of the window's surface), so the loop drew after every update; `DDInterface::FrameHeldByPace`
now holds a draw that would come sooner than one refresh interval after the last, and not with `-fast`, `-drawevery`, `-hold`, before loading or on
a renderer that vsync holds. fwtest on the software renderer: 1000 draws for 1000 updates before, 600 after. Pictures of the idle board at update
5000 are byte for byte the same in all three builds below, and the recorded game's pictures and the suites (this game's baseline, Twist, Big Money, Alchemy, AstroPop,
`unit`) are unchanged, on exes built after each change.

The idle board on the software renderer, three builds turn about (four timed runs each, one pinned core, the machine 47 to 97% busy with other
games, so the share moves a lot; the figures for each frame are the steadier ones; medians, then ranges):

| build | share of a core | processor ms a frame | Mcycles a frame | draws a second | updates a second |
|---|---|---|---|---|---|
| before `e19af272` | 64.3% (44.8 to 99.1) | 22.6 (15.5 to 22.8) | 47.8 | 26.8 (20.0 to 52.6) | 93.3 |
| `7413ba61`, exact opaque test | 60.3% (39.3 to 74.8) | 11.3 (9.4 to 13.3) | 23.9 | 53.6 (34.0 to 84.2) | 97.8 |
| `5eac4e4a`, with the pace | 52.3% (44.7 to 72.6) | 11.0 (8.7 to 13.0) | 23.3 | 56.4 (38.5 to 57.5) | 97.4 |

A frame costs half what it did, and the pace holds the draws to the display's rate where a quiet machine would otherwise draw at the updates' 100 a
second (the frames here were already near 10 to 12 ms, so on this busy machine the load held the draws before the pace did; the
fwtest run is the plain demonstration). These runs were all at the default 2x window (1,280 by 960), which is not the original's size; the like-for-like
figures follow.

**At the game's own size (2026-11-12, after the Mahjong session's finding that the framework starts a window at 2x the logical size when it fits the
desktop, where the original's is the game's own).** The window state of each run was read from a `screenshotfull` taken in it (its size is the render scale
times the game's size: 640 by 480 at 1x, 1,280 by 960 at the default 2x; 3D 1,024 by 768 and 2,048 by 1,536); the 1x window is the test option
`-windowscale=1` (commit `cf1dea5f` of the Mahjong branch, a local copy of its hunk in the framework's `SexyAppBase`, not committed here; it reaches the trunk with that
branch). Everything is the build at `5eac4e4a`, registry `ScreenMode=0` (windowed), the port under rundesk's hidden desktop (the renderer in the GPU
rows is SDL's default there, Direct3D 11 by the Mahjong session's finding; the port's log names none; the software rows are `SDL_RENDER_DRIVER=software`), the original on origrun's real clock; five rounds, one game process
at a time on one pinned core, the machine 34 to 51% busy with other sessions' games (up to six game processes at a start).

| Idle board, share of one core (the last 20 s), median and range | Window | Renderer | Share | Less the audio thread | Draws a second |
|---|---|---|---|---|---|
| Original, 2D | 640 by 480 | DirectDraw (origrun) | 9.4% (8.9 to 10.3) | 9.4% | |
| Port, 2D | 640 by 480 (1x) | SDL software | **6.6%** (6.4 to 7.2) | 6.1% | 58.6 |
| Port, 2D | 1,280 by 960 (default 2x) | SDL software | 48.5% (43.3 to 49.7) | 47.8% | 58.2 |
| Port, 2D | 640 by 480 (1x) | Direct3D 11 | **7.5%** (6.0 to 15.4) | 6.9% | 58.2 |
| Port, 2D | 1,280 by 960 (default 2x) | Direct3D 11 | 8.5% (7.5 to 9.2) | 7.9% | 58.4 |
| Port, 3D | 1,024 by 768 (1x) | Direct3D 11 | **8.8%** (8.0 to 12.5) | 8.3% | 58.2 |
| Port, 3D | 2,048 by 1,536 (default 2x) | Direct3D 11 | 8.6% (6.7 to 11.1) | 8.0% | 58.4 |

The original in 2D read 9.4% here and 12.7% in the first table (the machine was quieter now); the original's 3D under dgVoodoo2 was not run again (61.3% on 2026-11-11, with
dgVoodoo2's own window, which was not looked at). At the original's own size the software path is under the original's idle in 2D (6.6% against 9.4%), the GPU paths
are at or under it, and the exact opaque test and the pace are what got the software path there (at 1x the 2x window's cost of stretching every sprite is not
paid). At the default 2x window the software renderer is the only path that pays for the size (48.5%): a machine that runs it has no GPU
renderer, and what would lower it (render scale 1 with a nearest-neighbour present, which changes the look: the prototype took the 2x window to 23.8%) is the
user's call. The logical `screenshot` of the software path at a 1x window is not byte for byte the 2x window's (the GPU path's is: the same hash in both); the screens
of the frozen baselines are the default window's, and the difference was not looked into.

**Where the processor time goes in 3D and on the GPU** (the same counters, the GPU run, 3,424 frames in 61 s): 243,522 quads a run, 0.14 µs each (the
recording of a draw call), 71 a frame; per-thread cycles of the idle run: the main thread 66 to 69%, a second thread 25 to 27% (the Direct3D 11
driver's worker), the audio thread 4 to 5%. The idle board's 8.6 to 9.3% of a core is the frame's presentation (`ntdll` waits and the driver) and the
game's code is 0.14 ms an update, below 1%. In the first pass of this section (cycles of the whole 3D run without waiting: 6.2 G, ntdll 39%, the
exe 29%) the exe's share was the load's decoders: `Platform::DecodeImage`, `Quantize8Bit`, the JPEG, GIF and Vorbis decoders, `imagehlp.dll` 2.4%; the
port loads 3 to 5 times faster than the original even so.

**Where the memory goes** (the framework's `memreport`, in the commands file at the title, with the board up and at the end of the run). *3D*:
`privateAtStart` 30.8 MB (the libraries and the devices), 236 pictures at the title (38.7 MB of pixels and palettes, 17 of them with a texture,
10.2 MB estimated), 50 sounds (9.9 MB), one song's module file (0.4 MB); the process heap is 76 MB in 36,000 blocks; private bytes are 129 MB at
the title, 131 MB with the board up and 144.6 MB at the end of the run (the textures grow to an estimated 30 MB as the board draws). The biggest
pictures at the title are `Menu-Gadgets` and `GMAP` (1024 by 768, 3 MB each; `GMAP` is the galaxy map of the puzzle mode), `frame` 2.3 MB,
`firering` 1.6 MB and the font, hypergem and black-hole sheets 1.1 to 1.3 MB; the port loads the resource groups the original's manifest names. *2D*: `privateAtStart` 12.3 MB, 243
pictures at the title (16.2 MB, 3.3 MB of textures), private bytes 59.4 MB at the title and 100.7 MB with the board up (296 pictures, 33.2 MB of
pixels and 22.8 MB of textures with 111 of them having one): the pictures that were 8-bit pixels and a palette at the title are 32-bit pixels
with a texture once the board has drawn them (the framework's doing), and the original's private bytes rise by 3 MB at the same point (39.7 to 42.8 MB). Only two pictures are new at the board (the
backdrops, 1.2 MB each) besides the 4 MB the game makes itself. *The web build* (`nodemem.js` under Node, the 2D scenario through the page's
own wasm, `wasmrun.js game`): the same 6,423 updates and 643 draws and score 565, 3.2 s of processor time with the start-up, the wasm memory 45 MB
at the title and 93 MB with the board up (Node's own resident set peaks at 165 MB with its 33 MB and the game files' copy). Its frame time in a browser was not measured: the Browser pane may be hidden and runs no
`requestAnimationFrame` then, and a measurement in the user's own Chrome is the user's to ask for.

**Shared costs** (none of them is in this game's code; sent to the coordinator with the figures above):
1. *The 2D software path* (`DDInterface` with `mIs3D` false on SDL's software renderer): the three changes above; the first and third are done (`7413ba61`,
   `5eac4e4a`), which meets the target at the original's window size, and the second matters only for the default 2x window. The spec as sent: in the software fallback
   render at the logical size (the render scale forced to 1 where the renderer is software) and present with a nearest-neighbour copy at an integer
   ratio (linear only for a ratio that is not one); draw an image with no transparency as a copy; draw at most at the display's rate and skip a frame
   when nothing was marked dirty. Test with `games/bejeweled2/tools/profile/pairs.sh` (`KINDS="port-idle orig-idle"`) and `tools/check.sh --changed`.
2. *The load's decoders*, as in AstroPop and Typer Shark: `Quantize8Bit` and the GIF decoder, the JPEG and Vorbis decoders, and in 2D
   `CustomResourceManager::ShrinkImage` and the stretch copies that make the 640 by 480 pictures.
3. *`imagehlp.dll`*, 2.4% of the cycles of a 3D run without waiting, from a call site not found.
4. *The pixels the framework keeps after upload and the first draw's expansion* (3D: 128 MB at the title, of which the libraries and devices
   30.8 MB, the pictures 38.7 MB and the sounds 9.9 MB; 2D: 59 MB at the title and 101 MB with the board up, where the original holds 40 and 43).
5. *The commands file's poll* (a `stat` of the file on every pass of the main loop, about 340 k cycles a call): test-only, 1% of the recorded game's
   processor time here (0.1 s of 9.9), counted and taken off; made cheap on Windows by `fa73221e` (a kept handle, about 49 k cycles a call with the reads).

**To repeat.** `WAIT_S=60 python -I <main checkout>/tools/pincore.py -- bash games/bejeweled2/tools/profile/pairs.sh <out dir> 5` from the worktree root
(about 35 minutes; `KINDS="..."` runs only some of the kinds: `orig-idle port-idle port2dgpu-idle orig3d-idle port3d-idle`, the same with `-work` and
`-load`), then `python games/bejeweled2/tools/profile/summarize.py <out dir>/runs.txt`. A build with the draw and poll counters:
`python games/bejeweled2/tools/profile/statspatch.py apply`, build, run with `DRAWSTATS=1` (and `PERF_ARGS=-renderscale=1` for the logical-size render,
`PERF_SHOT=1` for a picture at update 5000), then `statspatch.py revert` and rebuild before any check. `PERF_STRETCH=idle`, `PERF_MODE=3d`,
`PERF_RENDER=gpu|software`, `PERF_CLOCK=real` and `PERF_MEM=1` are perf.py's own switches (its header). The web build's memory:
`node -r games/bejeweled2/tools/profile/nodemem.js games/bejeweled/tools/verify/wasmrun.js game build/web/bejeweled2.js -gamedir=<assets folder>
-userdir=<absolute folder with users/ and registry.ini> -commands=<file> -realinput=<empty file> -seed=909 -hold -fast`. Traps: a relative `-userdir`
(`u3`, `./u3/`) is not found by the port, though the same folder by its absolute path is, the port needs `-hold` to play a recording, and
`rundesk.exe` needs absolute paths. The raw samples and runs were deleted once these figures were written.

## The PopCap Game Collection version (2026-11-11; `docs/COORDINATION.md`, "Done means")

The port was made from the Steam copy (`WinBej2.exe`, the launcher with the game appended; the game carved out of it is
`re/bejeweled2/bin/Bejeweled2Game.exe`), so it is checked against the game's version on the PopCap Game Collection disc: the two
versions compared, and the port run on the Collection's files. The procedure is Bejeweled's (its GAME_NOTES section 23), Typer
Shark's (section 15) and AstroPop's (section 13); `tools/collection/` holds the tools and `run.sh`.

**Getting the files.** `unshield` (built for Bejeweled's check, `E:\retools\unshield-build`; the user allowed it on 2026-11-10) read the
disc image's cabinet, which was mounted read-only and dismounted afterwards. The group `Bejeweled2DeluxeComponent` came out: 874 files,
26 MB, in the main checkout's `re/_installers/Bejeweled2DeluxeCollection` (git-ignored). The copy has no launcher: `Bejeweled2.exe` is
the game itself, beside `bass.dll`, `vhwb.dat` (15 bytes, the Steam copy has the same file), `license.txt`, `readme.html` and the folders
`data`, `edata`, `images`, `music`, `properties`, `puzzles`, `sm_images` and `sounds`. The extraction names the puzzle folder
`Galaxy_One`; both exes spell the pack "Galaxy One" (the profile's default pack, `Profile.cpp`), so a copy of the game has the space and
`stage_assets.py` stages it so.

**The exe is another build, unlike Bejeweled's and Typer Shark's.** Set beside the carved game image (`tools/collection/cmpexe.py`), the
Collection's `Bejeweled2.exe` was linked on 21 January 2009 (the Steam game: 11 April 2007), is 2,880,808 bytes against 2,715,648
(the Steam image as carved), and has a bigger `.text` (2,400,256 raw bytes against 2,260,992) and `.rdata` (290,816 against 270,336); `.data`
and `.rsrc` are the same size. None of the four is the Steam image's bytes with other addresses, 18 of the 20 resources are the same, and
5,416 bytes of signature follow the last section. Its strings (`tools/collection/textcmp.py`, runs of six or more printable
characters with four letters in them) say what changed: of the Steam image's 2,598, 2,586 are in the Collection's exe, and the 12 that are
not are build details (the PDB path `C:\GameSrc\cpp\WinBej2\Release\WinBej.pdb` and the source paths of the modifiable values, the
zlib banner, the C runtime's floating-point message, the profiler's format, the day and month table) and one error text: "Failed to create
high score file! Please report to support@popcap.com." The Collection's 80 others are the same kind of thing (`c:\p4root\cvs\GameSrc\...`,
its own runtime texts, the profiler's new format) plus a newer framework's: `SexyCacheData`, `SexyCacheReq#%d` and `SexyCacheMessage`
(a file mapping of decoded pictures), `ImageFont2:`, `DDImage:`, `ResMan`, the locale name, a second "Redraw FullScreen Flip" text, a
`CustomerSupportLocation` property with `http://www.popcap.com/support`, and the same high-score error told another way ("Please contact
Customer Suppport at"). The game's own texts are in the data folders (the port reads none from the exe), and `default.xml` is the same file
in both versions.

What is not known: whether the 2009 build plays exactly like the 2007 one. Its code differs throughout (another build of the framework),
so no function was compared, and the Collection's own exe was not run (the end of this section). What speaks for it: every text, picture,
sound, song and puzzle of the game is the Steam game's, with the six constants below as the one change of data that affects play.

**The files** (`tools/collection/cmpfiles.py`; the extraction's `Galaxy_One` is not the Steam copy's `Galaxy One`, so the puzzles were compared
apart, by hash, once renamed). Of the Steam copy's 791 files and the Collection's 874, 466 are the same byte for byte and 26 differ (the 21
pictures, the licence, the readme, `config.xml`, `partner.xml` and its signature, below); the 240 puzzle files the Collection has are the
Steam copy's, byte for byte. Besides those, the Steam copy has the launcher's files (`winbej2.exe`, `steam.dll`, `flash.ocx`,
`drm.swf`, `drm.xml`, `drmss.jpg`, `drm/`, 51 files), `puzzles/Galaxy One/sol.txt` (a note of the developers' tool, the solution of
tutorial puzzle 8; nothing reads it) and `properties/Backdrops.xml`; the Collection has 142 more: `bejeweled2.exe` and `sm_images/` (141 TGA
pictures of the interface that the exe never names). The differences:

- **`properties/Backdrops.xml` is missing.** Both exes name `backdrops.xml` (`WinDMApp::Init` 0x5e2a19 reads it, ignores whether the read
  worked and parses the buffer). The file gives each backdrop a `UIColorize` and a `GemColorize`, which `BackdropsParser` stores in
  `WinDMApp::mBackdropColors`, and the port, which translates every function the runs reach, never reads those colours back. With no file
  the parse of an empty buffer returns at once, as the original's loop does, and the game plays as before: every play below shows it.
- **`properties/partner.xml`.** The settings' registry key is `PopCap\Bejeweled2`, not `Steam\Bejeweled2`; the Collection's file has no
  `Width`, `Height`, `VistaCompliant`, partner name, display name, "more games" link or Steam id, and adds `ProdVersion` 1.1.3.2523,
  `ProdLanguage` and `InstallTag`. `partner.xml.sig` is another signature. The port reads four of the keys (`NoReg`, `ProdName`,
  `DefaultWindowed` and `RegistryKey`, `SexyApp.cpp` and `SexyAppBase.cpp`), and only the last differs, so its settings file has its
  section as `[SOFTWARE\PopCap\Bejeweled2]` on the Collection's files; the window's size is the game's own (640 by 480, or 1024 by 768 in 3D).
- **21 pictures** have other bytes (`tools/collection/cmpimages.py`: an encoder's other choices, a grey picture stored as RGB, a palette
  picture as RGB). 18 decode to the same pixels in every channel: the fonts `_continuumbold26`, `_halfmoon30`, `bombfont` and `scorefont1`,
  `_alphaback1`, `comet`, `dialogtitle`, `editbox`, `lightning`, `lightning_center`, `mapbuttonglow`, `nr_ringdude`, `portal_`,
  `star_sparkle`, `starfield2`, `toggle`, `tunnelend` and `tunnelend_`. Three differ: `loaderbar` (767 of 45,710 pixels, by up to 138 levels:
  the rim of the loading screen's bar), `slider-thumb` (97 of 961, up to 70, at the edges of the Options sliders' knob) and `holemask` (8,882
  of 16,384 by one level of alpha, an encoder's rounding). The loading screen draws only while the load takes frames, and the port's
  runs load without drawing, so `loaderbar` is the one picture no screenshot of this check shows; the Options screenshots (`opts_a`) show
  the slider's knob, and are the same on both sets, so the knob's differing pixels are not visible there.
- **`properties/config.xml`: six constants.** Everything else of it is the same. The Steam copy has `PowerGemChance` 1.0, `ExplosionChainBonus`
  1000, `ExplosionChainBonusAdd` 1000, `ElectNormalBonus` 400, `EndlessBonus` 1 and `EndlessBonusMult` 1; the Collection has 0.05, 40, 10, 20,
  500 and 150. They are what a gem caught in a blast and a gem a hyper cube strikes are worth (`BoardEffects.cpp`, `BoardMatch.cpp`: 1,000 and 400
  times the level's point value on the Steam files, 40 and 20 on the Collection's), the level's worth in Endless (`SetupLevelData`, `BoardLevel.cpp`:
  the points a level adds to the score before it ends, (level times `EndlessBonusMult` plus `EndlessBonus`) times the point value of a gem)
  and the chance that a gem dealt in Finity is a power gem. The game's own defaults for the three it names in code (10, 40 and 20) are
  neither file's, apart from the 20 (the Collection's 40 and 10 are the first two swapped). So the two versions' games score differently: in Classic the bot's game first differs at a blast on
  level 2 (update 7,755), where the Steam files' game stands at 5,902 and the Collection's at 1,492. The port takes whichever file it is
  given, as the original does.

**The port on the Collection's files** (`tools/collection/run.sh`; `tools/stage_assets.py` stages a Collection folder as it does a Steam
one: it takes `Bejeweled2.exe` as well as `WinBej2.exe` and renames the puzzle folder). `portshots.sh` takes `PORT_EXE` and `REGKEY`
for the second set's copy of the exe and its settings section. The staging was run in two sets: the Collection's files as they are,
and the Collection's files with the Steam copy's `config.xml` (the one change of data that alters play), each with a copy of the exe in
a folder of its own, so a result that differs belongs to the six constants alone.
- **The nine modes' bot games** of `tools/check.sh bejeweled2` (`tools/playbymove`, seed 1; Classic to 1,200 steps, the others to 300 to
  500), whose transcripts and end screens that check holds as hashes for the Steam files: with the Steam `config.xml`, all nine are the
  same, transcript and end screen, on the Collection's files. With the Collection's own `config.xml`, six are the same (Action, Puzzle,
  Twilight, Hyper, Cognito and Hidden, which use none of the six constants) and three differ, the three the constants reach: Classic
  (first at the blast of update 7,755: 5,902 against 1,492), Endless (at update 800 the Steam files' game has finished its first level,
  whose worth is a few points, and the Collection's is still playing it) and Finity (the power gems it deals). Each is the game the constants make; no run showed an error, a
  missing-file line or a crash.
- **Screens**, played from the coverage scripts (`tools/coverage/scripts`: `menu_a`, the title and the main menu with Quit asked and the
  scores' reset question; `opts_a`, the Options dialog with each of its controls and its questions; `users_a`, the players' dialog: create,
  rename, delete; `title_q`, the window closed at the title; `game_a`, a Classic board with the hint, every cheat word, the focus lost and regained, the
  in-game options and the window closed in the middle of the game, two shots) once on each set of files, from the same
  profile, seed and input, compared by shotdiff (`bejeweled2-coll`, no crop: both sets are the port's own 640 by 480 pictures): 31
  pictures (`menu_a` 3, `opts_a` 15, `users_a` 10, `title_q` 1, `game_a` 2) on each of the two Collection sets are equal to the Steam
  files' pixel for pixel (shotdiff at tolerance 0: all matching, none faint), as are the nine end screens of the bot games above (the six
  that the six constants do not reach, on the Collection's own).
- **The web build under Node** (`wasmrun.js`, the page's code, no canvas): `menu_a` and `opts_a` (18 pictures), played on the Steam files
  and on the Collection's. The web build's pictures on the Collection's files are equal, pixel for pixel, to the web build's own on the
  Steam files, and each set is a few levels of colour off the native port's on the same files, at most 3 and 4 of 255 in faint boxes
  only (the web build's usual difference in 2D: the numbers are the same on both sets).
- **The importer** (`games/bejeweled2/web/page.js`: markers `properties/resources.xml` and `music/beyondnetwork.mo3`, the folders it
  takes): an ad-hoc run of `web/gameimport.js`'s rules on the Collection's folder as extracted and on its staging found the game root
  and picked the same 727 files (`data` 52, `edata` 8, `images` 371, `music` 1, `properties` 5, `puzzles` 240, `sounds` 50) with the
  markers present; `sm_images`, the exe and the licence are left out as the Steam copy's launcher files are. (`tools/gameimporttest.js`
  registers only the games the site lists as finished, so it has no Bejeweled 2 line.)

**Not done.** The Collection's own exe was not run, so none of its screens or sounds was compared with the port's. It is a 2009 build
whose code differs from the Steam image's, and a run needs the watches of `origcmp` (the board's score, level and state, the generator),
which are addresses of the Steam image: a run of the Collection's game means finding them in its exe first, by pairing its functions
with the export (strings and calls; the exe carries RTTI for the framework's classes, the Steam image's does too). The definition of done
asks that the port work with the Collection's files, which it does, so this is left as AstroPop's was (its section 13). What it would
show is whether the 2009 build's code reads the six constants as the Steam image does. Also not done: a save of a Collection copy (no
Collection copy of the game was installed here, so where it keeps its players is not known: `platform/saveimport.json`'s entry for
Bejeweled 2 covers the Steam copy's folder and its registry key `Steam\Bejeweled2` only, and the Collection's key is `PopCap\Bejeweled2`),
and the Collection's drawing in 3D (the game under dgVoodoo2 was never run from the Collection's exe).

## The readability pass (2026-11-11; `docs/CODE_STANDARDS.md`)

**Result.** `tools/namecheck.py games/bejeweled2` reports no finding in 75 files (1,119 before): no member, local, parameter or global is
called by an offset or by a decompiler name, every function says which original function it is or that it is the port's (a `// port:` line),
and the numbers that stood for a state, a dialog or a face are named. The pass, in order (commits `e6dcb13d` and `9df11278`, then the one
that names the numbers):

1. *Helpers.* 29 functions that exist only in the port got a `// port:` line saying why.
2. *Dead members.* After the unneeded-code pass, every member that nothing reads was found by a script (a use that is not the left side of a
   plain one-line assignment counts as a read; `x++` and `--x` count as stores) and read against the export to see that the original does not
   read it either. 26 members went with their stores: the Board's `m165b4`, `m176c`, `m1750` (and the empty `if` of `Draw` that tested it),
   `m15e90`, `m15ed8`, `m15f50`, `m15f6a`, `m15f70`, `m15fd8`, `m16020`, `m15f40` (a widget pointer nothing sets, with its `delete`), `m15f80`
   (its `Rand() % 0x80` draw stays as `Rand();`: the order of the draws is the original's), `m1914`, `m195c`, the `m1960` group (six numbers
   and the unreachable blocks that read them), `m15ebc` (only the removed `NewGame` set it, so the end of the game always shows the high
   scores) and `m1724` (the debug overlay of `Board::Draw` and its early branch in `UpdateWhirlpool`); the small classes' `mUnused` members of
   `TitleScreen`, `MainMenu`, `HighScoreWidget`, `PodDialog` and `EffectOverlay::mB4`. In the end screen the state 7 block (a second fade that
   nothing in the class sets) went too. A removal moves every offset after it, so objsnap cannot prove it: the proof is `tools/check.sh
   bejeweled2` (the bot's games in the nine modes, state compared with the frozen baseline), run after each batch, identical every time.
3. *The names.* 136 members got names for what they hold, in `Board`, `Piece`, `EffectOverlay` and `Profile`, each declared on a line of its own
   with its original offset (`// +0x17c ...`) and a comment; the old hex names in these notes were renamed with them. One-letter and `aTemp`
   locals of nine functions were renamed inside their blocks, and `ShrinksAt640` became `ShrinksInSmallWindow`. Proof: every object of the
   build compiles to the same code before and after (`tools/objsnap.py cmp`: 48 objects, 889 functions, 0 differ), the string and character
   literals of the 39 changed files are the same (a lexer over `git show HEAD:` against the working copy), the native and the web build pass.
   Only `mB4` was a hex name that two classes shared, and the member of `EffectOverlay` was dead, so no rename needed the compiler's help.
4. *Numbers with a meaning* (rule 7). 82 numbers compared with or assigned to the board's `mState` and `mScreenState` became the names of
   the `Board` enums that `mState` and `mScreenState` already had (`STATE_GAME_OVER`, `STATE_LEVEL_COMPLETE_SCORE` and the rest of the 24
   states; the screen's `SCREEN_SLIDING_IN` to `SCREEN_HIDDEN`; `STATE_7` and `STATE_9` of the enum are `STATE_GAME_OVER` and `STATE_LEVEL_COMPLETE_SCORE` now); 25 dialog ids became the names
   of `WinDMApp`'s `DialogId` (the galaxy map's "Planet is Locked", id 13, is `DIALOG_PLANET_LOCKED`; the original's instructions and colour
   depth messages shared that id and are gone), and the 20 numbers of the end
   screen's `mFaceState` became `EffectOverlay::FaceState` (`FACE_BARS` to `FACE_END`; 7 is the state the original never sets). The
   object code of these two batches is the same (objsnap 0 differ) except `EffectOverlay`'s `DoUpdate`, `Draw` and `InitFace`, which lost the
   state 7 block (and so a constant or two) and compare `!= FACE_OFF` where they compared `> 0`; the baseline run proves that.

**Kept although the original never sets them.** `mFreeMode` (+0x1723: a swap that makes no match is not swapped back), `mSkipBestRecords`
(+0x15ee0: the cascade and combo records are not kept) and `mFadingSound` (+0x1728: the sound that fades with the level change) are read
by the game's code and never set to anything but their start value, by the original and by the port; the branches they guard are the
original's and stay. `mNeverSetScoreMark` (+0x15ee4) and `mSavedNeverSet` (+0x15fa8) are only written into the saved game and read back
from it; the save's layout is the original's, so they stay as the save's fields, named for what the save does with them. `WhirlPoint::mRHW` is the reciprocal homogeneous w of a vertex
of the mesh the whirlpool turns (a raw vertex of 0x24 bytes); `PuzzleStateOverlay::mPodOver` is stored by the original's own setter
(0x5d1995, `SetPodOver`) and read by nothing. `tools/splitfields.py bejeweled2` lists exactly these (and `mStartCountdown`, which the update
decrements and tests).

**Names that say what the code does and are worth a second look when a screen is read against the original again:** `mHyperRole`,
`mHyperDelay` and `mHyperFade` (the three numbers of a gem that a hyper cube takes: read in `UpdateHyperCube`), `mTutorialMoveFade` and
`mTutorialNextFade`, `mPodArrowShown` and `mPodArrowPhase`, `mHintButtonArrowPhase`, `mFlameFromCross`, `mBigMoveTimer` and `mBigMoveGlow`,
`mBoardInOverlay`, `mWarpFade`, `mDrainGrace`, `mLastGoodTick`, `mCheerGiven`, `mCheerTier` and `mCheerDelay`, and `mTextSizeFactor` of the
overlay.
