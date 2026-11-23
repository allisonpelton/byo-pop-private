# Pizza Frenzy port

A port of Pizza Frenzy (Sprout Games, published by PopCap Games and GameHouse; © 2006 Sprout Games) to the shared
platform layer, natively (Windows, SDL2) and in the browser (Emscripten). The code is a translation of the game's exe,
found by decompiling it with Ghidra; the game's files (`PizzaFrenzy.saf`, which holds the pictures, fonts, screens,
cities, levels, stories and sounds, the loose `splash/` and `properties/`, and `PizzaFrenzy.exe`, whose string table
the game reads its texts from) come from the player's own copy and are never in the repository.

Pizza Frenzy is not a SexyAppFramework game. It runs on Sprout Games' own first-generation engine, the C++ namespace
`engine`, which Word Harmony also runs on; the port shares Word Harmony's translation of it, `engines/sprout`
(`docs/ENGINES.md`, "Sprout"; Word Harmony's `docs/ENGINE.md` describes it). Pizza Frenzy's exe has a different build
of the engine from Word Harmony's, though it was linked a day earlier: its node classes have more virtual slots, its
application, scene and screens are wired with sigslot's signals, its sounds are Ogg Vorbis streams. The engine grows
with what Pizza Frenzy's build adds, and where the two builds' versions of a function behave differently the game
sets which (below, "The signal build").

## The binary

<!-- private -->
`PizzaFrenzy.exe` (the Steam copy, 1,620,898 bytes) is PopCap's DRM wrapper with the game exe appended unencrypted
after the marker `!popcapdrmprotect!` (offset 352,256; the PE image starts at 352,674 and runs to the file's end).
`tools/extract_game_exe.py "<install>/PizzaFrenzy.exe" -o re/pizzafrenzy/bin/PizzaFrenzyGame.exe` carves it out
(1,268,224 bytes).
<!-- /private -->

The game exe (v5.8.30.1) was linked on 2006-02-21 by Visual Studio .NET 2003 and has RTTI only for three engine
classes. Its layout, in link order:

- 0x401000-0x461680: the game, 1,845 functions (354 KB): the controller and its states, the cities and their tiles,
  the vehicles, the customers and their orders, the pizzas and toppings, the three modes, the special events, the
  stories, the pizza designer and the topping book, the users, the settings and the high scores.
- 0x461680-0x49c000: Sprout's engine, about 1,200 functions: objects, the scene graph, screen layouts and their
  widgets, fonts, particles, the sound and music systems over DirectSound, the archive and the resource manager.
- 0x49c000-0x4ac440: libogg and libvorbis (the platform decodes Ogg Vorbis itself).
- 0x4ac440-0x4bcc50: the C runtime.
- 0x4bcc50-0x4d0a00: the engine's Windows layer: the application and its window (class "Gatsu"), the registry, the
  crash handler, the DirectDraw display and its surfaces, the image loader.
- 0x4d0a00 on: libpng 1.2.5 and zlib.

The game's texts are in the exe's string table (ids 104-105 and 201-235: the titles of the modes and their
descriptions, the level ends, the frenzy names, the credits); everything else a player reads is in the archive's XML
files. The window is 800x600 (the controller's configure, 0x4050d0, says so; `properties/partner.xml`'s 640x480 is the wrapper's), at 30 frames a second.

## The export

Everything derived from the game is in the main checkout's git-ignored `re/pizzafrenzy/`: the carved exe (`bin/`),
the archive extracted for reading (`saf/`, by Word Harmony's `tools/extract_saf.py`), the Ghidra export (`gh/`), the
vtables, the function match with Word Harmony (`match/`) and the symbol files applied to the export (`symbols/`). The
Ghidra project is `pizzafrenzy_game` in `E:/retools/projects`.

```
CLASS_RECOVERY=0 build/native/rundesk.exe bash tools/ghidra_import.sh re/pizzafrenzy/bin/PizzaFrenzyGame.exe \
    pizzafrenzy_game re/pizzafrenzy/gh                                    # import, analysis, first export
python -I games/pizzafrenzy/tools/vtables.py                              # re/pizzafrenzy/vtables.txt, vfuncs.txt
build/native/rundesk.exe bash games/pizzafrenzy/tools/ghidra_pass.sh     # the virtuals, the calls, the symbols
python -I games/pizzafrenzy/tools/port_names.py wordharmony re/pizzafrenzy/match/wordharmony_port_names.jsonl
python -I tools/funcmatch.py re/pizzafrenzy/bin/PizzaFrenzyGame.exe re/pizzafrenzy/gh/functions.jsonl \
    re/wordharmony/bin/WordHarmonyGame.exe re/wordharmony/gh/functions.jsonl re/pizzafrenzy/match/wh.match.jsonl
python -I games/pizzafrenzy/tools/enginematch.py re/pizzafrenzy/match/wh.match.jsonl \
    re/pizzafrenzy/match/wordharmony_port_names.jsonl re/pizzafrenzy/match/engine.match.jsonl \
    re/pizzafrenzy/symbols/wordharmony_engine.jsonl                       # then ghidra_pass.sh again
python -I games/pizzafrenzy/tools/classmap.py 401000 461680               # the game's classes, for reading
python -I games/pizzafrenzy/tools/fn.py <addr>...                         # functions of the export, compacted
```

Ghidra finds almost no vtables in a program without RTTI, and a virtual function only ever called through its vtable
is never disassembled; `vtables.py` finds 393 vtables (runs of code pointers in `.rdata` whose first entry the code
refers to; a slot may point anywhere in the code, to `_purecall` or to a tiny body the linker folded, 0x4d0ca0 among
libpng's), and the pass creates their 923 functions and gives the 4,400 indirect calls their argument counts.

The engine's functions are named from Word Harmony's port through the two exes' function match: `tools/funcmatch.py`
pairs the bodies that are the same after masking addresses (692) and their callees (151), and `enginematch.py` adds
the pairs that a string only they use, a vtable's slots or the order of their calls make (929 in all, 105 of them
functions Word Harmony's port names). There is no Mac build of Pizza Frenzy. Iggle Pop's Mac build
(`re/igglepop/mac/`) shares the engine's build and its RTTI names the engine's classes and their bases
(`engine::Component`, `Container`, `Scene`, `DrawContext`, `Application`...), which the shared engine knows by Word
Harmony's names (`Node`, `Container`, `Window`, `Graphics`, `App`); its symbols carry no function names, so
`tools/macmatch.py` finds nothing to carry over.

A virtual function Ghidra never disassembled shows in the export as a `PTR_LAB_` vtable; `re/pizzafrenzy/labels.txt`
lists such functions (the signal handlers the code passes as data, and the vtable slots that pointed at no function,
found by reading every `PTR_LAB_` and `PTR_FUN_` table of the export from the exe), and the pass creates them.

`fn.py`'s compacted output was wrong until 2026-11-14 in two ways, both fixed: it dropped `break;` and `continue;`
lines (its filter of unused declarations took them for one), and every constant store to a `uStack_` temporary
(which is how Ghidra shows arguments built on the stack, a key's time or a point). Where a function's floats pass
through the x87 stack Ghidra shows no value at all (the splash state's load bar share, stored into its frame
argument's slot); `llvm-objdump -d` (in the toolchain's `bin/`) shows them.

## The signal build

Iggle Pop's Mac build has the same build of the engine as Pizza Frenzy's exe, and its RTTI names the classes the
listener build (Word Harmony's) does not have, so the shared engine takes those names: `Scene`, `GenericApplication`,
`State` and `StateMgr`, `GenericScreen` (on `ScreenLayout`), `ScreenMgr`, `Transition`, `FadeTransition` and
`SlideTransition`, `PositionInterpolator`, `GenericGameOptions`, `SoundMgr` and `SoundMgrDocHandler`, `ImageButton`,
`CheckBox`, `EditBox`, `ListBox`, `Table`, `BaseDocHandler`, `UserInfo`; the game's own `UserGameData`,
`GameMusicPlayer` and `GameMusicClip` too. Classes both
builds have keep Word Harmony's names (`Node` for `engine::Component`, `Button` for `ButtonBase`, `LayoutHandler` for
`ScreenDocHandler`). The game sets `engine::gSignalBuild` (`engines/sprout/Build.h`) in its application's constructor;
a shared function that behaves differently in the two builds reads it:

| Function | The signal build's difference |
|---|---|
| `Node::isVisible`, `setEnabled`, `isEnabled` | visible also needs the drawable flag; enabled is the node's own mouse flag, not masked |
| `FadeContainer::FadeContainer`, `fade`, `update` | no name; a fade in starts from clear; a fade out can end with the container removed |
| `ScreenLayout::ScreenLayout`, `loadLayout` | no mouse flag cleared, no name; the key actions dropped and the screen named after its file first |
| `LayoutHandler` | the signal build's elements and attributes |
| `Button`, `ImageButton`, `CheckBox` | the mouse through the node's event slots, a state field, missing images made from the up image |
| `ImageNode`, `TextItem` | a clip and the image's size; the text's colour, shadow and word wrap; no names |
| `Image::setPivot(mode)` | a mode other than the centre puts the pivot back at the corner |

The signal build's parts of `ScreenLayout` (key actions, the default and cancel actions, connecting and disconnecting
while shown) and of `Node` (connect and disconnect, the mouse mask, removing a listener) are additions the listener
build never calls. The screen's slot 0x148 (0x465d60) would connect a button's signal, but nothing reaches it: the
layout reader hears buttons through `listenTo`, so it is left out. The players (`Players`, the registry key's `users`
subkey, a subkey a player and the key's default value the current one) and the local scoreboards are engine classes;
the online scoreboard and its posting are outbound and left out (see "Removed from the port").

Exception texts (a manifest that does not parse, an entry registered twice) stay literal in the code, as the shared
engine's already are: they are the original's messages to its developers, not texts a game shows.

The sounds and the music are opened through the application's stream (0x461a50, 0x461b70, 0x461c90), which reads
the loose file or, failing that, the archive's entry, as the exe does: every sound the game plays is in the archive.

## Removed from the port

Two buttons lead out of the game, and both are removed after their screen's layout loads, with every node only they
use:

- **The main menu's game link.** The Steam copy names its store page in `properties/partner.xml` and has the link's
  button images in `splash/`, so the Steam original shows the button at the bottom, between the Sprout logo (bottom
  left) and Quit (bottom right), nearer Quit. The port shows the exe's own look for a copy that names no link
  (0x43b0c0's other branch, the empty `noGameLinkGrp`): the logo and Quit stay in their corners and nothing moves.
  The menu state's `gameLink` action goes with it.
- **The high scores' global toggle.** It sat at the left of a row of three under the table, with Done in the middle
  and Clear Scores at the right; its labels and the global view's titles go with it, along with posting a score and the
  global post screen. Done and Clear Scores move from 0 and 200 to -100 and 100, keeping the original's spacing with
  the pair centred under the table.

The Sprout logo on the main menu is a button too, but it only shows a line of text in the menu's text box, so it
stays.

## Deviations

- **The player's pizzas** (`userPizzas.xml`) are kept in the user folder, as every port keeps its writable files;
  the original writes them beside its exe. A missing file is read as no pizzas, and the first pizza gets id 0.
- **A pizza's name with a double quote** is saved with an apostrophe in its place, so that the file stays valid XML;
  what the original does with one has not been checked yet.
- **The global score post's loading frame** stays, empty: the original makes the post's screen in a stage of its own
  (the load bar at 0.8), and the port keeps that frame with nothing in it, so the loading takes the original's frames.
- **The game's end goes to the menu** at once: the original opens the global score post's dialog there first (a state
  of its own, until the player closes it), which the port does not have.
- **The concentration mode's lock is fixed:** an event that changes the orders while a wave's orders still show (every
  customer dimmed) set the seconds they show for to 0 in the original, so they were never turned over and the level
  never ended (0x45b4f0); the port keeps the seconds then (`IDENTICAL_PLAY.md`, "Deviations left").

## The fixes mod

`mods/fixes` (docs/MOD_ROADMAP.md, "The fixes mod") holds each fix as a toggle, read through `Platform::FixOn`;
natively `-fix=<id>` turns one on. With the mod off, or absent, the port is the original, which every test and the
frozen baseline run.

- **steadyClock** (on): the original times each frame by the machine's clock (QueryPerformanceCounter, the frame's
  seconds capped at 0.33, `App::frameSignalled`, 0x4bd210), so a late frame moves every wait, van, timer and tip
  further, and the same input never plays the same game twice. With the fix the engine has the clock `-fast` gives
  it, one frame period (a thirtieth of a second) a frame, while the frames still wait for the machine's clock; the
  sounds' voices end on that clock too. The game reads time nowhere else (the seed from `time` at launch aside), so a
  real-time game with the fix is, frame for frame, the game `-fast` plays.

Looked at and left as the original's (the survey of 2026-11-22), each judged intended or with no effect a player sees:
a customer under the cursor or picked counts down without turning impatient or hanging up (one function, 0x452060, for
both: a picked customer's pizza is on its way, and one under the cursor hangs up once it leaves); a tip's size follows
the seconds since the last delivery (0x455ff0), not the customer's own wait; a wave with more events due than places
drops the extra one, whose timer starts again (0x45a9a0); a chameleon's change does not count toward the seconds the
customer waited, which nothing reads; the bonus round's seconds left are truncated for its time bonus (0x460040); a
coupon after one that is removed slides a frame later; and an event that asks for no pizza resets the combo (0x45a300),
as the Thief's and the Crank Caller's do. Two differences of the original are the port's already, as it cannot
reproduce them: the Clown's change of a topping no kitchen has reads past its list's end in the original (0x449850),
and the port takes the list's first; and the toppings' order, which follows their addresses in the original, follows
the manifest's in the port (`IDENTICAL_PLAY.md`, "Explained: the toppings' order").

## Progress

Where it stands (2026-11-15), the paragraphs after this one being the history: every mode plays, natively under
rundesk and on the web. The bot (`move`, "Test commands") plays from a first launch (it names the new player) through
the menu, the stories, the new toppings, the van choice, the levels and their summaries and the bonus rounds: the speed
mode through the first five cities into the sixth in 3000 moves, the memory and the concentration modes from the menu
to Riverdale's third level in 800, and the designer and the editor make, name, save and load a pizza. Runs with the same
seed give the same transcripts. `tools/suite/suite.sh` is the check suite (the speed, memory and concentration modes,
each twice, and against a frozen baseline when there is one; about three minutes). The page check
(`tools/web/pagecheck.js`) passes 11 of 11: losing the focus shows the pause menu and stops the clock in the port and
on the page alike, and the game stays paused when the focus comes back (to compare with the original). Not reached
yet: the last city and the game's won story; a game over only through the `lose` command. The logic phase is under
way (`IDENTICAL_PLAY.md`): the speed mode's first three levels, its bonus round and the second city's start play out
the same as the original's, value for value and draw for draw.

The engine's signal build is translated as far as the splash screen and the layouts need: the application's signals
and run loop, the scene, states, the generic screen with its transitions, the screen manager, the settings, the sound
manager, the layout reader's elements and their widgets, the players and the local scoreboards. The game has its
application's configure and init, the tile manifest (every entry the file has), the player's progress and save, the
music player, and the loading state's stages for the manifest, the pause menu, the confirmations, the restore dialog
and the music. The signal build's drawing is in: the node's colour, colour mode, smooth type and blit effect, the
blitters' colour modes and smoothing, scaled and copied images and drawing into an image, and the text's draw with its
shadow. The signal build's font lays its glyphs out with the whole gap on each side (0x47ead0), where the listener
build leaves half.

The front end is in: the loading state's stages for the main menu, the options, the designer's unlock notice, the
player select and the new player's entry, and the high scores with their reset confirmation; the dialog state (a
screen shown in a second screen manager over the darkened screens beneath) and the menu state, whose actions reach
the game's start, the city select and the topping book (the designer and the editor came later: `PizzaEditor`,
`PizzaEditorState`; the city editor of the original's development, `startPizzaDesigner`, is left out). The engine has the edit box, list box and table with their models, the text typer, and the generic
option, player select and high score screens. A copy with no player opens the new player's entry over the main menu;
a name typed there makes the player, the menu greets them, and the high scores and the options open and close.
The levels and the game screen's frame are in: the level manifest and the cities, the player's statistics, the city
select (each postcard is the city's first level drawn by the game screen into a canvas, then shrunk onto the card),
the game screen with its city map, tiles and decorations read from the city files, the HUD's layout and the splat
factory. The loading state now runs to its end and opens the menu state. With a player whose highest city is the
fourth, the city select shows the first four cities open and the others locked, each card showing its city. Next: the
speed mode and the game state (customers, deliveries, the timer and the score), then the other modes, the story, the
new toppings, the start and end of a level, and the banners.

The story, the new topping, the van choice and the topping book are in, and a city click goes through them to the
level's set-up, where the speed mode comes next. The player's toppings are a map keyed by the topping's address in
the original and the port alike, so the topping book's pages and the titles' points follow the order the toppings
were made in memory; the logic phase checks that this order matches the original's.

Left for later: the particle effects' systems (0x47bc40), among them the tip jar's coins and the bursts. The banners
(the message banner 0x42bb30, the speed bonus banner 0x44fb70) and the in-and-out effect (0x42c980); the splats'
definitions from splats.xml and their update (0x473b90); the city map's and the tiles' slots that only play reaches; the
game screen's active flag (+0x137); and most of the player's statistics, whose fields keep placeholder names until play
names them. The smooth blit's sub-pixel and rotated loops (0x4c47c0, 0x4c9f50-0x4ca2d0) are read as a bilinear sampler
and are not checked against the original's pixels yet. The options' version line (0x4669d0) shows the layout's own text:
the original formats its own file's version (0x4bdb20: FileVersion and SpecialBuild under the user's language and code
page 04B0, or "no version info"). The Steam copy's launcher has no version resource and only the wrapped game image has
one, so what the Steam original shows depends on how the launcher runs the image; an origrun run settles it in the looks
phase.

## Test commands

The standard commands of `docs/TEST_COMMANDS.md` (`-script`, `-commands`, the web test page), in
`games/pizzafrenzy/src/TestCommands.cpp`. Each answers with one line, `STATE {...}`, `MOVE {...}` or `MOVE null`,
`OK <command>` or `ERROR <command>: <reason>`. They play the speed, memory and concentration modes.

- `state`: the state line, below.
- `goto menu`, `goto highscores`, `goto options`, `goto toppings`: from the main menu (or a screen it opens), the menu
  state's own actions `highScoreDone`, `showHighScores`, `options` and `viewToppingBook`.
- `goto editor`: the menu's "design your own pizzas" button (`editPizza`): the pizza editor opens on the player's last
  pizza (a new one when the player has none), or the notice of the designer's lock when the player has too few toppings.
- `goto designer`: from the menu, or while the new topping screen shows: the designer's introduction (the screen of
  the new topping state that `NewToppingState::action` shows when the player gets the fifth topping), whose button
  opens the editor and whose end goes on to the level.
- `goto citySelect`: `PizzaFrenzy::showCitySelect` (0x4023d0) for the speed mode.
- `goto speed`, `goto memory`, `goto concentration`: `PizzaFrenzy::startGame` (0x40c700) of the mode at the first city;
  the game left in the mode is restored, as the menu's restore does. The bot's menu move presses the button of the
  mode the game was last in (the speed mode's from launch).
- `level [speed|memory|concentration] <day>`, `level [mode] <city> <level>`: from the main menu (numbers from 1; the
  day counts the levels of every city; the speed mode when none is named): the mode's saved game is put at the level with no score and `startGame` runs from it, so the story, the new
  toppings and the level start come as for any level; a bonus round is started the same way (the city's last level).
- `unlock all`: the last city is the player's furthest (`UserGameData::setMaxCity`), every topping the player lacks is
  at level 0 and no longer new, and the data is saved. The stories still show, since the furthest day is not touched.
- `set score|tips|money|happiness|ordersToGo <n>`: in a level: the stats' score, tip jar money, tips earned, the
  happiness (through `GameMode::addHappiness`, so the low warning follows) and the orders left.
- `win`, `lose`: in play. `win` has the waves done, the customers, the picks and the pizzas on their way cleared and
  every pizza ordered counted as delivered, so `checkLevelEnd` (0x456ac0) takes the prize blimp's path at its next
  update; `lose` takes the happiness away, which it ends the game on.
- `seed <n>`: `engine::crtSrand`, as `-seed=N` seeds the game.
- `botmode speed|memory|concentration|editor`: the mode whose button the bot's `move` clicks on the main menu (the
  mode the game was last in until it is sent), or with `editor` the design button (it opens the editor only for a
  player with five toppings); it changes nothing in the game, so a case recorded from a first launch can play the
  memory or the concentration mode, or design a pizza, with real input alone.
- `move`: the bot's next move, below.
- `memreport`: the framework's, with the engine's pictures and sounds added (`AddMemReportHolders`).

`state` prints `screen` (splash, menu, citySelect, highScores, story, newTopping, levelStart, game, pause, summary, or
the open dialog: options, toppings, players, newPlayer, deletePlayer, resetScores, restoreGame, quit, exitGame,
unlockNotice, pizzaName, dialog; designer: the introduction of the pizza designer; editor: the pizza editor; other for a state not listed), `mode`, `city` and `cityName`, `level` (in the city, from 1),
`day`, `frame`, `levelOver` (the level's end, the summary), `over` (the game has ended: it was played and the menu
(or the high scores a game over shows) is back), then `score`, `tips` (the tip jar), `tipGoal`, `money` (the tips
earned this level), `happiness`, `ordersToGo`, `timer` (the stats' seconds, which every mode keeps), `delivered`,
`ordered` and `combo`; with a game on, `phase` (intro, play, blimp, end), `picks` and a `board` of `customers`
(`cell`, `x`, `y` the point to click, `topping`, `picked`, `wait` seconds left, `clickable`, `event`, `chameleon`,
`hidden`: the order is behind the question mark, `age`: its place among the orders placed, the oldest 0, -1 for none),
`kitchens` (`cell`, `x`, `y`, `topping`, `clickable`, `vanOut`) and `tips` (`x`, `y`, `amount`, `waiting`). In the
concentration mode `showLeft` is the seconds the orders still show (0 once they are hidden).

In the editor and the pizza name dialog the state has an `editor` object instead of the stats and the board: `pizza` (the
shown pizza's place in the player's list, from 1), `pizzas` (the list's length), `id` and `nextId` (the number the next
pizza takes), `name`, `picked` (the topping on the cursor), `buttons` (the toppings the player has), `middle` (the point
of the middle of the pizza on the screen: a click within 128 pixels of it puts the picked topping), `nameEntry` (the
text of the name screen's box) and `toppings` (each `topping`, `x`, `y` from the middle of the pizza). The player's
pizzas are in `userPizzas.xml` of the user folder, written when the editor is left and read when the game loads.

In a bonus round the state has `phase` (intro, arrive: a pizza and its buttons come in, play, score: the toppings of
the target are scored, end) and a `bonus` object instead of the board: `timeLeft` and `pizzasLeft`, `playing` (the
player may put toppings), `selected` (the topping on the cursor), `targets` and `placed` (each `topping`, and `x`, `y`
the point of the player's pizza that the target has it at, or that it was put at), and `buttons` (`topping`, `number`
left to put, `x`, `y` the point to click). `levelOver` is true from the round's end phase.

The bot reads the state and nothing else, and draws from no generator. In the introduction it clicks to skip it. In
play, with no order picked it clicks a tip that waits, else picks the clickable customer with the least wait left (the
first of two); with an order picked it picks more customers of the same topping while the pick limit allows
(`GameStats::getPickLimit`, 0x41a670, read without its insertion), then clicks a kitchen that makes the topping. A
customer of a special event is left alone, since the events' pizzas are not served yet. Every move is a `move` step
and a `click` step and waits 2 updates; the blimp's flight and the level's end are `MOVE null`.
In the memory mode the customers must be served in the order they came, so the bot picks only the oldest customer
not picked yet (`age`; the police's own customer, which no list holds, first) and waits when that one takes no click.
In the concentration mode the orders are dimmed and take no click while they show, so the bot waits (reading the
hidden order's topping from the state, as a player who remembered it) until they are hidden.

In a bonus round (the introduction is skipped with a click, as above) the bot waits while a pizza comes in and while
one is scored, and in play takes the first topping of the target pizza that has no topping of its kind put for it (the
target's toppings in order, the ones of a kind matched with the ones put of that kind in order): a click on the
topping's button (`topping`) while another or no topping is on the cursor, then a click (`place`) on the point of the
player's pizza where the target has it, which scores a perfect topping. It never takes a topping back with the right
button. The round's end ends in the summary, which is pressed through as below.

On the screens before play the bot answers too, each move a real click or key on what a player presses, the point
read from the screen's own node (the centre of its bounds, with its parents' places added, in game points). A screen
that is still coming in gets `{"what":"wait","input":[{"wait":30}],"updates":30}`, and so does the splash screen
while it loads (the port's splash never waits for a click). The menu gets a click on the node `SpeedGame` (the Speed
Mode button, `speedMode`); the city screen a click on the postcard button of the furthest open city (`city`; the
`move` step before the click is the hover a postcard needs); the story the Escape key while its `skipString` shows
(`skipStory`), else a click on `continueButton` (`next`); the new topping screen a click on the first button of
`leftButtons` (`newTopping`); the level start, with three or more toppings, a click on `doneButton` (`levelStart`)
once every van has a topping, else, with no topping picked up, a click on the first topping button no van has
(`topping`) and with one picked up a click on the first van with none (`van`, the node `vanButton<n>`). The dialogs
have no move (`MOVE null`), except the pizza name dialog, below.

The designer's introduction gets a click on its first button (`tryDesigner`, "Try it now"). In the editor the bot puts
three toppings on the pizza, clicking the first topping button (`topping`) and then three places along the pizza's
middle line, 40 pixels apart (`place`); then, while the name is not BOT PIZZA, a click on `editCaption` (`editName`),
which opens the name dialog; there it types the name into the box, a Backspace for each letter in it first (`typeName`,
real key steps), and clicks `done` (`nameDone`); and in the end a click on the exit button `menu` (`exit`), which writes
the file. A pizza that already has the name (a later run's, read from the file) is left at once. A topping dragged on
the pizza (`PizzaTopping` slots 0xa0 and 0xd0) is not used: the placed toppings are not enabled, as in the original, so
a topping goes on by a click on its button and a click on the pizza (a right click takes one back).

The summary is pressed through like the story: the Escape key while its `skipString` shows, else a click on
`continueButton`; the upgrades' stories after it are met the same way, and then the next level's story, new topping and
level start come as above. A game over ends the run as `over` (the menu or the high scores is back; the game won
story is not tried). A run from launch goes through a city's levels, its bonus round and its summary into the next
city (the `-seed=1` run of 3000 steps reaches city 6).
