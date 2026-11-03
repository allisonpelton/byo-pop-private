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

## Oddities left alone

None so far. The tile bounces above are the one place the original reads memory it never set; the port reads 0 there
rather than imitating a heap's leftovers, which no one can see.

## The deviations that remain

`DEVIATIONS.md` lists them all, none with an effect a player can see: the host in place of SDL 1.2, SDL_image and
FMOD 3 (the game's own loop, with the framework running inside its sleeps; the music through libopenmpt; the cursor),
the JPEG decoding (stb_image rounds about 1% of a picture's pixels 1 to 3 levels away from IJG libjpeg, shared by every
game on the platform), the window menu the original never had, one copy at a time, missing texts, full screen on the
web from the page's button, and the test runs' own behaviour.
