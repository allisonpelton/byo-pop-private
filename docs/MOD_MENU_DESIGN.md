# Mod menu: design

How the mod menu works, section by section; the code cites these section numbers. The menu is
`framework/ModMenu.*`, the settings `platform/ModSettings.*`, the model and its files `platform/ModManager.*`,
and the web page's part `platform/WebApi.cpp` and `web/modpanel.js`. What a player or a mod author needs is in
[MOD_MENU.md](MOD_MENU.md) and is not repeated here: the menu's pages and keys, the files in the user dir, the
keys and types of a setting, placeholders, when a value applies, and what counts for the high scores.

It answers the user's request of 2026-10-19: "Add support for a mod menu to the web ports and Windows. Mods can
offer configurable values to the menu and be disabled from there. The mod menu could also restart the game".
The user's decisions of the same day:

1. **Hotkey.** F2; also the web page's Mods button, and a MODS or Options button where a game has a free slot.
2. **Look.** One shared look in every game, drawn with rectangles and the built-in font. No per-game skins.
3. **Bejeweled.** Gameplay and feature values apply at the next fresh game without a restart (6.2), held back
   while a saved game exists. Art, enabling, disabling and the order need a restart.
4. **The web panel** only adds and removes mods (import, remove, and the rescue when a mod stops the game from
   starting). Turning mods on and off, the order and the settings are in the in-game menu.

Also decided: the built-in font is written in the repo; nothing restarts by itself, and "Restart game" asks only
when a game is in progress; mods are not removed from the in-game menu; a restart drops `-nomods`; the Options
button shows only when a mod has settings; Big Money's high scores are scoped with the score key.

## 1. Overview and principles

### 1.1 Scope

Every game has the menu, natively and on the web: the installed mods in load order, a switch and an order for
each, its status and errors, one page of settings per mod, and a Restart button for the changes that need a new
process. A mod offers typed values (bool, int, float, choice, text, colour) in a `settings` block of its
`mod.json`; they are stored per user in `<user dir>/mod-settings.json` beside `mods.json` and flow into the
mod's own data. Not in scope: installing or removing mods natively (they stay folders), swapping art without a
restart, scripting.

### 1.2 Principles

1. **One model, several views.** `Platform::ModManager` holds the mods, their order, their enabled flags and
   their values, in an applied and a desired state. The in-game menu, the web panel (through the C API) and the
   test switches are views of it; JavaScript has no second implementation of ordering, tagging or validation.
2. **Without opening the menu, nothing changes:** no new file in the user dir, no rewritten `mods.json`, no new
   log line, no widget, image or font created, no `Rand()` draw. Every oracle case stays identical.
3. **Values are data.** A value is substituted into the mod's own JSON before anything reads it, so the engine
   cannot tell a value that sets `rules.match.minRun` from a mod that wrote the number.
4. **Where a value lands decides what it does:** whether it needs a restart and whether it splits the high
   scores are derived from where its placeholders sit. A mod can make a value stricter, never looser.
5. **Gameplay values get their own high scores; cosmetic values never do.** A mod whose values are all at their
   defaults keeps the score key it had without settings.
6. **A game in progress is never lost.** Every restart goes through the game's own shutdown save; a board keeps
   the save name it started with; a value that would strand a saved game waits, or the menu says where the game
   went.
7. **Restarts are explicit**, and the menu always says what is pending.
8. **Writes happen only on a user change, only in the user dir, and by merging:** re-read the file, change one
   thing, keep unknown keys, write atomically, mark the web backup dirty.
9. **To the game, the menu is a modal dialog.** It pauses play exactly as Options does (recorded as
   `PAUSE`/`UNPAUSE` in replays) and draws no random numbers.
10. **One framework-level menu**, built from drawing primitives and a built-in font, so a new game has it before
    it has any art. It works with a pointer alone.

## 2. Architecture

### 2.1 Layers

```
 web/shell.html + web/modpanel.js  (HTML Mods panel: add, remove, rescue; restart, autostart)
     | ccall byopop_mods_state/_edit/_rescan, byopop_menu, byopop_restart   ^ Module.onModsChanged
     v                                                                      | Module.onGameExit({restart})
 framework: SexyAppBase (F2, OpenModMenu, RequestRestart, script "modmenu", exit info)
            ModMenu (pages, controls, the look) + BuiltinFont
 platform:  ModManager  applied state: VFS roots, remaps, images, tunables, strings, score key
                        desired state: scan, mods.json, mod-settings.json, edits, pending, next keys
            ModSettings (schema, validation, values, placeholders, key entries)   SDL-free
            Relaunch (native), WebPersist (backup, sync), UserFiles, Json
 games:     Options button, restart arguments, messages, next-game commit (Bejeweled)
```

### 2.2 Applied and desired state

- **Applied** is what this process runs with. `Refresh()` sets it at start; after that only the next-game commit
  changes it (6.2). VFS roots, remaps, images, strings, fonts and the mod sign come from it.
- **Desired** is what the files say: the mods on disk, `mods.json` and `mod-settings.json`. The menu and the
  panel show and edit it. Where it differs from the applied state the change is **pending**, with the earliest
  level at which it can apply (6.1).
- **Change notification.** `GetChangeSerial()` increases on every edit or rescan; the open menu redraws when it
  moves, and on the web an `EM_JS` call tells the panel.
- **What a game re-reads.** `ModManager::SetNextGameFilter(path -> bool)` is asked for every data path a value
  can change (`$omit` changes none): a game declares which paths it reads again at a new game, so values that
  need art made at load stay restart-level. Without a filter everything is restart-level, and entries found
  under `"next"` count as committed, so they never show as pending for ever. The filter is set before `Refresh`.
- **Other hooks.** `SetGameplayFiles` (default `data/diff.txt`; called after `Refresh` it recomputes the applied
  owners and the score key) names the files that count for the scores, and `SetDataCheck` lets a game check
  resolved data; its messages are warnings in the state JSON.
- **Three score keys** are in the state JSON: `scoreKey` (applied), `restartScoreKey` (what a start now applies:
  the desired mods and the committed values) and `nextScoreKey` (the next fresh game, held values included).
  The restart question and the menu's key note use `restartScoreKey`: a held value never moves a game in
  progress.

### 2.3 Files

Each file's header says what it holds. What is not obvious from the names: `platform/UserFiles.*` has
`WriteUserFileAtomic` and the "user data changed" callback; `platform/Relaunch.*` has `Relaunch(args, gameName)`
(Windows `CreateProcessW`); `platform/WebApi.cpp` is web only and holds the exported functions of 4.5 that need
no running game; `web/modpanel.js` is appended with `--post-js` and leaves `Module.byopop`; `web/shell.html`
restores the backup before it reads anything and handles restart and autostart; `cmake/ByopopWeb.cmake` has
`byopop_web_game`, one function for every game's web link options (shell, `--post-js`, exported functions,
runtime methods); `tools/rundesk/rundesk.c` runs its child in a job object and returns when the whole job has
ended, so a relaunched game stays on the invisible desktop; `tools/modsettingstest/`, `tools/modmenutest/` and
the JSON-only fixture mod `tools/fixtures/mods/settings-demo/` are the tests of 7.3.

## 3. `settings` in mod.json

The author's view of this section is MOD_MENU.md ("Declaring settings", "Placeholders", "When a value
applies", "High scores and saves"). Here is what the code adds to it.

### 3.1 Example

The example is the fixture mod, `tools/fixtures/mods/settings-demo/mod.json`. In Bejeweled its `rules` settings
apply at the next game and score; its `features` setting applies at the next game and does not score; its
`strings` and `files` settings apply at a restart and are cosmetic.

### 3.2 Setting keys

The keys and their limits are listed in MOD_MENU.md. In the code: ints are limited to int32; the 100,000-step
limit applies to floats only; choice values are ids (`[A-Za-z0-9_.-]{1,32}`), int32 whole numbers or bools,
unique by canonical text. While an `enabledIf` condition is false the value resolves to its default, for the
score key too; a condition takes its dependants' level but not their score flag, since the dependant's own pair
already holds its effective value.

### 3.3 Types

Canonical text, used by the score key and the `map` lookup: bool `true`/`false`; int decimal; float the 16 hex
digits of the IEEE bits of the snapped value (no `map`); choice a string as it is, a number in decimal, a bool
as `true`/`false`; text the Latin-1 bytes with `%`, `&`, `=`, `?`, `;` and non-printable bytes escaped (no
`map`); color the lower-case text itself (no `map`). Floats are snapped to `min + k*step` in scaled integers,
so a value equals its JSON literal, and both platforms compute in IEEE double without fast-math: the bits are
identical natively and in wasm, and no `printf` formatting takes part in the key.

### 3.4 Placeholders

Placeholders are described in MOD_MENU.md; `as: "string"` gives the text the menu shows, not the canonical text.

`ModSettings::Resolve` runs in `ModManager::Apply`, and again in the next-game commit (6.2), over each enabled
mod's manifest after its `tunables` and `rules` files are read. The result is the **effective manifest**, and
everything later reads only that: VFS remaps, patches, strings, the flattened tunables, the rules and features
JSON and the game's own rule resolution. A mod without `settings` is never touched: its effective manifest is
its manifest.

On an error (an undeclared setting, a `map` with neither the value nor `"*"`, a wrong `as`, a placeholder in a
forbidden key, nesting deeper than 8) the top-level key that holds the placeholder is ignored for that mod, as a
mod whose rules fail their own checks is treated, and the error shows in the menu and in `mods-status.json`.
The rules for paths and strings are in MOD_MENU.md; `LoadProperties` checks the keys whose game text has
printf conversions and replaces a mod text with other conversions by the game's, with a log line.

### 3.5 When a value applies (the derived level)

| Where the value is used | Level |
|---|---|
| nowhere in the data: game code reads it with `ModManager::GetLiveValue(mod, id)` | `apply` if given, otherwise `live` |
| `rules`, `tunables`, `features` | `nextGame` for the paths the game's next-game filter accepts (2.2), otherwise `restart` |
| `files`, `images`, `imageScale`, `imagePatches`, `imageOps`, `strings` | `restart` |

A value's level is the latest of all its uses and of its own `apply`. Enabling, disabling and reordering mods
always need a restart, because they change the VFS roots, the mod sign drawn into the fonts at load, and the
score key.

### 3.6 Validation

A wrong declaration drops the setting (or, when `settings` is not an array, the whole block), with an error; a
wrong `enabledIf` is ignored and an `apply` looser than the derived level gives way to it, each with a warning;
a `map` on a float, text or colour, or with keys that cannot match, is a placeholder error (3.4).
`tools/modsettingstest` has every case.

Stored values are normalised as they are read, and normalising never writes the file: a wrong type, or a choice
that no longer exists, gives the default, with a warning; a value outside the range is clamped, with a warning;
a float off a step snaps to the nearest step; text too long is cut to `maxLength`; characters outside the
charset become `?`.

### 3.7 Score key and RulesId

- **Score-affecting.** A setting is score-affecting when one of its placeholders lies under `tunables` or
  `rules` or in the `files` member of a gameplay file (`data/diff.txt`), or when it says `"affectsScores": true`.
  A mod with such a setting, or with `tunables` or `rules`, is score-affecting. The "changes gameplay" tag in the
  panel and the menu is `AffectsScores()` of the desired state, computed in C++.
- **Key entries.** The key is FNV-1a over the entries of the score-affecting mods **in load order**
  (`ModManager.cpp`, `HashKey`). They were sorted once; a reorder in the menu could then change the rules under
  an unchanged key, and Bejeweled refused and deleted the v2 save kept under it. With one such mod, or several
  already in id order, the key is the same as the sorted one. A mod's entry is `id@version` while all its
  score-affecting settings are at their defaults (after `enabledIf`), otherwise `id@version?` followed by
  `sid=canonical` pairs joined with `&` and sorted by `sid`, for example `example-relaxed@1.0?hintPenalty=25`.
- **Consequences.** `example-relaxed` keeps `savegame-58ebd9d7.dat` whether or not it gains a settings block,
  as long as its values stay at their defaults. Every gameplay combination gets its own save, high-score table
  and `-seeded` table. Cosmetic values never change the key.
- **RulesId** (Bejeweled) is computed from the merged effective rules, so gameplay values change it and seed and
  leaderboard boards follow; values under `features` stay outside it. A replay carries its rules, so it plays
  back under its recorded values whatever the settings are.
- **The desired-state keys** need the mod that a gameplay file resolves to: `ModCatalog::DiffOwner()` follows
  the rule of `VFS::Resolve` (the highest-priority `files` remap wins, then the highest-priority file), and a
  unit test checks that the two agree.

## 4. Storage and sync

### 4.1 `mod-settings.json`

The format is in MOD_MENU.md: `"mods"` holds the committed values, which a start applies, and `"next"` the
values held for the next fresh game (6.2). Only values that differ from their defaults are stored, so a user who
never touched a value follows a default the author changes later; entries of mods that are not installed, and
unknown top-level keys, are kept. A missing file means defaults: nothing is written and nothing logged. A file
with a UTF-8 byte order mark is read as written.

### 4.2 `mods.json`

Unknown keys and the member order are kept. The order shown is the one ModManager uses: priority first, then
the saved order, then new mods appended. A mod swaps only with a neighbour of the same priority, so ▲ and ▼ are
disabled at a priority boundary. A write keeps the slots of ids in `order` that are not on disk.

### 4.3 How a write happens

Every edit re-reads the file, applies one operation, stops if the bytes are unchanged, and otherwise calls
`Platform::WriteUserFileAtomic(rel, bytes)`: write `<name>.tmp`, replace the file (Windows
`MoveFileExW(MOVEFILE_REPLACE_EXISTING)`, elsewhere `rename`), call `Platform::SyncUserData()` and run the "user
data changed" callback, with which `SexyAppBase` marks the web backup dirty. On the web, with no app running,
the write backs up straight away. Broken files move to `mods.bad.json` and `mod-settings.bad.json` at the first
write. Nothing is written at startup.

### 4.4 Web: before the game starts

The page must not read `mods.json` before the localStorage backup is restored, or the panel would show and save
an older file than the game uses. So after mounting, the page first calls `byopop_userdata_restore(userDir)`
(`RestoreUserDataBackup`, which is idempotent: restored files take the backup's time stamps, so `main`'s own
call changes nothing), and only then `byopop_mods_open(userDir, modsDir)`, which sets the folders and scans the
desired state without running `Apply`. The exported functions work before `callMain`, because the runtime is
already initialised (`-sINVOKE_RUN=0`).

### 4.5 Web: the C API, used by the panel and the tests

`cmake/ByopopWeb.cmake` lists them all in `-sEXPORTED_FUNCTIONS`.

`byopop_userdata_restore(dir)` runs at boot before any read (4.4); `byopop_mods_open(userDir, modsDir)` before
`callMain` sets the folders and scans the desired state; `byopop_mods_state()` returns the state JSON;
`byopop_mods_edit(json)` takes `{op: "enable"|"move"|"set"|"reset"|"forget", mod, on?, delta?, setting?,
value?}` and returns `""` or an error message; `byopop_mods_rescan()` follows a mod added or removed. These five
are in `platform/WebApi.cpp`. `byopop_menu(on)` (`framework/ModMenu.cpp`) takes 1, 0 or -1 (toggle) and returns
1 or 0 (open or not), or -1 when no game runs; `byopop_restart(confirmed)` returns `{"result":
"started"|"confirm"|"blocked"|"none", "message"}`, so the page can ask the game's question.

Events from the game to the page: `Module.onModsChanged()` (posted with `setTimeout`, never called inside an
edit; several changes make one call), `Module.onModMenuFocus(text)` (5.6), `Module.onModPanel` (the menu's
**Add or remove mods** button), `Module.onGameLoaded` (the rescue) and `Module.onGameExit(info)` (6.5).

The state JSON, which `-dumpmods` and `modmenu dump` also write, has the three score keys (2.2), `pending` (the
labels waiting for a restart and for the next game) and, for each mod and each of its settings, the declaration,
`value`, `applied`, `level`, `scores`, `enabled`, `pending`, `errors` and `warnings`; the fields are in
`ModManager.cpp`.

While the game runs, the panel and the in-game menu edit one in-memory model, so there is no two-writer
problem. The panel reads everything through this API; the pages have no copy of the mod scan, the order or the
gameplay tags, and no longer read `mods-status.json`.

**The panel** (decision 4; `web/modpanel.js` has the rest) lists the installed mods with their state, tags and
errors and has Add mod folder, Add mod .zip and Remove; before the game starts also Play without mods; and,
while the game runs and something waits, "Restart to apply: ..." with Restart game. The tool bar's Mods button
opens the in-game menu while the game runs, and the panel before that.

- **Adding.** One upload may hold many mods: every folder in it with a `mod.json` or one of the game's folders
  is a mod, the outer one where they nest (`importMods`); paths that would leave the mod folder are dropped. A
  new version of a mod in use is staged in `<persist>/mods-incoming/<folder>` and replaces the folder at the
  next boot. Adding a mod cancels a queued removal of its folder.
- **Removing.** A mod not in use is deleted at once, and `{"op": "forget"}` (`ModManager::EditForget`) takes its
  id out of `mods.json`, so the same mod added later starts on; its values stay. A mod the running game uses is
  turned off and queued in localStorage (`byopop-pending-remove:<game>`), because the game may still read its
  files: the page deletes and forgets it at the next boot, before `byopop_mods_open`. The panel offers Keep
  until then, and turning the mod on again in the menu counts as Keep.
- **Input.** Keys typed in a panel or on the tool bar stop there; F2 on the tool bar still reaches the game.
  While a panel is open, a press on the game closes the panel and reaches nothing else; its release and click
  are dropped too.
- **The rescue.** The page marks a start with mods in localStorage (`byopop-starting:<game>`) and clears the
  mark when the game has loaded (`Module.onGameLoaded`, an `EM_JS` call in `SexyAppBase::DoUpdateFrames`) or
  quits. A mark found at the next boot means that start never finished: the page does not autostart and offers
  Play without mods. `Module.onAbort` shows "The game stopped" with the same button, which reloads and starts
  once with `-nomods`. Natively the only rescue before start is `-nomods`.

### 4.6 Native: hand edits and two processes

Users can hand-edit both files; the menu re-reads them when it opens and before each write. There is no
single-instance check natively, so two processes can run at once: merging per operation keeps both processes'
changes, and each keeps its own applied state.

## 5. The in-game menu

### 5.1 Opening and closing

- **F2** toggles the menu: a press without modifiers that is not a key repeat, checked in `HandleEvent`'s
  `SDL_KEYDOWN` case after the scripted-input filter and before Alt+Enter and widget dispatch; its key-up is
  swallowed too. It works from the end of `Init` until `Shutdown`.
- **A game button** (8.2, 8.3) opens the menu over Options, and closing it returns to Options. On the web the
  tool bar's Mods button calls `byopop_menu`. Scripts use `modmenu open` (7.2).
- **Closing:** F2, Esc on the list page, or the Close button. A click on the dimmed backdrop does nothing, to
  avoid accidental closes on touch. The menu closes in `KillDialog`'s order and takes the focus back if the
  game moves it.
- Esc, Enter, Tab, Backspace and Space act on the character they type, so a key that closes the menu never
  reaches the board; while the key that closed it is held, its repeats (and the text they type) are dropped in
  `HandleEvent`.

### 5.2 Pages

The layout is in logical 640x480; the panel is `Rect(40, 24, 560, 432)`; the rest is in `framework/ModMenu.cpp`.
The list page's checkbox box and 4 px around it are the only part of a row a click switches; "Settings" (or
"About" for a mod without settings) opens the mod's page, as Enter does. Banner rows at the top say mods are off
for this run or that `mod-settings.json` could not be read. An int has a slider when the range has at most 200
steps, and a colour steps through a 16-colour palette.

- **A click on a label or a row only focuses it** (the user, 2026-10-25: label and row clicks switched too
  easily); only the checkbox switches a bool or a mod.
- **The message slot** under the content of every page is 34 px, two lines, reserved even when empty: on the
  list the game's messages about every mod (`GetModMessages` with no mod), on a mod's page its errors and then
  its warnings, the shared ones and the game's. With several, the first two get a line each and the second ends
  with "+N more". The rows therefore sit at the same place in every game, so the test scripts are shared
  (`tools/modmenutest`).
- **The footer** shows the focused row's help (a setting's help; on the list a mod's description or first
  message) and what is pending. Confirmations (Restart, 6.3; "Reset all settings of ...?") replace the footer
  inline, never as a second dialog. There is no Apply or Cancel: every change is saved at once. "Restart game"
  is always enabled, so the menu can also simply restart the game.

### 5.3 Look

One look for every game (decision 2), in `framework/ModMenu.cpp` (its colours are there), created when the menu
first opens and drawn only with `FillRect`, `DrawRect` and `DrawLine`.

The built-in font has a 12 px cell with an 8 px cap height and proportional advances. ASCII 32..126 is drawn by
hand, and Latin-1 160..255 is composed from a base glyph and a diacritic table. Bold is drawn twice, one pixel
apart. All glyphs are drawn white into one `MemoryImage` and tinted with colorize; at render scale N the font
scales with nearest filtering.

Mod text is UTF-8 and is converted with `UTF8ToLatin1`; characters outside Latin-1 become `?`. Long text is cut
with "..." and shown in full in the footer when its row is focused. The menu's own words come from
`GetString("MODMENU_...", default)`, so a game's properties or a strings mod can translate them.

### 5.4 Modality, pausing, focus and RNG

- **Opening** runs `AddWidget`, `BringToFront`, `mWidgetManager->AddBaseModal(menu)` (which takes the focus from
  the board), `SetFocus(menu)` and `ModalOpen()`. **Closing** runs `RemoveBaseModal` (which restores the
  previous focus), `RemoveWidget` and `ModalClose()`.
- **Pausing** is the game's own modal hooks: Bejeweled pauses through `Pause(mTimedMode)`, recorded as
  PAUSE/UNPAUSE; Big Money pauses only its Action type. Both count pauses, so the menu over Options nests.
  During a replay watch the menu behaves as Options does.
- **Staying on top.** If something adds a widget above the open menu, the menu raises itself again in its
  Update. It is not in the dialog map, so code that counts dialogs is unaffected.
- **No randomness.** The menu never calls `Rand()`. Its event handlers, Update and Draw run inside
  `MTAutoDisallowRand`, so a debug build asserts on any draw, and a static test checks the sources (7.3). The
  menu's Restart takes effect at its next Update, outside that scope.

### 5.5 Input

The keys are in MOD_MENU.md. A press that moves more than 6 px vertically scrolls a list; SDL turns touches into
mouse events, and hit targets are at least 24 px. Text fields take `KeyChar` and `SDL_TEXTINPUT`, limited to
their `charset` and `maxLength`.

### 5.6 Accessibility

- Everything works from the keyboard alone and from a pointer alone; the 2 px focus ring is always visible; Esc
  always backs out.
- The look has 15:1 contrast (white on `#1e2230`), and state is never shown by colour alone: checkboxes have a
  mark and "On"/"Off", and badges are words.
- Nothing moves by itself: no timing, no animation, no blinking caret.
- On the web the menu reports its focused item through `Module.onModMenuFocus(text)` into a polite live region
  that the panel adds to the page, and the panel is built of real form controls with labels.

## 6. Apply and restart

### 6.1 What applies when

| Change | Bejeweled | A game without a next-game filter | Why |
|---|---|---|---|
| enable, disable, reorder; a mod added or removed on disk | restart | restart | VFS roots, the mod sign drawn into the fonts, the score key |
| a value used in `files`, `images`, `imageScale`, `imagePatches`, `imageOps`, `strings` | restart | restart | loaded once |
| a value in the `files` remap of `data/diff.txt` | restart | restart | the values are copied into the app once |
| a value used in `rules`, `tunables`, `features` | next fresh game (6.2) | restart | Bejeweled calls `GameConfig::Load` at every NewGame and the board snapshots its rules |
| a value with no data use (`GetLiveValue`) | live | live | the code asks for the value when it needs it |

Swapping art and strings without a restart is not built: derived images (shrunken gems, timer bars, the mod sign
in the font) and cached strings stand in the way.

### 6.2 Bejeweled: next-game values

1. **Holding.** A pending next-game value is held in memory and in `mod-settings.json` under `"next"`, so a quit
   before the next game keeps both the saved game and the pending value.
2. **The commit.** `GameConfig::Load(canCommit)` starts with `ModManager::CommitNextGameValues(canCommit)`. The
   game passes true only when no game would be stranded: the outgoing board is not in progress, and no save
   exists under the current key (not while a replay is watched). A commit moves `"next"` into `"mods"`,
   re-resolves `rules`, `tunables` and `features`, recomputes the score key and writes the file once. With
   nothing pending it returns at once. At startup the save exists until "Continue?" is answered, so nothing
   commits. An untouched board is made again with the committed values.
3. **Each `Board` snapshots its save and sidecar names** in its constructor (`ScopedSaveGameFileName`), and
   `SaveGame`, `~Board`, `WinDMApp::Shutdown` and `GetShutdownSaves` use the snapshot: `NewGame` deletes the old
   board deferred and calls `Load` right after, so a static name would put the old game's save under the new
   key. The other users of `GetScoreKey()` and of scoped file names (`Board.cpp`, `HighScoreWidget.cpp`, which
   keeps its own file name, `GameConfigMods.cpp` and `LiveSplitObserver.cpp`) were audited for this.
4. **The mod sign.** `IsLiveSplitOnly` reads the effective manifest and ignores `settings`, so a livesplit mod
   that gains options stays sign-free.

The next-game filter is `GameConfig::IsNextGamePath`: every path under `rules`, `tunables` and `features` except
`features.livesplit`, because the LiveSplit observer's state outlives a board.

### 6.3 The restart (shared)

`SexyAppBase::RequestRestart()`:

1. **`CanRestartNow(&reason)`**, a virtual that defaults to true. Bejeweled and Big Money return false while
   their record-score dialog is open, because a restart there would drop the score, as quitting there does. The
   reason shows in the footer.
2. **A confirmation**, only when the game's `HasGameInProgress()` is true: that the game is saved and can be
   continued after the restart, or, when `restartScoreKey` differs from `scoreKey`, that it stays saved with the
   current mods and continues when the player switches back to them. A game that does not override
   `HasGameInProgress` restarts at once.
3. **The save.** It sets the restart flag and calls `Shutdown()`, so the game's own save path runs exactly as
   on Quit, and the registry is written.
4. **Test runs.** In a test run (`BYOPOP_TEST_RUN`, or `-script` or autoplay active) it logs `restart: <args>`
   and does not relaunch, unless `BYOPOP_RELAUNCH_TEST_ARGS` is set (7.2).

The virtuals, each with a default: `CanRestartNow`, `HasGameInProgress`, `GetRestartArgs` and
`GetModMessages(std::vector<ModMessage>*)` (a game's messages per mod, such as Bejeweled's rules status). A game
that overrides them says `override`.

### 6.4 Native relaunch

A game's `main` ends with the lines in `platform/Relaunch.h` (`Shutdown`, `TakeRelaunch(&args)`, `delete`, then
`Platform::Relaunch(args, name)`). Bejeweled, Bejeweled 2, Bejeweled Twist and Big Money have them. In a game
without them, Restart game shuts the game down, saved as on Quit, and does not start it again.

`Relaunch` takes the exe from `GetModuleFileNameW`, converts the UTF-8 arguments to UTF-16 with MSVCRT quoting
and calls `CreateProcessW` in the current directory with no inherited handles. On failure it says "Could not
restart <game>. Your game was saved; start it again." (in a test run only in the log). A relaunch from a folder
with non-ASCII characters is not done: the file layer's ANSI calls fail there first.

| Argument | Relaunched | Why |
|---|---|---|
| `-gamedir`, `-userdir`, `-modsdir`, `-changedir` | yes | the same install and saves |
| `-seed=N`, `-renderscale=N` | yes | the user's choices for the session |
| `-replaydir` (Bejeweled; Big Money only when it was given) | yes | |
| `-gameseed=N` | only while the seed still waits for its game | a used seed would be played again |
| `-nomods` | no | the restart is how the menu applies mods |
| `-script`, `-fast`, `-trace`, `-autoplay*`, `-replay` and every other test switch | no | test hooks |

The framework keeps its own keep list, and a game adds to it in `GetRestartArgs`. The new process restores
`PreferredX/Y` and `ScreenMode` from the registry, so it opens where the old window was. A process exit resets a
running LiveSplit run (the ASL's `exit`). rundesk waits for the whole job, so a relaunched game stays under it.

### 6.5 Web restart

1. Steps 1 to 3 of 6.3 run. The values were written and synced when they were edited.
2. The main loop ends: `MainLoopFinished` deletes the board, which saves the game; the backup is forced; and
   `sexyapp_notify_exit` passes `{restart: true, dropped: ["-gameseed"]}`.
3. The page's `onGameExit(info)` suspends audio, shows "Restarting…", waits for the storage sync to finish, sets
   `sessionStorage['byopop-autostart']`, removes `seed` from the URL when `-gameseed` was dropped (and `replay`
   when `-replay` was; `Session.restartPage`), and reloads. `?silent=1` stays in the URL.
4. After the reload the backup is restored first (4.4) and, with the autostart flag set, the game starts at once
   without the Play overlay. Audio stays silent until the first click or key, and the status line says so.
5. The page's Web Lock is freed on unload, so a second tab that is already waiting takes it first and this page
   then shows "Already open": accepted, as for a manual reload.

### 6.6 Games in progress

A restart with the score key unchanged saves the game at shutdown and offers "Continue your game?" afterwards. A
restart that changes the key (a gameplay mod toggled, or a restart-level gameplay value) saves it under the old
key; the confirmation and the note at the change say so, and it comes back when those mods or values are
switched back. A next-game gameplay value while a game is in progress or saved (Bejeweled) is held back (6.2),
so it never strands the game. A quit or closed tab with restart-level changes pending leaves them in the files
for the next start; a page killed without unload events leaves the save and both settings files in the
localStorage backup; a failed relaunch leaves the game saved, with a message to start it again.

## 7. Fidelity and tests

### 7.1 Why nothing changes without the menu

The hotkey is on the real-input path only; scripted runs drop real input first, and no oracle or autoplay
script sends F2. Nothing is created before the menu first opens: no menu, no font. Loading is unchanged for mods
without `settings`: the effective manifest is the manifest, so the tunables, rules and features JSON, the
RulesId and the score key are identical, and a missing `mod-settings.json` gives no read error, no log line and
no write. A game's Options button appears only when a mod has settings (8.2, 8.3), and none of the oracle's mods
has. The next-game commit returns at once when nothing is pending, and each board's snapshot names equal the
static names. The proofs are the games' own comparisons (Bejeweled's oracle in all three variants, Big Money's
script and autoplay comparisons) and `fwtest`.

### 7.2 Script commands and switches

The `modmenu` commands are framework commands in `RunScript`, so every game has them: `open`, `close`,
`page <modId>|list` (which opens the menu too), `enable`, `move`, `set`, `reset`, `restart [confirm]` and
`dump <file>` (the state JSON); their syntax is in `framework/SexyAppBase.h`. They use the UI's own functions:
edits write the user dir, and page changes redraw. Keys inside the menu use `key UP|DOWN|LEFT|RIGHT|RETURN|ESCAPE`,
`keydown`/`keyup` and `char x`; clicks use `click x y` at the layout coordinates of 5.2; `#` inside `"..."` is
not a comment. **`key F2` deliberately does not toggle the menu**: it goes to `WidgetManager::KeyDown`, not to
`HandleEvent`, and the hotkey is tested in `fwtest`. `-dumpmods` prints the state JSON and exits before the
window opens. With `BYOPOP_RELAUNCH_TEST_ARGS` set, a test run really relaunches, with the kept arguments plus
these (for example a second `-script` that ends in `quit`), under the same rundesk.

### 7.3 Tests

The tests' own headers say what each checks: `tools/modsettingstest` (native, and wasm under Node: validation,
placeholders, levels, score-key vectors such as `example-relaxed@1.0` giving 58ebd9d7 with and without a
defaults-only settings block, float bits equal on both, `DiffOwner` agreeing with `VFS::Resolve`),
`tools/modmenutest/run_tests.sh` (scripted games with the fixture mod: screens, the exact bytes of the two
files after edits, keyboard-only use, the trace unchanged by opening the menu mid-game, Bejeweled's next-game
values, restarts including a real relaunch under rundesk), `tools/modmenutest/check_no_rand.py` (5.4) and
`fwtest` (an F2 key-down toggles the menu; with modifiers, as a repeat or in script mode it does not). The web
pages in the Browser pane with `?silent=1` check that the panel's state equals native `-dumpmods`, F2, restart,
autostart and Continue, and that a changed value survives a tab closed without unload events.

## 8. Per-game integration

### 8.1 Shared (coordinator)

The model, the menu and its look, the restart plumbing and relaunch, the script commands and `-dumpmods`, the C
API, `modpanel.js`, `cmake/ByopopWeb.cmake`, rundesk, the unit tests and the fixture mod. Every game gets the
menu from the framework: F2 opens it on any screen, and on the web its page is the shared `web/shell.html` with
`modpanel.js`, linked through `byopop_web_game`. What a game adds is below.

### 8.2 Bejeweled (Bejeweled session)

- **Options** has a "MODS" `DialogButton` in the register slot (`mX+30+bw`, `mY+166`) while an **enabled** mod
  has settings; otherwise the row is the original. It gates on an enabled mod, not on an installed one as Big
  Money does: the oracle's examples case has `example-settings` installed but off, and its Options screens must
  not change. While a feature mod adds to Extras (`WinDMApp::HasExtras`; not a mod that only turns on
  `features.livesplit`), the slot holds the Extras button, and the Extras dialog has the "Mods..." button.
- **Next-game values and per-board names:** 6.2. Main sets the next-game filter before `Refresh`.
- **`GetModMessages`:** the rules errors, dropped mods and warnings of `RulesStatus` per mod, as of the last new
  game; a mod's page shows them.
- **`mods-status.json`,** when it is written, has `values` in its entries for enabled mods.
- **`GetRestartArgs`:** `-replaydir`, and `-gameseed` only while it is pending. **`CanRestartNow`:** false while
  `DIALOG_RECORD_SCORE` is open. **`HasGameInProgress`:** `mBoard && mBoard->IsGameInProgress()`.
- **Main:** the relaunch lines and `-dumpmods`.
- **`bejeweled_verify`** resolves `$setting` placeholders at their defaults (`ModSettings` `Analyze` and
  `Resolve` in `Verifier.cpp`) and links `platform/ModSettings.cpp` and `platform/Text.cpp`; a setting error is
  an error there.

### 8.3 Big Money (Big Money session)

- **Options:** while any installed mod, on or off, has settings, the Register slot (created but never laid out
  in this build) holds a MODS button (`OPTIONS_MODS`). It opens the menu over Options, which stays open.
- **Pausing:** the menu inherits `ModalOpen`, which pauses only the Action type; the other types have no timer.
- **Scores:** the save and the high-score files are scoped by the score key through Big Money's own
  `ScopedUserFileName` (`ModRules.h`), so a gameplay setting gets its own tables. Nothing changes while the key
  is empty.
- **Values:** all restart-level; Big Money sets no next-game filter. `ModRules` reads each enabled mod's
  effective manifest, so a value that leaves a key out (`$omit`) counts.
- **`HasGameInProgress`:** `CanSaveGame()`, a game the shutdown would save (never a replay). **`CanRestartNow`:**
  false while the record-score dialog is open. **`GetRestartArgs`:** the framework's, plus `-replaydir` only
  when it was given and `-gameseed` while that seed still waits for its game; not `-replay`.
- **Main:** the relaunch lines and `-dumpmods`. Its page keeps `/bmpersist` and its lock name
  `big-money-deluxe-instance`.

### 8.4 BookWorm (BookWorm session, M7)

`games/bookworm/src/main.cpp` has no relaunch lines and no `-dumpmods`, so Restart game shuts BookWorm down,
saved as on Quit, and does not start it again, and Options has no MODS button. What BookWorm has: the shared
part (8.1), the menu on F2 and the shared web page; its high scores and saved games kept apart by the score key
(`ScoreKeySuffix`, `games/bookworm/src/ModFeatures.h`); and feature mods whose settings it reads live
(`GetLiveValue`). A mod that changes its word list or other play data must count for the key
(`"affectsScores": true`).
