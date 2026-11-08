# Cosmic Bugs: identical play

## Phase one: plays identical (2026-11-03)

With the same input, a series of games now plays out the same in the original exe and in the port: the same random
numbers drawn at the same updates, the same score, lives and filled cells at every update, the same boards, levels
and endings. Three changes to the port were needed, and one of them also removes a run-to-run variation the original
has.

### How the games were compared

The original ran through origrun (`docs/ORIGRUN.md`) from a copy under `build/origrun/<run>/game`, invisibly under
rundesk, windowed, on origrun's virtual clock with its scripted cursor. Two test patches went into the copy, kept in
the git-ignored `tools/origrun/patches/cosmicbugs.txt`: the game seeds the C library's generator from the time at
start-up, which the first patch replaces by a seed of 0 (the runner's clock starts at 0, so both draw the same); the
second makes each frame wait exactly 10 ms of the virtual clock and then read its elapsed time, so that every frame
runs ten updates, as the port's runner does. With both, a 400-second game of the original runs in about a minute and
plays the same way every time it is run (but see the beams below). origrun watched the generator's seed (0x460a64),
the score (0x46e5e8), the lives (0x460040) and the board's filled cells (`[0x473138]+0x3b8`) at every frame, and
`--watch-max` (added for this) let it log every change of the seed over a whole game.

The port ran in its headless runner (`cosmicbugs_run`) with two options added for this: `CB_WATCH=1` prints the same
four values after each frame, in origrun's format, and `CB_NOWARP=1` ignores the game's cursor warps, as origrun's
scripted cursor does for the original's `SetCursorPos` (the ship's appearance warps the pointer to the middle every
update). A click the runner makes at T ms, with the pointer moved there 10 ms before, is the original's
`at T-19 click x y 30`: the original reads the cursor in the frame it is held at and the button at the next frame's
message pump. A right click and a key are mapped the same way, and a key is held 30 updates, as a click is
(`at N key vk 30`, added to origrun for this: a key pressed and let go within one update never reaches this game).

The scripts are in `games/cosmicbugs/test/plays` (`<name>.txt` for the runner, `<name>.orig.txt` for origrun), and
`games/cosmicbugs/tools/play` has what writes and runs them: `gen.py` writes a random play (a line every 1.5 to 3.5
seconds at a random place, the aim switched by a right click 40% of the time), `pair.sh <name>` plays it on both and
`cmp.py` compares the four watches' changes, value and update.

| Game | What it covers | Result |
|---|---|---|
| g1 | 200 s from a fresh start, the first boards | the same at every update (seed, score, lives, fill) |
| g2, g3, g4 | 400 s each from a fresh start: several boards finished, their bonuses and the next boards, lives lost | the same at every update, every seed change of the whole game included |
| a1 | Options, the shooter mode switched to automatic, Back, then 300 s of play | the same at every update |
| k1 | 300 s with the keys: Space to turn the ship, Enter to pause and again to go on, the '.' key (which ends the board), Esc to the title in the middle of a board (the game is saved), Start Game and the requester's OK to resume it, then more play | the same at every update as two of the original's three runs (see the beams below) |
| c7 | a second launch with a save from the original (board 4, one life left): Start Game, OK to resume, then play through the game's end | the same at every update |

The title left alone for 15 seconds and the suite's own scripted game (two lines on the first board) were compared
the same way and also matched. Both saves' contents matched except the player's name, which the original takes from
the Windows user name and the runner's host gives as GUEST.

`tools/check.sh cosmicbugs` now plays the series (all but c7, whose save comes from the original) on the runner and
checks the checksum of their watch lines against `build/baseline-cosmicbugs/`, a copy of the runner frozen at
commit 1d7e5e0 once the series matched the original. A change that alters play fails the suite; a deliberate one
refreezes the baseline and says why here.

### The changes

| Difference | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| The seed stepped differently whenever its top bit was set, so the series parted after a few hundred draws | `Rand` (0x420290) divides the seed with an unsigned `div` | divides unsigned | 085e7de |
| A rider (the spark that runs along a beam to the ship when an enemy touches the beam) was made one frame apart, and everything after differed | `Dist3` (0x420300) stores each difference and the sum of the squares as floats before the square root; the port worked them in double, and a rider is made at a distance just under its threshold (19.98 against 20 in one case) | stores them as floats | 085e7de |
| The same input played out differently from run to run, in the original and in the port, from one rider on | an enemy makes a rider on a beam it touches unless it is the beam it touched last (its pointer at +0x100), and nothing clears that pointer when the beam is freed. A later beam that the heap happens to put at the same address therefore makes no rider on that enemy, and Windows' heap picks among freed blocks at random | an enemy forgets a beam when the beam is deleted, which is what the original does whenever the address is not reused; every run of the port is now the same | 1d7e5e0 |

The runner's options are in 085e7de; the series, the scripts and the suite's baseline check are in 36676a0 and
c1eaab8.

Shared commits: 4cc3eec (`origrun --watch-max`, to log more than 4000 changes of a watch), 51a0a6e (`at N key vk k`
holds a key for k updates, and `keydown`/`keyup` post one edge) and c1eaab8 (the suite's series and baseline check).

### Oddities

**The original plays the same input differently from run to run.** This is the beam pointer above: whether a new beam
lands where an enemy's last one was is the heap's chance, so a rider is sometimes not made. To see it, run k1 on the
original three or four times (`bash games/cosmicbugs/tools/play/pair.sh k1` after copying `test/plays/k1.orig.txt` to
`build/origrun/k1.txt` and `k1.txt` to `build/cbw/`): one run in our three parted from the others at update 69541 and
joined them again some 160 frames later, and the other two parted from the first at 115061 for good. An early run of
g1 also parted at 30201, where three later runs did not. The port now always takes the outcome where the address is
not reused; reproducing the chance would take a model of the Windows heap, which is not worth it.

**The ship's open span is left uninitialised until the ship first moves.** The four floats at +0x4b0 hold whatever
the allocation held until the ship's first update in play (one run read 516 in the first). Nothing reads them before
then; the port starts them at 0.

### The user's report

The user saw yellow flickering on the title on a second launch, then numbers on the bottom panel's displays drawn
offset after resuming a game, staying so. It was not reproduced: the windowed exe launched twice in a row (the second
time with the first's save and settings, and with a save at board 4 from the original), the runner resuming the same
save and playing on, and the runner with every allocation filled with 0 and then 0xCD (to bring out any field left
uninitialised) all showed a steady title and the displays in place; the only difference the fills made was one pixel
of a particle in the board at one moment. The offset digits would fit a particle centring the handle of an image it
should not reach (particles call `Image_SetMidHandle` on every image they draw, and the displays' font images come
from the same art bank), but no run drew one. The report is most likely from the web build, so phase two's
comparison of the web build looks for it again, with a second launch in the same browser.

### Deviations left

None in play. The saves differ only in the player's name, which the site's player name hooks supply. The one pixel
that changed with the fill above means some particle field the original leaves uninitialised is still read; it is
for phase two's screen comparison.

## Phase two: looks identical (2026-11-04)

The original and the port now show the same screens, dialogs and texts. They play the same sounds and music at the
same updates, and they behave the same when the window loses and regains the focus. The web build draws the same frames
as the exe. Two changes to the port were needed. What is left is a handful of invisible one-step differences on the
soft edges of blended images, and the order in which overlapping sprites are drawn. Both are listed below.

### How the screens were compared

The original's shots were taken in Windows Sandbox through origrun (`--sandbox`, with the screen read from the game's
own surface by `--surface-screen`). The patches, seed and virtual clock were the same as in phase one.

On this machine the original is given a 5-5-5 screen, both windowed on the host and in the sandbox. origrun learned to
read a 15-bit surface for this (c36e00c). The port always draws 5-6-5, but the runner's `CB_555=1` makes it draw 5-5-5.

`CB_555` also repeats what the original does when it is handed its screen after loading. Its screen-lost callback
(0x41e6f0) converts the art again, makes every board's background again from the bank (0x40d1f0) and redraws. That
leaves the menus' images and logos blended onto the backgrounds once. The port never loses its screen, so it has
blended them twice. With `CB_555` the runner's shots can be compared pixel for pixel. Without it the difference is one
step on the soft edges of those images, which no one can see.

`games/cosmicbugs/tools/play/tour.py` turns a tour (in runner times) into a runner script and an origrun script, with
the timing of phase one. Three tours were shot and compared by `tools/shotdiff`, which flags any box whose pixels
differ:

| Tour | What it covers | Result |
|---|---|---|
| menus (`test/tours/menus.txt`, 47 shots) | the title and every button's hover, Instructions, Credits, High Scores, Options (both shooter modes, music off and on), Change User with a name typed, Start Game, a board with the ship turned, Enter to pause and again, Esc to the title, Start Game again | 35 identical, 12 one step on soft edges |
| g2 (100 shots, one every 4 s) | phase one's 400-second game: boards, bonuses, deaths, the next boards | 83 identical, 3 one step, 14 flagged (each explained below) |
| end (`test/tours/end.txt`, 10 shots) | phase one's c7 save played to its game over, the high-score list with a name typed, back to the title, the world screen | 8 identical, 2 one step |

Most of the 14 flagged g2 shots are the original's shot landing a frame or two late, because the sandbox's capture is
slower than the game on the virtual clock. Each of these equals the port's shot at the update it landed on: the
runner's re-shots at those updates were 3 of 3 identical. The rest differ only where two enemies overlap and are drawn
in the other order (see Oddities).

### Sound

All of the game's sound and music goes through BASS. `games/cosmicbugs/tools/bassproxy` is a logging stand-in for a
copy's `bass.dll`: it writes every BASS call with the game's update counter.

Under origrun, BASS cannot open a real device. It gets its DirectSound through COM, and `BASS_Init` fails on the host
as it does in the sandbox. After that failure the game loads no sounds. So the proxy gives BASS its "no sound" device,
on which the music still loads and plays, and answers the sample calls itself.

On the port's side, the runner's `CB_SOUNDLOG=1` prints the engine's sound and music calls. `tools/play/soundcmp.py`
compares the two logs by event and update. The events are loads, plays (by file), stopping all sounds, and the music's
load, play, pause, resume, free and volume.

| Run | Events | Result |
|---|---|---|
| g2 | 1492 | the same, update for update |
| the menu tour (Options' music off and on included) | 164 | the same, update for update |
| the end tour | 121 | the same, update for update |

The only difference is the shutdown's last four calls. They come ten updates later in the original, which is where
origrun ends the run. Before the fix, the end tour showed the original playing the electricity sound twice where the
port did not. That led to the second change below.

### Focus

origrun's `focus lose` and `focus regain` were compared with the runner's new `focus` events, on the title and in the
middle of g2. In both games, while the window is inactive, updating stops, the music pauses and the sounds stop. On
regaining the focus, the music resumes and play carries on.

After the mid-game loss, the rest of g2 is the same in both. Every seed change in the port's log has the original's
value at the same update. The score, lives, fill and all 1491 sound events are the same too.

### The web build

`tools/play/webtour.sh` plays a runner script three times, from the same save: on the runner, on the exe (`-fast`,
under rundesk) and on the web build under Node (`wasmrun.js`). It compares the exe's shots with the runner's and the
web build's with the exe's.

`fwscript.py` writes the exe's `-script` from the runner's script. A script command at update N runs after that
update's frame, so the runner's event at T ms is update ceil(T/10) - 1.

The game reads its buttons once an update. The framework's `rclick` presses and lets go within one update, so it never
reaches the game. The app's own script commands `rdown` and `rup` instead press and let go the right button where the
pointer is.

| Tour | exe against the runner | web against the exe |
|---|---|---|
| menus, 47 shots | all identical | all identical |
| g2, 100 shots | 98 identical; 2 with overlapping enemies drawn in the other order | 97 identical, 1 one step; 2 with overlapping enemies |
| end, 10 shots (a saved game resumed) | all identical | 9 identical, 1 one step |

The user's report from phase one did not show up. That report was a yellow flicker on the title on a second launch,
then the panel's digits drawn offset after resuming a game. The end tour is a second launch that resumes a saved game,
and its panel and title are the same on the web as natively.

### The changes

| Difference | Its cause in the original | What the port does now | Commit |
|---|---|---|---|
| Every text was one colour step too bright | `Palette_Gradient` (0x42bc10) converts each colour with `fistp` while the FPU's rounding control is set to chop, so it truncates; the port rounded | truncates | 006eb58 |
| The electricity sound (an enemy touching a beam) was sometimes not played | the enemy's constructor (0x404fc0) leaves the age of its last zap (+0xd0) as the heap had it, and the zap plays only once the enemy's age is 1000 past it. 0, what a fresh block holds, is what the original had in every run compared; the port's own leftover varied from run to run | starts it at 0 | 1ef5f12 |

Neither change alters play: `tools/check.sh cosmicbugs` still plays as the frozen baseline.

The tools are in two commits:
- 7923e44: `CB_555`, `tour.py` and the menu tour;
- b1cbade: the BASS proxy, `soundcmp.py`, the runner's sound log and focus events, the end tour, `webtour.sh`, and
  `rdown`/`rup`.

Shared: c36e00c (origrun reads a 15-bit surface).

### Oddities

**Overlapping sprites at the same depth are drawn in the order of their addresses.** The draw list (0x4211d0) sorts by
depth and then by a sequence number, which is the object's own address. So where two enemies of the same depth
overlap, the heap decides which is on top. The order differs between the original and the port, and between the
port's own builds. To see it, compare g2's shots at 112 s and 212 s. Matching it would take the original's heap.

**The menus after a game, and the Change User frame's edge, are one step off on soft edges.** The title seen after
returning from a game (the menu tour's `esc_title`, the end tour's `title_after`) and the frame around Change User have
had their images blended onto the background a different number of times than in the port, even with `CB_555`. The
difference is one step of a colour channel on the antialiased edges. It was not tracked down after more than an hour.

**Nine pixels of the top row.** The original keeps nine faint pixels in the screen's top row that the port never draws
there. One step or two, invisible.

### Deviations left

The port draws on a 5-6-5 screen, where the original on this machine gets 5-5-5. The port also never loses its screen,
so the menus' images keep their second blend: at most one step on soft edges, invisible.

The web build under Node can draw rotated and scaled sprites about a pixel off; this is known for every game, and none
of this game's tours showed one.

The port's enemies start with a last zap of 0, where the original's is the heap's leftover (above).

## Phase three: real play, the fixes mod and registration out (2026-11-08)

The port now plays whole games through the same input a hand uses, in the exe and in the web build, with the fixes mod
loaded and the registration and link code gone. No bug of the original that breaks play turned up (no dead control, soft
lock, crash or lost save in any of the eight games below), so the port took no play fix in this phase; what the real
input showed in the port and its test kit is listed under "Findings". The sessions did not play: code did. The bot behind
the standard `move` command (`src/CBTestCommands.cpp`) chooses every shot from the game's own state, and the drivers send
its steps as mouse and key events.

### Real-input play

Natively `tools/play/drive.py` runs the exe under rundesk, on a fresh user folder, with `-realinput` (SDL's own queue,
the path of a hand: the pointer, the buttons and the keys) and the live command channel for `state` and `move`. A game
of each configuration the player has, the shooter manual and auto (the game file offers no difficulty, so those are all of
its modes), each in the full screen the game starts in and in a window (Alt+Enter, sent as keys): the title, Options with
the shooter chosen and Back, Start Game, the instructions page of a first run, Play, then boards on the bot's shots until all
lives are gone, the click that leaves the game over, the Hall of Fame with the score under the player's name, Back to Main
Menu and the title. On the first board of each game Esc pauses and Enter resumes it, and the focus is taken from the window
and given back: the game's updates stand still while it is away and run again after (the original's own rule, compared in
phase two). The whole of it runs in real time, since the bot's shots follow the board as it is.

The four games, with the final exe (an earlier pass with the same drivers gave 13, 13, 15 and 13 boards; the games are not
seeded, so the scores differ from run to run):

| Game | Boards | Last score | Real time |
|---|---|---|---|
| Manual, full screen | 13 | 33086 | 188 s |
| Manual, window | 13 | 34632 | 195 s |
| Auto, full screen | 13 | 41577 | 180 s |
| Auto, window | 13 | 37967 | 182 s |

Each ended on the title after the game over, the Hall of Fame and its Back button, and each froze with the focus gone
(for example 10935 updates, then 10978 and 10978 while the window was inactive, 11489 half a second after the regain).

On the web `tools/web/play.js` does the same as a page driver (`?player=1&silent=1&fast=1&hold=1&driver=...`), through
`byopop.test`'s mouse and key events on the canvas and its `blur` and `focus`, in the Browser pane. The pane cannot enter full
screen, so `&scale=2` stands in for it: the canvas is drawn at 1600 by 1200, and every event lands at a scaled point, as in a
full screen. The web build's own Fullscreen button was not pressed. The four runs, each ending on the title with no error
in the page:

| Run | Boards | Last score | Moves | Updates (10 ms) |
|---|---|---|---|---|
| Manual, 800 by 600 | 11 | 25097 | 196 | 22150 |
| Auto, 800 by 600 | 9 | 22915 | 132 | 16940 |
| Manual, scaled 2 | 6 | 11436 | 139 | 14780 |
| Auto, scaled 2 | 13 | 44141 | 227 | 26140 |

Each run also pressed Esc and Enter and took the focus away and back on its first board: the game's updates stood still
for 30 framework updates with the focus gone and ran again after it came back.

### Findings

**The web build never froze with its window's focus gone.** Phase two compared the native game's focus handling with the
original's; the web build's `IsActive` answered true whatever the window did, so a hidden tab kept playing. It now follows the
framework's `mActive` on every platform, as the original's `WM_ACTIVATEAPP` does, and the web driver checks it on every run.
The same rule can stall a driven run: the Browser pane took the focus from the page once between two calls of the tool that
drives it, and the game stood still, as it should, until `byopop.test.focus()`. The page driver now logs the page's own blur
events and gives the focus back to a game that stands still (`blurs` and `regains` in its result; none in the four runs above).

**The `state` command's `requester` key saw only the first menu.** The "resume your previous game?" requester sits on the
last one, so a driver could not tell it was there. It now looks at all of them (`HasRequester`).

**The test kit missed answers.** A command answered `MOVE null` was never matched, because `web/testkit.js` had a backspace
character where its ANSWER pattern meant a word boundary, and `byopop.test.command` timed out ten seconds after every busy
move (shared commit 5247a25c). The game's replies also went to stdout, which reached the page's log only on some loads;
they go through `SDL_Log` now, which always does. The kit's `profile.wipe()` leaves the browser's backup of the saves
(`sexy-userdata-backup:/cbpersist-test/userdata`), which the game restores at its start, so a driver that wants a first run
removes it as well.

**A skipped warp leaves the hand where the game does not think it is.** When the ship comes in after a lost life the game puts
the pointer at the board's middle. A test run does not warp the pointer, so `-realinput`'s hand stays where it was, and a
move to that same place makes no event. The native driver moves by one step beside the target first. A shared fix would
let the framework move its hand when the game warps; that is a spec for the coordinator, not a port change.

**A pointer over the bottom panel stops the play** (the original's own rule), so the bot's first step on a board is to put
the pointer on it (`hover`).

### The fixes mod

`mods/fixes` has a toggle for each of its two fixes (README.md, "Mods"), loaded by the port's own start (`-modsdir`, the
`mods-cosmicbugs` folder beside the exe natively, which starts empty, so the native runs above played the unmodded port, and
`/cbpersist/mods` on the web, which installs the mod on by default: Cosmic Bugs is off `BYOPOP_WEB_NO_MODS`, shared commit
90130017). The runner, the suite and the frozen baseline never load it, so unmodded play is
what the baseline holds. Natively a scripted game over fourteen boards (seed 7, a state six times a board) gave 84 states
without the mod and 84 different ones with it, and with both toggles turned off through the mod menu's `modmenu set` and the exe
started again on the same folder, the 84 of the unmodded run, byte for byte. On the web the player page's status says
"1 mod active" with the mod on by default.

### Registration and links out

The trial, nag and registration code is out, and with it every button that led out of the game (README.md, "Registration and
links"): the game plays as the full version it already was, `tools/check.sh cosmicbugs` still finds every play as the frozen
baseline, and `cb.dat` keeps the original's format. The nag screens' levels are still read from the game file as inert
screens, since loading them draws from the generator.

A second look at what the trial left (2026-11-08, at the coordinator's request) found one thing to take out: the third field of
`cb.dat`, where the original kept the trial's time left, was still read into a global (`gTrialSlot`) and clamped to at least
60000 as the trial's loader does. It is now `gSaveFormatField`, a placeholder for the file's format that is written back as it was
read (3600000 in a new file), with no clamp and no reader, so saves still move between the port and the original byte for byte.
The only saves that differ from the original's registered copy are ones a trial copy wrote with under a minute left, which the
original's loader would have raised to 60000 on its next save; the baseline and the suite are unchanged. What else was checked
and kept is listed in README.md, "Registration and links": the nag screens' levels (the draws of their loading), the `nag_logo`
animation their menus name, the `closing_nag` item type of the title's QUIT, the `fv` skips, and `gDemo`, the attract mode nothing starts.

### Start-up logos

The site's "Always skip start-up logos" (2026-11-08) is read where the game's logo screens are (README.md, "Start-up logos"): with
it on, a logo screen (level kind 0xe, the PopCap logo, once for each portal animation file, about two seconds each) goes on as Esc
on it does, the original's own skip, to the main menu. The logos draw no random number: the headless runner's seed series
(`CB_WATCH=1`) is identical for a natural start, for Esc at the first logo and for the skip (`CB_SKIPLOGOS=1`), the first draw
being at update 2171, 11 and 11, so the game's draws after the logos do not move. Natively the exe with `-skiplogos` shows the title at its first
sample (update 1001 of a `-fast` run, where it shows the logo without the flag), and in the Browser pane the web build with the setting on
shows the title at update 411 where the same start without it is still on the logo at 311. With the setting off nothing changes
(the baseline and the suite hold).

### Performance

The original and the port back to back: README.md, "Performance". In short, both load in about half a second; the port holds
about 40 MB more memory (130 MB private bytes against 89) and, against the original paced as a 60 Hz screen paces it, uses about
three times its processor time (18 per cent of a core on a board against 5), while the unpaced original spins at 75 to 96 per cent.

### Oddities

None new.

### Deviations left

As in phase two. The port's new `IsActive` makes the web build stop as the exe does when its window is not the active one.
