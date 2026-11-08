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
- **Pictures.** SDL_image's PNG and JPEG decoding is stb_image's (the platform's, for every game). The PNGs come out
  the same. The 95 JPEGs do not: SDL_image used IJG libjpeg, and stb_image's inverse DCT and upsampling round
  differently, so about 1% of a picture's pixels are 1 to 3 levels apart (measured on 2026-11-01 against libjpeg-turbo,
  whose default decoding is libjpeg's; every one of the 95 differs, by 2 levels at most in 6 and 3 in 89). No one can
  see 3 levels; origcmp's comparisons stay within them. A libjpeg-exact decoder would be a change to the shared
  platform, for every game with JPEGs.
- **Sound.** FMOD's samples and channels are the platform mixer's sounds and voices, with FMOD's 64 channels, its 0-255
  volumes and its frequencies. The music (`Sound/music.oxm`, an XM with Ogg Vorbis samples) is played by libopenmpt in
  place of FMOD's module player. libopenmpt mixes an XM's channels at its sample pre-amp of 48, FMOD 3 at the full 256,
  so the host plays the song 256/48 times as loud (`loadSong`): the original's menu music, recorded through origrun on
  2026-11-07, was 5.3 times as loud as the port's before, and is now within 1% of it, while the sounds already matched.
- **Players' file names on the web.** The game's text is Latin-1, as the original's ANSI file names are; the browser's
  file system takes UTF-8, so on the web a saved game's name is written and listed as UTF-8 (`FileName`, `GameText`).
  Natively the bytes go as they are.
- **The cursor.** SDL 1.2's 32x32 black-and-white cursors become the framework's image cursor, drawn with the same
  pixels and the same hot spot.
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
  empty, and message boxes take their first button without being shown. The window is never moved, minimized or
  given clipboard text. `-seed=N` seeds `rand` with N instead of the time (`seedRandom`, 0x41f4e0), so a script's
  boards repeat. A script's `focus 0` and `focus 1` take the focus away and give it back, as a click on another
  window would; while it is away in a `-fast` run, the game's wait for its next frame (0x420770), which pumps without
  sleeping, runs the framework once a pump, since that run's clock moves only with the framework.

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
reads out of the exe (`exeFont`; the original's are static data) and the host's cursor image.

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
