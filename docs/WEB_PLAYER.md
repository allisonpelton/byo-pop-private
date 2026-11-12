# How the web player works

What a browser loads when it opens the site, how the pieces talk to each other, and the user's rules for the site. The
site is a listing page drawn as a 1995-style desktop ("Desktop 95") and a page for each game, built by
`cmake/ByopopWeb.cmake` into `build/web`. Native builds share the game code; only the outer layer differs. How a test
opens the pages (`?silent=1`, the test key, the Browser pane) is in CLAUDE.md, "Running games in tests".

## The pieces

For each game (`bejeweled` as the example) the build writes `bejeweled.html` (`web/shell.html` with the game's
`web/page.*`), `bejeweled.js` (Emscripten's loader, `Module`, with `web/modpanel.js` appended; readable with
`BUILD_TYPE=Debug tools/build.sh web`) and `bejeweled.wasm` (the game with `framework/`, `platform/` and SDL2). The
player's game files are copied into this browser's storage, never uploaded and never part of the build. A new site
script in `web/` goes into `BYOPOP_WEB_SHARED_JS` (every page) or `BYOPOP_WEB_INDEX_JS` (the listing page) in
`cmake/ByopopWeb.cmake`.

## What happens when a game's page opens

The page loads `<game>.js`, which compiles the wasm while `main()` waits (`INVOKE_RUN=0`). IDBFS mounts the game's
IndexedDB at its persist folder and copies it into memory (`game/`, `userdata/`, `mods/`, `replays/`), first putting
back saves that reached only the localStorage backup (`byopop_userdata_restore`). A Web Lock (`PAGE.lock`) lets one tab
run a game. The fixes mod is put in place, the terms are asked, and without the game's files the import card copies
only what `page.js` names, by the rules in `web/gameimport.js` (a pak named in `paks` is unpacked). Play calls `main()`
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
`@BYOPOP_TITLE@`; `byopop_web_shell()` adds the footer, the terms and the styles and writes `<game>-shell.html` (emcc's
`--shell-file`) and `build/web/<game>-copyright.html`.

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
`byopop_cursor_set_copy()` into `byopop-cursors`; the choice is `byopop-site-cursor`. A game's own cursor wins on its
page. A kind may carry animation `frames`, which the page plays itself (Banana Bugs will need them,
`docs/POPCAP_GAMES_SURVEY.md`).

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
comment) and `BYOPOP_WEB_WIN95` (yes, no or unknown). A tile carries them as `data-game`, `data-title`,
`data-released` and `data-win95`, and `data-unsold` where no store sells the game.

The user's rules for it:

- **Games no longer sold** are hidden unless "Show games no longer sold" is ticked or the game's files are here (the
  user, 2026-11-03: "default to hiding those games from the front page").
- **Windows 95 games only** (`win95Only`) hides the games whose original did not support Windows 95, and the unknown
  ones; a game released before the last one whose readme names Windows 95 (TipTop, 2002-12-12) counts as yes (the
  user, 2026-11-08).
- **Play time** counts only while the game runs and its page is shown and focused (the user, 2026-11-02: "Only count
  time with the game open and active"): `byopop-play` (`ByopopPlay` in `web/settings.js`).
- **A game's icon** is read out of the player's exe (`web/exeicon.js`; `importIcon: true` in `page.js` for a game that
  does not store its exe) and never sent anywhere; a game without one shows its initials on a plain white square (the
  user, 2026-11-08: "bare initials for missing games").
- **Buy links** are hidden once the player has ever loaded files for the game or played it here, so that the page never
  suggests it is for copies not bought.

**Which games are yours** is `byopop-game-files` in localStorage (`ByopopGameFiles` in `web/settings.js`), never
written with `?placeholders=1`; without an entry the listing looks in the game's database read-only, calling
`indexedDB.databases()` first so that it never creates one. Forgetting a game's files (`ByopopImport.forgetGame`)
keeps saves, mods and replays and holds the game's Web Lock.

### Jungle Juice

The site's second page, which will hold games decompiled by other projects (the user, 2026-11-04); for now a
placeholder with no third-party content. `byopop_web_jungle()` writes each `web/jungle/*.html` into
`build/web/jungle/` with its `{{key}}` markers filled from the site's text (`jungle.*`).

### Adding games from the listing page

`web/addgames.js` adds many games at once: a folder or .zip dropped on a finished game's tile is that game's, and
anything else goes to the Setup window. **Sorting** (`ByopopImport.sort` in `web/gameimport.js`): a folder is a game's
when it holds every one of the game's markers, and a marker may name alternatives (`"WinTS.exe|TyperShark.exe"`, Typer
Shark's Steam and PopCap Game Collection exes); a folder of several games is looked into three levels deep; where two
games' markers are at the same place the folder's name decides, else the player is asked. **Copying** writes the files
straight into the game's IndexedDB database in IDBFS's records (`FILE_DATA`), under the game's Web Lock and all or
nothing, the marker files last. It follows the game page's own rules: `byopop_web_index()` writes each listed game's
`page.js` into `import-games.js`. `tools/gameimporttest.js` (`unit-web`) checks the rules and the sorting with every
finished game's `page.js`.

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
   a tooltip, in About, or nowhere.
2. **A recognised icon, alone,** where a widely known one exists, drawn in our own pixels; its name is its tooltip and
   `aria-label`.
3. **Status is data in a display,** not a sentence. A test mode such as `?silent=1` shows a small indicator there.
4. **Views belong to the display.** Clicking it cycles them, with no check boxes.
5. **Credits and explanations go to About** and the licences page.
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
un-95"), in our own SVG.

**Windows.** A page is a desktop (`ByopopSkin.desk`) whose game, chat and panels are windows, added with
`ByopopSkin.desk.add(element, {id, title, ...})`. The desktop never scrolls, and F6 moves between its panes. The game
window's menu bar spreads the game's commands over Game, View, Options and Help (the user, 2026-11-09: "don't hide
everything under "Game""), and no menu shows Alt+F4 or Alt+Space, which the browser keeps. The game is always at a
whole multiple of its size in device pixels (`sizeStage`); the window takes the status bar's height rounded up
(`statusBarHeight`), and the canvas is focused with `preventScroll` everywhere, so that the game's area is never a
fraction short and a press on the game never scrolls it (the user's note of 2026-11-11). The framework offers the
game's own full-screen request to the page (`sexyapp_page_fullscreen` in `framework/SexyAppBase.cpp`), because SDL's
full screen would stretch the canvas.

**Its text** is Byopop Sans, `web/fonts/byopop-sans.woff`, built by `tools/web/pixelfont.py` from W95FA by Alina Sava
(SIL OFL 1.1, as is ours); after a change to the script, run it and commit the file. The font is sharp only at 11
pixels and its multiples, so every text is set at 11 (a few at 22), and a new part sets no size of its own. A glyph's
body has one pixel of bearing on its left, as W95FA's have but for y, ý and ÿ, which the script moves right a pixel (the
user approved it, 2026-11-11, of "ry" touching).

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
- **Up to three games run at once,** each in its own frame with its own module and saves. The window in front has the
  focus, and the other games' frames get the window blur the framework listens for (`EmscriptenFocusCallback`), so
  each behaves as a page that lost the focus.
- **The chat is one window for every game** ("The chat"), a message tagged with the game in front.
- **Closing a game** returns to the window the setting `gamesOpen` names (All games or the Library).

### The Library

The listing's second window of the games (`web/library.js`): a sortable grid evoking a game client of 2004, named for
no real program, built from the listing's own tiles and `ByopopPlay` so the two windows agree. A game's Properties
(its menu) are a property sheet of its values and, below them, a table of each site player's plays, time, first and last
day (the user, 2026-11-11: playtime per profile). `ByopopPlay` (`web/settings.js`) keeps them in `byopop-play`, by the
current player's id in `by` beside the totals, and the save backup keeps and merges them (`web/saves.js`, `mergedPlay`).

### Desktop themes

Settings > Appearance > Theme (`web/themes.js`, its table `web/themedata.js`; the user, 2026-11-11) gives the desktop one
game's look, as a 1995 Desktop Themes pack did: one of the game's own backgrounds as the wallpaper (in place of the pattern
and wallpaper, at the wallpaper's Display), the game's own cursor set (`web/cursors.js`, once its page has kept it), and
sounds from its soundboard for the window events that `web/skins.js` and `web/deskapps.js` send as `byopop-desk-event`
(open, close, minimize, restore, maximize, menu, ding). A theme is listed for each game in the table whose files are in this
browser, by the game's title. Everything comes from the player's own files at run time: the listing page, where they can be
read, prepares the chosen theme into `byopop-theme-wallpaper` (a JPEG data URL) and `byopop-theme-sounds` (the files' bytes,
each at most 96 KB) for every page; None, or the game's files forgotten, removes both. The sounds play at the Sounds
window's volume and mute and never at `?silent=1`.

A wallpaper the browser cannot decode as it is goes through `web/gamepics.js`: an image of a Retro64 bank (Cosmic Bugs,
Water Bugs; the table's `"<bank>.dat|bbk:<name>"`), raw pixels that the `.bbk` index beside it places and gives a palette;
and a JPEG 2000 picture (Bejeweled Twist's `.jp2`), by the framework's own decoder (`platform/openjpeg/JP2Decoder.cpp`) built
on its own as `jp2.js` and `jp2.wasm` (`web/jp2/jp2_web.cpp`, about 250 KB), loaded only when such a theme is prepared.
Both are tested by `tools/gamepicstest.js` (unit-web) on pictures made for it. The wallpaper becomes a JPEG, so a
picture's alpha mask is not read: each wallpaper chosen has none.

### The screen saver

Settings > Screen Saver (`web/saver.js`; the user, 2026-11-11) has the game, None by default, then each game with a bot
whose files are here; the Wait in minutes; and Preview. When the listing's desktop has had no key, mouse or touch for the
Wait, it goes dark and the game plays itself silently in a frame over it; any key or mouse movement ends it and leaves
everything as it was. The frame is the game's player-only page at `?saver=1`, which reads the player's own files and writes
nothing back (its `Storage` writes and IDBFS syncs do nothing), so a saver's games touch no save, high score or record.
Its page centres the game on black (`html.saver`), and a key pressed while the game's canvas has the focus ends the saver
too. The driver runs with `?begin=mode`, which starts a game still on its title or menus in the mode its state names.

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

### The fixes mod on the site

Every game is served with its fixes mod on (`docs/MOD_ROADMAP.md`, "The fixes mod"): one mod, `fixes`, each of whose
settings toggles one fix, in `games/<game>/mods/fixes`. The web build copies it beside the page as `<game>-fixes/` and
writes its `version` and file list into the page (`FIXES` in `web/shell.html`); a game in `BYOPOP_WEB_NO_MODS`, or
without the folder, gets none. Before the game can start, the page puts the mod into `/persist/mods/fixes`, on, its
fixes at their defaults, and notes it in the game's user dir, `site-fixes.json` (`{"installed": "<version>"}` or
`{"removed": true}`). A newer shipped version replaces the mod's files and leaves `mods.json` and `mod-settings.json`
alone, so the player's choices hold. A player who removes the mod never gets it back: a start that finds the note
saying "installed" and no folder notes it as removed. `-DBYOPOP_WEB_TEST_FIXES=<game>` (set back to empty afterwards)
gives a game with no fixes mod `tools/webtest/fixes`, a stand-in with one toggle that changes nothing.

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
- `pauseHidden` (off) stops a hidden tab's game; off must stay as it is, since the originals keep running unseen. A
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
the page, nothing uploaded, and reads one back. It takes each game's `userdata/`, its `replays/` for games with
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
their own files in this browser; the site ships no music and sends nothing anywhere.

**Music's data** is `web/soundtracks.js`, by game id, whose header gives an entry's fields; tunes are named plainly,
never with invented titles. A game's session sets these from its code:

- `where`: where the game's code plays a tune, named as a game music rip names it ("Main Menu", "Level Theme 1",
  "Danger 2": Title Case, one place, at most 20 characters, several tunes of one place numbered in the order the game
  reaches them; the user, 2026-11-11: "Rip style please!").
- `unused`: the tunes the game's code never plays.
- `cuts` and `chains`: tracks split and grouped as the game plays them (the user, 2026-11-11: "Group and split").
  `cuts` splits a subsong where the game's code starts a different tune partway through it, the second piece being
  `"<file>#<n>@<order>"`; `chains` makes one track of parts the game only ever plays as jumps inside one tune.
- `level`: "Music levels".

**The decoder** is the games' own two module renderers, built for the web as `openmpt.js` and `openmpt.wasm` (target
`byopop_openmpt`) and picked as `Platform::Mixer` picks them, so a tune sounds as it does in its game.

**The player's rules**, from the user: long silent endings are skipped (the user's notes, 2026-11-10: "Some of the
short tracks have a bunch of dead air at the end, where the tracker slows to a crawl"; `trimEnd`); only the playlist
scrolls (2026-11-09); closing the window stops the music and frees what it held (2026-11-09;
`ByopopMusicPanel.held()` tells a test what is still held).

**The views**, cycled by a click on the display (spectrum, tracker, piano roll, score, nothing), follow the user's
notes:

- **The spectrum** is steadied (the user, 2026-11-10: "a little too jittery").
- **The tracker** names the instrument over each channel (the user, 2026-11-11: labels on the columns). A channel's
  whole head toggles its mute (the user, 2026-11-11: "It's too hard to click on the channel to toggle the mute"), and a
  Ctrl-click or a double-click solos it (the user's notes, 2026-11-10: "tracker labels and mute"; the user, 2026-11-11:
  "Add double clicking a tracker channel to isolate its sound"), through the decoder's `bm_mute` or a followed game's
  `byopop_music_mute`. Channels that play no note in the tune are hidden, keeping their numbers (the user, 2026-11-11:
  "Some songs seem to have totally empty channels the whole way through. Possible to hide from tracker on a per-song
  basis?"; `tuneUsed`). The FX button adds an effect column (the user, 2026-11-11: "add a brief English description of
  tracker commands when they appear in the song as an optional column"), its words `music.fx.*` in `en.txt`.
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
made active, while open and not minimized (`frontFrame` in `web/deskapps.js`).

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
change. `ByopopMusicPanel.following()` tells a test the game followed and its view.

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

### The splits timer

The Splits window (`web/splits.js`) stands in for LiveSplit (the user: "a LiveSplit stand-in on the site so that people
really could do speedruns on it"), its splits kept in IndexedDB (`byopop-splits`) and imported and exported as
LiveSplit `.lss` files. Game Time is the update count of the game window in front when the run starts
(`byopop_update_count`) times the game's update step (10 ms; 5 ms in the Mahjong games); a game without the count has
none. There is no autosplitting: no web build has the desktop builds' LiveSplit block (Bejeweled's
`features.livesplit`).

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
the focus out, and the game never sees them. Moving the focus to a control on the page is not the game losing the
focus: SDL and the framework (`EmscriptenFocusCallback`) take that from the window's `focus` and `blur` events.

**The checker** is `tools/a11ycheck.js` (`unit-web`), which reads the built pages with no browser; markup that scripts
build is checked in a browser with `A11yCheck.checkHtml(document.documentElement.outerHTML, { live: true, kind: "index" })`
(`live` leaves out what is hidden at that moment; `kind` names the page's kind, since the music's canvases would make the
front page look like a game's).

**Desktop 95's windows** are what a screen reader meets on the front page, and its scripts build them. Each is a region
named by its title bar, whose text is the window's `h2` under the page's one `h1`, so the heading keys go from window to
window. A panel that the narrow page shows as a tab (Music, Settings, and the Settings sections) drops its `tabpanel`
role while it is a window. A landmark of its own (the games' `main`, the About `footer`, the chat's `aside`) keeps its
role and takes the title as its name. A heading inside the window that repeats the title goes quiet: a screen-reader-only
one is hidden, and a visible one (the chat's collapser) loses its heading role. When the desktop goes, every attribute is
put back (`web/skins.js`, `named`). **The desktop's check**, `tools/web/deskcheck.js`, opens every window in turn on a
development host served by `tools/webtest/serve.py` (`index.html?silent=1&driver=tools/web/deskcheck.js`, at least 760
pixels wide). For each window it checks that structure and runs the checker on the live page. The result is in
`window.byopopDeskCheck.result`, and the page's title ends with "deskcheck: PASS" or "FAIL". The chat's window is checked
closed, because opening it starts the chat.

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
context before the game has made its own, since that would take the canvas from the game.

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

### Controls

The Controls panel (`web/controls.js`) lists the game's controls in our own short words, from `web/controldata.js`,
keyed by game id, whose header gives an entry's form; only keys the game's own code handles are listed. The same table
drives key remapping (`byopop-keys`), the gamepad (`gamepad`, read once a frame) and the on-screen keys (`touchKeys`);
`inputDisplay` shows the keys and mouse buttons the game is sent.

### Phones

A phone is a touch screen whose short side is under 600 CSS pixels (`PHONE` in `web/shell.html`; `?phone=1` on a
development host for tests). There (the user, 2026-11-10: "the games should all default to full screen with pixel
perfect mode off") the game is as large as its window allows, and Play asks for full screen unless the "Full screen"
setting is off. The touch keys' keyboard button opens the screen keyboard through a hidden text field that is read-only
the rest of the time, so a tap on the game never brings the keyboard back (the user's note of 2026-11-11). "Touch keys
position" is `touchPlace`.

### Game speed

`speed` (0.25 to 4; the user, 2026-11-10: "Allow game speed to be set to any arbitrary value") sets
`SexyAppBase::mUpdateMultiplier` through the web-only `byopop_set_speed` (`framework/SexyAppBase.cpp`). At 1 nothing is
called, so unmodded play is unchanged. It does not reach games whose own host runs their loop (Bonnie's Bookstore, Ning
Po), nor a timer a game reads from the wall clock. Music and sounds keep their own speed.

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
goes on, as the native loop's `--first` does (the screen saver uses it). The result is
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
The page mutes the game while its tab is hidden (`SexyAppBase::EmscriptenPageHidden`).

For tests, `Module.byopopAudio` has the output's state (`kind`: `worklet` or `script`; `target`, the queue's length;
`gaps`, how often it ran dry; `late`). `Module.audioOutput = 'script'`, set before Play, forces the ScriptProcessor. A
function in `Module.onVoiceMixed` is called with the page's clock at the play call when a sound's first frame is mixed,
and `Module.audioChunkEar` holds the time that frame reaches the speaker. `?silent=1` mixes as usual and sends silence,
so the timing is the same.

## Memory use

A game takes more memory in a browser than natively: IDBFS's copy in memory of every stored file, decoded images held
both in the game's memory and as WebGL textures, and WebAssembly memory that never shrinks after the loading peak. Not
yet done: read files lazily from IndexedDB, free an image's decoded pixels once its texture is on the GPU, and decode
large images one at a time.
