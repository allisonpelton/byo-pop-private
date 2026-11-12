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
  thread of the original runs in line. Board's image preparation (`m15f60` and the like) is in line too.
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
- **`m1728` is never set** (the original's code never does either; kept only for the offsets).
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
`mSolutionMove` (+0x18fc), `m191c` (the port's `mHintTutorialShown` was never set) and `mIsPlanetIntro` (+0x191d, which the
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
- `Board::MouseUp` (0x59596f, vtable slot 56) was not translated: the board's "button is down" flag (`m15f38`) stayed set after the first press
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
text picture (the leak fix above). The rest, by why no run reaches it. None is removed now: they are the candidates of the phase that takes out
code the game does not need.
- **Dead in the original** (no caller, no vtable slot: 15): `GetColAt` 0x594c19, `GetRowAt` 0x594c42, `GetBoardSize` 0x5957b2, `GetOrMakePieceAt`
  0x59597c, `GetOldBoardHash` 0x5963ba, 0x5aef96 (`CurvedVal`), 0x5d3ffd and 0x5d8b5d (resource helpers), `DoPuzzleRating` 0x5dc25b,
  `ShowInstructionsDialog` 0x5df3d7, `ShowFullInstructionsDialog` 0x5df4d9, `ShowColorDepthDialog` 0x5df614, `ConfirmNewGame` 0x5e119e, `Board::NewGame`
  0x5a50a0 (only the dead dialog's button calls it) and `UpdateLevelTransition` 0x5a510a (no code sets board state 11).
- **The high-score name dialog** (8: `HighScoreNameDialog`, 0x5d31e4-0x5d34c8 and 0x5d3382): its only creator, 0x5dcdf0, has no callers; the score table takes
  the profile's name (`HighScoreWidget::NameEntered`, reached in the Action run).
- **The puzzle pack selector** (10: `PuzzlePackSelector` 0x5d0f09-0x5d13ae, `DoPuzzlePackSelector` 0x5dc1f6, `FinishPuzzlePackSelector` 0x5e15ea): shown only
  when `puzzles/` holds more than one pack folder; the shipped game has one (Galaxy One).
- **Error paths** (3: `BackdropsParser::Fail` 0x593732, `GalaxyParser::Fail` 0x5be56d and 0x5be07b): only a malformed or missing `backdrops.xml` or `galaxy.xml`.
- **The galaxy's finale** (6: `InitFace` 0x5b9071, the shape interpolation 0x5b592c and 0x5b5960, `powf` 0x5b5229, `GetPuzzleNum` 0x5d0ee6, `~EffectOverlay`
  0x5b8cee): the last planet of the puzzle galaxy; `DoHyperspace` sets `m191c` when the planet index is the last, and a profile with the whole galaxy
  open and a warp to it would reach them (a puzzle driver with the test commands; not done). `~EffectOverlay` never runs in the original.
- **`GalaxyMap` slot 47** (0x5ba705, a bare return: the map is never the focus widget).
- **Rare states and inputs** (9): `UpdateBoardDropping` 0x59814e (Action, no move left), `ShowNow` 0x59828a (a new game with no main menu around),
  `ShowPlanetCount` 0x59de98 (a planet starting with nothing solved and a pod chosen), `UndoAll` 0x59bb11 (the secret puzzles' "Reset Puzzle"),
  `UndoUntilSolvable` 0x5a96f9 (a puzzle made unsolvable, then HINT), `FinishPuzzleRatingDialog` 0x5e1a45, `SwitchScreenMode` 0x5e0b03 (Alt+Enter and
  the full-screen switch: a run that changes the window, kept for the overnight window), `DrawMouseover` 0x5c8afd (the original's hover run, `menu_d`,
  reaches `PodButton::MouseEnter` and `SelectMode` but never draws the glow; `Update`'s glow loop and `Draw`'s call read the same as the port's, and the
  port draws it; an oddity, not chased) and `WinDMApp::ButtonPress` 0x5dc1cf (the pod dialogs' own `ButtonPress` plays the click and does not call it).
- Not done, for later: the full-screen toggle run, the finale, the secret puzzle's reset, a puzzle made unsolvable. The Collection check (the PopCap Game
  Collection's Bejeweled2DeluxeComponent) and the site features come after the performance pass.
