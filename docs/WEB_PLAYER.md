# How the web player works

A plain-language tour of what a browser loads when you open one of the ports, for example
`bejeweled.html`, and how its pieces talk to each other. Native builds share the same game code; only
the outer layer differs.

## The pieces

| File | Made by | What it is |
|---|---|---|
| `bejeweled.html` | Emscripten, from `web/shell.html` and the game's `web/page.*` | The page itself: the top bar, the game's canvas, the start, import and exit cards, and the side panels. |
| `bejeweled.js` | Emscripten (`emcc`) at link time | The loader and runtime that the page's script calls `Module`. It fetches and starts the game, provides a file system, and connects SDL2 to the browser. `web/modpanel.js` is appended to its end. |
| `bejeweled.wasm` | the compiler, from the C++ code | The game: the translated game code, the SexyAppFramework port (`framework/`), the platform layer (`platform/`) and SDL2, compiled to WebAssembly. |
| Your game files | you, through the import card | The original game's images, sounds, music and data, copied from your install folder or a .zip into this browser's storage. They are never uploaded and never part of the build. |

Big Money (`bigmoney.*`) and BookWorm (`bookworm.*`) have the same three files.

## What happens when the page opens

1. The page shows "Starting…" and loads `bejeweled.js`, which downloads and compiles `bejeweled.wasm`.
   The game's `main()` does not run yet (the build sets `INVOKE_RUN=0`).
2. **Storage.** The page mounts the browser's IndexedDB at `/persist` (Emscripten's IDBFS) and copies
   everything stored there into memory: `game/` (your imported game files), `userdata/` (saves, high scores,
   settings, `mods.json`), `mods/` and `replays/`. Saves that only reached the localStorage backup when the
   last tab closed are put back first (`byopop_userdata_restore`).
3. **One copy at a time.** A Web Lock (`navigator.locks`) makes sure only one tab runs the game, because
   both would write the same saves. A second tab shows "Already open" and starts by itself when the
   first one closes.
4. **Mods.** The Mods panel finishes any removals that were waiting for this start.
5. **Terms.** Until the player has agreed to this game's terms (below, "The terms"), the terms card comes
   first, before any game files are imported or loaded. The card is off for now, and the pages must not be deployed
   until it is back.
6. **Game files.** If there are none yet, the import card asks for the install folder or a .zip. Only the
   folders the game reads (`images`, `data`, `sounds`, `music`, `properties`) are copied. The browser's
   own `DecompressionStream` unpacks a .zip. A game whose files are in a PopCap pak (Bookworm Adventures'
   `main.pak`, XORed with 0xF7) names it in its page's `paks`: the import unpacks it into the game's folder
   (`Module.byopop.pak.read` in `web/modpanel.js`, the format of `tools/popcap_pak.py`) and does not store the pak,
   so the browser keeps one copy of the files, the same tree native staging gives the game. A folder or .zip whose pak is
   already unpacked (the staged `assets/<game>/`, or a zip of it) is taken too: there the game's folders stand for the pak.
   `tools/paktest/paktest.js` checks the reader under Node. The rules (which folder is the game's, which files are
   copied, the paks) are in `web/gameimport.js`, which the listing page uses too ("Adding games from the listing page").
7. **Play.** Browsers allow sound only after you click or press a key, so the page waits for "Play". Then
   it calls the game's `main()` with folder arguments (`-gamedir=/persist/game -userdir=... -modsdir=...`),
   plus `-gameseed=N` for a `?seed=N` in the address, `-placeholders` for `?placeholders=1` and `-nomods` for
   "Play without mods". From then on,
   the game runs one frame per browser animation frame, as SDL drives it.
8. **Leaving.** Quitting from the game's menu shows the exit card. Closing the tab works too: the game's
   saves are written to IndexedDB as they happen, and a small backup goes to localStorage, because a
   closing tab cannot wait for IndexedDB.

## The top bar and side panels

The Mods button shows once the game's files are in the browser (the user, 2026-11-03: "Don't show the mods button unless
files are uploaded"), and never on the page of a game whose port loads no mods yet ("Don't show a mods button for games with
no mod support"): those games are the `BYOPOP_WEB_NO_MODS` list in `cmake/ByopopWeb.cmake`, which sets the page's
`MODS_SUPPORTED`, and a port leaves the list when it starts loading mods.

The bar and panels are ordinary HTML around the canvas, not part of the game. They reach the game in
four ways:

- **Function calls.** `platform/WebApi.cpp` exports a few C functions (`byopop_mods_open`,
  `byopop_mods_state`, `byopop_mods_edit`, `byopop_mods_rescan`), and `framework/ModMenu.cpp` exports
  `byopop_menu`, `byopop_restart` and `byopop_quit`. The page calls them with `Module.ccall`. The game calls back
  through `Module.onModsChanged` when the mods change. `byopop_quit` is for the Close of a game's window: it calls
  the app's `QuitAsGame` hook (`SexyAppBase.h`), which by default shuts down at once and which a game overrides to
  leave as its own Quit does (Bejeweled Twist plays its goodbye and shuts down when the voice ends). It returns 1
  when a game took the call and 0 when none runs; a build without the export is closed with `byopop_restart(1)` as
  before. The shutdown reaches the page through `Module.onGameExit` either way.
- **The screen's shape.** The stage is 4:3 unless the game calls `Module.onGameScreen(width, height)`
  before it makes its window: BookWorm's mod "widescreen" gives 854 x 480 (`SidePanel::TellPage`). The stage
  takes that shape and SDL sizes the canvas to it.
- **Files.** Adding a mod writes its files into `/persist/mods` and syncs them to IndexedDB. The game
  reads them at its next start.
- **Reloads.** "Restart game", in the game's F2 menu, reloads the page. A mark in localStorage tells the
  new page to start straight away.

The panels:
- **Mods** (`web/modpanel.js`) adds mods from a folder or .zip and removes them. A folder or .zip may hold many
  mods, a whole mods folder for one: each folder in it with a `mod.json` or one of the game's folders (`images`,
  `sounds`...) is added as its own mod, and the panel names what it left out. Turning mods on and off,
  their order and their settings live in the game's own F2 menu, as decided for the mod menu
  (`docs/MOD_MENU.md`). The panel also offers "Play without mods" when a mod stopped the last start.
- **Game files** imports the game again, or forgets the copied files. Saves and mods stay. The game runs only
  once in a page and must not have its files changed under it, so once it has started, both close it first
  (as the mod menu's Restart does, saving the game in progress) and go on in a reloaded page: the import
  card, which can go back to the Ready card. The page keeps the game's life as one state (`gameState` in the shell, the user,
  2026-11-07: "Find any implicit state machines in the website code and make them explicit"): waiting, starting, running, closing (the page asked it to close),
  closed and stopped (an error), with the moves between them listed in `GAME_MOVES`; the shell makes no other.
- **Controls** lists the game's controls in our own words, and lets the player choose their own keys and gamepad buttons
  for the game's keys (see "View, controls and play settings", below).
- **Record**, there only while the site setting "Clip recording" is on, records a clip of the game (see "Clips", below).
- **Settings** is the site's settings (see "Site settings", below), the same controls as the listing page's.
- **Sound** turns the page's sound off or on, below the game's own volume and mute options (see "Site settings").
- **Fullscreen** puts the stage in fullscreen. **F11** does the same in every game's page, and F11 again (or Esc) leaves it (see
  "Accessibility").
- A game can add panels of its own: Big Money's page also has **Replays**, for watching and importing recorded games.

## Why `bejeweled.js` is minified, and what is Bejeweled-specific about it

`emcc` writes `bejeweled.js` when it links the game. It is almost entirely Emscripten's standard
runtime:
- the code that downloads and starts the `.wasm`;
- the virtual file system (MEMFS, and IDBFS on top of it);
- SDL2's browser side: the canvas and WebGL, mouse, keyboard and touch events, and Web Audio;
- the small JavaScript pieces written inside the C++ code (`EM_JS`/`EM_ASM` blocks, such as the
  focus handling in `framework/SexyAppBase.cpp`);
- the list of exported functions;
- `web/modpanel.js`, appended with `--post-js`.

It is minified because `tools/build.sh` builds in Release mode by default. That runs Emscripten's
JavaScript optimizer, which shortens names and removes whitespace to make the download smaller. A
Debug build (`BUILD_TYPE=Debug tools/build.sh web`) writes readable JavaScript.

Very little of it is Bejeweled's own:
- its file names;
- its exported function list (Bejeweled adds `_fflush` for the Node.js test runner);
- whatever `EM_JS` code its C++ contains.

`bigmoney.js` is nearly the same file. Everything that makes the game Bejeweled is in `bejeweled.wasm` (the
code) and `bejeweled.html` (the page's texts and its import rules, from `games/bejeweled/web/page.js`).

## The HTML shell

`web/shell.html` is every game's page template. The page code exists once; what differs between the games
comes from the game's `web/` folder:

| File | What it holds |
|---|---|
| `games/<game>/web/page.js` | Required. `PAGE`, the game's settings: its name for `modpanel.js`, its storage folder and lock (never changed once players have saves there), the folders and files copied from its install, the marker files that make a folder its install, the import card's and Mods panel's texts, whether it takes `?seed=` and keeps replays, and optional functions: how to tell that a game in progress was saved, extra start arguments, a panel being opened. The full list is at the top of the shell's script. It runs first, in the shell script's scope, so its functions can use the page's (`web()`, `$()`, `openPanel()`) and its folders (`USER`, `REPLAYS`). |
| `games/<game>/web/page.css` | Optional. The game's accent colour (light and dark) and its brand mark (`.brand .mark`), and the style of its own panels. |
| `games/<game>/web/page.html` | Optional. The game's own panels: each `<aside class="panel" data-button="Name">` gets a button on the bar. Big Money's Replays panel is one. |

| Game | Storage folder | Lock |
|---|---|---|
| Bejeweled | `/persist` | `bejeweled-deluxe-instance` |
| Big Money | `/bmpersist` | `big-money-deluxe-instance` |
| BookWorm | `/bwpersist` | `bookworm-deluxe-instance` |

IDBFS names a game's IndexedDB database after its storage folder, so these names are where the players' saves
are: they stay as they are. A new game takes a folder and lock of its own.

At configure time `byopop_web_game()` (`cmake/ByopopWeb.cmake`) puts the game's parts into the shell at its
`byopop:game-js`, `byopop:game-css` and `byopop:game-panels` markers and its name at `@BYOPOP_TITLE@`. `emcc`
then copies the result to `bejeweled.html` and puts the `<script>` that loads `bejeweled.js` at its
`{{{ SCRIPT }}}` marker.

The shell holds:
- the bar, the canvas and the cards (terms, import, start, already open, exit) as HTML and CSS;
- its script, which boots in the order described above: storage, the instance lock, import, start, the
  `?seed=`, `?silent=` and `?placeholders=` address options, and the exit card;
- the panels' containers, which `web/modpanel.js` fills.
- a `<!-- byopop:footer -->` line, where the build puts the shared footer.

**The game's cursor on the page.** A SexyAppFramework game hides the browser's cursor over the canvas and draws its
own image cursor into the frame; so that the rest of the page does not fall back to the browser's arrow, the shell
polls `byopop_cursor()` every 100 ms (`framework/WebCursor.cpp`, read-only: 0 for the browser's cursor, otherwise a
number that changes with the image, which `byopop_cursor_copy()` then puts in `Module.byopopCursor` as RGBA with its
hotspot). The image is drawn at the size the game's cursor has on screen, at most 128 px, and set as the CSS cursor
of everything but the canvas (`.game-cursor` on `<html>`); it is the game's pointer while the mouse is outside the
canvas, and nothing is set when the game's custom cursors are off or a game has no cursor images.

**The site's cursor.** The user, 2026-11-03: "pull custom cursors out of the user's uploaded games so that they can select one
to use across the whole site (except for inside other games)", and "If the custom cursor is disabled in a game, could the site's
custom cursor still display over it?". Once a game has started, its page asks it for its set of cursors at each 100 ms poll until
the game has set its pointer: `byopop_cursor_set_copy()` (`framework/WebCursor.cpp`, read-only like the calls above) copies its
image for every kind from `CURSOR_POINTER` to `CURSOR_WAIT` (`mCursorImages`, whether or not its custom cursors are on) to
`Module.byopopCursorSet`, each with the hot spot the game draws it at, and the page keeps the six kinds the site shows (below). The page keeps them as PNG data URLs, at their own size (at most 128 px), in
`web/cursors.js`'s localStorage item `byopop-cursors`, one set per game. The Settings (the listing page's tab and every game page's
panel) have a "Site cursor" choice: the browser's own, or one of the games' sets, each shown by its pointer, hand and dragging
cursor, then the circle-slash and wait where it has them (an animated one by its first frame). The text cursor is not shown or
compared: every game's is the same I-beam on a canvas of another size (the user, 2026-11-04: "text cursor is the same
everywhere"), and the set chosen still shows its own in text fields. Each image is kept cropped to the pixels that show, with its
hot spot taken from the crop and a hash of the cropped pixels (`crop` and `picture` in `web/cursors.js`, from the game page's
`takeCursorSet`), so the same design drawn on canvases of different sizes (Bejeweled's 54 by 54, Alchemy's 37 by 37) keeps one
picture and makes smaller CSS cursors; sets kept before cropping are cropped once as a page of the site loads (`upgrade`), with no
need to start each game again. Games whose sets look the same share one entry named with all their titles, "Alchemy and
Big Money" (the user, 2026-11-04: entries that look the same are one entry, and differences nobody can see do not split
one). Two sets look the same when their pointers and their hands do: laid on each other by their hot spots (or with one moved a
pixel), the pictures are no more than two pixels apart in size and differ nowhere by more than 40 of 255 in any colour drawn over
white and over black, but for a stray few pixels up to 64 off (at most 1 in 50 of the pixels that show, and at least 2: Twist's
hand, as the JPEG 2000 decoder gives it, has 4 of 419 pixels 47 off Alchemy's, which had kept Twist apart; the user, 2026-11-08:
"The Twist cursor set is still shown separately from the other games in the list"), and the frames match (`setsAlike` in `web/cursors.js`, on pictures the page decodes once each). The dragging cursor, the circle-slash and the
wait cursor are pictured but not compared: Bejeweled's dragging cursor is an open hand without the shadow, since its mask is a
smaller image than its picture and the game, like the original, leaves a mask of another size out, and the user wants Bejeweled
with the others ("One-line fix it is, then", 2026-11-04). The games that use PopCap's usual cursors make two entries: Atomica and
Dynomite, whose cursors are green, and the rest (Alchemy, Bejeweled, Bejeweled Twist, Big Money, BookWorm, Mummy Maze, Pixelus,
Rocket Mania and Seven Seas; BookWorm's pointer has colours up to three steps off Alchemy's, and Bejeweled Twist keeps the same
pointer and hand as JPEG 2000, a few pixels up to 33 off, with faint edges its GIF masks add, the user 2026-11-07). An entry pictures the set of the member that stands
for it, the chosen one or else the first by title (Alchemy's), and the site shows the chosen game's own cursors. The sets are
still kept one a game, choosing the entry keeps one member's id, and the entries form again from what is kept, so a set that
changes leaves its group. The choice,
`byopop-site-cursor`, applies to every page of the site (`.site-cursor` on `<html>` and one style element), each cursor where the
site means what the game means by it (the table below): the pointer everywhere, the hand on links, buttons and other controls
that can be used, the text cursor in text fields, the dragging cursor on a cap being dragged, the circle-slash on a disabled
control and the wait cursor while the site is busy. On a game's page a game whose own cursor is showing keeps it (`.game-cursor` turns the site's rules off); where it shows
none, its custom cursors off or none of its own, the site's cursor shows over the game too, by the kind of cursor SDL gives the
canvas (a hidden cursor stays hidden). The images come from the player's own copy and stay in this browser; forgetting a game's
files forgets its cursors (`ByopopGameFiles.forget`). A game's cursors are there only once it has been started in this browser:
each game keeps its cursor images in its own formats and files, and the game itself is the one place that puts them together.

**No "Deluxe" in the site's names** (the user, 2026-11-04: "Let's not use the name Deluxe anywhere in names on the site,
since it is supposed to specifically refer to the original desktop versions and might draw the ire of EA"). Every name the site
shows for a game is its TITLE in `games/<game>/CMakeLists.txt` (`byopop_web_game`) or its row in `BYOPOP_WEB_UPCOMING`, none of
them with "Deluxe": the tiles, the page and tab titles, the copyright pages, the chat, the cursor choice and every message. A game
page's tab keeps its own title while the game runs (SDL's web backend would put the game's window title, the original's own
name, there), and its "already running" message names the TITLE, not `PAGE.shortName`. What still says "Deluxe" is what finds
the original's files: the install and Steam folder names the save import looks in (`platform/saveimport.json`), the folder names
the game import recognises (it ignores the word), store links, and the game's own art and texts from the player's files.

**Each game's cursors, and where they show** (the user, 2026-11-04: "Some games have cursor sets rather than a single cursor
used during all of gameplay. We should figure out in what game contexts a particular cursor is used and make them contextual on
the site too"). Read from the ports' code, the calls that set cursor images (`SetCursorImage`) and the ones that pick the cursor
(`SetCursor`). No game adds kinds of its own: every set uses the framework's (`framework/SexyAppBase.h`), and nearly all of them
only the first four. The framework itself picks the hand over a widget that asks for it (buttons, links, a list's clickable rows,
`Widget.cpp`, `ListWidget.cpp`), the text cursor over an edit field (`EditWidget.cpp`), the dragging cursor while a slider's thumb
or a dialog's title bar is dragged (`Slider.cpp`, `Dialog.cpp`) and the pointer everywhere else; it never picks the wait cursor.
The names that sound like cursors in Bejeweled Twist and Pixelus (`IMAGE_CURSOR_YELLOW1` to 24, `RING`, `MULTIBULBS`,
`MULTIPULSE`, `CLICKHERE`, `CLICKNOW`, `BWCURSOR`, Pixelus' aim cursors) are sprites the game draws on its board, not mouse
cursors. None of the games' cursor images is animated.

| Game | Its cursor images (pointer, hand, dragging, text) | Others | Where the game itself changes the cursor |
| --- | --- | --- | --- |
| Alchemy, Atomica, Bejeweled, Dynomite | `cursor1`, `pointer2`, `overhand`, `typer` | none | nowhere: the framework's places only |
| Big Money | `cursor_pointer`, `cursor_hand`, `cursor_dragging`, `cursor_text` | none | the dragging cursor on its own sliders' thumbs |
| BookWorm, Rocket Mania, Pixelus | `cursor_pointer`, `cursor_hand`, `cursor_dragging`, `cursor_text` | none | nowhere |
| Mummy Maze | `pointer`, `hand`, `dragging`, `text` | none | nowhere |
| Seven Seas | `cursor1`, `pointer2`, `overhand`, `typer` | `cursorshovel`, in the circle-slash's place | on the board: the pointer, the hand over the button a tutorial step points at, the shovel while a bonus is on; the pointer on the map |
| Bejeweled Twist | its pointer, hand and dragging; no text cursor | none | the pointer when the board goes or a tube takes the focus |
| Bookworm Adventures, Volume 2 | its own pointer, hand, dragging and text | none | the hand over a tile that can be picked, the battle's buttons while they can be used, the menu's items and (Volume 2) a book or Begin on the book screen; the pointer elsewhere, as screens, dialogs and curtains open; its scripts can pick either |
| Bonnie's Bookstore | none fixed: one cursor (`CURSOR_CUSTOM`) made from the original's 32 by 32 bitmap whenever the game asks for a new one | | as the game asks; the set has no pointer, so the site gets no set from it |
| Mahjong Escape: Ancient China, Ancient Japan | none: the browser's own hand over buttons and wait while a screen changes | | |
| Cosmic Bugs, Water Bugs | none: they hide the cursor | | |
| Ning Po, Noah's Ark, TipTop, Word Harmony | none | | |

The site maps a game's cursors by their meaning, and only where a site context means the same:

| Site context | The game's cursor | Without it |
| --- | --- | --- |
| everywhere else | pointer | the browser's |
| links, buttons and other controls that can be used | hand | the browser's hand |
| text fields | text | the browser's text cursor |
| a cap being dragged | dragging | the game's hand |
| a disabled control | circle-slash (no ported game has one: Seven Seas' is its shovel and is not kept, `OTHER_MEANING` in `web/cursors.js`) | the pointer, as before |
| the site busy: a game page's script loading, games being added, a save backup, restore or import (`ByopopCursors.busy`) | wait (no ported game has one) | as before |

The sizing arrows have no place on the site, and the games' other contexts (a tutorial's button, the shovel's bonus) have none
either, so with today's games the site shows what it showed before: the four kinds are the whole of every set. The circle-slash
and the wait cursor are there for a game that has them.

**Animated cursors.** None of the ported games has one; Banana Bugs will (`docs/POPCAP_GAMES_SURVEY.md`, its "Animated cursors on
the site"). Browsers animate no cursor image, so a stored kind may carry `frames` (the images after the first, at most 31, each a
PNG with its hot spot like the first) and `ms` (how long each shows, 40 ms to 2 s). Where the chosen set has frames,
`web/cursors.js` follows the element under the mouse and, while the site's rules give that element an animated kind, sets each
frame in turn as its inline cursor on one timer, which also changes the cursor while the mouse stays still. It gives the element
back when the mouse leaves it, when another rule applies (a press, the site turning busy) or a game's own cursor shows, and runs
no timer while the page is hidden or for a set without frames, so a page pays nothing for it otherwise. The frames, like every
cursor, are at most 128 px: the game page draws a larger one smaller. Filling `frames` from a game is Banana Bugs' part: its
cursor class would hand the framework its frames, and `byopop_cursor_set_copy` and the game page's `takeCursorSet` would copy and
keep them.

**The footer** is the page's last row, across its width (the user, 2026-11-07: "a true footer"): the game's copyright notice
in one sentence, its `COPYRIGHT` up to "All rights reserved." (the trademark lines are on the page the link opens), then a
**Full terms →** link, which opens the game's copyright page (`<game>-copyright.html`, next to the game) in a new tab. The
markup is `web/footer.html` (the build puts the notice at `@BYOPOP_NOTICE@`) and its style `web/footer.css`. Every link to the site's own
licence texts (`licenses/`), terms and copyright pages, opens a new tab with `target="_blank"` and
`rel="noopener"` (the user, 2026-11-08: "libopenmpt in the music pane doesn't open in a new tab"), so a game or the music
never stops for one; tools/linkstest.js (in check.sh's unit-web) fails on any that does not, in the pages, the scripts, the
site's text and the built pages.

**The copyright page** is made from `web/copyright.html`. It has the game's copyright notice, a statement that
the player is unofficial, non-commercial and comes with none of the game's files, and the license of every
library the page is built from, each in a collapsed section. The PopCap Games Framework's is first, with the
acknowledgment its license asks for. The license texts are `web/licenses/<name>.txt`: a title line, a blank
line, then the text.

At configure time `byopop_web_shell()` in `cmake/ByopopWeb.cmake` writes both files:
- `build/.../<game>-shell.html` is the game's shell with the footer at that line and the style before
  `</head>`. That generated file is what `emcc` gets as `--shell-file`.
- `build/web/<game>-copyright.html` is the copyright page.

Each game sets its own name, its notice (the copyright and trademark lines of its readme) and any libraries of
its own: `byopop_web_game(<target> TITLE "..." COPYRIGHT "..." [LICENSES <name>...])` in its `CMakeLists.txt`
(BookWorm: `LICENSES libopenmpt`). A web host serves the copyright page with the game's page.

### The terms

Before a game's files are imported or loaded, its page shows the game's terms with a checkbox ("I own a licensed copy
of ..., and I agree to these terms") and a Continue button that stays disabled until the box is ticked. The terms say
that the player owns a licensed copy and loads the files from it (a never-bought trial is not one), keeps the files to
themselves, plays for no commercial purpose, gets no rights in the game from the terms, takes the player as is, and
removes the files if they no longer own a copy. They are a record of the player's agreement, not a lock: nothing in a
page can check a purchase.

The text is `web/terms.html`, one file for every game. `byopop_web_shell()` fills in the game's `TITLE` and
`COPYRIGHT` and puts it on the terms card (at the shell's `<!-- byopop:terms -->`) and in the copyright page's Terms
section (`@BYOPOP_TERMS@`). The page keeps the agreement in localStorage under `byopop-terms-<PAGE.id>`, with the
first 12 hex digits of the SHA-1 of the filled-in text as its version: each game asks on its own, and any change to
the wording, or to a game's name or notice, asks again. Where localStorage fails, the agreement lasts until the page
closes. Boot shows the card, and the import, the start and "Play without mods" refuse to go ahead without the
agreement.

**The listing page asks too** (the user, 2026-11-03: "Make it go through an agreement for each game uploaded but allow it
to be done on the front page"). "Add games", a folder dropped on a game's tile or on the logo, and a folder of many games
("Adding games from the listing page") add a game's files only once its terms are agreed to. Before the queue adds a game
whose terms the player has not agreed to at their current version, the page shows that game's terms in a dialog
(`web/addgames.js`): the same filled-in text with its line on where to buy the game, the same checkbox and Continue, and
"Don't add this game". The build writes each listed game's text and version into `terms-games.js` from the same filled-in
terms as the game's card, and Continue keeps the same `byopop-terms-<id>` entry, so the game's page does not ask again.
Each game is asked on its own, one after another as the queue reaches it; one box never covers several games. A game left
out (its "Don't add this game", or Escape) is not written, its row says so, and a line under the panel names the games
left out, as it names the folders that hold no game. The recycle bin asks nothing: it only forgets files, which the
terms themselves ask of a player who no longer owns a copy.

**When the card skips itself** (the user, 2026-11-03: "We need a way to hide the terms during dev work that doesn't involve
manually switching them off and on"). Every build has the card, and no build option switches it off. `web/terms.js`
(`ByopopTerms`, loaded by the listing page and every game page) decides, for the game pages' card and the listing's
dialog alike, that a page skips it in two cases, and in both it keeps no agreement:

- On a development host: a page served from 127.0.0.1, localhost or [::1], from a name that ends in `.localhost` or
  `.test`, or opened as a `file://` page. A deployed site never has those hosts, so local work in the Browser pane,
  headless browsers on localhost and Node harnesses never meet the card, with nothing to switch. `?terms=1` brings the
  card back on such a host, to look at it.
- With the site's test key: a page opened with `#testkey=<key>` in its fragment hashes the key with SubtleCrypto
  (SHA-256) and compares it with the hash the build baked in. On a match it skips the card for the rest of that tab's
  pages (sessionStorage, `byopop-testkey`, holding the hash). The key goes in the fragment because a browser never sends
  the fragment to a server, in the request or in a Referer, and `terms.js` takes it out of the address bar before
  anything else runs on the page, so the history keeps no copy either (the fragment's other parts, such as the listing's
  `#mods`, stay). A wrong key, or a build without a key, shows the card as usual. `?terms=1` wins over the key. A key
  in the query (`?testkey=`) is not read.

A page that skipped the card for either reason, and any page on a development host even with `?terms=1`, counts no play
and reads no counts (`docs/PLAY_COUNTER.md`), so tests never add to the counts.

**The test key** is the first line of `secrets/web-testkey.txt`, in the checkout's root, which git ignores: it is never in
the repo or a published snapshot. `python tools/testkey.py` makes one (a random 32-character key; it never overwrites one
that is there), and `tools/setup_worktree.sh` copies the folder into a new worktree. The web build reads the file at
configure time and bakes only its SHA-256 into `terms.js` (configure again after making or changing it); a build without
the file has no test key, says so in a configure line, and its deployed card cannot be skipped. An agent that tests the
deployed site reads the key from the file and opens the pages with `?silent=1#testkey=<key>`, never writing the key into
chat, a commit or a doc.

**Deploying** needs nothing more for the terms than the test key's file in the checkout that builds the site, if agents
are to test the deployed pages. The card is on in every build.

### Where to buy

Under the terms, on the card and on the copyright page, one line says where the game can be bought digitally ("Where
to buy Big Money digitally, as of October 25, 2026: Steam, EA app", each store a link that opens in a new tab)
or that it is not for sale as of a date. The stores are in `cmake/ByopopStores.cmake`, one shared file for every
game: `set(BYOPOP_BUY_<target> <YYYY-MM-DD> [<store> <url>]...)`, the date the stores were checked and each store
that sells the game. A game with a web page and no entry stops the configure with a message. Only stores that sell
the game new and digitally under the publisher's license are listed, on their own trusted sites: the publisher's
store, platforms like Steam, established game stores. Key resellers and second-hand sales are not. A store that still
sells a game it no longer has a page for is linked by the address that puts the game in its cart (Shockwave's
BookWorm). The file records
which stores were checked; check them again before a launch, and change the date with the result. The line is not
part of the terms' version, so a change of stores asks nobody to agree again.

### The listing page

`build/web/index.html` lists the finished games, and the ones still to come ("Coming soon", below), so that the site's root shows them instead of a directory listing: a
tile for each, with the game's `TITLE` and a badge (its initials on a colour of its own from `BYOPOP_WEB_BADGES`, in the
shape of a bottle cap, drawn by the page, never from the game's files; "The bottle caps", below) linking to its page (a game still to come has no page, and its tile is no
link); opened with `?silent=1`, it passes that on to the
links. Its header has the logo at the size it is drawn, 210 by 64 pixels (the user, 2026-11-02: "Make the logo bigger on the
front page"), narrower when the window is. Its heading, "Games", is there for screen readers only and names the list of games
(the user, 2026-11-03: "remove Games from the front page header"). `byopop_web_index()` in `cmake/ByopopWeb.cmake` makes it from `web/index.html` at configure time, after every
game has been added. Which games it shows is the `BYOPOP_WEB_FINISHED` list in that file: the coordinator adds a game
there at its final merge, and a name in it with no web page stops the configure (unless the game's `BUILD_<TARGET>`
option is off). Below the tiles it has two tabs: Settings (`#settings`; "Site settings", below), which `web/settings.js` draws,
and Music. Players ("Player profiles", below), which `web/players.js` builds from the records in `web/profiles.js`, and Saves
("Save backup" and "Importing saves from an installed game", below) are sections of Settings, the first two (the user,
2026-11-09: "Fold players and saves into the settings menu"; see "The tabs", below); the build copies those files next to the pages (`BYOPOP_WEB_SHARED_JS` for the ones every page loads, `BYOPOP_WEB_INDEX_JS`
for the listing page's own: `players.js`, `saves.js` and `saveimport.js`, with the table `platform/saveimport.json` written beside them as `saveimport-table.js`).

**Coming soon.** The user, 2026-11-02: "Add tiles for the upcoming games to the site and have them display disabled and say
coming soon. Knock out all the research on where to buy and release date in advance." Among the finished games, sorted with them by
name or by release date, the listing has a tile for each game still to come: the `BYOPOP_WEB_UPCOMING` list in
`cmake/ByopopWeb.cmake`, `<target>|<title>` rows beside `BYOPOP_WEB_FINISHED`. It holds the game with a page not yet merged (Bejeweled
Twist) and the ports still to make (`docs/COORDINATION.md`), each under the title of its Windows
release and the target its port uses. The list is the coordinator's, and a game's final merge only moves its target from that list
to `BYOPOP_WEB_FINISHED` (its title then comes from its page): its badge (`BYOPOP_WEB_BADGES`), release date
(`BYOPOP_WEB_RELEASED`, with its source and a confidence in the comment above each row) and where to buy it (`BYOPOP_BUY_<target>`
in `cmake/ByopopStores.cmake`, where the 2026-11-02 check of Shockwave, Steam and EA's store is written down) are there already. A
row with an empty target or title, a game listed twice or in both lists, and a game still to come without a `BYOPOP_BUY` entry
of the shape a finished game's needs (the date checked, then a store and a non-empty page for each), stop the configure.

Such a tile is the finished one's badge, title and year, but not a link: `<div class="tile" role="group">` named "AstroPop,
2004, coming soon" (`aria-label`), with "Coming soon" in the place of the files' state. It is dimmed by its colours rather than by
opacity, so that its text keeps the page's contrast in both themes: the card on the page's background with a dashed edge in the muted colour (5.7:1 on the page's background in the light theme,
7.3:1 in the dark one, so the card's edge stays visible), the
name in the muted colour, and the badge desaturated (`filter: saturate(0.35)`, which keeps its lightness and so the white
initials' contrast). Its "where to buy" links are always shown, so that a visitor can get the game ready for when its page comes;
they are the card's only controls. The page's scripts (the files' state, the Players and Saves tabs, the chat's table of games)
take only the finished games' links, so the coming-soon tiles have no `data-game` and no chat tag.

**Sorting.** Above the tiles is a "Sort by" choice (a `<fieldset>` with a `<legend>` and four radio buttons, **Name**, **Release
date**, **Play time** and **Plays**, and a fifth, **Most popular**, by every player's plays, when the site has a play counter:
`docs/PLAY_COUNTER.md`), hidden until the page's script has run. Name is the page as built, the alphabetical order of the titles (so Bejeweled comes before Bejeweled 2 and Bejeweled 3; a title's "Deluxe", which no title on
the site has any more, would be left out of the order). Release date puts
the oldest game first. The order comes from the game's row in the `BYOPOP_WEB_RELEASED` table of `cmake/ByopopWeb.cmake`
(`<target>|<date>`, the date as YYYY, YYYY-MM or YYYY-MM-DD, with the source of each row in a comment above it), which is the
first release of the Windows edition the site plays: the Deluxe download where there is one, and for a game that first came out
under another publisher (Cosmic Bugs, Water Bugs, Word Harmony) the first release. `byopop_web_index()` writes it on the card as
`data-released`, as given, next to `data-title`, and shows the year small under the name, which is part of the link's accessible
name ("Bejeweled, 2001, play now"). A date that is less exact sorts later among the same year's or month's, so a game with
only a year comes after the full dates of that year, then by name; a game without a row gets a configure warning, shows no year
and goes last, and a row that is not a date stops the configure. Like the table of badges, the table is the coordinator's, who
adds a game's row at its final merge together with its place in `BYOPOP_WEB_FINISHED`. Play time and Plays (the user, 2026-11-03:
"Add sorts by playtime and number of plays too") put the most played first, by the counts on the cards (`ByopopPlay`, the
`byopop-play` item): the one breaks a tie in the other. The games never played here follow, those whose files are here first (the
user: "Play time/plays sort should sort added but unplayed games above unadded games"), then the others, the games still to come
among them, each in name order. They are read again at each sort, when a game is played in another tab, when the page comes back
from a game's, and each time the files check has shown the cards (its `byopop-tiles-checked` event).

**Games no longer sold.** The user, 2026-11-03: "add an option to hide games that can no longer be purchased, and default to
hiding those games from the front page". A game that no store sells (no store in its `BYOPOP_BUY_<target>` entry,
`cmake/ByopopStores.cmake`, the same that makes its page say it is not for sale) gets `data-unsold` on its card, and the page
hides those cards unless "Show games no longer sold", beside "Sort by", is ticked, or the game's files are in this browser (the
card's `data-files` is `yes`: a player who has them can play it). The choice is kept in localStorage under
`byopop-listing-unsold` (`show`, or nothing for the default, hidden). A page where every game is sold somewhere shows no box.

Choosing a sort moves the cards' `<li>` elements (no CSS `order`), so the tab order and a screen reader's order are the order on
screen, and the focus stays on the radio button that was chosen. A polite status line (`role="status"`, out of sight) says "Sorted
by release date", "Sorted by play time" and so on, for the player's own choice only. The choice is kept in localStorage under
`byopop-listing-sort` (`name`, `release`, `time` or `plays`), read and written inside `try`, so that a browser without storage shows the page in name
order; another tab that changes it changes this one. The Players and Saves tabs list the games in the built (name) order
whatever the sort.

**The tabs.** The user, 2026-11-02: "Turn the player, save, and settings managers into separate tabs under the games list
... Massively reduce how much text there is." The three managers are tabs under the games, **Players**, **Saves** and **Settings**,
each shown as an icon of our own (the user, 2026-11-03: "use icons rather than names"; inline SVG line drawings in the page: two
people, a floppy disk, a gear), named by its `aria-label` and with the name as its tooltip (`title`). They follow the ARIA pattern for tabs with automatic activation: a `role="tablist"` row of
`<button role="tab">`s, each `aria-controls` its `<section role="tabpanel">`, which is `aria-labelledby` the tab, in the tab order
(`tabindex="0"`) and hidden while another tab is chosen. Only the chosen tab is in the tab order (`aria-selected="true"`, the others
`tabindex="-1"`); Left and Right move along the tabs, wrapping round, and show each one's panel at once, Home and End go to the first
and the last, and Tab goes on into the panel. Each panel keeps an `<h2>` with the tab's name, out of sight, for the page's outline. A
link to `#settings` or `#music` chooses that tab, and `#settings-<section>` Settings at that section (on Desktop 95, the section's
window), whether it is followed here or opens the page (a game's Settings panel's "Manage players", `#settings-players`). The
addresses of the tabs Players and Saves were, `#players` and `#saves`, land on those sections of Settings (`settingsHash` in
web/index.html), as do `?open=` addresses that carry them. A section with nothing in it is not shown: a game's Settings panel has
Players (its player-name setting) but no Saves. A click on such a link in the page also puts the focus on the tab,
whether or not its hash was already in the address. A tab the player chooses (a click or the keys) goes into the address with
`history.replaceState`, which adds no history entry, so that a reload keeps it, and it is kept in localStorage under
`byopop-listing-tab` (the panel's id), read and written inside `try`, for the next visit; Players is the first. `tools/a11ycheck.js` takes the roving tabindex for what it is: every tab must
take the focus, by itself (a `<button>`) or with `tabindex="-1"`, and one tab of each tablist must be in the tab order, so a
`<div role="tab">` without a tabindex still fails.

The text in them is what a player needs to use them. The explanations went: what the players are for, "What each game last said of
its players" (in the user's words, "cruft that a human would never write"), the line per game that has no players, the list of games
that have not started since, when each game reported, what the backup leaves out and why, the browser's own backup being removed on a
restore, the steps of the import from an installed game, and the Settings' explanation, which is now "For every game. Game default
leaves a game as it is." The note that the browser may clear the saves when the drive runs low is shown only while it has not agreed
to keep them.

At the foot of the page is a `<footer>`, at the page's width and in its muted colours, with the disclaimer, in the words of
`web/copyright.html` and `web/terms.html` (an unofficial, non-commercial project, not affiliated with or endorsed by
Electronic Arts or PopCap Games; it serves none of the games' files; the trademarks are their owners'; the site accepts no
donations and a player with the money should buy games), and a credit line that links to the project's website, which
opens normally, with no name or email beside it.

**Which games are yours.** Under each game's name the card says, in words, whether the game's files are loaded in this
browser, as a sign that the player owns the game: "Play now" when they are, "Add your files" when they are not, and
"Files unknown" when the page cannot tell (below). The link's accessible name carries the same words, after the game's name
and the year of its release ("Bejeweled, 2001, play now"). The page knows without starting any game, from a record that the game pages keep. The record is one
localStorage item for the whole site, `byopop-game-files` (`ByopopGameFiles` in `web/settings.js`): a JSON object from a
game's id to `{loaded, forgotten, missing}`, with dates as YYYY-MM-DD. `loaded` is when the game's files were last found,
`forgotten` when the player used "Forget game files" on the game's page, and `missing` when a game page found the files
gone without that. The files count as there when there is a `loaded` date and neither of the others. A game page writes
the entry whenever `hasGameFiles()` is true in `boot()`, so after an import finishes and at every start that finds the
files, and finding them again clears `forgotten` and `missing`. "Forget game files" does not delete the entry: it adds
`forgotten`, with the date, and keeps `loaded`. A start that finds the files gone, or an import that wipes the old copy,
adds `missing` to a game that had an entry. Entries are never removed, since that a game was played here is part of what
the page says. With `?placeholders=1` a game page writes nothing, since part of a game stands in for all of it.

Files imported before the pages kept the record have none. For a game without a usable entry the listing page looks in
the game's IDBFS database, read only. The database's name is the game's persist folder from its `page.js` (`/ttpersist`),
which `byopop_web_game()` reads at configure time and `byopop_web_index()` puts on the card as `data-persist` (with the
game's id as `data-game`); the page does not read `page.js`. It first calls `indexedDB.databases()`, so that opening a
name never creates an empty database; a name that is not listed means no files, and a browser without `databases()`
shows a game with no entry as unknown instead of guessing. For a listed name it opens the database and walks the keys
with a key cursor, never reading a file's contents. A key is a file's when it is in a folder below the first level (a
file of the game's `game/`, of `userdata/`, `mods/`...), or has an extension at that level (TipTop's files are at the
top of `game/`), which tells a file from a folder without opening it. A file in `game/` counts the game as loaded (a
heuristic: the game page, which knows the game's marker files, has the last word and corrects the record at its next
start). A record that says the files are there is believed only while its database is still listed. The listing page
writes no record itself. It checks again when the page comes back into view (`pageshow`, `visibilitychange` to visible)
and when another tab changes the record (`storage`).

**The "?" for a game played before.** When a game has been played in this browser but its files are not there, the
card has a small "?" button next to "Add your files". Played before means any of these: the game has an entry in the
record (so a forgotten game stays one); its database holds files in other folders than `game/` (saves, mods, replays)
but no game files; or the game's save backup exists in localStorage (`sexy-userdata-backup:<persist>/userdata`, which
`platform/WebPersist.cpp` keeps). A game that was only opened, whose database holds nothing but its empty folders, is not
one. The "?" is a real `<button type="button">` labelled "Why aren't my files here?" with `aria-expanded` and
`aria-controls`. It opens a short popover under the card and closes with a second click, Esc (the focus then returns
to the button) or a click elsewhere; it works by keyboard and by pointer, and the popover follows the button in the tab
order. The text is the page's own words: the game's files were loaded here before but are not now (with the date they
were last found, when the record has one); the likely causes, which are that "Forget game files" was used on the game's
page (when the record says forgotten, it says so with the date), that the browser's data for the site was cleared or the
browser freed the storage, and that this is a different browser, profile or device; that the site never uploads the
game files and no server keeps a copy, that the games belong to their publishers, so the copy stays only in this
browser and each browser and device needs the files loaded from the player's own install; that the saves and settings
are still stored in this browser, said only when the database holds files in `userdata/` or the save backup exists; and
that getting the files back means opening the game (a link) and loading them again. It mentions no accounts or online
features, as there are none.

**Where to buy.** A game that can still be bought has a small link for each store under its label, "Buy on Steam" or "Buy on
the EA app" (a store whose name ends in "app" takes "the"), which opens in a new tab (`target="_blank" rel="noopener noreferrer"`;
the user, 2026-11-09: "The buy links should always open in a new tab") and whose accessible name has the game and says so ("Buy
on Steam, Big Money (opens in a new tab)"). Every store link of the site is such a plain link, never a script's `window.open`,
which a popup blocker may stop: the cards' (and the coming-soon tiles'), the Library's menu entries (links in the
menu, web/skins.js popMenu `href`), the game pages' and copyright pages' buy note and the terms card's. `byopop_web_index()` writes them at build time from the same data as the game pages'
buy note, `BYOPOP_BUY_<target>` in `cmake/ByopopStores.cmake` (a check date, then each store's name and page), so the two
cannot disagree; a game with no store shows nothing. The links are in the same list item as the game's link but not inside
it, in a `.buyrow` beside it, as a link cannot hold a link.

The links are hidden once the player has loaded files for the game, because they would otherwise suggest that the page is
used for copies that were not bought. The rule, in `show()` of `web/index.html`: they are hidden when the game's state is
"Play now", when its record says the files were ever loaded (`ByopopGameFiles.everLoaded`: a `loaded` date, whether the files
are there now or were forgotten (`forgotten`) or lost (`missing`) since), and when the game was played here before (the "?" case:
saves or a database with entries but no record). They are shown for "Add your files" without such a past, and for "Files
unknown". `byopop_web_index()` writes each finished game's `.buyrow` with the `hidden` attribute, so that a player who owns the
game never sees the links flash before the check has run; a game still to come has its `.buyrow` shown from the start. The game pages' terms card (off for now) and the copyright pages keep their buy
note: they are not the card that suggests where to get the game.

**Play time.** Once a game has been played in this browser, its card shows under the name how many times it was started, how long
it was played and the last day it was played: "3 plays · 2 h 15 min", then "Last played 1 Nov 2026" (the browser's own date format,
"today" for today). The user, 2026-11-02: "Add game playtime and last day played to the front page once a game has been played
once. Only count time with the game open and active. Also count number of plays." A play is a start of the game from its page (Play,
or the page's own start when it opens, and "Play without mods" after a start that failed); the start after the game's own restart
from its mod menu goes on with the same play. Both reload the page and start the game without a click, so the page tells them apart by
the run without mods, which the restart never asks for. The
time counts only while the game runs and its page is shown and has the focus (`document.hasFocus()`, so another window in front
stops it; the chat beside the game does not) and is not paused by the site's "Pause when the tab is hidden". The game's page checks
every second and at each change of visibility or focus, adds at most 5 seconds a step (a computer that slept adds at most those 5 seconds) and writes
the whole seconds every 30 seconds, when the count stops, when the game quits and when the page goes away. The record is one
localStorage item for the site, `byopop-play` (`ByopopPlay` in `web/settings.js`, beside `ByopopGameFiles`): a JSON object from
a game's id to `{plays, seconds, last}`, `last` as the player's own date, YYYY-MM-DD. The save backup takes it ("Save backup",
below). Another tab that changes it changes the listing at once (the `storage` event), and the listing reads it again when it comes
back into view.

**The bottle caps.** The user, 2026-11-03: "Change the placeholder icons to be in the shape of a bottle cap." A game's badge,
its initials on its colour, is cut to a crown cap's outline seen from above (`web/brand/cap.svg`, our own drawing: a circle with 21
rounded crimps, used as the badge's CSS `mask`), with the cap's skirt and the ring of its top drawn darker by a radial gradient over
the colour, which keeps the white initials' contrast. The game's own icon, once its files are here, takes the cap's place (no
mask). Desktop 95 draws no caps on the listing: its icons are plain ("The games' icons" under "Desktop 95"); the cap stays the
site's own sign, on the Pop button, the setup wizard and a game page's import card.

**The cap and the opener.** The opener's animation, levering a game's cap off when its files were added (the user's test
notes, 2026-11-03), ran only in the other skins and went with them on 2026-11-09 (`web/capanim.js` and `web/brand/opener.svg`
are gone): a game page's cap comes off at once when the import ends, and an added game's icon on the listing changes at once.

**The recycle bin.** The user, 2026-11-03: "Add a recycle bin to the front page that the game's cap can be dragged into to forget
its files." The bin is the button beside "Add games", with the bin's icon and named "Forget a game's files". The cap of a game whose files are here is draggable (`draggable="true"`,
set by the files check, which also marks the card `data-files`); dropped on the bin, it opens the bin's panel, which asks "Forget
<game>'s files in this browser? Its saves stay." with Forget files and Cancel (the focus on Cancel). Nothing is deleted without
that yes, which the page asks itself: a `confirm()` dialog is not shown in some places the site runs. Clicked, the bin opens the
same panel with a list of the games whose files are here, for the keyboard and for browsers without drag and drop. Forgetting
does what the game page's "Forget game files" does: the game folder's records go from the game's IndexedDB database (saves, mods
and replays stay) and the record of the files says when (`ByopopGameFiles.forget`). Like adding, it holds the game's Web Lock
(`ByopopImport.forgetGame` in `web/gameimport.js`), so a game whose page is open is refused. Escape closes the panel, and after a
game is forgotten its list is drawn again once the cards have been checked (the listing's `check()` resolves then).

**The game's own icon.** Once a game's files are loaded, its card shows the game's icon in place of the initials badge
(an `<img alt="">`, since the card has the name, at the badge's size and with `image-rendering: pixelated` so that a small
icon stays crisp when enlarged). The game's page shows it in its header in place of the coloured mark, at 20 px for the
screen's pixel ratio, and as its tab icon, in `<link rel="icon">`s at 16 and 32 px (no larger than the icon) that replace
the site's. Each is scaled down from the icon with the browser's best filter, at most halving at each step, and set when
the page starts (the user, 2026-11-02). The icon is read out of the game's exe, which the import already stores: the page takes
the `.exe` in the game's `gameFiles` (`games/<game>/web/page.js`; of several the largest, as the game's own binary is
bigger than a helper, though no game lists more than one) and `web/exeicon.js` reads it, with no library. It parses the PE
file (`MZ`, the PE header, the section table, the resource directory), finds `RT_GROUP_ICON` (14) and its `RT_ICON` (3)
images, and takes a 32-bit image at the size nearest 48 to 64 px or, without a 32-bit image, the largest (and, if that one
cannot be decoded, the next). An image is a PNG, used as it is, or a DIB (a `BITMAPINFOHEADER` with the colour bitmap and a
1-bit transparency mask under it, at 1, 4, 8, 24 or 32 bits per pixel, the palette up to 8) which is decoded to RGBA and
drawn on a canvas; the result is a PNG data URL of at most 64 px a side and 16 KB (a larger icon is made smaller until it
fits). An exe that is wrapped or damaged, or has no icon, gives nothing, and the badge stays; nothing in it throws. A game
that does not store its exe (Bejeweled, Big Money and Alchemy import none) says so in its `page.js` (`importIcon: true`): the
import then reads the icon out of the exe in the picked folder or .zip, at the top of the install (the first `.exe` that is not an
uninstaller or a setup, at most four tried), in `iconFromImport()` of `web/shell.html`, and keeps only the icon, in the ownership
record, for `boot()` to put with the entry once the files are marked as there. The exe is read from the pick and is never
stored, here or anywhere.

The data URL is kept in the game's entry of the ownership record (`icon`), where the listing page reads it. The game page
reads the exe in `boot()`, which follows an import and every start, when the entry has no icon yet, so the icon also
appears for files imported before this existed. It is derived from the files, so "Forget game files" deletes it with them
(and so does finding the files gone), and it is shown only while the files are there. It stays in this browser and is
never sent anywhere. `tools/exeicontest.js` checks the parser under Node against the exes in `assets/<game>/` (it prints
one line for each exe and writes no icon) and against synthetic exes made for every decoder, and feeds it damaged files.

### Jungle Juice

Jungle Juice is the site's second page, which will hold games decompiled by other projects (the user, 2026-11-04: "Prepare a
second page on the website, "Jungle Juice", that will eventually hold third-party decomps. All its pages will go under
/jungle, and the front page will link to it after the game tiles."). For now it is a placeholder: `web/jungle/index.html`, in
the listing page's look (its colour tokens, the logo and the footer's first and last lines, in Desktop 95's window), says in a line
what the page will hold and links back to the front page. It has no third-party content and no links out yet.

Every Jungle Juice page lives in `web/jungle/`, and `byopop_web_jungle()` in `cmake/ByopopWeb.cmake` writes each `*.html` there
into `build/web/jungle/`, its `{{key}}` markers filled from the site's text (`jungle.*` in `web/strings/en.txt`), so the site
serves them under `/jungle/`. They reach the site's shared files one folder up (`../strings.js`, `../skins.css`, `../skins.js`, `../logo.js`,
`../logo.svg`, the icons). The listing page links to `jungle/` in a card under the game tiles (`.jungle-link`).
`tools/stringstest.js` and `tools/a11ycheck.js` check these pages with the others.

### Adding games from the listing page

The user, 2026-11-03 (the "Site" section of their test notes): "Allow many games' files to be added at once. Try to
infer the game from the folder name, allow the user to manually specify which game a folder is for if unclear, and tell the user
which folders were rejected", and "allow dragging your game folder onto the logo to add it (representing the use of the bottle
opener). Also allow a game folder to be dragged onto its game tile to add its files." This is a rough first version; the opener
takes the caps off as the games are added ("The cap and the opener", above).

There are three ways in, all in `web/addgames.js`:

- **A game's card.** A folder (or a .zip of one) dropped on a finished game's card is taken as that game's, exactly as its own
  page's import card takes it: the game's markers are looked for in it and nowhere else, and a folder without them is refused
  with the import card's own words ("That folder doesn't hold Bejeweled's images/gemsheet6 file").
- **The logo.** Folders dropped on the logo stand for the bottle opener: each one is sorted into the game it is.
- **"Add games".** The button beside "Sort by" opens a folder picker (one folder: a game's, or the folder that holds many, such
  as "PopCap Games"), sorted the same way. It is the way in for the keyboard and for browsers without drag and drop.

On the listing's desktop the logo, "Add games", Add Games in the Pop menu and a drop anywhere on the desktop outside a game's
window all go to the Setup window instead (below), which sorts and copies through this same queue; a drop on a game's card stays
that card's. The panel described here is what a screen without the desktop uses.

Sorting (`ByopopImport.sort` in `web/gameimport.js`) goes by the files first and the name second, as the files say more surely
which game a folder is than its name does. A folder is a game's when it holds every one of the game's markers (the files its page
looks for); a folder that holds several games in different places (the "PopCap Games" folder) is looked into, three levels deep
at most, and a folder with no game this site plays in it is rejected. Where two games' markers are at the same place (a game and
its sequel can share file names), the folder's name picks the one whose title, short name or build target it matches, ignoring
case, punctuation and words every game folder has ("Deluxe", "PopCap", "Games"). When the name cannot tell, the panel asks
("Which game?", with only the games it could be). A folder that holds one game in one of several subfolders is looked into the same
way, so the subfolder is the row and the others are named as not added. The panel under the toolbar shows each folder with its
game and its state, and names the rejected folders in one line; one live region reads out how each folder ended, and the focus
goes on to the next folder still to choose. On a phone, where no folder can be picked, "Add games" picks .zip files of game
folders, which are unpacked as a dropped .zip is.

The games are added one at a time. The listing page runs no game, so it cannot use a game's Emscripten file system: it writes the
files straight into the game's IndexedDB database, in the records IDBFS itself writes and reads (`FILE_DATA`, keyed by path,
`{timestamp, mode, contents}`, with a directory record for every folder, as `web/saves.js` does for a restore), after deleting
the old game folder's records. It does so while holding the game's Web Lock (`PAGE.lock`), which a game page takes before it
reads its storage and holds for its whole life; so a game whose page is open in another tab is refused (the note says the files
are in use, or that this page is still working on the game when its own add, restore or bin holds the lock), and no page can
mount the database while it is written, then write its old copy back over the new one. A browser without Web Locks (a page not
served over https or from localhost) is refused too, since an open page could not be seen there. The write is all or nothing:
the first transaction deletes the old copy, then the files go in batches of about 32 MB, each with the folder records of its own
files, and the files the game page checks for (its markers; for a game whose marker is a pak, the files of its game folders, then
the markers) go last, so a copy cut off partway (the tab closed) is not taken for the game: the game page refuses it, and the
listing's files check lets the record, which says "missing" from the moment the old copy went until the add ends, win over the
files it finds in the database. A write that fails partway (the
storage full, a file of the drop that would not read) deletes what it wrote; one that fails before the old copy is deleted leaves
that copy and its record. The rules
for what is copied are the game page's own: `web/gameimport.js` holds the finding of the game root, the unpacking of paks, the
choice of files and the icon read out of the exe, the game pages call it from their import card, and the listing page has each
listed game's `page.js` in `import-games.js`, which `byopop_web_index()` writes, each wrapped in a function that returns its
`PAGE` (a page.js only declares things, and none of its functions is called there). The listing page loads `web/modpanel.js`
for its zip and pak readers, with an empty `Module` (it fills `Module.byopop` and starts nothing). After a game is added, the
record of its files is kept as the game page keeps it after an import (`ByopopGameFiles.mark`, with the icon), the browser is
asked to keep the site's storage, and the cards are checked again. `tools/gameimporttest.js` (in `unit-web`) checks the rules and
the sorting under Node with every finished game's `page.js` (a page.js that fails there is a failure, and a change to one picks
`unit-web` in `tools/check.sh`'s merge mode).

### The Setup window

The user, 2026-11-09: "I did still like the fake installer skin idea though, so add an installer window where files can be
dropped in bulk." `web/setup.js` is a window of the listing's desktop, "byo-pop Setup", a wizard over `web/addgames.js`'s
sorting and copying: it adds no rule of its own for what a game is or what is copied. The art column at its left is byo-pop's
mark over a box going into a grey case, drawn in CSS; the window parodies an installer without naming or drawing any real one
(no vendor's name, logo, icon or sound, and nothing that looks like a system prompt), and its text is only where the player acts.

Its pages, with Back, Next and Cancel under them:

- **Your games**: a drop target for game folders, .zip files or a folder of many games, and Browse (the same folder picker, or
  .zip files where no folder can be picked). Each folder found is a row with its game's tile badge, its name and "Ready", "Already
  here" or "Not a supported game"; where the name cannot tell two games apart the row asks which, and Next waits for it.
- **Licence: <game>**: one page for each game found that is not here yet and whose terms are not agreed, in the order found,
  showing that game's terms card (`ByopopTerms`) with its checkbox; Next agrees, "Don't add this game" skips the game. Each game
  has its own terms, so these come after the games are known, and a game agreed to before (or any game on a development host,
  where the terms skip themselves; `?terms=1` shows them) has no page.
- **Copying**: an overall bar and a row per game with its state, the games copied one at a time by addgames.js's queue. Cancel
  here lets the game being copied finish and drops the ones not begun.
- **Finish**: a row per folder: Installed, Already here, Not a supported game, Skipped, or "Error <code>" with its reason in the
  tooltip (1: the drop could not be read; 2: the files could not be copied, such as a game whose page is open). One live line
  reads "Installed N of M".

Add Games in the Pop menu and the Add games button open it at Your games. A drop of folders or .zip files on the logo, or on the
desktop outside a game's window, opens it with them, and it goes straight on to Copying when no game needs a choice or a licence. A drop
on a game's card keeps the single-game import above. Closing it after Finish leaves it empty for the next time.
`tools/setuptest.js` (in `unit-web`) checks the order of the pages for the games found and Finish's rows and counts.

### Coming soon games on a development host

The user, 2026-11-09, asked "Make built ports' Coming soon tiles open their game on the local site?": "Yes, locally". A game in
`BYOPOP_WEB_UPCOMING` whose page this build made (its `byopop_web_game()` was configured and its `BUILD_<TARGET>` is on) gets
marks on its Coming soon tile in the generated listing (`data-local-page`, `data-local-game`, `data-local-persist`,
`data-local-lock`). On a development host (127.0.0.1, localhost, [::1], *.localhost, *.test or a file:// page, `ByopopTerms.devHost`)
`web/localsoon.js` turns such a tile into the link a finished game's tile is, before anything reads the tiles, so the listing (a
double-click, Enter or Space), the Library, a favourite's desktop icon and the Pop menu's Games (once its files are here) open the
game in its window like any other. The card keeps its Coming soon look and its "Coming soon", with this computer's icon beside it
(tooltip "This computer only"). A game with no page in the build keeps its plain Coming soon tile, and on any other host the marks
are ignored. `?localsoon=0` keeps every one shut on a development host too. `tools/localsoontest.js` (in `unit-web`) checks the
hosts, the marks and that a deployed host or a forced-false devHost leaves all of them shut.

The site has no deploy step yet: build/web holds the pages, scripts and wasm of every port this build made, the ones still to come
too (`<target>.html`, `.js`, `.wasm`, `-copyright.html`), so whatever copies the site to a public host must leave those out, or the
site's build must turn their `BUILD_<TARGET>` off.

### Games this site does not play

The user, 2026-11-07: "Let's also add site code explicitly recognizing folders that seem to contain a game we haven't/won't
decomp, and either direct the user to an existing decomp project for it or just say 'coming soon'". So a folder that holds none of
the site's games but one it knows names that game, on the listing page (a line under the panel's list, "Not added: <folder>.") and
on a game's own page (after its "Missing files" error, and when the folder is dropped on another game's tile). Where another
project plays the game, a link to that project follows, opening a new tab as the store links do; where none does, or the game is one
of the site's own to come, the line says it is coming soon. A folder of several such games is looked into as a folder of the site's
own games is, each named in its own line.

The table is `cmake/ByopopOthers.cmake` (`BYOPOP_OTHER_GAMES`), one row a game: its id (the target, for a game in
`BYOPOP_WEB_UPCOMING`), its title, two marker files only it has, matched as a game's own markers are, and the project and its page,
or neither. `byopop_web_index()` writes it to `other-games.js`, which both pages load after `web/gameimport.js`, and stops at a
malformed row, a project whose page is not a GitHub project's, or a link for one of the site's own games to come. Its comment says
when each project was checked and what its README says. A project is linked only when its front page and README say it plays the
game, ship none of its files and need the player's own copy, on a trusted host; only the front page and README are read, never the
code ("No references" in `CLAUDE.md` binds every port), and a project whose state is unclear is not linked. Every row's markers were
checked against the user's own copies, and `tools/gameimporttest.js` sorts folders of them beside the site's games. Jungle Juice
(above) will host other projects' decompilations; once it does, these links can point there.

### The site's look: rules

The user asked for original site designs that evoke the games' era and its web, with selectable skins (2026-11-07). The
directions proposed so far, each as a listing, a game page and the import as a setup wizard, with studies for the bottle-cap
board, are on a design page made for the user (https://claude.ai/artifact/FZy69Pz8kFCPkfZvctTA7T, private to them). Whatever is
chosen keeps these rules, each in the user's words of 2026-11-07:
- **The game stays the game.** "Skip in-game chat. I want the default to not feel modded relative to the version people might
  build themselves." What happens inside the game's picture, and what its buttons do, stays as a port the player built
  themselves would have it; the page around the game is free to change. Whether a game's "More Games" button may open the
  listing is a question for the user, since it changes what an in-game button does.
- **The era, not the company.** "Site isn't going to be a history of the company itself. A love letter to the history just
  means it's acknowledging only historical parts (all the games are >15 years old by now). I want nothing to do with the EA
  subsidiary." And: "I don't want to *just* rearrange things that exist in the game for the sake of credits." So no company
  history, credits panels or liner notes, and nothing that ties the site to today's company.
- **Period looks are affectionate parody.** "What I'm proposing would be better understood as affectionate parody. Think of
  the various games that have been made with the style of Windows 95. The font is free to use and the color choices are low-bit
  due to technological limitations. No chance it gets confused with the real operating system." A skin may look fully like a
  1995 or 2001 desktop (bevels, title bars, wizard pages with Back, Next and Cancel), drawn in CSS or SVG. It uses no
  Microsoft or InstallShield names, logos, icons or sounds, is named after byo-pop and the game, and never imitates a real
  system prompt such as a security or permission dialog.
- **The player's own fonts.** "If it's just using text elements, it could use the user's own installed font." A period skin
  names the system fonts in CSS (`"MS Sans Serif", "Microsoft Sans Serif", Tahoma, "Segoe UI", sans-serif`) and ships no
  Microsoft font files. A free lookalike may be a fallback for machines without them, and only that one is listed on the
  licences page.

### Values, not sentences

The rule for every control surface on the site, from the Music player on (the user, 2026-11-08, of the player: "A well
recognized icon for "repeat" already exists. Still too much text in the player. It is instantly obvious AI was used. Figure out
some way to make elements that don't look like that"). Real software of 1995 to 2005 did none of the things that read as made by
a machine: sentences explaining a window inside it, a status line in prose, a labelled check box for every option, a word beside
every slider.
1. **No sentences inside a control surface.** A window shows values and standard labels. What needs explaining goes in a
   tooltip, in About, or nowhere.
2. **A recognised icon, alone.** Where a widely known icon exists (the transport's, repeat as two arrows in a loop and pressed in
   while on, shuffle, the speaker for the volume that mutes on a click), it stands with no word beside it, drawn in our own
   pixels; its name is its tooltip and its `aria-label`.
3. **Status is data in a display,** not a sentence: an LCD-style readout of names and times, as period players had. A test mode
   such as `?silent=1` shows a small indicator there or nothing.
4. **Views belong to the display.** Clicking it cycles them (the player's spectrum, tracker, nothing), with no check boxes; a
   view's own header is terse, in its own style (a tracker's "ORD 07  PAT 06  ROW 16/64").
5. **Credits and explanations go to About** and the licences page, never dropped.
6. **Density and hierarchy of real software:** small type, tight spacing, the controls grouped in one row, one display; no
   uniform cards, pill buttons or generous padding unless the skin's own style calls for them. Desktop 95 follows Windows 95's
   conventions.
7. **Evoke, don't copy.** Real players of the time (CD players, media players, module players) guide layout and density, but
   no real program's artwork, logo, name or skin is copied: our own drawing throughout.

The rest of the site took the rules on 2026-11-08, in one pass. Each setting's explanation sits behind a small "?" beside its
label (shown while it is hovered or focused, pinned by a click, closed by Esc, and linked to the control by `aria-describedby`),
and its row is a line in a list, not a card; the line under a label holds only live markers (applies at the next start, this
game's own value). The notes in the Saves, Controls, Performance, Mods and game files windows are terse facts; the game's start
card has its title and Play, no "Click to start."; the licences page has no introduction. Two exceptions are kept whole: the
terms (its text unchanged; the card leads with the agreement, so the full text scrolls below it) and the chat's note,
which became a value on 2026-11-09 (below, "The chat"). Text a player needs to act stays, as short steps: the save import is a wizard (`web/saveimport.js`), one
step a page (a heading, its fields, at most one short line) with Back, Next and Cancel, the facts a field needs in its tooltip
and its description, and the registry commands with their Copy buttons on their own page.

Desktop 95 keeps to what Windows 95 drew (the coordinator, 2026-11-08). Every button is the skin's grey bevelled one with black
text, and a destructive one is marked by its confirmation, not by a colour. The current player is in bold beside its checked
button, with no pill. A Settings section's window has one "?" in its title bar (What's This?), not one a row: it turns the
pointer into an arrow with a question mark, and the next click on a control shows its tip there instead of working the control;
F1 on a focused control shows the same tip, and the text stays the control's description for screen readers.

### Desktop 95

Desktop 95 is the site's look, an affectionate parody of the 1995 desktop. It was one of three skins, with Demo Disc and Soda
Fountain and the plain look besides, until the user kept Desktop 95 alone on 2026-11-09 ("drop the alt skins, including the
plain one"); the light and dark theme, the ambient light and the first visit's skin card went with them. `web/skins.js` sets
`<html data-skin="d95">` in each page's `<head>`, before the body is drawn, and `web/skins.css` holds the look. The player-only
page (`?player=1`) alone has none: it keeps the page's own plain styles, for the sessions' exact shots. A player's stored
`skin` or `ambient` (in `byopop-settings`, site-wide or a game's own) is dropped when the settings are read
(web/settings.js, `GONE_KEYS`), and the theme's `byopop-theme` item when skins.js loads, so a stored Demo Disc, Soda or Plain
simply opens Desktop 95. Every page is drawn in it, the copyright page, the licences and Jungle Juice too, each inside its
window on the desktop (an embedded page's body takes the window's face, below).

How it is written: it sets the pages' own colour variables (`--bg`, `--panel`, `--panel-2`, `--fg`, `--muted`, `--line`,
`--accent`, `--accent-fg`, `--font-ui`, `--radius`) and its own `--sk-*` ones, and stays light whatever the browser's own
colour scheme. Period fonts are named and never shipped: the 1995 desktop's sans serif, then Tahoma.

**The setup wizard.** A game page's three cards before play, the terms, the import and Ready, are the pages of a setup wizard
(`ByopopSkin.wizard`, called by web/shell.html): a title bar ("byo-pop Setup: <game>"), the steps Licence, Your folder, Copying
and Finish (the import card is Copying while its progress bar moves), and an art column at the left. The cards keep their
contents, buttons and rules: the terms card is the licence page (the user, 2026-11-07: "Yes, as the licence page") and keeps
everything in "The terms", development hosts and the test key included. The wizard's parts are hidden from assistive technology
(the cards read as they are) and are not drawn on the player-only page.

**The games' icons.** Desktop 95 has no bottle caps (the user, 2026-11-08: "The animated bottle caps clash with the Windows 95
skin. I'd prefer bare initials for missing games, and the plain favicon otherwise."): a game whose files are here shows its own
icon, read out of the player's own exe at the import (web/exeicon.js) and never shipped or redrawn, and any other its initials
on a plain white square icon with a black edge and a grey shadow, in the 16-colour palette; an added game's icon changes at
once. The icon is fitted to a 32 px box (`fitIcon` in web/index.html; the user, 2026-11-08: "The logos overlaid on the caps need
work so they fit properly and are better centered"). The record keeps two of the exe's images, the one nearest 48 to 64 px
(`icon`) and the one nearest 32 px (`small`, "" when the exe has none smaller); of these the page takes the smallest that is at
least the box's size in device pixels, else the largest, crops it to its visible pixels (many icons have empty space on one
side) and draws it at the screen's own pixels: smaller with smoothing, larger only a whole number of times, pixel for pixel,
centred on whole pixels. A record from before `small` was kept gets it the next time the game's page opens, where the game
stores its exe. The cap animation of the other skins (the opener levering a game's cap off, web/capanim.js) and the cap's
studies went with them.

**Desktop 95's windows.** A game's page is one desktop (web/skins.js, `ByopopSkin.desk`; html.d95-desk). The game, the
chat and every panel (Settings, Game files, Mods and the game's own) are
windows dragged by their title bars, or moved 8 pixels at a time with Alt and the arrows while the focus is in a title bar. A
column of icons at the left opens each one again; a panel's icon presses the panel's own button on the page's bar. The bar
itself is no window there (the user, 2026-11-08: "That tools window probably doesn't need to exist at all"): the game's name is
its window's title, alone, as a 1995 program's title is its name (the coordinator, 2026-11-08; the mods that run are counted in
the status bar, and the task bar's buttons take the name too); the status line is the
game window's status bar, in a sunken panel along its bottom; the panels are their own windows, as many open at once as wanted, each closed by its own Close (or Esc in it), and
a click on the game leaves them open (elsewhere one panel shows at a time and a click on the game closes it); the sound switch is in the
tray; full screen is the game window's maximize button, an entry of its control menu (its title bar's icon:
Minimize, Maximize, Close) and of the Pop menu. The game window's Close (its
button, its control menu's Close; no Alt+F4, which Windows gives the browser's window and never the page) closes the program (the user,
2026-11-08: "Closing the game window ought to go back to the home page/bring back All games"): the game closes as it does for
the panels' actions, its saves written (closeGameThen), the page's play time is kept as on any way out, and the listing opens
with All games open and in front (sessionStorage `byopop-desk-open`); no question is asked beyond the page's own. Minimizing
it keeps the game running on the task bar. The game window is always pixel-perfect: one canvas pixel for each game pixel, or the largest
whole multiple that fits (sizeStage), and its picture starts on a whole device pixel. Its size is the player's too (the user,
2026-11-08: "95 skin needs a way to size up the game window by an integer multiple. Big Money is very hard to see on my 4K
monitor"): its control menu lists the whole multiples of the game's size in device pixels whose window fits on the desktop
("2x (1280 x 960)"), the chosen one dotted, and so does the Pop menu's Game size; its resize corner, at the bottom right of
its status bar, snaps to them as it is dragged. The choice is kept for each game (localStorage `byopop-d95-scale`), and a
choice that no longer fits (a smaller window, a game of another size) gives way to the largest that fits beside the icons.
While the game starts its window is not shown, its task bar button already there and the 1995 hourglass over the page (the user,
2026-11-08: "While a game loads with a non-standard window size, it shows the standard window size as black. Hide the window
until the game is actually loaded"): from the start until the game has made its window (onGameScreen, which every game's
framework calls from MakeWindow) and two frames have been drawn in it, half a second where a hidden page draws none, or the game
has ended or stopped, and never longer than 20 seconds. It is hidden from the page's first paint too (game-loading on `<html>`
in the page itself), and shown before the game only once the page settles on waiting for the player (the Ready, import or terms
card), so a page that starts its game by itself (the setting "Start the game when its page opens", the game's own restart)
never paints it at its first size first (the user, 2026-11-08: "Twist's window still briefly appear in the small size before
loading. It then disappeared before finally reappearing when the game had started"). In Desktop 95 the window is not
shown before the game has drawn in it at all: the setup wizard's cards (the terms, the import, Ready) stand on the desktop by
themselves, with the window's task bar button, so pressing Play never shows the window at its first size before the game's
(game-unmade on `<html>`). Under 760 pixels wide the stage is hidden
the same way, with the `progress` cursor. The game gets the focus once it is shown, unless the player has put it elsewhere meanwhile.
**Restore.** While a window is maximized its maximize button is Restore, drawn as 1995's two overlapping windows, its name
"Restore <window>" and its control menu's entry Restore (web/skins.js `showMaxed`, watching the window's `d95-maxed` class).
A task bar button makes its window the active one, with the focus inside it, as the 1995 task bar did.

**Full screen in Desktop 95.** The game window's maximize button, its control menu's Maximize, the Pop menu's Full screen and
the game's own Full screen option put the game alone in full screen (the framework offers the game's request to the page first,
`sexyapp_page_fullscreen` in framework/SexyAppBase.cpp, and the page takes it in Desktop 95 as its maximize: SDL's own full screen
would make the canvas the full-screen element at the screen's size; the game's option follows the page as the player leaves): the largest whole multiple of its size in device pixels that fits the screen, or, when not even
one does, as large as fits at its own shape, centred on black (web/shell.html, `fitFullscreen`; web/skins.css). F11 is the
browser's own there (the user, 2026-11-08: "Full screen issue seems to be the difference between using the in-game full screen
and pressing F11"): the whole desktop fills the screen and the game's window stays a window; the game still never gets the key.
Leaving full screen puts every window back where it was, as their places are kept in pixels. Where the windows are, and whether the
game window is closed, is kept for each game in the localStorage item `byopop-desk`. Each game keeps its own page, so a game's
address still opens that game. The listing is its own desktop: the games in one window, each of the tabs under them (Settings,
Players, Saves, Music) in a window of its own, and the page's notes in another, each dragged, closed and opened from its icon
like a game page's, and kept under "index"; the games' window and the notes are open at first, the tabs' windows closed until
their icons or a link to #settings and the like open them. The desktop never scrolls (the user, 2026-11-08: "The whole window
shouldn't be scrollable"): every window is at most the size of the browser's window above the task bar and scrolls its own
contents, and it is moved to fit on the screen when it opens and when the browser's window changes size. Every window but the
game's is resized by its edges and corners, as in 1995 (the user, 2026-11-08: "Non-game windows should be resizable"): a band
5 pixels wide along each edge, from the frame's outer edge in (the window clips its contents at its border with `overflow: clip`
and a 2-pixel `overflow-clip-margin`, so the bevel itself is part of it), and 12 pixels along each side at each corner, from
200 x 120 up to the room above the task bar. Over them the cursor is 1995's resize cursor for that edge (the user,
2026-11-09: "The windows should use a resize cursor on the edges"), a black double arrow edged in white drawn in web/skins.css,
up and down, across or diagonal, over a site cursor set too; a maximized window has no edges to resize;
its contents scroll or reflow in it, and the size is kept with its place. Music opens 420 pixels high (600 wide, as the tabs' windows are), about a 1995 media
player's size, with its list scrolling inside (the user, 2026-11-08: "Music window is too tall"). The game's window keeps its
whole multiples (below). Every window has
the 1995 frame (the user, 2026-11-08: "This border needs to be fixed"): a 2-pixel bevel as the window's own edge, 2 pixels of
the grey face, the title bar 18 pixels high across the whole inner width, 2 more pixels of face, then the client area; where
the window scrolls its contents (the listing's windows, About) a thin sunken edge marks the client area and its scroll bar
stands inside it. The rule sits above the skin's own rules for the same elements off the desktop, which had put a padded box
between the frame and the title bar.

**Desktop 95 in one page.** The whole site is one page, the listing's desktop (the coordinator, 2026-11-08): every page of ours opens there as a window, with its own title, task bar button and, for a game, an entry under
the Pop menu's Games (web/deskapps.js, `ByopopApps`). A game, its copyright page (the full terms and the licences) and any other
page of ours are their own pages in a frame (`?embed=1`: the page draws no desktop of its own, no task bar, no icons, no header
or footer, and keeps its window places under `embed:<page>`); a licence text is fetched into the window as plain text. A link
to one of our pages opens its window instead, on the desktop and inside a window's page alike; a link to another site still
opens a tab.

The address bar shows only the site's root (the user, 2026-11-09: "What we're developing now is a SPA, and the url bar should
never need to show any path other than root"): its base as served (`/` on the local server, the base folder wherever the site
is hosted), with no file name, page path, fragment or `?open=`, whatever window is open, in front or closed. The windows live in
`history.state` (`{d95: true, open: [the pages' addresses from the site's folder], active}`, web/deskapps.js): opening a
window pushes a new entry with the same root address, bringing another to the front or closing one replaces the entry, so
Back and Forward close and reopen windows and never leave the page, and a reload, whose `history.state` survives, opens those
windows again (a game's page starts afresh in its window). The listing's own windows (All games, Settings and its sections,
Music, the Library, the chat, About) are kept where the player left them, open or closed, by the desktop's layout
(`byopop-desk`). A link to `#settings`, `#music` or `#settings-<section>` (and the old `#players` and `#saves`) opens that
window and leaves the address alone, and one typed into the address is opened and taken off again. An old address of a page
of ours, a bookmark or a link someone shared (a game's page, its copyright page, a licence text, Jungle Juice, any
`?open=`), opened in its own tab, goes to the root with `?open=<page>` (web/skins.js onePage), where the desktop opens that
page's window and replaces the address with the root. The tab's title stays the site's. A frame's own page
(`?embed=1`) is never in the address bar and keeps its own address.

The test parameters are the exception, and stay in the address they were opened with: `?silent=1` and `?assets=` stay on the
root, through a reload and every window, and `?silent=1` passes to every frame. A game's own page opened with `?silent=1`,
`?player=1` or `?assets=` (`<game>.html?silent=1`, as the sessions' drivers and tools open it: Bejeweled Twist's webperf.js,
Bonnie's paneplay.js, the pane tests in CLAUDE.md) stays that page, with no move to the desktop: on its own desktop, its canvas
in the game window and its status line (ending in "silent") in that window's status bar. `?desk=0`, which kept a page as it
was, is gone with the other skins: nothing used it.

Up to three games run at once, each in its own window (the frames are separate pages, so each game has its own module, its
own saves, play time and the mods it runs in its status bar); a fourth brings up a Windows 95 message box naming the
limit. The window in front has the focus, given to its game when it comes to the front by its title bar, its task bar button
or the Pop menu, and the other games lose it exactly as a page does: their frames get the window blur the framework listens
for (`EmscriptenFocusCallback`), so each does what it does when its page loses the focus; a minimized
window is never in front. A favourite's desktop icon and its menu open the game's window too. A game window has the menu bar of a Windows 95 program (the user, 2026-11-09: "don't hide everything under "Game""): Game
(Game files, Controls and any window of the game's own; Close), View (Fit window and the whole multiples of the game's size;
the scanlines, the colour filter, the run timer and the input display, set for that game as its Settings would; Full screen;
the chat), Options (Mods, Settings) and Help (the terms, copyright and licences; an About box with the game's name and the
copyright lines of its readme). Each menu has its access key, the first letter underlined: Alt and the letter opens it,
from the desktop or while the game has the keys, the left and right arrows go to the menu beside it, and Escape gives the keys
back to the game. No menu shows a key the page does not get: Alt+F4 closes the browser's window and Alt+Space opens its
window menu on Windows, so neither is handled (the user, 2026-11-09: "Alt-F4 captured by browser").
Every menu of the desktop (the Pop menu and its lists, a window's menu bar menus and control menu, the context menus, What's
This?) closes as Windows' did (the user, 2026-11-09: "The dropdown doesn't disappear properly when clicking out of it"): on a
press anywhere outside it, which still does what it does, in a game's frame too (the page sees no press there, only its own
window losing the focus to the frame); on Escape; and when its window is minimized, closed or goes, or another window comes to
the front (web/skins.js closeAllMenus).

The window's client area is the game on black with the status bar along its bottom edge, and the game is at a whole multiple
of its size in device pixels (web/shell.html embedWidth): the one chosen in View; when none is chosen, what the Game window size
setting in Appearance says (`gameSize`, site-wide or a game's own): 1x by default (the user, 2026-11-09, of games opening at
3x and 4x on a 4K screen) or "Largest that fits" (the user, the same day: "Add a setting for games to default to the largest
integer multiple that fits"), which a window takes at once when the setting changes; or with Fit window the largest
that fits the window, however the player sizes it (its edges, its maximize button, which fills the desktop above the task
bar and gives the window back its place on a second press). Its edges never drag it smaller than its game (the user,
2026-11-09: "Don't allow a window to be dragged smaller than the game inside it"): the game at the chosen multiple, or at 1x
with Fit window, and its status bar (web/shell.html `ByopopEmbed.least`, web/deskapps.js). The window takes its game's size when it opens: the chosen
multiple (at first the Game window size setting's), or with Fit window the largest the desktop has room for. View lists the multiples to two past the largest the desktop has room for
(at least 4x), the ones without room greyed; picking one sizes the window to it, smaller as well as larger, and restores a
maximized window first (the user, 2026-11-09, of 1x leaving a large window around a small game: "Not what I expected the 1x
button to do"), and the
status bar shows the multiple as a value ("2x"). The choice is kept for each game (localStorage `byopop-d95-scale`, 0 for Fit
window). Full screen (View's, or the game's own option) puts the window's frame in full screen, and the page in it
draws its stage alone there (`html.fs-frame`, frameFull), as a game's own page does: the game at the largest whole multiple of
its size that fits, centred on black, with no frame, menu bar or status bar. Esc or the game's own option leaves it, and the
window is as it was; F11 stays the browser's, as on the desktop.

The chat is one window of the desktop for every game (the user, 2026-11-09: "Let's just have a single chat window instead of
waiting for it to load with each game"), with its icon, its Pop menu entry and its task bar button. It is built when it first
opens and runs on while games open and close; a game's page in a window starts none of its own. It comes up behind the first
game the page opens (unless "Show chat" is off) and is in front only when the player brings it there. It stands at the
desktop's right edge, 300 by 480 pixels, and a game's window opens at the left, so the two sit side by side from 1280 pixels wide
with a game of 800 by 600 or less. What the player sends is tagged with the game in front, or the newest game open (`ByopopApps.frontGame`),
and with none while no game is open. Close
is the game's own way out, its saves written first (closeGameThen), then the window goes. A game window takes its game's size
in device pixels with its frame and status bar, as far as the desktop has room. `?silent=1` passes to every frame. Under 760
pixels wide one window shows at a time, maximized, the task bar switching between them, and only one game can be open.
Measured in the pane with Big Money, Alchemy and Twist: the games' wasm memory is 53, 30 and 284 MB, and the page's JavaScript
heap 14 MB with one or two of them and 100 MB with Twist's images loaded as well.

**The task bar.** Along the desktop's bottom edge (the user, 2026-11-08: "Add a task bar to the bottom with a start menu,
replacing "Start" with "Pop""): the Pop button at the left, with the site's cap beside the word, then a button for each open
window (the front one pressed in: a click brings a window to the front, or minimizes the one in front, and a minimized window
comes back from it), and the tray at the right with the clock and, on a game page, the sound switch. A window's minimize
button, and the chat's collapser on the desktop, put it there; whether a window is minimized is kept with its place. The
page's footer (the copyright line and "Full terms") is in an About window, opened from the Pop menu or its icon. The Pop menu
opens upward, with "byo-pop" down the band at its side: Games (a list of the games whose files are here, each opening its
page), Settings and the managers (on the listing their windows, on a game page its Settings panel and links to the listing's
Players, Saves and Music), on a game page each of its windows, About, and on a game page "Back to all games". It works from
the keyboard: the arrows, Home and End move through it, Enter, Space or the right arrow opens an entry, Escape (or the left
arrow in the games' list) closes it with the focus back on the Pop button, and Tab leaves it closed; the Windows key is left
alone. No entry imitates the real menu's words, and nothing in it is a system dialog.

**Favourites.** Every game's card on the listing has a star button (the user, 2026-11-08: "User should be able to save some
games as their favorites. In other skins, with a star icon, and on Windows 95, a desktop icon specifically for that game"), a
coming-soon game's too: an outline while the game is not a favourite, filled while it is, drawn pixel by pixel on a small
raised button. Its name is "Favourite: <game>" and its state `aria-pressed`. The favourites are kept
in this browser in the settings' own item, `byopop-settings`, as its field `favourites` (`ByopopSettings.favourites`,
`setFavourite`, web/settings.js): a list of {id, title, tag, colour, page}, so that a game page can draw them without the
listing, and the save backup and its restore carry them with the settings. A coming-soon game's id is "soon-" and its title's
letters. A game hidden as no longer sold stays a favourite. In Desktop 95 each favourite also has an icon on the desktop, on
the listing and on every game page, after the windows' icons: the game's own icon where the player's files hold one, else its
cap with its initials, and its name. A double-click, Enter or Space opens the game's page; its menu (a right click, the menu
key or Shift+F10) opens it or takes it out of the favourites, and the icon goes. In the Pop menu's Games the favourites come
first.

**Windows 95 games only.** A setting in Look, off unless chosen and heeded only in Desktop 95 (the user, 2026-11-08: an
"immersion" toggle "that will only list the games that actually supported Windows 95. That info is in the README for most
games"), lists only the games whose original supported Windows 95. Which did is `BYOPOP_WEB_WIN95` in `cmake/ByopopWeb.cmake`:
yes, no or unknown for each finished and coming game, with the game's own file that says so (its readme's system requirements)
in a comment above the row, and no readme text. The build puts it on each tile as `data-win95` and in the chat's table
(`chat-games.js`) as `win95`. With the setting on (`html.d95-win95only`, `web/skins.js`'s win95Sync), All games hides the other
games and shows the count above them as a value ("4 hidden", the reason in its tooltip), the favourites' icons of those games go (the favourites stay), and the Pop menu's
Games lists only the others, with the count as its last, greyed entry. An unknown game is hidden too. A game page already open
stays open. Six games are unknown: Cosmic Bugs, Water Bugs, Gyromancer and both Mahjong
games have no readme that says, and no copy of Feeding Frenzy is here; all six came out after TipTop. A game released before the
last one whose readme names Windows 95 (TipTop, 2002-12-12) counts as yes whatever its readme says (the user, 2026-11-08:
"Anything released before the last game that did support Windows 95, we'll assume supported 95"): Bejeweled, Alchemy, Dynomite
and Big Money, whose readmes found are of later releases (Windows 98 or later). Ten are yes, 24 no.

**Desktop 95's controls.** The skin draws its controls as the 1995 desktop did (the user, 2026-11-08: "the dropdowns, check
boxes, and selectors should be redesigned to look more like authentic Windows 95. Rounded corners are very un-95"): white
sunken fields, a drop-down's raised grey button with its black triangle, a check box's black tick, an option button's black
dot, raised buttons that press in with a dotted focus rectangle inside, a slider's sunken groove and raised thumb, and in
Chromium and Safari scroll bars with grey arrows, a raised thumb and the dithered track (the others the browser's own, in the
skin's grey). The pictures are SVG of our own, drawn pixel by pixel in web/skins.css. Nothing in the skin has rounded corners
but the round things (an option button, a game's cap). A drop-down's list is the browser's own. The controls' keys, focus
order and names are the pages' own.

**The keyboard in Desktop 95** (the user, 2026-11-08: "Very unclear how Tab is behaving on that skin"). Before this, Tab went
through the desktop's icons and then through every control of every window in the page's order (on the listing, the 150-odd
links and stars of All games before About and the task bar), whatever window was in front, and a window it reached did not come
forward. Now the desktop is in panes, each open window, the icons and the task bar: Tab and Shift+Tab go round the controls of
the pane with the focus, in reading order, and never leave it, as in a dialog of the time; closed and minimized windows are out
of it. F6 and Shift+F6 go to the next pane or the one before (the browser keeps Alt+Tab, Ctrl+Esc and the Windows key), back to
the control that last had the focus there. A window that takes the focus, by a key or a click, comes to the front and its title
bar is the active one; the others' are grey. In the icons and on the task bar the arrows move between the items. In the game's
window the canvas, once it has the focus (a click, or F6 to the game's window while it runs), gets every key, Tab too, since
some games use it; F6 leaves it. Every focused control is marked: the dotted rectangle inside a button and around a check box's
or option button's label, a tab, a link or a list's item; an icon's name in navy with the dotted rectangle; a menu's entry in
its navy bar. About says it in one line: Tab within a window, F6 between windows.

**What is selected and dragged.** The game's canvas and its stage start no text selection and no drag (the user,
2026-11-08: "A lot of elements on Windows 95 need to be made unselectable so that I'm not dragging around the canvas contents as
if it were an image"), and the page's own images (caps, icons, logos) are not picked up; a card over the stage keeps its text,
and the game's own drags, its focus, touch and right click are its pointer events, which this leaves alone. In Desktop 95 the
chrome takes no selection, as in 1995 (one rule on the page, web/skins.css): title bars, frames, icons, the task bar, menus,
buttons, tabs, labels and status bars, so a double click on a title bar or an icon selects nothing; what is there to be read or
copied keeps it: text fields, the chat's messages, the notes and About, the terms, the settings' explanations.

On a screen under 760 pixels wide a game's own page is laid out as usual, with its footer. The performance and controls panels, when they come, are added as windows with `ByopopSkin.desk.add(element, {id,
title, ...})`. The parody keeps to the rules of the look: everything drawn here, system fonts by name, no real logos, icons
or sounds, and no imitation system prompts.

### Light and dark

The light and dark theme (the user, 2026-11-03), its button and `web/theme.js`, went with the other skins on 2026-11-09:
Desktop 95 has no dark version, so every page is light whatever the browser's `prefers-color-scheme`, and the pages' own dark
rules are gone (`web/index.html`, `web/shell.html`, `web/copyright.html`, Jungle Juice, `web/settings.js`, `web/chat.js`,
`web/logo.js`, `web/skins.css`). A stored `byopop-theme` is removed when a page loads. The games' own `page.css` files still
carry dark rules, which nothing on the site needs now; they are the game sessions' to take out.

### The fixes mod on the site

Every game is served with its fixes mod on (`docs/MOD_ROADMAP.md`, "The fixes mod"): one mod, `fixes`, each of whose settings
is a toggle for one fix. A game's mod lives in `games/<game>/mods/fixes`, and the web build copies the folder beside the game's
page as `<game>-fixes/`, with its `version` and its list of files written into the page (`FIXES` in `web/shell.html`, from
`cmake/ByopopWeb.cmake`). A game in `BYOPOP_WEB_NO_MODS` gets none, since its port cannot load mods yet; it gets its fixes mod
once it can. A game without the folder ships nothing, and its page does nothing.

Once the game's files are here, the page's boot puts the mod into `/persist/mods/fixes`, the IndexedDB-synced folder the Mods
panel adds to, before the game can start (the status line says so while the files are written). It does this after the
panel's own boot, so a removal that waited for this start has happened by then. Like any mod new to `mods.json` it starts on,
with every fix at its default, which is on. A note beside the saves, `site-fixes.json` in the game's user dir, remembers what
happened: `{"installed": "<version>"}` once it was put in, `{"removed": true}` once the player removed it.

- **On every start**, the page compares the version of the mod in the folder with the one shipped. When the shipped version is
  new, it replaces the mod's files. `mods.json` (whether the mod is on, and its place in the order) and `mod-settings.json` (its
  toggles) are left alone, so the player's choices hold.
- **The player** turns single fixes off or on in the game's mod menu (F2), turns the whole mod off to play the original as it
  was, or plays once without mods from the rescue, as with any mod. Removing it in the Mods panel deletes its folder; the
  next start finds the note saying "installed" and no folder, notes it as removed, and never puts it back. If the player
  later adds a mod into the `fixes` folder themselves, the note follows that copy again.

A test build can ship a stand-in: configure with `-DBYOPOP_WEB_TEST_FIXES=<game>` (cmake's cache keeps it; set it back to
empty afterwards) and that game, if it has no fixes mod of its own, gets `tools/webtest/fixes`, a mod with one toggle that
changes nothing. It was tested on Bejeweled in the pane at `?silent=1`. The first start put the mod in and listed it as on.
With the mod turned off and its toggle off, a reload kept both. A copy marked as an older version was replaced by the shipped
one with both choices still kept. Removed with the Mods panel's Remove, it stayed out after a reload.

### Site settings

Site-wide overrides of the games' own options, which the player sets once and every game page applies when its game starts.
**Game default** leaves an option to the game, and it is what every setting starts as, so a player who never opens the settings
sees every game as it is. They are edited in two places that draw the same controls from one file, `web/settings.js` (copied
next to the pages and loaded by the listing page and by every game page): the **Settings** tab of the listing page, and the **Settings** panel of a game's page (the bar's Settings button, a side panel like Mods and
Game files, whose heading is still "Settings: site-wide overrides", though it also offers "This game only").

The settings are in sections (the user, 2026-11-08: "Settings should be split into sections with icons so the window isn't so
massive"), each a handful under a plain name: Player names, Sound, Appearance, On screen, Input (with the site's mouse
pointer), Starting, Game, Page and Chat options. A row of tabs with each section's icon and name shows one at a time (the arrows,
Home and End move between them). In Desktop 95's desktop the Settings window is a folder of their 32-pixel icons, chosen and
opened as the desktop's icons are (a double click, Enter or Space), and each section opens as its own small window, with its
icon and name on its title bar and only its settings; they apply as they change, as everywhere, so there is no OK or Apply. The
Pop menu's Settings has the sections beside it, and on the listing `#settings` opens the folder and `#settings-<section>` (`players`,
`sound`, `look`, `overlays`, `input`, `start`, `game`, `page`, `chat`) a section. The icons are our own, drawn in the 1995 manner
in the 16-colour palette at 32 and 16 pixels by tools/settingsicons.py, which writes web/settingsicons.js. Both begin with "For every game. Game default leaves a game as it is." The listing
page's tab is site-wide. The panel starts with a choice, **All games
(site-wide)** or **This game only**: "All games" edits the same stored values as the listing page; "This game only" edits values
that only this game has (below), and every control then starts with "Same as all games (<the value it follows>)", for example
"Same as all games (Game default)", which shows what the game inherits and removes the game's own value when chosen. In "All
games" a setting that this game overrides says so under its control.

Every setting can be set for one game. The item has one more field, `games`, an object from a game's id to that game's own
values (`{"games": {"bejeweled": {"fullscreen": "off"}}}`). A game uses its own value when it has one and the site-wide value
otherwise (`ByopopSettings.get(name, gameId)`), and "game" is a value like any other (a game can be left as it is while all
the others are set). A game's own value is stored even when it equals the default, since it then overrides the site-wide
one; removing it removes the field, and a game with none leaves no entry. Fields the code does not know are kept inside
`games` too.

They are one localStorage item, `byopop-settings`, a JSON object with one field per setting. A setting that is not in it
has its default, and the default always leaves the game as it is: **"game" keeps the original's behaviour**, so the unmodded
games stay identical to their originals. A choice is `"game"` (the default), `"on"` or `"off"`; a level is a whole number from
0 to 100; a switch is `"on"` or `"off"` and is `"off"` by default, except `showChat`, which is `"on"`. Setting a default removes the field. Fields that `settings.js`
does not know are kept when it writes. Every read and write is in a try/catch: with storage blocked the pages behave as if
nothing was set, and the controls say a setting was not kept.

The settings, by their field in the item, with the wording of their controls (each has a short hint, behind its "?" or, in Desktop
95, What's This? and F1, that says what it is for, in `FIELDS` in `web/settings.js`):

- `startVolume` (a switch), `startMusic` and `startSound` (levels, 50 until moved): **Set the starting volume**, a checkbox
  with a "?" button beside it, and when it is ticked two sliders, **Music** and **Sound**, each from 0 to 100 with its number
  beside it (a real `<input type="range">` and a number box that follow each other; the level is kept when the slider is let
  go). In "This game only" the checkbox is a choice, "Same as all games (...)", "Game default" or "Set the starting volume",
  and the sliders are the game's own levels. The "?" is a real button with `aria-expanded` and `aria-controls`; the explanation
  shows while the pointer is over the button or the text, while the button has the focus, and after a click (a second click, Esc or
  a click elsewhere closes it; a pointer has a moment to move from the button to the text; Esc does not close the panel around it),
  and it is also the description of the checkbox (`aria-describedby`): "Most games start with music and sound on, so a game
  can be loud before you can reach its options. This sets the volume every game starts at, 0 for silence. You can still change it
  in the game." It replaces the earlier "Start muted", "Music volume" and "Sound volume" (below, "The old volume
  settings"). What it does is in "The starting volume", below.
- `fullscreen` ("game", "on" or "off"): **Full screen at start**, with the choices **Game default**, **Full screen** and
  **Windowed**. "Full screen" asks for full screen when the player presses Play, by the same path as the Fullscreen button
  (browsers allow it only from a click, so the restart of the game's own menu, which has no click, does not ask, and a browser
  may refuse). "Windowed" keeps the game in the page (below, "Windowed").
- `cursors` ("game", "on" or "off"): **Custom cursors**, with the choices **Game default**, **When available** (`"on"`) and
  **Never** (`"off"`). "On" or "off" calls the game's `EnableCustomCursors(true/false)`. The export
  `byopop_site_apply(cursors, music, sfx)` does it and the volumes; it returns 0 until `SexyAppBase::Init` has
  finished, which is after the game has read its saved options, so the setting wins over them, and the page calls it again
  every 100 ms until it returns 1. Only SexyAppFramework games have the option; for the others it does nothing and
  raises no error. "Game default" leaves the game's own option alone.
- `accel3d` ("game", "on" or "off"): **3D acceleration**, with the choices **Game default**, **Always** (`"on"`) and
  **Never** (`"off"`), and a "?" explanation (the user, 2026-11-03: "add a sitewide setting between game's default, always use
  3D, and never like with other settings"). Before the game starts the page calls `byopop_site_accel3d` with -1, 1 or 0, as it
  calls `byopop_site_windowed`, and a game with a 3D option reads `Byopop::SiteAccel3D()` (`framework/WebSettings.h`, web
  only) where it reads that option: -1 the game's own choice, 1 always, 0 never. "Always" is 3D whenever the browser can
  (WebGL2): a browser without it still gets 2D, which the game checks as it does for its own option. The games that read it
  are listed in `ACCEL3D_GAMES` (`web/settings.js`), and a game page of any other game leaves the setting out (the listing
  page shows it, its explanation naming such a game). Bejeweled Twist and Bookworm Adventures 2 read it now (Volume 2 through the framework: `SexyAppBase::InitDDInterface` lets it decide the run's 3D flag in place of the saved
  "Is3D" and writes nothing); Bookworm Adventures 1 and Rocket
  Mania, whose originals also have a 3D option, may read it once their passes settle their 3D modes. BookWorm will not: its
  option only switches the framework's renderer, which the port always runs on the GPU. The setting
  applies at the next start, and the panel says so.
  Under the setting a line says what this browser offers (the user, 2026-11-03), tested as the games test it: a new
  canvas's `getContext("webgl2")`, then one with `failIfMajorPerformanceCaveat`, each context let go at once
  (`WEBGL_lose_context`): "3D available", "3D available, but without the graphics card" (WebGL2 only without the flag:
  SwiftShader with the browser's hardware acceleration off, say) or "not available, games use 2D" (`accel3d()` in
  `web/settings.js`).
- `profile` ("game", "fill" or "create"): **Player names**, with the choices Game default, **Fill in the name** and **Use the
  player**: what a game does with the site's current player (below, "Player profiles"). The setting is applied when a game
  starts, and the panel says so.
- `wakeLock` ("on" or "off", default off): **Keep the screen awake while a game runs** (`navigator.wakeLock`), taken after
  the game starts, released when it ends or the page is hidden, and taken again when the page is shown. A refusal or a
  browser without the API is fine.
- `confirmLeave` ("on" or "off", default off): **Ask before leaving a page while a game runs** (`beforeunload`). A
  closed game's page, and the reloads after a close, do not ask.
- `pauseHidden` ("on" or "off", default off): **Pause when the tab is hidden**: when on and `document.hidden`, the page stops
  the game's main loop and suspends the audio context, and resumes both when the tab is shown. Off must stay as it was, since
  the originals keep running in a window you cannot see. The Browser pane can be hidden while `document.hidden` stays false, so
  this does not touch the hidden-pane tests; test it by overriding `document.hidden` and dispatching `visibilitychange`.
- `autoStart` ("on" or "off", default **on**): **Start the game when its page opens**. Once the game's files are found, the
  Ready card starts the game on its own when the browser lets the page play sound without a click or key (the user, 2026-11-02),
  and otherwise waits for Play as before. Chrome and Edge allow it after a click on a page of the same site that led to this one
  (a game opened from the listing, a reload) or when the site may always play sound; Firefox when the site's Autoplay permission
  allows audio. Firefox answers `navigator.getAutoplayPolicy("audiocontext")`; other browsers are asked with a new
  AudioContext, whose `resume()` settles at once only when sound is allowed (300 ms at most). It does not start after a start
  that did not finish (the card offers the rescue), with "Full screen at start" on Full screen (only a click may ask for full
  screen), with `?silent=1` (tests press Play themselves; the Browser pane may allow sound without a click), or once a panel is
  open or the card has gone. The game then starts with sound, so the status line does not add "sound after a click or key".
- `quitHome` ("on" or "off", default off): **Go back to the games after quitting**. When the game quits by itself (its own
  Quit or Exit), the page waits for the saves to reach the browser's storage as always and then goes to the listing page
  (`./`, with `?silent=1` kept) instead of showing the "Thanks for playing" card with **Play again** (the user, 2026-11-02:
  "Add option for 'Quit Game' to take the player back to the home page"). It starts off, because the card is what a player who
  quit by mistake needs: Play again is one click away there, and the listing is always one click away on the logo. A restart
  from the mod menu, the Game files panel closing the game and a game that stopped with an error keep their cards.
- `showChat` ("on" or "off", default **on**): **Show chat**. A game's page draws the chat beside the game (below it on a narrow
  screen) unless it is off, and with it off nothing of the chat is built or started. It starts on because
  the chat is a part of the page rather than a change to a game, and it can be turned on and off while a game runs ("The chat",
  below).
- `chatPlace` ("edge" or "game", default edge): **Chat position**. Beside the game the chat sits at the page's right edge, or,
  with "Next to the game", right next to the stage, the two centred together (the user, 2026-11-07). Below the game it is the
  same either way.
- `pixelPerfect` ("on" or "off", default off): **Pixel-perfect**, in Display. The stage takes the game's own size instead of
  filling the window (the user, 2026-11-07): each game pixel is n device pixels, n the device pixel ratio rounded and at least
  1, so at 100% and 125% a 640 x 480 game takes 640 x 480 device pixels and at 150% and 200% twice that. A smaller n is taken
  while that does not fit the window, and the usual fit when not even one device pixel each does. The game gives its size as it
  makes its window (`SexyAppBase::MakeWindow` calls `Module.onGameScreen`); full screen is unchanged.

**The starting volume.** Most of the games start with music and sound on, and the only way to avoid that used to be the speakers,
so the setting says what volume every game starts at, 0 for silence, music and sound each. The page's mute
(`Platform::Mixer::SetPageMute`, the exports `byopop_audio_mute` and `byopop_audio_muted` in `framework/WebSettings.cpp`) is the
platform's audio output: the mixer goes on mixing, as with `?silent=1`, and the device gets silence. It sits below every game's
volume, mute and options, so it changes and saves none of them, and it reaches every game that plays through the mixer, the
SexyAppFramework ones and the engine ones alike. With the starting volume on the page holds that mute from the moment it opens
until the volumes have been applied, so that nothing plays at the game's own volume first, and then lets it go (the bar's
**Sound** button shows "off" meanwhile; if the button is used in the meantime, the player's choice stays). The hold also ends when the game has not finished starting a minute after Play. The **Sound** button is the page's own mute, separate from this setting, and
acts at once in any case. `?silent=1` stays silent whatever the settings say. How the levels reach a game depends on its sound:

- The SexyAppFramework games take them through `byopop_site_apply`, as the game's own music and sound volume options do
  (`SetMusicVolume(level / 100)`, `SetSfxVolume(level / 100)`), so a level of 100 is the game's full volume. The call returns 0
  until the game's own start has read its saved options, so that the site's value wins; the game then saves the volume as
  its own, as it would if the player had moved its slider.
- TipTop, Ning Po and Word Harmony keep their volume in an engine of their own, which SexyAppBase's volumes do not reach.
  Each has a web-only file that registers a function (`Byopop::gSiteVolume`, `framework/WebSettings.h`), which
  `byopop_site_apply` calls in place of `SetMusicVolume` and `SetSfxVolume`, and which returns 0 until the game's own start has
  read its saved volumes: TipTop sets its `gSoundVolume` (0 to 0xffff, as the options' slider does) and its music table
  (`BuildMusicVolumes`, 0 to 0x100); Ning Po its two settings (`mSoundSetting`, `mMusicSetting`, 0 to 1, as its sliders do;
  the playing music takes the music one once `init()` is done); Word Harmony its two category volumes (0 the sound, 1 the
  music): `Game::applyVolumes` sets a category to 1 when its options box is ticked, and under a site volume to the site's level
  instead (`SiteCategoryLevel`), so the box still turns it off.
- Cosmic Bugs and Water Bugs have no volume option and fix the output volume at the original's 60%, so the page sets the
  mixer's music and sound master gains instead (`Platform::Mixer`, `SetMusicMasterGain` and `SetSfxMasterGain`, through
  `SetSiteGains` and the export `byopop_mixer_gain`), at once when the game starts. The games set the music gain themselves
  at each level, so the page's level is kept apart (web only, in `platform/Audio.h`) and multiplies what the game sets; a level of
  100 leaves their volume as it is and 0 is silence (the list is `MIXER_VOLUME` in `web/shell.html`, or `PAGE.mixerVolume` in a
  game's `page.js`).

**Windowed.** SexyAppBase always starts the web build windowed (`SexyAppBase.cpp`, window creation: browsers enter full screen
only from a click), so the SexyAppFramework games, which keep their screen mode in `mIsWindowed`, never start in full screen
and need nothing. The games whose own option can say full screen are Word Harmony (the registry value `FullScreen`, which
defaults to 1: at its start it asks the page's stage for full screen, which a browser grants only soon after a click),
Cosmic Bugs and Water Bugs (`cp.cfg`'s `fullscreen`, which defaults to 1: the web build keeps them in the page whatever the
option says, but their options menu says full screen), and TipTop and Ning Po (default windowed; they ask for full screen when
their saved option says so). With "Windowed" the page calls `byopop_site_windowed(1)` before the game starts, and each of them
reads `Byopop::SiteWindowed()` where it reads that option and starts with the option off: Word Harmony's host reads
`FullScreen` as 0, TipTop sets `gFullScreen` before `InitScreen`, Ning Po sets `mFullscreen` after reading its settings, and
Cosmic Bugs and Water Bugs call `Screen_SetFullscreen(0)` after `LoadConfig`, as their options menu does. The game saves the
option as it would if the player had set it, so a later "Game default" starts windowed too. "Game default" and "Full screen" call
nothing.

**A refused full screen.** A game's own switch (Alt+Enter, an Options checkbox) asks SDL, and a browser grants that request
only while a gesture of the player's is active (a key or a click in the last few seconds) and where the page may have full
screen (an embedding page can forbid it). Asked otherwise, SDL's window took the screen's size at once while Emscripten held the
request for a gesture to come, so the game went on in its 640x480 canvas drawn several times too big, its clicks off (TipTop's
Alt+Enter from a test's DOM events, 2026-11-08). `SexyAppBase::SwitchScreenMode` now asks only when the browser would take the
request (`sexyapp_fullscreen_allowed`: full screen enabled, and `navigator.userActivation.isActive`, which is Emscripten's own
test), and otherwise stays windowed and logs "SwitchScreenMode: the browser would refuse full screen now". A request it makes
can still be refused later: the Browser pane takes about 15 s to refuse one that followed a real click, and refuses one without
a gesture at once with "Permissions check failed". Emscripten has sized the canvas for the screen by then, and puts it back only
when full screen ends, which a refusal never does. So the switch watches its request (`sexyapp_watch_fullscreen_refusal`): on
its `fullscreenerror` it sends the page a `fullscreenchange` with no full-screen element, which Emscripten and SDL take as the
player leaving full screen; the canvas gets its size back, SDL's window is windowed, and the framework's `mIsWindowed` follows
at the next `SIZE_CHANGED`. Both were tried on TipTop in the pane: the game stayed drawn at its size, and the next click hit its
button. A game that keeps its own copy of the mode keeps the copy it set (TipTop's `gFullScreen` says full screen after a
refused switch, so its next Alt+Enter only puts the copy back), as when the player leaves full screen with Esc.

The settings apply when a game starts. In a game's page, the Settings panel also applies at once what can be: the starting
volume and custom cursors (to the running game), the wake lock and pause when hidden; "ask before leaving" is read when the
page is left, and the Sound button acts at once. "Full screen at start", "3D acceleration" and "Player names" only apply at the next start, and
the panel says so. Choosing "Game default" again leaves the running game as it is.

**The old volume settings.** Until 2026-10-27 the volume was three settings: `mute` ("game" or "on"; "Start muted"), and
`musicVolume` and `sfxVolume` ("game" or 0 to 100). `settings.js` migrates a stored item that still has them when it reads it,
and writes the item back without them: `mute: "on"` becomes the starting volume on with music and sound at 0; stored volumes
become the starting volume on at those levels (50 for the one that was left to the game); both together give 0 and 0, since the
mute is what the player asked for first. The same goes for each game's own values in `games` (a game's missing values fall back on
the site's old ones; a game that was left to itself while the site's were set gets its own "off"). Settings left to the game
leave nothing. `tools/profilestest.js` checks it.

**To come: the controls panel and "Always skip tutorials".** The site never shows a game's own information elsewhere on the
page, with one exception the user made for controls (2026-11-07: "I ruled out re-presenting information from games to display
elsewhere on the page, but I think we should make an exception for controls"). Each game's controls will be written in its
page data, in the site's own short words and never the game's text, and shown beside the game; the remappable keys of the
controls setting use the same list. "Always skip tutorials", off by default, will use each game's own switch (a registry value,
a profile flag or an option) through the site's settings hook, leaving the ports unchanged; a game with no such switch is
listed and left as it is.

### The chat

Every game's page has one chat, the same room on all of them, in the `<aside id="chat">` that `web/shell.html` puts after the stage
and `web/chat.js` fills. For now it is a test: the messages are made up, the ones a player sends stay in this browser, and the
page makes no network request for it. `docs/CHAT_DESIGN.md` says how the panel and its source work, how it is laid out, and what a
server made only of Cloudflare's services could look like. A line reads `HH:MM [BW] <username> text`: the tag is the initials of the
game the message was sent from, a link to that game's page with the game's full title as its hover text. The shell hands the chat
its own game id (`PAGE.id`) and the table of the site's games, `chat-games.js`, which `byopop_web_index()` in
`cmake/ByopopWeb.cmake` writes beside the pages from `BYOPOP_WEB_BADGES` and the listing page's titles (so a new game's badge row
adds its tag, and the chat keeps no list of its own); a game the table lacks has no tag. The part that touches the shell is small: the page's body is a
`.workspace` row holding the `<main>` (the stage) and the chat, and the chat goes beside the stage only when the stage keeps the
width it has without it, else below it (the class `chat-below` on the root element), so the canvas and its scaling are never
changed by the chat; `Module.onGameScreen` asks the chat to work that out again when a game changes the stage's shape; the
"Show chat" setting above turns it on and off live; and the terms card and the "Already open" card make it inert like the rest
of the page. Below the stage the chat fills the space down to the footer, at least 18rem tall (the user, 2026-11-07), and
with "Chat position" on "Next to the game" the stage's box is only as wide as the stage (chat.js gives `--stage-px`), so the
two centre together. Below the stage the page is taller than the window, and the player must land on the game, scrolled to the top: the
page sets `history.scrollRestoration` to `manual` (else a reload, as Play again and a restart from the mod menu do, puts the page back
where it was scrolled to, the chat in view), and Home, End, PageUp and PageDown typed on the canvas are cancelled for the browser, which
would scroll the page to the chat with them (SDL cancels the arrows, Tab, Backspace, the function keys, Ctrl keys and Space, not
these); the game still gets them. Nothing on the page moves the focus into the chat by itself.

The chat's header shows facts as values, not sentences (the coordinator, 2026-11-09): a TEST tag (`chat.test`, an `abbr`
whose tooltip and accessible description are the old note, `info.note`, that the messages are made up and nothing is sent
anywhere), and a Sign in button that is disabled, its tooltip saying sign-in is not available yet (`chat.signin_note`). The
Player names section's line about modes shows only when the games differ: "Player names:" and each mode with the number of
games that use it (`players.mode.note`, `players.mode.part`), hidden while every game uses the same one.

### Player profiles

The player gives a name once, and the games use it: for the player a game keeps (its profile: the options and the progress
that go with a name), and for the name it asks for at each high score. The site has its own list of players, which the player
manages on the listing page (the **Players** section), and a setting, **Player names**, says what a game does with the one
in use. Nothing is sent anywhere: the list is kept in this browser, and so is what each game reports of its own players.

**The site's list** is the localStorage item `byopop-profiles`, `{profiles: [{id, name}], current: id}` (`ByopopProfiles` in
`web/profiles.js`, loaded by the listing page and every game page). A name is trimmed, runs of blanks become one space, control
characters go, and it is at most 32 characters; two players never have the same name whatever the case, since the games compare
names without regard to case. The first player added is the one in use. Every read and write is in a try/catch: with storage
blocked the page behaves as if there were no players and says a change was not kept.

**The setting `profile`** (in `byopop-settings`, so it can be set for all games and for one game, "Site settings", above) has
three values. `game`, the default, leaves the game as it is: it asks for a name as the original does, and no hook changes
anything, so the unmodded game plays exactly as before, with no change in its random numbers or anything else. `fill`
pre-fills the names the game asks for with the site player's name (a new player's name, a high score's name) and creates
nothing. `create` does that and, at the start of a game that keeps players, selects the game's player of that name, making it if
the game has none, by the same functions the game's new-player dialog uses, so the dialog never opens. A game without players
treats `create` as `fill`. With no site player, all three behave as `game`.

**A game pulls, the page never pushes.** A game's new-player dialog opens at start, before any message from the page could
arrive, so the page cannot win a race against it. Each game asks at its own decision points, through
`framework/WebProfiles.h` (web only; natively the header and its `.cpp` are empty): `SiteGet(id)` is an `EM_JS` call that
returns the mode, the current site player's name, the site players' names and the operations waiting for the game, as JSON
that `ByopopProfiles.forGame(id)` makes. Names cross as UTF-8 and become the game's text (Latin-1) at the border: what the
game cannot show is left out. The game's part is small and in a file of its own (`games/<game>/src/SiteProfiles.cpp`, empty
natively), plus one `#ifdef __EMSCRIPTEN__` line at each of its decision points. The file implements `SiteProfiles`, the game's
players in its own words: its names, the current one, `Find` (the game's own name for the one it counts as the same), `Fit` (a
name as its name box keeps it), `Create`, `Select`, and `Rename` and `Delete` where the game can. The framework does the rest:

- `SiteStart(id, game)` at the point where the game would decide to ask for a new player (and where its fonts are ready, so
  `Fit` knows the width): it applies the queued operations, makes and selects the site player under `create`, and reports. For a
  SexyAppFramework game the framework calls the start itself: `SexyAppBase::DoUpdateFrames` calls `Byopop::SiteLoaded` right after the
  game's `LoadingThreadCompleted`, and the game's web-only file registers its start function in `Byopop::gSiteLoaded` with a static
  initializer, so none of the game's own sources has a line for it. The engine games (TipTop, Ning Po, Word Harmony, Cosmic Bugs,
  Water Bugs) call `SiteStart` from their own start (the end of `GameInit`; `NingPo::init` after `loadSettings`; `Game::configure`;
  the end of `GameInit`).
- `SiteScoreNames` is the whole hook of a game that keeps no players and only asks for a name at each high score: it lists the
  last name the game remembers (the registry value `LastScoreName`, or nothing), renames it, and fits a name by typing it into a
  box with the game's font and limits. A game that remembers no last name (TipTop) lists the names in its high-score tables
  instead, leaving out the table it starts with, and calls `SiteScoresChanged` when a name enters a table (a6547b9).
- `SiteChanged(id, game)` wherever the game's list changes (a player made, renamed, deleted or chosen): it reports again.
- `SiteType(id, editWidget)` right after a SexyAppFramework name box is made: under `fill` and `create` the site player's
  name is typed into the box through `EditWidget::KeyChar`, one character at a time, so the box's own rules (its characters, its
  length, its width in the box's font, the game's `AllowChar`) keep or drop each character exactly as typing would, and the
  box ends as the game's own `SetText` leaves it (the text selected). Under `game` it returns at once. Games with another
  kind of box (TipTop, Word Harmony, Ning Po) ask `SiteNameFor(id)` and apply their own rules to the name.
- `SiteFitTyped(...)` is the typing without a box on screen, for a game's `Fit`.

**What a game reports** is the localStorage item `byopop-game-profiles`, `{<game>: {kind, names, current, fits, rules, max,
canRename, canDelete, deletes, at, results}}`, written by `ByopopProfiles.report` whenever the game's list changes and at every
start. `kind` is `profiles` for a game that keeps players and `scores` for one that only asks for a name at each high score
(`names` is then the last name it remembers, for the games that remember one). `fits` maps each site player's name to the name
the game's rules make of it (empty: nothing is left), which is how the Players section shows where a rule shortened or changed a
name. `rules` is the game's name rules in words, `deletes` what the game's own delete removes.

**Operations** from the Players section wait in `byopop-profile-ops`, `{<game>: [{id, op, ...}]}`, because a game that is not
running cannot be reached: `rename` (`from`, `to`) and `delete` (`name`). The game applies them at its next start, in
`SiteStart`, with the same functions its dialogs call, and its report names each by its `id` in `results` (`done` or
`skipped`, with a note), which removes it from the queue and keeps the result with the game's record. A rename to a name the game
already has is skipped and reported: two players' progress is never merged. A rename whose result differs from what was asked
(the game's rules shortened it) says so. The Players section shows operations that have not been applied as pending, per game,
until the game has started once.

**The Players section** (listing page, `#players`) adds, renames and deletes the site's players and picks the one in use. For
each player it lists the games that have one and the exact name each uses, with a flag where a game's rules shortened or
changed it. It ticks any number of game players, across games and with different names, and renames them all to one name in
one step; skipped games are reported afterwards. It deletes a player inside the games, which deletes that player's progress in
each game, after an in-page confirmation that lists what will be lost. It uses real form controls with labels, and never
`alert()`, `confirm()` or `prompt()`.

**The survey.** All eighteen games were read for these facts: whether the game keeps players (create, select, rename, delete, and where
each happens), whether it asks for a name at each score, the rules of its name boxes, and where it decides at start to show its
new-player dialog. "SiteStart" is where the game's hook runs the page's start: for a SexyAppFramework game it is the framework's
`SexyAppBase::DoUpdateFrames`, right after the game's `LoadingThreadCompleted`, when its fonts are loaded and its saved options are
read, and before its first frame that could ask for a name.

| Game | Players | Name at each high score | Name rules | Where it decides to ask |
| --- | --- | --- | --- | --- |
| Bejeweled Deluxe | none | yes (RecordScoreDialog), starts with the last name (LastScoreName) | as many characters as fit 104 px of the main font; ASCII 32 to 127; case kept | the dialog's constructor |
| Big Money Deluxe | none | yes, starts with the last name | 140 px of font 3; ASCII; case kept | the dialog's constructor |
| BookWorm Deluxe | up to 5 in the registry (`Users\<name>`): the new-user dialog makes one (by itself when there is none, else from the list's "create" line), the list chooses and (offered with exactly 5) deletes, the saved games stay on disk; no rename | no (the hall of fame uses the player) | letters and spaces; 22 characters and 200 px of Verdana 12 bold; ends trimmed; unique ignoring case | `BWGameSelector::Update` when no player exists, the box starting with the Windows user name when it is letters and spaces |
| Noah's Ark | none | yes, starts with the last name | 120 px; printable ASCII (32 to 126); case kept | the dialog's constructor |
| Seven Seas | none | yes, starts with the last name | 104 px; printable ASCII; case kept | the dialog's constructor |
| Atomica | none | yes (the monitor's name line), starts empty and is not remembered | 20 characters; ASCII | the dialog's constructor |
| Mummy Maze Deluxe | five named saved adventures (`adv0.sav` to `adv4.sav`, each starting with its name): the new-adventure dialog names one when a free slot is chosen, the delete dialog removes one; no rename | yes in the classic modes, starts with the last name | an adventure's name: letters and digits only, 20 characters, unique ignoring case (the OK refuses others); the score box 104 px, ASCII | the new-adventure dialog when the Adventure menu has no saved adventure, and when a free slot is chosen |
| TipTop | none | yes (a text field made at command 0x38), starts empty, not remembered | 10 characters; printable ASCII; case kept | where the field is made |
| Bookworm Adventures Deluxe | up to `MaxProfiles` (10), in `users/<name>.bwa`: the new-player dialog makes one (the first game's intro asks, or "new" in the main menu's list), the list chooses and deletes; no rename | no | letters, digits, space and `! @ # $ % & ( )`; 189 px of the large font; unique ignoring case; the box starts with "Lex" | the first game's intro (`StartIntro` with no profile) |
| Ning Po Mahjong | none | yes (`NingPo::enterName`), starts with the account's name, not remembered | 12 characters; printable ASCII | the best-time box |
| Pixelus Deluxe | any number, in `users/<number>` (the names inside): the new-player dialog makes one (by itself when there is none), "Who are you?" chooses, renames and deletes | no | letters, digits and spaces; 28 characters and 150 px of Garamond 14; leading, double and trailing spaces dropped; unique ignoring case | `PixelusApp::PlayClicked`, when the main menu would ask |
| Alchemy | none | yes (NameEntryDialog), starts with the last name | 104 px; ASCII; case kept | the dialog's constructor |
| Word Harmony | none | yes (`HighScoreEntryLayout`), starts with the last name (DefaultScoreName) | 16 characters; keys 0x20 to 0x87 | `HighScoreEntryLayout::startEntry` |
| Cosmic Bugs | ten slots in `cb.dat`: the menu chooses, renames and deletes (never the only player), a free slot asks for a name; the first run makes slot 1 after the account | yes at a new high score (a text input, starts empty) | the engine's text input: capitals; digits, letters, space and punctuation the font has; 12 characters for a player's name, 16 and 480 px for a high score | `LoadProfiles`, when the game starts |
| Water Bugs | as Cosmic Bugs | as Cosmic Bugs | as Cosmic Bugs | `LoadProfiles` |
| Rocket Mania Deluxe | none | yes, starts with the last name | 150 px; ASCII; case kept | the dialog's constructor |
| Dynomite Deluxe | none | yes, starts with the last name | 20 characters and 115 px; ASCII | the dialog's constructor (a second box names a saved game: it is not a player, and is left alone) |
| Bookworm Adventures Volume 2 | as Bookworm Adventures | no | as Bookworm Adventures | as Bookworm Adventures |

**What each game does under each mode.** Under `game` every hook returns before it does anything: the game asks and fills as the
original does, with no extra call and no random number drawn. Under `fill` and `create`, with a site player:

| Game | name boxes | at start (`create`) | renames | deletes |
| --- | --- | --- | --- | --- |
| Bejeweled, Big Money, Alchemy, Noah's Ark, Seven Seas, Rocket Mania, Dynomite | the high-score box starts with the name, typed through the box's own font and limits | nothing to make: as `fill` | the remembered last name (LastScoreName) | not offered (no player) |
| Atomica, TipTop, Ning Po | the high-score box starts with the name (its limit), instead of empty or the account's name | as `fill` | no name is remembered | not offered |
| Word Harmony | the box starts with the name instead of the last name | as `fill` | the last name (Settings::mDefaultScoreName, saved) | not offered |
| BookWorm | the new-user dialog's box (first run, and the list's "create") starts with the name, in place of the Windows user name | makes the player (`SaveUser`, the list, `SetCurrentUser`) or selects it, so no dialog; not when five players exist | a player of the new name with the old one's alias, options and saved games, then the old is deleted | `UserManager::DeleteUser` (saved games stay on disk) |
| Pixelus | the new-user dialog's box starts with the name, tidied as `GetName` does | `AddUser` and `Save`, or `Select`, so the menu never asks | `RenameUser` (the player keeps its number and progress) | `DeleteUser` (file and best results); the current player stays |
| Mummy Maze | the new-adventure box starts with the name in the form the game accepts (letters and digits), and the classic score box with it as typed | an adventure in a free slot (`CreateAdventure`) or the slot that has it | the name at the start of the adventure's file | the adventure's file |
| Bookworm Adventures, Volume 2 | the new-player dialog's box starts with the name, in place of "Lex" | a profile (`new Profile`, `Save`) or `SelectUser`, so the intro does not ask | the profile written under the new name (`Profile::Save`), its data file moved, the old file removed | the player's file, as the main menu's list does |
| Cosmic Bugs, Water Bugs | a free slot's and the rename's boxes, and the high-score box, start with the name; the first run's first player is named after it, not the account | a free slot (`Level_SetUserName`'s steps) or the slot that has it | the slot's name | the slot emptied as the menu's delete does; never the only player |

A rename is applied with the game's own rules: the name goes through the box's `Fit` first (shortened or filtered), a name another
player already has is skipped and reported ("Players are never merged"), and a rename whose name the rules changed says so. Under
`create`, a player deleted from the Players section is made again at the next start if it is the site's current player: delete it
under `fill`, or choose another current player first. Where a game's own rename or delete leaves part of a player behind (BookWorm's
saved games, Cosmic Bugs' saved game and records), so does the page's: it calls what the game's dialog calls.

### Save backup

The listing page's **Saves** tab (`#saves`; `web/saves.js`, copied next to `index.html`) backs up the player's saves and
restores them. The saves live in this browser only, and are lost when its data is cleared or the player uses another browser or
device, so the page makes one .zip of them, in the page and with nothing uploaded, and reads one back.

**Where a game's saves are.** The shell's folders (`web/shell.html`): a game's persist folder (`PAGE.persist` in
`games/<game>/web/page.js`, the name of its IndexedDB database, made by Emscripten's IDBFS: one object store, `FILE_DATA`,
keyed by path) holds `game/` (the imported files), `userdata/` (the game's saves, high scores, settings and registry),
`mods/` and, for the games that keep replays (`replays: true`: Bejeweled, Big Money, BookWorm), `replays/`. A backup takes
`userdata/` and `replays/`, and the game's own backup of `userdata/` in localStorage
(`sexy-userdata-backup:<persist>/userdata`, `platform/WebPersist.cpp`), which can hold a save that never reached IndexedDB
because the page closed first. It never takes `game/` (the files and the exe), the icons (they are in the ownership record, which
is derived from the game's files) or the mods. `byopop_web_index()` puts each game's persist folder, Web Lock name and replays flag
on its card (`data-persist`, `data-lock`, `data-replays`), read from its `page.js`, so nothing about a game is written into the page
by hand.

**Back up saves.** The page reads each game's database without making one (`indexedDB.databases()` first), and puts together the
files as the game would have them at its next start: it does what `webpersist_restore` does with the localStorage item, so a file
that the item has and is newer than the folder's (or that the folder lacks) is taken from the item, and a file that the item does
not list and that is older than the item was deleted before it, and is left out (`merged()`; `tools/savestest.js` checks it). The
backup is **one store-only zip** (no compression, no library: the page writes the headers and the CRC-32), named
`byo-pop saves <YYYY-MM-DD>.zip` and given to the browser as a download:

```
byopop-saves.json            the manifest: {format: "byo-pop saves", version: 1, made, games: {...}, files: {path: ms}, play: {...}}
byopop-settings.json         the item byopop-settings, as it was (when there is one)
byopop-profiles.json         the item byopop-profiles, as it was (when there is one)
<game>/userdata/<path>       a file of the save folder
<game>/replays/<path>        a replay
<game>/browser-backup.json   the game's localStorage backup item, as it was
```

The play times (the item `byopop-play`) are the manifest's field `play`, not a file of their own: a build from before them refuses a
zip with a top-level name it does not know, and reads past a manifest field it does not know, so a backup made now still restores
there (without the play times). For the same reason a restore skips a top-level `byopop-<name>.json` it does not know, one that a
later build may add, instead of refusing the zip (`classify()`; `tools/savestest.js` checks both).

Under the section the page says, for each game, what it took: the number and size of the save files, the replays, and whether
files came from the game's own browser backup. A game whose page is open in another tab is flagged: what it has not saved yet may be
missing from the backup until it is closed.

**Restore saves.** A file field takes such a zip. The page reads it (checking every file's checksum), sorts its entries, and
**refuses the whole zip** when one is outside a game's save folder (a path that is not `<listed game>/userdata/...`,
`<listed game>/replays/...`, one of the names above or another top-level `byopop-<name>.json`, which is skipped; a path with `..`, `.`, an empty part, a backslash, a control character or
more than 240 characters; a game that is not listed; a name twice), when it is not a backup made here (no manifest), is encrypted or
zip64, or is damaged. A valid one is not written yet: the page shows, in the page and not in a browser dialog, for each game what the
zip holds and how many of its files it would replace in this browser and how many it would add, that files that the zip does not
have stay, that each game's own localStorage backup is removed (the restored files take its place, and a left one would delete the
restored files at the game's next start), that the site's settings and players are replaced, and that the play times become, for each
game, the larger of the backup's and this browser's number of plays and of seconds and the later day (`mergedPlay()`), so that an
older backup takes nothing away. Only the **Restore** button writes.

A restore never runs while a game page is open: each page holds a Web Lock (`PAGE.lock`) while it runs, and the page checks them with
`navigator.locks.query()` when the zip is read and again at the button, and then holds every game's lock (`ifAvailable`) while it
writes, so that no game page opens in between (a game page waits for its lock before it reads its storage). A browser without Web
Locks cannot tell, so it backs up but does not restore. Each game's files are written in one IndexedDB transaction, so a game's
folder is restored whole or not at all, with a directory entry for every missing parent (IDBFS needs them) and the file's own date
from the manifest. The page then tells what it wrote and sends a `storage` event to its own sections so that the settings and players
show the restored values.

### The Library

Desktop 95's other window of the games, on the listing's desktop (web/library.js; the coordinator, 2026-11-08): a grid that
evokes a game client of 2004 in a look of its own (olive and khaki, flat panels with 1-pixel bevels, small Tahoma, an
amber-olive selection, square buttons and its own title bar, web/skins.css `.lib-*`), named for no real program and with no
real program's art. Each game is a row: its 16-pixel icon (the one from the player's files, else its cap's colour and
initials) and name, its status (Ready, Needs your files, Coming soon, No longer sold), its play time, the day it was last
played and its year. A click on a column's header sorts by it, a second click the other way, the choice kept in localStorage
`byopop-library-sort`; ties go by name, and the sort is announced and set as `aria-sort` on the header. A double-click or Enter
plays the selected game (its window on the one page, web/deskapps.js), as does the Play button above the grid; the Filter box
beside it narrows the rows to the names that hold its text, and the status bar counts them. A row's menu (a right click, the
menu key or Shift+F10) has Play, Add files (the game's window, whose setup asks for them), Add to or Remove from favourites,
Properties (a sheet of the game's values: name, status, release date, plays, play time, last played, Windows 95) and the
card's buy links, which open a new tab. The grid is one tab stop, its selected row's first cell: the arrows, Home and End, Page
Up and Down move the selection, and typing the first letters of a name goes to it; Tab moves between the Play button, the
Filter box, the column headers and the grid, and F6 between the desktop's windows. It is a real grid (`role="grid"`, rows
with `aria-selected`, each named by its game's cell, a row header, and column headers with `aria-sort`).
Everything comes from the listing's own cards (their files check, favourite and buy
links) and from ByopopPlay, so the two windows always agree; the rows are read again when the window comes to the front and
when a card changes. Its icon stands beside All games', and the Pop menu has it beside Games. The setting "Games open in"
(Look, Desktop 95 only; All games by default) chooses the window that the first entry of the Pop menu's Games and a closed
game's return open. Under 760 pixels the window is maximized like the rest, without the Last played and Year columns.

### The music player

The listing page's fourth tab, Music (`web/music.js`, the user's request of 2026-11-07), plays the music of the games the player
added, from those games' own files in this browser. The site ships no music and sends nothing anywhere: the tab reads each game's
database read-only (opened without a version, so a game that was never added gets no empty database) the first time the tab is
shown, and only for the games whose record says their files are here.

Which files hold a game's music is in `web/soundtracks.js`, by the game's id: path patterns in the game's folder as the import
writes it, matched without regard to case. A module that matches gives its subsongs as tunes, and an Ogg Vorbis file is one tune.
Subsongs shorter than two seconds are left out, since in these games they are the silences and stings between tunes (Atomica, Big
Money, Dynomite and Mummy Maze each have one or two). Two games play their music in ways the table describes as well:

- **Bonnie's Bookstore** plays its one module, `sound/music.oxm` (32 minutes as one subsong), as nine
  tracks, each a range of its orders, from the track table in its exe (`games/bonnies/game/Music.cpp`): the game starts a track
  at its first order and stops it once the order passes its last. The entry's `parts` lists those ranges, so the player plays
  each from its first order until the order after its last (libopenmpt's time for an order gives its length). Its logo tune,
  `sound/Logo.ogg`, is a tune of its own.
- **Pixelus and Rocket Mania** join their in-game music (and Rocket Mania its menu music) from short Ogg pieces in the order a
  playlist in the music folder gives: three header lines, then the first word of each line names a piece (Rocket Mania's
  `AssembleMusic`, `games/rocketmania/src/AppMusic.cpp`; Pixelus's playlist has the same form). The entry's `joined` names each
  playlist and the path of its pieces, and the player decodes the pieces and plays them one after the other, whole, as Rocket
  Mania's original does. Pixelus's in-game tune comes out at 3:42, the playlist's own total.

An entry can also list `skip` (tunes that are not tunes, as `"<file>#<n>"`) and `names` (a plain name for a tune). Tunes are named
plainly: the file's name, and "part n" when a file holds several; Bonnie's tracks are named for where the game plays them (the
menu, or in a game). No invented titles, and no long passages of a game's text. The module games are Alchemy, Atomica, Bejeweled,
Big Money, BookWorm, Bookworm Adventures, Bejeweled Twist (both of its modules), Cosmic Bugs, Water Bugs, Dynomite, Seven Seas and
Mummy Maze; the Ogg games are Bookworm Adventures Volume 2, Noah's Ark, Pixelus, Rocket Mania and Bonnie's Bookstore (its module
too).

The decoder is the games' own two module renderers, built for the web on their own as `openmpt.js` and `openmpt.wasm` beside the
pages (target `byopop_openmpt`, from `web/music/openmpt_web.c`, a handful of C wrappers: load, subsongs, select, orders, repeat,
length, render, seek, the tracker view's position and rows, and stb_vorbis's whole-file decode). As `Platform::Mixer` picks them, an MO3 or OXM plays through libopenmpt at the
port's settings and every other module (S3M, IT, XM) through libxmp-lite, so that a tune sounds as it does in its game (below,
"Music levels"); libxmp-lite's sequences are the subsongs there, and for every game's modules they are libopenmpt's, the same
number of the same lengths. It holds no game code, and the page loads it only when the tab is first opened. Its licence is
copied to `licenses/libopenmpt.txt`, which the tab links. An Ogg file is decoded whole by the browser (`decodeAudioData`, at the
AudioContext's rate); a browser that cannot gets stb_vorbis from the same wasm, and music.js resamples its output. A tune's length
in the list is read from the Ogg file's last page without decoding it. The main thread sends 4096-frame stereo chunks to an
AudioWorklet (its source is a string in music.js, loaded through a Blob URL), keeping about a second queued. The worklet says when it
has used a chunk, and when it runs dry after a tune's end, which moves on to the next tune; after the last tune the first one plays
again. A new tune flushes the worklet's queue, and the worklet answers the flush, so the messages about the tune before are ignored.

The player is in one state at a time: idle, loading, playing or paused. It is laid out as players of 1995 to 2005 were, by the
rules in "Values, not sentences" (below; the user, 2026-11-08: "Still too much text in the player. It is instantly obvious AI was
used"): one display, a seek bar under it and one row of controls, then the playlist. The display is an LCD-style readout: the
state (a small play, pause or stop sign, blinking while a tune loads or is paused), the time heard in large figures, the tune's
name with its game's below it smaller (each cut with an ellipsis; both whole in the tooltip; a live region for screen readers),
the tune's length, and under them the view. Where there is no tune to name it shows a value instead ("Reading…", "No music",
"Error"), the full sentence in its tooltip. The controls are icons of our own drawing (SVG paths in music.js), each named by
its tooltip and `aria-label`: Previous (the tune's own start once it has played more than 3 seconds, else the tune before),
Play/Pause, Stop (the tune stays chosen, back at its start), Next; Repeat (two arrows in a loop, pressed in while on: libopenmpt
repeats a subsong for ever, a range of orders or decoded sound starts again at its start); and the speaker, which mutes on a
click (its picture crossed out while muted), with the volume slider beside it (a GainNode). The time heard is the decoder's
position at the start of the chunk the worklet plays plus the context's time since it began it, so it is what the listener hears,
not the second ahead the decoder has rendered. A seek moves the decoder (libopenmpt to the time; libxmp-lite to the frame nearest
it, `xmp_seek_time_frame`; decoded sound to its sample) and flushes the worklet. The playlist is a line a tune under its game's
name: its number, its name on one line (cut with an ellipsis, whole in its tooltip) and its length, the one playing marked as a
selection. Repeat, mute, the volume and the view are kept in `localStorage` (`byopop-music-loop`, `byopop-music-mute`,
`byopop-music-volume`, `byopop-music-vis`). Where the music comes from and the libopenmpt and libxmp-lite credits are in the
listing's About (its footer), with links to their licences.

**The display's height** (the user, 2026-11-09: "increase how many rows show in the tracker") is set by a splitter between
the player and the playlist: dragging it gives the display whole tracker rows more or fewer, from 5 to 64, and the arrow keys
on it one row (Page Up and Down five, Home and End the least and the most). The spectrum takes the same height. It has no
words: the resize cursor (Desktop 95's own), and its name as a tooltip and for screen readers ("Display height", a
`role="separator"` whose `aria-valuenow` is the rows). The rows are kept for this browser (localStorage `byopop-music-rows`,
read and written in try/catch).

**Only the playlist scrolls** (the user, 2026-11-09: "only the list of songs should scroll within the music menu, so that the
tracker/spectral analyzer and controls are always visible"). In Desktop 95 the Music window's body never scrolls: the
playlist scrolls inside it with the window's drawn scroll bar (the `scroll: ".mu-list"` option of the window) and takes the
height the player leaves, so a taller window shows more of it and a shorter one takes the height from the list first. The
window keeps the player, the splitter and three of the list's rows: its least height (`minHeight` of the window, which
web/skins.js's resizing asks, from music.js's `minHeight`), and the splitter gives the display only as many rows as leave the
list those three in the window as it is. Rows asked past that, by a drag of the splitter or its keys, grow the window downward
by what they need (the user, 2026-11-09: "Doesn't seem like I'm able to expand the tracker view to add more rows"), as far as
the desktop's bottom edge and then 64 rows (`ByopopSkin.desk.grow`); fewer rows give the height back to the list and never
shrink the window. A dragged edge also stops at the desktop's own edges, so the window's bottom never goes under the task bar.
It is worked out again when the browser's window or the panel changes size (a `resize` listener and a
ResizeObserver; a hidden pane runs no ResizeObserver, so a test there fires `resize` after showing the panel). The rows asked
for are kept, so a larger window gives them back.

The view is the display's lower half, and a click on it (or Enter or Space) goes to the next: the spectrum, then the tracker,
then nothing, as players of the time cycled their visualisations. **Spectrum**: 24 bars from 40 Hz to 16 kHz, spaced as the ear
hears them, each with a peak that falls back (a Web Audio AnalyserNode, put between the worklet and the volume only while the
spectrum is shown, so the bars are the tune's own whatever the volume, and still move at `?silent=1`). **Tracker**, for a module's
tune: a terse header in tracker style ("ORD 07  PAT 06  ROW 16/64") and the rows around the one heard, it in the middle while the view can scroll to it (the view
never leaves the pattern: at its start the view sits at row 0 and the highlight moves down, near its end the last row stays at
the bottom, and a display with room for the whole pattern does not scroll at all, the rows below it left empty; the user,
2026-11-09: "it should never scroll empty lines into the top or bottom"), each channel's cell
as libopenmpt writes it (`openmpt_module_format_pattern_row_channel`) or, for libxmp-lite's modules, its note, instrument, volume
and effect (the effect as libxmp-lite's own type, in hex). Only the channels that play a note somewhere in the module are shown
(`bm_used`, read once a tune; the user, 2026-11-08: "the window isn't wide enough for all the tracker instruments"), each cell
whole: the full cell while every such channel fits with it, otherwise a tracker's narrow view of note and instrument, and as
many whole columns as the view's width holds, measured in the rows' own font. When some still do not fit, the header adds
"CH 1-12/37" and a scroll bar drawn by the player sits under the rows (Windows 95's in Desktop 95, a thin one in the display's
colours elsewhere; the browser's own bar draws no arrows in Chrome): its arrows move a channel, its track a page, its thumb
drags, Left and Right on the display do the same, and it follows nothing on its own. The player is as wide as its window or
panel, so widening it shows more channels. An Ogg tune has no patterns, so with the tracker chosen it shows the spectrum. While the tracker is shown and the player is on the screen, the decoder renders in slices of 512 frames and notes its
pattern and row before each, so the view shows the row of the slice being heard; otherwise it renders as before and is never
asked, so a closed view or window costs nothing. A frame's look at the clock and the views runs only while a tune plays (and
while the spectrum's bars fall back after it). In Desktop 95 the display is black with green figures in a sunken edge (the
spectrum's peaks yellow, the tracker's row in the selection's navy), the buttons are raised squares whose toggles stay pressed
in, and the playlist is a white list box with the navy selection. tools/musictest.js (unit-web) checks the decoder's view, rows
and seek on a small ProTracker module it makes, and on a staged MO3 when there is one. The Media Session API gets the
tune's name, the game as the artist and the game's icon, and its play, pause, next, previous and stop handlers, so the keyboard's
media keys and the system's media controls drive the player.

Closing the Music window stops the music (the user, 2026-11-09: "Music should quit playing if the window is closed"): its
Close button, its control menu's Close and its task bar button's Close all go through the desktop's one close
(`afterClose`, web/index.html), which calls `ByopopMusicPanel.shutdown()`. That stops the tune, disconnects the worklet, the
analyser and the gain, closes the AudioContext, frees the decoder's module and its view buffer, and takes the Media Session's
handlers and metadata away, so nothing is left rendering or answering the media keys. Minimizing keeps it playing. Reopened, the
window shows the player stopped with the last tune chosen, and Play loads the decoder again. `ByopopMusicPanel.held()` tells a
test what is still held (the context's state, the node, the decoder, the media handlers).

The tunes are read the first time the window opens (web/music.js `shown`: the panel lifted into an open window, or its tab
chosen on a narrow screen). Until 2026-11-09 they were read only when the panel's `hidden` attribute went, which on the desktop
happened only if the Music tab had last been chosen in another skin, so on a fresh profile the window stayed at 0:00 with no
tunes.

**Following a game.** While a game window is in front and its game plays music, Music's views show that music live (the user,
2026-11-09: "Would it be possible to make the tracker/spectral analyzer sync with what's playing in the active game?"). The
tracker shows the game's module, its pattern scrolling with the game, with the order, pattern and row in its top line, and the
spectrum the game's own sound output. A tag over the view's top right corner names the game, its tooltip the module's own title
where it has one. The game in front is the game window last made active, while it is open and not minimized (web/deskapps.js
`frontFrame`); a game window brought forward makes it the followed one. When no game window is open, or the one in front plays
no music, the views show Music's own tune as before. Music's own playback is not started, stopped or changed by any of this:
the two can play at once, and the views then show the game's.

The game's side is a site hook in the platform layer, as the player-name and settings hooks are: no game's code changes, and
native builds do not compile it. `Platform::Mixer` answers four calls the page makes with `Module.ccall` (platform/Audio.cpp,
linked through `-sEXPORTED_FUNCTIONS`): `byopop_music_song()`, the song the game plays (the loudest playing one, 0 for none);
`byopop_music_view(song)`, as JSON where it is (its kind, `module` or `stream`, and for a module its pattern, row, the pattern's
rows, its channels, its order and its title); `byopop_music_row(song, pattern, row)`, a row's cells separated by `|`; and
`byopop_music_used(song)`, the channels that play a note anywhere. A libxmp-lite module (S3M, IT, XM, MOD) is read from its
`xmp_module`, a libopenmpt one (MO3, OXM) through `ModulePlayer`'s web-only `GetView`, `GetRowText`, `GetUsedChannels` and
`GetTitle` (platform/openmpt/OpenMPTPlayer.cpp). The cells are written as Music's own decoder writes them (libxmp-lite's in
platform/ModuleText.h, which both use), so the tracker draws a game's rows as it draws its own, with the same rules for the
channels shown. A stream (Ogg, MP3, WAV) or a player that cannot tell (TipTop's MIDI synthesizer) gives the spectrum only.

The page asks only while the Music window is on the screen, once an animation frame, and reads a pattern's rows once each. The
spectrum is an `AnalyserNode` in the game's own `AudioContext`, connected as one more output of the game's last node (the
worklet or script processor of "Sound", which goes on to the speakers as before), so what is heard does not change; it is
disconnected when the game leaves the front, stops its music, or the Music window closes or is minimized. A game started with
`?silent=1`, or muted by its page, sends silence to its speakers, and so its spectrum is flat: the tracker still follows.
`ByopopMusicPanel.following()` tells a test the game followed, its view and whether its output is tapped.

**The Sounds window** (web/sounds.js, its table of the games' sounds in web/soundboards.js; the user asked for a soundboard
demo for BookWorm, 2026-11-09) sits on the listing's desktop after Music, closed at first: the games' drop-down, Stop, the
speaker and the volume, and the sounds as push buttons in group boxes. It reads the games' files with Music's read-only reader
and decodes with Music's decoders. It reads the games when it opens, and closing it stops what plays and drops the decoded
sounds and its audio context (a minimized window keeps them). `#sounds` opens it and leaves the address at the root, as
`#music` does. Under 760 pixels wide, with no desktop, it is not shown. At `?silent=1` the volume's GainNode reaches the speakers only through a
second gain of 0, so they get exact zeros (the display shows nothing of it). The graph has to reach the destination all the
same: a browser never runs a worklet whose output goes nowhere, and the tune would stand still.

### The soundboard

Begun as a demo for BookWorm (the user, 2026-11-09: a soundboard with the games' sounds, readable names or quotes), and wanted for
every game ("Soundboards are really fun to use, and I will want them for every game"): Desktop 95's Sounds window, on the listing's desktop beside Music, with its own icon and an entry in the Pop menu
(`web/sounds.js`; the other skins do not have it). It plays the sound effects of the games the player added, from those games'
own files in this browser, read with the music player's read-only reader, so the site ships no sound and sends nothing anywhere.
Which files a game has, in which group box, under what name and when the game plays each is in `web/soundboards.js`, by the
game's id; a game with no entry there, or none of whose files are here, is not listed. Names are plain and our own: a voice line
is its words, in quotes, which a file name gives only in part (the user hears and corrects them: BookWorm's lex_welcome says
"Welcome to BookWorm!", and bonusword is Lex's "Bonus!"), a sound the game never plays says so, and a silent file is left out. The when-lines come
from the port's own calls of each sound, never from the game's text. The window is a row of tools (a drop-down of the games once
two are listed, Stop, which silences everything playing, and Music's speaker and volume, kept as `byopop-sounds-mute` and
`byopop-sounds-volume`), the buttons in their group boxes, the only part that scrolls and reflows as the window is resized, and a
status bar with the count. A button plays on a click, Enter or Space and starts again when pressed while it plays; different
sounds overlap. Each file is decoded once, on its first play (the browser's own decoder, stb_vorbis from `openmpt.wasm` for an
Ogg file the browser refuses, and a reader of our own for a Sun `.au` file such as BookWorm's `blank.au`), and the decoded sounds
and the audio context go when the window closes. The grid is one tab stop whose arrows walk it across the group boxes, each box
is a named group, and at `?silent=1` a final gain of 0 keeps the speakers silent, as in the music player.

### Music levels

The user found Bonnie's Bookstore's music "way too quiet" (2026-11-07): FMOD 3 mixes an XM at a sample pre-amp of 256 and
libopenmpt at 48, so Bonnie's now plays it at 256/48 (b1807c11). Every other game's engine was then checked the same way
(2026-11-08), with `tools/musiclevel`: `musiclevel.exe` (32-bit, `tools/musiclevel/build.sh`) decodes a module with a copy of the
game's own `bass.dll` or `fmod.dll` under `build/`, silently (BASS's no-sound device and a decoding channel, FMOD 3's no-sound
output and a DSP unit after its clip-and-copy unit), and `portlevel` (a native target) renders it through `Platform::Mixer` as the
game does; both print the RMS of the first seconds. The originals' engines are BASS 1.7 (Alchemy), 1.8 (Dynomite, Pixelus, Rocket
Mania), 2.0 (Big Money), 2.2 (Bejeweled Twist, both Mahjongs), 2.3 (Bejeweled, BookWorm, Bookworm Adventures and its Volume 2,
Cosmic Bugs, Water Bugs) and FMOD 3.50 to 3.74 (Mummy Maze, Atomica, Seven Seas, Noah's Ark, Bonnie's). No game sets BASS's
amplify but the two Bookworm Adventures, so the others play at its default of 50; FMOD's at full master volume.

Over the first 30 s of each module, the original's level against the port's, in dB:

| Game | Engine | Modules: original minus port |
|---|---|---|
| Alchemy | BASS 1.7, S3M | -0.7, +0.3 |
| Dynomite | BASS 1.8, IT | +0.7, -0.7 |
| Big Money | BASS 2.0, IT | +1.1 |
| Bejeweled | BASS 2.3, S3M | -0.4, +1.2, +0.6 |
| Mummy Maze | FMOD 3.50, IT | -0.4 |
| Seven Seas | FMOD 3.61, IT | +1.9 |
| BookWorm | BASS 2.3, MO3 | +2.4 |
| Cosmic Bugs | BASS 2.3, MO3 | +1.4, +1.4 |
| Water Bugs | BASS 2.3, MO3 | -0.8 |
| Bejeweled Twist | BASS 2.2, MO3 | +0.9, -1.6 |
| Bookworm Adventures | BASS 2.3, MO3 | -4.5 at amplify 50, which its amplify of 100 makes up (`SetMusicAmplify`) |

Atomica's first 30 s from order 0 are silent in both (its first subsong is 0.9 s long). So libxmp-lite, which plays the S3M and IT
games, is within about 1 dB of every engine (Seven Seas 1.9), and nothing in the port changes. On MO3, libopenmpt against BASS
2.2 and 2.3 differs from one module to the next, both ways, by up to 2.4 dB; no setting of libopenmpt's or field of the modules
explains it, and a gain measured for each module would be a value taken from runs, so it is listed as an oddity and left. The
site's music player was the real fault: it played every module through libopenmpt, which was 2 to 3 dB quieter than the games on
S3M and 8 to 11 dB quieter on most IT (Seven Seas', Dynomite's and Big Money's). It now renders each module with the game's own
renderer and settings, and measured through `openmpt.js` under Node it gives the port's level to 0.01 dB on every module above.

The levels are shared code: `platform/MusicLevels.h` (C) holds the render settings (libopenmpt's two-tap interpolation and full
separation, libxmp-lite's spline), BASS's measured amplify curve (`ByopopBassAmplifyDB`, used by `SDLMusicInterface`) and FMOD 3's
XM pre-amp, and the music player's decoder includes it. A game whose own code plays its music at another level says so in
`web/soundtracks.js` as `level` (`{bassAmplify: 100}` for Bookworm Adventures, `{fmod3XM: true}` for Bonnie's OXM), and the
player applies the same gain from the header (`bm_level_bass_amplify`, `bm_level_fmod3_xm`).

Not covered yet:

- **TipTop** keeps MIDI music inside its .dat, which would play through TinySoundFont.
- **Word Harmony** plays WAV files.
- **Ning Po and the two Mahjongs** (Ancient China, Japan) keep their music packed inside other files (Ning Po's `ningpo.pyz`,
  the Mahjongs' `Resource.dll`), which the player would have to open first.

### Importing saves from an installed game

A player who has played an original game on this computer can bring its saves, high scores, players and settings into the port, in the
browser and natively. Both read one table, `platform/saveimport.json`, which says for each game which registry values and
which files of the original are the player's save and where each goes in the port. Nothing outside the table is ever taken: not a game's
files, not an exe, not an installer value. The original's registry and folders are only read, never written.

**Where the native saves go.** A native game keeps its user folder at `%APPDATA%\byo-pop\<Game Name>` (`SDL_GetPrefPath("byo-pop", "<Game
Name>")` in each game's `main.cpp`; it was `wasm-bejeweled` until the project was renamed). There is no code that moves an old folder:
rename `%APPDATA%\wasm-bejeweled` to `%APPDATA%\byo-pop` by hand. `-userdir=<dir>` names another folder, as it always did. The user folder
holds `registry.ini` (the port's emulated registry), the saves, `mods.json` and the like. The web shell's `userdata/` folder is the same
folder (`<persist>/userdata` in the game's IndexedDB), so a path below the user folder is the same on both.

**Where an original keeps its data.** The older SexyAppFramework games write `HKEY_LOCAL_MACHINE\SOFTWARE\PopCap\<Game>`, which a 32-bit
game on 64-bit Windows sees as `HKLM\SOFTWARE\WOW6432Node\PopCap\<Game>`; BookWorm keeps its players under the subkey `Users`. A game that
was not run as administrator cannot write there, and Windows redirects the write to `HKCU\Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\PopCap\<Game>`;
the game then reads that copy in preference to the machine's key, value by value, so a merge takes the VirtualStore's value where both have
one. Bookworm Adventures (both games) and Bejeweled use the user's key (`HKCU\Software\PopCap\WinBAD`), Word Harmony
`HKCU\Software\GameHouse\WordHarmony` and Ning Po `HKCU\Software\Pyrogon\NingPo`. The files an older game writes are beside its exe (some in a
`data` or `userdata` folder below it); in `C:\Program Files` Windows keeps them in a private copy, `%LOCALAPPDATA%\VirtualStore\Program Files
(x86)\PopCap Games\<Game>` (or `Program Files\...`), which the game reads in preference to the install folder; a Steam install is writable,
so its saves stay in the install folder. A game that sets an app data folder (Bookworm Adventures and Volume 2, `WinBAD`; Bejeweled and
Alchemy, which make the folder) uses `C:\ProgramData\PopCap Games\<name>`. Mahjong Escape: Ancient Japan (JollyBear's engine) keeps its
settings in `HKCU\Software\JollyBear\Mahjong Escape Ancient Japan` and its players and saved games in
`%LOCALAPPDATA%\JollyBear\Mahjong Escape Ancient Japan`, which the native import does not search yet: the page's note asks for that folder.

**The table.** `platform/saveimport.json` is plain JSON with one entry for each game id (the id of its `data-game` attribute on the listing
page and the string its `main.cpp` passes to the helper). It is the only place that says what is imported; CMake turns the same file into
`saveimport-table.js` for the listing page (`byopop_web_index()`) and into a header that `framework/CMakeLists.txt` compiles into every
native game (`platform/SaveImport.cpp`), so the web and native imports cannot disagree. `tools/saveimporttest.js` checks it (every game, no
path that leaves the user folder, no installer value among the values taken). An entry has:

- `folder`, `steam`, `appdata`: the install folder's usual name (for the page's instructions), the Steam folder's name where the game is sold there, and the `C:\ProgramData\PopCap Games\` folder name where the game uses one. `note` is a line for the page.
- `registry`: `key` (the port's `mRegKey`, so the port's registry key is `SOFTWARE\<key>`), `hives` (where the original may keep it, in the order of preference: `vs`, `hklm` and `hkcu`, above), `sets` (`base`: the framework's own options, `MusicVolume`, `SfxVolume`, `Muted`, `ScreenMode`, `CustomCursors`, `WaitForVSync`, `PreferredX` and `PreferredY`; `app`: `TimesPlayed` and `TimesExecuted`) and `values`, each `Name=type` with the type the port reads, `sz` (text), `dword` or `hex` (binary data). A name may have `*` (any characters inside one part) or `#` (digits) and may reach into subkeys with a backslash (`Users\*\Alias`). A value the table does not list is not taken; `skipValues` (and a game's own `skip`) say why a value that looks like a save is left out: the installer's values (`InstallPath`, `ReferId`, `DownloadId`, `RegisterLink`, `DontUpdate`, `NoPopLink`, `Variation` and the like), the original's running flag `InProgress`, its update-check time and hardware test, `TotalBooks` (worked out again at each start).
- `files`: `from` (a path inside the folder that holds the saves, with `/`; `*` and `#` as above) and `to` (the path in the user folder; the captures of `from` fill the `*` and `#` of `to` in order), and what the file is. `skipFiles` are files that are found and left alone, with the reason (a layout the port does not read).

The per-game table (generated from the file; the registry places are listed in the order the game would read them):

| Game | Registry key (places, in the order the game would read them) and values | Files, in the original's folder, and what they become in the user folder | Left alone, with a reason in the table |
| --- | --- | --- | --- |
| Bejeweled Deluxe (`bejeweled`) | `PopCap\Bejeweled` (user, then VirtualStore, then machine): the framework's options and times played; `LastScoreName` | `userdata/savegame.dat` as `savegame.dat`, `userdata/hiscores.dat` as `hiscores.dat` |  |
| Big Money Deluxe (`bigmoney`) | `PopCap\BigMoney` (VirtualStore, then machine, then user): the framework's options and times played; `LastScoreName`, `SkillLevel`, `GameType`, `DontShowHelp*` | `savegame.dat`, `hiscores.dat` |  |
| BookWorm Deluxe (`bookworm`) | `PopCap\BookWorm` (VirtualStore, then machine, then user): the framework's options and times played; `Users\User`, `Users\*\Alias`, `Users\*\ShowHelp`, `Users\*\ShowHints`, `Users\*\ShowDefs`, `Users\*\TimesPlayed`, `Scores\Classic`, `Scores\Action`, `Scores\Best`, `Scores\Longest` | `players/*/classic.dat`, `players/*/action.dat` | `players/*/profile.dat`, `players/*/scores.dat` |
| Noah's Ark (`noahsark`) | `PopCap\NoahsArk` (VirtualStore, then machine, then user): the framework's options and times played; `LastScoreName`, `ShowHints`, `ShowTutorial` | `hiscores.dat` | `data/savegame.dat` |
| Seven Seas (`sevenseas`) | `PopCap\SevenSeas` (VirtualStore, then machine, then user): the framework's options and times played; `LastScoreName`, `BonusNag`, `WarnBlueNag`, `WarnGhostNag`, `WarnSerpNag`, `WarnWhirlNag` | `data/savegame.dat` as `savegame.dat`, `hiscores.dat` |  |
| Atomica (`atomica`) | `PopCap\Atomica` (VirtualStore, then machine, then user): the framework's options and times played; `GameHints`, `ShowSymbols`, `ShowTips` | `HS.CFG`, `SAVEGAME.DAT` |  |
| Mummy Maze Deluxe (`mummymaze`) | `PopCap\MummyMaze` (VirtualStore, then machine, then user): the framework's options and times played; `LastScoreName`, `Speed`, `Name`, `Clock`, `Ankh`, `TwoMummyNag`, `GateNag`, `TrapNag`, `RedNag`, `ScorpNag`, `DarkNag`, `AnkhNag` | `hiscores.dat`, `data/saves/classic.sav` as `classic.sav`, `data/saves/adv#.sav` as `adv#.sav` |  |
| TipTop (`tiptop`) | none | `Preferences.dat`, `savegame.dat` |  |
| Bookworm Adventures Deluxe (`bookwormadv`) | `PopCap\WinBAD` (user, then VirtualStore, then machine): the framework's options and times played; `LastUser`, `AppTime` | `users/*.bwa`, `users/data/*.dat` |  |
| Ning Po Mahjong (`ningpo`) | `Pyrogon\NingPo` (user): `Layout`, `Tileset`, `Background`, `Autohint`, `Fullscreen`, `Sound Volume`, `Music Volume`, `HS:*` | none |  |
| Pixelus Deluxe (`pixelus`) | `PopCap\Pixelus` (VirtualStore, then machine, then user): the framework's options and times played; `HintsDisabled` | `users/directory.txt`, `users/#`, `users/his#` |  |
| Alchemy (`alchemy`) | `PopCap\Alchemy` (VirtualStore, then machine, then user): the framework's options and times played; `LastScoreName`, `LastInternetScoreName`, `SkillLevel`, `TimeTrial` | `userdata/hiscores.dat` as `hiscores.dat` | `userdata/savegame.dat` |
| Word Harmony (`wordharmony`) | `GameHouse\WordHarmony` (user): `FullScreen`, `MusicEnabled`, `SoundEnabled`, `HighestPuzzleLevel`, `HighestActionLevel`, `HighestSolitaireLevel`, `ShowPuzzleHelp`, `ShowActionHelp`, `ShowSolitaireHelp`, `SaverEnabled`, `DefaultScoreName`, `PuzzleSavedGame`, `ActionSavedGame`, `SolitaireSavedGame`, `scores\*\scoreName#`, `scores\*\scoreValue#` | none |  |
| Cosmic Bugs (`cosmicbugs`) | none | `cp.cfg`, `cb.dat` |  |
| Water Bugs (`waterbugs`) | none | `cp.cfg`, `cb.dat` |  |
| Rocket Mania Deluxe (`rocketmania`) | `PopCap\RocketMania` (VirtualStore, then machine, then user): the framework's options and times played; `LastScoreName`, `HintsDisabled`, `HintsDisabledFlag#` | `Players/hiscores.dat`, `Players/savea.dat`, `Players/saveb.dat`, `Players/savec.dat` |  |
| Dynomite Deluxe (`dynomite`) | `PopCap\Dynomite` (VirtualStore, then machine, then user): the framework's options and times played; `LastScoreName`, `LockCursor`, `FossilGameType`, `SkillLevel`, `GameType`, `StompedLevel` | `STOMPED.HS`, `ENDLESS.HS`, `FOSSIL.HS`, `TIMETRIAL.HS`, `SLOT#.SAV` |  |
| Bookworm Adventures Volume 2 (`bookwormadv2`) | `PopCap\WinBAD` (user, then VirtualStore, then machine): the framework's options and times played; `LastUser`, `AppTime` | `users/*.bwa`, `users/data/*.dat` |  |
| Mahjong Escape: Ancient Japan (`mahjongjapan`) | `JollyBear\Mahjong Escape Ancient Japan` (user): `SoundsOn`, `SoundsVolume`, `MusicOn`, `MusicVolume`, `FullScreen`, `WideScreen`, `HardwareAcceleration` | `list.usr`, `#.usr`, `#_#.sav`, `#_#_D.sav` (in `%LOCALAPPDATA%\JollyBear\Mahjong Escape Ancient Japan`) |  |
| Mahjong Escape: Ancient China (`mahjongchina`) | `JollyBear\Mahjong Escape` (user): `SoundsOn`, `SoundsVolume`, `MusicOn`, `MusicVolume`, `FullScreen`, `WideScreen`, `HardwareAcceleration` | `list.usr`, `#.usr`, `#_#.sav`, `#_#_D.sav` (in `%LOCALAPPDATA%\JollyBear\Mahjong Escape`) |  |
| Bonnie's Bookstore Deluxe (`bonnies`) | none | `SavedGames/General_Preferences.cfg`, `SavedGames/*.GAM`, `Data/HighScores.dat` |  |
| Bejeweled Twist (`bejeweledtwist`) | `PopCap\BejeweledTwist` (user, then VirtualStore, then machine): the framework's options and times played; `LastProfileName`, `DoViz`, `ShowHints` | `users/users.dat`, `users/scores.dat`, `users/*/profile.dat`, `users/*/savegame_*.dat` (in `C:\ProgramData\PopCap Games\BejeweledTwist`; the Steam edition's are in `C:\ProgramData\Steam\BejeweledTwist`, and its settings, under `Steam\BejeweledTwist`, are not read) |  |


The framework's options (`base`) and the times played (`app`) are taken for every game that has a `registry` entry and derives from `SexyApp`; TipTop, Cosmic
Bugs, Water Bugs and Bonnie's Bookstore (an SDL game that keeps its saves beside its exe) keep no registry values the port reads, and Ning Po and Word Harmony (engine games) have only their own values. Bookworm Adventures and
Volume 2 share the key `PopCap\WinBAD` and the folder `WinBAD`, so each imports what the other wrote there. What the table leaves out and why, game by game, is in
its `skipFiles` and `skipValues`: Noah's Ark's `data\savegame.dat` (the port's file keeps its random number generator in another form, so the original's cannot be
continued), Alchemy's `userdata\savegame.dat` (the port adds a version byte), the Steam edition of BookWorm's `players\<name>\profile.dat` and `scores.dat` (a later
build than the 2004 one that the port follows; BookWorm's import takes the 2004 edition's registry players and `classic.dat` and `action.dat`), Pixelus's
`levels\maxedscores.txt` (a game file), Bejeweled's mod saves and every file the port itself adds.

**How a value becomes the port's.** The port's registry is `userdata/registry.ini`, an INI with a section for each key (`[SOFTWARE\PopCap\BigMoney]`) and a line
for each value: `Name=sz:text` (a backslash, a line feed and a carriage return in the text are written `\\`, `\n` and `\r`), `Name=dword:123` (a signed decimal)
or `Name=hex:0a1b` (lower-case, no commas). The conversion goes by the type the table names and the type the original stored: REG_SZ and REG_EXPAND_SZ are text,
REG_DWORD is a number, REG_BINARY and REG_MULTI_SZ (BookWorm's score tables) are the same bytes as `hex:`; a value of another type than the table's, or a REG_QWORD,
is left out with that said. The games' text is Windows-1252 (a .reg file is Unicode: a character the game's text does not have makes the value left out). The page and
the native import write the file the way the port does (`SexyAppBase::GetRegistryText`: the keys in the order of its case-insensitive map, a blank line before each,
the first comment line), so the port, when it saves, changes nothing in it; a value that exists already keeps the case of its name and is replaced in place.

**On the web** (the listing page's Saves tab, `web/saveimport.js`, loaded with `saveimport-table.js`): "Import from an installed game" starts with a choice of
game. The page then says, from the table, which folders hold the game's save files (the install folder, a Steam folder, the VirtualStore copy and the ProgramData
folder, with the file paths the game keeps in them) and gives the exact `reg export` command for every place of the registry the game may use, each writing a
.reg file and each with a Copy button (and Copy all commands): `reg export "HKCU\Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\PopCap\BookWorm"
bookworm-vs.reg /y`, `reg export "HKLM\SOFTWARE\WOW6432Node\PopCap\BookWorm" bookworm-hklm.reg /y` and `reg export "HKCU\Software\PopCap\BookWorm"
bookworm-hkcu.reg /y`. They need no administrator rights and only read; a command that reports the key not found means the game did not use that place. (Checked on
the development machine for every command of the table: the ones whose keys exist write their file, and the others say the key is not found.) The player then
chooses the folder with the save files, a second folder if there is a VirtualStore copy (its files are used in preference to the first folder's), and the
.reg files. The page reads the choices in the browser (a .reg file of version 5.00, in UTF-16 with a byte order mark, or REGEDIT4; string escapes, `dword:`,
`hex:`, `hex(2)`, `hex(7)`, `hex(b)`, long hex values continued with a backslash, subkeys, `[-key]` and `"name"=-` deletions; a key of another game or another
place than the game's is reported and left out), merges the VirtualStore over the machine's key over the user's (or the other way, as the table says) value by value,
matches the values and the files against the table, and shows what would be added, replaced or left as it is, and what was left out and why, in the page. Only the
Import button writes, after the player's yes in the page (an alertdialog; Esc cancels): the page checks with Web Locks that the game is not open (it refuses to
import into a game that is, and in a browser without Web Locks), holds the game's lock while it writes, and writes in one IndexedDB transaction of the game's
database (the same path as a restore: `writeFiles` in `web/saves.js`): the changed `registry.ini` and the files in `userdata/`, with each file's own date. The
game's localStorage backup is first folded in (a file that only it had is written, one it had dropped is removed) and then removed, as a restore does, since a
left one would overwrite the import at the game's next start. Controls are labelled, the status line is a live region, an error an alert, focus moves into the
question and back; `tools/a11ycheck.js` checks the page, and the markup this script builds with `A11yCheck.checkHtml(document.documentElement.outerHTML)` in a browser.

**Natively**, each game's `main.cpp` calls `Platform::RunSaveImportIfRequested` (`platform/SaveImport.h`) right after it knows its game folder and user folder. With
`-importsaves` (or `-importsaves=<install folder>`; the default is the game folder the port found, which for a staged copy is not the original's install: give the
folder) the import runs and the game exits without starting; without the option nothing changes, not even a log line. On Windows it reads the registry with
the Win32 API, read only (`KEY_READ`): the real machine key in the 32-bit view (`KEY_WOW64_32KEY`), the VirtualStore copy and the user's key, merged as above, and the
save files from the install folder, from `C:\ProgramData\PopCap Games\<name>` for a game that has one, and from the VirtualStore copy of the install folder
(`%LOCALAPPDATA%\VirtualStore` plus the install path without its drive), the later folder winning, by the same table. It writes into the user folder; before it replaces
anything it copies the files it will replace (`registry.ini` and any saved file that differs) to `<user dir>\before-import-<date>\` (a counter after the date if
there is one already), and it writes `<user dir>\import-report.txt` (the same lines as the page's preview, and what it wrote), which it also prints. The exit code is 0
(done, or nothing to import), 1 (a file could not be written, with the copy made first so nothing is lost) or 2 (the table has no entry for the game).
Elsewhere than on Windows the option prints that it is not available and exits with 2. The web builds do not compile the call in. The helper does not check whether the
port's game is running; import with it closed.

**Tests.** `tools/saveimporttest.js` (in `tools/check.sh unit-web`) checks the table and the page's code: the .reg reader on both versions and every kind of value, the
merge of the VirtualStore over the machine's key, the conversion and the registry file (a registry file of the port reads and writes back unchanged), the file patterns and
the commands. The native import was run under `build/native/rundesk.exe` with a scratch `-userdir` against the development machine's own originals, and its
`registry.ini` for Big Money compared byte for byte with the page's from the exported .reg files of the same keys: it is the same.

### Accessibility

The pages are built to be used from the keyboard and with a screen reader (the design is `docs/MOD_MENU_DESIGN.md`, 5.6). Every page has
`<html lang="en">`, one `<h1>` and no skipped heading level. A game's page has a banner (`<header>`, the bar: the logo link to the listing,
the game's name as the `<h1>`, the status line and the buttons), one `<main>` (the stage, with the canvas and the cards), an `<aside>` for each
panel, named by its own `<h2>` (`aria-labelledby`), and the footer. The listing page has a banner with its `<h1>`, a `<main>` with the games as
a real `<ul>` and the Players, Saves and Settings tabs (a `tablist` whose `tabpanel` sections are each `aria-labelledby` its tab, with an
`<h2>` out of sight), and the footer. The copyright pages have a
banner and a `<main>`. Buttons and links are real ones: the buttons that open a file picker are `<button data-pick="...">` that click a hidden
`<input type="file">`, not labels. Choices that go together are a `<fieldset>` with a `<legend>`, every control has a visible `<label>` or an
`aria-label`, and help text is linked with `aria-describedby` (each setting's hint, the import card's note, the canvas's hint).

**The keyboard.** The first thing on a game's page to take the focus is the link "Skip to game", which shows while it has the focus and moves
it to the canvas while a game runs, else to the card that is up. The focus order then follows the page: the logo link, the bar's buttons, the
canvas (only while a game runs: before that and after it closes it is out of the tab order and `aria-hidden`), the open card's buttons, the chat, the
panels, the footer link. The focus ring is a 2 px `:focus-visible` outline in the accent colour on every control; the canvas has a white ring
inside the stage and an accent ring around it, so one of them shows over any picture. A panel's button has `aria-expanded` and `aria-controls`
(the Mods button has neither while a game runs, when it opens the game's own mod menu instead, and says so with `aria-describedby`), the Sound button
is an icon, a speaker with waves while the sound is on and crossed out while it is off, Desktop 95's own drawing (the user,
2026-11-08: "On all skins, replace the Sound: on/off button with an icon"); it is `aria-pressed` (pressed: the sound is on),
and its name and tooltip say "Sound on" or "Sound off". Opening a panel moves the focus into it. **Esc** closes it from wherever the focus is on the
page, and so does **Close**, and with the keyboard the focus goes back to the button that opened the panel (or to the canvas when the game's
menu opened it); with the mouse, closing goes back to the game, as before. Esc on the import card is its Cancel. The cards are dialogs
(`role="dialog"`, named by their heading and described by their text): a card that comes up takes the focus (the Play and Choose folder buttons, else
the card), "Already open" and the terms card are `aria-modal`, the others are not, because the bar stays usable behind them.

**How the game gets keys, and how to leave it.** SDL's Emscripten backend listens for keys on the whole window unless it is given an element, and
then it takes every key typed anywhere on the page, Tab among them, and calls `preventDefault` on it. The build sets no hint for it, so the shell
gives it one: `Module.preRun` puts `SDL_EMSCRIPTEN_KEYBOARD_ELEMENT=#canvas` in `ENV`, which SDL reads through `SDL_GetHint` before the game starts
(`JSEvents.eventHandlers` then lists keydown, keyup and keypress on the canvas, not on the window). The game therefore gets keys only while its
canvas has the focus, which a click or Tab gives it; while it has it, every key goes to the game exactly as before, Tab, Backspace, the arrows and
the function keys included, all but F11 (below). A listener on the window, added before SDL's, stops keys typed elsewhere from reaching SDL should a build ever
listen on the window again.

Tab cannot be the way out: the framework uses it (a dialog's fields, `Widget::KeyDown`), BookWorm's mod "solver" asks for a hint with it and
TipTop moves its focus with Tab and Shift+Tab. No game uses **F6**, which in browsers and Windows applications is the key that moves between
regions, so F6 takes the focus from the canvas to the next control on the page (the chat, the footer link, or the open panel) and Shift+F6 to the one
before it (the Fullscreen button), as Tab and Shift+Tab would; the game never sees either key. The chat does the same: F6 typed in it moves the focus
on, to the control after the chat, and Shift+F6 to the one before it, and nothing typed in it reaches the game, which takes keys only from its canvas. The canvas is named "<game>, game" (`role="application"`,
so that a screen reader passes its keys on) and described by "Keyboard captured by game. Use F6 to leave, Shift-F6 to re-enter." (`game.canvas_hint`),
which is also shown, in small muted type at the end of the bar and never over the game (2026-11-08), while the keyboard has put the focus there, until the first key goes to the game;
Desktop 95's desktop never shows it, as F6 leaves every window there and its About says so. Keys held when the focus leaves
the canvas are let go in the game (a keyup is sent for each), as SDL only does that when the whole window loses the focus. F2 is the game's key to
open its mod menu: typed on the bar, or with nothing focused, it is passed on to the canvas, and typed in a panel it closes the panel, as before.

**F11** is the Fullscreen button, wherever the focus is (the user, 2026-11-02: F11 "should make all games go full screen in the
browser"), and F11 again leaves full screen, as Esc does; in Desktop 95's desktop it is the browser's own instead (above). The page stops the browser's own F11, which would put the whole page in full screen,
only when it can put the stage there itself (`document.fullscreenEnabled`); then a listener on the document, which sees the key before the
canvas, keeps it from the game. Of the games only TipTop has an F11 of its own (`App::HandleKeys`, its windowed and full screen switch); in the
browser it would ask SDL for full screen outside a click, which browsers refuse, so the page's is the one that works. TipTop's Options checkbox
still goes the game's way. Both F11 and the button close an open panel first, which would otherwise stay behind the full-screen stage, and
give the canvas the focus only while the game runs: before a start or after an exit the focus stays on the card's button.

Moving the focus from the canvas to a control on the page does not count as the game losing the app's focus. SDL and the framework
(`EmscriptenFocusCallback`) take the focus from the window's `focus` and `blur` events, which an element's focus change does not raise, so
the game neither mutes nor pauses, and the framework's "mute on lost focus" and Bejeweled's pause on a lost focus stay for the window
really losing the focus (another window, another tab), which is the game's behaviour and is not changed.

**Live regions.** The status line is `role="status"`. The in-game mod menu reports the item that has the focus through
`Module.onModMenuFocus(text)` (`framework/ModMenu.cpp`), which `web/modpanel.js` writes into a polite live region in the shell (`#menu-live`, there from the
start so that a screen reader has it before the first change). The Mods panel's message line, the Game files panel's question (`role="alert"`),
the import card's note and exit card's message, the Replays panel's line and the listing page's Players, Saves and Settings lines and its sort's status ("Sorted by release date") are live too.
An error is an alert, announced at once, and the field it is about is `aria-invalid` and described by it (a rejected setting, a name the Players
section would not take); the import card's error is an alert in the card's note, read with both buttons that pick the files. The import's
progress bar is a `role="progressbar"` with a value (percent, and "12 of 117 files").

**Colour and motion.** Both themes pass WCAG AA for text (4.5:1) on every surface the page puts text on, and the focus ring against the page
(3:1): the light theme's `--ok` and `--warn` and the accents of five games (Atomica, Rocket Mania, TipTop, Dynomite, BookWorm) were darkened a little
for that. State is never colour alone: the mod tags say "On" and "Off", badges are words, errors say what happened. The page's one animation (the
import bar's slide) and every transition stop under `prefers-reduced-motion: reduce`; nothing blinks or moves by itself.

**The checker.** `tools/a11ycheck.js` reads the built pages with no browser (`node tools/a11ycheck.js build/web`, or `BYOPOP_WEB_DIR` for another
folder) and checks the language, title and viewport, the headings, the landmarks and their names, duplicate ids, every id that an `aria-*`
attribute or a `<label for>` names, the accessible name of every control, dialog and progress bar, `aria-label`s that drop the visible words,
clickable `<div>`s, tab indexes, focusable things inside `aria-hidden`, legal ARIA values, lists, fieldsets, links that open a new tab without
saying so, the skip link and the status line on a game's page, outlines taken away, motion without a reduced-motion rule, and the contrast of
the page's colour tokens in both themes (the CSS of each page, with the game's own accent). `--selftest` runs it on pages with one fault each. The
markup that scripts build (the Settings panel, the Mods panel, the Players section) is not in the files; in a browser the same checks run on the
live page: `A11yCheck.checkHtml(document.documentElement.outerHTML)`, after loading the file. `tools/check.sh unit-web` runs it on `build/web`.
Given a folder it checks only the site's own pages, and takes the list from the build, not from a list in the checker: `index.html`, every
game page that `index.html` has a tile for (the tiles are made from `BYOPOP_WEB_FINISHED` by `byopop_web_index()`, so the checker reads their
`href`s), and the copyright page each of those game pages links to. A build folder also holds test pages (`fwtest.html`, `smoke.html`), which
are not part of the site and are not checked; naming a file checks it anyway.

## The site's text

The user, 2026-11-03: "At the very least, I'd like to be able to edit a single file to rewrite the prose on the site to be more
human instead of asking you to change individual labels." Every word the site shows is in `web/strings/en.txt`, one
`key = text` a line (its header says how to write one: lines that go on, `{name}` fills, the `a | b` forms of a count, and the
`_html` keys that hold markup). The terms (`web/terms.html`) and the licences are not in it: they stay as written. Neither are
each game's own paragraphs in `games/<game>/web/page.js` (`importHtml`, `modsIntro`), which belong to the game sessions, nor
what the games themselves draw.

The build reads the file once (`byopop_web_strings_load()` in `cmake/ByopopWeb.cmake`). The pages' markup has `{{key}}`
markers, which `byopop_web_fill()` replaces with the string, HTML-escaped unless its key ends in `_html`; the game pages and
their copyright pages fill `{title}` with the game's title. The prose the build writes itself (the buy line with its date, the
listing's store links and "Coming soon", the Steam steps) comes through `byopop_web_string()`. Text that the scripts write
comes from `strings.js`, which the build writes and every page loads first in its `<head>`: the strings as `BYOPOP_STRINGS`,
then `web/i18n.js`, whose `ByopopStrings.t(key, vars)` fills `{name}` from vars and picks a count's form by `vars.n` with the
page language's plural rules (`Intl.PluralRules`; the forms in the order zero, one, two, few, many, other, leaving out those
the language lacks), and `ByopopStrings.html()` does the same for innerHTML with its vars escaped. A key that a page or the
build asks for and the file lacks stops the build; one a script asks for and the file lacks shows as the key itself, with a
note in the console. `tools/stringstest.js` (in `unit-web`) checks that every key used is in the file and that every key in
the file is used. The Node tests of the site's scripts check keys, never wording, which is the user's to rewrite:
`tools/stringsload.js` gives them `web/i18n.js` over a table in which each string reads as its key followed by what it
fills in (`size.kb 2.0`, `chat.day.today chat.day.date ...`), so a test sees which string a script picked and with what.

Translations are not made yet. The plan (the user, the same day: "could at least have LLM translations for languages that
PopCap games have been released in"): a copy of the file for each language (`de.txt` and so on, the same keys), from which
the build makes a set of pages for that language (`index.de.html`, `bejeweled.de.html`) with `<html lang>` set, so that a
page is drawn in its language from the start; a language menu in Settings, and the browser's language on a first
visit. The languages of PopCap's releases on Steam are French, German, Italian and Spanish (Gyromancer also Japanese).

## Steam on a Mac or Linux

The user, 2026-11-03: "On pages for games with a Steam release, if user agent is detected that suggests Mac OS, give
instructions on how to get the depot for the Windows release of a game", then "this should probably support Linux too". Steam
on a Mac or Linux installs only that system's version of a game (most of these games have none, a few have a Mac version),
and the ports read the Windows files, but its console's `download_depot <app> <depot>` downloads any depot of a game the account owns. A game released on
Steam, whether or not Steam still sells it, has a `BYOPOP_STEAM_<target>` entry in `cmake/ByopopStores.cmake`, its app and
the depot of its Windows files (read from Steam's own app data in the user's Steam client, never a demo's depot; each is the
app's number plus one), and
`byopop_web_shell()` puts the steps on its import card, hidden: open the console (a `steam://open/console` link, or the same
pasted into the address bar), type the command, wait for "Depot download complete", then choose the folder it names, whose
usual path the card gives (under `~/Library/Application Support/Steam/Steam.AppBundle/...` on a Mac,
`~/.local/share/Steam/steamapps/content/...` on Linux) with the file window's key for typing a path (Shift-Command-G, Ctrl+L).
The page shows them when the browser says it runs on macOS or Linux (`navigator.userAgentData.platform`, else the user agent,
leaving out an iPad asking for the desktop site, Android and ChromeOS), with only that system's folder and keys. A depot
downloaded this way is the same set of files as a Windows install's folder, so the import takes it like any Steam copy.

## The site's logo and icons

`web/brand` holds byo-pop's logo and icons. `byopop_web_game()` copies them next to the pages (`build/web`), and
both page templates link them. A web host serves them with the pages.

| File | What it is |
|---|---|
| `favicon.svg` | The mark: a bottle opener (chrome head, red handle) on a sky-blue tile. It is the tab icon in browsers that take SVG icons. |
| `favicon.ico` | The mark at 16, 32 and 48 px, for browsers without SVG icons. |
| `apple-touch-icon.png` | 180 px, opaque, for home screens (iOS rounds the corners itself). |
| `logo.svg` | The wordmark "byo-pop", whose hyphen is the bottle opener, its head toward "pop", and whose "o" in "pop" is a red crown cap (the user, 2026-11-03, instead of the mark beside the wordmark: "Use the version with head toward pop"). `web/logo.js` puts its drawing inline in the image's place on the listing page, the game pages and the copyright page, its "byo" in `--bp-logo-ink` (white on the desktop's teal), so that on mouseover (or the link's focus) the opener nudges toward "pop" and levers twice, as if to open the cap, which stays on ("animate the logo so on mouseover the bottle opener moves as if to open the cap (without actually opening it)"); not with `prefers-reduced-motion`. Where logo.svg cannot be read the image stays, before the game's mark and name, and it is a link to the listing page (`./`, relative, so it works on any host and path); the link's name, "byo-pop, all games", is the logo's alt. Leaving through the link is leaving the page like any other: the game saves on `beforeunload` and the backup is written (`SexyAppBase::PageGoingAway`), and "ask before leaving" (Site settings) asks. The copyright page shows the logo at the top. |
| `cap.svg` | A crown cap's outline seen from above, 21 rounded crimps: the mask that cuts a game's badge to a cap. |

- **Colours.** The colours are taken from PopCap's classic logo: its red (#ED1C24), sky blue and chrome. The shapes
  and lettering are byo-pop's own, and none of PopCap's are copied. The outline is the dark blue #12304A.
- **The 16 px grid.** The mark is drawn on a grid of 4 units in its 64-unit box. One unit of that grid is one pixel
  at 16 px, so the tab icon stays sharp: a white head with a dark mouth, the tooth hanging into it, and a red handle.
- **Editing.** The SVGs are the sources; edit them by hand. After a change to `favicon.svg`, run
  `python tools/brand_icons.py` to make the ICO and the PNG again. It draws them with headless Edge, so they match
  what a browser draws from the SVG.

## Testing other copies of a game: `?placeholders=1`

For testing a game's files from another source, `?placeholders=1` (`-placeholders` natively) lets the game start
when some of its files are missing: a missing image becomes a magenta and black checkerboard, a sound is silent, a
font without its descriptor uses the framework's built-in letters (`platform/Placeholders.h`). The import then
accepts any folder that holds one of the game's folders. The status line counts the missing files and the Game
files panel lists them. Scores made with the switch on are kept apart. Each game's own start check and its data
files that cannot be stood in for (a word list, a difficulty table) are the game's to handle.

A game's part, for its session when the user asks for it (not yet done in Bejeweled, Big Money or BookWorm): its
main() calls `Platform::InitPlaceholders(argc, argv)` before its game-folder check and, with the switch on, lets
`-gamedir` through with marker files missing; it overrides `SexyAppBase::GetPlaceholderSize` with its images' real
sizes (64x64 otherwise); an image the game may lack on purpose loads with `GetOptionalImage`; an override of
`GetModMessages` calls the base, which adds the missing-files banner. The page's parts are in the shared shell, so
every game has them. What wrong sizes did in testing:

- Bejeweled: the gem sheet is 780x728 (15 frames of 52 px, 104 px a colour); at 64x64 a gem crossfade drew NULL
  (the framework now grows both images first). `data/diff.txt` has no stand-in; the headless verifier's built-in
  values could serve.
- Big Money: a stack image under 6 px tall hangs the game. The coins' width / 29 sets how long a dying coin lasts,
  so the bot's game changes (score 56708 instead of 66646 with 55 files missing). The mouth frames come from
  mrmoneybags' height, the bill cels and title heights from their images; the cobrand image is optional.
- BookWorm: `tile_fire_fx1` must be at least 57 px tall and the ember and spark images square (else a modulo by
  zero). `wordlist.txt` has no stand-in, and without `bookworm.exe` the game's texts are empty (GameText).

## The performance panel

A game page shows its performance panel while the site setting "Performance panel" (`perf`, the On screen section) is on; it is
off by default (`web/perf.js`). In Desktop 95 it is a window of the desktop with its own icon. It only reads, so the game
plays the same with it on. With it off nothing of it runs: no animation frame, observer, timer or wrapped WebGL call. Its
link in the framework's frame hook returns at once, and the hook's own cost is two clock reads a frame. Each second it shows
the following.

- **Frames:** frames per second and frame time (average and longest), from requestAnimationFrame. Also the game's own time
  per frame: the framework's frame hook (`sexyapp_test_frame`, `Module.byopopTestFrame`) passes the time
  `EmscriptenMainLoop` spent on the frame's updates and draw.
- **Updates:** updates per second, and catch-up frames, those that ran more than one update. Both come from
  `byopop_update_count` read at each frame.
- **Main thread:** the game's share of the page's main thread (its frame time over the second), and the long tasks the
  browser reports, those over 50 ms (count and share). Both are labelled as shares of the one thread the page runs on,
  never as the computer's CPU use.
- **Memory:** the wasm heap's size and the bytes malloc has handed out, and the framework's images (its MemoryImage set,
  counting pixels and the blitters' caches) and loaded sounds (their samples), with their bytes. These come from
  `byopop_perf_json`, a web-only, read-only framework call, made once a second while the panel is open. Also the
  JavaScript heap where the browser gives it (`performance.memory`), and the site's storage used and quota
  (`navigator.storage.estimate`, every 10 s).
- **Graphics:** once the game has drawn, the WebGL context it draws with (GPUInterfaceGL's, the one `getContext` gives
  back) has its draw, texture-upload and buffer calls counted by wrapping them while the panel is on; turning the panel
  off puts the originals back. The panel shows draw calls per frame, uploads per second and the bytes uploaded, and the
  renderer's name. GPU time per frame comes from a timer query only where the browser really offers
  `EXT_disjoint_timer_query` (or its WebGL 2 form), with disjoint results dropped. A query runs from one frame's end to
  the next, so it times that frame's GL work. A game drawing without WebGL says so. The panel never asks for a context
  before the game has made its own, since that would make one and take the canvas from the game.
- **Sound:** the audio context's `baseLatency` and `outputLatency`, the mixed sound queued ahead of what the context has
  played, and how often the queue ran dry (`Module.byopopAudio`, "Sound" below).

## View, controls and play settings

These are site settings ("Site settings", above), each off or at the game's own behaviour by default. Like the performance
panel, they act on the page around the game: what is drawn over its picture, the input it is sent, and how often its
framework runs its updates. None of them changes a port. With all of them at their defaults nothing of them runs, except a
link in the framework's frame hook (`Module.byopopTestFrame`) that returns at once.

### View settings

`web/view.js` has three settings. **Scanlines** (`scanlines`: off, light, strong) lays a dark line over every second row of
the game's own pixels. It is an overlay above the canvas, sized from the canvas's height through a ResizeObserver, so it
follows the stage's scale and full screen. **Colour filter** (`colorFilter`: none, greyscale, sepia, warm, high contrast,
vivid) is a CSS filter on the canvas. **Run timer** (`runTimer`) is a timer in the game's top left corner. It counts real
time between drawn frames that come less than half a second apart, so a paused game, which draws nothing, or a hidden page
stops it, and it starts from 0 each time the page opens.

### Controls

The bar's **Controls** button opens the Controls panel (`web/controls.js`). It lists the game's controls in our own short
words, from the site's table `web/controldata.js`, which is keyed by game id like `web/soundtracks.js`. Each entry is either
a mouse action, `{does, how}`, or one thing done by keys, `{does, keys}`, with the keys as `KeyboardEvent.code` values.
Above each game's entry, a comment names the handlers it was read from. The table lists only keys that the game's own code
handles: not the framework's (Alt+Enter, the mod menu's F2), not debug, cheat or mod keys, and not keys that only type into
a name box. A game whose pause takes any key (Alchemy, Bejeweled) lists that as a mouse-style entry, since it has no one
key to remap.

The panel's entries with keys can be remapped. "Change key" waits for the next key, and that key then stands for the game's
key on this page: a listener on the window, in the capture phase, stops the player's key before it reaches Emscripten's
listener on the canvas, and sends the game's key in its place. The new event is a keydown or keyup, with its keypress where
the key types a character, built as a browser builds one (key, code, keyCode, which), so the game sees only its own key.
The game's own key still works, and a remapped key types the key it stands for. The map is kept per game in the
localStorage item `byopop-keys` (`{game: {keys: {from: to}, pad: {button: to}}}`). Esc gives up a change, and "Reset to
the game's keys" clears the game's map.

**Gamepad** (`gamepad`, on by default) does nothing until the browser reports a connected gamepad. From then on, the frame
hook reads the gamepad once a frame. The left stick or the d-pad moves a pointer over the game, sent as mouse moves to the
canvas. A is the left button and B the right. X, Y, LB, RB, Back and Start press the game's key entries in the table's
order, unless the panel gives an entry another button or none.

**Touch keys** (`touchKeys`: automatic, on, off; automatic shows them where the main pointer is coarse) puts a button under
the game for each key entry. It is pressed while the button is held. A touch on the game itself reaches it as SDL's
touch-to-mouse events, as before.

**Input display** (`inputDisplay`) shows the keys and mouse buttons held over the game, in its bottom left corner. Those
include the stand-ins sent for remapped keys, the gamepad and the touch keys, and not the player's remapped key itself.

### Game speed

**Game speed** (`speed`: ½×, ¾×, normal, 1¼×, 1½×, 2×) sets SexyAppFramework's own update multiplier,
`SexyAppBase::mUpdateMultiplier`, through the web-only `byopop_set_speed` (`framework/SexyAppBase.cpp`). `Process` then runs
the game's `mFrameTime` updates that many times as often in real time, and its rule of at least ten draws a second scales
with it, as in the original framework. The page sets it from the frame hook once the application exists (`byopop_speed`
answers 0 before that). It is never set on the player-only test page (`?player=1`), whose runs keep the game's own speed.
At normal speed the multiplier stays at 1 and nothing is called, so unmodded play is unchanged: the frozen baselines prove
it, natively and in the web build, since the call exists only in the web build and only the setting makes it.

It reaches every game whose loop is SexyAppBase's. It does not reach the games whose own host runs their loop, Bonnie's
Bookstore (`BonniesHost.cpp`) and Ning Po Mahjong (`NingPoApp.cpp`), so the setting's hint names them. Within a game it
reaches, a timer that the game reads from the wall clock itself (`timeGetTime`, `GetTickCount`) rather than counting
updates keeps real time. Music and sounds play at their own speed.

### Clips

**Clip recording** (`clips`) puts a Record button on the bar (`web/clips.js`). The button records the canvas's own stream
(`captureStream(60)`) and the game's sound with the browser's MediaRecorder: WebM with VP9 or VP8 and Opus where the browser
has them, MP4 otherwise. The sound comes from connecting the game's audio output node (`Module.byopopAudio.node`,
`platform/Audio.cpp`) to a MediaStream destination as well as to the speakers, for the clip's length only. The clip is saved
as a file on the player's computer (`<game>-<date>-<time>.webm`) when the button is pressed again, when the page is left, or
after ten minutes. Nothing is sent anywhere. The page's mute mutes the clip's sound too.

### Always skip tutorials

**Always skip tutorials** (`skipTutorials`, off by default) turns the game's own tutorial switch off each time it starts.
This is the game's own switch, as its options would turn it off. The port registers a function for it in its web-only file
(`Byopop::gSiteTutorials`, `framework/WebSettings.h`), as the games with their own volume engines register `gSiteVolume`.
The page calls `byopop_site_tutorials` until the game has started and the function answers 1. A game whose switch belongs
to a player answers 0 until one is loaded. A game with no registered function answers -1, and is left alone. The site never
turns a switch back on: with the setting off, the game's own choice stands. The setting is not applied on the player-only
test page.

The games with a switch of their own register it in their own sessions; until a game has registered its function, it answers
-1. Thirteen have one. Atomica, Big Money, BookWorm, Bejeweled Twist, Noah's Ark, Pixelus, Rocket Mania, TipTop, Word Harmony,
Mummy Maze and Seven Seas have a switch in their options, a "don't show again" box, or a saved flag. Bookworm Adventures 1 and
2 have only a player's "intro help" flag, and Bejeweled only its first-game instructions flag, so their sessions judge whether
these count. Nine games have no switch, and the setting leaves them as they are: Alchemy and Dynomite (whose tutorial runs only
from its own button), Cosmic Bugs and Water Bugs (whose first-level help follows the player's progress), Bonnie's Bookstore
(whose per-player flags the first board's generation also reads), and Ning Po and the two Mahjong Escapes (which have an idle
hint, but no tutorial).

### Always skip start-up logos

**Always skip start-up logos** (`skipLogos`, off by default, and like every setting it can be set for one game only) has the
games go straight past their start-up logos (the user, 2026-11-08: "Add a site-wide option to skip all start-up logos"). A
player who never sets it sees every game as it is. Start-up logos are the PopCap logo, a publisher's or developer's logo and
a "presents" splash before the loading or title screen. The loading screen's progress bar and the title screen's own
animation are not logos. With the setting on, a game goes on from each logo as a click or key on it would take it, the
original's own skip, so play afterwards is the same. Logos that held the player are already gone under the loading rule
(CLAUDE.md, "Port first, unmodified"), with `-popcaplogo` and `-splashhold` keeping them for comparisons, so they need nothing.

The logos come before anything the page could call once the game is running, so the page tells the game before it starts,
as it does for "Full screen at start": `byopop_site_skiplogos(1)` before `callMain` (`framework/WebSettings.cpp`). It is
never set on the player-only test page. A game reads `Byopop::SiteSkipLogos()` (`framework/WebSettings.h`). The call exists on
both builds, so game code needs no `#ifdef`. Natively it is true under `-skiplogos`, which `SexyAppBase::HandleCmdLineParam`
takes, so a session can test its wiring under rundesk. A game whose own host parses its command line (Bonnie's Bookstore,
Ning Po Mahjong) calls `Byopop::SetSiteSkipLogos(true)` for `-skiplogos` itself.

A game that does not read it is left as it is, logos and all. Games that read it so far: Cosmic Bugs (its PopCap logo
screens, one for each portal animation, each taken as Esc on it, straight to the main menu), Water Bugs (its one PopCap
logo screen, as Esc on it; the logo the menu can call up later still plays) and Bejeweled Twist (the loader's PopCap logo
and its trademark sign, which never fade in; the game's title stays). None of their logos draws a random number, so play
afterwards is the same either way. Each game's session names what its game skips when it is ready to merge, and the
coordinator adds it here.

## The player-only page

For sessions that test a game in the Browser pane (the user, 2026-11-07: "For agent browser testing use, build a stripped down
version that loads ONLY the player", with tools "that will help sessions with testing"). It is a game's own page with
`?player=1`, and it is there only on a development host (`ByopopTerms.devHost`: 127.0.0.1, localhost, [::1], `*.localhost`,
`*.test`, file://): elsewhere the parameter does nothing and `web/testkit.js` is never loaded, and the kit checks the host again
itself. The page shows the game alone at the top left, one canvas pixel for each game pixel whatever the screen's scale: no bar,
chat, panels, terms, skins or footer. It is silent unless `?silent=0`, each frame is the browser's or a
20 ms timer's, whichever comes first (a hidden Browser pane sends no frames, though `document.hidden` stays false), "pause when hidden" never applies, and plays are not counted. Imported files and
saves work as on the full page, in the page's own storage, unless `?assets=` asks for the test profile.

Its parameters:

| Parameter | What it does |
|---|---|
| `player=1` | the player-only page and its tools, `window.byopop.test` |
| `assets=<name>` | the game's files from the staged `assets/<name>/` (`assets=1`: the page's own id), served by `tools/webtest/serve.py` and taken through the page's own import, which fetches only the files it picks; everything goes into a test profile of its own (the page's storage and Web Lock with "-test" after their names), apart from the player's, and the record of the player's games is left alone |
| `fast=1` | the game on its virtual clock (`-fast`), on the page's own frames (below) |
| `framems=N` | with `fast=1`, the longest a frame runs the game's steps, in ms (25 by default) |
| `frames=browser` | with `fast=1`, the browser's frames instead of the page's own (to compare a run with them) |
| `hold=1` | the game waits for `byopop.test.start()`, so that a test can wipe or seed the profile first; otherwise it starts as soon as it is ready. A driver then runs once the files are in, before the game starts, and starts it itself; `?driver=move` starts it at once |
| `silent=0` | sound on (the user's speakers: only when the user says so) |
| `realfocus=1` | the browser's own blur and focus events reach the game, besides the kit's `blur()` and `focus()` (below) |
| `driver=<path>` | a session's driver script, `games/<game>/tools/<path>.js` in the repo, or a shared one, `tools/web/<name>.js` (the server serves those, and nothing else of the repo, at `/__repo/`), run once the game is ready (below) |
| `run=<id>` | the folder of `byopop.test.shot(name)`'s files under `build/webshots/` (default "default") |
| `seed=N`, `placeholders=1` | as on the full page |

`tools/webtest/serve.py` (`python tools/webtest/serve.py [--port 18780]`) serves `build/web` on the loopback address only, with
the staged files at `/__assets/<name>/` (`__list` lists them) and `POST /__shot`, which saves a shot as
`build/webshots/<run>/<name>.png`; it writes nothing else and refuses another Host or Origin. The shots go to `tools/shotdiff`
as the port's side: `python tools/shotdiff/shotdiff.py --game <g> --orig build/origrun/<run>/shots --port build/webshots/<run>
--run <id>` (`docs/SHOTDIFF.md`).

**Fast runs** (2026-11-08). A hidden Browser pane sends no animation frames and runs its timers about once a second, so with
`fast=1` the page drives the game itself: each frame the runtime asks for is a MessageChannel message, posted at once, so the
game runs as fast as the machine allows, hidden or not. Each frame stops once the game's steps took `framems` (the framework's
`byopop_set_frame_budget`, web only: `EmscriptenMainLoop` returns between two steps of the update state machine, which carries
on in the next frame), so the page's own JS, the test commands and the tool calls are served between frames. Neither changes an
update or a draw. Mummy Maze's bot, from `seed 7` and `level 1` queued in the same update, went through levels 1 and 2 by the
same positions, mazes and scores on these frames as on the browser's (`frames=browser`), in a third of the time. Pixelus's
driver went through the dialogs and the story to its third solved puzzle at about 700 updates a second, which is Pixelus's own
speed with a draw every 10 updates. A run that compares states with another must reach its starting point by commands sent in
the same update (`Promise.all` of `command()` calls), since the number of updates between two commands sent one after the other
depends on the frames.

`byopop.test` (every call that acts resolves once a frame has been drawn after it):

- **ready**, a promise that resolves at the first drawn frame with `{updates, screen}`; `progress()` before then gives the status
  line, the page's state, the frames and the updates. `start()` starts a `hold=1` page and resolves with `ready`; `loaded`
  resolves once the page could start it (its files are in), which is when a `hold=1` page runs its driver.
- **errors**, an array that collects every abort, uncaught exception and rejection, every printed line that reports an error
  (Lua's among them), every message box and every failure to fetch the staged files, each `{kind, text, update, at}`. A message
  box is answered at once and never shown: an alert is dismissed, a confirm answered with `confirmAnswer` (true by default).
- **input** at game coordinates, as the DOM's own events on the canvas, which Emscripten and SDL take on a player's path:
  `move(x, y)`, `down(x, y, button)`, `up(x, y, button)`, `click(x, y, button)` (down and up on separate frames), `drag(x0, y0,
  x1, y1, steps)`, `wheel(x, y, dy)`, `key(name, mods)`, `keyDown`, `keyUp` and `type(text)`. Keys are named as the DOM names
  them ("Enter", "Escape", "ArrowLeft", " ", "a", "F1"), with the keyCode and code a browser gives them. `keyCheck(name,
  mods, element)` sends a press at once (down, its keypress where it types a character, up) to the canvas or the element given
  and resolves with which of them the page cancelled, `{key, down, press, up}` (`press` null without one): the browser acts on
  a key only when the page did not cancel it.
- **time**: `updates()` is the framework's update count; `pause()` holds the game between two updates (it is still drawn and
  its input still arrives, but no update runs), `step(n)` runs n more and holds again, `resume()` lets it go, `wait(n)` waits for
  n updates holding nothing, and `frame()` for the next drawn frame. The hold is `byopop_hold_updates_at` in
  `framework/SexyAppBase.cpp`, web only: the update loop's test for a due update also asks whether the count has reached the
  hold, and nothing else changes. Input sent while the game is held reaches its widgets at once, as a player's would, since the
  framework hands input to them outside the updates.
- **shots**: `shot()` resolves with a PNG data URL of the canvas, read after the next frame at the game's resolution. On this
  page the game's WebGL context keeps its drawing buffer (`preserveDrawingBuffer`), so a held game, which draws nothing, can be
  shot too; the pixels drawn are the same. The kit learns of each frame from the framework's main loop
  (`Module.byopopTestFrame`, called by `EmscriptenMainLoop` with whether it drew; the runtime's own `Module.postMainLoop` is
  compiled out of these builds);
  `shot(name)` also saves it through the server.
- **focus**: `blur()` sends what a real loss of the focus sends (blur on the canvas and the window, and `document.hasFocus()`
  false), `blur({hidden: true})` also a `visibilitychange` with `document.hidden` true, and `focus()` gives it all back. The
  focus is the test's alone: the browser's own blur and focus events on the window, and on the canvas while the window has lost
  the focus, stop in the kit before SDL and the page see them, and `document.hasFocus()` leaves out the window's real focus. A
  hidden Browser pane sends a real blur, which paused TipTop's runs until their driver gave the focus back (2026-11-08); native
  `-realinput` drops the system's focus events the same way. The visibility is the test's as well: the browser's own
  `visibilitychange` stops in the kit, and `document.hidden` is true only between `blur({hidden: true})` and `focus()`. A hidden
  page takes the game's focus away at once (`SexyAppBase::EmscriptenPageHidden`), while focus coming back waits a frame in
  SDL's queue. So a real `visibilitychange` that came after `focus()`'s gain had been taken left Noah's Ark without the focus,
  its updates running and the game paused, until a second `focus()` (2026-11-08). `?realfocus=1` lets the browser's events
  through, its visibility too.
- **the log**: `log(since)` the game's printed lines from number `since` on (`{seq, kind, text, update}`, the last 20000 kept),
  `logNext()` the number the next line will have, and `waitLog(pattern, {since, timeout})`, which resolves with the first line
  from `since` on (by default the next one printed) that matches a RegExp or contains a string, and rejects after `timeout` ms
  (60000 by default, 0 for ever). `waitUpdates(n)` is `wait(n)`.
- **test commands** (the user, 2026-11-07, "Yes, add them"; `docs/TEST_COMMANDS.md`): `command(line, {timeout})` sends one
  command (state, goto <screen>, level <n>, unlock all, set <name> <value>, win, lose, seed <n>) through the framework's live
  channel, `byopop_test_command`, which queues it for the next update and the game's `HandleScriptCommand`, and resolves with the
  lines printed from the call up to its answer, the last of them: `STATE {json}`, `MOVE {json}|null`, a game's own answer of
  that shape (an upper-case word and JSON, such as Pixelus's `TARGETS {json}`), `OK <command>` or `ERROR <command>: <reason>`
  (SDL's "INFO: " taken off). A held game is stepped one update so that the command runs. `state()` is `command("state")`'s JSON.
  A build without the channel rejects at once. `botMove()` is `command("move")`'s move, or null when the bot has none;
  `playMove(m)` sends one through real DOM input in game coordinates: its `input` steps in order (`{move:[x,y]}` a hover,
  `{click:[x,y]}` with `button:"right"` if it is one, `{press:[x,y]}` and `{release:[x,y]}` for a drag, `{key:"B"}`,
  `{wait:n}` updates; `docs/TEST_COMMANDS.md`, under `move`), else an older move's `clicks` (`[x, y]` or `{x, y, button}`),
  then its `drags` (`{x0, y0, x1, y1, steps}`), then its `keys` (DOM key names), and waits its `updates` (1 by default).
- **readings**, all copies: `watch()` a port's
  watch values where it gives them (`byopop_watch_json`), null otherwise; `mixer()` the mixer's state once the shared
  session's `platform/Audio.cpp` gives it (`byopop_mixer_json`), null until then; `screen()` the game's size.
- **profile** (test profile only): `wipe()` deletes its saves and their localStorage backup (`sexy-userdata-backup:<dir>`, which the game would restore at its start; the staged files stay), `seed({path: data})` writes files into
  its user folder from a fixture save (strings, ArrayBuffers or Uint8Arrays), `list()` and `read(path)` read them; then reload
  or `start()`.

A session's test, with the server on port 18780 and the pane at
`http://127.0.0.1:18780/bejeweled.html?player=1&assets=1&fast=1&hold=1&run=title`:

```js
var t = byopop.test;
await t.profile.wipe();
await t.start();                    // resolves at the first drawn frame
await t.wait(300);                  // the logo and the loading screen
await t.click(320, 400);            // a button, at game coordinates
await t.pause();
await t.shot("title");              // build/webshots/title/title.png
t.errors.length                     // 0
```

**Drivers.** Game sessions play their real-input checks with driver scripts (the user, 2026-11-07: "Yes, use drivers"): a
script in the page reads the game's state from its log, chooses its moves and sends them as real DOM events, so that a whole
game costs a call or two. A driver is a file in its game's tools folder, named with `?driver=`; the page runs it once the game is
ready as the body of an async function of `t` (`byopop.test`), so it may `await`, and what it returns becomes
`byopop.test.result` (`{error}` if it threw; the error also goes into `errors`). `byopop.test.driver` is a promise of the same
result, which one `javascript_tool` call can await. An example, `games/<game>/tools/web/onegame.js`, for a port that logs its
board and its game's end (the lines here stand for whatever the port prints):

```js
// plays one game: reads the board the port logs, makes the first move it finds, until the game ends
var moves = 0;
await t.waitLog(/^board /, { since: 0 });          // the first board, printed when the game starts
for (;;) {
  var line = await Promise.race([t.waitLog(/^board |^game over/), t.wait(600).then(function () { return null; })]);
  if (line && /^game over/.test(line.text)) break;
  var board = t.log().filter(function (l) { return /^board /.test(l.text); }).pop().text.substring(6).split("/");
  var m = findMove(board);                          // the driver's own rule, in this file
  if (!m) break;
  await t.drag(m.x0, m.y0, m.x1, m.y1, 4);
  moves++;
}
await t.shot("end");
return { moves: moves, updates: t.updates(), errors: t.errors.length };
```

opened as `...bejeweled.html?player=1&assets=1&fast=1&driver=games/bejeweled/tools/web/onegame.js&run=onegame`, then
`await byopop.test.driver` in the pane.

**The built-in driver `?driver=move`** plays any game by its port's own bot (the user, 2026-11-07: "the scripts that play the
games should be developed in a way that their code can be reused for a mod that adds a bot"; `docs/TEST_COMMANDS.md`, "The bot
behind `move`"). It loops: `state()`, then `botMove()`, then `playMove()`, so every move is real input. It stops when the
state says `"over": true`, on `MOVE null` twice running while the game is not busy (it waits 30 updates after the first), on
an ERROR, or after `?steps=` moves (2000 by default). While the state says `busy` (or `board.busy`), as Alchemy's does while a
board clears, `MOVE null` only means "not yet": it waits 5 updates and asks again, and stops with "busy" after 20000 of them. `byopop.test.result` is then `{moves, stop, state, updates, errors}`: `stop` is "over", "limit",
"no move", "busy" or "error: <message>", and `state` the last state read. `playByMove({steps})` runs the same loop from a driver of a
session's own. It needs the build's `byopop_test_command` and a port that answers `move`; the MOVE fields are agreed with the
shared session, whose native tool plays the same moves.

**Shared drivers.** A driver in `tools/web/` serves every game: `?driver=tools/web/<name>.js` runs it bare, and a game's own
driver runs it with that game's table by `return await t.run("tools/web/<name>.js", {...})`, the table arriving as `args`.

### The page check

`tools/web/pagecheck.js` checks how a game's page behaves as a browser page, which no comparison with the original reaches
(the user, 2026-11-09: "Most of the described tests sound like things that could be automated at least somewhat"; what is
left to a player is the browser itself, Firefox first). Every game takes it in its real-input pass, through a table of its
own in `games/<game>/tools/web/pagecheck.js` (Bejeweled's and Alchemy's were the first), opened as

    <game>.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/<game>/tools/web/pagecheck.js&run=pagecheck

with `tools/webtest/serve.py` serving it (the launch configuration `pagecheck-webtest`, port 18780). It wipes the test
profile, starts the game, sends the table's `start` commands once the first screen is up and waits for play, then runs its
checks and reloads the page, as a player's F5 would. So it ends in two halves: `await byopop.test.driver` resolves with
`{reloading: true}` from the first, and once the page is back, the same call resolves with the result, `{pass, fail, info,
results}`, each line `{check, result, detail}` with `result` "pass", "FAIL" or "info". The checks:

- **Focus.** The page is hidden and shown again mid-play (`blur({hidden: true})`, 200 updates, `focus()`, 200 more), and
  the state keys that changed while it was hidden, and after, must be the ones the native build changes when its window
  loses and regains the focus. `tools/web/pagecheck_native.py` gives that answer with the same start (`--first` for each
  start command, `--title-move` for `titleMove`, which plays the bot's move while a title refuses `goto`) and prints the
  line for the table's `focus` field. Bejeweled pauses
  (`paused` and its `tick` change while hidden, its time bar does not); Alchemy goes inactive and mutes.
- **Keys.** With the game focused, the page must cancel Tab, Backspace, Space, the arrows, Page Up and Down, Home and End,
  and the table's `keys`, so that the browser moves no focus and scrolls nothing, and `'` and `/`, which open Firefox's quick
  find when the page lets their keypress through. Which browser keys it takes while the game has the focus (F5, F11, F12,
  F7, Alt, Ctrl+R, Ctrl+F, Ctrl+L) is a line of information. And nothing typed into a text field outside the game is
  cancelled: SDL listens on the canvas alone (`SDL_EMSCRIPTEN_KEYBOARD_ELEMENT`), and this proves it.
- **Resize.** The canvas is drawn at 1.5, 0.75, 2 and 1 times its size, as a resized window or full screen draws it, and
  the mouse moved to the table's `point`; the framework's `pointer` command (`docs/TEST_COMMANDS.md`) must answer that same
  game pixel, give or take one.
- **Pages.** The same game opened again in an iframe, as in a second tab, must wait for the first (the shell's Web Lock,
  "one copy at a time") and run no update, and the first must still answer. The first game is held meanwhile, since a
  second page can take a minute to load in a hidden pane and a timed game would run out.
- **Reload.** The bot first makes the table's `moves`, so that a game is in progress. Every save file the test profile
  held before the reload must come back with the same bytes, read before the game starts and again once it has; a file
  may change when the page's backup restores the version a quit would have written (the registry,
  `platform/WebPersist.cpp`), or when the table's `reloadChanges` names it. And every save the backup holds once the
  page has gone, the game in progress among them, must be in the profile once the game has started (Bejeweled's and
  Alchemy's `savegame.dat`). `afterReload` adds a check of the game's own.

The run needs `hold=1` and `assets=`, and stops with an error without them. Errors caught on the page are a line of each
half. A run takes about three minutes in a hidden pane, most of it loading the game's files twice; read
`byopop.test.result` between calls rather than awaiting the driver across the reload.

Its first runs (2026-11-09), Bejeweled in timed mode and Alchemy's strategic game, passed every line. The information line
was the same for both: while the game has the focus, the page cancels F5, F11, F12, F7, Ctrl+R, Ctrl+F and Ctrl+L (SDL
cancels the function and Ctrl keys, as the shell's note on `PAGE_SCROLL_KEYS` says) and leaves Alt alone. So F5 and
Ctrl+R reload nothing while the game has the focus, until the player clicks outside it or presses F6; whether a browser
still keeps F11 or F12 for itself is the browser's choice, which only a player's own browser shows.

## Sound

Every game's sound goes through the shared mixer (`platform/Audio.cpp`), which on the web plays through the page's own
output rather than SDL's. The user found the sound late in TipTop, Dynomite, Big Money, Bonnie's Bookstore and BookWorm
Adventures Volume 2 (2026-11-07), and still playing for a moment after the tab lost the focus. SDL's output was a
ScriptProcessor of 2048 frames: each mix waited for its turn and was then held for a whole buffer, so in the Browser pane a
sound reached the speaker 109.5 ms after the game played it (the median of 20 menu sounds in TipTop; 94 to 145 ms).

The page's output is an AudioWorklet that keeps a queue of mixed sound and asks the page for the next chunk of 256 frames
whenever the queue runs short. The page mixes the chunk at once, between the game's frames, and sends it over. The queue
is at least 2048 frames (43 ms at 48 kHz), long enough to cover the game's slower frames; when the page answers late
(loading, a heavy frame) it grows at once, up to 4096 frames, and shrinks again after three seconds of quick answers. A
sound the game starts does not wait behind the queue: its first frames are mixed straight away, alone, and added into the
sound already queued, from 640 frames (13 ms) ahead of what the context has played, and the next chunk carries on from
there. A sound now reaches the speaker 59 ms after the game plays it (the same 20 sounds: 54 to 65 ms), of which 50 ms
is the browser's own output latency (`baseLatency` and `outputLatency`). A page with no AudioWorklet (one served over
plain http from another machine) falls back to a ScriptProcessor of 1024 frames that is pulled the same way, at 84.5 ms
(76 to 97 ms).

Muting drops the queued sound, so a mute is heard at once, as natively: the game's own mute (`SexyAppBase::Mute`, which
a game calls when it loses the focus) and the page's sound button. A hidden tab gets no animation frames, so the game
would not see its focus loss until it came back: the page mutes it when the tab is hidden
(`SexyAppBase::EmscriptenPageHidden`). In the pane, a window blur muted TipTop on its next frame, 17 ms later, and a
hidden tab muted it at once.

For tests, the page's `Module` has the output's state in `Module.byopopAudio` (`kind` is `worklet` or `script`, `target`
the queue's length in frames, `gaps` how often it ran dry, `late` how many frames of a sound's start came too late to
be mixed in). `Module.audioOutput = 'script'`, set before Play, forces the ScriptProcessor. A function in
`Module.onVoiceMixed` is called with the page's clock at the play call (`performance.now()`) when a sound's first frame
is mixed, and `Module.audioChunkEar` then holds the time that frame reaches the speaker (from `getOutputTimestamp`): the
difference is the delay. `?silent=1` mixes as usual and sends silence, so the timing is the same with it.

## Memory use

The same game takes more memory in a browser than natively. The extra comes from:
- **The browser tab itself.** Any page costs some tens of MB.
- **Game files in memory.** IDBFS works by copying every stored file into memory at the start, so all
  imported game files and mods are held in RAM as the compressed files they are, before the game has
  read a single one.
- **Decoded images, twice.** The game decodes each image into 32-bit pixels in its own memory (natively
  too). Then SDL uploads it to the GPU as a WebGL texture, which the browser also counts.
- **A heap that never shrinks.** WebAssembly memory can grow but never shrink. So the peak during
  loading, with compressed bytes, decoder buffers and the decoded pixels all held at once, stays reserved
  for the whole session.

An HD 4x texture mod multiplies the pixel count by 16 for every image it replaces, and each of the
points above scales with it. Ways to reduce it, not yet done:
- read files lazily from IndexedDB instead of copying them all into memory at the start;
- free an image's decoded pixels once its texture is on the GPU, when the game never reads them again;
- decode large images one at a time, to keep the loading peak low.
