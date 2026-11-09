# Mahjong Escape: Ancient China: notes on the original

This file records what the comparison with Ancient Japan found in `MahjongEscapeAC.exe` (linked 2006-08-21, three
months before Japan's), for whoever reads or changes the port. China is Japan's program with other resources and a
few changes of its own, so `games/mahjongjapan/docs/GAME_NOTES.md` describes the program, its saves and its rules,
and this file only what differs. `ENGINE.md` says where China's engine differs from Japan's map, and `DEVIATIONS.md`
lists where the port's code differs from the original. Each change is listed with its address in
`re/mahjongchina/diffnotes.txt` (git-ignored, with the exes' exports).

## How the port was made

The two exes were compared function by function (`tools/compare_exes.py`): the functions paired by their calls and
their shape (`re/mahjongchina/compare/addrmap.tsv`), then for each pair the constants each side has that the other
has not (`compare/immediates.txt`) and the decompiled difference (`compare/diff/`). Most of the differences are the
classes' fields a word lower, the sizes of what is allocated and engine calls that China's compiler inlined. The
others are the game's own: other places, fonts and pictures in nearly every screen, other counts, and a few changes
of behaviour, all ported. Two kinds of change hide from a comparison of constants as sets, and both turned up late:
the same constants in another order (Board::Init loads its sounds in another order, which changes nothing but the
log; `re/mahjongchina/order.py` finds such pairs, and that was the only one) and a constant that is simply gone
(Board::Measure has no -5, so China's board stands 5 pixels lower than Japan's; the stage-2 comparison with the
original found it).

## What is different

**Dynasty Adventure** takes the place of Emperor's Adventure: 52 levels in 11 stages (stages of 3, then 4, then 5
levels; 0x410ef0), through China's dynasties, against Japan's 60 levels in 12. A stage ends after levels 2 and 6 and
then every fifth (0x411060). The rules of a level, the special tiles and how a level is dealt and won are Japan's.
The map has its own places, path and bar, all read from the stage tables in `Resource.dll`. Once every level is done
the map no longer offers to start the escape again, and a level's end is a dialog with the time, the score and the
next level's text on a scroll, without Japan's bests and records. The timed tips come at levels 3, 7 and 12 (2, 5 and
9 in Japan), and Japan's rule that skipped the tips once a level had a best score is gone.

**Classic** has 126 layouts (162 in Japan) on 14 pages of nine, and fifteen backgrounds. The in-game menu of a classic
game has no restart. The restart comes only from the second button of a classic game's end, and asks before it
throws away a game in progress. A new player's difficulty is normal (a quarter of the deal in fours; Japan's starts at relaxed), and
a player keeps 150 classic records and 52 of the adventure (175 and 60 in Japan).

**Elsewhere.** The dialogs have dragon corners (an animation on two of them), the main menu has its own buttons and
two animations, the music takes other orders on the map and while keys are in play, and the ending has eleven lines
drawn centred on their places. `Resource.dll` holds 178 layouts: 52 for the adventure and 126 for the classic game.

## The standard test commands, the fixes mod and the finishing pass (2026-11-08)

China's finishing pass is Japan's, done once for both (the same code, so the same files: `src/Bot.cpp`, `src/Bot.h` and
`src/TestCommands.cpp` are copied from Japan's by `games/mahjongjapan/tools/realplay/syncbot.sh`). `games/mahjongjapan/docs/GAME_NOTES.md`
describes each of them; this section says what differs here.

- **The standard test commands** (`docs/TEST_COMMANDS.md`) are Japan's: `state`, `goto`, `level`, `unlock all`, `set`, `win`,
  `move`, and `lose` and `seed` as errors. The levels are 52 and the classic layouts 126 (`goto classic 1` to `126`). The
  bot solves every one of the 52 stages and every classic layout. Stage 11 of China (layout 395) was the hard one: its keys
  and locks need the keys saved for the locks that hold the goal tiles, which the bot's first tries now do, and it solves in
  38 moves. An earlier note of this pass that said the solver finds no solution for it was about the first solver.
- **The fixes mod** (`mods/fixes`, `seededDeals` and `steadyClock`, on by default on the site) is Japan's with
  China's own addresses; `tools/realplay/modtest.py --game china` passes all its checks (the menu at update 260 as the port
  starts and 1550 with `-logos`; the same seed and another start time dealing the same classic board and shuffle; the panel's clock
  showing 2, 4 and 6 s after 400, 800 and 1200 updates). The native folder is `mods-mahjongchina` beside the exe and the
  web copy `mahjongchina-fixes`. The candidates that were checked and are none are Japan's too: the panel's `AddSeconds`
  never reaches its odd branch, and no layout of the 178 deals a free tile that cannot be clicked at the start (0
  unreachable). The soft lock after a Shuffle at the last pair is fixed in the port as in Japan (`Board::OnCommand`).
- **The start.** Japan's (`games/mahjongjapan/docs/GAME_NOTES.md`, "The start"), at China's addresses (`re/mahjongchina/compare/addrmap.tsv`):
  the two logo screens (2 s each) and the 0.77 s of black before them are the original's wait before the game can start, so the
  port opens at its main menu and `-logos` keeps the original's start (`tools/check.sh` passes it). The launch to the menu
  took 7.7 s and takes 1.29 s. The rest of the launch has no wait of the original's making, as Japan's.
- **Registration and links.** China has no registration code either; the PopCap and SpinTop logos at the main menu's
  corners are pictures that take no click, as in Japan, and the shared libjpeg 6b loader decodes the backgrounds (the screens
  of `tools/check.sh mahjongchina` were taken again for it: at most 3 levels of a channel, in the JPEG backgrounds only).
- **Real input.** `tools/realplay/drive.py --game china` plays the same: the menu and its help, the name, all 52 levels of
  the Dynasty Adventure to the ending by real SDL events (16.0 minutes of real time), two classic games, the focus lost and
  regained, Quit; and again in full screen. The web driver (`mahjongjapan/tools/web/realplay.js`, taken by the China page
  too) plays the same through the DOM's events.

## Performance (2026-11-08)

Measured as Japan's (`games/mahjongjapan/docs/GAME_NOTES.md`, "Performance"): the port only, windowed, in real time, with no
mods, three runs of the minute of play of `tools/check.sh mahjongchina` (the menu, a new player, Dynasty Adventure stage 1 won
by the solver's clicks, back to the map: 74 s), the processor time in the window 20 s to 50 s.

| | port (3 runs) |
|---|---|
| Launch to the main menu, real time (`tools/realplay/perfload.py`, 5 runs) | 1.29 s median (1.23 to 1.35); with the original's start (`-logos`, 3 runs) 7.70 s (7.69 to 7.78) |
| Peak working set | 172.3 MB median (172.1 to 180.9) |
| Peak private bytes | 170.9 MB median (170.4 to 172.5) |
| Processor time in the window | 5.0 s of 29.9 s: 16.7 % of one core (16.1 to 22.1 %) |
| Processor time, whole run | 10.9 s median (10.7 to 14.0) of 74.4 s |
| Processes and threads | 1 process, 42 to 50 threads |

The original was not measured, for Japan's reasons: its exe runs only inside Windows Sandbox under `tools/origrun`, and a run on
the host would play its music on the speakers. Its two logos and the black before them hold the menu back about 6.5 s (0x42f650 in Japan): the
port takes 7.7 s to the menu with `-logos` and 1.3 s without.

## Coverage check (2026-11-08)

The first whole coverage check of Ancient China (`docs/COVERAGE.md`, "A game's whole check"), made after Ancient Japan's
(`games/mahjongjapan/docs/GAME_NOTES.md`, same title), whose runs, scripts and settings it reuses. origrun's `--coverage`
recorded which of the original's functions ran, in 12 runs on the host (`games/mahjongjapan/tools/coverage/run.sh` and
`mkscript.py`, which write and start both games' runs; China's places are a block in `mkscript.py`; the hits are in
`build/coverage/mahjongchina/hits/`). The adventure's and the classic game's runs used origrun's virtual clock; the
tour, players, Esc, resume and ending runs ran on the real clock with the silent sound stand-in. Every audio session was
muted from its start and none made a sound.

- **The screen tour**, **players**, **Esc in every state** (`escprobe`), **the resume question**, and **the ending**
  (`chinaend`: China's map has a button that shows the ending again, and its restart box).
- **The escape adventure**: the stages before 46 in four runs (`advA` to `advD`) and stages 46 to 52 with the ending in
  `advF`, the clicks of every stage from the port's solver. **Stage 11 was not played**: the solver gave up on its deal
  twice within its limits (160 tiles, a quarter of them fours). It is an ordinary stage with no start tip, so its code
  is what the other stages run; it stays an open item.
- **A whole classic game** on layout 441 (144 tiles), as in Japan: `classicsave` starts it with the deal's seed
  pinned (origrun's `--seed` and `--patches` replace the two `GetTickCount` calls of Board::SetLayout and Board::Start in
  the copy with 12345), makes four moves and leaves by the in-game menu, which saves it; `classicplay` resumes the
  save, plays the other 68 pairs of the solver, takes the result box's next-layout button, and ended at its 400 s
  timeout like Japan's (the next layout began and the last clicks did not reach Quit).

**The deal is checked here as well.** The saved game holds the deal that the port's solver
(`mahjongchina_dealtest --solve 441 12345 25`, `MJ_CLASSIC=1`) produces for the same seed: 0 of the 144 tiles differ
in kind. China's default share of fours is 25 percent (0x19, `User` at +0x18), not 0 as in Japan: with 0 the saved
board differed in 139 tiles, which is how the difference was found, and the first classic runs played the wrong deal's
moves.

`games/mahjongchina/coverage.txt` sets the game's own code as two ranges, 0x401000-0x4171f0 and 0x41dea0-0x4374f0, 1274
functions. The runs reached 1050 of them, the port defines 985, and 354 are listed as left out on purpose.

**The port's comments carry Japan's addresses.** The port was made from Japan's, so the comment above a function says
where Japan's exe has it, not China's. `tools/coverage.py` reads them through `re/mahjongchina/compare/addrmap.tsv`
(`addrmap` in coverage.txt, columns 1 and 2). The map pairs the two exes' functions (4460 rows when this began); at
first the report showed 200 functions reached in the original with no counterpart and 103 neither reached nor
defined, many of them simply missing from the map. `games/mahjongchina/tools/pairgaps.py` adds the pairs the map
lacked: the two builds keep their functions in the same order, so the functions between two mapped neighbours of one
build are paired with those between their images in the other (in order, and by size or callees when the counts
differ), and a call-graph pass pairs what is left by the callers and callees that are mapped. It added 112 rows (110
by the gaps and the call graph, 2 by hand: `TextEdit::Update` 0x428010, where China's compiler inlined
`Object::IsActive`, and the tip box's destructor 0x408c80, which the map had paired with another destructor of the
same shape). `python -I games/mahjongchina/tools/pairgaps.py --write` redoes it on the original map.

**China's exe puts the device layer and libraries inside its game ranges.** The JPEG and PNG decoders, the Direct3D
and DirectDraw wrapper and other device code sit between the game's functions (0x42ba30-0x42d4a0, 0x42d720-0x430480,
0x4328a0-0x432ed0, 0x433050-0x4342d0 and more), where Japan's exe keeps them outside. They are `none` lines with the
reason that the host and the platform replace them. The compiler-made functions and those the host replaces were
sorted for Japan, and their China addresses are the 110 `none` lines translated through the map.

**Reached in the original with no counterpart in the port: 200 at first, 0 at the end.**

- **Left out on purpose, as `none` lines with a reason (146 in all, 3 of them the ones it started with):** the
  compiler-made functions (scalar deleting destructors, static initializers and their `atexit` registrations,
  `CString` and `std::string` helpers, the crash handler), and the platform's (`WinMain`, the window, the registry,
  `LinkButton`, the Direct3D path, the sound mixer), and the device layer above.
- **Placed:** the inlined functions got their Japan address in the comment above the function that holds them, as
  in Japan (Object's defaults, the manager's list add and remove, the layout name copy, the keyed copy's row loop
  0x42e960, the user and shared folder getters); China-own addresses in `GameDialogs.cpp` and `.h`
  (the level-done scroll's functions) and two comments in `Board.cpp` and `MahjongApp.cpp` were changed to Japan's, since
  the map would translate a China-own address wrongly. The options box's `OnInit` (0x4152a0 in China), whose
  `SetUserFlag(true)` the compiler inlined, is a `none` line: the port defines it in `Options.h` under Japan's 0x401060.
- **Missing logic: the two of Japan's,** now in China's port: the window's key slot leaves full screen on Escape
  (`MahjongWidget::KeyDown`), and the exe's version string for the logo screen (`MahjongApp::Init`).

**Never reached and not defined in the port: 103 at first, 0 at the end:** two vtable-only stubs, a default virtual that
returns false (0x401040) and a scalar deleting destructor (0x432870), are `none` lines; the rest are the same sorts
as above.

**Defined in the port, never reached: 92.** Every one is a function whose Japan counterpart (through the inverse map)
was read again against the export in Japan's check (74 R rows, reachable by a play the runs did not make, and 18 U rows,
reachable by none); all matched but two, which China had as well and now has fixed, like Japan:

- **0x42a340, `Layer::AppActive`** returned the display's flag where the original asks the manager's active flag, so
  the pause box a game shows when it starts with the window inactive never appeared. `Layer::AppActive` now returns
  `mManager->GetActive()`.
- **0x42ed70, `Image::SaveBmp`** left the header's size-of-image field zero; `Put32(34, aSize)` writes it.

The China-only functions (the map's ending button, `MapView::OnTimer`, `OnLButtonDown`, `MapLayer::ShowEnd`) were
unreached until the last runs: `advF` (the last stages and the ending) reached two, and the classic game's second run the
third. The plays that none of the runs made are Japan's list: a held press (a drag of a
volume slider, of the name box), a double click, a modifier key (Ctrl+C, X, V, Z), Shift+Tab, the right click on the
board, the no-moves box with its three buttons, OK and Cancel clicked in the new-player box, the player list's scroll
arrows, the styles and backgrounds boxes, the display options, minimize and restore, the classic list built from scratch,
a deal that the solver gives up on, and a saved game with a wild-linked tile.

**Candidates for taking out unneeded code: 18**, the same as Japan's U rows that China has (none was removed):
the deleting destructors of `Dialog`, `SavedGame`, `MahjongGame`, `Deal`, `Layout`, `User` and `PlayerList`;
`Object::SetVisibleByParent`, `GetDirty` and `SetDirty`; `Font::LoadKeyed`, `DrawWrapped`, `SetTabStops` and
`GetTabWidth`; and `Animation::Stop`, `Hold`, `StepLoop` and `StepPingPong`. The reasons are in Japan's notes.

**Not in the address map: 147 comment addresses.** The port's comment gives Japan's address, and the map has no China
partner for it: 95 of them are functions of Japan's exe (most of them of 16 bytes or fewer, thunks and getters, and the
list operations of the splash, the display and the manager) and 52 point inside a function. The report cannot say whether
the original ran them. This is the weakest part of China's check, and a map made by a better pairing would shrink it.

**Open items.** Stage 11's play; the plays above; and the 147 unmapped addresses.

**What it cost.** The 12 runs took about 30 minutes of machine time (400 s of it the classic game's second run), at most
three game processes at a time. The sorting and the re-read were Japan's work (two Sonnet agents; tokens not counted),
reused through the map; China's own work was `pairgaps.py`, the 146 `none` lines, the classic game's deal, and the runs
of the stages and the ending.
