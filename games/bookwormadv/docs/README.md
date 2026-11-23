# Bookworm Adventures Deluxe port

A port of Bookworm Adventures Deluxe (PopCap Games, 2006; the exe linked 2007-06-20) to the shared framework and
platform layer: it plays natively (Windows, SDL2) and in the browser (Emscripten). The game code is a translation of
`BookwormAdventures.exe`, found by decompiling it with Ghidra; the game's files (`main.pak`, the loose cut scenes and
properties, and the exe, whose texts are read at run time) come from the player's own copy and are never in the
repository.

Much of the game is not in the exe's C++: its rules, its battles and its books are 621 Lua scripts, compiled for
PopCap's own fork of Lua 5.0 (PopLua), and its creatures and effects are PopAnim animations (`.pam`); the cut scenes
are Flash movies. The port therefore has, beside the game's C++ classes in `src/`, a PopLua virtual machine and
compiler (`poplua/`, `docs/POPLUA.md`), the layer that links C++ objects to Lua (`lualib/`, `luaapp/`), JeffLib, the
library of helpers in the exe that the game's classes use (`jefflib/`), a PopAnim player (`popanim/`) and a
Flash player for the cut scenes (the shared `engines/swf`). `docs/ENGINE.md` maps the exe's regions to these folders and explains how
C++ objects appear in Lua.

## The binary

`BookwormAdventures.exe` is linked by Visual Studio 2005 on SexyAppFramework 1.3 or close to it, with RTTI for the
game's classes. The game's C++ is 0x401000-0x4c8000, the framework 0x4c8000-0x5a0000, LuaApp 0x650000-0x6a0000,
JeffLib 0x6a0000-0x6c9a00, LuaLib 0x6c9a00-0x6ee000 and PopLua 0x6ee000-0x71f000 (`docs/ENGINE.md`, "Code
regions"). The scripts call C++ methods through 358 kinds of method wrappers; `tools/bindings.py` lists the bound
methods from the exe into `re/bookwormadv/bindings.txt`, and `src/GameAppLua.cpp` binds the port's under the same
names.

## Reproducing the analysis

The export lives in `re/bookwormadv/` (git-ignored, like everything derived from the game).

```
python games/bookwormadv/tools/decread.py 44ad10       # a function of the export, readable
python games/bookwormadv/tools/asm.py 0x44ad10         # its disassembly
python games/bookwormadv/tools/fnfind.py -r 0xA 0xB    # the functions in a range (or by string or namespace)
python games/bookwormadv/tools/regtables.py            # the luaL_reg tables of the Lua libraries
python games/bookwormadv/tools/luc.py dis <file.luc>   # a compiled script, disassembled (also: names, stats)
python games/bookwormadv/tools/gametext.py             # src/GameTextTable.inc: the texts read from the exe
```

The decompiler drops x87 operands and some stack arguments, so floating point code and some calls were read in the
disassembly. Every ported function names its address in a comment (`// 0x...`).

## Building and running

```
tools/setup_worktree.sh                                                    # a new worktree: the git-ignored files
python games/bookwormadv/tools/stage_bookwormadv_assets.py "<the game's folder>"   # stages assets/bookwormadv/
cmake --build build/native --target bookwormadv                            # build/native/bookwormadv.exe
build/native/rundesk.exe build/native/bookwormadv.exe -userdir=<dir>       # a game, invisible and silent as tests run it
```

The staging unpacks `main.pak` (`tools/popcap_pak.py`) and names every file in lower case. The game's 42 JPEG 2000
pictures are decoded at run time by the platform's OpenJPEG; its music, `sounds/music.mo3`, is one tracker module in
which each song starts at an order, played by the platform's libopenmpt player and queued by JeffLib's song player
(`jefflib/SongPlayer.cpp`).

For the browser, configure `build/web` with `emcmake cmake` as `tools/build.sh` does and run
`cmake --build build/web --target bookwormadv`. Serve `build/web` (`.claude/launch.json` has `bookwormadv-web` on port
18775) and open `bookwormadv.html`; in the Browser pane always with `?silent=1`. The page imports the game from its
install folder or a zip of it: `main.pak` (unpacked into the game's folders), `BookwormAdventures.exe` and the
`images` and `properties` folders, into its own IndexedDB folder (`/bwapersist`), where the profiles are kept too.

Native options: `-gamedir=<dir>` (else `assets/bookwormadv` next to the exe or up to two levels up), `-userdir=<dir>`
(the profiles and `registry.ini`), `-modsdir=<dir>` and `-nomods`, and the framework's test hooks `-fast`, `-seed=N`,
`-trace` and `-script=<file>`. Scripts take the framework's commands (`<update> move <x> <y>`, `click <x> <y>`,
`key <name>`, `screenshot <file>`, `quit`) and three of the game's own (`src/TestHooks.cpp`): `word [attack]` spells
the longest word the battle's grid allows (and attacks with it), `lua <chunk>` runs a line of Lua in the game's state,
for example `lua profile.Set('UnlockedArena', true)` once the main menu is up (about update 950), and `spectrum` logs
how many cells of the media player's spectrum are lit.

### The drivers' commands and the board log

The test commands of `docs/TEST_COMMANDS.md` (in `src/TestHooks.cpp`, given in a `-script` or live on the `-commands`
channel) each answer with one log line, `OK <command>` or `ERROR <command>: <reason>`. A command that waits for its screen
(a curtain still moving, the title not yet gone on to the menu) tries again every 10 updates, 300 times at most.

- `state` logs `STATE {json}` with `screen` (title, menu, cutscene, map, battle, minigame, minigames, treasure, tome,
  highscores, credits, loading), `mode` (adventure, or the mini-game's class), `level` (book.chapter, from the
  profile), `curtain`, `dialogs` and `talk` (a conversation's panel up); in a battle `health`, `maxhealth`, `enemy`,
  `enemyhealth`, `enemymaxhealth` (hearts, in quarters); on a board `board` (its rows of letters, `+` after one that
  can be picked, `.` for an empty cell), `paused` and `busy`.
- `goto menu` waits for the main menu. `goto adventure|arena|minigames|tome|mediaplayer|options|highscores` presses
  that main menu item (`MainMenu::ActivateItem`, the menu's own action, so a locked item stays locked). `goto
  wordmaster|letterrip|linknspell` opens the mini-games' menu if needed and presses that game's button.
- `unlock all` sets the profile's unlocks as the end of a book does (the arena, the mini-games, the tome, and the
  announcements of them, which Lex then makes on the main menu). The arena still asks for the first book's sixth
  chapter.
- `set health <n>` and `set enemy <n>` go through the scripts' own `_LexSetHealth` and `_EnemySetHealth`.
- `win` kills the battle's enemy with the scripts' `_KillEnemy`, once the battle waits for a word (it sets the
  scripts' `CHEATED`, as the original's cheat key did). `lose` leaves Lex a quarter of a heart (`_lex_near_death`):
  the battle decides a death only after an enemy's attack, so the next one ends it.
- `level` and `seed` answer with an error: the map's chapters belong to the scripts (`goto adventure`, then Enter or a
  chapter's click), and the seed is `-seed=<n>` at the start.
- `move` logs `MOVE {json}`, the move the drivers' bot makes next (`GameApp::MoveForTests`), or `MOVE null` while
  there is none (the board busy, a curtain moving, a screen with nothing to do). Its `what` is, first to last: `dialog`
  (the top dialog's first button), `talk` (a conversation goes on), `tutorial` (the first battle's one tile, or
  Attack), `wait` and `lostturn` (Lex stunned: the grid, once two questions 30 updates or more apart have
  seen it), `hint` (an
  arrow's first hot spot; one a click left standing is not offered again after 60 updates), `overlay` (Level Up's
  "Click to continue" or a script's overlay over the board: a click above it), `gameover` (a mini-game's
  "Click anywhere to continue"), `unstick` (a word left standing in the bar for 100 updates: Submit when it is a word, else its last letter
  given back), `guess` (Word Master: its keys, a letter it has twice 60 updates apart with a press on the empty curtain
  between, which the fast clock would otherwise make a double click the key ignores), `word` (the board's longest word, with `word`; in a mini-game the
  longest this mini-game's bot has not offered yet, since Letter Rip wants new words from the same letters) or `scramble`, `treasure` (the next of Lex's treasures) and
  `continue` in the treasure room, and `enter` on the map. `input` holds the clicks, the moves over Attack and the
  waits that make it. It changes nothing. `tools/movedriver.py` is the thin loop that plays by it natively (`state`,
  `move`, the move's input as real events on the live channel, `-commands` with an empty `-realinput`; `--realtime` for
  the mini-games, whose clock runs out under `-fast` between two questions). On the web `tools/web/moveloop.js` is the
  same loop on the test page (`?player=1&fast=1&driver=games/bookwormadv/tools/web/moveloop.js`): `?first=` the commands
  to start with (comma-separated), `?until=` the screen to stop at once it was left, `?seconds=`, `?scale=2` the canvas at
  twice its size in place of full screen, `?lockstep=1` the game held in a mini-game and stepped only by the moves.

`-boardlog=<file>` (on the web, the page adds it through `Module.callMain`) makes the game append what a driver of
real input needs, each line once as it changes: `screen: <n> <name>`, `loadend: <n> <update>` (a chapter load's screen gone, at the update after the one the original's side reads it at), `opened: <n> <update>` (then the curtain over it open, mTransitionState none, likewise), `talk: <n> 1|0`, `dialog: <n> <id> ok x,y | header | lines` (its
first button), `treasure: <n> continue x,y 1|0` (the treasure room's Continue, and whether it is enabled: a
driver picks Lex's treasures from the left column until it is), `hint: <n> <id> x,y ...|-` (the tutorial's
arrows that wait for a click on a hot spot, and each spot's centre: the drivers click the first), `tutorial: <n> <state> x,y|attack x,y` (the
first battle's tutorial: the one tile it lets be picked, or Attack), `select: <n> LETTERS|-` (the tiles picked so far, as they change), `lostturn: <n> x,y` (Lex stunned: a click there,
on the grid, ends the lost turn's round, once the game takes one), `board: <n> <WORD|-> x,y ... attack x,y scramble x,y words W1,W2,... letters L@x,y;...` (the longest word
the tiles spell, its tiles, the buttons, the 40 longest words and the tiles that can be picked; only while the board
is at rest and a tile can be picked) and, in Word Master, `wordmaster: <n> <GUESS> x,y ... submit x,y rows ...`, a
guess that fits every answered row, from the game's word list. The drivers read it and click: `tools/driver.js` in the
page with DOM events (`bwa.battle()`, `bwa.minigame()`, `bwa.settle()`, each returning a summary) and
`tools/realrun.py` natively with posted click bursts (`chapter`, `minigame`, `settle` steps). A command or option never
given changes nothing: the suite's runs stay the same.

## Testing

`tools/check.sh bookwormadv` (about two and a half minutes) runs the PopLua compiler's dialect checks, loads every
compiled script of the game with the port's VM, every PopAnim animation, and four Flash cut scenes to their end, then
plays four runs of the test profile. The first goes from the title to the main menu, whose quit asks (Play! stays,
Quit quits). The second wins the first chapter, spelling with `word attack`, and saves it. The third goes through the
screens off the main menu: the credits from the options (skipped), the players dialog (cancelled), the mini-games'
menu and its way back, and the arena from its intro and treasure room through Cassandra's tutorial to its first word. The fourth opens the
media player, plays a song and counts its spectrum's lit cells in both of the screen's kinds, the mouse over the book.
The unit suite (`tools/check.sh unit`) checks the JPEG 2000 decoder against Pillow's pictures in
`re/bookwormadv/jp2ref`, and `unit-web` checks the page's pak reader against the staged files.

What the suite does not reach was checked with scripts: a timed fight in the arena, the hall of fame, the tome, the
players dialog's choosing, adding and deleting (and a name in use), and the three mini-games. In the browser: the
import, the main menu, the map, a battle, the credits, the players dialog, the mini-games' menu, a game of Word Master
left for the hall of fame, the tome, and the window losing and regaining the focus.

## Deviations from the original

Each is a difference in pixels without a visible effect, measured on the screenshot pilot (2026-11-01, shotdiff's faint boxes;
`docs/SHOTDIFF.md`, "Visible and faint").

The JPEG 2000 art (the main menu's and the mini-games' screens) is decoded by OpenJPEG; the original links j2k-codec. On the
main menu's bare background the two differ by a median of 2 levels or less a channel, by 8 at the 99th percentile, and by more
than 8 on 0.65% of it, at edges. The JPEG art matches exactly: the original links libjpeg 6b with its default settings, and
the port's decoder gives the same pixels.

Sprites drawn through a transform (Lex and the enemies, the credits' creatures and fire bowl, the map's rotating rays) sit
at the same place on both sides (no whole-pixel offset in any box measured), but their pixels are sampled differently. With
`Is3D` 0, as the pilot runs both sides now, the original draws them with the framework's software rasterizer
(`MemoryImage::BltMatrix`, since `DDImage::Check3D` is false without 3D), and the port draws them as the original's 3D path
does, through its renderer. The Trojan Spearman's idle animation keeps 420 to 480 visibly different pixels in every frame
(a pale noise over the whole sprite and a one-pixel line along an edge of a limb), and no update of the port does better: a
screenshot at every update from 30 before to 30 after (`tools/pilot/lag.py`) finds the best match at the same update. So it
is not timing. These items wait on the framework's 2D software path (the user, 2026-11-01: 2D must play identically too;
the framework gets SAF's software drawing of transformed sprites when `Is3D` is off, after origrun's `--3d on`). They are
neither masked nor accepted: the battle's enemy (the item in 7 shots, and the attacks clip's residue), the battle intro's
spear, and the credits' creatures and fire bowl. Once `--3d on` works, the same screens are compared with 3D on, where the
port's drawing should already match.

### The PopCap logo taken out of the startup (2026-11-08)

The original shows the PopCap logo before anything loads: a fade in, a hold of the properties' `PopCapLogoTimer` (100
updates) and a fade out, 356 updates (3.6 s) in all, with no click or key to leave it, and only then closes the title's
curtain and starts its loading thread. By the loading rule (`CLAUDE.md`, "Making a port", 2026-11-07) that wait of the
original's making comes out of the port itself: `TitleScreen::Update` goes on to the curtain and the load at once, as the
logo's end does, so a player reaches the title loaded in 1.7 s rather than 5.25 ("Performance"). The logo draws no random
number (the suite's first chapter plays as before). `-popcaplogo` keeps it, for comparisons with the original: the pilot's
`run.sh` and `lag.py` pass it.

### "Deluxe" taken out of the title and the window's name (2026-11-21)

The user's note of 2026-11-18: "Remove the "Deluxe" text from the main screen of any game it may appear in, to avoid
misleading people about whose game version they're playing". In this game the word is a sprite of its own,
`IMAGE_TS_DELUXE`, which `TitleScreen::Draw` puts under the logo while the game loads and until the main menu takes over
(the main menu's logo, `IMAGE_MM_LOGO`, has no "Deluxe"), and it is in the window's name. The port draws the title without
the sprite (the logo, Lex and the tiles are as before) and names its window "Bookworm Adventures". No image is edited:
the sprite is still loaded with its group and is simply not drawn. The folder of the port's saves keeps its name, so no
player's saves move. The title clip's shots (`title`, `title_3d`) show the logo without the word; the boxes around the word
in their frames are this deviation, left unmasked.

### Registration code taken out (2026-11-07)

The port plays as the registered original does, and the trial code that only an unregistered copy reached is gone (the
user, 2026-11-07). Most of it was never ported: the trial's window, its "buy" screen and the checks of its marketing
modes (an hour's play, the lockout after the hydra in the first book's seventh chapter) are named in `port:` notes
where the original called them. What came out now: the two trial dialogs (`DIALOG_NEW_PROFILE`, the "new profile?"
at the trial's end, and `DIALOG_TRIAL_COMPLETE`, its "buy the full version?") and their answers in `ButtonDepress`;
the app's `mQueuedHydraLockout` and every test of it (the map's Back, Continue and Options, the treasure room's Back,
the map's mouse); the scripts' `ShouldDoHydraLockout` (no script calls it), so `DoAfterHydraConvPanel` always sets the
warp-in sound's callback; the `AppTime` registry value and `mAppTime`, which counted play time only for the trial's
hour; and the resource globals only the trial used, the whole `UpsellTrailer` group and `SOUND_MM_BUY_NOW`
(`tools/resgen.py` leaves them out of `Res.h` and `Res.cpp`). `QueuedHydraLockout` stays as the scripts' method,
answering false as a registered copy does, since the map's script asks it. None of this has a visible change: the
same scripted runs of the port built before and after (chapter 1 by the word-attack script, and book 1's first three
chapters won by the drivers' `win` with the map, the conversations and the treasure room clicked through) give the
same output, sound log, saved profile and screenshots, byte for byte.

No button leads out of the game: neither the original's source nor its scripts have a "More Games" button, a link or
any other call that opens a URL or a program, so the coordinator's rule of 2026-11-07 (every such button out, its
neighbours closing the gap) has nothing to take out here.

### Faults the pilot found

The tome stayed locked on the port's main menu. The original's profile load (0x48a390) calls cheat.lua's global
`_debug_profile_loaded_value(key, name)` for each value read and `_debug_profile_done_loading(name)` at the end, whenever
the app has a profile (+0x950) and the load is not the hall of fame's (its argument 1), with no cheat check; the second
sets UnlockedBestiary for a profile past the first book. The port skipped both. Fixed (2026-11-01): `Profile::Load` makes
the calls; the hall of fame's loads pass `theQuiet`.

The battles showed a hill behind the enemy in the port only (the item seen in 5 shots). arena.lua draws the midground
animation (`images/pam/MG<book>-<chapter>.pam`, the island) only when `gCApp:Is3DAccelerated()`, and the port reported 3D
whenever its SDL renderer was accelerated, ignoring the registry's `Is3D` (the user's is 0). Fixed (2026-11-01): the game's
3D mode is `LuaApp::Is3DMode()`, read from `Is3D` (on when the value is missing, as when the original's 3D test passes) and
written back on exit only when the player changed it, as the original does, set by the Options dialog's box, and what the scripts and the game's C++ ask. With `Is3D` 0
the pilot's battle shows no island, like the original. Under origrun on this machine the original turns `Is3D` 1 back to 0
and draws in software; it runs in 3D only on the visible desktop (`tools/pilot/visrun.py`). The original's questions at exit
(0x53c120: "Hardware Acceleration Confirmation", asked when 3D was switched on and is not recommended, or did not work)
cannot come up in the port, whose renderer is always recommended and working.

The buttons of the battle's menu, the quit dialog and every other `GameDialog` had their end caps stretched, and the new
player's name sat in a stretched frame. The original draws a stretched `FadeButton` (0x442b10, its byte +0x12b) and that frame
(0x482aa0) with `Graphics::DrawImageBox` (0x4f5ca0 and 0x4f5c70: the corners a third of the cel at their own size, the edges
and the middle tiled); the port drew one stretched image. Fixed (2026-11-01). Every other caller of the two box functions
in the port already drew a box.

The hall of fame's arena list showed 0:14:40 for the authors' run to the ninth stage, where the original shows 0:14:50: the
port's table of the ten default runs had 88000 hundredths of a second for it, the original's (0x4632a0, MakeTimeList) 89000.
Fixed (2026-11-01); the other nine, and the ten default bonus runs, match.

The mini-games showed an empty plate where the original has its greyed Submit! button. The scripts give the attack button
the Submit! picture (`ChangeAttackBtnImage`), and the port's `SetAttackDisabled` and `ShowImmuneAttack` then took the cels'
size from the Attack picture; the original's (0x41a070, 0x41a3e0) take it from the picture the button has (+0xb0). Fixed
(2026-11-01).

A hint's lines (Letter Rip's tutorial, any hint with its lines up) stood 19 pixels apart; the original's ArrowHint::Draw
(0x4036c0) steps them by the font's ascent (14), not its height (0x403953). Fixed (2026-11-01).

### Timing in the pilot

Both sides are scripted by the game's update count (`tools/pilot/gen.py`). The port runs a script line "N ..." at the start
of update N, before the widgets update, so its screenshot shows update N-1; origrun's `at N` holds the original once update
N has run. The port's lines are therefore one update later than the original's (2026-11-01). That alone moved nothing that
follows a click, since the click and the shots moved together.

What remains is the original running one or two updates behind the port after a click, and further behind around a load,
and it varies by an update from frame to frame. `tools/pilot/lag.py` measures it by matching each of the original's pictures
with a port screenshot taken at every update. In the attacks clip every original frame matches a port update with 14 to 216
pixels left on a half-size picture of the battle (the Trojan Spearman's sampling, above), one update behind and jittering by one. In the Enter! transition every
original frame is a port frame exactly (0 pixels), two updates behind until the chapter loads and 10 to 12 behind after it (before `-chapterloadupdates`, below):
the curtain finishes only once it has been drawn at its last frame (`mDrawn`, set in its Draw, 0x4413b0, and read in its
Update, 0x4411d0), so every screen behind a curtain begins at an update that depends on how often the game draws, on both
sides. In the credits the first name shakes (`Rand() % 5` each update after a creature's `causedamage`): in the pilot's run
the original's shot shows the port's shake of two updates earlier, but `lag.py`'s run, which draws the port at every update
(each screenshot is a draw), matches the original's shot exactly (0 pixels over the names at the same update). So the random
draws agree, and the difference is when the credits began.

Part of the lag looks like origrun's: a click is posted while the original's window thread is suspended, with 60 ms of sleeps
(`post_click`), and a held capture takes its own time, so the original's clock runs on and it catches up with updates it
does not draw. origrun's own fix for this is under way (2026-11-01). Until then these items are timing, each with its lag measured: the
attacks clip, the Enter! transition, the credits' shake, and the Attack button's sparkle in word_rain (an effect that starts
with the last letter's click). The title clip's frames 18 to 30 are the
original's loading screen, shown for as long as its loading thread takes; the port loads in slices of 20 ms between frames,
so it shows it for a different time (until `-startloadupdates`, below).

How often each side draws matters as much. Under rundesk the port's scripted run manages about 85 updates and 37 draws a
second (its log's counts), so it catches up with updates it does not draw, and a wait for a draw (the curtain) ends
later. So the pilot runs the port with `-drawevery` (the framework's test switch: an update that has not been drawn is
drawn before the next one runs, off by default), and lag.py does too. With it the credits' shake matches the original's
shot exactly one update earlier (0 pixels over the names), and Letter Rip's tutorial matches but for its "PAUSED" text,
which blinks 150 updates on and 50 off from the Yes click: the shot fell exactly on that edge, and the original's click
lands a few updates apart from run to run (a burst of the original turned the text off at update 5383 in one run, and
the screens run still showed it at 5385, where the port turns it off). The shot now comes 100 updates after the click.

The chapter load decided the battle's board (2026-11-01). ChapterLoadScreen's Update (its timer at +0xb0, from 50) draws
a new hint with `Rand()` every 50 updates, and the original loads the chapter on a thread, so how many updates the screen
stays up, and so how many draws it makes, follows the machine: 61 and 69 updates in two sandbox runs, one hint draw
more than the port's native load of about 3 updates, which shifted every later draw and gave the battle another board.
`run.sh` now has origrun watch that timer (`--watch "[[0x8259e4]+0x83c]+0xb0"`), and `tools/pilot/loads.py` counts each
load's updates from the log. The port takes them with the test option `-chapterloadupdates=N,...`, which holds the
n-th chapter load's screen for N updates and spreads the loading evenly over them. With it the boards agree and every
frame of the Enter! transition, before and after the load, matches the original's at lag 0 (each within a run of
identical port frames that includes the original's update, by `landed.tsv`). What still differs is the loading bar's
fill while it is up, 238 to 656 pixels in three frames: the original's thread fills it fast at first and then stalls,
at a pace no update count gives. That is timing, the bar's box masked during a chapter load.

The test option `-holdloadhints` (off unless given, 2026-11-22) holds those hint draws: a chapter load's screen keeps
the hint it opened with and draws no new one, as the original does with the ChapterLoadScreen patch
(`tools/origrun/patches/bookwormadv.txt`, `tools/pilot/seedpatch.py --holdhints`). Only the screens pilot gives it
(`tools/pilot/run.sh`, `vis3d.sh`), because the steps the port's bot makes fit one number of hint draws, and the
loads on a busy machine took anything from 5 to 260 updates (the 3D desktop's 5 to 86). The frozen baseline,
`tools/check.sh` and the logic comparisons (`-randlog`, the state checks) never give it: there the hint's draws and
the original's own load lengths are compared as before.

The test option `-loadanchors` (off unless given) adds the board log's `loadend` and `opened` lines, the update a
chapter load's screen went and the update the curtain over it finished opening, which the pilot's steps count from.
Only `tools/pilot/run.sh` gives it, so the frozen baseline's board logs compare line for line.

The pilot's steps also anchor each chapter load (`tools/pilot/gen.py`'s `loaded <ms>`). On the host the original's loader
runs on the wall clock while its updates run on origrun's virtual clock, so a load lasted 41 to 7415 updates there. The
original's script waits until the load's screen (app+0x83c) has come and gone and the curtain over it has opened
(app+0x8ec, mTransitionState, back to 0), marks the update counter, and times the steps after it from that mark
(origrun's `waitfor`, `mark` and `at +N`). The port's lines after the load count from its own curtain's opening, the
`opened` line of a `-fast` pass with the original's load lengths (`anchors.py`, `run.sh`), so each side counts from its
own. The curtain, not the load's end: it steps a cel every 5 of its own updates, counted from before the load, so it
opens 0 to 4 updates later or sooner after a load of another length, and a battle's first clicks then land early or late
on the board (book 1's last battle stuck on the invalid word's hint after a load of 2016 updates; the arena parted from
the port at its third lost turn, the original's gap from Enter! to the load being 135 updates on the host and the
port's 141). `<ms>` is where the curtain opened in the port run the steps were made with (90 updates a load; `opened`
in a `--nominal` port run of the part), so the steps after it keep their timing. A part that shoots the curtain as it
opens (the battle, the map's mini-game stop, the new game) says `loaded <ms> load` and counts from the load's end
(`loadend`) instead, its later steps loose enough not to mind the curtain's 4 updates. A port run with no original
(`tools/coverage/port.sh`) gives gen.py `--nominal`, which times the lines after a load from that end.

The startup load did the same to everything after the title: the original ran about 190 updates behind the port from
its menu on. Its loading thread ran from update 406, where StartLoadingThread (0x549480) sets the app's +0x4f9, to 546,
where LoadingThreadCompleted sets the app's +0x919 (the thread's own +0x4fa in the same update): 140 updates in the
sandbox. `run.sh` watches both bytes, `loads.py --start` gives the count, and the port's `-startloadupdates=N` completes
its load N updates after StartLoading, its tasks spread evenly over them. With it the title clip matches at lag 0 from
the loading screen's frame 18 on, through the menu (each frame's run of identical port updates includes the original's).
Frames 20 to 26 differ, 69 to 683 pixels at a quarter of the size: Lex eats across the screen as far as the load has
got, at the original thread's pace, so his place is timing too and is masked while the title loads. There is no draw
from the RNG in the title's load.

The original under origrun showed no hover. On mouse messages it asks where the real cursor is (ClientToScreen,
GetCursorPos and WindowFromPoint, in SexyAppBase's message handling, 0x547ea0, from 0x548635), and rundesk's desktop has no
cursor at all (GetCursorPos fails), so it took the mouse as gone and sent the widgets a mouse exit. With the user's leave to
patch the test copies (2026-11-01), the pilot's `seedpatch.py` makes that test answer "inside" in the staged copy
(0x5486b7; the cursor's own position is read only on the "outside" path, and origrun's mouse never leaves the window). It is
a change to the test harness's copy of the original, never to an install and never to the port, and it is the one patch
besides the seeds. With it the original lights Link n' Spell's YES and the tome's scroll arrow as the port does: both
pairs are now faint only.

The pilot's third part (`tools/pilot/screens.txt`, 2026-11-01): the tome and its list scrolled, and each mini-game from its
welcome to its menu and the quit (Word Master with a guess, Letter Rip into its tutorial, Link n' Spell losing and regaining
the focus). Besides the fault above and the items here, what differs is the animations' sampling and timing (Lex, Moxie,
the Submit! button's glow).

The fourth part (`tools/pilot/win.txt`): the first battle won with DRAWING after battle.txt's RAIN, the enemy's defeat,
Cassandra's three lines, the walk to the next battle and its start with its hint. Its talk matches exactly; the walk and
the next battle's hint match the port's picture of one update earlier (lag.py: 40 and 0 pixels left, the 40 in the
sprites), and the rest is the enemies' sampling and the Attack button's sparkle, both above.

Once the port held its chapter load as long as the original's (`-chapterloadupdates`, above), its first battle got the
original's board (UAEN / GNOH / AIJW / RGRE by rows), not the one the battle and win scripts were written for: their
clicks spelled UEJG, no word, and the win part no longer won. Both scripts now spell RAIN on that board, and the win part
DRAWING on the board after it (2026-11-01). The board stays the same while the original's load lasts 50 to 99 updates
(one hint draw); the loads measured so far ran 52 to 85.

The web build ran the same four parts' scripts (2026-11-01): the page was given `-script`, `-seed=12345` and `-drawevery`
from the Browser pane (by wrapping `Module.callMain` before Play; the page itself has no such option), with the pilot's
two profiles in its folder, and its screenshots were compared with the native port's by shotdiff. The tome and mini-games
(17 shots), the battle (12, both clips) and the won battle (8) all match or differ only faintly, and so do the menus but
for three things. The media player's spectrum differs, as it follows the browser's audio clock (above). Once, in
three web runs, the main menu's library glow after the quit dialog was in step with the native build at every update (a
run with a shot every 10 updates); in the other two it was three updates ahead of it, from somewhere after the media player.
The glow pulses by the update count (MainMenu::Update), so something before it depends on the wall clock in the
browser; not found yet (the media player's music and the pane's focus are the candidates).

## Oddities

The original's own run-to-run variation, which the port does not copy in normal play (`CLAUDE.md`, "Oddities are not
chased").

How long the loading screens stay up. The original loads at startup and before each chapter on a thread, so the
screens last as long as the machine takes: 134 to 263 updates for the startup load and 52 to 85 for the first chapter
in the pilot's sandbox runs. The chapter's screen draws a hint, one `Rand()`, every 50 updates, so a load of 100 updates
or more gives the next battle another board. The port loads in 20 ms slices between frames and shows the screens for its
own time. To see it: origrun's `--watch "[[0x8259e4]+0x83c]+0xb0"` (the chapter screen's timer) or `"[0x8259e4]+0x919"`
(the startup load's end), as `tools/pilot/run.sh` does. Copying it would mean loading at the original thread's speed,
which no fixed number of updates gives. The test options `-chapterloadupdates=N,...` and `-startloadupdates=N` hold the
port's loads for the counts measured in a run, so that the pilot's runs agree; normal play never uses them. Within a
held load the loading bar (chapters) and Lex's place (startup) still move at their own pace on each side.

## Fix candidates (2026-11-08)

None for the fixes mod. What was checked: the port's play against the original in the identical-play pass, and the
real-input play natively and on the web (a new player through the introduction and the tutorial, the whole adventure
of three books by the bot to the Hall of Fame, the arena, the three mini-games, `IDENTICAL_PLAY.md`), looking for waits
that only cost the player time and for bugs of the original that do not break play. The bugs found broke play or were
not the original's, so they are fixed in the port itself: the mini-games' end ("Expected boolean in LuaClassRet") and the
music lost after a boss. The loads run at the machine's speed, in 20 ms
slices so that the loading screens keep drawing, as the loading rule asks. Nothing else came up: every
conversation, cutscene page, Level Up and "Click to continue" answers a click at once, and the curtains between screens,
the battle intros and the attacks are the game's own animations, which hold the player only as the game's rules do.
This is a result of the play-throughs, not of a line-by-line reading of the export for waits, so a later find is added
here.

## Performance (2026-11-16)

The performance pass. Both sides ran on this machine (a Core i7-12700F with a GTX 1080, Windows 11) in real time, held
at the title with a saved player and no input, since the title draws every frame on both sides (Lex, the logo, the
curtains' frame) and a minute of play on the original would need its board read back through origrun. The port ran
under rundesk with the test profile (`tests/menu`), in 3D as on a card that passes the original's Direct3D test (the
original's first run on a player's machine), windowed in its own 800x600 window (ScreenMode 0, `-windowscale=1`; its log:
"client 800x600 px, logical 800x600, render scale 1, windowed, renderer direct3d11"), with no `-commands` channel in the
processor runs. The original ran through origrun on rundesk's desktop with the user's saved player, where DirectDraw has
no hardware and it runs in 2D (each run's log: "3D mode: the game reports OFF"; none logged a late window).
`tools/profile/memrun.py` reads either process from outside: private bytes every 50 ms, the process's and each thread's
cycle counts (QueryProcessCycleTime, QueryThreadCycleTime) at two moments, `tools/memmap.py` and every private
allocation at chosen times, and the port's `memreport`. `tools/profile/pairs.sh` runs the two sides turn about, each
pinned to a performance core of its own (`tools/pincore.py`), the cycles counted from 40 to 100 s and the memory read at
105 s; `pairsum.py` gives the medians.

| Five pairs, pinned | The original (2D) | The port (3D, 800x600) |
|---|---|---|
| Processor time, 40 to 100 s | 24.4 G cycles (17.5 to 26.6) | 7.9 G cycles (6.9 to 9.2); 6.9 G (6.0 to 8.5) without SDL's audio thread |
| Private bytes at the title | 91.9 MB (91.4 to 94.1) | 79.3 MB (77.4 to 80.3) |
| Peak private bytes | 92.2 MB (91.6 to 94.5) | 101.1 MB (97.4 to 105.2) |

The port takes a third of the original's processor time. The original's music thread (BASS, which mixes its MO3 in
the game's process) is counted on its side, so the port's figure with its mixer is the one to set beside it. The
port's default is full screen, which on rundesk's desktop is 3840x2160 at render scale 4: one pinned run took 16.0 G
cycles, still under the original, and held 77.3 MB at the title with a peak of 100.1. Unpinned, the port's main thread
took 5.7 to 25.6 G cycles in the same minute (four runs: 10.2 to 30.9 G in all), as its vsync present spins on the free
cores of a desktop that has no vertical blank; pinned, as the trunk measures every GPU-drawn port (`docs/MEMORY.md`,
"Like for like, for a renderer whose cost lands in a driver thread"), it does not. In 3D through dgVoodoo (`--dgvoodoo
--3d on`, one run) the original held 157.7 MB at the title and peaked at 188.9 MB; dgVoodoo loads Direct3D 9's and 11's
driver images both, so that is an upper figure for its 3D mode.

**Where the memory went.** At the start of the pass the port held 98.4 MB at the title (three runs, 98.2 to 98.5) and
peaked at 117 to 121 MB while it loaded. Against the original's 2D figure that is 6 MB at rest with the Direct3D 11
device's 28 MB inside it, so the port's own allocations were already below the original's. Two holders came out:

- **The curtain's three copies, 16 MB.** The original makes the flipped and mirrored copies of the curtain for each
  transition and frees them with it; the port makes them once and keeps them (no 25 to 30 ms hitch at each curtain),
  and kept their pixels too, 5.3 MB each. A copy's pixels now go once its texture is up and are made again the same
  way when anything needs them (a lost device, a software draw): `MemoryImage::BitsSource::mMake`, a framework hook
  for a picture a game made, as a file makes a loaded picture's again. With `BYOPOP_DROPCHECK=2` (each dropped picture
  made again at once and compared) no picture differed.
- **The word list, 5 MB.** The original keeps its 167,624 words as a `std::set<std::string>`, a node of 40 to 64 bytes
  each with the heap's header (307,895 blocks in the port's heap, 7.3 MB of them the set's). `Lex` now holds the
  letters in one array and the words' places, sorted in the set's order, in another, about 2.3 MB, with the same
  answers; the heap fell to 97,223 blocks. This one is below what the original holds.

What is left at the title (`memreport`): the libraries and devices at start 34.3 MB, the Direct3D 11 device about 28 of
it; the pictures' pixels 7.7 MB (palettes 2.8 MB, the curtain's own indices among them, and the PopAnim pictures,
which load outside the resource manager); the 92 sounds 5.5 MB, as 16-bit samples at their own rates as the original's
buffers hold them; the music's module file 2.0 MB and the player of the song that plays; PopLua's heap 2.1 MB. The
textures, 62.8 MB by the estimate, live on the card and cost no private bytes; the original in 2D keeps all of them as
32-bit pixels in its own memory.

**The load's peak** is the graphics driver's, not the game's. Between 5 and 16 s one private allocation of 20.4 MB,
write-combined (protection 0x404), shrank to 3.1 MB about 6.5 s after the load ended: the NVIDIA driver's staging for
the pictures' uploads, which the trunk sends to the GPU every 2 MB (`platform/GPUFlush.cpp`). It is shared code, so it
went to the coordinator as a note rather than a change here. The original's 2D mode has no such device; its 3D mode
through dgVoodoo peaked 31 MB above its own resting figure.

**Looked at and left.** `Mixer::SetSongPlayersPooled`: JeffLib's song player loads the one module twice and crossfades
between the two songs, and the pool keeps a stopped player for the next song, so both players live after the first
crossfade either way; the port already makes each at its first play. Lua's heap: 2.1 MB at the title, as the
original's (`docs/MEMORY.md`, "Bookworm Adventures' Lua heap"). Loading: 1.70 s to the title against the original's
6.4 s (the performance notes of 2026-11-08, before its PopCap logo came out), not measured again.

The frozen baseline is identical after both changes (`tools/check.sh bookwormadv`).

## Coverage check (2026-11-15)

The whole check of `docs/COVERAGE.md`. origrun's `--coverage` recorded which of the original's functions ran in 17 runs
on the host, on the virtual clock (one, `realclock`, on the real clock with the silent sound stand-in), each from a
copy under `build/` with the pilot's seed and hover patches (`tools/coverage/run.sh`; the steps in
`tools/coverage/steps/`, the hits in `build/coverage/bookwormadv/hits/`). They are the pilot's three passes (menus,
battle and win, the tome and mini-games); the idle title; the options and credits; the players (one chosen and
deleted, a new one through the intro and into the tutorial); the first battle won and the first chapter played on
(`battle`, `win`, `win1`); Word Master, Letter Rip and Link n' Spell played to their ends; a chapter at the end of the
first book (`ch1_10`); and the arena's first fight (`arena`). Every run ended with the game exiting normally, and
origrun's guard saw no program, file or link launched.

The later runs start from saves the port's own bot made (`tools/coverage/advance.sh` plays the adventure on with the
port's movedriver and keeps each save), and their steps are made by the port too: `tools/coverage/play.py` runs the
port from the start after every step and reads its board log, as `tools/realrun.py` does, so each step answers what
the port shows at that update (a dialog's button, the tutorial's tile, a hint, the board's best word and Attack, the
treasures, Enter). The original deals the same boards when no hint is drawn while a chapter loads (a test patch of
`ChapterLoadScreen::Update` in `tools/origrun/patches/bookwormadv.txt`; "Oddities" has why the load's length matters)
and the port loads natively. For the original's runs past the first book's seventh chapter, a second test patch makes
the trial's end check answer as the registered game's does ("Registration code taken out"). `tools/coverage/bwa.py`
reads and changes a save (the arena's run unlocks the arena in a save at the third book), and `port.sh` runs any steps
on the port, windowed as the original's runs are.

`games/bookwormadv/coverage.txt` sets three ranges as the game's own code: the game (0x401000-0x4cce30), the
framework's FlashWidget and PopAnim that the shared framework lacks (0x527ea0-0x52efc0), and LuaApp, JeffLib, LuaLib
and PopLua (0x650000-0x71efb0). They hold 7169 functions; the runs reached 2980.

**Reached in the original with no counterpart in the port: 1457 after the first runs, 2058 at the most, 0 at the
end.**

- **Placed: about 420** (the port's functions with an address went from 1124 to 1547).
  Functions the port translates inline or in another shape got their address in the comment
  above the function that holds them: the Lua binding's method calls (`lualib/LuaBind.h`), LuaWidgetTpl's events and
  listener callbacks (one template here, an instance per widget class there), Lua library helpers, sort comparators,
  and the like. Comment edits only.
- **Left out on purpose: 3525 functions, in 1931 `none` lines, each with its reason.** `tools/coverage/nonelines.py`
  finds 2880 of them from the export alone (the Lua binding's template instances and wrappers, bodies the compiler
  emitted for several types, and scalar deleting destructors); the other 645 were read one by one, by Sonnet agents given `tools/coverage/worklist.py`'s lists: the C++ library's containers and
  strings, the Ogg Vorbis decoder in the exe (the platform decodes with stb_vorbis), the parts of PopLua the port does
  without (`docs/POPLUA.md`), LuaSupport's developer features, the registration code and the update check.
- **Missing, translated: one.** `GameApp::SwitchScreenMode` (0x4458b0) and the two flags `GameApp::Init` sets: in full
  screen, and whenever the screen is no larger than the game, the original turns `mVSyncUpdates` and `mWaitForVSync`
  on, so the framework calls the screens' `UpdateF` (each calls it itself otherwise), and an open options dialog's
  Full Screen box follows a switch (Alt+Enter). With the port's fixed 100 Hz the update path gives the same play: the
  suite's four runs, which start full screen, keep every board, word, script line and save of the frozen baseline,
  except the registry's `WaitForVSync`, now 1 after a full-screen run as the original writes it. The baseline is to
  be frozen again from this commit; its other difference, the `screen: map` line logged while the game quits, is the
  board log's own fix (the test hook no longer logs the screens coming down).
- `GameApp::InitPropertiesHook` (0x446c90) is the framework's `SexyApp::InitPropertiesHook`: partner.xml's NoReg,
  ProdName, DefaultWindowed and Title.

**Defined in the port, never reached: 716 after the first runs, 316 at the end.** The later runs reached the arena,
the later books' screens and the mini-games. Of the rest, 87 are PopLua's compiler (`lparser.cpp`, `lcode.cpp`,
`llex.cpp`): the game loads only compiled scripts and no run compiled one, so the compiler is a candidate for taking
out (the suite's dialect checks use it). The other 229 (233 counterparts) were
read again against the export by a Sonnet agent and checked here. 156 a player can reach (script-bound methods of
later books, dialogs, error paths), and 77 nothing in the game calls: these are candidates for taking out, with the
compiler. They are mostly developer code: the Lua widget bindings no script uses, the dprops editor that nothing
loads, string and math functions no script calls, the Lua stream readers, the profiler (`perf.Start` and `perf.End`),
the developers' script reload (`LuaSupport::DoFile`, which in the original also clears and registers the bindings
again) and the PopAnim mod parser's error message (the original's names the line in a popup). Five slips came out:
`GameDialog::KeyDown` now ignores Escape while the credits run, `LoadResGroup` gives false at once when a group fails
and relaxes the update backlog only after a load, as the original does, and the other three are the profiler, the
reload and the parser's message just named, left as candidates. `TreasureScreen::DoChapterTransition` (0x4a4ff0) was
not settled: what it does to the treasure room's buttons 10 to 12 at its end may differ, but they are hidden by then.

**Oddities.** `ch1_10` reached its battle in the original but no word was taken there: the original dealt another
board than the port's play expected, though both started from the same save. The chapter starts from the map, where
the first chapter's run (`win1`) agrees; what differs was not looked for (an hour's rule). The run still reached the
map, treasure room and battle code it was for. In the port the first click on the main menu after Lex's unlock pages
only ends the panel (`arena`'s steps click the colosseum twice); the original was not checked for the same.

**Cost.** The original's runs took about 40 minutes of machine time, the port's step-by-step plays (each step a run
of the port from the start) about two hours, and the sorting about 30 minutes of Sonnet agents.

## Unneeded code (2026-11-16)

What nothing reaches was found two ways. First the linker: an -O0 build of the game alone in `build/gc`, compiled
with `-ffunction-sections -fdata-sections` and linked with `-Wl,--gc-sections -Wl,--verbose` (docs/COVERAGE.md), whose
"Discarded" lines `tools/coverage/gcdead.py` matches against the functions each of the game's own objects defines.
Second, a function defined inline in a header that nothing calls is never compiled, so the linker never sees it;
`tools/coverage/inlinedead.py` lists every inline member whose name no other line of the game's sources uses. Each
candidate was then read against the export (its callers, its vtable slots, `bindings.txt`) before it went, and has a
`none` line in `coverage.txt` with the reason. A second linker run after the cuts found nothing more in the game's code.

Taken out: `Board::LoadScripts` (0x4185d0, a developer's reload of the scripts, with no caller); the conversation
panel's `Deactivate` (0x4320b0, whose only caller was the trial's buy screen, already out) and
`SetAllContinueTextColors` (0x432cb0, a vtable slot nothing calls and no script binding names); `Tile::Fall` and
`Tile::IsInRow`, `TileEngine::DrawTiles` (no caller in the original either), `Board::GetGridWidth` and `GetGridHeight`;
`ExtractResourcesByName` (0x4aead0, no caller) and the MiniGameCommon group's globals and their Extract function, which
no C++ code reads (the mini-games' scripts take those pictures from the resource manager), now left out by
`tools/resgen.py` so that `Res.h` and `Res.cpp` stay generated; 19 of `JeffLib::Animator`'s virtuals (the other draws,
the tints, ranges, direction, random order, the frame setters) and its copying, now deleted, since the one Animator in
the game (a conversation portrait) only loops; `BonusText`'s second constructor (0x6c4820); `DataDrivenClass::Reload`
(0x6b3e50) with the two virtuals only it called and the credits' versions of them, and the file name it kept for it;
`LuaResourceManager`'s three lookups (0x65d500, 0x65d580, 0x65d5f0: the port's `LuaApp` bindings look resources up
themselves), `LuaApp::LuaAppHook` (an empty slot), `LuaSupport::SetOwned`, `LuaClassRet::Number`,
`LuaLinkedClass::GetLinkedObject` and `Class<T>::Info`.

Kept. `Animator::Update` and its draw colour, and `BonusText::Update`, keep the original's branches for what only the
removed functions turned on (ping-pong, ranges, reverse and random order, tints, a target height): each is one function
of the original translated whole, and taking its branches apart would rewrite it rather than take code out. The 21
functions of PopLua the linker lists (`lua_dofile`, `luaL_ref`, `lua_equal` and the like, and the host hooks
`poplua_setprint` and `poplua_setcwd`) are Lua's C API in PopCap's fork, kept whole as a library, as the C runtime is.
The compiler stays too, since the loader compiles a `.lua` that is newer than its `.luc`, as the original's does, so a
player's file reaches it, and so do the script-facing functions that the coverage review found no script calling
(`LoadResGroup`, the Lua widgets' forwarders, `string.rep` and the rest): a script can call them, and they are what a
script mod has to work with.

## Readability (2026-11-16)

The pass followed `docs/CODE_STANDARDS.md`. `namecheck.txt` now brings `jefflib/`, `luaapp/`, `lualib/` and `popanim/` into the
check beside `src/`; `poplua/` stays out, as PopCap's fork of Lua 5.0.2 in Lua's own names and style, kept as a library. The
first run found 248 things; `tools/namecheck.py games/bookwormadv` now finds none.

**Fields.** The fields nothing reads came out where nothing depends on them: BattleEngine's +0x34, the tile's +0x80, the board's
+0xe5 and its two strings at +0x130 and +0x14c (copied and cleared, never read), the stats book list's +0x138, and the
Animator's four unknowns (+0x2c, +0x30, +0x98, +0x99, read only by its copying, which is gone). Two were read and got names:
the board's `mIgnoreUnpauseClick` (+0xe9) and `mLastSpacePauseTime` (+0x10c). The flag that the scripts pass to a new tile and
an arrow hint, and that nothing uses, is `theIgnoredFlag`: the scripts' own `CreateLinkedObject` takes two arguments and drops
the third. `tools/splitfields.py` listed 15 fields written and never read. Fourteen now say in their comment that nothing reads
them, and one, the credits' animation id, is read after all, as the id each new animation takes. It also listed 20 members
that one of the original's functions touches and its translation never names. Every one is a pairing of the wrong
function with an address mentioned in a comment, or an offset through a base class (Draw and Update see the object 0x28
on): none is a difference in what the port does.

**Addresses.** 184 functions had neither an address nor a `port:` marker. About 105 got their address. The Lua bindings
took theirs from `bindings.txt`, the rest came from the export, and `F_Rand` (0x66d4b0) came from the disassembly, since the
export does not show the pointer its registration passes. About 80 are the port's own, mostly helpers that read a Lua
call's arguments and the curtains' callbacks, and say `port:`, naming the original function they come from where there
is one. `GameText.cpp` is the port's in its first comment. 28 one-letter names that ran over ten lines were renamed
(`aPanel`, `aSupport`, `aTransform`, ...).

**Proof.** The field cuts by the suite's frozen baseline (identical). The addresses, the comments and the renames by the
object code: `tools/objsnap.py` found the game's 94 objects and 4,895 functions the same before and after.

## The PopCap Game Collection version (2026-11-16)

The port was made from the retail install (`C:/Program Files (x86)/PopCap Games/Bookworm Adventures Deluxe`), so it is
checked against the game on the PopCap Game Collection disc (`docs/COORDINATION.md`).

**Getting the files.** `unshield` (`E:\retools\unshield-build`) read the group `BookwormAdventuresDeluxeComponent` out of the
disc image's cabinet, which was mounted read-only and dismounted afterwards. That gave 13 files, 33 MB. They were deleted once
this was written; to bring them back, make a folder, enter it and run `unshield -g BookwormAdventuresDeluxeComponent -d . x
F:/data1.cab`. Like the install, the copy keeps nearly everything in `main.pak`.

**The files** (`games/bejeweled/tools/collection/cmpfiles.py`, and `tools/popcap_pak.py` to unpack both paks). Of the
paks' 5,157 files, 5,156 are the same byte for byte. The one that is not is `scripts/common/dprops_editor.luc`, a developer's
properties editor, where only the source path compiled into the chunk differs (another folder on PopCap's build machine).
The four cutscenes, `bass.dll`, `Flash.ocx` and `j2k-codec.dll` are the same. `properties/partner.xml` differs in its
distribution tag and offering number only; its registry key (`PopCap\WinBAD`) is the install's, so a player's settings sit
in the same place. The licence and readme differ.

**The exe is another build.** `games/bejeweled/tools/collection/cmpexe.py` puts the Collection's `BookwormAdventures.exe`
(4,765,000 bytes) beside the install's (4,760,904). Every section differs throughout, and `.text` is 4 KB longer. The port
reads 273 texts from the player's exe by address (`GameText.h`). None is at its address in the Collection's build, but 272 are
there byte for byte at other addresses. `LoadGameTexts` now looks for the texts it cannot find at their address by their
content, as Astro Pop's does: every zero-terminated run in the image, and every tail of one, matched by length and hash. The
one it cannot find is the fourth of the profile format's changes (0.0.5). The Collection words it a little differently,
and it is shown only for a profile saved before 0.0.5, from before the game shipped, which no player has. The two builds
list the same 14 format changes and so the same format, 0.1.5.

**The port on the Collection's files.** The Collection's folder was made from the staged install with its exe,
`partner.xml` and editor script put in. The suite's chapter and screens runs (`tools/check.sh`, with a screenshot of the
loading screen added) then ran on both folders. The board logs, script lines, saved profiles and all seven screenshots
are the same byte for byte; on the Collection's exe the log says 272 of the 273 texts were found by content.

What is not known: whether the Collection's own exe plays the same as the install's. Its code differs throughout, so no
function was compared and it was not run; what speaks for it is that its data is the same files. Where the Collection's
build keeps a player's profiles was not checked either: the two builds name them the same way, and no Collection save was
at hand.

## The lite mod (2026-11-16)

There is no lite mod: `LITE_MOD.md` says what was looked at and why nothing is taken. The C++ logic has no search (the word
check is a binary search over the word list, the letter deal a weighted draw, the refill a fall). The rules are in the
original's Lua scripts, which the port runs unchanged. Over the chapter run's battles under `-fast`, the update is 0.314 M
cycles an update, 49% of the process, against 0.300 M for the picture. A sampling profile puts the scripts' interpreter at 21%
of the battles' cycles and the bot's own search at 1.7%. None of the board's or the tile engine's functions reaches 0.1%. In
real play, at 100 updates a second, the battles' update is under 2% of a core.

## Leftovers and developer features (2026-11-16)

This section covers what the shipped scripts still hold from development. Each point was read from the scripts' bytecode
with `python games/bookwormadv/tools/luc.py dis <script>`. The line numbers are the scripts' own, from their debug
information. The port runs the scripts unchanged, so all of it is in the port as in the original. Apart from the
best-word placeholders, a player of the shipped game never meets any of it.

**The flags.** At its top main.lua defines `CHEATED` false (line 24), `BETA_BUILD` false (line 26) and `DISABLE_CHEATS`
true (line 29). The exe sets the last two again. It calls `main.SetDisableCheats(true)` right after it runs main.lua,
and `main.SetBetaBuild(false)` after `main.LoadInitialResources` at the end of the load (`GameApp.cpp`). Across every
script, nothing else writes either flag, so the two debug gates below stay shut. `CHEATED` is written only by the kill
cheat and by the end of a battle.

**The cheat functions.** cheat.lua is one of the startup scripts (the list in packs/default.lua) and defines 25 global
functions, each named with a leading underscore:
- Lex's and the enemy's health: set, raised, lowered, filled, made very large, or for Lex left at a quarter of a heart
  (`_lex_near_death`).
- A potion of the kind given, health, attack or purify (`_GiveLexPotion`), a level-up (`_LexLevelUp`) and full experience (`_LexMaxXP`).
- The kill (`_KillEnemy`).
- `_CompleteChapter`, which marks the selected chapter completed and fills the profile's treasures.
- `_NukeProfile`, which clears every chapter's completion and saves.
- The tile attributes: add, remove, test, and a test of their tooltips on random tiles.
- `_Particles` and `_StatusTest`, a particle burst and every ailment and effect applied at once.
- `_SetPtrs`, which sets two globals to the player and the enemy.
- `_DefaultMP`, the media player's default list.
- The two profile hooks `_debug_profile_loaded_value` and `_debug_profile_done_loading`.

The exe calls the two hooks when it loads a player's profile; the second is what unlocks the tome ("Faults the pilot found"). Nothing
in the exe or the other scripts calls any of the other functions, and neither binds a key to them. The kill refuses the
hydra and the sphinx with a debug message box that names Tab as the kill key and Delete as the way to weaken them, so the
developers' build had keys for both. The port's test commands `win`, `lose`, `set health` and `set enemy` call these
functions ("The drivers' commands and the board log").

**The best and longest words.** At the end of a battle (`EndBattle`, line 898; lines 1024 to 1067 within it) the player's
best and longest words are taken from `mRoundWords`. The best is the word of the highest value (the tile engine's
`GetWordValue`, rounded and capped at 12). The list
is emptied when a battle (re)starts (`UpdateState`'s restart, line 3046) and grows only when a word is submitted
(`SubmitTiles`, line 1799). With the list empty, `mBestWord` and `mLongestWord` get a pair of placeholders: the best is
the word for cheating, and the longest is a short accusation of it, written as one word. The pair goes to the word scroll
after a win (line 1076); after a loss, the best word goes to Lex's gravestone (line 1209). So a battle lost without a word
spelled, such as scrambling until Lex dies, shows the placeholder on the gravestone. Which other damage could end a battle
with the list empty was not traced.

`_KillEnemy` (lines 104 to 107) sets `CHEATED` and appends a fake entry to `mRoundWords` and `mChapterWords`. The entry's
word is the same word for cheating, with a value of 1. A kill made with it alone therefore shows that word as both
best and longest, where the empty list would give the accusation as the longest. `CHEATED` has one reader. At the end of a
battle (line 946) it skips `ToughItUp`, the record of Lex's health over the last five battles (`Last5HP`) from which the
scripts set the profile's `BeatdownMultiplier`, so a cheated win does not change the difficulty. It goes back to false at
line 1643. It is not an anti-cheat: nothing checks the saves or the memory.

**The stats book's layout editor.** In the stats book's `MouseDown` (statsbook.lua lines 976 to 982), a click with a
count of 3 toggles `mDebugMode`. In SexyApp a count of 3 is the middle button. The toggle needs `BETA_BUILD` false and
`DISABLE_CHEATS` false, and the stats book must not be the media player. In that mode, dragging moves the selected
creature's picture (`mNudgeX`, `mNudgeY`). A right click (a negative count) calls `WriteNudgeValues` (line 938), which
writes every offset to `scripts\nudge.lua` (line 945) as a commented `gNudgeTable`. The stats book loads
`scripts/nudge` plus the scripts' extension when it is made (lines 27 to 33). The game ships `nudge.luc`, and with
compiled packs the extension is `.LUC`, so a nudge.lua written by the editor would be read back only in a setup that has
source packs.

**The battle's debug gate.** The battle's `MouseUp` (battleengine.lua line 3489) has the same gate, plus a click count of
3 and the tutorial's `mHideLore` off. A middle click on one of the enemy's lore panels (its attacks' descriptions) sets
that attack's `mDebugMaxUrgency`. The attack classes then report the attack as ready (`CanAttack`) and give it an urgency
of 999999 once (`GetUrgency`, which clears the flag), so the enemy uses it next. While it is set, the enemy's
`DrawLorePanel` (pambasedenemy.lua line 364, behind the same two flags) draws a black box with a red label saying the
attack is forced.

**Packs and expansion hooks.** main.lua's script loading (lines 345 to 372) requires packs/default.lua first. It then sets
`EXTENSION` to `.LUA` when `scripts/packs/*.lua` finds any file, and otherwise to `.LUC` with the `.luc` files, so source
packs are preferred when present (the loader also compiles a `.lua` that is newer than its `.luc`; "Unneeded code").
Every pack but DEFAULT is required in name order: book1 to book3 and bosslist1 to bosslist3. For each pack named BOOK
with a number and not ending in `_INIT`, the book's `__Book<n>GetFullEnemyList()` is appended to `MAIN_ENEMY_LIST`.

main.lua lines 37 to 49 define five empty tables for an expansion, and they are used to different degrees:
- `EXPANSION_HOF_ARENA_DETAILS` is the one that is filled: the three boss lists fill it with the arena's bosses, and the
  hall of fame (`HallOfFame.cpp`) reads it through the functions at lines 1398 to 1414.
- The treasure screen walks `EXPANSION_TREASURE_INIT_FUNCS` and `EXPANSION_TREASURE_DRAW_FUNCS`, both empty here.
- `NeedsIntroCutscene` (line 1433) walks `EXPANSION_INTRO_INFO`, also empty.
- `EXPANSION_MM_EXTRA_BUTTONS` gets one test entry (line 1464): the tome's three images, a test label and the callback
  `main.callbacktest`, which only prints its own name. Nothing in the exe or the scripts reads the table.
- `main.NeedsMMExpansionExpansion` (line 1465) returns false, and nothing calls it.

**Covered elsewhere in these notes:**
- "Faults the pilot found": the profile hooks that unlock the tome, and the island that arena.lua draws only in 3D.
- `ENGINE.md` ("LuaSupport") and "Status": the Lua debugger console and the save points (`luatemp.dat`).
- "Coverage check": the profiler (`perf.Start`, `perf.End`), the developers' script reload and the DProps editor that
  nothing loads.
- "Unneeded code": the compiler, kept because the loader compiles a newer `.lua`.
- "The PopCap Game Collection version": the DProps editor's chunk, which records another source folder on PopCap's
  build machine, and the message for a profile saved before format 0.0.5.
- The arena is the code's "timed mode": `MainMenu::StartArena` (0x47c290) calls `main.SetTimedMode(true)`, and the app's
  `mTimedMode`, the scripts' `gTimedMode` and the profile's `TM_` keys all belong to it.

## The autosplit block (2026-11-21)

The port writes the shared autosplit block that the site's Splits window and LiveSplit read (`platform/AutosplitBlock.h`,
layout 2; `docs/WEB_PLAYER.md`, "One design for every port"), with the game id `BWAD`. The observer is
`src/LiveSplitObserver.cpp`, in the Windows and web builds. `GameApp::UpdateFrames` calls `LiveSplitObserver::AfterUpdate`
after each update, and the app's destructor calls `AppDestroyed` for the last write. The observer only reads the game: no
random draw, no game state, no save and no call into the scripts. Until something asks for the block, it only tests two
flags. On the web the page asks by calling `byopop_livesplit_block` (exported, `byopop_web_game(... EXPORTS
_byopop_livesplit_block)`); natively the `livesplit` feature of an enabled mod asks (`mods/livesplit/mod.json`, a manifest
only). `-livesplitlog=<file>` writes a line for each event.

**What it reads.** The rules live in the scripts, so the observer reads what they leave on the C++ side. That is the
player's profile (the values the scripts set: `CompletedChapters`, `Farthest*`, `Current*`, `InGame`, `BeatRegularGame`,
the arena's `TM_` keys), the game screen (`mBookManager`, with `mTimedMode` for the arena), and the battle's state
(`gBattleEngine->mState`, which `BattleEngine::SetState` mirrors from the scripts). A board is a session of one mode, from the
main menu and back to it. `mode` is 0 for the adventure and 1 for the arena; 2, 3 and 4 are Word Master, Letter Rip and Link
n' Spell when started from the main menu's tent (a mini-game played from the adventure's map belongs to the adventure).
`skill` is 0: the game has no difficulty. `progress` is the books completed (0 to 3, -1 in a mini-game). `level` is the
chapters completed, or the arena's enemies beaten. `displayLevel` is the chapter in play, `state` is the battle's state (-1
with no battle), and `tick` is the app's update count. The game's own four words are the book, chapter and stage in play,
and the session's falls. A paused board has `AUTOSPLIT_FLAG_PAUSED`, a session that continues a game has
`AUTOSPLIT_FLAG_RESUMED`, and `AUTOSPLIT_FLAG_VANILLA` is on while the mods' score key is empty (no enabled mod changes the
play).

**Events.**
- *GO*: a session's first update, when it starts a new game. In the adventure that means a player who has completed no
  chapter, got no farther than the first battle and has no battle saved. In the arena it means a new run (`TM_CurrentStage`
  -1, as `Profile::ResetTimedMode` leaves it). Every mini-game started from the tent counts. Any other session is RESUMED
  and raises no GO.
- *Level clear*: a chapter completed in the adventure (the count of `CompletedChapters` rises), or an enemy beaten in the
  arena (`TM_LastEnemyIndex` rises).
- *Game over*: kind 0 when Lex falls in the adventure (the battle enters `BE_DYING`). The adventure goes on with the
  battle again, so the session stays in play and counts the fall. Kind 1 when Lex falls in the arena, which ends the run.
  Kind 2 at a mini-game's end, when its hall of fame comes up. A mini-game left from its menu ends no game.
- *Win*: the adventure's `BeatRegularGame` becomes true (the third book's end), or the arena's win dialog comes up
  (`DIALOG_ARENA_WIN`).

The bot's run through the whole adventure with the block on (`tools/coverage/advance.sh` with the mod and the log) gave one
GO, 30 level clears and the win, with Lex's falls counted on the way. A Word Master game from the tent gave GO in mode 2
and its end. The arena's events were checked with the screen pass's arena steps (`IDENTICAL_PLAY.md`, "Every screen").

## Mods

`mods/livesplit` (a manifest only) turns the autosplit block on natively ("The autosplit block" above). No mod changes
the game yet (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones
worth making.

## Status (2026-10-26)

The adventure, the arena, the mini-games, the hall of fame, the tome, the credits and the players dialog play
natively and in the browser (where the arena was not reached, as only a script command unlocks it for a test
profile). Tiles and letters are chosen by clicking, as in the original: no widget of the exe chooses them by typed
letters (the board's keys are only 1 to 4, Enter and space; the scripts' KeyChar handlers are empty). The media
player's spectrum takes its bins at the mixer's rate (48 kHz in the browser), where the original's BASS took them at
44.1 kHz, so a tone stands a little further left. Left out: the trial and its buy screens (the port plays as bought; "Registration code taken out" above),
and the developers' tools in the exe (the DProps editor, the Lua debugger console). Notes starting `port:` in the code mark every place
where the port differs from the original.

## Where the work was hard

PopLua is Lua 5.0.2 with PopCap's changes, read from the exe since nothing documents them: its arrays and its table
constructors start at 0, `string.sub` takes a start and a length, and it has a class dialect of its own
(`docs/POPLUA.md`). The port's VM runs the game's compiled scripts as they are, and its compiler takes source in the
same dialect (the suite's 150 dialect checks), which is what script mods would need.

## Changes outside the Bookworm Adventures folders

`CMakeLists.txt` (the `BUILD_BOOKWORMADV` option), `tools/check.sh` (the `bookwormadv` suite and the unit suite's
jp2test), `.claude/launch.json` (the `bookwormadv-web` preview on port 18775) and the platform's JPEG 2000 images
(`platform/openjpeg`, registered with the framework's image loader). The performance pass added `MemoryImage::BitsSource::mMake`
(`framework/MemoryImage.h`, a `shared:` commit): a picture a game made can drop its pixels after its upload, as a
loaded picture does; off unless a game sets it.
