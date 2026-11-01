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
3D mode is `LuaApp::Is3DMode()`, read from and written to `Is3D` as the original does (on when the value is missing, as when
the original's 3D test passes), set by the Options dialog's box, and what the scripts and the game's C++ ask. With `Is3D` 0
the pilot's battle shows no island, like the original. Under origrun on this machine the original turns `Is3D` 1 back to 0
and draws in software; origrun is getting a 3D-mode option. Not done: the original's question at exit after 3D was switched
on during a session ("Hardware Acceleration Confirmation", 0x53c120).

The buttons of the battle's menu, the quit dialog and every other `GameDialog` had their end caps stretched, and the new
player's name sat in a stretched frame. The original draws a stretched `FadeButton` (0x442b10, its byte +0x12b) and that frame
(0x482aa0) with `Graphics::DrawImageBox` (0x4f5ca0 and 0x4f5c70: the corners a third of the cel at their own size, the edges
and the middle tiled); the port drew one stretched image. Fixed (2026-11-01). Every other caller of the two box functions
in the port already drew a box.

The hall of fame's arena list showed 0:14:40 for the authors' run to the ninth stage, where the original shows 0:14:50: the
port's table of the ten default runs had 88000 hundredths of a second for it, the original's (0x4632a0, MakeTimeList) 89000.
Fixed (2026-11-01); the other nine, and the ten default bonus runs, match.

### Timing in the pilot

Both sides are scripted by the game's update count (`tools/pilot/gen.py`). The port runs a script line "N ..." at the start
of update N, before the widgets update, so its screenshot shows update N-1; origrun's `at N` holds the original once update
N has run. The port's lines are therefore one update later than the original's (2026-11-01). That alone moved nothing that
follows a click, since the click and the shots moved together.

What remains is the original running one or two updates behind the port after a click, and further behind around a load,
and it varies by an update from frame to frame. `tools/pilot/lag.py` measures it by matching each of the original's pictures
with a port screenshot taken at every update. In the attacks clip every original frame matches a port update with 14 to 216
pixels left on a half-size picture of the battle (the Trojan Spearman's sampling, above), one update behind and jittering by one. In the Enter! transition every
original frame is a port frame exactly (0 pixels), two updates behind until the chapter loads and 10 to 12 behind after it:
the curtain finishes only once it has been drawn at its last frame (`mDrawn`, set in its Draw, 0x4413b0, and read in its
Update, 0x4411d0), so every screen behind a curtain begins at an update that depends on how often the game draws, on both
sides. In the credits the first name shakes (`Rand() % 5` each update after a creature's `causedamage`): in the pilot's run
the original's shot shows the port's shake of two updates earlier, but `lag.py`'s run, which draws the port at every update
(each screenshot is a draw), matches the original's shot exactly (0 pixels over the names at the same update). So the random
draws agree, and the difference is when the credits began.

Part of the lag looks like origrun's: a click is posted while the original's window thread is suspended, with 60 ms of sleeps
(`post_click`), and a held capture takes its own time, so the original's clock runs on and it catches up with updates it
does not draw. Asked of the coordinator (2026-11-01). Until then these items are timing, each with its lag measured: the
attacks clip, the Enter! transition, the credits' shake, and the Attack button's sparkle in word_rain (an effect that starts
with the last letter's click). The title clip's frames 18 to 30 are the
original's loading screen, shown for as long as its loading thread takes; the port loads in slices of 20 ms between frames,
so it shows it for a different time. That runs on the wall clock on both sides and is not compared.

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
