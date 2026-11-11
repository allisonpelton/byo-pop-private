# How the web player works

What a browser loads when it opens the site, and how the pieces talk to each other. The site is a listing page drawn
as a 1995-style desktop ("Desktop 95") and a page for each game, built by `cmake/ByopopWeb.cmake` into `build/web`.
Native builds share the game code; only the outer layer differs.

## The pieces

For each game (`bejeweled` as the example):

- `bejeweled.html`, made from `web/shell.html` and the game's `web/page.*`: the page, with its bar, the game's canvas,
  the terms, import, start and exit cards, and the side panels.
- `bejeweled.js`, written by `emcc` at link time: Emscripten's loader and runtime, which the page's script calls
  `Module`. It fetches and starts the game, provides a file system and connects SDL2 to the browser.
  `web/modpanel.js` is appended to it (`--post-js`). A Release build minifies it;
  `BUILD_TYPE=Debug tools/build.sh web` writes readable JavaScript. Almost none of it is the game's own.
- `bejeweled.wasm`: the game code, the framework (`framework/`), the platform layer (`platform/`) and SDL2.
- The player's game files: the original's images, sounds, music and data, copied from the player's install folder or
  a .zip into this browser's storage. They are never uploaded and never part of the build.

The site's own scripts are in `web/`. The build copies them next to the pages: `BYOPOP_WEB_SHARED_JS` lists the ones
every page loads and `BYOPOP_WEB_INDEX_JS` the listing page's own (both in `cmake/ByopopWeb.cmake`); a new script goes
into one of them.

## What happens when a game's page opens

1. The page loads `<game>.js`, which downloads and compiles the wasm. `main()` does not run yet (`INVOKE_RUN=0`).
2. **Storage.** The page mounts IndexedDB at the game's persist folder (Emscripten's IDBFS) and copies it into memory:
   `game/` (the imported files), `userdata/` (saves, scores, settings, `mods.json`), `mods/` and `replays/`. Saves
   that only reached the localStorage backup when the last tab closed are put back first (`byopop_userdata_restore`).
3. **One copy at a time.** A Web Lock (`navigator.locks`, the game's `PAGE.lock`) lets only one tab run a game,
   because both would write the same saves. A second tab shows "Already open" and starts when the first closes.
4. **Mods.** The Mods panel finishes any removals that waited for this start, then the fixes mod is put in place ("The
   fixes mod on the site").
5. **Terms.** Until the player has agreed to the game's terms, the terms card comes first ("The terms").
6. **Game files.** If there are none, the import card asks for the install folder or a .zip. Only the folders and
   files the game's `page.js` names are copied. The rules are in `web/gameimport.js`, which the listing page uses too:
   which folder is the game's (its marker files), which files are copied, and paks. A game whose files are in a PopCap
   pak names it in its page's `paks`; the import unpacks it (`Module.byopop.pak.read` in `web/modpanel.js`, the format
   of `tools/popcap_pak.py`) and does not store the pak. A folder whose pak is already unpacked is taken too.
   `tools/paktest/paktest.js` checks the reader under Node.
7. **Play.** Browsers allow sound only after a click or key, so the page waits for "Play" (unless the setting
   `autoStart` is on). It then calls `main()` with `-gamedir=/persist/game -userdir=... -modsdir=...`, plus
   `-gameseed=N` for `?seed=N`, `-placeholders` for `?placeholders=1` and `-nomods` for "Play without mods". The game
   then runs one frame per browser animation frame.
8. **Leaving.** Quitting from the game's menu shows the exit card on a game's own page, and closes the game's window
   on the desktop. Closing the tab works too: saves are written to IndexedDB as they happen, and a small backup goes
   to localStorage, because a closing tab cannot wait for IndexedDB.

The shell keeps the game's life as one explicit state, `gameState`: waiting, starting, running, closing, closed and
stopped (an error), with the allowed moves listed in `GAME_MOVES`. The game runs only once in a page and its files
must not change under it, so "Restart game", a new import and "Forget game files" close it first (saving the game in
progress) and go on in a reloaded page.

## The bar and the panels

The bar and panels are ordinary HTML around the canvas. They reach the game in four ways:

- **Function calls.** `platform/WebApi.cpp` exports `byopop_mods_open`, `byopop_mods_state`, `byopop_mods_edit` and
  `byopop_mods_rescan`; `framework/ModMenu.cpp` exports `byopop_menu`, `byopop_restart` and `byopop_quit`. The page calls
  them with `Module.ccall`, and the game calls back through `Module.onModsChanged`. `byopop_quit` is for the Close of a
  game's window: it calls the app's `QuitAsGame` hook (`SexyAppBase.h`), which shuts down at once by default and which
  a game overrides to leave as its own Quit does (Bejeweled Twist plays its goodbye first). It returns 1 when a game
  took the call and 0 when none runs; a build without the export is closed with `byopop_restart(1)`. The shutdown
  reaches the page through `Module.onGameExit` either way.
- **The screen's shape.** The stage is 4:3 unless the game calls `Module.onGameScreen(width, height)` before it makes
  its window (BookWorm's widescreen mod gives 854 x 480). SDL sizes the canvas to the stage.
- **Files.** Adding a mod writes it into `/persist/mods` and syncs to IndexedDB; the game reads it at its next start.
- **Reloads.** "Restart game" in the F2 menu reloads the page; a mark in localStorage starts the new page at once.

The panels:

- **Mods** (`web/modpanel.js`) adds mods from a folder or .zip and removes them. A folder may hold many mods: each
  folder in it with a `mod.json` or one of the game's folders is added as its own mod. Turning mods on and off, their
  order and their settings are in the game's own F2 menu (`docs/MOD_MENU.md`). The panel offers "Play without mods"
  when a mod stopped the last start. The button shows only once the game's files are here, and never for a game in
  `BYOPOP_WEB_NO_MODS` (`cmake/ByopopWeb.cmake`, which sets the page's `MODS_SUPPORTED`); a port leaves that list
  when it starts loading mods.
- **Game files** imports the game again or forgets the copied files. Saves and mods stay.
- **Controls**, **Record** and **Settings**: see "View, controls and play settings" and "Site settings".
- **Sound** turns the page's sound off or on, below the game's own volume options.
- **Fullscreen** puts the game in full screen ("Accessibility" for F11).
- A game can add panels of its own (Big Money's **Replays**).

## The HTML shell

`web/shell.html` is every game's page template. What differs between games comes from the game's `web/` folder:

- `games/<game>/web/page.js` (required) holds `PAGE`: the game's name, its storage folder and lock, the folders and
  files copied from its install, the marker files that make a folder its install, the import card's and Mods panel's
  texts, whether it takes `?seed=` and keeps replays, and optional functions. The full list is at the top of the
  shell's script. It runs first, in the shell script's scope.
- `games/<game>/web/page.css` (optional): the game's accent colour, its brand mark and the style of its own panels.
- `games/<game>/web/page.html` (optional): the game's own panels; each `<aside class="panel" data-button="Name">`
  gets a button on the bar.

IDBFS names a game's IndexedDB database after its storage folder (Bejeweled `/persist`, Big Money `/bmpersist`,
BookWorm `/bwpersist`, and so on), so the folder and the lock are never changed once players have saves there. A new
game takes a folder and lock of its own.

At configure time `byopop_web_game(<target> TITLE "..." COPYRIGHT "..." [LICENSES <name>...])`, called from the
game's `CMakeLists.txt`, puts the game's parts into the shell at its `byopop:game-js`, `byopop:game-css` and
`byopop:game-panels` markers and its name at `@BYOPOP_TITLE@`. `byopop_web_shell()` adds the footer, the terms and
the styles and writes `build/.../<game>-shell.html`, which `emcc` gets as `--shell-file`, and
`build/web/<game>-copyright.html`.

**Names.** Every name the site shows for a game is its `TITLE` (or its row in `BYOPOP_WEB_UPCOMING`), never with
"Deluxe" (the user, 2026-11-04: "Let's not use the name Deluxe anywhere in names on the site"). A game page's tab
keeps that title while the game runs. "Deluxe" stays only where it finds the original's files: folder names in
`platform/saveimport.json` and the game import, store links, and the game's own art.

**The footer and the copyright page.** `web/footer.html` and `web/footer.css` are the page's last row: the game's
copyright notice in one sentence and a "Full terms" link to `<game>-copyright.html`. That page, from
`web/copyright.html`, has the notice, the terms, a statement that the player is unofficial and non-commercial and
ships none of the game's files, and the licence of every library the page is built from (`web/licenses/<name>.txt`:
a title line, a blank line, the text; the PopCap Games Framework's first). Every link to the site's licence texts,
terms and copyright pages opens a new tab (`target="_blank" rel="noopener"`), so a game or the music never stops for
one; `tools/linkstest.js` (check.sh `unit-web`) fails on any that does not.

### The game's cursor on the page

A SexyAppFramework game hides the browser's cursor over the canvas and draws its own. So that the rest of the page
does not fall back to the browser's arrow, the shell polls `byopop_cursor()` every 100 ms (`framework/WebCursor.cpp`,
read-only: 0 for the browser's cursor, otherwise a number that changes with the image); `byopop_cursor_copy()` puts
the image in `Module.byopopCursor` as RGBA with its hot spot. The page draws it at its on-screen size, at most
128 px, and sets it as the CSS cursor of everything but the canvas (`.game-cursor` on `<html>`). Nothing is set when
the game's custom cursors are off or it has none.

### The site's cursor

The player can choose one game's cursors for the whole site (the user, 2026-11-03: "pull custom cursors out of the
user's uploaded games so that they can select one to use across the whole site (except for inside other games)").
The code is `web/cursors.js`; `tools/cursorstest.js` checks it.

- **Where the sets come from.** Once a game has started, its page calls `byopop_cursor_set_copy()` at each poll until
  the game has set its pointer. That copies every kind from `CURSOR_POINTER` to `CURSOR_WAIT` to
  `Module.byopopCursorSet`, and the page's `takeCursorSet` keeps the six kinds the site shows. So a game's cursors
  exist only once it has been started in this browser, and forgetting its files forgets them.
- **Storage.** `byopop-cursors` in localStorage holds one set per game: PNG data URLs cropped to the visible pixels
  (at most 128 px), each with its hot spot and a hash. The choice is `byopop-site-cursor`.
- **One entry for sets that look alike.** `setsAlike` compares the cropped pictures laid on each other by their hot
  spots, with a small tolerance, so games that share PopCap's usual cursors make one entry named with all their
  titles. The text cursor is not compared.
- **Where each kind shows** (`.site-cursor` on `<html>` and one style element): the pointer everywhere, the hand on
  links and usable controls, the text cursor in text fields, the dragging cursor on something being dragged, the
  circle-slash on a disabled control and the wait cursor while the site is busy (`ByopopCursors.busy`). A missing
  kind falls back to the browser's. On a game's page the game's own cursor wins while it shows; where the game shows
  none, the site's cursor shows over the game too.
- **Animated cursors.** No ported game has one; Banana Bugs will (`docs/POPCAP_GAMES_SURVEY.md`). A stored kind may
  carry `frames` (at most 31 further images) and `ms` (40 ms to 2 s). Browsers animate no cursor image, so
  `web/cursors.js` sets each frame as the inline cursor of the element under the mouse on one timer, and runs no
  timer for a set without frames or while the page is hidden. Filling `frames` from a game is still to do.

### The terms

Before a game's files are imported or loaded, its page shows the game's terms with a checkbox and a Continue button
that stays disabled until the box is ticked. The terms say that the player owns a licensed copy and loads the files
from it, keeps them to themselves, plays for no commercial purpose and removes the files on no longer owning a copy.
They are a record of agreement, not a lock.

The text is `web/terms.html`, one file for every game. `byopop_web_shell()` fills in the game's `TITLE` and
`COPYRIGHT` and puts it on the terms card (`<!-- byopop:terms -->`) and the copyright page (`@BYOPOP_TERMS@`). The
agreement is kept in localStorage under `byopop-terms-<PAGE.id>`, with the first 12 hex digits of the SHA-1 of the
filled-in text as its version: each game asks on its own, and any change to the wording, name or notice asks again.
The import, the start and "Play without mods" refuse to go ahead without it.

The listing page asks too (the user, 2026-11-03: "Make it go through an agreement for each game uploaded but allow it
to be done on the front page"). Before a game is added there, its terms are shown with the same checkbox and "Don't
add this game", one game at a time; one box never covers several games. The build writes each listed game's text and
version into `terms-games.js`, and the agreement is the same `byopop-terms-<id>` entry, so the game's page does not
ask again. Forgetting a game's files asks nothing.

`web/terms.js` (`ByopopTerms`) decides when the card skips itself. Every build has the card and no build option
turns it off. A page skips it, keeping no agreement, in two cases:

- **On a development host** (`ByopopTerms.devHost`): 127.0.0.1, localhost, [::1], a name ending in `.localhost` or
  `.test`, or a `file://` page. `?terms=1` shows the card there.
- **With the site's test key**: a page opened with `#testkey=<key>` hashes the key (SHA-256) and compares it with the
  hash the build baked in. On a match it skips the card for the rest of that tab's pages (sessionStorage
  `byopop-testkey`). The key goes in the fragment because a browser never sends that to a server, and `terms.js`
  takes it out of the address bar first. `?terms=1` wins over the key; a key in the query is not read.

A page that skipped the card, and any page on a development host, counts no play and reads no counts
(`docs/PLAY_COUNTER.md`).

The test key is the first line of the git-ignored `secrets/web-testkey.txt`. `python tools/testkey.py` makes one, and
`tools/setup_worktree.sh` copies it into a new worktree. The web build bakes only its SHA-256 into `terms.js` at
configure time; a build without the file has no key. An agent testing the deployed site opens pages with
`?silent=1#testkey=<key>` and never writes the key into chat, a commit or a doc. `tools/termstest.js` checks the
rules.

### Where to buy

Under the terms, on the card and on the copyright page, one line says where the game can be bought digitally, as of a
date, or that it is not for sale. The stores are in `cmake/ByopopStores.cmake`:
`set(BYOPOP_BUY_<target> <YYYY-MM-DD> [<store> <url>]...)`, the date checked and each store. A game with a page or a
coming-soon tile and no entry stops the configure. Only stores that sell the game new and digitally under the
publisher's licence are listed, never key resellers. Check the stores again before a launch and change the date. The
line is not part of the terms' version.

Every store link on the site is a plain link that opens a new tab (`target="_blank" rel="noopener noreferrer"`),
never a script's `window.open`.

### The listing page

`build/web/index.html` is the site's root, made by `byopop_web_index()` from `web/index.html` after every game has
been added. On the desktop its parts are windows ("Desktop 95"): All games, the Library, Settings, Music and About.

The lists in `cmake/ByopopWeb.cmake` are the coordinator's, who edits them at a game's final merge:

- `BYOPOP_WEB_FINISHED`: the games with a page. A name with no web page stops the configure.
- `BYOPOP_WEB_UPCOMING`: `<target>|<title>` rows for the games still to come ("Coming soon"). A final merge moves the
  target to `BYOPOP_WEB_FINISHED`.
- `BYOPOP_WEB_BADGES`: each game's initials and colour, for the icon of a game whose files are not here.
- `BYOPOP_WEB_RELEASED`: `<target>|<date>` (YYYY, YYYY-MM or YYYY-MM-DD), the first release of the Windows edition
  the site plays, with its source in a comment.
- `BYOPOP_WEB_WIN95`: yes, no or unknown, whether the original supported Windows 95.

**The tiles.** Each finished game has a tile linking to its page, with its title, year and state. Each has
`data-game`, `data-persist` (the persist folder, read from `page.js` at configure time), `data-title`,
`data-released`, `data-win95` and, where no store sells it, `data-unsold`. Opened with `?silent=1`, the listing
passes that on.

**Coming soon.** A game in `BYOPOP_WEB_UPCOMING` has a tile that is no link: a `role="group"` named "<title>, <year>,
coming soon", dimmed by colour, not opacity, so its text keeps its contrast. Its store links always show. It has no
`data-game`, so the page's scripts leave it alone.

**Sorting.** "Sort by" is a radio group: Name (the built order), Release date (oldest first; a less exact date sorts
after exact ones), Play time and Plays (most played first, then games whose files are here, then the rest), and Most
popular when the site has a play counter (`docs/PLAY_COUNTER.md`). Sorting moves the `<li>` elements, so the tab
order is the order on screen, and a polite status line says the new order. The choice is `byopop-listing-sort`
(`name`, `release`, `time`, `plays` or `popular`).

**Games no longer sold** are hidden unless "Show games no longer sold" is ticked (`byopop-listing-unsold` = `show`)
or the game's files are here (the user, 2026-11-03: "default to hiding those games from the front page").

**Which games are yours.** A tile says "Play now", "Add your files" or "Files unknown". The page knows without
starting a game, from `byopop-game-files` in localStorage (`ByopopGameFiles` in `web/settings.js`): a JSON object
from a game's id to `{loaded, forgotten, missing, icon, small}`, dates as YYYY-MM-DD. The files count as there when
there is a `loaded` date and neither of the others. A game page writes the entry whenever its `boot()` finds the
files; "Forget game files" adds `forgotten`; a start that finds them gone adds `missing`. Entries are never removed.
With `?placeholders=1` a game page writes nothing. For a game without a usable entry the listing looks in the game's
IDBFS database, read-only: it calls `indexedDB.databases()` first so that it never creates a database, then walks
the keys with a key cursor. A browser without `databases()` shows "Files unknown". The listing checks again on
`pageshow`, on becoming visible and on a `storage` event, and fires `byopop-tiles-checked` when done.

**The "?" for a game played before.** When a game was played here but its files are gone (an entry in the record,
saves in its database, or the save backup `sexy-userdata-backup:<persist>/userdata`), the tile has a "?" button that
opens a short popover (`index.why.*` in the site's text) giving the likely causes and saying that no server keeps a
copy.

**Buy links.** A game that can be bought has a link for each store beside its tile link, in a `.buyrow`. They are
hidden once the player has ever loaded files for the game (`ByopopGameFiles.everLoaded`) or played it here, because
they would otherwise suggest the page is for copies that were not bought. The build writes a finished game's
`.buyrow` hidden, so the links never flash.

**Play time.** A played game's tile shows its plays, time and last day. The record is `byopop-play` (`ByopopPlay` in
`web/settings.js`): game id to `{plays, seconds, last}`. A play is a start from the page; the game's own restart
goes on with the same play. Time counts only while the game runs and its page is shown and focused (the user,
2026-11-02: "Only count time with the game open and active"); the page checks every second, adds at most 5 seconds a
step and writes every 30 seconds and on the way out.

**The game's icon.** A game whose files are here shows its own icon, read out of the player's exe by
`web/exeicon.js` (a PE resource parser with no library: `RT_GROUP_ICON`, PNG or DIB images) and kept in the game's
`byopop-game-files` entry as `icon` (nearest 48 to 64 px) and `small` (nearest 32 px), PNG data URLs of at most
16 KB. A game that does not store its exe sets `importIcon: true` in its `page.js`, and the import reads the icon
from the picked folder without storing the exe. Any other game shows its initials on a plain white square (the user,
2026-11-08: "bare initials for missing games"). `fitIcon` in `web/index.html` crops the icon to its visible pixels
and draws it in a 32 px box at whole pixels. The game's page uses the icon in its header and as its tab icon. It is
deleted with the files and never sent anywhere. `tools/exeicontest.js` checks the parser against the exes in
`assets/<game>/` and synthetic ones. The bottle cap (`web/brand/cap.svg`) stays the site's own sign, on the Pop
button and the setup wizard.

**Forgetting a game's files.** The bin button beside "Add games" lists the games whose files are here and asks
before forgetting one; the page asks itself, since `confirm()` is not shown everywhere the site runs. It does what
the game page's "Forget game files" does (`ByopopImport.forgetGame` in `web/gameimport.js`): the game folder's
records go, saves, mods and replays stay. It holds the game's Web Lock, so a game whose page is open is refused.

**Favourites.** Each tile has a star button (`aria-pressed`, "Favourite: <game>"). Favourites are the field
`favourites` of `byopop-settings` (`ByopopSettings.favourites`, `setFavourite`): a list of
`{id, title, tag, colour, page}`, so a game page can draw them without the listing. Each favourite has an icon on
the desktop and comes first in the Pop menu's Games. A coming-soon game's id is "soon-" and its title's letters.

**Windows 95 games only.** A setting that lists only the games whose original supported Windows 95
(`BYOPOP_WEB_WIN95`; `html.d95-win95only`, `win95Sync` in `web/skins.js`). An unknown game is hidden too, and the
count of hidden games shows as a value. A game released before the last one whose readme names Windows 95 (TipTop,
2002-12-12) counts as yes (the user, 2026-11-08).

**Tabs and addresses.** Settings and Music are the listing's tabs (ARIA tabs with a roving tabindex; the choice is
`byopop-listing-tab`). Players and Saves are sections of Settings. `#settings`, `#music` and `#settings-<section>`
open that window; the old `#players` and `#saves` land on those sections (`settingsHash` in `web/index.html`).

### Jungle Juice

Jungle Juice is the site's second page, which will hold games decompiled by other projects (the user, 2026-11-04).
For now it is a placeholder with no third-party content. Its pages live in `web/jungle/`; `byopop_web_jungle()`
writes each `*.html` into `build/web/jungle/` with its `{{key}}` markers filled from the site's text (`jungle.*`).
They reach the shared files one folder up (`../strings.js`, `../skins.css`). The listing links to it
(`.jungle-link`). `tools/stringstest.js` and `tools/a11ycheck.js` check these pages too.

### Adding games from the listing page

Many games can be added at once (`web/addgames.js`). A folder or .zip dropped on a finished game's tile is taken as
that game's, exactly as its own page's import card takes it. "Add games", the logo and a drop anywhere else on the
desktop go to the Setup window, which sorts and copies through the same queue.

**Sorting** (`ByopopImport.sort` in `web/gameimport.js`) goes by the files first and the name second. A folder is a
game's when it holds every one of the game's markers. A folder of several games ("PopCap Games") is looked into,
three levels deep at most. Where two games' markers are at the same place, the folder's name picks the one whose
title, short name or target it matches, ignoring case, punctuation and "Deluxe", "PopCap" and "Games"; when the name
cannot tell, the player is asked. On a phone, where no folder can be picked, .zip files are picked.

**Copying.** The listing runs no game, so it writes the files straight into the game's IndexedDB database in the
records IDBFS uses (`FILE_DATA`, keyed by path, `{timestamp, mode, contents}`, with a directory record for every
folder), one game at a time, while holding the game's Web Lock. A game whose page is open is refused, and so is a
browser without Web Locks (a page not on https or localhost). The write is all or nothing: the old copy is deleted
first, the files go in batches of about 32 MB, and the game's marker files go last, so a copy cut off partway is not
taken for the game. A write that fails deletes what it wrote. The record says `missing` from the moment the old copy
goes until the add ends.

The rules for what is copied are the game page's own: `byopop_web_index()` writes each listed game's `page.js` into
`import-games.js`, wrapped in a function that returns its `PAGE`. The listing loads `web/modpanel.js` with an empty
`Module` for its zip and pak readers. After an add the record is marked (`ByopopGameFiles.mark`, with the icon) and
the browser is asked to keep the site's storage. `tools/gameimporttest.js` (`unit-web`) checks the rules and the
sorting with every finished game's `page.js`.

### The Setup window

`web/setup.js` is "byo-pop Setup", a wizard over `web/addgames.js` (the user, 2026-11-09: "add an installer window
where files can be dropped in bulk"). It adds no rule of its own, and parodies an installer without naming or
drawing a real one. Its pages, with Back, Next and Cancel:

- **Your games**: a drop target and Browse. Each folder found is a row: "Ready", "Already here" or "Not a supported
  game"; a row that could be two games asks which.
- **Licence: <game>**: one page per game found whose terms are not agreed (`ByopopTerms`); Next agrees, "Don't add
  this game" skips it. On a development host there is none unless `?terms=1`.
- **Copying**: an overall bar and a row per game. Cancel lets the game being copied finish.
- **Finish**: a row per folder: Installed, Already here, Not a supported game, Skipped, or "Error <code>" (1: the
  drop could not be read; 2: the files could not be copied).

A drop that needs no choice or licence goes straight to Copying. `tools/setuptest.js` (`unit-web`) checks the order
of the pages and Finish's rows.

A game's own page shows its terms, import and Ready cards as the same wizard (`ByopopSkin.wizard`): the steps
Licence, Your folder, Copying and Finish. The cards keep their contents and rules.

### Coming soon games on a development host

A game in `BYOPOP_WEB_UPCOMING` whose page this build made gets marks on its tile (`data-local-page`,
`data-local-game`, `data-local-persist`, `data-local-lock`). On a development host `web/localsoon.js` turns such a
tile into a working link before anything reads the tiles, so the game opens like any other (the user, 2026-11-09:
"Yes, locally"). The tile keeps its Coming soon look, with a "This computer only" icon. On any other host the marks
are ignored, and `?localsoon=0` keeps the tiles shut on a development host too. `tools/localsoontest.js` (`unit-web`)
checks it.

The site has no deploy step yet. `build/web` holds the pages and wasm of every port the build made, the ones still
to come too, so whatever copies the site to a public host must leave those out or build with their `BUILD_<TARGET>`
off.

### Games this site does not play

A folder that holds none of the site's games but one it knows is named to the player, with a link to another project
that plays the game or "coming soon" (the user, 2026-11-07). The table is `BYOPOP_OTHER_GAMES` in
`cmake/ByopopOthers.cmake`: an id, a title, two marker files only that game has, and the project and its page or
neither. `byopop_web_index()` writes it to `other-games.js` and stops at a malformed row or a page that is not a
GitHub project's. A project is linked only when its front page and README say it plays the game, ships none of its
files and needs the player's own copy; only those two are read, never the code ("No references" in `CLAUDE.md`).
`tools/gameimporttest.js` sorts such folders beside the site's games.

### The site's look: rules

The rules, each from the user on 2026-11-07:

- **The game stays the game.** What happens inside the game's picture, and what its buttons do, stays as a port the
  player built themselves would have it ("I want the default to not feel modded"). The page around it may change.
- **The era, not the company.** No company history, credits panels or liner notes, and nothing that ties the site to
  today's company.
- **Period looks are affectionate parody.** The site may look fully like a 1995 desktop, drawn in CSS or SVG. It
  uses no Microsoft or InstallShield names, logos, icons or sounds, and never imitates a real system prompt.
- **The player's own fonts.** The CSS names the system fonts (`"MS Sans Serif", "Microsoft Sans Serif", Tahoma,
  "Segoe UI", sans-serif`) and ships no Microsoft font files.

### Values, not sentences

The rule for every control surface on the site (the user, 2026-11-08: "It is instantly obvious AI was used. Figure
out some way to make elements that don't look like that"):

1. **No sentences inside a control surface.** A window shows values and standard labels. What needs explaining goes
   in a tooltip, in About, or nowhere.
2. **A recognised icon, alone,** where a widely known one exists, drawn in our own pixels; its name is its tooltip
   and `aria-label`.
3. **Status is data in a display,** not a sentence. A test mode such as `?silent=1` shows a small indicator there.
4. **Views belong to the display.** Clicking it cycles them, with no check boxes.
5. **Credits and explanations go to About** and the licences page.
6. **Density and hierarchy of real software:** small type, tight spacing, no uniform cards or pill buttons.
7. **Evoke, don't copy.** No real program's artwork, logo, name or skin is copied.

In practice: every button is the grey bevelled one, and a destructive one is marked by its confirmation, not a
colour. A Settings window has one "?" in its title bar (What's This?): the next click on a control shows its tip
instead of working it, F1 on a focused control shows the same, and the text is the control's description for screen
readers. The terms keep their full text. Text a player needs in order to act stays as short wizard steps (the save
import, `web/saveimport.js`).

### Desktop 95

Desktop 95 is the site's only look, an affectionate parody of the 1995 desktop (the user, 2026-11-09: "drop the alt
skins, including the plain one"). `web/skins.js` sets `<html data-skin="d95">` in each page's `<head>` and
`web/skins.css` holds the look. It sets the pages' colour variables (`--bg`, `--panel`, `--panel-2`, `--fg`,
`--muted`, `--line`, `--accent`, `--accent-fg`, `--font-ui`, `--radius`) and its own `--sk-*` ones, and is light
whatever the browser's colour scheme: the shell sets `data-theme="light"`, and the dark rules a game's `page.css`
may still carry never apply. Only the player-only page (`?player=1`) has no skin. Stored `skin`, `ambient` and
`byopop-theme` values from the looks the site once had are dropped on reading (`GONE_KEYS` in `web/settings.js`).

**Windows.** A page is a desktop (`ByopopSkin.desk`; `html.d95-desk`): the game, the chat and every panel are windows
with the 1995 frame, dragged by their title bars (or moved 8 pixels at a time with Alt and the arrows), with a column
of icons at the left that opens each one again. A new window is added with
`ByopopSkin.desk.add(element, {id, title, ...})`. The rules:

- The desktop never scrolls. Every window is at most the room above the task bar, scrolls its own contents and is
  moved to fit when the browser's window changes size.
- Every window but a game's is resized by its edges and corners (a 5-pixel band, 12 pixels at the corners), from
  200 x 120 up, with the skin's own resize cursors. A maximized window's button is Restore (`showMaxed`).
- Places, sizes and whether a window is open or minimized are kept in the localStorage item `byopop-desk`, per page
  (the listing under "index", an embedded page under `embed:<page>`).
- Every menu (the Pop menu, menu bars, control menus, context menus, What's This?) closes on a press outside it, on
  Escape, and when its window goes or another comes to the front (`closeAllMenus`).
- The chrome (title bars, icons, the task bar, menus, buttons, labels) takes no text selection; text meant to be
  read or copied keeps it. The canvas and the page's own images start no selection and no drag.
- Under 760 pixels wide one window shows at a time, maximized, and only one game can be open.

**The game window.** Its title is the game's name alone, and the page's status line is its status bar. It has a
menu bar (the user, 2026-11-09: "don't hide everything under "Game""): Game (Game files, Controls, the game's own
windows, Close), View (Fit window and the size multiples; scanlines, the colour filter and the input display, set
for that game; Full screen; the chat), Options (Mods, Settings) and Help (terms, copyright, licences, About). Alt
and a menu's underlined letter opens it; Escape gives the keys back to the game. No menu shows Alt+F4 or Alt+Space,
which the browser keeps.

- **Size.** The game is always at a whole multiple of its size in device pixels (`sizeStage`, `embedWidth` in
  `web/shell.html`). View lists the multiples, greying those without room; the choice is kept per game in
  `byopop-d95-scale` (0 for Fit window, the largest that fits however the window is sized). With none chosen, the
  setting `gameSize` decides: `one` (1x, the default) or `fit` (the largest that fits). The window cannot be dragged
  smaller than its game (`ByopopEmbed.least`). The status bar shows the multiple as a value ("2x").
- **Loading.** The window is hidden until the game has made its window (`onGameScreen`) and drawn two frames, at
  most 20 seconds (`game-loading` and `game-unmade` on `<html>`), so it never shows at a wrong size first. Until
  then the setup wizard's cards stand on the desktop by themselves.
- **Close** closes the program: the game closes with its saves written (`closeGameThen`), then the window goes. A
  game that quits by itself closes its window the same way. Minimizing keeps the game running.
- **Full screen.** The maximize button, View's Full screen and the game's own option put the game alone in full
  screen at the largest whole multiple that fits, centred on black (`fitFullscreen`; `html.fs-frame` in a frame).
  The framework offers the game's request to the page first (`sexyapp_page_fullscreen` in
  `framework/SexyAppBase.cpp`), because SDL's own full screen would stretch the canvas. F11 is the browser's own
  there: the whole desktop fills the screen and the game stays a window.

**The task bar** has the Pop button, a button for each open window (a click brings it to the front, or minimizes
the one in front) and the tray with the clock and, on a game page, the sound switch. The Pop menu opens upward:
Games (the games whose files are here, favourites first), the Library, Settings with its sections, Music, each
window of a game page, Add Games and About. The arrows, Home, End, Enter and Escape work it. About holds the page's
footer. No entry imitates the real menu's words.

**Controls** are drawn as the 1995 desktop drew them (the user, 2026-11-08: "Rounded corners are very un-95"):
sunken white fields, raised grey buttons with a dotted focus rectangle, pixel-drawn ticks, option dots, sliders and
scroll bars, all SVG of our own in `web/skins.css`. A drop-down's list is the browser's own.

**The keyboard.** The desktop is in panes: each open window, the icons and the task bar. Tab and Shift+Tab go round
the controls of the pane with the focus and never leave it; F6 and Shift+F6 go to the next and previous pane, back
to the control that last had the focus there. A window that takes the focus comes to the front. In the icons and on
the task bar the arrows move between items. Once the game's canvas has the focus it gets every key, Tab too; F6
leaves it.

**Its text.** Byopop Sans, `web/fonts/byopop-sans.woff` (about 5 KB), which `tools/web/pixelfont.py` builds from W95FA
by Alina Sava (SIL OFL 1.1; `tools/web/w95fa.woff2`, the Latin subset of `@fontsource/win95fa` 5.3.0, the user's OK for
the download 2026-11-11) and which is under the same licence (`web/licenses/byopop-sans.txt`). W95FA's own grid is 12.5
pixels to the em and its advances are not whole pixels, so no CSS size shows it sharp; the script sets its glyphs on
a grid of 11 pixels to the em with whole-pixel advances (to the end of each glyph's ink; the digits one width) and
adds the characters the site uses that it lacks, drawn in the script (the arrows, the multiplication sign, the
ellipsis, the dashes, the middle dot, the fractions, the grave accent, the copyright and trademark signs). After a
change to the script, run it and commit the file; `--check` says whether the file is the script's. `web/skins.css` sets
every text of the desktop at 11 pixels, unsmoothed, a few large texts at 22: the font is sharp only at 11 and its
multiples, so a new part does not set a size of its own. It has one weight; bold is the same glyphs struck twice a
pixel apart and a pixel wider, which is exactly how Windows made the bold of its bitmap font.

**Colour schemes.** The setting `scheme` (Appearance, site-wide) picks one of nine schemes of our own, named after soda
flavours; "Standard" is the look the site always had (the user, 2026-11-11, of the proposal's schemes and patterns:
"Yes, 1 then 2"). `web/schemes.js` holds the table: for each scheme the desktop, the face of windows and buttons, its
shadow and its soft light, the title bar (one colour, or two as a gradient: each scheme decides) and the selection.
It sets them as variables on `<html>` (`--bg`, `--panel`, `--sk-lo`, `--sk-mid`, `--accent`, `--sk-title`, `--sk-band`),
which `web/skins.css` draws from: a new rule there uses the variables, never the standard greys or blues as literals. The
pictures skins.css holds as SVG in data addresses cannot read a variable, so for a scheme other than Standard their
rules are written again with the scheme's colours (`uriRules`). White highlights, black edges and text sizes are the
same in every scheme. `tools/schemetest.js` (in `unit-web`) checks every scheme: whole, named in the site's text, black
and muted text and links on its face, white on its selection, title bar and desktop.

**Pattern and wallpaper.** Three more settings of Appearance, site-wide, also in `web/schemes.js`: `pattern` (seven
patterns of our own, 8 by 8 pixels in two colours, as rows of bits), `wallpaper` (three tiles of our own, drawn in
translucent white and black so they suit every scheme, or "My picture") and `wallpaperFit` (centre, tile, stretch, for
the player's picture). The picture is chosen with Browse under the settings (`ByopopSchemes.chooser`), drawn into a
canvas, scaled to 1600 pixels at most and kept as a JPEG in localStorage `byopop-wallpaper`; it is never uploaded. The
layers go on `<body>` of the page that has the desktop (not a page in a window of the one page). `tools/schemetest.js`
checks the patterns' shape and that every choice is named.

### Desktop 95 in one page

The whole site is one page, the listing's desktop (`web/deskapps.js`, `ByopopApps`). A game, its copyright page and
Jungle Juice each open there as a window holding the page in a frame with `?embed=1` (the page then draws no
desktop, task bar, header or footer of its own); a licence text is fetched into a window as plain text. A link to
one of our pages opens its window; a link to another site opens a tab.

- **The address bar shows only the site's root** (the user, 2026-11-09: "What we're developing now is a SPA, and
  the url bar should never need to show any path other than root"). The open windows live in `history.state`
  (`{d95: true, open: [...], active}`): opening a window pushes an entry, bringing one forward or closing one
  replaces it, so Back and Forward close and reopen windows, and a reload opens them again.
- **Old addresses.** A page of ours opened in its own tab goes to the root with `?open=<page>` (`onePage` in
  `web/skins.js`), where the desktop opens its window and takes the parameter off again.
- **Test parameters stay.** `?silent=1` and `?assets=` stay on the root and pass to every frame. A game's own page
  opened with `?silent=1`, `?player=1` or `?assets=` stays that page, on its own desktop, as the sessions' drivers
  and tools open it.
- **Up to three games run at once,** each in its own frame with its own module and saves; a fourth brings up a
  message box. The window in front has the focus, and the other games' frames get the window blur the framework
  listens for (`EmscriptenFocusCallback`), so each behaves as a page that lost the focus.
- **The chat is one window for every game** ("The chat"). A message is tagged with the game in front
  (`ByopopApps.frontGame`).
- **Closing a game** returns to the window the setting `gamesOpen` names (All games or the Library).

### The Library

The listing's second window of the games (`web/library.js`, styles `.lib-*` in `web/skins.css`): a grid evoking a
game client of 2004, named for no real program. Each game is a row with its icon, name, status (Ready, Needs your
files, Coming soon, No longer sold), play time, last played and year. A click on a column's header sorts by it
(`byopop-library-sort`, announced and set as `aria-sort`). A double-click, Enter or the Play button opens the
selected game; the Filter box narrows the rows. A row's menu has Play, Add files, the favourite toggle, Properties
and the buy links. It is a real `role="grid"` with one tab stop; the arrows, Home, End, Page Up and Down and typed
letters move the selection. Everything comes from the listing's own tiles and `ByopopPlay`, so the two windows
agree.

### The fixes mod on the site

Every game is served with its fixes mod on (`docs/MOD_ROADMAP.md`, "The fixes mod"): one mod, `fixes`, each of whose
settings toggles one fix. It lives in `games/<game>/mods/fixes`. The web build copies it beside the page as
`<game>-fixes/` and writes its `version` and file list into the page (`FIXES` in `web/shell.html`). A game in
`BYOPOP_WEB_NO_MODS`, or without the folder, gets none.

Once the game's files are here, the page's boot puts the mod into `/persist/mods/fixes` before the game can start,
after the Mods panel's own boot. Like any new mod it starts on, with every fix on. A note in the game's user dir,
`site-fixes.json`, remembers what happened: `{"installed": "<version>"}` or `{"removed": true}`.

- On every start the page replaces the mod's files when the shipped version is new. `mods.json` and
  `mod-settings.json` are left alone, so the player's choices hold.
- A player who removes the mod in the Mods panel never gets it back: the next start finds the note saying
  "installed" and no folder, and notes it as removed.

To test with a stand-in, configure with `-DBYOPOP_WEB_TEST_FIXES=<game>` (set it back to empty afterwards): a game
with no fixes mod of its own then gets `tools/webtest/fixes`, a mod with one toggle that changes nothing.

### Site settings

Site-wide overrides of the games' own options, applied when a game starts. They are drawn from one file,
`web/settings.js`, in two places: the listing's Settings window and a game page's Settings panel.

**Storage.** One localStorage item, `byopop-settings`, a JSON object with one field per setting (`DEFS` in
`web/settings.js`). A missing field has its default, and setting a default removes the field. **The default always
leaves the game as it is** ("Game default", `"game"`), so the unmodded games stay identical to their originals. The
field `games` holds each game's own values (`{"games": {"bejeweled": {"fullscreen": "off"}}}`);
`ByopopSettings.get(name, gameId)` returns the game's value where it has one, else the site's. A game page's panel
edits either "All games" or "This game only". Unknown fields are kept on writing. Every read and write is in a
try/catch. Old volume fields (`mute`, `musicVolume`, `sfxVolume`) are migrated to the starting volume on reading;
`tools/profilestest.js` checks it.

**Sections.** Player names, Sound, Appearance, On screen, Input, Starting, Game, Page and Chat. On the desktop the
Settings window is a folder of their icons and each section opens as its own window; settings apply as they change,
with no OK or Apply. `#settings-<section>` opens one (`players`, `sound`, `look`, `overlays`, `input`, `start`,
`game`, `page`, `chat`). The icons are drawn by `tools/settingsicons.py`, which writes `web/settingsicons.js`. Each
control's hint is in `FIELDS`.

**The settings**, by field:

- `startVolume` (switch), `startMusic`, `startSound` (0 to 100, default 50): the volume every game starts at ("The
  starting volume", below).
- `fullscreen` (game, on, off): "on" asks for full screen when the player presses Play (browsers allow it only from
  a click); "off" keeps the game windowed ("Windowed", below).
- `cursors` (game, on, off): calls the game's `EnableCustomCursors` through `byopop_site_apply(cursors, music,
  sfx)`. That export returns 0 until `SexyAppBase::Init` has read the saved options, so the page calls it every
  100 ms until it returns 1 and the setting wins. Only SexyAppFramework games have the option.
- `accel3d` (game, on, off): before the start the page calls `byopop_site_accel3d` with -1, 1 or 0, and a game with
  a 3D option reads `Byopop::SiteAccel3D()` (`framework/WebSettings.h`) where it reads that option. The games that
  read it are `ACCEL3D_GAMES` in `web/settings.js`; other game pages leave the setting out. A line under it says
  what this browser offers (`accel3d()`: WebGL2, with or without the graphics card). Applies at the next start.
- `profile` (game, fill, create): "Player profiles".
- `wakeLock` (off): keeps the screen awake while a game runs (`navigator.wakeLock`).
- `confirmLeave` (off): asks before leaving a page while a game runs (`beforeunload`).
- `pauseHidden` (off): when the tab is hidden, stops the game's main loop and suspends the audio context. Off must
  stay as it is, since the originals keep running unseen. To test it, override `document.hidden` and dispatch
  `visibilitychange`; a hidden Browser pane does not set it.
- `autoStart` (on): the Ready card starts the game by itself when the browser allows sound without a click
  (`navigator.getAutoplayPolicy("audiocontext")` in Firefox, else a new AudioContext whose `resume()` settles
  within 300 ms). It does not start after a failed start, with `fullscreen` on, or with `?silent=1` (tests press
  Play themselves).
- `showChat` (on) and `chatPlace` (edge, game): "The chat".
- `pixelPerfect` (off): on a game's own page the stage takes the game's size at a whole number of device pixels
  per game pixel instead of filling the window.
- `gameSize` (one, fit), `gamesOpen` (games, library), `win95Only` (off): "Desktop 95" and "The listing page".
- `perf` (off): "The performance panel".
- `scanlines`, `colorFilter`, `inputDisplay`, `touchKeys`, `gamepad`, `clips`, `skipLogos`, `skipTutorials`,
  `speed`: "View, controls and play settings".

In a game's page the panel applies at once what can be (the starting volume, cursors, the wake lock, pause when
hidden). `fullscreen`, `accel3d` and `profile` apply at the next start, and the panel says so.

**The starting volume.** The page's mute (`Platform::Mixer::SetPageMute`; exports `byopop_audio_mute` and
`byopop_audio_muted` in `framework/WebSettings.cpp`) is the platform's audio output: the mixer goes on mixing and
the device gets silence, so it changes and saves none of a game's options. With the starting volume on, the page
holds that mute from the moment it opens until the volumes are applied (at most a minute after Play), so nothing
plays at the game's own volume first. The bar's Sound button is the same mute, separate from the setting.
`?silent=1` is silent whatever the settings say. How the levels reach a game:

- SexyAppFramework games take them through `byopop_site_apply` (`SetMusicVolume(level / 100)`, `SetSfxVolume`),
  and then save them as their own.
- TipTop, Ning Po and Word Harmony keep their volume in their own engine. Each registers a function in
  `Byopop::gSiteVolume` (`framework/WebSettings.h`), which `byopop_site_apply` calls instead.
- Cosmic Bugs and Water Bugs have no volume option, so the page sets the mixer's master gains (`SetSiteGains`, the
  export `byopop_mixer_gain`), which multiply what the game sets. The list is `MIXER_VOLUME` in `web/shell.html`, or
  `PAGE.mixerVolume`.

**Windowed.** SexyAppBase always starts the web build windowed. The games whose own option can say full screen
(Word Harmony, Cosmic Bugs, Water Bugs, TipTop, Ning Po) read `Byopop::SiteWindowed()` where they read that option;
with `fullscreen` off the page calls `byopop_site_windowed(1)` before the start. The game saves the option as if the
player had set it.

**A refused full screen.** A browser grants full screen only during a gesture. `SexyAppBase::SwitchScreenMode` asks
only when the browser would take the request (`sexyapp_fullscreen_allowed`: `navigator.userActivation.isActive`)
and otherwise stays windowed and logs it. A request can still be refused later, after Emscripten has sized the
canvas for the screen, so the switch watches it (`sexyapp_watch_fullscreen_refusal`): on `fullscreenerror` it sends
a `fullscreenchange` with no full-screen element, which Emscripten and SDL take as leaving full screen.

### The chat

The site has one chat, the same room for every game (`web/chat.js`; `docs/CHAT_DESIGN.md` for its design and a
possible server). For now it is a test: the messages are made up, what a player sends stays in this browser, and the
page makes no network request for it. A line reads `HH:MM [BW] <username> text`; the tag is the initials of the game
the message was sent from, linking to that game. The table of games is `chat-games.js`, which `byopop_web_index()`
writes from `BYOPOP_WEB_BADGES` and the titles, so the chat keeps no list of its own. `tools/chattest.js` checks it.

On the desktop it is one window, built when first opened, which comes up behind the first game unless `showChat`
is off (the user, 2026-11-09: "Let's just have a single chat window"). On a game's own page it is the
`<aside id="chat">` after the stage: beside the stage only when the stage keeps its width, else below it
(`chat-below` on the root element), so the canvas and its scaling never change for the chat. `chatPlace` puts it at
the page's edge or next to the game. Nothing moves the focus into the chat by itself, and the page cancels Home,
End, PageUp and PageDown typed on the canvas for the browser, which would scroll to the chat (the game still gets
them). Its header shows values: a TEST tag whose tooltip says the messages are made up, and a disabled Sign in
button.

### Player profiles

The player gives a name once and the games use it: for the player a game keeps, and for the name it asks for at a
high score. Nothing is sent anywhere. `tools/profilestest.js` checks the page's side.

**The site's list** is the localStorage item `byopop-profiles`, `{profiles: [{id, name}], current: id}`
(`ByopopProfiles` in `web/profiles.js`). A name is trimmed, at most 32 characters and unique whatever the case.

**The setting `profile`** has three values. `game`, the default, leaves the game as it is: every hook returns before
it does anything, with no random number drawn. `fill` pre-fills the names the game asks for with the site player's
name. `create` does that and, at the start of a game that keeps players, selects the game's player of that name,
making it if needed with the functions the game's own dialog uses, so the dialog never opens. A game without
players treats `create` as `fill`. With no site player all three behave as `game`.

**A game pulls, the page never pushes,** because a game's new-player dialog opens before any message from the page
could arrive. The game asks through `framework/WebProfiles.h` (web only; empty natively). `SiteGet(id)` is an
`EM_JS` call returning the mode, the current site player's name, all the names and the waiting operations, as JSON
from `ByopopProfiles.forGame(id)`. Names cross as UTF-8 and become Latin-1 at the border. The game's part is
`games/<game>/src/SiteProfiles.cpp`, which implements `SiteProfiles` (its names, the current one, `Find`, `Fit`,
`Create`, `Select`, and `Rename` and `Delete` where the game can), plus one `#ifdef __EMSCRIPTEN__` line at each
decision point. The framework's calls:

- `SiteStart(id, game)` where the game would decide to ask for a new player: it applies the queued operations,
  makes and selects the site player under `create`, and reports. For a SexyAppFramework game the framework calls it
  (`SexyAppBase::DoUpdateFrames` calls `Byopop::SiteLoaded` right after `LoadingThreadCompleted`; the game's file
  registers its function in `Byopop::gSiteLoaded`). The engine games call it from their own start.
- `SiteScoreNames` is the whole hook of a game that keeps no players and only asks for a name at a high score.
- `SiteChanged(id, game)` wherever the game's list changes: it reports again.
- `SiteType(id, editWidget)` right after a name box is made: the site player's name is typed into the box through
  `EditWidget::KeyChar`, one character at a time, so the box's own rules keep or drop each character as typing
  would. Games with another kind of box ask `SiteNameFor(id)`. `SiteFitTyped(...)` is the typing without a box on
  screen, for a game's `Fit`.

**What a game reports** is the item `byopop-game-profiles`, `{<game>: {kind, names, current, fits, rules, max,
canRename, canDelete, deletes, at, results}}`, written by `ByopopProfiles.report` at every start and change. `kind`
is `profiles` or `scores`; `fits` maps each site player's name to what the game's rules make of it.

**Operations** from the Players section wait in `byopop-profile-ops`, `{<game>: [{id, op, ...}]}` (`rename` with
`from` and `to`, `delete` with `name`), because a game that is not running cannot be reached. The game applies them
in `SiteStart` with the functions its dialogs call and reports each by `id` in `results` (`done` or `skipped`). A
rename to a name the game already has is skipped: two players' progress is never merged.

**The Players section** of Settings (`web/players.js`) adds, renames and deletes the site's players and picks the
one in use. For each it lists the games that have one and the name each uses, renames any number of them in one
step, and deletes a player inside the games after an in-page confirmation. It never uses `alert()`, `confirm()` or
`prompt()`.

**The games.** BookWorm, Pixelus, Mummy Maze (its named adventures), both Bookworm Adventures, Cosmic Bugs and
Water Bugs keep players; the others only ask for a name at a high score. Each game's name rules and decision points
are in its `SiteProfiles.cpp`. Under `create`, a player deleted from the Players section is made again at the next
start if it is the site's current player. Where a game's own rename or delete leaves part of a player behind, so
does the page's.

### Save backup

The Saves section of Settings (`web/saves.js`) makes one .zip of the player's saves, in the page with nothing
uploaded, and reads one back. `tools/savestest.js` checks it.

**What is taken.** From each game's persist database: `userdata/` and, for games with `replays: true`, `replays/`;
and the game's localStorage backup (`sexy-userdata-backup:<persist>/userdata`, `platform/WebPersist.cpp`), which
can hold a save that never reached IndexedDB. Never `game/`, the icons or the mods. `byopop_web_index()` puts each
game's `data-persist`, `data-lock` and `data-replays` on its tile from its `page.js`.

**Back up.** The page reads each database without creating one and merges the localStorage item as
`webpersist_restore` would (`merged()`). The result is one store-only zip written with no library, named
`byo-pop saves <YYYY-MM-DD>.zip`:

```
byopop-saves.json            the manifest: {format: "byo-pop saves", version: 1, made, games, files: {path: ms}, play}
byopop-settings.json         the item byopop-settings
byopop-profiles.json         the item byopop-profiles
<game>/userdata/<path>       a file of the save folder
<game>/replays/<path>        a replay
<game>/browser-backup.json   the game's localStorage backup item
```

The play times (`byopop-play`) are the manifest's field `play`. A restore skips a top-level `byopop-<name>.json` it
does not know, so that a later build may add one (`classify()`).

**Restore.** The page checks every file's checksum and refuses the whole zip when an entry is outside a listed
game's `userdata/` or `replays/`, has a bad path (`..`, a backslash, a control character, over 240 characters),
names a game twice, or when the zip has no manifest, is encrypted, zip64 or damaged. A valid zip is not written at
once: the page shows what it would replace and add for each game, and only the Restore button writes. Files the zip
lacks stay; each game's localStorage backup is removed (a left one would delete the restored files at the next
start); settings and players are replaced; play times become the larger of the two (`mergedPlay()`).

A restore never runs while a game page is open: it checks the games' Web Locks with `navigator.locks.query()` and
holds every one while it writes. A browser without Web Locks backs up but does not restore. Each game is written in
one IndexedDB transaction, with a directory entry for every missing parent and each file's date from the manifest.

### The music player

The Music window (`web/music.js`) plays the music of the games the player added, from those games' own files in this
browser. The site ships no music and sends nothing anywhere. The window reads each game's database read-only (opened
without a version, so no empty database is made) the first time it opens (`shown`), and only for games whose record
says their files are here.

**Which files** hold a game's music is in `web/soundtracks.js`, by the game's id: path patterns in the game's folder,
matched without regard to case. A module gives its subsongs as tunes (those under two seconds are left out as
stings), and an Ogg Vorbis file is one tune. An entry may also have:

- `parts`: ranges of orders played as separate tracks (Bonnie's Bookstore's one module is nine tracks, from the
  track table in `games/bonnies/game/Music.cpp`).
- `joined`: a playlist file and the path of its short Ogg pieces, decoded and played one after another (Pixelus and
  Rocket Mania; `AssembleMusic` in `games/rocketmania/src/AppMusic.cpp`).
- `skip` (`"<file>#<n>"`), `names` (a plain name for a tune) and `level` ("Music levels").

Tunes are named plainly, by file name and "part n". No invented titles.

**The decoder** is the games' own two module renderers, built for the web as `openmpt.js` and `openmpt.wasm`
(target `byopop_openmpt`, from `web/music/openmpt_web.c`). As `Platform::Mixer` picks them, an MO3 or OXM plays
through libopenmpt and every other module (S3M, IT, XM) through libxmp-lite, so a tune sounds as it does in its
game. The page loads it only when the window first opens. An Ogg file is decoded by the browser
(`decodeAudioData`), or by stb_vorbis from the same wasm where the browser cannot. The main thread sends 4096-frame
stereo chunks to an AudioWorklet (its source a string in `music.js`), keeping about a second queued; the worklet
reports each chunk used and when it runs dry, which moves on to the next tune.

**The player** is in one state at a time: idle, loading, playing or paused. It follows "Values, not sentences": one
LCD-style display (state, time heard, the tune's and game's names, length, and the view), a seek bar, one row of
icon controls (Previous, Play/Pause, Stop, Next, Repeat, the speaker with its volume slider) and the playlist. The
time shown is what the listener hears, not the second ahead the decoder has rendered. The Media Session API gets the
tune's name, game and icon and the transport handlers, so the media keys work. Credits for libopenmpt and
libxmp-lite are in About.

- **Storage:** `byopop-music-loop`, `byopop-music-mute`, `byopop-music-volume`, `byopop-music-vis` and
  `byopop-music-rows`.
- **The piano roll** (the user, 2026-11-09: "add the piano roll too") is the view after the tracker, for a module
  only, as the tracker is. Its notes are bars on a pitch grid: time runs left to right in rows, as wide as lets 64
  rows (about a pattern) fill the display, 4 pixels at least (the user, 2026-11-11: "way too zoomed out"), with the
  heard row under a fixed playhead a third of the way in, and pitch runs up the display. A margin at the left names
  each C in the tracker's spelling (C-4), and every note where a semitone is 11 pixels high or more; the C of each
  octave has a faint line, and the black keys' rows are a shade lighter where a semitone is 4 pixels or more. Each
  pattern has a line every beat and a brighter one every bar, from its metre (below, as in the score), and a brighter
  one still at its first row; the metre is a value at the top left. Each instrument has one colour of twelve from the 16-colour palette (blue
  left out, too dark on black), and a bar carries its instrument's number as the cell writes it and its name, cut to
  the bar's length, where there is room (10 pixels high, 15 wide); where the semitones are thinner, a key at the bottom
  right gives each colour drawn its instruments' numbers and names, on one line or two, the names cut to 12 characters
  at most and 6 at least, or left out where even that would not fit. The names (the user, 2026-11-11: "If there is any way to get instrument names, let's do
  it") come from the decoder (`bm_names`) or the followed game (`byopop_music_names`): a module's instruments' names,
  or its samples' where it has no instruments; only the instruments that play are ever shown, so a composer's message
  written over unused sample slots stays out of sight, though not the lines of it on slots that play. A note lasts until the next note, key off, cut or fade in its channel, or to its pattern's end. The
  roll reads the cells the tracker reads (`rowText`, `byopop_music_row`), each pattern once. For the player's own
  tunes the decoder gives the pitch range at once (`bm_range`, the tune's lowest to highest note) and the order list
  (`bm_order_pattern`, `bm_pattern_rows`; a "+++" order is passed over), so the display is filled with the orders
  on either side. For a game's module, whose order list the page does not have, the range is that of the patterns
  heard so far, so it can widen; the pattern heard before the current one is shown at its left, and at its right the
  patterns that followed it the last time the tune went on from it, so the right of the display fills once the tune
  comes round again. The roll
  takes the splitter's height and is drawn only when its row, range or size changes.
- **The score** (sheet music) is the view after the piano roll, for a module only. It draws the tune in staff
  notation with VexFlow 5.0.0 (`web/third_party/vexflow-bravura.js`, MIT, served as `vexflow/vexflow-bravura.js`,
  728,171 bytes with its fonts Bravura and Academico by Steinberg Media Technologies, SIL OFL 1.1; the licences are
  `web/third_party/vexflow.txt` and `bravura.txt`, served under `licenses/` and linked from the listing's footer).
  The page fetches the file only when the score is first chosen, and draws once the music font is ready. It scrolls
  instead of fitting the window's width (the user, 2026-11-11: "sheet music should scroll instead of fitting to window
  width"): the patterns run on side by side, as on the piano roll (the orders on either side for the player's own tunes,
  the one heard before and those that followed it for a game's), and slide past a playhead a third of the way in, while
  the clefs and time signature stand still at the left. The scale is fixed by the display's height over the staves, 0.45
  to 0.8, each row 26 pixels wide before scaling; the staves scroll up and down where they do not fit. Each pattern is
  drawn once and kept (32 at most), as drawing one takes about 100 ms of the page's time, and at most one new pattern is
  drawn a frame besides the heard one. Each instrument that the patterns on the display play (the heard one and those
  either side of it) has a staff in its piano-roll colour, so the staves change at a pattern's end where the instruments
  do and those sheets are drawn again (a whole tune can have a hundred instruments, Bejeweled Twist's), the staff lines,
  bar lines, clefs and time signatures in grey. A staff's clef is treble when the instrument's notes in the tune (all of them for the
  player's own, which the page reads through once; those heard so far for a game's) lie at G below middle C or higher
  on average, else bass. An instrument that plays one pitch only, four times or more, is taken for a drum and
  drawn on a percussion staff with x noteheads. A tracker's C-5 is written as middle C. Within an instrument a note lasts
  until its channel's next note, key off, cut or fade, or until the instrument's next note in any channel. Notes that
  start together are a chord, and a length that no single note value gives, or a note held over a bar line, is tied
  notes. Beams follow VexFlow's own grouping, and sharps are written as the bar needs them (VexFlow's rule for C major).
  Effects (slides, vibrato, volume) are not drawn.
- **The metre** (the user, 2026-11-11: "not every pattern has 4 bars. Some have 8 or 12") sets the score's bars and the
  piano roll's lines, and is shown on both as a value: the time signature, the note a row is, and where it came from
  ("4/4 16TH FILE"). FILE is a pattern's rows per beat and per bar as the module stores them: an IT's row highlights
  (its header, read by `byopop_it_meter` in `platform/ModuleText.h`, as libxmp-lite keeps none) or what libopenmpt reads
  (an IT's, an MO3's), through `bm_meter` and `byopop_music_meter`; 4 and 16, libopenmpt's default and Impulse Tracker's, cannot
  be told from a chosen 4 and 16 and count as none (every game's IT has them, so every game's tune takes NOTES). NOTES is found from the notes when nothing
  is stored: the shortest bar of 8, 12, 16, 24 or 32 rows at which the channels' note onsets come round again (a channel
  that repeats every 2, 3 or 4 rows, a hi-hat's, is left out), doubled while a bar would last under 1.2 seconds at the
  speed and tempo playing (`bm_tempo`, the `speed` and `bpm` of `byopop_music_view`), with the beat that divides it into
  2 to 4 beats nearest 110 a minute. DEFAULT, 4 rows a beat and 16 a bar, is where the notes repeat too little to tell
  (half their onsets at best). A beat of 4 or 6 rows makes a row a sixteenth, 2 or 3 an eighth, 8 or 12 a thirty-second;
  3, 6 and 12 are compound time, a beat a dotted quarter (6/8, 12/8). The bars run on: a pattern whose length is not a
  whole number of bars ends on a short one.
- **The view** is the display's lower half; a click cycles spectrum, tracker, piano roll, score, nothing. The spectrum is 24 bars from
  40 Hz to 16 kHz from an AnalyserNode connected only while it shows. It is steadied (the user, 2026-11-10: "a little too jittery"):
  the analyser's smoothing is 0.8, a bar is the mean of its bins with their highest, rises over a couple of frames and
  falls by time, about its whole height a second, and a peak holds half a second before it falls. The tracker, for a module, has a terse header
  ("ORD 07  PAT 06  ROW 16/64"), a line over the columns (the user, 2026-11-11: labels on the columns) with each
  channel's number and the name of the instrument it played last up to the heard row, cut to the column's width, or
  the number alone where the column leaves fewer than 6 characters for the name, as the narrow cells do (XM,
  MOD and S3M have no field for a channel's own name and libxmp-lite keeps none from an IT, so the instrument's name
  stands in for it), and the rows around the one heard. A click on a channel's number mutes it and another brings it
  back; a Ctrl-click (Cmd on a Mac) leaves that channel alone, and a Ctrl-click on the channel already alone brings
  every channel back (the user's notes, 2026-11-10: "tracker labels and mute"). A muted channel's number is dimmed and
  struck through; a new tune starts with none muted. The player's own tunes mute through the decoder (`bm_mute`:
  libxmp-lite's `xmp_channel_mute`, libopenmpt's interactive interface's `set_channel_mute_status`), heard once the
  second it has rendered ahead has played; a followed game's through its mixer (`byopop_music_mute`), at once. A game
  built before that call, or a decoder cached from before `bm_mute`, does not mute, and the number stays as it was.
  The tracker never scrolls past the pattern's ends. Only
  the channels that play a note somewhere are shown (`bm_used`); when they do not all fit, a scroll bar drawn by
  the player moves through them. An Ogg tune shows the spectrum instead. While the tracker shows, the decoder
  renders in 512-frame slices and notes its row before each; a closed view costs nothing.
- **The display's height** is set by a splitter (`role="separator"`) between the player and the playlist, in whole
  tracker rows from 5 to 64. Rows asked past the window's room grow the window downward
  (`ByopopSkin.desk.grow`).
- **Only the playlist scrolls** (the user, 2026-11-09), so the display and controls stay visible; the window keeps
  at least three list rows (`minHeight`). A hidden pane runs no ResizeObserver, so a test there fires `resize`
  after showing the panel.
- **Closing the window stops the music** (the user, 2026-11-09): the desktop's close (`afterClose` in
  `web/index.html`) calls `ByopopMusicPanel.shutdown()`, which closes the AudioContext, frees the decoder and takes
  the Media Session handlers away. Minimizing keeps it playing. `ByopopMusicPanel.held()` tells a test what is
  still held.
- **At `?silent=1`** the volume's GainNode reaches the speakers only through a second gain of 0. The graph must
  still reach the destination, because a browser never runs a worklet whose output goes nowhere.

`tools/musictest.js` (`unit-web`) checks the decoder's view, rows and seek on a small module it makes, and on a
staged MO3 when there is one.

### Following a game

While a game window is in front and its game plays music, Music's views show that music live: the tracker shows the
game's module scrolling with the game, and the spectrum the game's own output (the user, 2026-11-09: "make the
tracker/spectral analyzer sync with what's playing in the active game"). A tag names the game. The game followed is
the game window last made active, while open and not minimized (`frontFrame` in `web/deskapps.js`). Music's own
playback is not changed; when both play, the views show the game's.

**Focus and sound (the user, 2026-11-10).** A site window is a desktop window: a game whose window is not the active one
does what its original does without the focus (most mute themselves; the framework's own focus handling, nothing added
for the web). `byopop_music_view` says whether the song reaches the output (`heard`), and the tag shows a muted speaker
for a game that is not heard; the position shown stays the game's own. Only one game window is heard at a time
(`hush` in `web/deskapps.js`): the game in front; with no game in front, none while Music plays a tune of its own, else
the game last in front. The others' output node is disconnected from their speakers, not their page's mute, which is
the player's own switch.

The game's side is a site hook in the platform layer: no game's code changes, and native builds do not compile it.
`Platform::Mixer` answers these exported calls (`platform/Audio.cpp`):

- `byopop_music_song()`: the song the game plays (the loudest playing one, 0 for none).
- `byopop_music_view(song)`: JSON with its kind (`module` or `stream`) and, for a module, its pattern, row, the
  pattern's rows, channels, order and title, and the speed and tempo playing.
- `byopop_music_meter(song, pattern)`: the pattern's rows per beat and per bar as the module stores them, beat << 16 |
  bar, 0 for none.
- `byopop_music_row(song, pattern, row)`: a row's cells separated by `|`.
- `byopop_music_used(song)`: the channels that play a note anywhere.
- `byopop_music_mute(song, channel, mute)`: a channel muted (1) or brought back (0), the tracker's header's click; 1
  when done. It is the window's own, apart from anything the game sets.
- `byopop_music_names(song)`: its instruments' names (its samples' where it has no instruments), a line each, "1A Bass"
  (`platform/ModuleText.h`'s `byopop_name_line`, empty names left out); a game built before the call has none, and
  the page then shows numbers only.

A libxmp-lite module is read from its `xmp_module`, a libopenmpt one through `ModulePlayer`'s web-only `GetView`,
`GetRowText`, `GetUsedChannels` and `GetTitle` (`platform/openmpt/OpenMPTPlayer.cpp`). Cells are written as Music's
own decoder writes them (`platform/ModuleText.h`). A stream, or TipTop's MIDI synthesizer, gives the spectrum only.

The page asks only while the Music window is on screen, once an animation frame. The spectrum is an AnalyserNode in
the game's own AudioContext, connected as one more output of the game's last node, so what is heard does not
change. A game at `?silent=1` has a flat spectrum; the tracker still follows. `ByopopMusicPanel.following()` tells a
test the game followed, its view and whether its output is tapped.

### The soundboard

The Sounds window (`web/sounds.js`) plays the sound effects of the games the player added, from their own files,
read with Music's read-only reader (the user, 2026-11-09: "I will want them for every game"). `#sounds` opens it.
Which files a game has, in which group box, under what name and when the game plays each is in
`web/soundboards.js`, by the game's id; a game with no entry is not listed. Names are plain and our own: a voice
line is its words in quotes, and the when-lines come from the port's own calls, never the game's text.

The window has a drop-down of the games, Stop, the speaker and volume (`byopop-sounds-mute`,
`byopop-sounds-volume`), the sounds as push buttons in group boxes, and a status bar with the count. A button plays
on a click, Enter or Space; different sounds overlap. Each file is decoded once, on first play (the browser's
decoder, stb_vorbis from `openmpt.wasm`, or our own reader for a Sun `.au` file). Closing the window stops what
plays and drops the decoded sounds and the audio context. The grid is one tab stop. It is silent at `?silent=1` as
the music player is, and is not shown under 760 pixels wide.

### The splits timer

The Splits window (`web/splits.js`) is a stand-in for LiveSplit, so that a speedrun can be timed on the site (the
user: "a LiveSplit stand-in on the site so that people really could do speedruns on it"). It is in Desktop 95 only,
under the Pop menu and as a desktop icon, closed at first. It holds one set of splits at a time: a game, a category,
a start time (LiveSplit's offset), the attempt count and the segments, each with the personal best's split time and
the best segment time, in Real Time and Game Time. Every set is kept in this browser's IndexedDB (`byopop-splits`,
one record a game and category), the last one opened comes back with the page (`byopop-splits-last`), and the
options are in `byopop-splits-options`.

The window shows the game and category with the attempt count, a row for each segment (the one being run selected)
with its delta and its time (the split's once it is done, the comparison's before), the big timer, the previous
segment's delta against the comparison and the sum of best segments. Ahead is green, behind red, a segment faster
than its best gold, and a finished run that is a new personal best blue; a paused timer is grey.

- **File:** New (Edit Splits on a blank set), Open (a list box of the kept sets, with Delete), Save, Import Splits
  and Export Splits. Import reads a LiveSplit `.lss` file: the game and category names, the offset, the attempt
  count and each segment's name, its "Personal Best" split time and its best segment time, in both methods; the
  rest of the file (icons, history, metadata, autosplitter settings) is not kept. Export hands the player a `.lss`
  file with the same parts, indented as LiveSplit writes it; a round trip keeps every time to LiveSplit's 100 ns.
- **Edit:** Edit Splits, a dialog with the names, the start time, the attempt count and a grid of the segments
  (name, split time, best segment) in the method being timed, with Insert Above, Insert Below, Remove, Move Up and
  Move Down. A time is read again only when its text was changed, so one left alone keeps its precision.
- **Control:** Start or Split, Undo Split, Skip Split, Pause or Resume, Reset, each with its hotkey shown.
- **Options:** Hotkeys (a dialog: a button for each action takes the next key pressed, Backspace clears it, and
  "Global hotkeys" turns them all off), the comparison (Personal Best or Best Segments) and the timing method (Real
  Time or Game Time).

The attempt count goes up at each start. Reset with a finished run faster than the personal best (in the method
being timed; a first finished run always is) makes it the personal best, as LiveSplit does at Reset, so an undone
last split never leaves a wrong one. If the attempt beat any best segment, in either method, a Windows 95 message
box asks whether to keep the new golds. File and Edit are off while a run is going.

Real Time is `performance.now`. Game Time is the update count of the game window in front when the run starts
(`byopop_update_count`) since that start, times the game's update step (10 ms; 5 ms in the Mahjong games); a game
without the count, or a run started with no game window, has none, shown as a dash. Pausing the timer pauses Real
Time only: the game's updates go on, as LiveSplit's Game Time follows the game.

The hotkeys are Num 1 split, Num 3 reset, Num 8 undo and Num 2 skip at first, Pause has none. They are heard on the
page and in every game window (same origin; a listener is added to each window's frame as it appears), and never
taken from the game: the key still reaches it. They are ignored while a dialog of the window is open, while the
focus is in a text field of the page, with Ctrl, Alt or Meta held, and while the Splits window is closed.

No web build has the LiveSplit block that desktop builds have (Bejeweled's `features.livesplit` is desktop only), so
there is no autosplitting here, and the run timer the site once had is gone (it was removed earlier).

### Music levels

Each port's music was measured against its original's engine (BASS or FMOD 3) with `tools/musiclevel`:
`musiclevel.exe` decodes a module silently with a copy of the game's own `bass.dll` or `fmod.dll`, and `portlevel`
renders it through `Platform::Mixer`; both print the RMS of the first seconds. libxmp-lite (S3M and IT) is within
about 1 dB of every engine. libopenmpt on MO3 differs by up to 2.4 dB either way from one module to the next, with
no setting that explains it, so that is left as an oddity.

The levels are shared code. `platform/MusicLevels.h` holds the render settings (libopenmpt's two-tap interpolation
and full separation, libxmp-lite's spline), BASS's amplify curve (`ByopopBassAmplifyDB`, used by
`SDLMusicInterface`) and FMOD 3's XM pre-amp of 256 against libopenmpt's 48. The music player's decoder includes
it, and so plays each module with the game's own renderer and settings. A game whose code plays its music at
another level says so in `web/soundtracks.js` as `level` (`{bassAmplify: 100}` for Bookworm Adventures,
`{fmod3XM: true}` for Bonnie's OXM).

Not in the music player yet: TipTop (MIDI inside its .dat), Word Harmony (WAV files), and Ning Po and the two
Mahjongs (music packed inside `ningpo.pyz` and `Resource.dll`).

### Importing saves from an installed game

A player who has played an original on this computer can bring its saves, scores, players and settings into the
port, in the browser and natively. Both read one table, `platform/saveimport.json`, which says for each game which
registry values and files of the original are the player's save and where each goes. Nothing outside the table is
taken, and the original's registry and folders are only read.

**Where the port keeps saves.** A native game's user folder is `%APPDATA%\byo-pop\<Game Name>`
(`SDL_GetPrefPath`), or `-userdir=<dir>`. It holds `registry.ini` (the port's emulated registry), the saves and
`mods.json`. The web's `<persist>/userdata` is the same folder.

**Where an original keeps its data.** The older SexyAppFramework games write
`HKLM\SOFTWARE\WOW6432Node\PopCap\<Game>`. Not run as administrator, Windows redirects that to
`HKCU\Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\PopCap\<Game>`, which the game then prefers, value
by value. Some games use `HKCU` (Bejeweled, both Bookworm Adventures, Word Harmony, Ning Po, the Mahjongs). Files
are beside the exe; under `C:\Program Files` Windows keeps them in `%LOCALAPPDATA%\VirtualStore\...`, which the game
prefers. A Steam install is writable. Some games use `C:\ProgramData\PopCap Games\<name>`. The Mahjongs keep theirs
in `%LOCALAPPDATA%\JollyBear\...`, which the native import does not search yet.

**The table** has one entry per game id (the tile's `data-game`, and the string `main.cpp` passes). CMake turns it
into `saveimport-table.js` for the listing and into a header compiled into every native game
(`platform/SaveImport.cpp`), so the two cannot disagree. An entry has:

- `folder`, `steam`, `appdata`: the usual install folder name, the Steam folder name and the ProgramData folder
  name. `note` is a line for the page.
- `registry`: `key` (the port's `mRegKey`), `hives` (where the original may keep it, in order of preference: `vs`,
  `hklm`, `hkcu`), `sets` (`base`: the framework's own options; `app`: `TimesPlayed` and `TimesExecuted`) and
  `values`, each `Name=type` with `sz`, `dword` or `hex`. A name may have `*` (any characters in one part) or `#`
  (digits) and may reach into subkeys (`Users\*\Alias`). A value not listed is not taken; `skipValues` and `skip`
  say why a value that looks like a save is left out (installer values, `InProgress` and the like).
- `files`: `from` (a path in the folder that holds the saves; `*` and `#` as above) and `to` (the path in the user
  folder, filled from the captures). `skipFiles` are files found and left alone, with the reason.

**How a value becomes the port's.** `userdata/registry.ini` has a section per key (`[SOFTWARE\PopCap\BigMoney]`) and
a line per value: `Name=sz:text` (backslash, line feed and carriage return written `\\`, `\n`, `\r`),
`Name=dword:123` (signed decimal) or `Name=hex:0a1b`. REG_SZ and REG_EXPAND_SZ are text, REG_DWORD a number,
REG_BINARY and REG_MULTI_SZ bytes; a value of another type than the table's is left out. Text is Windows-1252. The
file is written as the port writes it (`SexyAppBase::GetRegistryText`), so the port changes nothing when it saves.

**On the web** (`web/saveimport.js`, a wizard in the Saves section): the player picks a game; the page names the
folders that hold its saves and gives a `reg export` command for each registry place, with Copy buttons (they need
no administrator rights and only read). The player then chooses the folder, a VirtualStore folder if there is one,
and the .reg files. The page reads them (version 5.00 in UTF-16 or REGEDIT4, every value kind), merges the places
in the table's order, and shows what would be added, replaced or left out. Only the Import button writes, after an
in-page confirmation: it refuses a game that is open (Web Locks; also a browser without them), holds the game's
lock and writes `registry.ini` and the files in one IndexedDB transaction (`writeFiles` in `web/saves.js`). The
game's localStorage backup is folded in first and then removed, as a restore does.

**Natively**, each game's `main.cpp` calls `Platform::RunSaveImportIfRequested` (`platform/SaveImport.h`). With
`-importsaves` or `-importsaves=<install folder>` the import runs and the game exits without starting. On Windows it
reads the registry read-only (`KEY_READ`, the 32-bit view) and the files from the install folder, the ProgramData
folder and the VirtualStore copy, the later winning. Before replacing anything it copies what it will replace to
`<user dir>\before-import-<date>\`, and it writes and prints `<user dir>\import-report.txt`. Exit codes: 0 done or
nothing to import, 1 a file could not be written, 2 no entry for the game (or not on Windows). It does not check
whether the port is running.

**Tests.** `tools/saveimporttest.js` (`unit-web`) checks the table (every game, no path that leaves the user
folder, no installer value taken), the .reg reader, the merge, the conversion, the file patterns and the commands.

### Accessibility

The pages are built to be used from the keyboard and with a screen reader (`docs/MOD_MENU_DESIGN.md`, 5.6). Every
page has `<html lang="en">`, one `<h1>` and no skipped heading level. A game's page has a banner (the bar), one
`<main>` (the stage), an `<aside>` per panel named by its `<h2>`, and the footer. Buttons and links are real ones:
a file picker is a `<button data-pick="...">` that clicks a hidden input. Grouped choices are a `<fieldset>` with a
`<legend>`, every control has a label or `aria-label`, and help text is linked with `aria-describedby`. The
desktop's keys are in "Desktop 95".

**The keyboard on a game's page.** The first focusable thing is the link "Skip to game". The canvas is in the tab
order only while a game runs. A panel's button has `aria-expanded` and `aria-controls`; the Sound button is an icon
with `aria-pressed`. Opening a panel moves the focus into it; Esc or Close closes it and, with the keyboard, puts
the focus back on its button. The cards are dialogs (`role="dialog"`); "Already open" and the terms card are
`aria-modal`.

**How the game gets keys, and how to leave it.** SDL's Emscripten backend would listen on the whole window and take
every key on the page, so the shell's `Module.preRun` sets `SDL_EMSCRIPTEN_KEYBOARD_ELEMENT=#canvas`. The game gets
keys only while its canvas has the focus, and then every key, Tab included, except F11. Tab cannot be the way out
because games use it, so **F6** moves the focus from the canvas to the next control and Shift+F6 to the one before;
the game never sees either. The canvas is `role="application"`, named "<game>, game" and described by
`game.canvas_hint`. Keys held when the focus leaves the canvas are released in the game. F2 typed on the bar is
passed to the canvas.

Moving the focus from the canvas to a control on the page is not the game losing the focus: SDL and the framework
(`EmscriptenFocusCallback`) take that from the window's `focus` and `blur` events, so the game neither mutes nor
pauses.

**F11** on a game's own page is the Fullscreen button, wherever the focus is, when the page can put the stage in
full screen (`document.fullscreenEnabled`); the game never gets the key. On the desktop F11 is the browser's own.

**Live regions.** The status line is `role="status"`. The in-game mod menu reports the focused item through
`Module.onModMenuFocus(text)` (`framework/ModMenu.cpp`), which `web/modpanel.js` writes into `#menu-live`. An error
is an alert, and its field is `aria-invalid` and described by it. The import's progress bar is a
`role="progressbar"` with a value.

**Colour and motion.** Text passes WCAG AA (4.5:1) on every surface and the focus ring 3:1. State is never colour
alone. Every animation and transition stops under `prefers-reduced-motion: reduce`.

**The checker.** `tools/a11ycheck.js` reads the built pages with no browser (`node tools/a11ycheck.js build/web`;
`tools/check.sh unit-web` runs it): language, headings, landmarks, duplicate ids, ARIA references and values,
accessible names, tab indexes, new-tab links, the skip link, reduced motion and contrast. `--selftest` runs it on
pages with one fault each. It checks `index.html`, every game page that has a tile, and their copyright pages.
Markup that scripts build is checked in a browser with
`A11yCheck.checkHtml(document.documentElement.outerHTML)`.

## The site's text

Every word the site shows is in `web/strings/en.txt`, one `key = text` a line (the user, 2026-11-03: "I'd like to be
able to edit a single file to rewrite the prose on the site"). Its header says how to write one: continued lines,
`{name}` fills, the `a | b` forms of a count, and `_html` keys that hold markup. Not in it: the terms
(`web/terms.html`), the licences, each game's own paragraphs in its `page.js` (`importHtml`, `modsIntro`) and what
the games draw.

The build reads the file once (`byopop_web_strings_load()`). Page markup has `{{key}}` markers, which
`byopop_web_fill()` replaces, HTML-escaped unless the key ends in `_html`. Prose the build writes itself comes
through `byopop_web_string()`. Scripts get their text from `strings.js`, which the build writes and every page loads
first: `BYOPOP_STRINGS`, then `web/i18n.js`, whose `ByopopStrings.t(key, vars)` fills `{name}` and picks a count's
form by `vars.n` with `Intl.PluralRules`; `ByopopStrings.html()` does the same for innerHTML with its vars escaped.
A key the build asks for and the file lacks stops the build; one a script asks for shows as the key itself.

`tools/stringstest.js` (`unit-web`) checks that every key used is in the file and every key in the file is used.
The Node tests of the site's scripts check keys, never wording: `tools/stringsload.js` gives them a table in which
each string reads as its key followed by what it fills in.

Translations are not made yet. The plan is a copy of the file per language (`de.txt`, the same keys), from which
the build makes a set of pages for that language with `<html lang>` set.

## Steam on a Mac or Linux

Steam on a Mac or Linux installs only that system's version of a game, and the ports read the Windows files, but
its console's `download_depot <app> <depot>` downloads any depot of a game the account owns. A game released on
Steam has a `BYOPOP_STEAM_<target>` entry in `cmake/ByopopStores.cmake` with its app and the depot of its Windows
files. `byopop_web_shell()` puts the steps on the import card, hidden: open the console (`steam://open/console`),
type the command, then choose the folder it names. The page shows them when the browser says it runs on macOS or
Linux (`navigator.userAgentData.platform`, else the user agent), with that system's folder and keys.

## The site's logo and icons

`web/brand` holds byo-pop's logo and icons, which the build copies next to the pages:

- `favicon.svg`: the mark, a bottle opener on a sky-blue tile, drawn on a grid where one unit is one pixel at
  16 px. `favicon.ico` (16, 32, 48 px) and `apple-touch-icon.png` (180 px) are made from it by
  `python tools/brand_icons.py`, which draws them with headless Edge; run it after a change to the SVG.
- `logo.svg`: the wordmark "byo-pop", whose hyphen is the opener and whose "o" in "pop" is a red crown cap.
  `web/logo.js` puts it inline so that on hover or focus the opener levers at the cap (not with
  `prefers-reduced-motion`). It links to the listing (`./`), named "byo-pop, all games".
- `cap.svg`: a crown cap's outline seen from above.

The colours are taken from PopCap's classic logo (its red #ED1C24, sky blue, chrome); the shapes and lettering are
byo-pop's own. The SVGs are the sources and are edited by hand.

## Testing other copies of a game: `?placeholders=1`

`?placeholders=1` (`-placeholders` natively) lets a game start when some of its files are missing: a missing image
becomes a magenta and black checkerboard, a sound is silent, and a font without its descriptor uses the framework's
built-in letters (`platform/Placeholders.h`). The import then accepts any folder that holds one of the game's
folders. The status line counts the missing files, the Game files panel lists them, scores are kept apart and the
record of the player's files is not written.

A game's part, done when the user asks for it: `main()` calls `Platform::InitPlaceholders(argc, argv)` before its
game-folder check and lets `-gamedir` through with markers missing; it overrides
`SexyAppBase::GetPlaceholderSize` with its images' real sizes (64x64 otherwise), since wrong sizes can crash or
change play; an image the game may lack on purpose loads with `GetOptionalImage`; an override of `GetModMessages`
calls the base, which adds the missing-files banner. Data files with no stand-in (a word list, a difficulty table)
are the game's to handle.

## The performance panel

A game page shows the panel while the setting `perf` is on (`web/perf.js`; off by default). It only reads, so the
game plays the same with it on, and with it off nothing of it runs. In a game's window on the one page it is part of
the window, at the right of the game, and the window is that much wider (`sideWidth` in `web/shell.html`); nothing but
the game and its mod menu is drawn over the game (the user, 2026-11-10), so the input display is in the window's
status bar too. A game window's View menu shows and hides it ("Performance panel", ticked while it shows; the user,
2026-11-11: "it's difficult to show/hide the performance panel on mobile"), as the game's own setting. `?perf=1` on the listing's address or a game page's turns it on for that visit whatever the setting
says (the one page passes it to its game windows), and a phone then does not go to full screen at Play, which would
cover it. On a page under 600 pixels wide it is under the game, as wide as the page and scrolling in itself; on a
small screen its Sound group comes first. Each second it shows:

- **Frames:** frames per second and frame time, and the game's own time per frame, which the framework's frame hook
  passes (`sexyapp_test_frame`, `Module.byopopTestFrame`).
- **Updates:** updates per second and catch-up frames, from `byopop_update_count`.
- **Main thread:** the game's share of the page's one thread and the browser's long tasks (over 50 ms). Never
  labelled as the computer's CPU use.
- **Memory:** the wasm heap, the bytes malloc has handed out, and the framework's images and sounds with their
  bytes (`byopop_perf_json`, a read-only, web-only call made once a second); the JavaScript heap where the browser
  gives it; storage used and quota.
- **Graphics:** draw calls per frame, uploads per second and bytes, and the renderer's name, counted by wrapping
  the game's WebGL calls while the panel is on. GPU time per frame only where the browser offers
  `EXT_disjoint_timer_query`. The panel never asks for a context before the game has made its own, since that
  would take the canvas from the game.
- **Sound:** the context's `baseLatency` and `outputLatency`, the sound queued ahead and how often the queue ran
  dry (`Module.byopopAudio`).

## View, controls and play settings

These are site settings, each off or at the game's own behaviour by default. They act on the page around the game:
what is drawn over its picture, the input it is sent, and how often the framework runs its updates. None changes a
port. With all at their defaults nothing of them runs except a link in the frame hook that returns at once. The speed
and the two skip settings are never applied on the player-only page.

### View settings

`web/view.js`. **Scanlines** (`scanlines`: off, light, strong) is an overlay above the canvas, a dark line over
every second row of the game's own pixels, sized through a ResizeObserver. **Colour filter** (`colorFilter`: none,
grey, sepia, warm, contrast, vivid) is a CSS filter on the canvas.

Two more choices of the colour filter show the game as a 256-colour screen did (the user, 2026-11-09: "Is it possible to
make a filter for the game windows to make them appear in 256 colors only?"): **256 colours** (`pal256`) makes each
pixel the nearest colour (in plain RGB distance) of the palette Windows kept for programs that set none, its 20 static
colours and the 6x6x6 cube of levels 0, 51, 102, 153, 204 and 255: 228 distinct colours, since eight static colours
are on the cube. **256 colours, dithered** (`pal256d`) first moves each colour by an 8x8 ordered (Bayer) dither of up to
half a step of the cube, in the game's own pixels at any scale. They are drawn by a WebGL canvas the size of the game
canvas's own pixels, laid over it at its exact box (left, top, size and `image-rendering` copied; it follows the
window's scales, maximize and full screen) and under the scanlines, taking no input. It is filled in the framework's
frame hook (`Module.byopopTestFrame`) on each frame the game drew, while that frame's picture is still in the game's
canvas, by a shader that rounds to the cube and compares with the twelve static colours the cube lacks. Until the
game's next drawn frame the overlay stays unseen, since the game's canvas keeps no picture between frames. Another
choice removes the canvas and loses its context; the hook stays installed and returns at once.
Checked in the pane (2026-11-11) with Bejeweled and BookWorm: the overlay's box equals the canvas's at 1x, centred in a
maximized window and at 2x. Every pixel read back from it is in the palette, and none is farther from the game's pixel
than the nearest palette colour. At 2x every game pixel stays one colour. The game's hover and clicks pass through it.
The copy and the draw take about 1 ms a drawn frame at 1280 x 960 (a GTX 1080, read back to force them to finish), and
0.2 ms of the main thread.

### Controls

The Controls panel (`web/controls.js`) lists the game's controls in our own short words, from
`web/controldata.js`, keyed by game id. An entry is a mouse action, `{does, how}`, or one thing done by keys,
`{does, keys}`, the keys as `KeyboardEvent.code` values; a comment above each game names the handlers it was read
from. Only keys the game's own code handles are listed: not the framework's, not debug, cheat or mod keys.

- **Remapping.** "Change key" waits for the next key, which then stands for the game's key: a capture-phase
  listener on the window stops the player's key before Emscripten's listener and sends the game's key in its place,
  built as a browser builds one. The map is kept per game in `byopop-keys`
  (`{game: {keys: {from: to}, pad: {button: to}}}`).
- **Gamepad** (`gamepad`, on) does nothing until a gamepad connects. Then the frame hook reads it once a frame: the
  left stick or d-pad moves a pointer, A and B are the mouse buttons, and X, Y, LB, RB, Back and Start press the
  game's key entries in the table's order unless remapped.
- **Touch keys** (`touchKeys`: auto, on, off; auto shows them where the main pointer is coarse) puts a button
  under the game for each key entry.
- **Input display** (`inputDisplay`) shows the keys and mouse buttons the game is sent, in its bottom left corner.

### Phones

A phone is a touch screen whose short side is under 600 CSS pixels (`PHONE` in `web/shell.html`; `?phone=1` on a
development host for tests). There (the user, 2026-11-10: "the games should all default to full screen with pixel
perfect mode off"):

- The game is as large as its window allows, not a whole multiple of its size, and Play asks for full screen unless
  the "Full screen" setting is off.
- Before the game is made, the cards (the terms, the files, Ready) have the whole page and scroll, without the
  wizard's art column (a media query at 600 pixels, phone or not).
- The touch keys (`web/controls.js`) always have a keyboard button: it puts the focus in a text field kept out of
  sight, so the screen keyboard comes up, and what is typed goes to the game's canvas as key events (letters, Backspace,
  Enter). The field is read-only except while the button has it open, and a touch anywhere else closes it: the game's
  touch handlers cancel a touch's default, so the focus never left the field by itself, and a phone brought the keyboard
  back for a tap on the game, which then missed its button (the user, 2026-11-11, tapping a profile to switch players).
- The setting "Touch keys position" (`touchPlace`: automatic, under, beside) puts the keys in a column at the right of
  the game; automatic does so while the page is wider than tall. In full screen the keys stay on the screen and the
  game keeps clear of them (`--touch-w`, `--touch-h`, `fitFullscreen`).

### Game speed

`speed` (the listed 0.5 to 2, or any number from 0.25 to 4 typed in the field beside the list; the user, 2026-11-10:
"Allow game speed to be set to any arbitrary value") sets `SexyAppBase::mUpdateMultiplier` through the web-only `byopop_set_speed`
(`framework/SexyAppBase.cpp`), from the frame hook once the application exists. At 1 nothing is called, so unmodded
play is unchanged. It does not reach games whose own host runs their loop (Bonnie's Bookstore, Ning Po), nor a
timer a game reads from the wall clock. Music and sounds keep their own speed.

### Clips

`clips` puts a Record button on the bar (`web/clips.js`). It records the canvas's stream (`captureStream(60)`) and
the game's sound with MediaRecorder: WebM where the browser has it, MP4 otherwise. The sound comes from connecting
the game's audio output node (`Module.byopopAudio.node`) to a MediaStream destination as well. The clip is saved to
the player's computer when the button is pressed again, when the page is left, or after ten minutes. Nothing is
sent anywhere.

### Always skip tutorials

`skipTutorials` turns the game's own tutorial switch off at each start. The port registers a function for it in
its web-only file (`Byopop::gSiteTutorials`, `framework/WebSettings.h`). The page calls `byopop_site_tutorials`
until the function answers 1; a game whose switch belongs to a player answers 0 until one is loaded, and a game
with no function answers -1 and is left alone. The site never turns a switch back on. Each game's session
registers its own; games with no switch (Alchemy, Dynomite, the Bugs games, Bonnie's Bookstore, Ning Po, the
Mahjongs) are left as they are.

### Always skip start-up logos

`skipLogos` ("Skip start-up logos after the first launch") never applies to a game's first launch in this browser,
which shows its logos once (the user, 2026-11-10); the page keeps the games launched in `byopop-logos-seen`. After
that it has the games go straight past their start-up logos (the PopCap logo, a publisher's or developer's
logo, a "presents" splash; not the loading bar or the title's own animation), each as a click or key on it would,
so play afterwards is the same. The page tells the game before it starts: `byopop_site_skiplogos(1)` before
`callMain` (`framework/WebSettings.cpp`). A game reads `Byopop::SiteSkipLogos()` (`framework/WebSettings.h`), which
exists on both builds; natively it is true under `-skiplogos` (`SexyAppBase::HandleCmdLineParam`), and a game whose
own host parses its command line calls `Byopop::SetSiteSkipLogos(true)` itself. A game that does not read it is
left as it is. Cosmic Bugs, Water Bugs and Bejeweled Twist read it so far; each game's session names what its game
skips when it merges.

## The player-only page

For sessions that test a game in the Browser pane (the user, 2026-11-07: "build a stripped down version that loads
ONLY the player"). It is a game's own page with `?player=1`, and exists only on a development host
(`ByopopTerms.devHost`); elsewhere the parameter does nothing and `web/testkit.js` is never loaded. The page shows
the game alone at the top left, one canvas pixel per game pixel: no bar, chat, panels, terms, skin or footer. It is
silent unless `?silent=0`, each frame is the browser's or a 20 ms timer's, whichever comes first, and plays are
not counted.

Its parameters:

- `player=1`: the page and its tools, `window.byopop.test`.
- `assets=<name>`: the game's files from the staged `assets/<name>/` (`assets=1`: the page's own id), taken
  through the page's own import. Everything goes into a test profile (the storage and Web Lock names with "-test"),
  apart from the player's.
- `fast=1`: the game on its virtual clock (`-fast`), on the page's own frames. `framems=N` is the longest a frame
  runs the game's steps (25 ms by default); `frames=browser` uses the browser's frames instead.
- `hold=1`: the game waits for `byopop.test.start()`, so a test can wipe or seed the profile first.
- `silent=0`: sound on, only when the user says so.
- `realfocus=1`: the browser's own blur, focus and visibility events reach the game.
- `driver=<path>`: a driver script ("Drivers"). `run=<id>`: the folder of `shot(name)`'s files under
  `build/webshots/` (default "default").
- `seed=N`, `placeholders=1`: as on the full page.

`python tools/webtest/serve.py [--port 18780]` serves `build/web` on the loopback address only, with the staged
files at `/__assets/<name>/`, the repo's driver scripts at `/__repo/` and `POST /__shot`, which saves
`build/webshots/<run>/<name>.png`. The shots go to `tools/shotdiff` as the port's side (`docs/SHOTDIFF.md`).

**Fast runs.** A hidden Browser pane sends no animation frames and runs its timers about once a second, so with
`fast=1` the page drives the game itself: each frame is a MessageChannel message, posted at once. A frame stops
once the game's steps took `framems` (`byopop_set_frame_budget`; `EmscriptenMainLoop` returns between two steps of
the update state machine), so the page's own JS and the tool calls are served between frames. Neither changes an
update or a draw. A run that compares states with another must send its starting commands in the same update
(`Promise.all` of `command()` calls), since the updates between two commands sent one after another depend on the
frames.

**`byopop.test`** (every call that acts resolves once a frame has been drawn after it):

- **ready** resolves at the first drawn frame with `{updates, screen}`; `progress()` reports before then.
  `start()` starts a `hold=1` page; `loaded` resolves once the files are in.
- **errors** collects every abort, uncaught exception, rejection, printed error line, message box and failed
  fetch, each `{kind, text, update, at}`. A message box is answered at once and never shown (`confirmAnswer`, true
  by default).
- **input**, at game coordinates, as the DOM's own events on the canvas: `move(x, y)`, `down`, `up`,
  `click(x, y, button)`, `drag(x0, y0, x1, y1, steps)`, `wheel(x, y, dy)`, `key(name, mods)`, `keyDown`, `keyUp`,
  `type(text)`. Keys are named as the DOM names them. `keyCheck(name, mods, element)` sends a press and resolves
  with which events the page cancelled, `{key, down, press, up}`.
- **time**: `updates()`; `pause()` holds the game between two updates (it still draws and takes input), `step(n)`
  runs n more, `resume()` lets it go; `wait(n)` (also `waitUpdates`) waits n updates; `frame()` the next drawn
  frame. The hold is `byopop_hold_updates_at` in `framework/SexyAppBase.cpp`.
- **shots**: `shot()` resolves with a PNG data URL of the canvas at the game's resolution; `shot(name)` also saves
  it through the server. The WebGL context keeps its drawing buffer on this page, so a held game can be shot.
- **focus**: `blur()` sends what a real loss of focus sends, `blur({hidden: true})` also a `visibilitychange`, and
  `focus()` gives it back. The focus and visibility are the test's alone: the browser's own events stop in the kit
  (a hidden pane sends a real blur, which would pause a game), unless `?realfocus=1`.
- **the log**: `log(since)` gives the game's printed lines (`{seq, kind, text, update}`, the last 20000),
  `logNext()` the next line's number, and `waitLog(pattern, {since, timeout})` the first matching line (timeout
  60000 ms by default, 0 for ever).
- **test commands** (`docs/TEST_COMMANDS.md`): `command(line, {timeout})` sends one command through
  `byopop_test_command` to the game's `HandleScriptCommand` and resolves with the lines printed up to its answer
  (`STATE {json}`, `MOVE {json}|null`, `OK <command>` or `ERROR <command>: <reason>`). `state()` is the state's
  JSON. `botMove()` is the bot's move or null; `playMove(m)` sends one through real DOM input: its `input` steps
  in order (`{move:[x,y]}`, `{click:[x,y]}`, `{press:[x,y]}`, `{release:[x,y]}`, `{key:"B"}`, `{wait:n}`).
- **readings**: `watch()` (a port's watch values, `byopop_watch_json`), `mixer()` (`byopop_mixer_json`), `screen()`.
- **profile** (test profile only): `wipe()` deletes its saves and their localStorage backup, `seed({path: data})`
  writes files into its user folder, `list()` and `read(path)` read them.

### Drivers

Game sessions play their real-input checks with driver scripts (the user, 2026-11-07: "Yes, use drivers"): a script
in the page reads the game's state, chooses its moves and sends them as real DOM events, so a whole game costs a
call or two. A driver is a file named with `?driver=`: `games/<game>/tools/<path>.js`, or a shared one,
`tools/web/<name>.js`. The page runs it once the game is ready (on a `hold=1` page, once the files are in; it then
starts the game itself) as the body of an async function of `t` (`byopop.test`). What it returns becomes
`byopop.test.result` (`{error}` if it threw), and `byopop.test.driver` is a promise of the same, which one
`javascript_tool` call can await. A game's own driver runs a shared one with its table by
`return await t.run("tools/web/<name>.js", {...})`, the table arriving as `args`.

**The built-in driver `?driver=move`** plays any game by its port's own bot (`docs/TEST_COMMANDS.md`, "The bot
behind `move`"). It loops `state()`, `botMove()`, `playMove()`, so every move is real input. It stops when the
state says `"over": true`, on `MOVE null` twice running while the game is not busy, on an ERROR, or after
`?steps=` moves (2000 by default). While the state says `busy`, `MOVE null` means "not yet": it waits 5 updates and
asks again. The result is `{moves, stop, state, updates, errors}`, with `stop` one of "over", "limit", "no move",
"busy" or "error: <message>". `playByMove({steps})` runs the same loop from a session's own driver.

### The page check

`tools/web/pagecheck.js` checks how a game's page behaves as a browser page, which no comparison with the original
reaches. Every game takes it in its real-input pass, through a table of its own in
`games/<game>/tools/web/pagecheck.js`, opened as

    <game>.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/<game>/tools/web/pagecheck.js&run=pagecheck

with `tools/webtest/serve.py` serving it (the launch configuration `pagecheck-webtest`, port 18780). It needs
`hold=1` and `assets=`. It wipes the test profile, starts the game, sends the table's `start` commands, runs its
checks and reloads the page. So it ends in two halves: `await byopop.test.driver` resolves with
`{reloading: true}`, and once the page is back, with `{pass, fail, info, results}`, each line
`{check, result, detail}`. Read `byopop.test.result` between calls instead of awaiting across the reload. A run
takes about three minutes in a hidden pane. The checks:

- **Focus.** The page is hidden and shown again mid-play, and the state keys that changed must be the ones the
  native build changes when its window loses and regains the focus. `tools/web/pagecheck_native.py` gives that
  answer with the same start (`--first`, `--title-move`) and prints the line for the table's `focus` field.
- **Keys.** With the game focused, the page must cancel Tab, Backspace, Space, the arrows, Page Up and Down, Home,
  End, the table's `keys`, and `'` and `/` (Firefox's quick find). Which browser keys it takes (F5, F11, F12,
  Ctrl+R and so on) is an information line. Nothing typed into a text field outside the game may be cancelled.
- **Resize.** The canvas is drawn at 1.5, 0.75, 2 and 1 times its size and the mouse moved to the table's `point`;
  the framework's `pointer` command must answer that game pixel, give or take one.
- **Pages.** The same game opened again in an iframe must wait for the first (the Web Lock) and run no update.
- **Reload.** After the bot makes the table's `moves`, every save file the profile held must come back with the
  same bytes, unless the page's backup restores a newer version or the table's `reloadChanges` names it; and every
  save the backup held must be in the profile once the game has started. `afterReload` adds a check of the game's
  own.

While a game has the focus, SDL cancels the function and Ctrl keys, so F5 and Ctrl+R reload nothing until the
player clicks outside it or presses F6.

## Sound

Every game's sound goes through the shared mixer (`platform/Audio.cpp`), which on the web plays through the page's
own output, not SDL's, whose 2048-frame ScriptProcessor made sounds late.

The output is an AudioWorklet that keeps a queue of mixed sound and asks the page for the next 256-frame chunk
whenever the queue runs short; the page mixes it at once, between the game's frames. The queue is at least 2048
frames, grows to 4096 when the page answers late and shrinks again after three seconds of quick answers. A sound
the game starts does not wait behind the queue: its first frames are mixed straight away and added into the sound
already queued, from 640 frames ahead of what the context has played. A sound reaches the speaker about 59 ms after
the game plays it, of which 50 ms is the browser's own output latency. A page with no AudioWorklet (plain http from
another machine) falls back to a 1024-frame ScriptProcessor pulled the same way.

Muting drops the queued sound, so a mute is heard at once: the game's own (`SexyAppBase::Mute`, called on losing
the focus) and the page's Sound button. A hidden tab gets no animation frames, so the page mutes the game when the
tab is hidden (`SexyAppBase::EmscriptenPageHidden`).

For tests, `Module.byopopAudio` has the output's state (`kind`: `worklet` or `script`; `target`, the queue's
length; `gaps`, how often it ran dry; `late`). `Module.audioOutput = 'script'`, set before Play, forces the
ScriptProcessor. A function in `Module.onVoiceMixed` is called with the page's clock at the play call when a
sound's first frame is mixed, and `Module.audioChunkEar` holds the time that frame reaches the speaker. `?silent=1`
mixes as usual and sends silence, so the timing is the same.

## Memory use

The same game takes more memory in a browser than natively:

- **The tab itself** costs some tens of MB.
- **Game files in memory.** IDBFS copies every stored file into memory at the start, so all imported files and mods
  are held in RAM before the game reads one.
- **Decoded images, twice:** once as 32-bit pixels in the game's memory, once as a WebGL texture.
- **A heap that never shrinks.** WebAssembly memory cannot shrink, so the loading peak stays reserved.

An HD 4x texture mod multiplies each of these by 16 for every image it replaces. Ways to reduce it, not yet done:
read files lazily from IndexedDB, free an image's decoded pixels once its texture is on the GPU when the game never
reads them again, and decode large images one at a time.
