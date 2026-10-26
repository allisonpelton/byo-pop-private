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
   first, before any game files are imported or loaded.
6. **Game files.** If there are none yet, the import card asks for the install folder or a .zip. Only the
   folders the game reads (`images`, `data`, `sounds`, `music`, `properties`) are copied. The browser's
   own `DecompressionStream` unpacks a .zip.
7. **Play.** Browsers allow sound only after you click or press a key, so the page waits for "Play". Then
   it calls the game's `main()` with folder arguments (`-gamedir=/persist/game -userdir=... -modsdir=...`),
   plus `-gameseed=N` for a `?seed=N` in the address, `-placeholders` for `?placeholders=1` and `-nomods` for
   "Play without mods". From then on,
   the game runs one frame per browser animation frame, as SDL drives it.
8. **Leaving.** Quitting from the game's menu shows the exit card. Closing the tab works too: the game's
   saves are written to IndexedDB as they happen, and a small backup goes to localStorage, because a
   closing tab cannot wait for IndexedDB.

## The top bar and side panels

The bar and panels are ordinary HTML around the canvas, not part of the game. They reach the game in
four ways:

- **Function calls.** `platform/WebApi.cpp` exports a few C functions (`byopop_mods_open`,
  `byopop_mods_state`, `byopop_mods_edit`, `byopop_mods_rescan`), and `framework/ModMenu.cpp` exports
  `byopop_menu` and `byopop_restart`. The page calls them with `Module.ccall`. The game calls back
  through `Module.onModsChanged` when the mods change.
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
  card, which can go back to the Ready card.
- **Fullscreen** puts the canvas in fullscreen.
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

**The footer** is the same on every game's page: one link, which opens the game's copyright page
(`<game>-copyright.html`, next to the game) in a new tab. The markup is `web/footer.html` and its style
`web/footer.css`.

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
agreement. A test in the Browser pane ticks the box once per origin (and game), as a player would; the agreement then
stays for that origin.

### Where to buy

Under the terms, on the card and on the copyright page, one line says where the game can be bought digitally ("Where
to buy Big Money Deluxe digitally, as of October 25, 2026: Steam, EA app", each store a link that opens in a new tab)
or that it is not for sale as of a date. The stores are in `cmake/ByopopStores.cmake`, one shared file for every
game: `set(BYOPOP_BUY_<target> <YYYY-MM-DD> [<store> <url>]...)`, the date the stores were checked and each store
that sells the game. A game with a web page and no entry stops the configure with a message. Only stores that sell
the game new and digitally under the publisher's license are listed, on their own trusted sites: the publisher's
store, platforms like Steam, established game stores. Key resellers and second-hand sales are not. The file records
which stores were checked; check them again before a launch, and change the date with the result. The line is not
part of the terms' version, so a change of stores asks nobody to agree again.

## The site's logo and icons

`web/brand` holds byo-pop's logo and icons. `byopop_web_game()` copies them next to the pages (`build/web`), and
both page templates link them. A web host serves them with the pages.

| File | What it is |
|---|---|
| `favicon.svg` | The mark: a bottle opener (chrome head, red handle) on a sky-blue tile. It is the tab icon in browsers that take SVG icons. |
| `favicon.ico` | The mark at 16, 32 and 48 px, for browsers without SVG icons. |
| `apple-touch-icon.png` | 180 px, opaque, for home screens (iOS rounds the corners itself). |
| `logo.svg`, `logo-dark.svg` | The mark and the wordmark "byo-pop", whose "o" in "pop" is a red crown cap. One file for light backgrounds, one for dark. The top bar shows the one that matches the page's theme, before the game's mark and name. The copyright page shows it at the top. |

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
