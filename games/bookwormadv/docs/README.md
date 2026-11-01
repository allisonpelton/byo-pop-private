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
Flash player for the cut scenes (`swf/`). `docs/ENGINE.md` maps the exe's regions to these folders and explains how
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

## Mods

None (the user's rule: no mods before every PopCap game works in the browser). `MOD_IDEAS.md` lists the ones worth
making.

## Status (2026-10-26)

The adventure, the arena, the mini-games, the hall of fame, the tome, the credits and the players dialog play
natively and in the browser (where the arena was not reached, as only a script command unlocks it for a test
profile). Tiles and letters are chosen by clicking, as in the original: no widget of the exe chooses them by typed
letters (the board's keys are only 1 to 4, Enter and space; the scripts' KeyChar handlers are empty). The media
player's spectrum takes its bins at the mixer's rate (48 kHz in the browser), where the original's BASS took them at
44.1 kHz, so a tone stands a little further left. Left out: the trial and its buy screens (the port plays as bought),
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
(`platform/openjpeg`, registered with the framework's image loader).
