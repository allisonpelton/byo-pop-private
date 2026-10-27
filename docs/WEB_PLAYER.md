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
   so the browser keeps one copy of the files, the same tree native staging gives the game.
   `tools/paktest/paktest.js` checks the reader under Node.
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
- **Settings** is the site's settings (see "Site settings", below), the same controls as the listing page's.
- **Sound** turns the page's sound off or on, below the game's own volume and mute options (see "Site settings").
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

**The game's cursor on the page.** A SexyAppFramework game hides the browser's cursor over the canvas and draws its
own image cursor into the frame; so that the rest of the page does not fall back to the browser's arrow, the shell
polls `byopop_cursor()` every 100 ms (`framework/WebCursor.cpp`, read-only: 0 for the browser's cursor, otherwise a
number that changes with the image, which `byopop_cursor_copy()` then puts in `Module.byopopCursor` as RGBA with its
hotspot). The image is drawn at the size the game's cursor has on screen, at most 128 px, and set as the CSS cursor
of everything but the canvas (`.game-cursor` on `<html>`); it is the game's pointer while the mouse is outside the
canvas, and nothing is set when the game's custom cursors are off or a game has no cursor images.

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

**Off for now; this blocks deployment.** On 2026-10-26 the user turned the terms card off on every game's page
until further notice: `BYOPOP_WEB_TERMS_GATE` in `cmake/ByopopWeb.cmake` is `OFF`, so the page treats the terms as
agreed and never shows the card, and every configure says so in a status line. The copyright page still shows the
terms. Bringing the card back blocks deployment: the pages must not go on a public host, by any tool, until the
variable is `ON` again and a page has been seen to show the card. What follows describes the card when it is on.

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

`BYOPOP_WEB_TERMS_GATE` is a plain CMake variable rather than a cache option, so changing it in the file reaches every
build folder at its next configure; `byopop_web_shell()` writes it into the page as `TERMS_GATE` (`true` or
`false`).

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

### The listing page

`build/web/index.html` lists the finished games, so that the site's root shows them instead of a directory listing: a
tile for each, with the game's `TITLE` and a badge (its initials on a colour of its own from `BYOPOP_WEB_BADGES`,
drawn by the page, never from the game's files) linking to its page; opened with `?silent=1`, it passes that on to the
links. `byopop_web_index()` in `cmake/ByopopWeb.cmake` makes it from `web/index.html` at configure time, after every
game has been added. Which games it shows is the `BYOPOP_WEB_FINISHED` list in that file: the coordinator adds a game
there at its final merge, and a name in it with no web page stops the configure (unless the game's `BUILD_<TARGET>`
option is off). Below the tiles it has the Players section (`#players`; "Player profiles", below), which `web/players.js` builds
from the records in `web/profiles.js`, and the Settings section (`#settings`; "Site settings", below), which `web/settings.js`
draws; the build copies those files next to the pages (`BYOPOP_WEB_SHARED_JS` for the ones every page loads, `BYOPOP_WEB_INDEX_JS`
for the listing page's own).

At the foot of the page is a `<footer>`, at the page's width and in its muted colours, with the disclaimer, in the words of
`web/copyright.html` and `web/terms.html` (an unofficial, non-commercial project, not affiliated with or endorsed by
Electronic Arts or PopCap Games; it serves none of the games' files; the trademarks are their owners'; the site accepts no
donations and a player with the money should buy games), and a credit line that links to the project's website, which
opens normally, with no name or email beside it.

**Which games are yours.** Under each game's name the card says, in words, whether the game's files are loaded in this
browser, as a sign that the player owns the game: "Your copy" when they are, "Needs your files" when they are not, and
"Files unknown" when the page cannot tell (below). The link's accessible name carries the same words ("Bejeweled Deluxe,
your copy"). The page knows without starting any game, from a record that the game pages keep. The record is one
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
card has a small "?" button next to "Needs your files". Played before means any of these: the game has an entry in the
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
the EA app" (a store whose name ends in "app" takes "the"), with `rel="noopener"` and an accessible name that includes the
game ("Buy Big Money Deluxe on Steam"). `byopop_web_index()` writes them at build time from the same data as the game pages'
buy note, `BYOPOP_BUY_<target>` in `cmake/ByopopStores.cmake` (a check date, then each store's name and page), so the two
cannot disagree; a game with no store shows nothing. The links are in the same list item as the game's link but not inside
it, in a `.buyrow` beside it, as a link cannot hold a link.

The links are hidden once the player has loaded files for the game, because they would otherwise suggest that the page is
used for copies that were not bought. The rule, in `show()` of `web/index.html`: they are hidden when the game's state is
"Your copy", when its record says the files were ever loaded (`ByopopGameFiles.everLoaded`: a `loaded` date, whether the files
are there now or were forgotten (`forgotten`) or lost (`missing`) since), and when the game was played here before (the "?" case:
saves or a database with entries but no record). They are shown for "Needs your files" without such a past, and for "Files
unknown". `byopop_web_index()` writes each `.buyrow` with the `hidden` attribute, so that a player who owns the game never sees
the links flash before the check has run. The game pages' terms card (off for now) and the copyright pages keep their buy
note: they are not the card that suggests where to get the game.

**The game's own icon.** Once a game's files are loaded, its card shows the game's icon in place of the initials badge
(an `<img alt="">`, since the card has the name, at the badge's size and with `image-rendering: pixelated` so that a small
icon stays crisp when enlarged), and the game's page uses it as its tab icon (a `<link rel="icon">` that replaces the
site's, set when the page starts). The icon is read out of the game's exe, which the import already stores: the page takes
the `.exe` in the game's `gameFiles` (`games/<game>/web/page.js`; of several the largest, as the game's own binary is
bigger than a helper, though no game lists more than one) and `web/exeicon.js` reads it, with no library. It parses the PE
file (`MZ`, the PE header, the section table, the resource directory), finds `RT_GROUP_ICON` (14) and its `RT_ICON` (3)
images, and takes a 32-bit image at the size nearest 48 to 64 px or, without a 32-bit image, the largest (and, if that one
cannot be decoded, the next). An image is a PNG, used as it is, or a DIB (a `BITMAPINFOHEADER` with the colour bitmap and a
1-bit transparency mask under it, at 1, 4, 8, 24 or 32 bits per pixel, the palette up to 8) which is decoded to RGBA and
drawn on a canvas; the result is a PNG data URL of at most 64 px a side and 16 KB (a larger icon is made smaller until it
fits). An exe that is wrapped or damaged, or has no icon, gives nothing, and the badge stays; nothing in it throws. A game
that does not store its exe (Bejeweled, Big Money and Alchemy import none) has no icon.

The data URL is kept in the game's entry of the ownership record (`icon`), where the listing page reads it. The game page
reads the exe in `boot()`, which follows an import and every start, when the entry has no icon yet, so the icon also
appears for files imported before this existed. It is derived from the files, so "Forget game files" deletes it with them
(and so does finding the files gone), and it is shown only while the files are there. It stays in this browser and is
never sent anywhere. `tools/exeicontest.js` checks the parser under Node against the exes in `assets/<game>/` (it prints
one line for each exe and writes no icon) and against synthetic exes made for every decoder, and feeds it damaged files.

### Site settings

Preferences that the player sets once and every game page applies when its game starts. They are edited in two places
that draw the same controls from one file, `web/settings.js` (copied next to the pages and loaded by the listing page and
by every game page): the Settings section at the foot of the listing page, and the **Settings** panel of a game's page
(the bar's Settings button, a side panel like Mods and Game files). The listing page's section is site-wide. The panel
starts with a choice, **All games** or **This game**: "All games" edits the same stored values as the listing page; "This
game" edits values that only this game has (below), and every control then starts with "Same as all games (<the value it
follows>)", which shows what the game inherits and removes the game's own value when chosen. In "All games" a setting that
this game overrides says so under its control.

Every setting can be set for one game. The item has one more field, `games`, an object from a game's id to that game's own
values (`{"games": {"bejeweled": {"mute": "on"}}}`). A game uses its own value when it has one and the site-wide value
otherwise (`ByopopSettings.get(name, gameId)`), and "game" is a value like any other (a game can be left as it is while all
the others are muted). A game's own value is stored even when it equals the default, since it then overrides the site-wide
one; removing it removes the field, and a game with none leaves no entry. Fields the code does not know are kept inside
`games` too.

They are one localStorage item, `byopop-settings`, a JSON object with one field per setting. A setting that is not in it
has its default, and the default always leaves the game as it is: **"game" keeps the original's behaviour**, so a player
who never opens the settings sees every game exactly as before, and the unmodded games stay identical to their
originals. A choice is `"game"` (the default), `"on"` or `"off"`; a volume is `"game"` or a whole number from 0 to 100;
a page feature that the originals lack is `"on"` or `"off"` and is `"off"` by default. Setting a default removes the
field. Fields that `settings.js` does not know are kept when it writes. Every read and write is in a try/catch: with
storage blocked the pages behave as if nothing was set, and the controls say a setting was not kept.

The settings, by their field in the item:

- `mute` ("game" or "on"): "on" starts the game muted. The page's mute is the platform's audio output
  (`Platform::Mixer::SetPageMute`, the exports `byopop_audio_mute` and `byopop_audio_muted` in
  `framework/WebSettings.cpp`): the mixer goes on mixing, as with `?silent=1`, and the device gets silence. It sits below
  every game's volume, mute and options, so it changes and saves none of them, and it reaches every game that plays
  through the mixer, the SexyAppFramework ones and the engine ones (TipTop, Word Harmony, Cosmic Bugs, Water Bugs) alike.
  The bar's **Sound** button turns it off and on in any case; the setting sets it when the page opens (so the button
  shows it before Play) and again when the game starts, unless the button has been used since. `?silent=1` stays silent
  whatever the settings say.
- `fullscreen` ("game" or "on"): "on" asks for full screen when the player presses Play, by the same path as the
  Fullscreen button (browsers allow it only from a click, so the restart of the game's own menu, which has no click,
  does not ask, and a browser may refuse).
- `cursors` ("game", "on" or "off"): "on" or "off" calls the game's `EnableCustomCursors(true/false)`. The export
  `byopop_site_apply(cursors, music, sfx)` does it and the volumes below; it returns 0 until `SexyAppBase::Init` has
  finished, which is after the game has read its saved options, so the setting wins over them, and the page calls it again
  every 100 ms until it returns 1. Only SexyAppFramework games have the option; for the others it does nothing and
  raises no error. "game" leaves the game's own option alone.
- `musicVolume` and `sfxVolume` ("game" or 0 to 100): the game's own music and sound volume options
  (`SetMusicVolume`, `SetSfxVolume`, through the same export and call as the cursors). TipTop, Ning Po and Word Harmony keep
  their volume in an engine of their own, which SexyAppBase's volumes do not reach. Each has a web-only file that registers a
  function (`Byopop::gSiteVolume`, `framework/WebSettings.h`), which `byopop_site_apply` calls in place of `SetMusicVolume` and
  `SetSfxVolume`, and which returns 0 until the game's own start has read its saved volumes, so that the site's value wins:
  TipTop sets its `gSoundVolume` (0 to 0xffff, as the options' slider does) and its music table (`BuildMusicVolumes`, 0 to
  0x100); Ning Po its two settings (`mSoundSetting`, `mMusicSetting`, 0 to 1, as its sliders do; the playing music takes the
  music one once `init()` is done, and only when the site set one); Word Harmony its two category volumes (0 the sound, 1 the
  music): `Game::applyVolumes` sets a category to 1 when its options box is ticked, and under a site volume to the site's level
  instead (`SiteCategoryLevel`), so the box still turns it off. A level of 100 is the game's full volume. Cosmic Bugs and Water
  Bugs have no volume option and fix the output volume at the original's 60%, so the page leaves them out (the list in
  `web/shell.html`, or `PAGE.siteVolume` in a game's `page.js`) and the controls are off in their panels.
- `profile` ("game", "fill" or "create"): what a game does with the site's current player (below, "Player profiles"). The
  setting is applied when a game starts, and the panel says so.
- `wakeLock` ("on" or "off", default off): keeps the screen awake while a game runs (`navigator.wakeLock`), taken after
  the game starts, released when it ends or the page is hidden, and taken again when the page is shown. A refusal or a
  browser without the API is fine.
- `confirmLeave` ("on" or "off", default off): asks before leaving the page while a game runs (`beforeunload`). A
  closed game's page, and the reloads after a close, do not ask.
- `pauseHidden` ("on" or "off", default off): when on and `document.hidden`, the page stops the game's main loop and
  suspends the audio context, and resumes both when the tab is shown. Off must stay as it was, since the originals keep
  running in a window you cannot see. The Browser pane can be hidden while `document.hidden` stays false, so this does not
  touch the hidden-pane tests; test it by overriding `document.hidden` and dispatching `visibilitychange`.

The settings apply when a game starts. In a game's page, the Settings panel also applies at once what can be: the volumes
and custom cursors (to the running game), the wake lock and pause when hidden; "ask before leaving" is read when the page
is left, and the Sound button acts at once. "Start muted" and "full screen at start" only apply at the next start, and
the panel says so. Choosing "game" again leaves the running game as it is.

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
  `Fit` knows the width): it applies the queued operations, makes and selects the site player under `create`, and reports.
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

**The survey.** All eighteen games were read for these facts: whether the game keeps players (create, select, rename,
delete, and where each happens), whether it asks for a name at each score, the rules of its name boxes, and the point at start
where it decides to show its new-player dialog.

| Game | Players | Name at each high score | Name rules | Where it decides to ask |
| --- | --- | --- | --- | --- |
| Bejeweled Deluxe | none | yes (record score dialog), starts with the last name | 104 px of the main font; ASCII 32 to 127; case kept | the dialog's constructor |
| Big Money Deluxe | none | yes, starts with the last name | 140 px; ASCII 32 to 127; case kept | the dialog's constructor |
| BookWorm Deluxe | up to 5, in the registry (`Users\<name>`): created by the new-player dialog (automatically when there is none, else from the player list), chosen in the list, deleted from the list (offered when there are exactly 5; the saved games stay on disk); no rename | no (the hall of fame uses the player) | letters and spaces only; 22 characters and 200 px of Verdana 12 bold; trimmed; unique ignoring case | `BWGameSelector::Update` when no player exists; the box starts with the Windows user name if it is letters and spaces |
| Noah's Ark | none | yes, starts with the last name | 120 px; ASCII 32 to 126; case kept | the dialog's constructor |
| Seven Seas | none | yes, starts with the last name | 104 px; ASCII 32 to 126; case kept | the dialog's constructor |
| Atomica | none | yes (the monitor's box), starts empty | 20 characters; any ASCII | the dialog's constructor |
| Mummy Maze Deluxe | 5 named adventure slots: created by the new-adventure dialog when a free slot is chosen, deleted from the delete dialog; no rename | yes in classic play, starts with the last name | adventure name 20 characters, any ASCII; score name 104 px | the new-adventure and score dialogs' constructors |
| TipTop | none | yes (a text field), starts empty | 10 characters; printable bytes; case kept | where the score field is made (screen 0x38) |
| Bookworm Adventures Deluxe | up to 10 (`MaxProfiles`), files `users/<name>.bwa`: created by the new-player dialog (the intro of a first game asks it, or "new" in the list), chosen and deleted in the list; no rename | no | letters, digits, space and `! @ # $ % & ( )`; 189 px of the large font; unique ignoring case | the first game's intro (`StartIntro` with no profile) |
| Ning Po Mahjong | none | yes, starts with the account's name | 12 characters; the engine's text box | `NingPo::enterName` |
| Pixelus Deluxe | any number, in `users/`: created by the new-player dialog (automatically when there is none, else from "Who are you?"), chosen, renamed and deleted in "Who are you?" | no | letters, digits and spaces; 28 characters and 150 px of Garamond 14; double and edge spaces dropped; unique ignoring case | `PixelusApp::PlayClicked`, after the loading screen |
| Alchemy | none | yes, starts with the last name | 104 px; any ASCII | the dialog's constructor |
| Word Harmony | none | yes, starts with the saved default name | 16 characters; keys 0x20 to 0x87 | `HighScoreEntryLayout::startEntry` |
| Cosmic Bugs | 10 slots in `cb.dat` (the first holds the account's name, capitalised, on a first run): chosen, renamed and deleted from the menu; a free slot asks for a name | no (the records use the player's name) | the engine's text input: 12 characters in the menu | `LoadProfiles`, when the game starts |
| Water Bugs | as Cosmic Bugs | no | as Cosmic Bugs | `LoadProfiles` |
| Rocket Mania Deluxe | none | yes, starts with the last name | 150 px; any ASCII | the dialog's constructor |
| Dynomite Deluxe | none | yes, starts with the last name | 20 characters and 115 px | the dialog's constructor (a second box names a saved game: it is not a player and is left alone) |
| Bookworm Adventures Volume 2 | as Bookworm Adventures | no | as Bookworm Adventures | as Bookworm Adventures |

## The site's logo and icons

`web/brand` holds byo-pop's logo and icons. `byopop_web_game()` copies them next to the pages (`build/web`), and
both page templates link them. A web host serves them with the pages.

| File | What it is |
|---|---|
| `favicon.svg` | The mark: a bottle opener (chrome head, red handle) on a sky-blue tile. It is the tab icon in browsers that take SVG icons. |
| `favicon.ico` | The mark at 16, 32 and 48 px, for browsers without SVG icons. |
| `apple-touch-icon.png` | 180 px, opaque, for home screens (iOS rounds the corners itself). |
| `logo.svg`, `logo-dark.svg` | The mark and the wordmark "byo-pop", whose "o" in "pop" is a red crown cap. One file for light backgrounds, one for dark. The top bar shows the one that matches the page's theme, before the game's mark and name, and it is a link to the listing page (`./`, relative, so it works on any host and path); the link's name, "byo-pop, all games", is the light logo's alt, which a dark theme keeps in the page, out of sight, for that. Leaving through the link is leaving the page like any other: the game saves on `beforeunload` and the backup is written (`SexyAppBase::PageGoingAway`), and "ask before leaving" (Site settings) asks. The copyright page shows the logo at the top. |

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
