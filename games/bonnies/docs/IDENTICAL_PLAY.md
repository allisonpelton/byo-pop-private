# Bonnie's Bookstore Deluxe: the identical-play pass

This is the report on the pass that compared the port with the original game, `BonniesBookstore.exe` (staged as
`BonniesGame.exe`), screen by screen, natively and in the web build, in October and November 2026. It says what was
compared and how, every change the pass made, the oddities it left alone and the deviations that remain.

## How the two were compared

The original ran under `tools/origrun` inside Windows Sandbox (`docs/ORIGRUN.md`), invisibly under rundesk, from a copy
staged in the run's own folder with the saves the installer ships. Its scripts are in `tools/origrun/`: `tour.txt`
plays the port's scripted tour (`tools/tour_test.txt`) by the clock, from the logos through a new player, the
difficulty, the story, the first board's tutorial and three words, the game's options, high scores, credits and the
three pages of help, to the main menu and its options; `action.txt` continues from the saves the tour leaves with a new
action game, one word, the window losing the focus (the game pauses) and getting it back; `resume.txt` loads a saved
classic game; `actend.txt` loads an action game saved with little time left, starts its clock and lets the time run
out, through the "time's up" message to the main menu. `tools/origrun/patches.txt` pins the original's random seed to
the port's `-seed=1` (a five-byte change in the copy's `seedRandom`, 0x41f4e0).

The port's screenshots come from the same scripts run with `-fast -seed=1`, and `tools/shotdiff` paired the two sides'
pictures and found the pixels that differ. `games/bonnies/shotdiff.json` masks the animations that run on the game's
own clock, each with its reason: the logos' fade, the swinging sign and Bonnie's waving hand on the menus, the Submit
button's colour cycle, help 3's demonstration tiles, and the board's letters in runs played by the clock. The board's
letters cannot match there because the sparkles draw from `rand` every frame, so the board depends on how many frames
ran before it, which a run by the clock cannot fix; the board was therefore compared on saved games both sides load,
where nothing is masked.

The result: every screen of the tour matches (16 of 19 pixel for pixel, the other three only faintly: the text
caret's blink, a tile's light in help 3 and two pixels at the edge of the waving hand). The loaded classic game, the
resume dialog, the action game, its pause and resumption on a focus loss, the loaded action game over 100 seconds, the
"time's up" message and the main menu after it all match, with nothing masked but the clock animations above.

Sound was compared by the code rather than by ear. The sandbox records what a game plays through DirectSound, but the
original's FMOD 3 created no DirectSound buffer there (the stand-in attached and saw nothing), so no recording of the
original exists. `tools/soundcount.py` instead counts, for every function of the game, the original's calls to the
sound system (samples, music, streams, pausing and resuming) in the Ghidra export and the port's calls to the same,
and the arguments at the differing sites were read by hand. All 36 functions that make sound match, apart from two
places where the original's compiler merged a call shared by several branches into one (one error sound for three
cases in 0x414400, one sound for two in 0x41a810); the port has the call in each branch, which plays the same sounds.
`tools/rngcount.py` does the same for the random draws: all 58 sites in 15 functions match.

The web build was compared with the native build, which matched the original: the whole tour was run on the web
build under Node (`games/bookwormadv2/tools/wasmrun.js`, which takes a native run's test switches) and its screens and
saves set against the native run's. The saves came out byte for byte the same (the same boards, words, scores and
random draws), and with the same masks all 23 screens match, 21 pixel for pixel and two only faintly. Under `-fast`
the web build's clock reads slightly differently from the native one's, so its clock animations stand at other
points; that is the test runs' clock, not the game. The tour's focus step was left out of that run, because under
`-fast` on the web a script's `focus 0` stalls the test clock (the game waits for a clock that only moves with the
framework); the focus was checked on the web in real time instead, on the loaded action game, and its paused and
resumed screens match the native build's exactly, whose own matched the original's.

## The changes the pass made

| Difference | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| The high-score screen lit its top row as the last game's score, though no game had ended yet | The last game's rank and table (0x473e38, 0x473e3c) start at -1 in the exe's data; the port started them at 0 | They start at -1, so no row is lit until a game has placed one | efb69cb |
| Every image and sample still cached at the end leaked (85 images and 12 samples in the tour) | The shutdown (0x42ade0) deletes the sound system and the graphics before it releases the resource cache, and the cache's free skips an image or sample whose system is gone | The cache is released first (listed in `DEVIATIONS.md`) | a89120e |
| Two identical runs saved different games | A tile's bounces (+0x718) are read by the refill (0x4079a0) and saved for all 81 cells (0x406770) before a tile has ever fallen there, so the original saves whatever its heap held | The game is made on zeroed memory: those reads give 0 and the saves repeat (`DEVIATIONS.md`) | a89120e |

The pass also added test hooks that change nothing in play: a script's `focus 0` and `focus 1` take the window's focus
away and give it back (277001c), and `-seed=N` seeds the game's `rand`.

It then added the standard test commands (`docs/TEST_COMMANDS.md`: `state`, `goto`, `level`, `unlock all`, `set`, `win`,
`lose`, `seed`, `move`), reachable only from a test, the bot's `move` among them (1083179e, 93628761). The registration
code came out, though none of it could be reached (the game always counted itself unregistered with no way to register:
the register panel, the key check, the pre-registered dialog and the stored "buy now" address), and so did the options
menu's case that opened a web page, which no widget used; `docs/DEVIATIONS.md`, "Loading and the registration", lists
them. Loading needed no change: the game loads its resources in one call behind a logo that a click skips.

## The user's faults from the browser (2026-11-07)

After the pass the user played the web build and reported two faults: the game did not fill in their name although
the site's player setting asked it to, and the soundtrack seemed missing, then "way too quiet". Both were checked
against the original, and the game was then played with real input only.

**The soundtrack.** The original's menu music was recorded in Windows Sandbox with origrun's `--sound wav`. The
sound system's start (0x41e760) tells FMOD 3 to use WinMM before Vista and lets it choose from Vista on, and FMOD's
choice there made no DirectSound buffer for origrun to record, so a test patch (`tools/origrun/dsound_patch.txt`, used
only for this recording) asks it for DirectSound;
`tools/origrun/music.txt` clicks through the logos and waits at the menu. The port's music was recorded natively under
rundesk through SDL's file-writing audio driver. Over the first 5, 10 and 20 seconds of the menu song the original was
5.3 times as loud as the port, while the PopCap logo's sound was equally loud in both, so only the music was off.
libopenmpt mixes an XM's channels at its sample pre-amp of 48 and FMOD 3 at the full 256, a factor of 5.33, so the
host now plays the song at 256/48 (`loadSong`); after the change the port's menu music is within 1% of the original's.
The music played on the web before as well, only at a fifth of its level.

**The player's name.** Bonnie's had no part in the site's player setting. It has one now (`src/SiteProfiles.cpp`, with
the game's side in `game/SitePlayers.cpp`): at the first menu the site's player is made or chosen under "create", the
new-player box starts with the site player's name under "fill", kept to the box's characters and twenty letters, the
players are reported to the page when they change, and the site's volumes set the options' two. Checked in the
Browser pane: under "create" the menu welcomed the site's player at once, and under "fill" the New Player box opened
with the name filtered as the game's own typing would ("Zoë-Q. Smith" became "ZoëQ Smith").

That check and the real-input play found four more faults, all fixed:

| Difference | Its cause | What the port does now |
|---|---|---|
| The players panel was always empty, natively and on the web, and at start the game never fell back to the first saved player | The host's listing of a folder (`listFiles`) looked the folder up with a function that finds files only | The host finds the folder by its name in any case |
| On the web a name with an accented letter was saved under an unreadable file name and listed wrongly | The game's text is Latin-1 and the browser's file system takes UTF-8 | On the web the host writes and lists saved games' names as UTF-8 |
| The players panel's Delete removed nothing (a fault of the original, which breaks play) | The original builds the path with a string it never sets (0x47a1ec) in place of ".gam" | The port removes the saved game; the site can delete a player the same way |
| On the web the options' Full Screen box was ticked while the game ran in a window | `Data/Portal.cfg` starts the game in full screen, which the web build ignores (a browser enters full screen only from a click) | On the web the option starts off |

**Real input.** The port's tour (`tools/tour_test.txt`) was turned into real-input steps (`tools/realplay_tour.txt`,
every wait twice the tour's) and played in the Browser pane at `?silent=1`, with requestAnimationFrame shimmed while
the pane was hidden, by mouse and key events sent to the page's canvas: a click is a move, then a press and a release,
and typing is a key down, a key press and a key up per letter. From launch through the logos, a new player's typed
name, the difficulty, the story, the tutorial's FROG and CAKE, the game's options, high scores, credits and help, the
main menu's options, an action game with TALE played, the focus lost and regained, to the end of its time and Quit,
the game did as it does under its script (the page starts without a fixed seed, so its boards were others, and the
action game ended with the same score, 20). Afterwards the options' Full Screen box and the players panel's New, Accept
and Delete were tried the same way.

**Real input natively.** Under rundesk the window is driven by posted window messages (`tools/realplay.py`). SDL 2.32
takes a posted mouse move, but the move also makes it ask Windows to report when the mouse leaves the window, and
Windows reports that at once, because the real cursor is somewhere else on rundesk's desktop. SDL then moves its mouse
to the real cursor, at the client's corner. So a press posted on its own lands in the corner, and a press held over a
few frames is dragged there. realplay.py posts each click's move, press and release back to back, as Bookworm
Adventures' `visrun.py` does, so SDL handles all three before the leave arrives. Posted keys need no such care, but
each key-down must come a moment before its key-up: SDL's TranslateMessage makes the character from the key-down and
queues it behind whatever is already posted, and the host joins a character to its key-down only when nothing comes
between them. With that, the menus, the name box, the difficulty, the story's pages and the dialogs all answer
natively. The board does not: it follows a held press from frame to frame, so posted clicks never take a tile. The
board was therefore played natively through the framework's live channel instead (below), which puts its clicks
into SDL's own event queue.

Natively, then, the window's own part was played with keys, buttons and the window's commands
(`tools/realplay_window.txt`), windowed at 1600x1200 on the 200% display. Alt+Enter, posted as a keyboard sends it
(Alt's own key-down first), switched to full screen at the menu and on the board of a classic game. The window filled
the 3840x2160 screen with the game pillarboxed, and a second Alt+Enter brought it back to 1600x1200 both times. The
window was minimized and restored on the board, and it came back in full screen as it left. A posted close ended the
game. The game kept running while minimized, as a test run does, since a test run always has the focus
(`docs/DEVIATIONS.md`, "Test runs"); the pause on a lost focus was checked in the browser.

**A whole classic game natively.** `tools/realmove.py` plays the game through the framework's `-realinput`, fed
live through `-commands` (`docs/REAL_INPUT.md`): every click, key and focus change goes into SDL's own event queue and
through the game's real input path, never through `-script`. It asks the port's own bot for each move with the test
command `move` (`game/GameScreen.cpp`, `testMove`: a tutorial balloon's button or its word, else the best word on the
board, else the game's hint) and sends back the clicks it returns; it answers dialogs with their first button that
neither leaves the game nor opens anything, clicks on through the story's pages, and presses Scramble when the latest
words fill no new cell (three words with three cells or fewer left, eight otherwise) or when the game refuses the same
word twice. On 2026-11-08 one run under rundesk, with `-fast`, went from launch through the menu's Classic Game, a
typed name, Normal, the story and the tutorial, to the end of the story: all 50 levels, 1199 words, 83 scrambles, and
the high scores at the end, in 36 minutes. On the first board the focus was lost and regained through the channel's
real SDL focus events: the game paused while away and went on when it came back. After the first level Alt+Enter (Alt
held, Return, Alt let go) switched to full screen, and levels 2 to 50 were played in it; the game saved
`ScreenMode=1` at its exit.

Two faults of the drivers' own were found on the way and fixed in the bot, not the game: a tutorial balloon's Continue
button can sit on another screen than the board, so `move` now looks at every screen, and a wild tile could be taken
as Q, which the game spells QU, so the bot no longer tries Q for a wild tile.

**A whole classic game in the browser.** `tools/paneplay.js` plays the page in the Browser pane at `?silent=1` (with
`tools/panetimers.js`, which keeps requestAnimationFrame and the timers going while the pane is hidden), by mouse events
sent to the canvas, each a move, a press and a release; it takes its moves from the same `move` command, through the
web build's `bonnies_test_command`, and scrambles by the same rule. It played the whole classic story from the new
player's name through the tutorial to the story's last page after level 50 ("The end, for now"), at a score of
12,581,085. It was not one unbroken session: the driver was fixed several times on the way (the scramble rule, the
waits after a word, the board's two-letter tiles), and the machine restarted once, so the page was reloaded and the
saved game resumed at the start of a level eight times (levels 9, 10, 15, 16, 17, 18 twice, and 24). A saved game
resumes at the start of its level in the original as well, so every level was still played whole on the page. The last
stretch, levels 24 to 50, was one run: 714 words and 57 scrambles.

**Full screen in the browser.** Not verified. The pane's own click on the page's Fullscreen button arrives as a
trusted click with the user activation live, and the page calls `stage.requestFullscreen()`, but the hidden pane never
answers: the promise was still pending seconds later and `document.fullscreenElement` stayed null (Alchemy saw the
pane reject the same call outright, Seven Seas saw the same silence). The game's own full screen was played natively
above, and the options' Full Screen box in the browser was tried with the tour's real input; the page's full screen
waits for a browser that grants it.

## Oddities left alone

None so far. The tile bounces above are the one place the original reads memory it never set; the port reads 0 there
rather than imitating a heap's leftovers, which no one can see.

## The deviations that remain

`DEVIATIONS.md` lists them all, none with an effect a player can see: the host in place of SDL 1.2, SDL_image and
FMOD 3 (the game's own loop, with the framework running inside its sleeps; the music through libopenmpt; the cursor),
the JPEG decoding (stb_image rounds about 1% of a picture's pixels 1 to 3 levels away from IJG libjpeg, shared by every
game on the platform), the window menu the original never had, one copy at a time, missing texts, full screen on the
web from the page's button, and the test runs' own behaviour.
