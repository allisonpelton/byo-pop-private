# Bonnie's Bookstore Deluxe: where the port differs from the original

The port is the original game, unmodified. This list holds every place where its code does something other than what
the game in `BonniesBookstore.exe` does, and why. Only the faults fixed at the end have an effect a player can see.
Each place in the code names the original's address, so a grep for the address finds both sides.

## The host

The original is an SDL 1.2 program with SDL_image and FMOD 3 that also calls Windows directly. The port calls
`bonnies::Host` (`engine/Host.h`) wherever the original called one of those, and `src/BonniesHost.cpp` implements it
on the framework (`ENGINE.md`, "The port"). What follows from that:

- **The loop.** The game keeps its own loops. The framework runs inside the game's sleeps, so the window, the clock and
  the sound keep going exactly as long as the original's `Sleep` would have waited. On the web the sleep yields to the
  browser.
- **Pictures.** The PNGs are decoded by stb_image and the JPEGs by IJG libjpeg 6b (the platform's
  `LoadImageFromMemory`), as SDL_image decodes them in the original, so both come out pixel for pixel the same.
- **Sound.** FMOD's samples and channels are the platform mixer's sounds and voices, with FMOD's 64 channels, its 0-255
  volumes and its frequencies. The music (`Sound/music.oxm`, an XM with Ogg Vorbis samples) is played by libopenmpt in
  place of FMOD's module player. libopenmpt mixes an XM's channels at its sample pre-amp of 48, FMOD 3 at the full 256,
  so the host plays the song 256/48 times as loud (`loadSong`, the shared `BYOPOP_FMOD3_XM_GAIN`): the original's menu music, recorded through origrun on
  2026-11-07, was 5.3 times as loud as the port's before, and is now within 1% of it, while the sounds already matched.
- **Players' file names on the web.** The game's text is Latin-1, as the original's ANSI file names are; the browser's
  file system takes UTF-8, so on the web a saved game's name is written and listed as UTF-8 (`FileName`, `GameText`).
  Natively the bytes go as they are.
- **The cursor.** SDL 1.2's 32x32 black-and-white cursors become an SDL 2 window cursor on the desktop
  (`SDL_CreateCursor`, the same pixels and hot spot, as the original's SDL 1.2 makes a system cursor), and the framework's
  image cursor, drawn with the same pixels and hot spot, on the web. An image cursor keeps the shared display out of its
  surface mode, so the desktop does not use one.
- **The screen.** The game draws into one 800x600 buffer and presents about 30 times a second, whether or not anything
  moved. On the desktop the host compares the buffer with the last frame it showed and hands the shared `SurfaceDisplay`
  (`framework/SurfaceDisplay.h`) the rectangles that differ, none at all when nothing did: the display shows them in the
  window's own surface when the window is the screen's size or a whole multiple of it, and through the renderer
  otherwise (`BonniesWidget::SendScreenChanges`). On the web the host copies the buffer into a framework picture
  (opaque, uploaded as it is) when its pixels differ from the last copy, and when they do not it draws nothing
  (`BonniesWidget::Update`). Two in three presents of the tutorial's board matched the one before. Nothing visible
  changes (the frozen screens are unchanged, and `SURFACE_VERIFY=1` compares the window's surface with the buffer after
  every present), and the game's own drawing, its rects and its frame timing are untouched.
- **Messages and the window menu.** The window never had a menu (its handle, 0x4809c4, is never set), and the window
  messages SDL passed on (0x420630) only served that menu, so the host sends none. Refreshing the window frame for the
  menu (0x41fb20) does nothing.
- **One copy at a time.** The check for another running copy (0x41ffc0) always finds none. A copy runs in its own
  browser tab or window, and the user folder is its own.
- **Missing texts.** Asked for a text id it does not have, the original builds a "not found" message and still returns
  an empty string (0x428f70); the port returns the empty string without building the message. The game asks only for
  ids it has.
- **Full screen on the web.** `Data/Portal.cfg` can start the game in full screen. A browser enters full screen only
  from a click, so on the web the game's request is ignored and the page's button does it. The options' Full Screen
  box starts off there (`bonniesMain`, 0x403b80), so it says what the game does.
- **The site's players and volumes (web only).** The site's player setting reaches the game at its first menu
  (`MenuScreen::init`), where under "create" the site's player is made or chosen as the new-player box and the players
  panel do it; under "fill" the new-player box starts with the site player's name, kept to the box's own characters and
  length. The site's music and sound volumes set the options' two (`src/SiteProfiles.cpp`, `game/SitePlayers.cpp`).
  Under the setting's default nothing changes.
- **Test runs.** These differences only appear under rundesk or a script (`IsTestRun`), never in play. The game always
  has the input focus, as the user's window would. The desktop is reported as 1920x1080, the player's Windows name is
  empty. The window is never moved, minimized or
  given clipboard text. `-seed=N` seeds `rand` with N instead of the time (`seedRandom`, 0x41f4e0), so a script's
  boards repeat. A script's `focus 0` and `focus 1` take the focus away and give it back, as a click on another
  window would; while it is away in a `-fast` run, the game's wait for its next frame (0x420770), which pumps without
  sleeping, runs the framework once a pump, since that run's clock moves only with the framework.
- **Real input natively.** In a `-realinput` run (`docs/REAL_INPUT.md`) the host gives the game only the focus and the
  mouse the run's own events give (`getAppState`), reads the cursor in full screen from those events instead of the real
  one (`getCursorPos`), and, with `-fast`, runs the framework at every pump while the game has lost the focus, as it
  already did for a script's focus loss, so that the clock moves and the next command comes. A run without
  `-realinput` is as before.
- **The test state.** For playing with real input only, the port reads its state out without changing it
  (`testState` and `testWord`, `game/GameScreen.cpp`): the buttons on screen, the level, score, turns and phase, the
  tutorial's balloon and the tiles it marks, the hint the game shows, and a word on the board found with the game's
  dictionary (no random number drawn; the dictionary's last match is put back). Natively the host writes it to the file
  `BONNIES_STATE` names, ten times a second and only in a test run, for `tools/realplay.py`; on the web the page has it
  through `bonnies_test_state`, for `tools/paneplay.js`. Neither does anything until a test asks.

## Loading and the registration

**Loading.** Bonnie's needs no change for fast loading (the user, 2026-11-07). The logo screen loads every resource in
one call on its first update (`BonniesApp::loadResources`, from `LogoScreen::update`), and the flags `mLoadImages`,
left from a loader that worked in stages, are all set on the first update, so nothing waits on them. What remains is
the logo animation, which a click or key hurries (`gLogoSkip`) and which holds no loading back.

**The registration, taken out** (the user, 2026-11-07). The port plays as the registered original does. The copy's
registration never ran: no menu button carries its id (0x133), the check of a key took any key, and `isRegistered`
answered no. These were taken out, with no visible change, since none of it could be reached:
the registration panel (`RegisterPanel`, 0x4197c0 and its vtable 0x458844, with its key check 0x42a480, its
buttons and text fields, and the flag `gRegistering`); the menu's handling of id 0x133, which opened the panel or
said the copy was registered; `isRegistered` (0x42a4d0) and the menu's call of it, whose answer nothing used; the
dialog's "registration reminder" layout (`gPreRegister`, bit 0 of `gameDialog`'s last argument), which no caller asks
for; and Portal.cfg's buy page address (`BuyNowURL`), which only the panel used. Portal.cfg's reader still reads past
that key, so the keys after it parse as before. The suite's frozen screens are unchanged.

**Ways out of the game, taken out** (the user, 2026-11-07). Bonnie's has no "More Games" button. The options menu's
handler had a case, id 0x25e, that opened the game's web site, but no button carries that id, so it was taken out with
no visible change. The registration's web site button went with the panel, above.

## Test commands

The standard test commands (`docs/TEST_COMMANDS.md`) are the game's, in `game/GameScreen.cpp` (`testCommand`), and
reach it through the host's `HandleScriptCommand`: from `-script` natively, and in the web build from the page
through `bonnies_test_command` (`src/BonniesHost.cpp`, `Module.ccall('bonnies_test_command', 'string', ['string'],
['state'])`), which returns the answer line; the live command channel takes them once there is one. Each answers with one line on stdout. They share their reading of the game with the test state above,
and a command never given changes nothing (the suite's frozen screens are unchanged). The game keeps its own loops and
the framework runs inside its sleeps, so a command only changes what the game's own code then acts on.

`state` prints `STATE {...}` with `screen` (title, menu, game, help, story, credits, highscores), `modal` (the depth
of open dialogs) and `paused`, and in a game also `mode` (classic or action), `level` (1 to 50), `score`, `phase`
(1 in play, 2 and on the steps after a won level), `moves` (the classic mode's turns) or `timer` (the action mode's
time bar, 0 to 1), `filled` and `fill` (the cells filled and the number the chapter needs), `board` (nine rows from the
top, a character a column: the tile's letter, `*` a star, `?` and `!` wild tiles, lower case a two-letter tile, `.` no
cell, a blank an empty cell), `fixed` (the cells of locked tiles, column*9+row) and `unfilled` (the cells still to fill,
the same way).

`goto title|menu|help|credits|highscores` asks for the screen as the game does (`setState`, which switches at the
next update). The options are a dialog, not a screen, and a mode starts only through the menu's player and
difficulty dialogs, so `goto options`, `goto classic` and `goto action` answer ERROR. `level <n>` (1 to 50, in play)
sets the level before it and ends that one as won, so the game goes on to level n as after any won level, through its
story page. `win` ends the level as won (`Game::levelWon`, submit's own ending, moved into a function of its own for
this). `lose` sets the turns (classic) or the time (action) to none, and the game's update ends the game as it does
then. `set score|moves|timer <value>` sets that key. `seed <n>` seeds `rand`, which every random number comes from.
`unlock all` answers ERROR: nothing in Bonnie's is locked.

`move` prints the move of the port's own bot, `MOVE {...}`, or `MOVE null` off the board or while a dialog is open. It
is a balloon's own button (`kind` button, its `id`), the word a tutorial balloon names over the tiles it marks (`kind`
tutorial), the best word the bot finds (`kind` word: up to ten tiles, by what it fills, the stars it brings down and
the turns it gives or costs, by the game's own dictionary check, with wild tiles as any letter but Q, since a wild tile
is one letter and Q is two), or the game's own hint when the bot has none (`kind` hint), with `word`, `cells` and
`input`: a click off the board (which clears a path left over), a click on each tile, then Submit, in game pixels. The
bot reads the game and changes nothing; its search is kept for a board it has searched before. Scramble, the story's
pages and the dialogs are left to the driver (`tools/paneplay.js`, `tools/realmove.py`).

`memreport [file]` is the framework's own (`SexyAppBase::DoMemReport`; the table with a file name), which asks the
port for what the game keeps beside the framework's pictures (`BonniesHostApp::AddMemReportHolders`): the resource
cache's images, the dictionary, the decoded samples and the screen buffer. It only reads.

## Memory the original leaks

The translation keeps the original's ownership: the same objects are made and freed at the same places. A leak tracker
(operator new replaced in a separate build, the blocks still held at exit listed by their callers) found one leak in
the tour of `tools/tour_test.txt`, which plays the classic and the action game, every options, help, credits and high
scores screen, and ends through the main menu's Quit. After the fix nothing the game allocated is still held at exit.

- **The resource cache at shutdown.** The platform's shutdown (0x42ade0) deletes the sound system and the graphics
  before it releases the resource cache, and the cache's free (0x421b30) skips an image when the graphics are gone and
  a sample when the sound is. So every image and sample still cached at the end leaked (85 images and 12 samples in
  the tour). The port releases the cache first, while both are still there.

Two port-side objects are freed at the end too, though the original had no such leak: the font descriptions the port
reads out of the exe (`exeFont`; the original's are static data) and the host's cursor (the window's SDL cursor on the
desktop, the image on the web).

Where the original handled memory by hand and the translation uses a C++ string or vector (the engine's string, 0x41d6c0,
is `std::string`), the tracker could not see a leak of the original's; such a leak would be gone in the port already.

## Memory the original reads before setting it

- **A tile's bounces.** The game's constructor (0x406ff0) sets only `mInProgress`, and a cell's `mBounces` (+0x718) is
  set only when a tile falls into it, by the refill (0x4079a0). The same refill reads it first for a '*' tile in the
  bottom row (the tile falls out only below 3), and the save (0x406770) writes all 81. So the original reads and saves whatever
  its heap held there. The port makes the game on zeroed memory (`Game::operator new`): those reads give 0, the saved
  games of two identical runs are the same, and a '*' tile that never fell drops out as one that has settled does. The
  tour's saves differed in exactly these 81 values before.

## Random draws

None. Every draw from `rand` is where the original makes it: `tools/rngcount.py` counts, for each function the port
names by address, the original's calls to `rand` (0x430366) and to `randomRange` (0x41f500) in the Ghidra export and
the port's calls to `crtRand` and `randomRange`. On 2026-11-01 all 58 sites in the 15 functions that draw matched, and
the seed is set at the one place the original sets it (0x41f4e0, from the platform's start). `rand` itself is the
CRT's (`crtRand`, `Str.cpp`), with its multiplier and increment.

## Faults of the original that break play

Fixed in the port itself (the user, 2026-11-07); the faults that only change how the game plays wait for the fixes mod.

- **Deleting a player.** The players panel's Delete (0x4189b0) builds the save's path with a string the original never
  sets (0x47a1ec) in place of ".gam", so it removes nothing and the player stays in the list. The port removes the
  saved game by the path `playerSaveExists` (0x4023a0) checks.
- **A line along y with no height.** `drawLine` (0x4275c0) divides by the line's height in pixels when it steps along
  y; the original divides by zero when the two ends round to the same row, which crashes it. The port returns there
  instead. Its only caller is the checkbox's tick (`CheckBox::draw`, 0x42d7a0), whose strokes are diagonal, so no play
  of either reaches it (found by the coverage check, `GAME_NOTES.md`).
