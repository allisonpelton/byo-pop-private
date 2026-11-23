# How the web player works

What a browser loads when it opens the site, how the pieces talk to each other, and the user's rules for the site. The
site is a listing page drawn as a 1995-style desktop ("Desktop 95") and a page for each game, built by
`cmake/ByopopWeb.cmake` into `build/web`. Native builds share the game code; only the outer layer differs. How a test
opens the pages (`?silent=1`, the test key, the Browser pane) is in CLAUDE.md, "Running games in tests".

## The pieces

For each game (`bejeweled` as the example) the build writes `bejeweled.html` (`web/shell.html` with the game's
`web/page.*`, then `web/modpanel.js` inline before the game's script), `bejeweled.js` (Emscripten's loader,
`Module`; readable with `BUILD_TYPE=Debug tools/build.sh web`) and `bejeweled.wasm` (the game with `framework/`, `platform/` and SDL2). The
player's game files are copied into this browser's storage, never uploaded and never part of the build. A new site
script in `web/` goes into `BYOPOP_WEB_SHARED_JS` (every page) or `BYOPOP_WEB_INDEX_JS` (the listing page) in
`cmake/ByopopWeb.cmake`.

## What happens when a game's page opens

The page loads `<game>.js`, which compiles the wasm while `main()` waits (`INVOKE_RUN=0`). IDBFS mounts the game's
IndexedDB at its persist folder, and the page copies it into memory (`game/`, `userdata/`, `mods/`, `replays/`), then
puts back saves that reached only the localStorage backup (`byopop_userdata_restore`). The copy is the shell's own
`loadStorage`: one read of the whole store, each file made under its folder's node, leaving the same files, order,
modes and times as IDBFS's `syncfs(true)`, which reads the files one at a time and finishes the copy if `loadStorage`
fails. On a return visit to Bookworm Adventures 2 (9,334 entries, 65 MB) it takes 0.8 to 0.9 s where `syncfs(true)` took
2.1 to 2.8 s. Keeping each pak whole beside its unpacked files would read faster still, by about half a second, but
would store the pak's size again and need a pak reader in the platform, so a game's files stay unpacked only (item 8,
2026-11-23). Since `loadStorage` uses IDBFS's internals (`getDB`, `DB_STORE_NAME`, `storeLocalEntry`) and MEMFS's
(`node_ops`, `stream_ops`), `tools/storageloadtest.js` in unit-web checks it after every Emscripten update: in the smallest
game's own module under Node, with an IndexedDB of its own in memory, it stores a tree through `syncfs(false)`, loads it
once with `syncfs(true)` and once with `loadStorage`, and compares the files, their order, modes, times and contents, and
what the next `syncfs(false)` would store. It takes about a second. A Web Lock (`PAGE.lock`) lets one tab
run a game. The fixes mod is put in place, the terms are asked, and without the game's files the import card copies
only what `page.js` names, by the rules in `web/gameimport.js` (a pak named in `paks` is unpacked). The card takes the
game's folder or a .zip of it from its two buttons, or dropped anywhere over it (the user, 2026-11-16: "I should be able
to drag the folder into the load files screen"): a drop is read by `fromDrop`, as the listing reads one, with the folders
and files found counted in the card's line, and the card shows a dotted line round its contents while files are over
it. No other part of a game page drops files into the browser: a window listener cancels a file drag that nothing took
(the no-entry pointer) and its drop, so a folder dropped anywhere else no longer opens the browser's listing of it. The exe of a game
that `web/exetrim.js` lists is stored without its code sections, and a launcher's inner image alone, as the port reads
only texts and tables from it (about 32 MB down to 7 MB over the listed games; `tools/exetrimtest.js` checks every table
address reads the same; a game stored before keeps its whole exe until it is imported again). Files no port opens stay
out too, by `NEVER_OPENED` in `web/gameimport.js`: AstroPop's and Dynomite's `sounds/cached_*.wav`, the originals'
decoded copies of their Ogg sounds (15.4 MB of the Steam copies). `tools/web/unreferenced.py` lists every stored file
nothing names, as a list to check by hand: names the code builds keep most of the rest of its 20.5 MB in use. Play calls `main()`
with `-gamedir=/persist/game -userdir=... -modsdir=...`, plus `-gameseed=N` for `?seed=N`, `-placeholders` for
`?placeholders=1` and `-nomods` for "Play without mods"; the game runs one frame per animation frame. Saves go to
IndexedDB as they happen, with a small backup in localStorage, because a closing tab cannot wait for IndexedDB.

The game's life is one state, `gameState`, with the allowed moves in `GAME_MOVES`. A game runs only once in a page and
its files must not change under it, so "Restart game", a new import and "Forget game files" close it first, saving,
and go on in a reloaded page.

## The bar and the panels

The panels are ordinary HTML around the canvas. They call the game with `Module.ccall` (`byopop_mods_open`,
`byopop_mods_state`, `byopop_mods_edit`, `byopop_mods_rescan` in `platform/WebApi.cpp`; `byopop_menu`,
`byopop_restart` and `byopop_quit` in `framework/ModMenu.cpp`), and the game calls back through
`Module.onModsChanged`, `Module.onGameScreen(width, height)` (before it makes its window, for a stage other than 4:3,
as BookWorm's widescreen mod's) and `Module.onGameExit`. `byopop_quit` is a game window's Close: it calls the app's
`QuitAsGame` hook (`SexyAppBase.h`), which shuts down at once by default and which a game overrides to leave as its own
Quit does (Bejeweled Twist plays its goodbye first); it returns 1 when a game took the call and 0 when none runs, and a
build without the export is closed with `byopop_restart(1)`.

**Mods** (`web/modpanel.js`) adds and removes mods, which the game reads from `/persist/mods` at its next start; the
rest is the game's F2 menu (`docs/MOD_MENU.md`). A game in `BYOPOP_WEB_NO_MODS` (the page's `MODS_SUPPORTED`) has no
Mods button; a port leaves that list when it starts loading mods. A game can add panels of its own (Big Money's
**Replays**).

## The HTML shell

`web/shell.html` is every game's page template; the game's `web/` folder adds `page.js` (required: `PAGE`, its fields
listed at the top of the shell's script, in whose scope it runs first) and optionally `page.css` and `page.html` (its
own panels, each `<aside class="panel" data-button="Name">`). IDBFS names a game's database after its storage folder
(Bejeweled `/persist`, Big Money `/bmpersist`), so a folder and lock never change once players have saves there; a new
game takes its own. `byopop_web_game(<target> TITLE "..." COPYRIGHT "..." [LICENSES <name>...])`, called from the
game's `CMakeLists.txt`, fills the shell's `byopop:game-js`, `byopop:game-css` and `byopop:game-panels` markers and
`@BYOPOP_TITLE@`; `byopop_web_shell()` adds the footer, the terms and the styles and writes `<game>-shell.html` and
`build/web/<game>-copyright.html`. The page is written apart from the game's link (`cmake/ByopopWebPage.cmake`, the
`<game>_page` target): its `{{{ SCRIPT }}}` line becomes `web/modpanel.js` inline, then the game's script as emcc
writes it, and the page is minified as emcc minifies one. So an edit to a page, to `web/shell.html` or to
`web/modpanel.js` rebuilds every page in seconds and relinks no game.

**Names.** Every name the site shows for a game is its `TITLE` (or its row in `BYOPOP_WEB_UPCOMING`), never with
"Deluxe" (the user, 2026-11-04: "Let's not use the name Deluxe anywhere in names on the site"). "Deluxe" stays only
where it finds the original's files: folder names in `platform/saveimport.json` and the game import, store links, and
the game's own art.

**The footer and the copyright page.** `<game>-copyright.html` (from `web/copyright.html`) holds the notice, the terms
and the licence of every library the page is built from (`web/licenses/<name>.txt`: a title line, a blank line, the
text). Every link to the site's licences, terms and copyright pages opens a new tab (`target="_blank" rel="noopener"`),
so a game or the music never stops for one; `tools/linkstest.js` (`unit-web`) fails on any that does not.

### The game's cursor on the page

A SexyAppFramework game draws its own cursor over the canvas. The shell polls `byopop_cursor()` every 100 ms
(`framework/WebCursor.cpp`, read-only: 0 for the browser's cursor, else a number that changes with the image), and
`byopop_cursor_copy()` puts the image in `Module.byopopCursor`, which the page sets as the CSS cursor of everything but
the canvas (`.game-cursor` on `<html>`).

### The site's cursor

The player can choose one game's cursors for the whole site (the user, 2026-11-03: "pull custom cursors out of the
user's uploaded games so that they can select one to use across the whole site (except for inside other games)"). In
`web/cursors.js` (checked by `tools/cursorstest.js`) a started game's page takes its cursors with
`byopop_cursor_set_copy()` and keeps them in IndexedDB (below); the choice is `byopop-site-cursor`. A game's own cursor wins on its
page. A kind may carry animation `frames`, which the page plays itself (Banana Bugs will need them,
`docs/POPCAP_GAMES_SURVEY.md`). Water Bugs and Cosmic Bugs draw their own pointer, the "pointer" animation of their
files, so their pages read it from those files instead (`web/flicursor.js`, `ByopopFlicCursor`): every frame of the FLIC
with its mask and the shadow the game draws, the hot spot at the top left, and the frame time from the animation file, in
the game's updates of 1 ms (the games step once for each millisecond passed): 50 ms and 30 ms, 10 frames each, looping.
Sets kept before 2026-11-16 read the updates as 10 ms (500 and 300 ms, the user: "They animate extremely slowly"), and
`web/cursors.js` puts them right once (`retime`). Ning Po's pointer sprite is empty, so it has no set. Venice draws
its cursor as a scene object, so its port reads that picture again and gives it as the set's pointer
(`games/venice/src/Cursor.cpp`, `WebCursorSetKind`; the user, 2026-11-19: "Venice is missing its custom cursor"). Bonnie's Bookstore
gave one from 2026-11-16 until the user had it taken out (2026-11-17: "they look just like the Windows default"); a set
kept for it is dropped at load, with a choice of it, and never kept again (`withdraw`).

The page swaps an animated cursor's frames itself, as the inline cursor of the element under the mouse, keeping that
element's own inline cursor aside and putting it back after. Each frame first looks again at what applies, so the swap
stops as soon as the game shows its own cursor (the `game-cursor` class) or hides the browser's over its canvas (SDL's
`cursor: none`). Before 2026-11-16 the swap wrote over SDL's `none` and so drew the site's cursor over a game drawing
its own (the user: "The Cosmic Bugs cursor shows duplicated over the game cursor"). The listing page builds those
two sets the same way as it adds their files (`web/gameimport.js`, `flicSet`), so they are in Settings' choice before the
game is first opened. The other games' sets come only from a running game: the framework holds their pictures once the
game has loaded its images.

The sets are kept in IndexedDB, in the database `byopop-cursors` (its store `sets`, by game id), their pictures as binary
PNG Blobs (the user, 2026-11-16, of the three changes below: "Will want all three improvements applied"). Until 2026-11-22
they were base64 in localStorage (`byopop-cursors`, 1 MB at most together, 196 KB for every game's set on 2026-11-16); a
page that finds that item moves the sets across as it loads, cropped and retimed as before, and removes it once all are
in. A picture with at most 256 colours is written again with a palette where that comes out smaller than the browser's
own PNG: the site's own writer, colour type 3 at 1, 2, 4 or 8 bits a pixel, its rows unfiltered and deflated by the
browser's `CompressionStream`. Every pixel that shows keeps its exact value, half-see-through ones included. An animated
cursor's frames are one strip, in cells of one size side by side, each frame placed so that its hot spot sits at the
same point of its cell; they are cut into frames again as the set is read. In the pane's profile on 2026-11-22, Heavy
Weapon's and Bejeweled 2's pictures came out 14 to 38 percent smaller than the browser's PNGs of the same pixels, but
for Bejeweled 2's pointer, which stays the browser's; ten drawn frames took 4,658 bytes as a strip against 6,552 as ten
PNGs.

localStorage keeps only small items: `byopop-cursors-kept`, each set's title and signature (a set given again with the
same signature is not written again), and `byopop-cursor-shown`, the chosen set as the site shows it (5 KB for Heavy
Weapon's), so that every page shows it from its first line and the browser's own cursor never shows while the database
is read. A page reads the database only for what it needs: the chosen set when it has no copy of it, and every set when
Settings' choice is drawn. The screen saver's frame keeps its sets in its memory (`ByopopInMemory`, set by the page's
first script with its memory storage). A set the browser will not store, or whose pictures cannot be read, is not
dropped unseen. Its game is kept in `byopop-cursors-unkept` until its set is kept or its files are forgotten, and
Settings' choice names it (the user, 2026-11-16: "I'm missing some custom cursors"). `GIVES` lists the games that give a
set, and `tools/cursorstest.js` checks it against their sources. On the listing page, Settings' choice names each of
those games whose files are here and whose set is not, as needing to be opened once. In the choice, an animated cursor's
picture plays its frames at the cursor's pace while the mouse is over its row (the user, 2026-11-16: "Add mouse over
for animated custom cursors to show the animation"), the frames laid on each other by their hot spots in a box of one
size, and shows its first frame again when the mouse leaves.

### The terms

Before a game's files are imported or loaded, its page asks the player to agree to the game's terms: a record of
agreement, not a lock. The text is `web/terms.html`, one file for every game, filled with the game's `TITLE` and
`COPYRIGHT` by `byopop_web_shell()`. The agreement is kept in localStorage under `byopop-terms-<PAGE.id>`, versioned by
the first 12 hex digits of the SHA-1 of the filled-in text, so each game asks on its own and any change to the wording,
name or notice asks again; the import, the start and "Play without mods" refuse to go ahead without it. The listing
page asks too, one game at a time (the user, 2026-11-03: "Make it go through an agreement for each game uploaded but
allow it to be done on the front page"), from `terms-games.js`, writing the same entry.

Every build has the card, and no build option turns it off. `web/terms.js` (`ByopopTerms`, checked by
`tools/termstest.js`) skips it, keeping no agreement and counting no play (`docs/PLAY_COUNTER.md`), on a development
host (`ByopopTerms.devHost`) unless `?terms=1`, and for the rest of a tab opened with the site's test key in the
fragment (`#testkey=<key>`, which a browser never sends to a server; the build bakes in only its SHA-256). The key is
the first line of the git-ignored `secrets/web-testkey.txt` (CLAUDE.md says who may use it and where it never goes),
made by `python tools/testkey.py`.

### Where to buy

One line under the terms says where the game can be bought digitally, as of a date, or that it is not for sale, from
`cmake/ByopopStores.cmake`: `set(BYOPOP_BUY_<target> <YYYY-MM-DD> [<store> <url>]...)`; a game with a page or a
coming-soon tile and no entry stops the configure. Only stores that sell the game new and digitally under the
publisher's licence are listed, never key resellers; check them again and change the date before a launch. A store link
is a plain link opening a new tab (`rel="noopener noreferrer"`), never a script's `window.open`.

### The listing page

`build/web/index.html`, the site's root, is made by `byopop_web_index()` from `web/index.html`. The lists in
`cmake/ByopopWeb.cmake` are the coordinator's, edited at a game's final merge: `BYOPOP_WEB_FINISHED` (the games with a
page), `BYOPOP_WEB_UPCOMING` (`<target>|<title>` rows for "Coming soon"), `BYOPOP_WEB_BADGES` (initials and colour),
`BYOPOP_WEB_RELEASED` (`<target>|<date>`, the first release of the Windows edition the site plays, its source in a
comment) and `BYOPOP_WEB_WIN95` (yes, no or unknown). Each game is a card in the page's `main` (`ul.games`), carrying
them as `data-game`, `data-title`, `data-released` and `data-win95`, and `data-unsold` where no store sells the game;
the listing's script adds `data-files` (yes, no or unknown) once it knows whether the game's files are here. The cards
are the page without its scripts: Desktop 95 does not show them, and the Library ("The Library", below) reads its games
from them.

The user's rules for it:

- **Games no longer sold** are hidden unless the Library's "Show games no longer sold" is ticked
  (`byopop-listing-unsold`) or the game's files are here (the user, 2026-11-03: "default to hiding those games from the
  front page"); the box is there only while some game is no longer sold.
- **Windows 95 games only** (`win95Only`) hides the games whose original did not support Windows 95, and the unknown
  ones; a game released before the last one whose readme names Windows 95 (TipTop, 2002-12-12) counts as yes (the
  user, 2026-11-08). The Library's status bar counts the games it hides ("3 hidden", the setting in its tooltip).
- **Play time** counts only while the game runs and its page is shown and focused (the user, 2026-11-02: "Only count
  time with the game open and active"): `byopop-play` (`ByopopPlay` in `web/settings.js`).
- **A game's icon** is read out of the player's exe (`web/exeicon.js`; `importIcon: true` in `page.js` for a game that
  does not store its exe) and never sent anywhere; a game without one shows its initials on a plain white square (the
  user, 2026-11-08: "bare initials for missing games").
- **Buy links,** at the foot of a game's menu in the Library, are hidden once the player has ever loaded files for the
  game or played it here, so that the page never suggests it is for copies not bought.

**Which games are yours** is `byopop-game-files` in localStorage (`ByopopGameFiles` in `web/settings.js`), never
written with `?placeholders=1`; without an entry the listing looks in the game's database read-only, calling
`indexedDB.databases()` first so that it never creates one. Forgetting a game's files (`ByopopImport.forgetGame`)
keeps saves, mods and replays and holds the game's Web Lock. A game whose files were loaded here once and are gone
has "Why aren't my files here?" in its menu in the Library (and a small "?" button on its card in the page without
scripts), which opens a message box titled with the game's name (`ByopopApps.tell` in `web/deskapps.js`, with the
information sign): the day the files were last loaded and the day they were forgotten, the likely causes, whether saves
are still here, and the buttons Open game and Close.

### Jungle Juice

The site's second page, which will hold games decompiled by other projects (the user, 2026-11-04); for now a
placeholder with no third-party content. `byopop_web_jungle()` writes each `web/jungle/*.html` into
`build/web/jungle/` with its `{{key}}` markers filled from the site's text (`jungle.*`).

### Adding games from the listing page

`web/addgames.js` adds many games at once: a folder or .zip dropped on a finished game's row in the Library is that
game's, and anything else, or the Library's Add games, goes to the Setup window. Its panels (the adding rows and the
bin) show at the top of the Library. **Sorting** (`ByopopImport.sort` in `web/gameimport.js`): a folder is a game's
when it holds every one of the game's markers, and a marker may name alternatives (`"WinTS.exe|TyperShark.exe"`, Typer
Shark's Steam and PopCap Game Collection exes); a folder of several games is looked into three levels deep; where two
games' markers are at the same place the folder's name decides, else the player is asked. **Copying** writes the files
straight into the game's IndexedDB database in IDBFS's records (`FILE_DATA`), under the game's Web Lock and all or
nothing, the marker files last. It follows the game page's own rules: `byopop_web_index()` writes each listed game's
`page.js` into `import-games.js`. `tools/gameimporttest.js` (`unit-web`) checks the rules and the sorting with every
finished game's `page.js`.

**A pak marker** (`main.pak` of Bookworm Adventures, its Volume 2, Bejeweled Twist and Venice) is found either as the pak
or with the pak already unpacked, as a staged copy or a zip of one has it. For the first three, any file in the game's
folders shows the pak unpacked, since their other marker is their own exe, which no other game has. Venice's other marker is
`config.xml` and its folders are `audio`, `gfx`, `levels`, `objects` and `properties`, names many games use, so another
game's folder was taken as Venice (the user, 2026-11-16: "Fix it now"). A page may therefore name `unpacked`, a few files
only its pak holds; Venice names two, one under `levels` and one under `objects`, and only a folder holding both stands
for its `main.pak`. The sort looks for them among the files it already picks out by name, so a big drop takes no longer.

**A big drop** (the user, 2026-11-16, of their Steam library's 146,409 files in 24,158 folders):

- **The count.** While a dropped folder is read, the reading row counts the folders and files found so far, four times a
  second, and has a Cancel button that stops the walk at its next folder.
- **Every folder is listed.** A game is known by its files, at any depth, so no folder can be passed over before its
  files are seen, and the walk does not try.
- **Markers by name first.** The sort looks for markers only among the files named as some game's marker is
  (`markerNames`), and gives the same result as looking at every file.
- **The sort in steps.** `sortSteps` sorts in steps of about 16 ms, the page drawing between them.
- **The folders with no game,** from four of them on, read as one line, "96 folders hold no game this site plays", with
  their names behind Details.

On a synthetic library of that size (`tools/steamdroptest.js`), the sort took 10.5 to 13 s at once before this and takes
0.16 to 0.18 s now, in steps of at most 18 ms, with the same result. The walk is the rest: read from the disk under Node
(`--disk DIR`, which writes the library under `build/` and deletes it afterwards), the same library's 47,939 listings took
6.9 s, which is what the count and Cancel are for. `unit-web` runs the test at a tenth of the size.

**An installer dropped** (the user, 2026-11-16: "detect when an installer was dragged in and provide explicit direction to
the user against that"). When the sort finds no game in a drop, `installerIn` looks for a game's installer in it: a
program (.exe or .msi) alone or with a few files (twelve files at most, three programs at most), or the only program in a
dropped .zip. A program is an installer when its name holds "setup" or "install" (never an uninstaller, DirectX's setup
or a redistributable, which game folders hold beside the game), or when it carries an installer's mark in its first 4 MB
or last 64 KB: a Windows Installer package's compound-file header, Inno Setup's (its "Inno" at 0x30, its loader's table or
its version text), NSIS's, InstallShield's or Wise's text or section, or a self-extracting archive (7-Zip, RAR, a cabinet,
WinZip's text or a zip's end record). The site never runs or unpacks it. The drop's row then says to install the game
with it first and drop the folder it installs to. Where the installer's name, or its folder's, names a game the site
plays, the row names the game and its usual folder from the game page's import text (`C:\Program Files (x86)\PopCap
Games\...`, or the folder under Steam's `steamapps\common`); an installer dropped on a game's card names that card's game.
Where the name names a game the site does not play, the row says so as it does for that game's folder. The drag target is
unchanged: the files are known only at the drop. `tools/gameimporttest.js` checks the marks on small synthetic files and
that a game folder holding a setup.exe or an uninstaller is still added.

### The Setup window

`web/setup.js` is "byo-pop Setup", a wizard over `web/addgames.js` (the user, 2026-11-09: "add an installer window
where files can be dropped in bulk") with no rule of its own; it parodies an installer without naming or drawing a
real one. A Finish row's "Error 1" means the drop could not be read, "Error 2" that the files could not be copied.
`tools/setuptest.js` (`unit-web`) checks it.

### Coming soon games on a development host

A game in `BYOPOP_WEB_UPCOMING` whose page this build made gets marks on its tile (`data-local-page`,
`data-local-game`, `data-local-persist`, `data-local-lock`), which `web/localsoon.js` turns into a working link on a
development host only (the user, 2026-11-09: "Yes, locally"; `?localsoon=0` keeps them shut there). The site has no
deploy step yet: `build/web` holds every port the build made, the ones still to come too, so whatever copies the site
to a public host must leave those out or build with their `BUILD_<TARGET>` off.

### Caching

Every page the build writes loads the site's scripts and style sheets with their contents' hash in the address
(`name.js?v=<hash>`, `byopop_web_stamp()` in `cmake/ByopopWeb.cmake`): the listing, each game's page and copyright page, and
Jungle Juice's pages. A page loaded after a rebuild therefore never runs an old script. The build also writes
`build/web/_headers`, the cache lifetimes for the site's host (Cloudflare Pages or Workers' static assets, the host
`docs/CHAT_DESIGN.md` plans; it reads the file and does not serve it). Each stamped file gets
`Cache-Control: public, max-age=31536000, immutable`, so it is kept a year and never asked for again. Everything else is
left to the host's default, `public, max-age=0, must-revalidate` with an ETag: asked for again on each visit. That covers
the pages, each game's own script and data (written by its link, unstamped), the music player's decoder (`openmpt.js`),
the picture decoder (`jp2.js`), VexFlow, pictures, fonts and tunes. A rule names its file by its path, which a request's
query does not change. A file is therefore listed only if every page loads it stamped. The build stops if a script of the
site's loads a listed file by its bare name, and if the list passes the host's 100 rules.

### Games this site does not play

A folder that holds none of the site's games but one it knows is named to the player, with a link to another project
that plays the game or "coming soon" (the user, 2026-11-07). The table is `BYOPOP_OTHER_GAMES` in
`cmake/ByopopOthers.cmake`: an id, a title, two marker files only that game has, and the project and its page or
neither; `byopop_web_index()` writes it to `other-games.js` and stops at a malformed row or a page that is not a GitHub
project's. A project is linked only when its front page and README say it plays the game, ships none of its files and
needs the player's own copy; only those two are read, never the code ("No references" in CLAUDE.md).

### The site's look: rules

The rules, each from the user on 2026-11-07:

- **The game stays the game.** What happens inside the game's picture, and what its buttons do, stays as a port the
  player built themselves would have it ("I want the default to not feel modded"). The page around it may change.
- **The era, not the company.** No company history, credits panels or liner notes, and nothing that ties the site to
  today's company.
- **Period looks are affectionate parody.** The site may look fully like a 1995 desktop, drawn in CSS or SVG. It uses
  no Microsoft or InstallShield names, logos, icons or sounds, and never imitates a real system prompt.
- **The player's own fonts.** The site ships no Microsoft font files; after its own font ("Desktop 95") the CSS names
  the system fonts.

### Values, not sentences

The rule for every control surface on the site (the user, 2026-11-08: "It is instantly obvious AI was used. Figure
out some way to make elements that don't look like that"):

1. **No sentences inside a control surface.** A window shows values and standard labels. What needs explaining goes in
   a tooltip, in About (System Information on the listing page), in a "?" message box beside its control, or nowhere.
2. **A recognised icon, alone,** where a widely known one exists, drawn in our own pixels; its name is its tooltip and
   `aria-label`.
3. **Status is data in a display,** not a sentence. A test mode such as `?silent=1` shows a small indicator there.
4. **Views belong to the display.** Clicking it cycles them, with no check boxes.
5. **Credits and explanations go to About** (System Information's General tab on the listing page) and the licences
   page.
6. **Density and hierarchy of real software:** small type, tight spacing, no uniform cards or pill buttons.
7. **Evoke, don't copy.** No real program's artwork, logo, name or skin is copied.

In practice every button is the grey bevelled one, and a destructive one is marked by its confirmation, not a colour.
A Settings window has one "?" in its title bar (What's This?): the next click on a control shows its tip instead of
working it, F1 on a focused control shows the same, and the text is the control's description for screen readers. The
terms keep their full text. Text a player needs in order to act stays as short wizard steps (the save import).

### Desktop 95

Desktop 95 is the site's only look, an affectionate parody of the 1995 desktop (the user, 2026-11-09: "drop the alt
skins, including the plain one"): `web/skins.js` sets `<html data-skin="d95">` and `web/skins.css` holds the look and
the pages' colour variables (`--bg`, `--panel`, `--fg`, `--accent`, `--font-ui`, `--sk-*`). Only the player-only page
has no skin. Its controls are drawn as the 1995 desktop drew them (the user, 2026-11-08: "Rounded corners are very
un-95"), in our own SVG. Where a choice is between a plainer look and the one Windows 95 had, take Windows 95's, even when
it changes every window or box on the site (the user, 2026-11-23: "Always prefer more authentic designs for the 95 look",
on giving every message box's default button its black frame).

**The skins** (docs/SKINS.md) are the versions of Windows the desktop can be drawn as: 95, the base, 98 and 2000. `web/skins.js`
sets `<html data-look="95">` beside `data-skin="d95"`, which stays the desktop's own name. Every other skin draws over 95
with its own style sheet, `web/skin-<id>.css`, its rules under `:root[data-look="<id>"]`; only a page drawn in it loads it,
from the page's `<template id="skin-sheets">` (the listing's and the game pages'), which the build stamps as it does the
page's own files (`lookSheet`). A page without the template, a copyright page or Jungle Juice's, is drawn in 95 and says
so (`data-look="95"`), so `web/schemes.js` gives it 95's colours. The player
picks a skin with the setting `look`, the Skin list at the top of Appearance, and the choice redraws every page of the site
open in the browser (the setting's event, and `storage` in the others). A test picks a skin with `?skin=<id>`,
which the address keeps (`testQuery`) and the pages in the desktop's windows are opened with (`web/deskapps.js`,
`frameUrl`); an unknown one is 95. What a skin sets is in
`:root[data-skin="d95"]`'s variables at the top of `web/skins.css`: the colours (`--panel`, `--sk-*`; a 3D edge's darkest
row is `--sk-dk` and the black of glyphs and the focus rectangle `--sk-ink`, both black in 95), the sizes
(`--sk-title-h`, the caption buttons' `--sk-cap-w` and `--sk-cap-h`, a dialog button's `--sk-btn-w` and `--sk-btn-h`, the
task bar's `--sk-task-h`, the menus' entries `--sk-item-h`, `--sk-task-item-h`, `--sk-pop-item-h` and `--sk-sub-item-h`),
the title bar's background (`--sk-title-bg`, 95's one colour; from 98 on the gradient to the scheme's second colour,
`--sk-title-2`), an inactive one's (`--sk-title-in-bg` and `--sk-title-in-fg`, a gradient from 2000 on), the Pop menu's
band (`--sk-band`, `--sk-band-w`, `--sk-band-fg1`, `--sk-band-fg`) and the font (`--font-ui`). A skin whose banner was a
16-colour bitmap gives it as `--sk-band-ramp`: its height, its colours and where each row stands among them. `web/skins.js`
(`bandRamp`) draws that as our own picture, dithered 4 by 4 as such a bitmap was, at the band's foot, with `--sk-band`, the
bitmap's top colour, above it. A skin whose version had other default colours has its own standard scheme in
`web/schemes.js` (`STANDARDS`: 2000's Windows Standard), which "standard" means under it, and a scheme of ours keeps its
colours under every skin, with the skin's dark edge. The SVG
drawings take a skin's colours as they take a scheme's (`web/schemes.js`, `uriRules`: the face, shadow and soft light of
95's drawings swapped for the target's, and 95's black for the skin's dark edge where it is a drawing's edge, its first
rectangle, as large as the drawing); a drawing whose shape differs in a version is that skin's own rule. A drawing's rule
gives its colour in a declaration of its own, never beside the drawing in one shorthand with a `var()`, which hides the
drawing from that walk. What a version did beyond its drawing is the skin's behaviours, `<html data-behaviours>`
(`web/skins.js`, `BEHAVIOURS`), each with its setting: 2000's "cues", the focus rectangles hidden until a key shows
keyboard use (`cuesSync`, the setting `hideCues`), and "personal", the Pop menu's Games list showing the games played
lately first with an arrow for the rest (`fillSub`, the setting `personalMenus`, set in the Properties box at the foot of
the task bar's menu, `barProps`). A settings row that only some skins have names the behaviour it needs (`needs` in
`web/settings.js`) and shows only under them. A message box's button marked `def` is its default, focused first; Escape
takes the last either way.

**The logo** stands at the desktop's bottom right, above the task bar, under every icon and window (the user, 2026-11-23:
"move the byo-pop logo to the bottom right"), where the icons' columns, filled from the top left, do not reach it at 800 by
600, 1024 by 768, wider or narrow. It is still a drop target for game folders. The windows' first places start 12 pixels
under the desktop's top edge (`top()` in `web/index.html`). **Title bars and the Pop band** are Windows 95's (item 10,
2026-11-23): the active title bar in one colour, not Windows 98's gradient (a scheme's second title colour is kept in
`web/schemes.js` for a 98 look), and the Pop menu's band 21 pixels wide in the shadow grey, the name's first part up to its
last hyphen in the face grey and the rest in white, as a real Windows 95 screenshot shows the Start menu's banner.
**A dialog's buttons** are Windows 95's: in the message boxes, the Splits dialogs, a game's Properties, Setup and the save
import, a row's buttons are 75 by 23 pixels. The button Enter takes, which is the focused push button or else the dialog's
default (the class `d95-def`: a box's last button, Setup's and the import's Next; or a page's `primary`), has the black
frame at its edge, so the frame moves with the focus from button to button and comes back to the default when the focus
goes to a field; Enter on anything but a button, a link, a list box or a text area presses Setup's and the import's Next.
**The focus** shows its dotted rectangle on the focused control from the first click on, as Windows 95 did, with no wait
for a key (hiding it until a key came with Windows 2000): a button, a list box, a check box, a link, a list's item, an
icon's name, and the Pop button while its menu is open. A text field shows only its caret, as Windows 95's edit boxes did;
a menu's item shows its highlight alone; a title bar's buttons and the tray's speaker, which Windows never focused, show
it only for a key.

**Windows.** A page is a desktop (`ByopopSkin.desk`) whose game, chat and panels are windows, added with
`ByopopSkin.desk.add(element, {id, title, ...})`. The desktop never scrolls, and F6 moves between its panes. Alt+\`
switches windows as Windows' Alt+Tab did, since the browser keeps Alt+Tab (the user, 2026-11-13): held down, each \`
steps through the open windows, the one used last first, in a box of their desktop icons with the chosen one's title
(Shift+\` steps back, Escape lets go), and letting go of Alt brings that window to the front, restored if it was
minimized; a quick Alt+\` goes back to the window before. From a window's page on the listing's desktop the keys go to
that desktop (`ByopopApps.altTab`), so the game's canvas never sees them. Alt+4 closes the window in front, standing in
for the Alt+F4 the browser keeps (the user, 2026-11-14), and with no window open opens the Pop menu on Shut Down; it goes
to the desktop the same way (`ByopopApps.closeFront`), and leaves AltGr+4 (Ctrl+Alt on Windows) and text fields alone,
where it types a currency or cent sign. On Linux, Chrome and Firefox switch to the fourth tab on Alt+4; the page calls
`preventDefault`, which has not been tried there, so the switch may still happen. **Shut Down** (the Pop menu's last
entry) closes every window as its Close does, a game's with its saves, and covers the page with a wordless black last
screen until a key or a click brings the desktop back with its windows closed; on a game's own page the game closes and the
listing shows it (`byopop-desk-open` "shutdown"), and from a window's page the desktop around it shuts down.

**As in 1995** (the user, 2026-11-14, by the coordinator's list):
- **Moving and sizing:** a window is moved and sized by a dotted outline that inverts what it crosses, and goes there when
  let go (`showGhost`, `settle`).
- **The control menu:** every title bar has its icon, which opens it, and a right click on the title bar or the window's
  task bar button opens it too. It holds Restore, Move, Size, Minimize, Maximize and Close, those that do nothing greyed.
  Move and Size work from the arrow keys (Size's first arrow picks the edge), with Enter or a click to place and Escape to
  cancel (`keyMove`).
- **Caption buttons for a finger:** on a screen worked by touch (`pointer: coarse`), Minimize, Maximize, Close and "?" are drawn
  at twice their size, glyphs and all, as a larger caption button size in Windows 95's Appearance settings drew them. Each is
  then a target of at least 24 pixels, as Lighthouse asks. A mouse keeps the 16 by 14 buttons.
- **Minimize and restore:** a caption flies between the window and its task bar button (`fly`). There is none with
  reduced motion, and a timer shows the window even where no animation runs.
- **The icons:** they stand in columns until one is dragged, then each stays where it was put (`byopop-desk-icons`, by
  page). A drag on the empty desktop selects them with a dotted rectangle, and a drag of a selected icon moves them all.
- **The empty desktop's right click:** Arrange Icons (back to the columns), Line up Icons (each to the nearest free
  place) and Properties (Settings' Appearance).
- **The task bar's right click:** Cascade, Tile Horizontally, Tile Vertically and Minimize All Windows. Each of them first
  keeps every window's place, size, state and order (`keepLayout`), and the menu then offers its Undo: Undo Cascade or
  Undo Tile under the three, and Undo Minimize All in Minimize All's place (the user, 2026-11-18: "I accidentally
  horizontally tiled my windows, losing my layout. It is frustrating to put it back. I should be able to restore how my
  windows were previously laid out."). Undo puts each window back as it was, minimized or not, at its own size or its
  style sheet's, in its old order, with the window that was in front in front again (`undoLayout`). A window closed in
  the meantime stays closed, and a maximized window is never cascaded or tiled, so it stays as it was. One undo is kept,
  the last arrangement's, until it is used. Undo Minimize All is offered while a window it minimized still is. A
  window's least size stands in a tile, so on a small screen the tiles of large windows overlap, as 1995's did.
- **Pictures** (the user, 2026-11-21: "The \"Pop\" menu is messed up, and could use icons"): a window has one picture at two
  sizes, its desktop icon's (`picOf`). The Pop menu's first level shows it large, beside the band, and the lists beside it
  show it small, as do the window's title bar and its task bar button; a game's list entry shows the small icon read from
  its exe, else its cap. Every window that is not a game's is one of the site's apps and has the app's own icon (the user,
  2026-11-22: "Give the other apps on the site better icons than just the window display"; "The site's apps' icons"
  below), the same in each of those places and in Alt+Tab's box, its name in the tooltip as before; a window of no app
  is drawn as a little window. Entries that are no window draw their own (`drawnPic`: the games' folder, full screen,
  the game's size, Shut Down's computer), and a game page's Music, Sounds and Library show those apps' icons. A screen
  too short for the large pictures gets the small ones on the first level too, and a list opens level with its entry, raised only as far as the task bar needs. The
  Pop button's mark is a yellow burst of our own, and the active window's task bar button is pressed with bold text
  (struck twice, as the desktop's one-weight font draws every bold). Every picture lands on whole pixels of the screen,
  at 1x and 2x: the task bar's buttons are 160 wide while they fit and share the room out in whole pixels when they do
  not (`fitTasks`), and a title bar of its own height (the Library's) leaves its icon whole pixels from its edge.
- **Tooltips:** drawn as 1995's, a pale yellow box with a black border half a second after the pointer rests. The
  browser draws its own for any title under the pointer, so the titles of the element and of its titled ancestors are held
  off them until the pointer leaves the element: moving onto a child of it (a Sounds button's label), a press, a key and
  the box's six seconds take only the box away. A title the page gives a held element (Music's Play becoming Pause) is
  held and shown too, and one given to the element under the pointer starts its box. An element named only by its title
  keeps that name. A game window's frame has none, since the pointer's movements in it never reach the desktop's page (the
  user, 2026-11-16, of native tooltips mixed in with Desktop 95's, and of a stray "Bejeweled 2" over a game's board).
- **The tray's speaker:** it opens a Volume slider with Mute (`ByopopVolume`: `byopop-master-volume`,
  `byopop-master-mute`). Everything the site plays follows it, each sound at its own level times this one: the games,
  through a gain the game's page puts after its output node (`siteVolume` in `web/shell.html`, which the hush in
  `web/deskapps.js` then takes off the speakers), plus Music, Sounds and a theme's sounds.
- **The Recycle Bin's icon,** on the listing: it opens the bin's panel in the Library, and a game's row dragged out of
  the Library and dropped on it asks there to forget that game's files (Uninstall, in the Library's and a shortcut's
  menus, does the same through the same code: "The Library", below). It is full while the bin holds the names of the games forgotten (`byopop-bin`), until Empty Recycle
  Bin, from its menu, clears them. A favourite's desktop shortcut dragged onto it lights it, and dropped there it is taken
  out of the favourites, as its menu's Remove from favourites does. The game's files and its place in the listing stay,
  and the bin does not fill. As with Windows 95's drag to the bin, nothing asks first and no sound plays. Any other icon
  dragged with the shortcuts goes back where it was.
- **Installing by a drop on the desktop:** files dragged over the desktop's own area dim it inside a dashed edge. A box
  under the pointer, with an installer's floppy disk, says a drop installs the game (`dropZone` in `web/addgames.js`). A
  drag tells only that files are coming, so the words fit any, and Setup sorts what is dropped. Windows, the task bar and
  the logo keep their own drops, and the target goes when the pointer leaves the desktop, on a drop, on Escape, or when
  the drag ends. A drop there opens Setup with the files, as before. Each game that run installs becomes a favourite with
  a desktop shortcut: the first at the drop point, the others in the next free places (`iconPlaceAt` and `addShortcuts`
  in `web/skins.js`). Nothing is added for a game that failed, was skipped, was already here, or already has a
  shortcut, which stays where it is.
- **Icon names** are cut to two lines with an ellipsis, as Windows 95 draws them, which the grid has room for. The whole
  name shows on the icon with the focus, or the one icon selected, drawn over its neighbours.
- **The theme's events** now include start, exit, empty and error. Start comes at the first press or key of the browser's
  session on the listing, since a page may play nothing before that, and again when Shut Down's last screen goes. Exit is
  Shut Down, and empty is Empty Recycle Bin. Error is a game stopped by an error and an import refused, on either page.
- **The player's own event sounds:** Settings' Sound section on the listing lists the events, each with the file kept for
  it, Browse, a test press and Remove, plus Add files and a drop for many at once. A file goes to its event by its name, as
  a sound scheme's files were named (`OWN_NAMES` in `web/themes.js`: "Windows Minimize.wav", recycle.wav, tada.wav), and
  a name that matches none is listed for Browse. The files stay in this browser, in IndexedDB (`byopop-own-sounds`, at
  most 4 MB each), and nothing is uploaded or shipped. An event with its own sound plays it whatever the theme; every open
  page reads them again when `byopop-own-sounds-at` changes.
- **Copying a game's files** shows the file-copy dialog (`ByopopCopy`), on either page's import: our own drawing of a
  page flying between two folders, the file being copied, its size and a progress bar. A game page inside the listing
  shows it on the listing's desktop. Without the desktop the pages' own messages say how the copy goes.
- **The readme:** the import keeps the game's own `readme.html` or `readme.txt` from beside its exe, and the game window's
  Help menu, named by that file, opens it as text in a text window. A copy imported before this has none until it is
  imported again. The address does not keep the window.
- **A game stopped by an error** turns its window blue (`blueScreen` in `web/shell.html`), with only the engine's own words
  for the error and the WebAssembly offset of the innermost frame of the game's code. The stopped card comes at the next key
  or click. A game that stops before its screen is made shows the card at once.

The game
window's menu bar spreads the game's commands over Game, View, Options and Help (the user, 2026-11-09: "don't hide
everything under "Game""), and no menu shows Alt+F4 or Alt+Space, which the browser keeps. Game holds the game's files' two actions
itself, Import game files again... and Forget game files... (the user, 2026-11-21: ""Game files" under Game is just two
buttons that could be moved directly into the tab"), and their questions come in the desktop's message box
(`ByopopApps.ask`) with Cancel; the Game files window is in the menu only while files were missing, for their list. The game is always at a
whole multiple of its size in device pixels (`sizeStage`). A multiple chosen in the View menu is drawn only while the desktop
has room for its window; otherwise the game is drawn at the largest multiple that fits, or at 1x with the window scrolling
when not even that fits, and the choice is kept, so the game goes back to it when there is room (`embedWidth`,
`scaleInUse`). The window is fitted again whenever the multiple drawn changes. The desktop tells each game's page when its
room changes (`web/deskapps.js`, `roomChanged`: the browser window's size, full screen, the desktop's zoom, the task bar's
height), since a window that keeps its size gives its page no resize. The status bar gives the multiple drawn; the View menu
ticks the one chosen and greys those without room. The game is centred by its margins, so that a game larger than its
window starts at the top left and scrolls instead of being cut off (the user, 2026-11-20: "Ning Po window is cut off"). The
window takes the status bar's height rounded up
(`statusBarHeight`), and the canvas is focused with `preventScroll` everywhere, so that the game's area is never a
fraction short and a press on the game never scrolls it (the user's note of 2026-11-11). The framework offers the
game's own full-screen request to the page (`sexyapp_page_fullscreen` in `framework/SexyAppBase.cpp`), because SDL's
full screen would stretch the canvas.

**Resizes** are settled on every page, in the first lines of `web/skins.js` (checked by `tools/resizetest.js`). A window
that changes size step by step sends a resize event for each step: Firefox's full-screen transition, F11 in it, a browser
window dragged larger. Each step had SDL size the canvas's pixels again and the page and the desktop lay themselves out
again, so the page lagged and the sound crackled (the user, 2026-11-17: "The site lags a lot and causes audio crackling
because it tries to resize repeatedly when Firefox enters full screen"). Now the first event of a run goes on at once and
the rest are held, by a listener capturing at the window, until none has come for 120 ms; then one resize goes on for the
size reached. The page's own resizes, which its scripts send so that SDL measures the stage again, go on at once.

**Loading** (the user, 2026-11-20: "The site briefly shows the all games window while loading. Replace that load with
something 95-themed"). A page that becomes a desktop, the listing or a game's own page, used to be drawn in stages: the
games' list as a plain page, then the desktop, then each window in its saved place. Now, from the first line of
`web/skins.js` until the desktop is whole, `html.d95-booting` covers the page with the desktop's colour and the hourglass,
and in the middle a raised box says "Please wait while the desktop loads..." over a sunken strip of blue blocks, as
1995's Setup showed its progress. The strip fills as the page's scripts and style sheets arrive, measured against those
the page has named so far, and it never goes back. The cover is the root's two pseudo-elements, there before the body is,
and it covers the page instead of hiding it, so the page's scripts lay it out and give the focus as they always did. It goes
once the page has loaded with its fonts and, on the listing, every window the one page puts back has its page and a game's
window its game's size (`bootHold("apps")` and `bootDone` from `web/deskapps.js` `start`; a game's size waits at most 3 s
after its page). In any case it goes 15 s after the page's text was read, so that a file that never comes cannot keep the
desktop away. A page in a window, the player-only page, the screen saver's page and a game page too narrow for windows
never show it. Checked through a proxy that served one file at a time with a delay: on a cold first visit, a reload, a
reload with a game window open and a game's own page, the cover held until the end of the load, and no window moved or
changed size after it went.

**What the listing loads, and when** (Lighthouse's report, 2026-11-23). The listing's head holds only what the cover needs:
the site's text (`strings.js`), the skin (`skins.css`, `skins.js`), the scheme (`schemes.js`) and a preload of Byopop
Sans. Everything else comes in the body, each script just before the first that uses it: the terms before the Coming
soon marks, the cursors and the desktop themes before the Settings, and the logo's drawing deferred to the page's end.
Under the cover the page is the plain listing until the desktop turns on, and then everything moves at once. Unseen, it
is still laid out as before (`html.d95-booting:not(.d95-desk) body { visibility: hidden }` in `web/index.html`), and a
browser no longer counts that move as a layout shift. A measured load had counted it as 0.28. While the box is up nothing
under it has a transition, so the page shows at once when the desktop turns on. Where less motion is asked for, every
property has a 0.01 ms transition, and each element's first instant still hidden had kept the Library from its first focus. The windows that are closed
as the desktop starts and have scripts of their own load those scripts as they first open: Sounds with its table of the
games' sounds, Trophies, Fonts, Splits with its autosplitters, and the chat. That is about 670 KB the page no longer
waits for. Each is an empty `section` with its icon until then (`web/index.html`, `lazyWin`; `o.lazy` in
`ByopopSkin.desk.add`), and its script builds into it (`fill(el)`). While a script loads, the pointer is Windows 95's
"working in background" arrow with its small hourglass, and the window opens once it is filled. A window the last visit
left open is filled as the desktop loads, and the cover waits for it. A script that cannot load leaves its window closed
with Windows 95's message box. `ByopopLazy.need(group)` loads a group's scripts by the addresses the build stamped in the
page's `<template id="lazy-scripts">`. `desk.openWin` returns a promise that settles once the window is open, and
`desk.ready(id)` fills a window without opening it (a driver's way in: `tools/web/splitscheck.js`). Desktop themes load the
Sounds script for its decoders when the browser cannot decode a game's sound itself. Music's script stays in the page:
the themes and the other windows read games' files through it.

**Window sizes** (the user, 2026-11-18: "A lot of windows ought not to be resizable, like settings and anything accessed
through it. Alternatively, a maximum height/width that ensures they display all their content without pointless
whitespace. Some windows should have a minimum size too, like the library. Evaluate EVERY window to figure out
minimum/maximum bounds for proper display."). Each window is either a dialog or sizable. A dialog (`fixed` in
`ByopopSkin.desk.add`) is drawn at its contents' own size, as its style sheet lays it out. It has no edges to drag, and
Size and Maximize are grey in its control menu. It keeps no size, and Cascade and Tile move it without sizing it. A
sizable window is dragged by its edges between a least and a most. Its least is 200 by 120 pixels, or more where its
layout needs it (`minWidth`, `minHeight`). Its most is the room above the task bar, and in height it is also its
contents' own height at the width it has (`tallest`, measured with the height unset), so no size leaves empty space under
them. The game's own window and the one page's frames are left out of that measure, since a frame has no height of its
own. A most never goes under the least, and a least larger than the screen gives way to it. Each window's bounds were
measured in the Browser pane at 800 by 600, from the narrowest width before a part spills out of the window or wraps
badly:

| Window | Kind | Least | Most | Why |
|---|---|---|---|---|
| Library | sizable | 560 × 200 | the room | its bar on one row, its columns, five rows |
| Music | sizable | 360 × its player and three rows | the room | its controls on one row |
| Sounds | sizable | 300 × 200 | the room | its bar on one row, four rows of sounds |
| Splits | sizable | 240 × 180 | the room | its menu bar on one row, the timer and its two lines under it |
| Trophy Cabinet | sizable | 300 × 200 | the room | its player list on one row (an empty cabinet was all there was to see) |
| Fonts | sizable | 480 × 300 | the room | its game, font and Save As on one row, and a row of the sample sheet |
| Gallery | sizable | 420 × 290 | the room | its game list, Venice's picture and Save As on one row, and a building's height of the well at 25% |
| Guides | sizable | 320 × 240 | the room | its game list, the banner and a few lines of the guide |
| Chat (both pages) | sizable | 260 × 320 | the room | its controls and five lines of messages |
| A game in a window | sizable | its game at the multiple drawn, its status bar and frame | the room, and Maximize | `web/deskapps.js` |
| A text in a window | sizable | 200 × 120 | the room | |
| A game's own window (game page) | its sizes | each whole multiple that fits, from its grip and control menu | | `sizes` |
| Settings (the listing's folder, the game page's panel) | dialog | 600 or 440 wide, its icons' height | | |
| Each Settings section | dialog | 440 wide, its rows' height | | |
| Setup (Add Games) | dialog | 560 × 400 | | a wizard, one size for every step; its list scrolls |
| System Information (listing page) | dialog | 420 wide, its tallest tab's height | | the footer, `web/sysinfo.js` |
| About (game page) | dialog | 520 wide, its text's height | | |
| Mods, Game files, Controls | dialog | 440 wide, their rows' height | | |
| Performance | dialog | its graphs' size | | |

A dialog taller than the room is cut to it, and its contents scroll. A message box was a dialog already.

**Its text** is Byopop Sans, `web/fonts/byopop-sans.woff`, built by `tools/web/pixelfont.py` from W95FA by Alina Sava
(SIL OFL 1.1, as is ours); after a change to the script, run it and commit the file. The font is sharp only at 11
pixels and its multiples, so every text is set at 11 (a few at 22), and a new part sets no size of its own. A glyph's
body has one pixel of bearing on its left, as W95FA's have but for y, ý and ÿ, which the script moves right a pixel (the
user approved it, 2026-11-11, of "ry" touching). The 2000 skin's text is Byopop Sans 2000, `web/fonts/byopop-sans-2000.woff`,
drawn by the project in Tahoma's manner on the same grid and built by `tools/web/pixelfont2000.py` (which writes the file
through pixelfont.py's `font_bytes`); `web/skin-2000.css` puts it first in `--font-ui`, Byopop Sans after it for the
characters it lacks.

**Its zoom.** One pixel of the font is one of the screen's only where `devicePixelRatio` is whole, so a desktop page zooms
(`zoom` on `<html>`, set by `web/skins.js`) by that ratio rounded over the ratio: at 125% it is drawn as at 100%, at 150% as
at 200% (the user, 2026-11-11: "Zoom desktop to a whole-number scale"). A page inside one of its windows finds the
rounded ratio as its own and zooms by 1. Under the zoom a pointer's position, `getBoundingClientRect` and the window's
`clientWidth` and `innerWidth` are the screen's CSS pixels while a style's are the page's, so code that turns the one into
the other divides by `ByopopSkin.zoom()` (`unzoom`, `viewW`, `viewH`), a canvas drawn at the screen's pixels uses
`ByopopSkin.ratio()`, and the style sheets' viewport units are divided by `--d95-z` (`calc(100vw / var(--d95-z, 1))`;
a new rule with `vw` or `vh` does the same). `?deskzoom=<z>` sets the zoom for a test at any scale.

**Colour schemes.** `scheme` picks one of nine schemes of our own named after soda flavours, "Standard" being the
original look (the user, 2026-11-11, of the proposal's schemes and patterns: "Yes, 1 then 2"). `web/schemes.js` sets
the scheme as variables on `<html>` (`--bg`, `--panel`, `--sk-lo`, `--sk-mid`, `--accent`, `--sk-title`, `--sk-band`):
a new rule in `web/skins.css` uses them, never the standard greys or blues as literals, and the SVG pictures in data
addresses, which cannot read a variable, are written again for each scheme (`uriRules`). `tools/schemetest.js`
(`unit-web`) checks every scheme.

**Pattern and wallpaper** (`pattern`, `wallpaper`, `wallpaperFit`, also in `web/schemes.js`; "My picture" stays in
`byopop-wallpaper`, never uploaded). A pattern's pixel is a whole number of screen pixels (`devicePixelRatio` rounded),
laid on 1:1, so it is crisp at every scaling (the user, 2026-11-11: "The background patterns don't work well with high
DPI. I can hardly see any of them on my 4K, 27" display"); `--d95-dot` is the same size, for the skin's checkerboard
fills.

**A game's background as the wallpaper** (the user, 2026-11-18: "Still would like usable backgrounds from the games
either as pixel perfect or fit to screen (no stretching)"). Before this, a game's background could be the wallpaper only
through its desktop theme, which also changes the cursor and the sounds, and only centred, tiled or stretched. Now the
Wallpaper list also has each game whose files are here, by its title (`game:<id>`): the background its theme names,
prepared on the listing page like the theme's (`web/themes.js`, `prepareWall`) into `byopop-game-wallpaper`
`{id, url, w, h}`. The picture is kept at its own size up to 2048 pixels wide, as a PNG when it had no loss (a PNG or GIF
file, a Retro64 bank's image) and else as a JPEG, and it is made smaller only when it would not fit the browser's
storage. Display has two more choices, for any picture (ours, the player's own, a game's, a theme's):
**Pixel-perfect** draws the picture at the largest whole-number scale that fits the browser's window (its own size when
even that does not fit), centred with its corner on a whole screen pixel and without smoothing, so that each of its
pixels is a square of whole screen pixels at any `devicePixelRatio` and desktop zoom (`pictureLayer` in
`web/schemes.js`, placed again when the window changes size). **Fit to screen** draws it as large as fits with its shape
kept (`contain`), the desktop's colour around it.

### Desktop 95 in one page

The whole site is one page, the listing's desktop (`web/deskapps.js`, `ByopopApps`). A game, its copyright page and
Jungle Juice each open there as a window holding the page in a frame with `?embed=1` (the page then draws no desktop of
its own); a licence text is fetched into a window. A link to one of our pages opens its window; a link to another site
opens a tab.

- **The address bar shows only the site's root** (the user, 2026-11-09: "What we're developing now is a SPA, and the
  url bar should never need to show any path other than root"). The open windows live in `history.state`
  (`{d95: true, open: [...], active}`): opening a window pushes an entry, bringing one forward or closing one replaces
  it, so Back and Forward close and reopen windows, and a reload opens them again. A page of ours opened in its own tab
  goes to the root with `?open=<page>` (`onePage` in `web/skins.js`), which opens its window and is taken off again.
- **Test parameters stay.** `?silent=1` and `?assets=` stay on the root and pass to every frame. A game's own page
  opened with `?silent=1`, `?player=1` or `?assets=` stays that page, as the drivers open it.
- **Several games run at once,** each in its own frame with its own module and saves, as many as the player opens past
  a warning (below). The window in front has the focus, and the other games' frames get the window blur the framework
  listens for (`EmscriptenFocusCallback`), so each behaves as a page that lost the focus.
- **The chat is one window for every game** ("The chat"), a message tagged with the game in front.
- **Closing a game** returns to the listing with the Library open and in front (`byopop-desk-open`), as a game page's
  Pop menu's Library (`./#library`) does.

**How many games at once.** Up to 2026-11-16 the desktop opened at most three games, and one on a narrow screen; a
fourth got a message box naming the limit. Asked whether the cap should become a warning past a memory budget, the user
answered "Make it a warning" (2026-11-16). Every game window is a frame of the one page, so all of them share one browser
process. When it runs out of memory, Chrome puts its out-of-memory error in place of the whole page and an iPad reloads
it, so every open game closes at once and loses what it had not saved. A cap prevents that, but it also stops players
whose machines have room: Valve's survey of October 2026 has 16 GB as the most common amount, at 41%, then 32 GB at 37%
and 8 GB at 7.5%. So each game has a weight, the browser has a budget, and a game that would take the open games past the
budget asks first (`WEIGHTS` and `budget()` in `web/deskapps.js`).

A game's weight is its own memory at a first board in units of 100 MB, rounded, and at least 1. It is read from
`docs/MEMORY.md`, "The web builds", "At a first board": the renderer's private bytes less about 60 MB for the page
itself. Bejeweled Twist (about 400 MB of its own) weighs 4 and Bookworm Adventures 2 (about 190 MB) weighs 2. Every
other measured game comes to 1, Rocket Mania (about 55 MB since its music plays from its segments) among them, and a
game not measured counts 1. A new measurement changes the table by the same rule. The budget is 6. Where the browser
says how much memory the device has (`navigator.deviceMemory`, only in Chromium browsers, which give 8 or, in newer
ones, up to 32 for a large machine), it is 3 at 4 GB and 1 at 2 GB or less. Safari and Firefox say nothing and get 6.

A game that takes the total past the budget brings up Desktop 95's message box, titled Memory: "All the games open here
share the browser's memory. With this one open as well, it may run out. If it does, the browser closes every open game,
and anything not saved is lost." Its buttons are Open anyway and Cancel, and Cancel has the focus and answers Escape.
It asks each time a game would go past the budget, but not when no other game is open, since then there is nothing else
to lose. It never asks while windows are being put back by a reload, Back or Forward, since they were open before. A
game's page that finishes loading behind the box would take the focus into its frame, so the box takes it back. A
narrow screen (under 760 pixels) needs no limit of its own: every window there is maximized, and the task bar switches
between them as it does for the other windows. Its one-game limit was about memory only, so narrow screens now get the
same warning.

**Graphics contexts.** A browser keeps only so many WebGL contexts at once, and Chrome drops the oldest past about 16
in a renderer process, which the desktop page and its frames share. Each game uses one, or two with the 256-colour view
filter (`web/view.js`). It was tested in the Browser pane with Mahjong Escape: Ancient Japan running and 16 more
contexts made in its page. Seventeen games would be past the machine's limit of 10 game processes for tests, and the
page's own contexts count the same as its frames'. The game's context went, its canvas went blank and stayed blank,
and the game ran on unseen, with no way back: its pictures on the graphics card were gone. Now the page listens for the
canvas's `webglcontextlost` (`pictureLost` in `web/shell.html`). The game closes its own way, its saves written first,
and its card says "The game stopped drawing" and offers Play again, which starts it with a new context. A graphics
driver's reset ends the same way.

### The Library

The listing's window of the games (`web/library.js`): a sortable grid evoking a game client of 2004, named for no real
program, built from the listing's cards and `ByopopPlay`. On 2026-11-23 it took in every feature of All games, the
listing's first window of the games, which went (the coordinator's list of 2026-11-22: "Add all the features from All
games to Library and then get rid of the all games window"). A desk saved before then comes back with the Library in
All games' place and size where it had none of its own, open if either was (`RENAMED` and `merged` in `web/skins.js`), and the old sort
(`byopop-listing-sort`) is read once into the Library's own (`byopop-library-sort`).

- **Its bar:** Play, Add games (the Setup window), Show games no longer sold (while some game is) and Filter, which
  finds a game by any word of its title with a letter or two wrong (All games' Find).
- **Its columns:** Name, Status (Installed, Not installed, Files unknown, Coming soon, No longer sold; the user, 2026-11-23, for "Ready" and "Needs your files"), Phase on a
  development host, Plays, Play time, Last played, Year, and Popular, every player's plays, when the site's play
  counter answers (its tooltip says how many play now). A header sorts by its column; Plays, Play time and Popular
  sort most first at the first click. A narrow desktop shows Name, Status, Phase where it is shown, and Play time.
- **Its status bar:** the count, the games Windows 95 games only hides, and the link to Jungle Juice with its line.
- **A row:** double-click or Enter plays the game. A game whose files are here can be dragged to the Recycle Bin's
  icon, and a folder or .zip dropped on a row is that game's files. Its menu has Play, Add files, Uninstall, the
  favourite, Properties, "Why aren't my files here?" where that applies, and the buy links. A game's Properties are a
  property sheet of its values and, below them, a table of each site player's plays, time, first and last day (the
  user, 2026-11-11: playtime per profile).

**Uninstall** (the coordinator's list of 2026-11-22: "an uninstall option with confirmation in the Library and shortcut
context menus, in addition to the recycle bin"), in a game's menu in the Library and in its desktop shortcut's menu,
greyed while the game's files are not here. It asks Windows 95's question in a message box, "Confirm Uninstall" with the
question sign, Yes and No, No the default (Escape and Enter on it), and Yes does what a game dropped on the Recycle
Bin does, through the same code (`ByopopAddGames.uninstall` in `web/addgames.js`): `forgetGame` deletes the game's
folder (`<persist>/game`) under its Web Lock and its screen saver profile (`<persist>-saver`), the record says the
files were forgotten that day, and the game goes into the bin's list (`byopop-bin`). Its saves, mods, replays and
play time stay, as do its favourite and its place in the Library, and the row and the shortcut show it at once. A
game open in another tab is refused with the bin's message. On a game's page, whose shortcuts are the favourites, Yes
hands the game to the listing (`byopop-uninstall` in sessionStorage), which uninstalls it with the Library in front.
The bin itself is unchanged. `tools/web/uninstallcheck.js`, a page driver, uninstalls a stand-in game three ways (the
bin, the Library's menu, a shortcut's menu) and checks that each leaves the same state, that No and Escape change
nothing, the greyed entries and where the focus goes.

**Its contrast** (Lighthouse, 2026-11-22: 8.3 pt text at 4.5:1): each colour moved the least that reaches 4.5:1, the
label and the dim text going lighter. The Filter label and the count, #a0a890 on #4c5844, are #c5cabc (4.50:1); the
status and year of a game that is not ready, #a0a890 on #3e4637, are #acb39d (4.54:1); the selected row, white on
#958831, is on #82772b (4.54:1), and the sort arrows keep #958831. The inactive title bar, #c0c0c0 on #808080, stays
1995's. `ByopopPlay` (`web/settings.js`) keeps them in `byopop-play`, by the
current player's id in `by` beside the totals, and the save backup keeps and merges them (`web/saves.js`, `mergedPlay`).

### Desktop themes

Settings > Appearance > Theme (`web/themes.js`, its table `web/themedata.js`; the user, 2026-11-11) gives the desktop
one game's look, as a 1995 Desktop Themes pack did: one of the game's own backgrounds as the wallpaper (in place of the
pattern and wallpaper, at the wallpaper's Display, the new Pixel-perfect and Fit to screen too), the game's own cursor
set (`web/cursors.js`, once its page has kept it), and sounds from its soundboard for the window events that
`web/skins.js` and `web/deskapps.js` send as `byopop-desk-event` (open, close, minimize, restore, maximize, menu, ding).
A theme is listed for each game in the table whose files are in this browser, by the game's title. Everything comes from
the player's own files at run time: the listing page, where they can be read, prepares the chosen theme into
`byopop-theme-wallpaper` (a data URL with its size) and `byopop-theme-sounds` (the files' bytes, each at most 96 KB) for
every page; None, or the game's files forgotten, removes both. The sounds play at the Sounds window's volume and mute
and never at `?silent=1`.

A wallpaper the browser cannot decode as it is goes through `web/gamepics.js`: an image of a Retro64 bank (Cosmic Bugs,
Water Bugs; the table's `"<bank>.dat|bbk:<name>"`), raw pixels that the `.bbk` index beside it places and gives a palette;
and a JPEG 2000 picture (Bejeweled Twist's `.jp2`), by the framework's own decoder (`platform/openjpeg/JP2Decoder.cpp`) built
on its own as `jp2.js` and `jp2.wasm` (`web/jp2/jp2_web.cpp`, about 250 KB), loaded only when such a theme is prepared.
Both are tested by `tools/gamepicstest.js` (unit-web) on pictures made for it. A picture's alpha mask is not read: each
wallpaper chosen has none.

### The screen saver

Settings > Screen Saver (`web/saver.js`; the user, 2026-11-11) has the game, None by default, then (Random) once two games
are offered, then each game with a bot whose files are here; the Wait in minutes; and Preview. Only games that can be lost are
offered (the user, 2026-11-19: "Screen saver should only be for games with a losing condition"): a game its bot never loses
would hold one game on the screen for good, so Pixelus, Mummy Maze and the Mahjong Escapes are not. (Random) draws a game at
each start and another, not the same, each time a game ends (the user, 2026-11-19: "pick a random game each time and switch
games upon losing (thus actually *saving* the screen from burn-in)"). When the listing's desktop has had no key, mouse or touch for the
Wait, it goes dark and the game plays itself silently in a frame over it; any key or mouse movement ends it and leaves
everything as it was. Input in the desktop's windows counts too: each window's frame keeps its events to itself, so the saver
listens in every frame's page as it loads, and a game playing in the active window (`ByopopEmbed.running`) holds it off
with no input at all (the user, 2026-11-17: "Screensaver activated while I was playing Typer Shark"). The frame is the game's player-only page at `?saver=1`.

**The saver's own profile** (the user, 2026-11-18: "Allow the screensaver to have its own profile totally separate from the
user's saves and not made directly accessible to the player"). Of the player's, the frame reads the game's files alone: it
opens the game's database only to read the entries under its `game` folder, into the page's memory. Its saves, mod folder and
replays are in a database of its own, the game's with `-saver` after its name (`ByopopSaver.PROFILE`), which IDBFS loads and
writes back as it does a game's, so the saver's progress lasts from one run to the next. The frame's Web Lock has the same
ending, so it never waits for the player's own page of the game. Its `localStorage` and `sessionStorage` are stores in
memory, empty at each start (an inline script after `terms.js`, before any script reads them): nothing of the player's
settings, players, records, terms or cursor sets is read, and nothing is written there, the user dir's localStorage backup
included, which the saver's games do without. No listing opens the saver's databases: the Saves window, its export and
import, the listing page's states and the Trophy Cabinet each open a game's own name. Forgetting a game's files, on its page
or in the listing's bin, deletes its saver profile with them; and a run that goes wrong (an error, or no move at all) deletes
it once, before the saver starts that game afresh. A second such run stays on the screen it stopped at.

The saver's games run unmodded (the user, 2026-11-19: "Screensavers shouldn't use whatever mods are installed"): the
frame puts in neither the fixes nor the lite mod and starts the game with `-nomods`.

Its page centres the game on black (`html.saver`), and a key pressed while the game's canvas has the focus ends the saver
too. Only the player's own input counts: the driver's moves are events it makes itself (`isTrusted` false), and before they
were ignored a bot that moves by keys ended the saver as soon as a level began (the user, 2026-11-19: "Mummy Maze
screensaver seems to just exit once it gets into a level; ditto with Noah's Ark").

**Modes and pace** (the user, 2026-11-17: "Screensavers should choose between different modes if applicable to the game",
"Let the bot unlock them", and bots that "play nicer"). `PLAY` in `web/saver.js` gives each game its modes: those its bot
can play, that can be lost, and that a new profile has open, so a locked mode comes into the saver only once the bot's own
play has opened it in the saver's profile, never by a test command. One is drawn at each start: by its `goto` name
(`?begin=<name>`), for a game whose bot has no move on its title or menus, or by the `set mode` its bot reads (`?prep=`), for
one whose bot clicks through its menus itself; a game with a 3D view has 2D or 3D drawn as well (`3d on` or `3d off`). A
choice every player has from the start is drawn too (`choose`: Ning Po's layout, tile set and table, by the `set` its title's
arrows answer; the user: "only plays one setup"), and a first run's screen no bot answers gets the test command that does its
work (`setup`: Venice's name box, `player Saver`). A game left out plays as its bot starts it (`?begin=mode`). `pace` is the updates waited after each move, about a person's speed
(a hundred are about a second), and none for a game whose bot plays in real time, aiming or dodging, where a pause would only
lose. `gap` spaces each move's clicks (`?clickgap=1`, above), for Bonnie's tile clicks. Bonnie's bot clicks through its title,
menus, a new player's name (Saver) and the difficulty itself, so it has no modes drawn. Its game runs its own loop, so its
page reports its frames to the test kit from its own sleep (`bonnies_sleep`), as Ning Po's does. A saver's frame opened with `?assets=` (a test of the saver from a first run, with the staged files) keeps a test
profile of its own, the game's name with `-saver-test`, apart from the game's own tests' `-test`.

**The bot's hook.** The game plays by one call that asks it for its next move. Today that is the test command `move`
(`docs/TEST_COMMANDS.md`, "The bot behind move"), sent through `byopop_test_command` by the page's `?driver=move`; it exists
only on a development host, so the live site lists None alone. A game's bot mod (`docs/MOD_ROADMAP.md`, standard mod 11,
made in its modding phase) answers the same question in a live build through `byopop_bot_move()`: an export taking no
argument and returning the same text `move` prints after `MOVE ` (the move as JSON with its `input` steps in game
coordinates, or `null`), changing nothing in the game. The saver lists a game on the live site once its build exports it,
and plays each answer's `input` as real events, as the driver does.

### The trophy cabinet

The Trophy Cabinet (`web/trophies.js`; the user, 2026-11-11) is a Desktop 95 window on the listing's desktop that gathers each
game's own records from its saves in this browser and shows them, a table for each of the game's lists, under the names the
games give their modes. Its Player drop-down picks a site player (`web/profiles.js`), and then only the rows under that
player's name are shown, compared as the games compare names, without regard to case. It reads the files as the Sounds window
does (`ByopopMusic.files`), each time the window opens, and writes nothing.

Every reader follows the port's own save code, which its entry in `RECORDS` names, and is tested under Node by
`tools/trophiestest.js` (unit-web) on files made for it. A file that is missing, of the wrong size or version, or cut short
gives what it holds or nothing. The rows are shown as the game keeps them, its built-in rows too, in its order. A day's best
appears as the Today place only on the day it was set, by the game's own day count. Times are shown as m:ss, or as h:mm:ss
where they can run past an hour (the Mahjong Escapes, BookWorm Adventures' arena).

| Game | Read from (in the game's user folder) | Tables |
|---|---|---|
| Bejeweled, Big Money, Alchemy, Seven Seas, Noah's Ark, Mummy Maze, Rocket Mania, Bejeweled 2 | `hiscores.dat` (Rocket Mania `Players/`, Bejeweled 2 `users/`): PopCap's checked high-score records | one table a mode, with Today where the game keeps it |
| Atomica | `HS.CFG` | Strategy, Action, with Today |
| Dynomite | `STOMPED.HS`, `ENDLESS.HS`, `FOSSIL.HS`, `TIMETRIAL.HS` | one table a file; Time Trial as m:ss |
| Bejeweled Twist | `users/scores.dat` | Classic, Blitz |
| AstroPop | `users/datahs.usr` | Classic by points, Survival by its time (m:ss) |
| Cosmic Bugs, Water Bugs | `cb.dat` | the eight high scores |
| Typer Shark | `users/user.idx` | the score table and the abyss's |
| Bonnie's Bookstore | `Data/HighScores.dat` | Classic, Action, with the longest word |
| Tip Top | `Preferences.dat`, its first 560 bytes | Regular, Speed, Strategy (the Timed table, which no button reaches, left out); empty places not shown |
| BookWorm | `registry.ini`, the key `Scores` | Classic, Action, Best Word, Longest Word |
| Ning Po | `registry.ini`, the values `HS:<layout>x<rank>` | the played places of each layout, as m:ss |
| Word Harmony | `registry.ini`, the keys `scores\<mode>` | Puzzle, Solitaire, Action |
| Mahjong Escape: Ancient China, Ancient Japan | every player's `<id>.usr` | each layout and escape level someone has won, the players by score with their best times |
| BookWorm Adventures and its Volume 2 | every player's `users/<name>.bwa` | Top Words, Arena (by stage, with its time) |

The registry's games are read from the port's registry file (`userdata/registry.ini`, `framework/SexyAppBase.cpp`), whose
values a small parser decodes (`parseIni`). Where a game builds its list across players (the Mahjong Escapes, BookWorm
Adventures), the cabinet builds it as the game's code does, from every player's file. A Word column appears where a game keeps
a word beside the name (BookWorm, Bonnie's Bookstore, BookWorm Adventures' top words), and a Time column where it keeps a best
time beside the score (the Mahjong Escapes).

Names are read at run time from the player's own files and never written into the site: a Mahjong layout's name is the name
of its `LAYOUTS` resource in the player's own `Resource.dll`. Where a game names something only in a picture, the table gets a
number (Ning Po's layouts, "Layout n"; the Mahjong Escapes' escape levels).

Left out on purpose: BookWorm Adventures' mini-game tables, which the game's scripts name and format; Bejeweled Twist's
challenge progress and per-player bests in `profile.dat`, and AstroPop's per-player bests, which are a player's record rather
than a high-score list; Pixelus, which keeps no score list.

### The Fonts window

The Fonts window (`web/fontview.js`; the user, 2026-11-17: "Add a font viewer to the site with a text generator for the games'
custom fonts") is a Desktop 95 window on the listing's desktop, in the manner of Windows 95's font viewer, closed at first, its
icon after the Trophy Cabinet's. Its Game list holds the games whose files are in this browser, and its Font list that game's
fonts. For the font chosen it shows the font viewer's facts (typeface name, file, point sizes, line height, layers, characters),
then a sample sheet: every character the font draws, in rows that fit the sheet, and the Text box's text at each of its point
sizes. Colour is the text's colour, as a game passes it when it draws; Background is the sheet's (None, a checkerboard, leaves
the saved picture clear); Zoom draws every pixel as a square of whole screen pixels near 100% to 400%; and Layers has a check
box for each tag the font's layers ask for, off at first, since a game turns a tag on only for the draws it is for (Bejeweled
Twist's GLOW and OUTLINE layers, drawn in their own colours). Save As writes the text at the chosen size as a PNG, at the
font's own pixels. The window reads the files as the Trophy Cabinet does (`ByopopMusic.files`), with a `main.pak`'s files
(`Module.byopop.pak`) after the folder's own; it decodes the pictures in the page and keeps them only while it is open; it
writes nothing.

It reads the SexyAppFramework's ImageFonts as `framework/ImageFont.cpp` does. A descriptor is split into commands by
DescParser's rules (a semicolon ends one outside quotes, a `#` at a line's start is a note) and each runs as
`FontData::HandleCommand` runs it: Define, CreateHorzSpanRectList, SetDefaultPointSize, SetCharMap, CreateLayer and
CreateLayerFrom, and every Layer command, with the framework's own messages where a game would fail to load the font. The older
form, `FontData::LoadLegacy`, is the space's width, the ascent and each character's width, beside a strip picture of the same
name. A layer is active at a point size within its range and with its tags (`GenerateActiveFontLayers`), and a string is laid
out and drawn as `DrawStringEx` does it: each character's layers placed from the ascent and their offsets, the widest layer's
advance taken with its spacing and kerning, and the pieces drawn in their order, colourized by the text's colour times the
layer's multiplier plus its addition, additive where the layer's draw mode is 1. Two things differ from the games: a layer
drawn at a point size other than its own is scaled by nearest pixels (the framework's stretch filters), and a picture with an
alpha channel of its own comes through the browser's decoder, which can round the colour of nearly clear pixels.

A game's fonts are found by their text: a `.txt` that holds CreateLayer, LayerSetImage or LayerSetImageMap is a descriptor,
and one that starts as a width list, with a picture of its name (in `images/` first), is a legacy font. A descriptor whose
picture the files lack is left out (Big Money's `new_bigmoneyfont1` and Typer Shark's `gillsansmtcondensed12`, which the games
never load). A picture is found as `platform/ImageLoader.cpp` finds one: its name with any picture extension (`.jp2` too), its
alpha from `_<name>` or `<name>_` (the mask's low byte), white where there is only a mask. `tools/fontviewtest.js` (unit-web)
checks the reader on fonts made for it, then reads every font of the staged games: 319 fonts in 22 games, 301 descriptors and
18 width lists, Bejeweled Twist's from its `main.pak` too. The other engines' fonts (Retro64's in Cosmic Bugs, Water Bugs and
Venice; Bonnie's; Jolly Bear's in the Mahjong Escapes; Ning Po's, Tip Top's and Word Harmony's) are not read yet, so those
games' Font lists are empty.

### The Gallery

The Gallery (`web/gallery.js`; the user, 2026-11-22: "Rename Venice Buildings to Gallery; we'll collect other interesting
graphics there for each game once they're all done. In particular, I'd like to show off animations and other things you
can't see just by looking at the image files directly") is a Desktop 95 window on the listing's desktop, closed at first, its
icon (a framed picture) after the Fonts window's, there only where the listing has a game with an entry. Its Game list, named
as the listing names the games, holds those games, and under it the chosen game's entry draws its own controls, picture and
status line; an entry is a module whose `panel(game)` makes the panel, listed in the Gallery's `ENTRIES`. A window that was
Venice Buildings on a saved desktop (the id `venicebuildings`, open or closed, placed, its icon moved) comes back as the
Gallery (`RENAMED` in `web/skins.js`). Other games' entries wait until the ports are done and the user has ruled on a survey
of what to show.

One entry so far, Venice's buildings (`web/buildings.js`; the user, 2026-11-18: "I'd love to see a combined image of all of
Venice's buildings as a continuous shot, if something like that is possible, and full images of the individual buildings of
Venice"). It reads Venice's files as the Fonts window does, its status line "No game files" when they are not in this
browser. Its Picture list holds All the buildings, Building 1 to Building 8 in the order Journey builds them, and the Trophy; Background
puts the level select's scene behind them or leaves them on their own (None, clear in the saved picture); Zoom is 25% to
200%; and Save As writes the picture shown at its own pixels as a PNG (`venice-buildings.png`, `venice-building-3.png`,
`venice-trophy.png`). The whole row is 3280 by 600 pixels with its scene and 3255 by 434 without; a building is 400 by 600
in its scene and its own 400 by 400 picture without.

Each building is one picture in the files, the thumbnail its levels name (`gfx/levelselect/thumbnail1.png` to
`thumbnail8.png`, the win sequence's `thumbnail9.png` the trophy), so a building whole is that picture. The level select
shows them in a row that slides past, never all at once, and the window lays the row out as `LevelSelect::Load` does: down
Journey's list of levels (`config.xml`'s `level_file_relaxed`), a picture at the levels 1, 10, 19 and so on of those with a
thumbnail, each level moving the next place on by a ninth of its picture's width less four, the win sequence's picture 90
further on, and each picture a little lower in depth than the one before, so the earlier building is drawn over the later
where they overlap (36 pixels). The game's order is not the files' numbers: buildings 1, 3, 4, 5, 6, 2, 7 and 8 by file.
Flood's list (`level_file_game` after the plain `level_file` entries, where the tutorial's second part names building 1)
places the trophy one level, 40 pixels, further on; the window follows Journey's.

The scene is `levels/levelselect.xml` read as the engine reads a scene (its tags, each object's stages, depth order as
`SceneObject::InsertChildByZOrder` keeps it) and each object placed as `SceneObject::DrawSelf` and `DrawImageRecord` place it
in 2D, as it stands at the end of its first stage: rounded where it is not bilinear, an unscaled picture's left its place
less half its width and its top one pixel below its place less half its height, a scaled one centred there, tiles stepping
by the scaled size, the water's clip, a sheet's first frame from `config.xml`'s `<image>` entries. Three things differ from
any one screen of the game: both layers sit at their home places, where in play the near one slides twice as far as the
distant city; the tiled rows (the quay and the water) are carried on to the row's ends where they stop short (the quay and the
water's underlay lack 10 pixels at the left); and of the screen's own pictures, which stay put while the row slides (the sky, the birds and
clouds, the scroll and its text), only the sky is kept, repeated along the row with every other copy mirrored so that the
copies meet. `tools/buildingstest.js` (unit-web) checks the readers and the placing on scenes made for it, then lays out the
staged Venice's row: the pictures in the game's order, building 1 at x -9 and y 161, each next 364 on (a place truncated
toward zero), the trophy at 2976, and the quay and the water reaching both ends.

### The Guides window

The Guides window (`web/guides.js`, its texts in `web/guidetexts.js`) holds the site's AI-generated guides, the one exception
to the rule of values, not sentences (the user, 2026-11-18: "I've decided I'm actually ok with AI-generated guides for the
games that actually benefit from it on the site, as long as they are clearly marked as AI-generated"). It is a Desktop 95
window on the listing's desktop in the manner of a help topic, closed at first, its icon after the Gallery's,
with a guide for each game of the listing that has one; it needs none of the game's files. Its Game list chooses the guide,
and the white page shows it under a banner the window draws for every guide, "AI-generated guide" with the date it was checked
against the game's code, and the status line says AI-generated beside the count of tips. A guide is drawn as text only. How a
guide is written, which games get one, where its facts may come from and the markup are in `docs/GUIDE_STYLE.md`, a draft for
the user; `tools/guidestest.js` (unit-web) checks the markup and every guide against the rules there that a program can check.
One guide so far, Mummy Maze's.

### The fixes and lite mods on the site

Every game is served improved (`docs/MOD_ROADMAP.md`, "What a player gets", the user, 2026-11-12): its fixes mod on and
its lite mod on once it has one. The fixes mod (`docs/MOD_ROADMAP.md`, "The fixes mod") is one mod, `fixes`, each of
whose settings toggles one fix, in `games/<game>/mods/fixes`. The lite mod ("The lite mod") is one mod, `lite`, each of
whose settings toggles one faster computation, in `games/<game>/mods/lite`. The site treats the two alike. The web build
copies each beside the page as `<game>-fixes/` and `<game>-lite/` and writes its `version` and file list into the page
(`FIXES` and `LITE` in `web/shell.html`, from `byopop_web_site_mod` in `cmake/ByopopWeb.cmake`); a game in
`BYOPOP_WEB_NO_MODS`, or without the folder, gets none. Before the game can start, the page puts each into
`/persist/mods/fixes` and `/persist/mods/lite`, the fixes mod first, on, its toggles at their defaults, and notes each
in the game's user dir, `site-fixes.json` and `site-lite.json` (`{"installed": "<version>"}` or `{"removed": true}`).
A newer shipped version replaces the mod's files and leaves `mods.json` and `mod-settings.json` alone, so the player's
choices hold. A player who removes a mod never gets it back: a start that finds the note saying "installed" and no
folder notes it as removed. `-DBYOPOP_WEB_TEST_FIXES=<game>` (set back to empty afterwards) gives a game with no fixes
mod `tools/webtest/fixes`, a stand-in with one toggle that changes nothing. The lite mod needs no stand-in: Mahjong
Escape's two games ship one, `quickRedeal`, and their log says `lite: quickRedeal` when the game reads it as on.

**The Original switch** is a check box, Original, in the game's Mods panel (`web/modpanel.js`). It is shown whenever
the game has a `fixes` or `lite` mod. Turning it on turns both mods off through the game's own model, as the mod menu
would, and keeps each one's earlier on/off state in the note as `"original": {"fixes": true, "lite": false}`. Turning it
off puts back exactly those states. The mods' own settings, every fix's toggle in `mod-settings.json`, are never touched,
so they hold through both. The record stays in the note through a new shipped version. While the switch is on, a fixes
or lite mod that arrives later (its first install) is turned off and remembered as on, as it would have started. If the
player turns one of the two back on in the mod menu, they have left the original: the switch goes off and their mods
stay as they are. A removed fixes or lite mod stays removed whatever the switch says, because the switch only turns on
what is there. The record is kept in `site-fixes.json` for both mods, also in a game that has only a lite mod. Like
the note, the switch belongs to one game's user dir and never changes another game. A change while a game runs waits for
its restart, like any change in the mod menu.

### System Information

The listing page's About window became System Information (`web/sysinfo.js`; the user, 2026-11-18: ""About" section has
accumulated cruft. It was originally a footer. I suggest a new window, System Information, that indicates how the site is
performing overall (sort of like the performance panel) and also has that text, maybe in one of the window's tabs. Figure
out a 95-related app to model the design on"). It takes the shape of Windows 95's System Properties, a property sheet of
three tabs, and keeps About's place: the window id `about`, its icon after the others and its line in the Pop menu,
which names it by its title. Its picture is a monitor with a chart on it.

**General** is 1995's General tab: the site's mark at the left where Windows had its logo, and beside it the site
("byo-pop" and its number of games) and the computer as the browser reports it (its processor threads and its screen
in device pixels, read again each time the window comes up). Under an etched line is the page's old footer, cut down:
what the site is, the trademarks, a list of what it is made with (each library or font with its licence and what it is
for), who made it, and the desktop's keys. The tracker's explanation went to the Music window's "?" and the long
sentence of credits became the list. The page's `footer` is this tab, so off the desktop (Desktop 95 turned off) the
page ends with the same text and the window's other parts are not shown.

**Performance** is 1995's "Performance status": the storage the site uses (`navigator.storage.estimate`), how many games
have their files here out of the site's games, and how many game windows are open, then a details list of the running
games with each one's frames a second, frame time and sound gaps (the times its sound queue ran dry, `byopopAudio.gaps`,
since its page opened). Its "Advanced settings" group holds the performance panel's switch, the site setting `perf`.

**Monitor** is Windows 95's System Monitor: four line charts of one running game, chosen in a list, a minute of
readings each, with the scale's top at the left, a dotted line at its half and the last value beside the chart's name.
The charts are frames a second, frame time, the game's share of the main thread and sound gaps a second.

The readings are the game window's own: its performance panel's code (`web/perf.js`) reads them while System
Information watches it (`ByopopPerf.watch`), without the panel and without wrapping WebGL. The window watches each
running game once a second while it is open and not minimized, and stops when it is closed or minimized or the desktop
goes; a hidden page takes no readings. The game windows open are `ByopopApps.running()` (`web/deskapps.js`).

### Site settings

Site-wide overrides of the games' own options, applied when a game starts, in `web/settings.js`. **The default always
leaves the game as it is** ("Game default", `"game"`), so the unmodded games stay identical to their originals. They
are one localStorage item, `byopop-settings` (fields in `DEFS`), whose `games` holds each game's own values
(`{"games": {"bejeweled": {"fullscreen": "off"}}}`); `ByopopSettings.get(name, gameId)` returns the game's value where
it has one, else the site's. Each setting is described where it acts; the ones a port or a test must know:

- `cursors` calls the game's `EnableCustomCursors` through `byopop_site_apply(cursors, music, sfx)`, which returns 0
  until `SexyAppBase::Init` has read the saved options; the page calls it every 100 ms until it returns 1.
- `accel3d`: the page calls `byopop_site_accel3d` (-1, 1 or 0) before the start, and a game with a 3D option reads
  `Byopop::SiteAccel3D()` (`framework/WebSettings.h`) where it reads that option; those games are listed in
  `ACCEL3D_GAMES` in `web/settings.js`.
- `fullscreen` off: SexyAppBase always starts the web build windowed, and a game whose own option can say full screen
  reads `Byopop::SiteWindowed()` where it reads that option; the page calls `byopop_site_windowed(1)` before the start,
  and the game saves the option as if the player had set it.
- `startVolume`, `startMusic`, `startSound`: SexyAppFramework games take the levels through `byopop_site_apply`; a
  game with its own volume engine registers a function in `Byopop::gSiteVolume` (`framework/WebSettings.h`), which
  `byopop_site_apply` calls instead; a game with no volume option is listed in `MIXER_VOLUME` in `web/shell.html` (or
  sets `PAGE.mixerVolume`), and the page sets the mixer's master gains (`byopop_mixer_gain`).
- A hidden tab's game is frozen, with no setting for it (the user, 2026-11-16: "Freeze games in hidden tabs. Only the
  music player should continue while hidden"); "Sound", below, has how. The `pauseHidden` setting, which did this only
  when switched on, is gone with its strings and its handler (2026-11-16): the freeze is what every page does now. A
  hidden Browser pane does not set `document.hidden`, so a test overrides it and dispatches `visibilitychange`.
- `autoStart` (on) never starts a game at `?silent=1`, where tests press Play themselves.

A browser grants full screen only during a gesture: `SexyAppBase::SwitchScreenMode` asks only when the browser would
take the request (`sexyapp_fullscreen_allowed`), and a request refused later (`sexyapp_watch_fullscreen_refusal`) is
taken by Emscripten and SDL as leaving full screen.

### The chat

The site has one chat, the same room for every game (`web/chat.js`; `docs/CHAT_DESIGN.md` for its design and a
possible server). For now it is a test: the messages are made up, what a player sends stays in this browser, and the
page makes no network request for it. A line is tagged with the initials of the game it was sent from
(`chat-games.js`, from `BYOPOP_WEB_BADGES`). On the desktop it is one window (the user, 2026-11-09: "Let's just have a
single chat window"). On a game's own page it never changes the canvas or its scaling, and the page cancels Home, End,
PageUp and PageDown typed on the canvas for the browser, which would scroll to it (the game still gets them).

### Player profiles

The player gives a name once and the games use it: for the player a game keeps, and for the name it asks for at a high
score. Nothing is sent anywhere. The site's list is the localStorage item `byopop-profiles`,
`{profiles: [{id, name}], current: id}` (`ByopopProfiles` in `web/profiles.js`, checked by `tools/profilestest.js`); a
name is trimmed, at most 32 characters and unique whatever the case.

**The setting `profile`.** `game`, the default, leaves the game as it is: every hook returns before it does anything,
with no random number drawn. `fill` pre-fills the names the game asks for with the site player's name. `create` does
that and, at the start of a game that keeps players, selects the game's player of that name, making it if needed with
the functions the game's own dialog uses, so the dialog never opens; a game without players treats `create` as `fill`.
With no site player all three behave as `game`.

**A game pulls, the page never pushes,** because a game's new-player dialog opens before any message from the page
could arrive. The game asks through `framework/WebProfiles.h` (web only; its header describes the hooks), names
crossing as UTF-8 and becoming Latin-1 at the border. The game's part is `games/<game>/src/SiteProfiles.cpp`, which
implements `SiteProfiles` (its names, the current one, `Find`, `Fit`, `Create`, `Select`, and `Rename` and `Delete`
where the game can) and holds the game's name rules, plus one `#ifdef __EMSCRIPTEN__` line at each decision point:
`SiteStart(id, game)` where the game would decide to ask for a new player (a SexyAppFramework game's file registers it
in `Byopop::gSiteLoaded`, which the framework calls right after `LoadingThreadCompleted`), `SiteChanged(id, game)`
wherever its list changes, and `SiteType(id, editWidget)` right after a name box is made, which types the name through
`EditWidget::KeyChar` so the box's own rules keep or drop each character (`SiteNameFor(id)` and `SiteFitTyped(...)` for
other boxes). `SiteScoreNames` is the whole hook of a game that keeps no players and only asks for a name at a high
score.

**What a game reports** is the item `byopop-game-profiles` (`ByopopProfiles.report`), each game's players and what its
rules make of each site player's name. **Operations** from the Players section of Settings (`web/players.js`) wait in
`byopop-profile-ops` until the game's next `SiteStart`, which applies them with the functions its dialogs call and
reports each as `done` or `skipped`. A rename to a name the game already has is skipped: two players' progress is never
merged.

### Save backup

The Saves section of Settings (`web/saves.js`, checked by `tools/savestest.js`) makes one .zip of the player's saves in
the page, nothing uploaded, and reads one back. Its window has two group boxes: the saves in this browser as a details
list (a game a row: its save files, its replays while some game keeps them, their size, and "Own backup" while some
game's localStorage backup is there) with Back up saves and Restore from backup… under it, the second opening the
browser's file dialog; and the import's wizard (below). The restore's question, each result and each error are message
boxes of the desktop's (`ByopopApps.warn` and `ByopopApps.tell` in `web/deskapps.js`), and the status bar at the foot holds
the last message beside where the saves are kept. It takes each game's `userdata/`, its `replays/` for games with
`replays: true`, and the game's localStorage backup (`sexy-userdata-backup:<persist>/userdata`,
`platform/WebPersist.cpp`), which can hold a save that never reached IndexedDB; never `game/`, the icons or the mods.
The zip, `byo-pop saves <YYYY-MM-DD>.zip`, is store-only:

```
byopop-saves.json            the manifest: {format: "byo-pop saves", version: 1, made, games, files: {path: ms}, play}
byopop-settings.json         the item byopop-settings
byopop-profiles.json         the item byopop-profiles
<game>/userdata/<path>       a file of the save folder
<game>/replays/<path>        a replay
<game>/browser-backup.json   the game's localStorage backup item
```

The play times (`byopop-play`) are the manifest's `play`. A restore skips a top-level `byopop-<name>.json` it does not
know, so that a later build may add one (`classify()`), and refuses the whole zip on any bad entry. Files the zip lacks
stay; each game's localStorage backup is removed (a left one would delete the restored files at the next start);
settings and players are replaced; play times become the larger of the two. It holds every game's Web Lock while it
writes, so it never runs while a game page is open.

### The music player

The Music window (`web/music.js`, checked by `tools/musictest.js`) plays the music of the games the player added, from
their own files in this browser; the site ships no music and sends nothing anywhere. Its list follows the games' files
while the page is up: `ByopopGameFiles` (`web/settings.js`) tells each change of its record on the page
(`byopop-game-files`), and another page (a game's window, another tab) is heard by the storage event of the record. When the
games with music change, the list is read again while the window is shown, or at its next showing: a game that came gets
its tunes read, the tunes of the games still there are kept, and a game whose files went loses its own, the tune playing
stopping with them (the user, 2026-11-16: "Music required reloading the page"). The Sounds window does the same while it
is open.

**Music's data** is `web/soundtracks.js`, by game id, whose header gives an entry's fields; tunes are named plainly,
never with invented titles. A game's session sets these from its code:

- `where`: where the game's code plays a tune, named as a game music rip names it ("Main Menu", "Level Theme 1",
  "Danger 2": Title Case, one place, at most 20 characters, several tunes of one place numbered in the order the game
  reaches them; the user, 2026-11-11: "Rip style please!").
- `unused`: the tunes the game's code never plays.
- `order`: the list's order, the tunes as a player hears them from launch through a game to its end, modes, levels and
  books as the game reaches them, unused tunes last (the user, 2026-11-13: "more closely follow what you might hear instead
  of having the parts in order, and put unused tracks at the end").
- `cuts` and `chains`: tracks split and grouped as the game plays them (the user, 2026-11-11: "Group and split").
  `cuts` splits a subsong where the game's code starts a different tune partway through it, or where one song of a
  medley ends and the next begins, the later pieces being `"<file>#<n>@<order>"`; `chains` makes one track of parts the
  game only ever plays as jumps inside one tune. `starts` is for an order the game also starts a subsong at while the tune
  before plays on through it: the piece from there is a track, and the subsong stays whole. No game uses it now:
  Bejeweled 2's long subsong, which it once served, is a medley of fourteen songs that plays straight through and back to
  its start, and is cut at the composer's own markers between them instead (the user, 2026-11-19: "I am hearing some very
  mild seams that make me think the track breaks aren't correct here"), each piece named as the composer's album names it
  and checked against its lengths. Its places are "Classic Mode 1" to "7" (the first two songs both 1) and "Action Mode 1" to "6": the game starts
  it at order 2 for Classic, Endless, Twilight and the hidden Classic mode, at order 122 for Action, Puzzle, Hyper,
  Cognito and Finity, and nothing else moves it, at a level or anywhere.
- `titles`: a tune's own title, shown in quotes beside its rip name (the user, 2026-11-19: "add these titles beside the
  rip names, in quotes"): the user's word, cited at the entry, or a title the module's own text holds, read from the
  user's file as the list is read and never written in the table: `{own: true}` the module's title, `{line: "<order>"}`
  what the module's message says beside that order, up to a comma or a bracket (`inner` for the words in its brackets).
  The decoder's `bm_meta` gives a module's title, artist, tracker, message and subsong names. Alchemy's, Bejeweled's,
  Dynomite's, Bookworm's, Mummy Maze's and Twist's are taken from their files this way; the other modules' texts hold no
  song titles (a working title, "untitled", the composer's name, or places).
- `alone`: subsongs that are one pattern running on into the module's silence, listed with the unused patterns as that
  pattern played alone (Bejeweled 2's two, which music.xml names but the code never reads; the user, 2026-11-19: "really,
  they're only single patterns that we could just file with the others").
- `shuffle`: what the player's shuffle leaves out beyond what it leaves out by itself, and the tunes it plays together.
- `level`: "Music levels".

**The decoder** is the games' own two module renderers, built for the web as `openmpt.js` and `openmpt.wasm` (target
`byopop_openmpt`) and picked as `Platform::Mixer` picks them, so a tune sounds as it does in its game. A MIDI file
(TipTop's one tune; `bank` in `soundtracks.js`) plays in the same decoder as TipTop's port plays it
(`web/music/midi_web.c`): the game's own sequencer, a pass over the tracks each millisecond, into TinySoundFont with the
port's General MIDI bank cut from GeneralUser GS. The page fetches the bank from `soundfonts/` (3.3 MB, served beside
the pages, as its licence asks) the first time a MIDI tune plays. Each millisecond renders whole, so the sound does not
depend on how the worklet's chunks fall. Against the port's `musicwav` render it differs by at most one step in 32767 over
180 seconds, the song's loop included, the margin the port's notes give between its native and web builds. With repeat on
the song plays on from its start as the game's does, the notes still sounding carried over. Seeking runs the sequencer
silently to the place, setting its programs and controllers but playing no notes. A MIDI tune has no tracker, piano roll
or score.

**The player's rules**, from the user: long silent endings are skipped (the user's notes, 2026-11-10: "Some of the
short tracks have a bunch of dead air at the end, where the tracker slows to a crawl"; `trimEnd`), a pattern played alone
too (`soloSeconds`: Bejeweled 2's two, whose rows slow to about five seconds each after their last note), the first piece of a cut subsong too (Twist's Game Over: a 3.5-second jingle,
then the module's own 13 seconds of silence before the cut); only the playlist
scrolls (2026-11-09); closing the window stops the music and frees what it held (2026-11-09;
`ByopopMusicPanel.held()` tells a test what is still held). Each format in the list names itself in full as a tooltip
(the user, 2026-11-16: "I don't know what an IT is, Show what the extension stands for").

**Repeat** (the user, 2026-11-22: "Repeat seems to break on some tunes, like BookWorm's game over 2, and in any case the
position indicator doesn't loop back to the start") goes round a tune as its game would, and the seek bar and the time go
round with the sound. A subsong whose decoder goes on, once it has played through, at a place the tune has played and with
sound there follows that loop seamlessly, as the game hears it (`loopOf`: libopenmpt tells the place, libxmp-lite is
played across its end), and its position goes back to the time the tune first reached that place, after the intro where
there is one. The decoder's own time cannot be shown as it is: libopenmpt's counts on past the end once a tune has gone
round, and libxmp-lite's mostly goes back by itself, so while a loop is followed the player looks at the decoder's place
every 512 frames and keeps the time each place was first reached. A loop into another tune's orders (Dynomite's Time
Trial Clear goes on at an order of its Game Over), into a silent pattern that jumps to itself (BookWorm's two game overs,
which the games play once) or past the tune's own time plays the tune again from its start instead, as does a range of
orders, a cut piece or a joined playlist; a recorded tune goes round to its loop's start (`loopFrom`), a MIDI song to its
start. `tools/musicrepeattest.js` plays every tune of every staged game past its end with repeat on, unheard at 8000 Hz,
and checks that the sound goes on, that it goes on from where the decoder itself goes on (or the start, as above), and
that the position falls back there; before the fix 137 of its 223 tunes failed, nearly all because the position never went
back, and BookWorm's game overs looped on a silent pattern for ever. For about twenty subsongs libopenmpt's length is
0.7 to 1.1 seconds longer than the tune plays, so the seek bar goes back from just short of its end.

**Shuffle** (the user, 2026-11-19: "make the shuffle group together tracks that are only ever played in direct sequence,
never play unused tracks, and skip tracks that would not make sense to have on shuffle (for instance, no intros, short
themes that could just as well be sound effects, or stings)") is a toggle beside Repeat, kept in `byopop-music-shuffle`.
On, the tune after one that ends, and Next, is drawn from every game's tunes, each once before any comes again (the last of
a round never first of the next); a group (`shuffle.groups`: Bejeweled 2's second intro and the song it leads into, Big
Money's level-up fanfares and the pause tunes they run into) is drawn as one and plays in its order. Never drawn: unused
tunes, unused patterns, a tune under 20 seconds outside a group (a sting, fanfare or jingle), and those `shuffle.out`
names (intros, loading, splash and logo tunes). Previous goes back the way it came; picking a tune goes on shuffling
from it.

**Rip style, themes and series** (the user, 2026-11-22: "Add a "rip-style" toggle to the music player that loops themes
twice when they would normally be looped in game (rather than going to another track or simply ending) and does a fade out
at the start of the third loop over a short period, with an option to instead transition using the game over theme the way
the game does it, if applicable. Also add an option for autoplay to stop after going through a single game, and an option
that only plays themes and skips one-offs like a game over sound unless they lead into another track (as in Big Money).
Finally, allow looping a series of tracks that can play together, like Bejeweled 2 for each of its modes."). The window has
a menu bar, as a player's window had, with one menu, Options (Alt+O): Rip Style, Game Over Ending (greyed while Rip Style
is off), a rule, Stop After Game and Themes Only, each ticked while it is on, kept in `byopop-music-rip`,
`byopop-music-rip-over`, `byopop-music-one-game` and `byopop-music-themes`, and heard at once on the tune playing. They
read what each game does at a tune's end from `soundtracks.js` (`loops`, `once`, `next`, `stings`, `gameOver` and
`series`, documented in its header and filled for every game from its code and notes; Word Harmony's `intenseEnd`, which
the game may never play since its stop fades the drums layer out before that last clip, stays the table's game-over tune,
played once).

- *Rip Style*: with Repeat off, a tune the game loops (`loops`, or a series played as one tune) goes round as with Repeat,
  intro once, and from the start of its third pass fades out over ten seconds and ends, then autoplay goes on. Two loops
  and a ten second fade are vgmstream's defaults, the usual figures for game rips. A tune the game plays once plays once.
  It counts the passes by the tune's position going back, the same for every kind of source: a subsong round its own loop,
  a range of orders, a cut piece, a playlist of joined pieces round to its loop piece, decoded sound, a MIDI song and a
  series of one module's pieces. A short loop goes on round during the fade, as a rip's does. With Repeat on, Repeat wins.
  A seek starts the count again.
- *Game Over Ending*: where the game goes from a looping tune into its game-over tune (`gameOver`: Atomica, AstroPop, Big
  Money, both BookWorms, Bejeweled Twist, Dynomite, Seven Seas, Typer Shark, Ningpo, Noah's Ark, Heavy Weapon, Rocket
  Mania), the start of the third pass goes into that tune the game's way instead of the fade: at once (`cut`), the tune
  faded out over the game's time and then the other (`fade`), or the two crossed over that time, the one fading out as the
  other fades in (`cross`, the framework's FadeOut and FadeIn), the time from the game's own fade speed. The game-over
  tune is read and made ready during the last pass and handed over without a pause; the list and the display then show it,
  and autoplay goes on after the tune that led into it, passing over the game-over tune.
- *Stop After Game*: autoplay stops where the next tune would be another game's, or the game's first again; with Shuffle
  on, it draws only from the game playing, each of its tunes once after the one chosen, and then stops.
- *Themes Only*: autoplay, Next and Previous pass over a one-off (`stings`: game-over tunes, fanfares, level-complete
  stings, results) that leads into no other tune, and Shuffle never draws one. A one-off that does lead on (`next`: Big
  Money's level-up fanfares into their pause tunes) stays, and its tune plays after it; with Shuffle the two are drawn as
  one.
- With Rip Style or Themes Only on, autoplay follows `next` as the game does (in Shuffle only a one-off's), then goes on
  after the tune that led there.
- *Series* (`series`): tunes a game plays in turn as one set are an entry of their own in the game's list, after its tunes,
  its place "Series": Bejeweled 2's Classic Mode (its eight pieces) and Action Mode (six), Bejeweled Twist's In-Game (its
  six level tunes, a level each), and the two Mahjong games' Layout Select. Pieces of one module are one tune of all their
  segments, which goes on from piece to piece as the game does; any other set plays tune by tune. Repeat on a series goes
  round the whole set, never one member; Rip Style plays the set twice and fades. Autoplay and Shuffle pass over series,
  which play when chosen. In the game Bejeweled 2's last Classic piece runs on into Action Mode's first and the last Action
  piece back to Classic's first, so a long game goes round all fourteen; the series follow the user's "each of its modes".

`tools/musicfieldstest.js` (unit-web) checks the fields for every game against the list `tunesOf` makes from the staged
files: each tune the list shows in exactly one of `loops`, `once` and `next`, every id named one of the game's tunes, a
`next` and a `gameOver` leading to another tune, a `gameOver` from a tune the game loops with its way and time, and each
series an entry with its members. It then plays, unheard at 8000 Hz through the player's own sources and `ripStyle`, the
shortest looped tune of each kind (subsong, range, piece, decoded, joined, MIDI, series of pieces): two passes, then a ten
second fade from the third pass's start with no sound at its end, and with Repeat on no end; a tune played once ends
after one pass; and each Game Over Ending way between stand-in tunes, handed over at the third pass's start, at once or
after the game's time.

**Bookworm Adventures' titles** come from the game's own music player list, which `scripts/battleengine.luc` builds
(Bookworm Adventures' session, 2026-11-22): `titles` takes `{luc, list, data}`, a string of a Lua script read as the
listing scans the game (`lucConstants` reads PopLua's Lua 5.0 constants, its own tags: 4 a whole number, 5 a string; the
function whose constants hold `"NeedsMPDialog"`), the one just before the whole number that is the tune's order there, or
with `after` the after-th string following it (Book 1's battle tune, whose order 0 the list does not write again). The
titles are never written into the repository; `musicfieldstest` checks each is found.

**Nothing drawn unseen** (the user, 2026-11-15: lower the web builds' processor time; the site's animated parts do no work
while nobody can see them). Music's views and the Splits window's running timer draw a frame at a time only while their
window is on the screen: closed, minimized, scrolled out of view or in a hidden tab, their loop stops (a tune plays on, a
run's time goes on), and shown again (an IntersectionObserver on the window, and the tab's visibility) they draw from the
current state at once. A tune playing in a minimized Music window then costs only its decoding.

**Unused patterns** (the user, 2026-11-15: "Add unused tracker patterns to their own section in the music player (since
they're not compositions unto themselves, not listed alongside their game, and don't start playing them while going down
through the list, only when one is selected)"; part of a game's definition of done) are a module's patterns that no order
of any subsong plays (markers skipped), as the decoder reads them (`bm_patterns`, `bm_order_pattern`), leaving out one
with no notes (`bm_pattern_notes`). They come from the scan for every game at once, with no field in `soundtracks.js`,
and are listed in their own section after every game's tunes, never under their game: each entry is "<file>, pattern
<n>" with its game beside it. Playing on and next/previous wrap at the last real tune and never enter the section; a
pattern plays only when picked, once, and then stops (next/previous step among the patterns while one is chosen). The
decoder's `bm_solo` plays one alone from its first row and reports it as the module's only order, so the tracker, piano
roll and score read it as any tune: libopenmpt's order list is put aside for one naming the pattern
(`web/music/openmpt_solo.cpp`, which reaches the module's sound file because libopenmpt's C interface cannot change that
list), and libxmp-lite's first order plays it instead, ending where play leaves that order or goes back to an earlier row.
A pattern's own commands still count (Bejeweled's Network pattern 10 sets speed 10 on its first row and lasts 33 seconds).
An unused pattern that is a copy of one an order plays, cell for cell, is left out, and copies among the unused ones are
one entry, "<file>, pattern <n> (same as <m>)" (the user: "Unused pattern 61 of AtomicTank is a dupe of pattern 64").
A game whose code starts a module at a pattern outside its order lists would say so in `soundtracks.js`; none does yet.

**The views**, cycled by a click on the display (spectrum, tracker, piano roll, score, nothing), follow the user's
notes:

- **The spectrum** is steadied (the user, 2026-11-10: "a little too jittery"). Its bars are green, then yellow, then red
  up the display's height, so a bar's top shows its level (the user, 2026-11-18: "The spectral analyzer would benefit
  from a gradient for the volume"; Desktop 95's `--mu-bar`, `--mu-bar-mid`, `--mu-bar-high`). It reads the player's own
  tune before the player's volume, and a followed game's music with the game's own music volume undone, from the `gain`
  its view gives (the music volume times the song's own; "It should also be independent of in-game volume when synced
  with that"). A game whose build gives no gain is drawn as heard.
- **The tracker** names the instrument over each channel (the user, 2026-11-11: labels on the columns). A channel's
  whole head toggles its mute (the user, 2026-11-11: "It's too hard to click on the channel to toggle the mute"), and a
  Ctrl-click or a double-click solos it (the user's notes, 2026-11-10: "tracker labels and mute"; the user, 2026-11-11:
  "Add double clicking a tracker channel to isolate its sound"), through the decoder's `bm_mute` or a followed game's
  `byopop_music_mute`. Channels that play no note in the tune are hidden, keeping their numbers (the user, 2026-11-11:
  "Some songs seem to have totally empty channels the whole way through. Possible to hide from tracker on a per-song
  basis?"; `tuneUsed`). The FX button adds an effect column (the user, 2026-11-11: "add a brief English description of
  tracker commands when they appear in the song as an optional column"), its words `music.fx.*` in `en.txt`. The "?"
  beside it opens a message box on how to read the tracker (`music.tracker_help`), which was in About until System
  Information took its place. A note held is a faint line of the display's colour down the middle of its place, from the
  row after it until the channel's next note, note-off, cut or fade, or the pattern's end, as the piano roll's bars show it
  (the user, 2026-11-22: "Yes, add the line"; `heldRuns`); a row with only an effect does not end it.
- **The piano roll** (the user, 2026-11-09: "add the piano roll too") has rows at least 4 pixels (the user,
  2026-11-11: "way too zoomed out") and instrument names (the user, 2026-11-11: "If there is any way to get instrument
  names, let's do it"; `bm_names`, `byopop_music_names`).
- **The score** is VexFlow 5.0.0 (`web/third_party/vexflow-bravura.js`), fetched when first chosen, and scrolls (the
  user, 2026-11-11: "sheet music should scroll instead of fitting to window width").
- **The metre** (the user, 2026-11-11: "not every pattern has 4 bars. Some have 8 or 12") shows as "4/4 16TH FILE":
  FILE from the module's own rows per beat and bar (`bm_meter`, `byopop_music_meter`, `byopop_it_meter` in
  `platform/ModuleText.h`), NOTES from the notes' repeats, DEFAULT otherwise.

### Following a game

While a game window is in front and plays music, Music's views show that music live (the user, 2026-11-09: "make the
tracker/spectral analyzer sync with what's playing in the active game"), tagged with the game: the game window last
made active, while open and not minimized (`frontFrame` in `web/deskapps.js`). While Music plays or loads a tune of its
own, the views and the seek bar are that tune's, and no game is followed (the user, 2026-11-12: "jumping around in the
song does not actually skip the audio").

**Focus and sound (the user, 2026-11-10).** A site window is a desktop window: a game whose window is not the active one
does what its original does without the focus (the framework's own focus handling, nothing added for the web). Only one
game window is heard at a time (`hush` in `web/deskapps.js`): the game in front; with no game in front, none while
Music plays a tune of its own, else the game last in front. The others' output node is disconnected from their
speakers, not their page's mute, which is the player's own switch.

**The game's own track (the user's notes, 2026-11-10: "The tracker sync should show what track is actually playing and
its progress", "If possible, make the controls change what plays in game").** Music shows the list's tune the game
plays (`tunesOf`) as its own; the seek bar, Previous and Next move the game's song (`byopop_music_seek`), while Play,
Pause and Stop stay the player's.

The game's side is a site hook in the platform layer: no game's code changes, and native builds do not compile it.
`Platform::Mixer` exports the `byopop_music_*` calls (`platform/Audio.cpp`), reading a libopenmpt module through
`ModulePlayer`'s web-only `GetView` and its neighbours (`platform/openmpt/OpenMPTPlayer.cpp`); a stream, or TipTop's
MIDI synthesizer, gives the spectrum only, tapped from the game's own AudioContext so that what is heard does not
change. A game that plays its tunes as sounds (Venice: `voices` in `web/soundtracks.js`) has no song while its music
plays; its tune is the loudest of its voices playing one of the files its tunes list (`byopop_sounds_view`, the Sounds
window's call; `frontFrame` gives the game's id), followed as a stream is (the user, 2026-11-18: "Venice's audio doesn't
sync with the spectral analyzer"). `ByopopMusicPanel.following()` tells a test the game followed and its view.

### The soundboard

The Sounds window (`web/sounds.js`, `#sounds`) plays the sound effects of the games the player added, from their own
files (the user, 2026-11-09: "I will want them for every game"), as `web/soundboards.js` lists them by game id; its
header gives an entry's fields. Names are plain and our own, and the when-lines come from the port's own calls, never
the game's text.

**Presets** are sequences that sound in the game, played as the game plays them, in a group box of their own
("Sequences") before the others (the user, 2026-11-11: "preset sequences of sound effects for the soundboards based on
sequences that sound off in game"): a cascade of each depth, a level up, the game over. Each is a stretch of the sound
log (`BYOPOP_SOUNDLOG`) of a game the port's own bot played (`tools/playbymove`), made into a preset by
`tools/soundpreset.py <log> <first update> <last update>` and named rip style, as Music's places are.

**Other games' sounds.** A when-line ends with what the user's reviews found of the sound in other games
(`tools/web/soundcompare.py`, its decisions in `testnotes/`). "Also in *game* as *name*" is the same recording,
noted where one side is unused or the other game is not ported (`tools/soundmatch.py`). "Resembles *game*'s *name*" is
a near copy the user heard as similar, with how this one differs. "Made from the same sound as …" is a pair the user
accepted from `tools/soundbased.py`'s list (2026-11-13), with what sets the two apart: pitched, sped up or slowed,
layered with another sound, filtered, a tail cut or added. Its direction is the detector's, which of the two holds more
of the other, not which game came first (in 23 pairs Bejeweled 3's sound is the one held in Bejeweled 2's), so the words are
"made from the same sound", never "based on". A sound with three or more such partners names them in one sentence and
says only what they differ in. A decision on a pair stands for every identical copy of either side.

### The splits timer

The Splits window (`web/splits.js`) stands in for LiveSplit (the user: "a LiveSplit stand-in on the site so that people
really could do speedruns on it"), its splits kept in IndexedDB (`byopop-splits`) and imported and exported as
LiveSplit `.lss` files. Game Time is the update count of the game window in front when the run starts
(`byopop_update_count`) times the game's update step (10 ms; 5 ms in the Mahjong games); a game without the count has
none.

**Auto splitting** (the user, 2026-11-11: "Add Bejeweled autosplitter to the site. Also, let's build in autosplitters to
every port of a game with a leaderboard on speedrun.com"). `web/autosplit.js` holds each game's auto splitter: the block
the port publishes, how to read it and the decisions. With Options, Auto Splitter on (the default), the window polls a game
every 10 ms, as LiveSplit does, while a game window is open (with none, every half second, so the page is not woken a
hundred times a second for nothing; the user, 2026-11-15). That is the game window in front while the timer is idle, and the run's own window while it
runs. It does so only when the game has an auto splitter and the splits' game name is the game's speedrun.com name. Its
decisions go through the timer's own Start, Split and Reset, so an automatic reset asks about new bests as the player's does.
The timer is idle again before that question is asked, and the answer applies to the attempt that was reset: a reset for a
new game comes with that game's start, which a timer waiting on the answer would miss. A split undone by hand is split again on the next poll, and the run's window closing is the script's exit. File, New from
Template offers each game's templates, named after its speedrun.com categories, and opens the stored splits of the same
name when they exist. Game pages export `HEAPU8`, which the page reads the block from.

Bejeweled publishes the desktop's LiveSplit block (`games/bejeweled/src/LiveSplitBlock.h`, layout 1, filled by
`LiveSplitObserver.cpp` after every update). In the web build the page turns it on by asking for its address
(`byopop_livesplit_block`), as `features.livesplit` does natively; the observer only reads the game. Its decisions are
`games/bejeweled/tools/livesplit/lsdecide.py`'s, line for line: start at GO in the category's mode, split at each wormhole
or when the score on screen reaches a segment's number, and reset when the run's board ends, with each reset kind a setting.
`tools/autosplittest.js` (in `check.sh unit-web`) replays lsdecide.py's 27 scenarios, 53 replays on the game and probe logs,
from `tools/autosplit/bejeweled-scenarios.json.gz`. `tools/autosplit/bejeweled_fixture.py` writes that file from
`run_tests.sh`'s runs, with the events lsdecide.py decides, and every event, its poll and its board must agree.

#### One design for every port

Every other port publishes the same block and is read by one decider, driven by a table of the game's rules. Bejeweled
keeps its own decider, which is lsdecide.py's, but its block is already the shared one.

**The block** is the one Bejeweled, Big Money and BookWorm already write, kept as it is: the magic `BYOPOP-LIVESPLIT`
F0 F1 F2 F3, a four-letter game id, the version and size, the `seq` protocol (odd while the game writes), and the
append-only rule. In all three, bytes 0x00 to 0x7F already mean the same thing: `active`, the `updates` heartbeat,
`flags` (RESUMED, VANILLA, CHOSEN_SEED, REPLAY, GAME_OVER, PAUSED), the board serial, the GO, level-clear and game-over
counters with the serial of the board each happened on, four death kinds that add up to the game overs, and `tick`,
`state`, `level`, `displayLevel`, `displayScore`, `score`, `boardLevelClears`, `goTick` and the board's highest score.
Bytes 0x80 to 0x9F are the game's own (Bejeweled's rules id; Big Money's and BookWorm's mode, skill and score key), and
0xA0 holds the clock. Layout 2 moves nothing and appends 0x20 bytes at 0xB0, for a size of 0xD0:

| Offset | Field | Meaning |
|---|---|---|
| 0xB0 | u32 `wins`, `winSerial` | the game was won (its last level, an escape, the final chapter), and on which board |
| 0xB8 | i32 `mode` | the mode being played, as the game's table numbers it; -1 off a board |
| 0xBC | i32 `skill` | the difficulty, as the table numbers it; 0 for a game without one |
| 0xC0 | u32 `inGame` | 1 from the board's GO until its game over or win, 0 otherwise (a pause keeps it 1) |
| 0xC4 | i32 `progress` | a figure of the game's own that a category ends on, where level and score can't say it; -1 when unused |
| 0xC8 | u8 `reserved[8]` | zero |

These are the six things a speedrun.com category needs. The mode and skill are at 0xB8 and 0xBC in every game, so the
decider needs no per-game offsets. The level is `displayLevel`, with `levelClears` as its events. The score is
`displayScore`, with the board's highest score frozen at its game over. In-game is `inGame`. Won is `wins`, and lost is
`gameOvers` with its death kinds. A game whose code has no GO moment of its own takes the first update on which a fresh
board accepts input. The death kinds keep each game's own names: the table says what they mean.

**The writer** is a shared header, `platform/AutosplitBlock.h`, with the struct, its `static_assert`s, the log line
`-livesplitlog` writes, and a small writer class, `AutosplitWriter`. A port calls `Activate` when the block is first
asked for, `NewBoard(serial)` when its board changes, `Go`, `LevelClear`, `GameOver(kind)` and `Win` at those moments,
`SetDisplayScore` (or `RaiseScoreMax`, where the game's highest score follows another score than the one on screen), sets
the rest of the update's fields in `Next()`, and calls `Publish` after every update. The class keeps the counters,
serials and latched flags and does the `seq` writes. Big Money's and BookWorm's headers become views of its first 0xB0
bytes, tied to it by `static_assert`s, so their scripts, logs and tools read the same bytes as before; Bejeweled's
follows. Big Money writes it from Milestone 2, on the web too.

**Inert.** The block is written only once something asks for it: on the web, the page asking for its address
(`byopop_livesplit_block`, which every port exports under the same name); natively, `features.livesplit`. Until then the
only cost is one test of a flag after each update. The writer only reads the game: no random draw, no game state, no
save, no drawing, no timing. Each converted game proves it with its frozen baseline, run unchanged both with the block
off and with it on.

**The rules table** is in `web/autosplit.js`, one entry for each game:

- `name`: the game's name on speedrun.com, which the splits' game name must match (the game's own title where it has no
  board of its own: Rocket Mania Deluxe);
- `id`: the block's game id;
- `modes` and `skills`: the names behind the block's numbers;
- `deaths`: what each of the four death kinds is (each one a reset setting, as in Bejeweled; one with `goesOn` is no end of the
  run, as Venice's sunk Flood building, played again on the same board);
- `templates`: one per speedrun.com category and subcategory (the site's own where there is no board), each with `mode` and `skill` (none for any, or a
  list of the allowed ones), its segments, and `end`, how the run finishes:
  - `levels N` (the Nth level clear);
  - `score N` (the score on screen reaches N);
  - `won` (`wins` rises on the run's board): the run's end whatever segment it is in, the segments left split with it, and
    seen even when the board has gone in the same poll (Dynomite's tutorial wins as it closes);
  - `lost` (a game over; for a death% category), or `lost N` (a game over while the game's progress is at most N; any
    other game over resets the run as usual);
  - `progress N` (the game's own figure reaches N);

  and, where a category's splits are not its levels, `by: "progress"`: each segment but the last splits when the game's
  progress reaches its number (Mummy Maze's treasures).
  `targets: [N, ...]` splits each segment but the last at that many level clears, for a game of many levels (Cosmic
  Bugs' 577 boards in 13 segments). `limit: {field, max, setting}` resets the run when a block field passes `max` (AstroPop's SUPA Weapon count, in No SUPA
  Weapon). `games: N` makes a run of N games, a segment each, each ending at the end's progress; between them the run
  waits, and goes on with the next new game of a skill it hasn't had (AstroPop's 100%, one game a pilot).

One decider reads every entry. It starts at a GO in the template's mode and skill, but never on a resumed save or a
replay. It splits at each level clear, or when the score reaches a segment's number (Bejeweled's naming rules: "Level 3",
"25,000", "25K"), and makes the last split on the template's `end`. It resets when the run's board is lost (unless `end`
is `lost`), replaced, or quit, or when its window closes, with each kind a setting. It reports the timer's Game Time
as now: the game's updates times its update step. A category's timing rules, real time or game time and where they say
it starts, are noted in the template, and the template follows them.

**Rows written so far.** BookWorm (speedrun.com: Bookworm Deluxe): its progress is the rank the summary shows, the levels
cleared up to 31 (BookWorm's session: Board's level counts up only when the next level starts, so the summary of a level shows
it plus one). Bookworm Supreme (Any%) is `progress 31`, reached on the update the 31st level is complete; Bookburner
(Death%) is `lost 0`, a game over before the first level is cleared. Seven Seas (speedrun.com: Seven Seas Deluxe) has
Level 30 on Easy, Average and Hard (skill 1, 2, 3): its level clear is the update its level counter goes up, after the level's
panel, and the map after levels 10, 20 and 30 is made in that same update, so `progress 30` (levels completed) ends the run
where the board's rules do; its one game over is running out of lives, and New Game takes the board away (a quit). Mummy
Maze (speedrun.com: Mummy Maze Deluxe): a board is a whole game, menu to menu, and a level clear is each chamber's
level-complete page. Its progress is the treasures collected, which in Classic rises when the next pyramid has begun: 1st
Pyramid (Classic) is 15 chamber splits and the end at `progress 1`, the spawn in the next pyramid. 100% Run (Adventure)
splits at each treasure, a template with `by: "progress"` (each segment's split is the progress reaching its number, not
a level clear), and ends `won`, at the last pyramid's chest. Being caught is no game over; Abandon Hope is the only one. Rocket
Mania Deluxe has no board of its own (the one named Rocket Mania! is for a Java edition), so its row is the site's own
categories, named after the game: Level 10, 20 and 30 on each difficulty, from "Get Ready!" (the first update of a new
game) to the last level's "Level Up!"; a split at each Level Up. Arcade and Classic both have levels and Strategy never ends
one, so the templates' `mode` is the list [Arcade, Classic]. Its losses
(the night running out, the top line filling, no matches left) count at the game-over screen, since a launch still burning
can clear the level in Classic. It has no win. Alchemy (speedrun.com: Alchemy Deluxe): 1 Board and 7 Boards in
Strategic or Time Trial, from the click on Start New Game to the board's result screen (the statistics panel), with any
skill, and Tutorial%, which ends `won` at the tutorial's last step appearing: that is the code's one moment for done, and
the button under it only leaves. Its deaths are the forge overflowing (one kind in each mode), the Time Trial forge
staying full, and leaving a game mid-board. AstroPop (speedrun.com: Astropop Deluxe): Any% and Any% (No SUPA Weapon)
for each pilot (the skill), a new Classic game from level 1 to level 32's "Level Up". Its GO is the new game's first
update: the board's start click comes before the pilot is chosen, which the block can't see. Its progress is the level
just cleared, since a fast warp gate skips levels, so the splits are `by: "progress"`. No SUPA Weapon has a `limit`: the
game's own first word (`own0`, block 0x80) counts the weapon's uses, and the first resets the run (`resetSupa`). 100% is
the four pilots, each a new game, and a template with `games: 4`: a segment a game, each ending at that game's `progress
32`. Between games the run waits: the finished game's END GAME resets nothing, and the next new game with a pilot the run
hasn't had carries it on. Survival has no timer. Typer Shark (speedrun.com: Typer Shark Deluxe): a block board is a
whole game, its GO the first update after the difficulty or the Abyss is chosen. Adventure Mode, for each difficulty,
ends on returning to the map after level 11 is beaten. Its progress is the levels beaten and left, rising as the map
comes back, so the splits are `by: "progress"`; a level's own clear comes earlier, before the player leaves it. Abyss
Mode ends `lost`, at its game over (the screen's time is the game's, in its own first word, which the site doesn't use
yet), and Death% at an adventure's first game over. "End this game" in the options is no game over: the game goes
away, a quit. Cosmic Bugs (speedrun.com: Cosmic Bugs; its board's 2005 PC game is Retro64's, the one PopCap
published in 2007): Any%, from Play on the instructions page, the screen a new player's Start Game shows, to the
mission-complete screen, its win. A continued game or a chosen board skips that page and starts nothing. A mission is
577 boards, so the template has `targets`: the level clears at which each segment splits, here the first board and the
last of each of the 12 steps of 48 boards, and the end at the win. Dynomite (speedrun.com: Dynomite! Deluxe): a board is
a whole game. Endless Puzzle, for each amount (1K to 50K) and difficulty, runs from the difficulty's click to the score on
screen reaching the amount, its segments the amounts up to it. Time Trial, for each difficulty, runs from the first egg
fired (its GO, after the shot and not at the click) to the board cleared, its win; the board times it in game time, which
is the game's updates from that GO. Fossil Challenge, Strategic or Panic, splits at each fossil found (the third fragment
off the bottom) and ends at the fifteenth, its win. Tutorial splits at each mode's part and ends at the main menu with
every part done; that win comes in the update the tutorial closes. The board's Easy, Normal and Hard are the game's Normal,
Hard and Expert. Leaving a game is a game over of its own kind. Venice Deluxe (speedrun.com: Venice): a board is one pass through a mode, from its click on the mode menu to the main menu.
Journey and Flood run from the click away from the world map before building 1 in a new game (the tutorial is not timed),
split at each building cleared and end at the map's CONGRATULATIONS! after building 71, its win; a Journey game out of
tries resets, a sunk Flood building is played again and the run goes on. Trick Shots runs from the click on the mode to
the win scene with every trick shot made. A continued game starts nothing. The board's per-level Journey and Flood
categories have no template. Bejeweled Twist: Level 10 in Classic and in Zen run from the start button's click to level
10's "Level Complete" picture; Unlock% from a new Classic game to the fourth mode opening. Enigma% and Challenge 100% are
runs over many challenges, a board each (`across` in the template): from the first challenge's click to the win of
Voltage 1 or Enigma 7, read from the block's own words (the challenge's category and number). A failed challenge, tried
again, resets nothing, and each earlier win splits while segments are left. The per-level 100% category has no template.
Bejeweled 2 (B2DX): Classic, Action and Hyper to a level or a score, and Puzzle to its win or to the puzzles solved on the
run's board (25% at 20, All Puzzles at 80; not the profile's count, which an opened galaxy starts at 80), each from the
board's GO, after the portal and How to Play. 4 Square and Switcharoo run over many boards (`ownLevels` in the template)
and count the levels the block keeps per mode: the modes with four each, and Action's and Endless's together. Endless's
"N Gem", Hyper's Max Score and Full Loop, and the per-level 100% have no template. Its block logs replay in the test.
Heavy Weapon Deluxe (HWDX; speedrun.com: Heavy Weapon): Mission Mode (PC) runs from the click on Campaign in a new game
to the final level's results screen, its win in the same update as that level's clear, and splits at each level's
results. A campaign continued from the menu starts nothing. The game-over box's TRY AGAIN plays the level again on the
same board and the run goes on (a death that `goesOn`); QUIT or the menu after it resets as a quit, as the main menu
after Venice's sunk Flood building does. Survival and the console categories have no template.
Hammer Heads Deluxe (HHDX; speedrun.com: Hammer Heads Deluxe): Classic% and Tough Cookie% run from the Go! that starts a
new game's first level (Classic's level card, Tough Cookie's "Ready?" box), split at each of the 25 levels' ends, and end
at the Go! on the "Ready?" box after the trophy room that awards King Globus's trophy (`end: "progress 1"`), as the
board's rules say; the last level's own end and the win come after it. Out of health is a game over (`resetHealth`): its
try again is a new game on a new board. A continued game starts nothing. Marathon% is timed by the game's own clock, which
the block has not, and All Trophies% runs over many games and modes, so neither has a template.
Big Money has no board, so its row is its native
script's rules, with mode and skill in the category's name and levels or scores as segments. Both replay their native
scripts' scenarios in `tools/autosplittest.js`. Every other game with a board gets its row in Milestone 3, from the
speedrun.com categories and its session's answer on where in its code GO, a level clear, a game over and a win happen.

**Native** keeps a LiveSplit `.asl` per game, as Bejeweled's, Big Money's and BookWorm's. Because the block and the
decider are now one, the next step is one script that reads the table (exported from the same entries) instead of a
script per game. That is a tools change for the coordinator, not part of this milestone.

**Tests.** `tools/autosplittest.js` gets a replay set per converted game, as Bejeweled's: block logs from the game's
bot and scripted runs, the events the decider must see, and each template's start, splits and end.

### Music levels

Each port's music was measured against its original's engine (BASS or FMOD 3) with `tools/musiclevel`:
`musiclevel.exe` decodes a module silently with a copy of the game's own `bass.dll` or `fmod.dll`, and `portlevel`
renders it through `Platform::Mixer`; both print the RMS of the first seconds. libopenmpt on MO3 differs by up to 2.4 dB
either way from one module to the next, which is left as an oddity. `platform/MusicLevels.h`, which the music player's
decoder includes too, holds the render settings, BASS's amplify curve (`ByopopBassAmplifyDB`) and FMOD 3's XM pre-amp.
A game whose code plays its music at another level says so in `web/soundtracks.js` as `level` (`{bassAmplify: 100}` for
Bookworm Adventures, `{fmod3XM: true}` for Bonnie's OXM).

### Importing saves from an installed game

A player who has played an original on this computer can bring its saves, scores, players and settings into the port's
user folder (natively `%APPDATA%\byo-pop\<Game Name>`, on the web `<persist>/userdata`), in the browser and natively.
Both read one table, `platform/saveimport.json`, which says for each game which registry values and files of the
original are the player's save and where each goes; nothing else is taken, and the original's registry and folders are
only read. An original keeps them under `HKLM\SOFTWARE\WOW6432Node\PopCap\<Game>` (or its VirtualStore copy, for a game
not run as administrator) or `HKCU`, and in its install folder, its VirtualStore copy or
`C:\ProgramData\PopCap Games\<name>`; the Mahjongs' `%LOCALAPPDATA%\JollyBear\...` is not searched natively yet.

**The table** has one entry per game id (the tile's `data-game`, and the string `main.cpp` passes). CMake turns it into
`saveimport-table.js` for the listing and into a header compiled into every native game (`platform/SaveImport.cpp`), so
the two cannot disagree. An entry has `folder`, `steam` and `appdata` (the usual install, Steam and ProgramData folder
names) and a `note` for the page; `registry`, with `key` (the port's `mRegKey`), `hives` (where the original may keep
it, in order of preference: `vs`, `hklm`, `hkcu`), `sets` (`base`, the framework's own options; `app`, `TimesPlayed` and
`TimesExecuted`) and `values`, each `Name=type` with `sz`, `dword` or `hex`; and `files`, each with `from` (a path in
the folder that holds the saves) and `to` (the path in the user folder). A name or path may have `*` (any characters in
one part) or `#` (digits), and a value name may reach into subkeys (`Users\*\Alias`). A value not listed is not taken;
`skipValues`, `skip` and `skipFiles` say why something that looks like a save is left out (installer values,
`InProgress` and the like).

**How a value becomes the port's.** `userdata/registry.ini` has a section per key (`[SOFTWARE\PopCap\BigMoney]`) and a
line per value: `Name=sz:text` (Windows-1252, with `\\`, `\n`, `\r`), `Name=dword:123` (signed decimal) or
`Name=hex:0a1b`; a value of another type than the table's is left out. The file is written as the port writes it
(`SexyAppBase::GetRegistryText`), so the port changes nothing when it saves.

**On the web** `web/saveimport.js`, a wizard in the Saves section, reads the folders and the `.reg` files of the
`reg export` commands it names and writes as a restore does (`writeFiles` in `web/saves.js`). **Natively** each game's
`main.cpp` calls `Platform::RunSaveImportIfRequested` (`platform/SaveImport.h`): `-importsaves[=<install folder>]`
imports and exits without starting the game, copying what it replaces to `<user dir>\before-import-<date>\` and writing
`<user dir>\import-report.txt`; its exit codes are 0 (done or nothing to import), 1 (a file could not be written) and 2
(no entry for the game, or not on Windows). `tools/saveimporttest.js` (`unit-web`) checks the table (every game, no
path that leaves the user folder, no installer value taken), the .reg reader, the merge and the conversion.

### Accessibility

The pages are built to be used from the keyboard and with a screen reader (`docs/MOD_MENU_DESIGN.md`, 5.6): one `<h1>`
and no skipped heading level, landmarks, real buttons and links, every control labelled, the cards as dialogs, and
"Skip to game" first on a game's page. Text passes WCAG AA (4.5:1) on every surface and the focus ring 3:1; state is
never colour alone; every animation stops under `prefers-reduced-motion: reduce`.

**How the game gets keys, and how to leave it.** The shell sets `SDL_EMSCRIPTEN_KEYBOARD_ELEMENT=#canvas`, so the game
gets keys only while its canvas has the focus, and then every key, Tab included, except F11; **F6** and Shift+F6 move
the focus out (with the speed keys on, Shift+F6 alone; "Game speed"), and the game never sees them, nor Alt+\` (the window switch). Moving the focus to a control on the page is not the game losing the
focus: SDL and the framework (`EmscriptenFocusCallback`) take that from the window's `focus` and `blur` events.

**The checker** is `tools/a11ycheck.js` (`unit-web`), which reads the built pages with no browser; markup that scripts
build is checked in a browser with `A11yCheck.checkHtml(document.documentElement.outerHTML, { live: true, kind: "index" })`
(`live` leaves out what is hidden at that moment; `kind` names the page's kind, since the music's canvases would make the
front page look like a game's).

**Desktop 95's windows** are what a screen reader meets on the front page, and its scripts build them. Each is a region
named by its title bar, whose text is the window's `h2` under the page's one `h1`, so the heading keys go from window to
window. A window is a `section` (a region by being named, with no role written) and its title bar the section's `header`
(a `div` in System Information's `footer` and in a dialog, where a `header` would be a second banner for the page); a
box or a sheet that opens over the desktop is a `dialog` (the message boxes, Properties, the volume, the copy, the
shutdown screen, and a game page's cards); a status bar is the window's `footer`. The task bar is a `nav`, its clock a
`time`. Every menu (the Pop menu and its games, the context menus, a window's menu bar) is a `menu` with the menu role, each
entry in an `li` that is no item to a screen reader (role `none`) and lays no box (`web/skins.css`), and a rule between
entries is an `hr`. On the desktop the windows are moved into the listing's `main`, which draws nothing of its own, so the
page keeps its main landmark (`web/skins.js`, `mainOf`); System Information's `footer` and the chat's `aside` stay beside
it. Lists of things (the Library's games, Splits' segments) are tables, the Library's a grid whose names are its rows'
headers. A panel that the narrow page shows as a tab (Music, Settings, and the Settings sections) drops its `tabpanel`
role while it is a window. A landmark of its own (the games' `main`, System Information's `footer`, the chat's `aside`) keeps its
role and takes the title as its name. A heading inside the window that repeats the title goes quiet: a screen-reader-only
one is hidden, and a visible one (the chat's collapser) loses its heading role. When the desktop goes, every attribute is
put back (`web/skins.js`, `named`). **The desktop's check**, `tools/web/deskcheck.js`, opens every window in turn on a
development host served by `tools/webtest/serve.py` (`index.html?silent=1&driver=tools/web/deskcheck.js`, at least 760
pixels wide). For each window it checks that structure and runs the checker on the live page. The result is in
`window.byopopDeskCheck.result`, and the page's title ends with "deskcheck: PASS" or "FAIL". The chat's window is checked
closed, because opening it starts the chat.

Five more checks run the same way, each named in `driver=` and each reporting through the same result and title.
`tools/web/closecheck.js` closes game windows whose page never answers the close: a page that is not there, a frame that
could not load, and a page that takes the close but never hands the window back. The window must go at once, or, for the
silent page, after its four-second wait, and at once on a second Close (`web/deskapps.js`, `closeApp`).
`tools/web/splitscheck.js` resets the speedrun timer with No, with Escape, after an abandoned run and after an Undo, and
compares the stored splits with what they were before the attempt. It then answers Yes and checks that the run's times were
kept (`web/splits.js`, `reset`); it removes its own test splits at the end. `tools/web/autosplitcheck.js` opens a game with
an auto splitter (named in localStorage, `byopop-autosplitcheck`, since the page rewrites its address; BookWorm when unset)
in a desktop window with the test kit in its frame (`player=1&assets=1&fast=1` added to the window's own address), plays
runs of its speedrun.com categories by its bot and the `goto`, `win` and `lose` commands, and checks that the Splits window,
as a player has it, reads the block, starts each run at a new game, splits on every level cleared and ends the run at the
category's end, resets on a new game (and starts the next run at once) and on a game over. Each game's plan adds its own:
BookWorm's Bookburner ends at a game over at rank 0 and resets at rank 1; Seven Seas' Easy run never starts on a Hard game. Heavy Weapon's
has no new game during a run (a player leaves the level first, a quit): its game-over box's TRY AGAIN keeps the run going
to the level's results, QUIT resets it, and its plan starts each campaign from a screen `goto` takes away (the menus,
the high scores), since one under the armory, the map or a cutscene leaves that screen up for the bot. Hammer Heads' bot
only smashes, so its plan sends the game's `botclick` before each move for the level card, the "Ready?" box and the game
over box, and presses the trophy pages and the room's Back itself; its Classic% run is cut to Level 1 and Level 25, and
`set level 25` after level 1, then `set kings 1` once that level is in play, bring the trophy room whose Go! ends it (a
stat set looks for trophies at once, and the plan presses the room only over a level in play). It also plays into a level
before `lose`, since the game checks the player's health only during play. It answers every question No and removes the
splits it stored. `tools/web/musicmutecheck.js` mutes a channel from
Music's tracker, turns the view off and on, switches tunes and comes back, and compares the tracker's header with what the
player itself has muted (`bm_mute_status`). It then sends the keyboard's media keys with Music open and closed and checks
the player's state and its sound context, not its buttons (`web/music.js`, `syncMutes` and the media keys). It needs games
added that have two module tunes. `tools/web/fitcheck.js` loads the desktop in a frame of the page whose size it sets, as a
browser window, and opens one game's window for each screen size (640 by 480, 800 by 600 and 1024 by 768, given through the
page's `Module.onGameScreen`, so no files are needed) with 2x chosen. It then makes the frame small, smaller, large and small
again. After each step the game's picture must start at its area's top left and scroll in full, be drawn at 2x where the
desktop has room for that window and at 1x where it has not, and have its whole window on the desktop at 2x; the status bar
must give the multiple drawn, and the View menu keep 2x ticked, greyed where it does not fit. It puts the choice of scale and
the desktop's layout back at the end.

### Site shots

`tools/web/siteshots.js` takes shots of the site itself for a before-and-after comparison with `tools/shotdiff`: the
desktop as it starts, each of its windows open alone, the Pop menu, the desktop's context menu, a message box, and the
pages that open as windows (Jungle Juice's, a game's copyright page, a game without its files). It shoots at a desktop's size,
a phone's width and each at 200% zoom, in the standard scheme and in grape, and the pages in frames in the browser's dark
scheme too. It runs headless Edge over the DevTools protocol, each page in a fresh browser context, on a fixed clock and a
seeded `Math.random`. With `--skin <id>` it opens every page in that skin (`?skin=`) and takes the skin's own ten shots
(docs/SKINS.md, (f)), the baseline of a skin other than 95. With `--audit` it also lists, for each shot, every element that takes clicks or keys (by the
browser's own list of its listeners) and is not a control, every field with no name, every role a native element would give
or that repeats its element's own, and every picture a screen reader would read.

**The guard** is the same audit with `--check` (`unit-web`, about a minute, one browser): the desktop at a desktop's size,
each window, the menus, a message box and the three pages in windows, with no shot taken. It fails on any of those faults
except the ones it lists with their reasons: a window's title bar and the task bar, which take a double click and a right
click as Windows 95's did, each a pointer's way to a button or a menu that the keyboard reaches too; Windows 95's block
progress bars, which tell their value in words; and Music's display, which holds the tracker's own scroll bar and so cannot
be a button. A key handler on a list that is not focusable (Music's tunes, the Library) counts as the list hearing its own
controls' keys.

## The site's text

Every word the site shows is in `web/strings/en.txt`, one `key = text` a line (the user, 2026-11-03: "I'd like to be
able to edit a single file to rewrite the prose on the site"); its header says how to write one. Not in it: the terms
(`web/terms.html`), the licences, each game's own paragraphs in its `page.js` (`importHtml`, `modsIntro`) and what the
games draw. The build fills page markup's `{{key}}` markers with `byopop_web_fill()` (HTML-escaped unless the key ends
in `_html`) and its own prose with `byopop_web_string()`; scripts read `strings.js` through `web/i18n.js`
(`ByopopStrings.t(key, vars)`). A key the build asks for and the file lacks stops the build. `tools/stringstest.js`
(`unit-web`) checks that every key used is in the file and every key in the file is used; the Node tests of the site's
scripts check keys, never wording (`tools/stringsload.js`).

## Steam on a Mac or Linux

Steam on a Mac or Linux installs only that system's version of a game, and the ports read the Windows files, but its
console's `download_depot <app> <depot>` downloads any depot of a game the account owns. A game released on Steam has a
`BYOPOP_STEAM_<target>` entry in `cmake/ByopopStores.cmake` with its app and the depot of its Windows files;
`byopop_web_shell()` puts the steps on the import card, shown when the browser says it runs on macOS or Linux.

## The site's logo and icons

`web/brand` holds byo-pop's logo and icons, which the build copies next to the pages. The SVGs (`favicon.svg`,
`logo.svg`, which `web/logo.js` puts inline, and `cap.svg`) are the sources, edited by hand; `favicon.ico` and
`apple-touch-icon.png` are made from `favicon.svg` by `python tools/brand_icons.py` (with headless Edge), run after a
change to it. The colours are taken from PopCap's classic logo; the shapes and lettering are byo-pop's own.

### The site's apps' icons

Each of Desktop 95's apps has an icon of its own, drawn by `tools/appicons.py` into `web/appicons.js` (run it after a
change; `--preview` also writes `build/appicons.png`, every icon at four times its size). They are our own drawings in
the manner of 1995's program icons, as `tools/settingsicons.py` draws the Settings sections': a few hard-edged shapes in
the 16-colour palette, outlined in black, drawn at 32 by 32 and again at 16 by 16 (each its own drawing, with the detail
a small icon cannot hold left out, never the large one scaled down), and written as SVG of one rectangle per run of a
colour, so that they stay sharp at the desktop's 2x zoom. None holds text but the Fonts window's letter A. Their colours
are the same on every colour scheme, as a program's icon was on any wallpaper, so none is drawn mostly in black or a dark
colour that would be lost on the dark desktops (Cola, Grape, Cherry): Music's notes stand on a silver disc, the A is white
inside its outline. The apps and their pictures: the Library (books on a shelf), Setup (an open box with an arrow into it), Settings (a panel of three sliders, the listing's folder and a game
page's panel alike), Music (two beamed notes on a disc),
Sounds (a board of six pads), Splits (a stopwatch), the chat (a balloon with three dots), System Information and a
game page's About (a tower computer), Performance (a line chart), the Trophy Cabinet (a cup), Fonts (a serif A), the
Gallery (a picture in a gold frame), Guides (a book with a question mark), and on a game's page Controls (a keyboard),
Game files (a folder with its pages), Mods (a wrench over a screwdriver) and Replays (a video cassette). `web/skins.js`
(`appIcon`) finds a window's app by its id less `tab-` or `panel-`, and a Settings section's window (`set-`) by its
section in `web/settingsicons.js`; the large picture goes on the desktop, in the Pop menu's first level and in Alt+Tab's
box (for a window with no desktop icon too, such as Setup or a Settings section), the small one in title bars, on task
bar buttons and in the menus' lists. A game page's Chat and panels draw their own headers, which have no control menu
and so no icon; theirs show everywhere else.

## Testing other copies of a game: `?placeholders=1`

`?placeholders=1` (`-placeholders` natively) lets a game start when some of its files are missing, with stand-ins from
`platform/Placeholders.h`; scores are kept apart and the record of the player's files is not written.

A game's part, done when the user asks for it: `main()` calls `Platform::InitPlaceholders(argc, argv)` before its
game-folder check and lets `-gamedir` through with markers missing; it overrides `SexyAppBase::GetPlaceholderSize` with
its images' real sizes (64x64 otherwise), since wrong sizes can crash or change play; an image the game may lack on
purpose loads with `GetOptionalImage`; an override of `GetModMessages` calls the base, which adds the missing-files
banner. Data files with no stand-in (a word list, a difficulty table) are the game's to handle.

## The performance panel

A game page shows the panel while the setting `perf` is on (`web/perf.js`; off by default, `?perf=1` for one visit).
It only reads, so the game plays the same with it on, and with it off nothing of it runs. It sits beside the game,
since nothing but the game and its mod menu is drawn over the game (the user, 2026-11-10), and the game window's View
menu shows and hides it (the user, 2026-11-11: "it's difficult to show/hide the performance panel on mobile"). The
game's share of the main thread is never labelled as the computer's CPU use, and the panel never asks for a WebGL
context before the game has made its own, since that would take the canvas from the game. System Information on the
listing page reads the same numbers from a game window with `ByopopPerf.watch(true)`: the readings run without the
panel and without the WebGL counts, until `watch(false)` or the panel's own switch leaves nobody reading.

## The sound panel

The setting `soundPanel` (`web/soundpanel.js`, off by default) shows the sounds the game plays, beside the game at the
right of the performance panel, and the game window's View menu shows and hides it (the user, 2026-11-13: "Add a
toggleable menu that appears beside the game like the perf panel that shows what sounds are being played in the game and
at what volume and pitch"). Like the performance panel it only reads. With it off no timer runs and the mixer keeps no
log; with it on, ten times a second it calls `byopop_sounds_view` (`Mixer::WebVoicesView` in `platform/Audio.cpp`, web
builds only), which gives the voices playing and the ones started since the last call, so a sound shorter than a tenth
of a second still gets its row. A row is one voice: the sound's file name, its volume and pan as the game set them
(before the effects volume, which the header gives), and its pitch as a rate and in semitones; a looping sound is marked,
and a row stays greyed for two seconds after its voice ends. A game built before the call existed says it needs a
rebuild.

## View, controls and play settings

These are site settings, each off or at the game's own behaviour by default. They act on the page around the game: what
is drawn over its picture, the input it is sent, and how often the framework runs its updates. None changes a port.
With all at their defaults nothing of them runs except a link in the frame hook that returns at once. The speed and the
two skip settings are never applied on the player-only page.

### View settings

`web/view.js`: `scanlines` is an overlay above the canvas, and `colorFilter` a CSS filter on the canvas, or one of two
that show the game as a 256-colour screen did (the user, 2026-11-09: "Is it possible to make a filter for the game
windows to make them appear in 256 colors only?"): `pal256` makes each pixel the nearest colour of the palette Windows
kept for programs that set none (its 20 static colours and the 6x6x6 cube), and `pal256d` first applies an 8x8 ordered
dither. Both are drawn by a WebGL canvas over the game canvas, filled in the frame hook (`Module.byopopTestFrame`).
Three more are for colour-blind players, standard mod 15's palettes made a site filter (the user, 2026-11-07: "Yes,
move them"): `protan`, `deutan` and `tritan` recolour the picture by daltonization, an SVG colour matrix in linear RGB
named by the canvas's CSS filter. Each matrix is worked out in `web/view.js` from the simulation of that deficiency
(Machado, Oliveira and Fernandes 2009, severity 1) and a shift of what it loses into channels the player still tells
apart; the comment there gives the shifts and how far apart they put colours the simulation confuses.
The scanlines lie over the canvas's own box, not the whole stage (in full screen the canvas is centred in a larger
stage), with one line a game row high every two of the game's rows: the rows come from the game's size (`--aspect`) and
the height from the canvas's drawn box, never from the canvas's pixels, which SDL sizes on its own time (300 x 150 before
the game makes its window, the old size for a while after a resize) and which made the lines briefly too large (the user,
2026-11-21). They are sized again whenever the canvas or the stage changes size, the stage's style changes, the window
is resized or full screen begins or ends.

### Controls

The Controls panel (`web/controls.js`) lists the game's controls in our own short words, from `web/controldata.js`,
keyed by game id, whose header gives an entry's form; only keys the game's own code handles are listed. The same table
drives key remapping (`byopop-keys`), the gamepad (`gamepad`, read once a frame) and the on-screen keys (`touchKeys`);
`inputDisplay` shows the keys and mouse buttons the game is sent. Where a game's entry starts with `{uses}`, which names the
mouse buttons its own code handles and whether it takes typed letters or any key, the display shows only that input and
its listed keys, so the framework's own keys and the buttons the game ignores drop out; a game without one shows
everything.

### Phones

A phone is a touch screen whose short side is under 600 CSS pixels (`PHONE` in `web/shell.html`; `?phone=1` on a
development host for tests). There (the user, 2026-11-10: "the games should all default to full screen with pixel
perfect mode off") the game is as large as its window allows, and Play asks for full screen unless the "Full screen"
setting is off. The touch keys' keyboard button opens the screen keyboard through a hidden text field that is read-only
the rest of the time, so a tap on the game never brings the keyboard back (the user's note of 2026-11-11). "Touch keys
position" is `touchPlace`.

### Game speed

`speed` (any value from 0.01 to 16; the user, 2026-11-10: "Allow game speed to be set to any arbitrary value") sets
`SexyAppBase::SetGameSpeed` through the web-only `byopop_set_speed` (`framework/SexyAppBase.cpp`). 0 is "as fast as the machine
allows": the updates run flat out in real time and the draws that cannot be shown are skipped; the list offers it as "Max", the
native mod menu's word for it (the number field takes 0.01 to 16 and stands empty at 0). The list's other entries are the native speed
keys' steps (`SexyAppBase::SPEED_STEPS`). At 1 nothing is called, so unmodded play is unchanged. The game clock follows the speed:
`Sexy::GetTickCountMS` (what `timeGetTime` and `GetTickCount` are in the ports) and `SexyAppBase::GetClockTime` are game
time, real time scaled by the speed and accumulated, so a change of speed mid-game leaves the clock continuous. The framework's
own pacing (the update accumulator, draws, sleeps) reads `Sexy::GetRealTickCountMS`. A game whose own host runs its loop
(Bonnie's Bookstore, Ning Po) has no hook yet, and on their pages both speed settings are greyed
(`ByopopSettings.ownSpeed`), with a line under the speed that says the game keeps its own. Music keeps its own time at every speed (tracker and Ogg alike; a fade the
framework steps per update is scaled back to real time, `SexyAppBase::MusicStepFactor`); sound effects keep their behaviour.
The native build has the same speed in the mod menu and on F6/F7/F8 (`docs/MOD_MENU.md`), and `tools/check.sh speed` checks
that every game follows it.

**The speed keys** (`speedKeys`, off by default; the user, 2026-11-17: "Add optional hotkeys for game speed (probably using
function keys)") are the native ones: F6 slower, F7 faster, F8 back to normal, without modifiers, while the canvas has the
focus. They step along the same list from a speed between two steps to the next one in that direction, past 16 to unlimited
(Max); below the lowest step, F6 leaves the speed where it is. The page
takes the press and its release in a window capture listener (`web/controls.js`), so neither SDL nor the page's own F6 sees
them, and the web build's mod menu has no speed rows, so nothing steps twice. The new speed is written as this game's own
setting and shows over the game for a second and a half in a raised grey box. With the keys on, Shift+F6 still takes the
focus out of the game, and the hint shown while the game has the keyboard says so.

### Clips

`clips` puts a Record button on the bar (`web/clips.js`), which records the canvas and the game's sound
(`Module.byopopAudio.node`) with MediaRecorder and saves the clip to the player's computer; nothing is sent anywhere.

### Always skip tutorials

`skipTutorials` turns the game's own tutorial switch off at each start. The port registers a function for it in its
web-only file (`Byopop::gSiteTutorials`, `framework/WebSettings.h`). The page calls `byopop_site_tutorials` until the
function answers 1; a game whose switch belongs to a player answers 0 until one is loaded, and a game with no function
answers -1 and is left alone. The site never turns a switch back on. Each game's session registers its own.

### Always skip start-up logos

`skipLogos` ("Skip start-up logos after the first launch") never applies to a game's first launch in this browser,
which shows its logos once (the user, 2026-11-10). After that the games go straight past their start-up logos (the
PopCap logo, a publisher's or developer's logo, a "presents" splash; not the loading bar or the title's own animation),
each as a click or key on it would, so play afterwards is the same. A game reads `Byopop::SiteSkipLogos()`
(`framework/WebSettings.h`), which the page sets with `byopop_site_skiplogos(1)` and which natively is true under
`-skiplogos` (a game whose own host parses its command line calls `Byopop::SetSiteSkipLogos(true)` itself). Each game's
session names what its game skips when it merges.

## The player-only page

For sessions that test a game in the Browser pane (the user, 2026-11-07: "build a stripped down version that loads
ONLY the player"). It is a game's own page with `?player=1`, and exists only on a development host
(`ByopopTerms.devHost`); elsewhere the parameter does nothing and `web/testkit.js` is never loaded. The page shows the
game alone at the top left, one canvas pixel per game pixel. It is silent unless `?silent=0`, each frame is the
browser's or a 20 ms timer's, whichever comes first, and plays are not counted. Its parameters:

- `player=1`: the page and its tools, `window.byopop.test`.
- `assets=<name>`: the game's files from the staged `assets/<name>/` (`assets=1`: the page's own id), taken through the
  page's own import into a test profile (the storage and Web Lock names with "-test"), apart from the player's.
- `fast=1`: the game on its virtual clock (`-fast`), on the page's own frames. `framems=N` is the longest a frame runs
  the game's steps (25 ms by default); `frames=browser` uses the browser's frames instead.
- `hold=1`: the game waits for `byopop.test.start()`, so a test can wipe or seed the profile first.
- `silent=0`: sound on, only when the user says so. `realfocus=1`: the browser's own blur, focus and visibility events
  reach the game.
- `driver=<path>`: a driver script ("Drivers"). `run=<id>`: the folder of `shot(name)`'s files under `build/webshots/`
  (default "default"). `seed=N`, `placeholders=1`: as on the full page.
- `env=NAME=VALUE,...`: environment variables the game reads (a native run's switches, `BYOPOP_STREAM_MUSIC=0` for
  one), set before the runtime starts.

`python tools/webtest/serve.py [--port 18780]` serves `build/web` on the loopback address only, with the staged files at
`/__assets/<name>/`, the repo's driver scripts at `/__repo/` and `POST /__shot`, which saves
`build/webshots/<run>/<name>.png` for `tools/shotdiff` (`docs/SHOTDIFF.md`).

**Fast runs.** A hidden Browser pane sends no animation frames, so with `fast=1` the page drives the game itself, each
frame ending once the game's steps took `framems` (`byopop_set_frame_budget`); neither changes an update or a draw. A
run that compares states with another sends its starting commands in the same update (`Promise.all` of `command()`
calls), since the updates between two commands sent one after another depend on the frames.

**`byopop.test`** (`web/testkit.js`) gives a test the game's errors, input as the DOM's own events at game
coordinates, a hold on the updates (`byopop_hold_updates_at`), shots, focus and visibility of its own (the browser's
stop in the kit unless `?realfocus=1`, since a hidden pane sends a real blur), the game's printed log, readings and the
test profile. The test commands (`docs/TEST_COMMANDS.md`) go through `command(line, {timeout})`, which sends one
through `byopop_test_command` to the game's `HandleScriptCommand` and resolves with the lines printed up to its answer
(`STATE {json}`, `MOVE {json}|null`, `OK <command>` or `ERROR <command>: <reason>`), and `state()`, `botMove()` and
`playMove(m)`, which sends a move's `input` steps as real DOM input (`{move:[x,y]}`, `{click:[x,y]}`, `{press:[x,y]}`,
`{release:[x,y]}`, `{key:"B"}`, `{wait:n}`).

### Drivers

Game sessions play their real-input checks with driver scripts (the user, 2026-11-07: "Yes, use drivers"): a script in
the page reads the game's state, chooses its moves and sends them as real DOM events, so a whole game costs a call or
two. A driver is `games/<game>/tools/<path>.js` or a shared `tools/web/<name>.js`, named with `?driver=`. The page runs
it once the game is ready (on a `hold=1` page, once the files are in, and it starts the game itself) as the body of an
async function of `t` (`byopop.test`); what it returns becomes `byopop.test.result` (`{error}` if it threw), and
`byopop.test.driver` is a promise of the same. A game's own driver runs a shared one with its table by
`return await t.run("tools/web/<name>.js", {...})`, the table arriving as `args`.

**The built-in driver `?driver=move`** plays any game by its port's own bot (`docs/TEST_COMMANDS.md`, "The bot behind
`move`"). It loops `state()`, `botMove()`, `playMove()`, so every move is real input. It stops when the state says
`"over": true`, on `MOVE null` twice running while the game is not busy, on an ERROR, or after `?steps=` moves (2000 by
default); while the state says `busy`, `MOVE null` means "not yet" and it waits 5 updates. With `?begin=mode`, the first
`MOVE null` twice running while the state names a `mode` (the game on its title or menus) sends `goto <mode>` once and
goes on, as the native loop's `--first` does. `?begin=<name>` sends `goto <name>` instead, for a game whose state names no
mode off its board; a goto the game answers with an ERROR (its title not yet gone, a dialog in the way) is sent again
after the next two empty moves, up to 20 times, and one it takes gets about 24 seconds of empty moves for its board to
load before the driver stops. `?prep=<command>;<command>` sends those test commands before the first move (`set mode
action`, `3d on`), one the game refuses being sent again after each move, up to 20 times (Ning Po's start dialog), and
`?pace=<n>` waits n updates after each move. The screen saver uses all three. `?clickgap=1` spaces a move's clicks as a
player's are, the pointer's move onto the point, the press and the release each followed by two frames, where a click
otherwise sends the move with the press; the saver sets it for Bonnie's, which misses some tile clicks sent the quick way.
The result is
`{moves, stop, state, updates, errors}`, with `stop` one of "over", "limit", "no move", "busy" or "error: <message>".
`playByMove({steps})` runs the same loop from a session's own driver.

### The page check

`tools/web/pagecheck.js` checks how a game's page behaves as a browser page, which no comparison with the original
reaches. Every game takes it in its real-input pass, through a table of its own in
`games/<game>/tools/web/pagecheck.js`, opened as

    <game>.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/<game>/tools/web/pagecheck.js&run=pagecheck

with `tools/webtest/serve.py` serving it (the launch configuration `pagecheck-webtest`, port 18780). It reloads the page
partway, so `await byopop.test.driver` first resolves with `{reloading: true}`; read `byopop.test.result` after the
reload for `{pass, fail, info, results}`. A run takes about three minutes in a hidden pane. The table's main fields
(the script's header lists them all):

- `start`: the commands that start a game.
- `focus`: the state keys a lost and regained focus changes in the native build, the line
  `tools/web/pagecheck_native.py` (`--first`, `--title-move`) prints; the page is hidden and shown mid-play and must
  change the same ones.
- `keys`: keys the page must cancel besides Tab, Backspace, Space, the arrows, Page Up and Down, Home, End, `'` and `/`.
- `point`: where the mouse goes at 1.5, 0.75, 2 and 1 times the canvas's size; the `pointer` command must answer it,
  give or take one game pixel.
- `moves`, `reloadChanges`, `afterReload`: the bot's moves before the reload, after which every save file must come
  back with the same bytes unless the page's backup restores a newer one or `reloadChanges` names it, and a check of the
  game's own.

It also checks that the same game opened again in an iframe waits for the first (the Web Lock) and runs no update.

While a game has the focus, SDL cancels the function and Ctrl keys, so F5 and Ctrl+R reload nothing until the player
clicks outside it or presses F6.

## Sound

Every game's sound goes through the shared mixer (`platform/Audio.cpp`), which on the web plays through an AudioWorklet
of the page's own, keeping a short queue that it refills on demand, rather than SDL's ScriptProcessor, which made sounds
late; a page with no AudioWorklet (plain http from another machine) falls back to a ScriptProcessor pulled the same way.
A hidden tab's game is frozen (the user, 2026-11-16: "Freeze games in hidden tabs. Only the music player should continue
while hidden"). The browser gives a hidden page no animation frames, so the game neither updates nor draws;
`SexyAppBase::EmscriptenPageHidden` mutes it at once, as a lost focus does; and `web/shell.html` suspends the game's
AudioContext, so its mixer stops too. Mixing silence had cost 0.24 to 0.51 s of processor time for every 10 s hidden (2 to
5 % of a core); suspended, 11 games held hidden took 0.01 to 0.04 s and sent no sound at all. Shown again, the game's music and any long sound go on from where they were when
the tab was hidden. That differs from the originals, whose sounds move on in silence while the game is unfocused, and it
follows from the user's choice to freeze. The site's Music player has an AudioContext of its own in the listing page and
plays on in a hidden tab: nine minutes hidden in headless Edge with no stall, and its page's timers still fired once a
second past the five-minute mark, where Chromium can cut a silent hidden page's to once a minute. Only the browser's own `visibilitychange` suspends: a test's
pretended hide is not trusted, and the test kit stops the browser's own unless `?realfocus=1`.

For tests, `Module.byopopAudio` has the output's state (`kind`: `worklet` or `script`; `target`, the queue's length;
`gaps`, how often it ran dry; `late`). `Module.audioOutput = 'script'`, set before Play, forces the ScriptProcessor. A
function in `Module.onVoiceMixed` is called with the page's clock at the play call when a sound's first frame is mixed,
and `Module.audioChunkEar` holds the time that frame reaches the speaker. `?silent=1` mixes as usual and sends silence,
so the timing is the same.

## Memory use

A game takes more memory in a browser than natively: IDBFS's copy in memory of every stored file, decoded images held
both in the game's memory and as WebGL textures, and WebAssembly memory that never shrinks after the loading peak. A
picture's decoded pixels are dropped once its texture is on the GPU, as natively (`docs/MEMORY.md`, step 1), and
pictures are decoded one at a time. Against the peak, the heap grows 4 MB at a time (`-sMEMORY_GROWTH_LINEAR_STEP` in
`cmake/ByopopWeb.cmake`) rather than by Emscripten's fifth of itself, and an Ogg sound is decoded straight into its own
samples (`Platform::Mixer::LoadSoundData`), so that a long track no longer needs three times its size for a moment.
`tools/webtest/webmem.js` measures a game at its first board, and `docs/MEMORY.md` ("The web builds") has every game's
figures.

Reading the game's files from IndexedDB only when the game opens them, rather than holding them all in memory from the
start, was dropped (the user, 2026-11-16: "Drop it, record why"). The game would have to wait on a read in the middle of
its own code, and both ways to let it cost more than the reading would save. Asyncify slows every game whether or not
it ever waits: AstroPop, Bejeweled Twist and Venice, linked again with it and nothing else changed, grew their wasm by
42 to 52 % (30 to 36 % gzipped), took 11 to 38 % longer to reach their menus where the processor sets that time
(Venice's logos set its own), and took 16 to 21 % more processor time held at a board and 9 to 41 % more from the start
to it, against the ports' target of the originals' processor time. JSPI works in Chromium alone, and as linked it stops
at the runtime's start; every way into the wasm (the start, `main`, the frame callback, the page's exports and the test
commands) would have to become an asynchronous export, and the page's calls into the game asynchronous with them. At
best it would save the game's files held in the page, 6.7 MB for AstroPop, 42 MB for Bejeweled Twist and 34 MB for
Venice, kept as JavaScript buffers beside the WebAssembly memory, less the files a game keeps open, such as a streamed
track.
