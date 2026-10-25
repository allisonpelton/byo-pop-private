# Mod menu: design

Status (2026-10-19): the shared stages M0-M4 and M8 are built (branch `shared/modmenu`), and so is Big Money's M6
(d4fe95b, 8.3). M5 (Bejeweled) and M7 (BookWorm) are the game sessions' (9). The guide for mod authors is
[MOD_MENU.md](MOD_MENU.md).

This document has four parts:
- **The user's decisions** (just below), taken after the design and its review. They win over the design.
- **As built**: where the shared stages differ from the design.
- **The design** (sections 1-11), as written before anything was built. Section numbers are cited in the code.
- **Appendix A**, an adversarial review of the design against the code. The shared stages apply its corrections.
  A9 (the startup board), A10 (Big Money) and A11.5 (a check of the saved game's rules) are for the game stages.
  Two parts of A15 are not done: a native rescue before start, and a relaunch from a non-ASCII folder.

Line references are to the commits named in the design's own introduction and are not kept up to date.

## The user's decisions (2026-10-19)

1. **Hotkey.** F2, as designed. Also the web page's Mods button, and a MODS or Options button where a game has
   a free slot.
2. **Look.** One shared look in every game: the design's default look, drawn with rectangles and the built-in
   font. There are no per-game skins, so 5.3's `ModMenuSkin` and the skin rows of 8.2-8.4 are dropped.
3. **Bejeweled.** Gameplay and feature values apply at the next fresh game without a restart (6.2). They are
   held back while a saved game exists. Art, enabling, disabling and the order still need a restart.
4. **The web panel** only adds and removes mods: import, remove, and the rescue when a mod stops the game from
   starting. Turning mods on and off, the order and the settings are in the in-game menu. This replaces the
   design's rebuilt full panel (4.5, 5.1, open question 4).

Taken from the design's recommendations:
- the built-in font is written in the repo;
- nothing restarts by itself, and "Restart game" asks only when a game is in progress;
- mods are not removed from the in-game menu;
- a restart drops `-nomods`;
- the Options button shows only when an installed mod has settings (Bejeweled: an enabled one, 8.2);
- Big Money's high scores are scoped with the score key.

## As built (shared stages)

### M0, M1: groundwork and the settings model
- **Next-game filter.** `SetNextGameKeys(bool)` became `ModManager::SetNextGameFilter(path -> bool)`. It is asked
  for every data path a value can change; `$omit` changes none. A game declares which paths it re-reads at a
  new game, so keys that need art made at load (review A5) stay restart-level.
- **Other game hooks.** `SetGameplayFiles` (default `data/diff.txt`, review A11.4) and `SetDataCheck`, whose
  messages appear as warnings in the state JSON.
- **`$omit`.** `{"$omit": true}` removes a member or element. `null` keeps its meaning in the data (review A6).
- **`as: "string"`** gives the text the menu shows (a float with its decimals), not the canonical key text.
- **Ranges.** Ints are limited to int32. The 100,000-step limit applies to floats only. Floats are snapped in
  scaled integers, so a value equals its JSON literal (review A12).
- **Choice values** must be ids (`[A-Za-z0-9_.-]{1,32}`), int32 whole numbers or bools, unique by canonical text
  (review A11.2). The canonical text of a text value also escapes `;` and non-printable bytes.
- **Paths and strings** (review A7, A13):
  - text and colour values cannot be placed directly in `files`, `images`, `imagePatches`, `imageOps` or a
    `strings` list of files; they need a mapped choice, bool or int;
  - a map entry in those places may not name a file outside the mod folder (`../`): an error. A mod's own
    literal paths are not checked, as before (nothing changes for a mod without settings);
  - a text setting used under `strings` cannot contain `%`;
  - `LoadProperties` checks the keys whose game text has printf conversions (Bejeweled's `LEVEL`, Big Money's
    two rank texts): a mod text for one with other conversions is replaced by the game's text, with a log line.
    Texts the game only draws, and keys a mod adds, are taken as they are;
  - under `strings`, a number or bool from `as` or a map entry becomes its JSON text, and a map entry that is an
    object or a list is an error (review A13).
- **Held values without a filter.** Entries of `"next"` whose setting is not next-game-level in this game (no
  filter, a level that changed, a hand edit) count as committed: the start applies them, so they never show as
  pending for ever. `SetNextGameFilter` belongs before `Refresh`; `SetGameplayFiles` after `Refresh` recomputes
  the applied owners and the score key.
- **Score keys.** The state JSON has `scoreKey` (applied), `restartScoreKey` (what a start now applies: the
  desired mods and the committed values) and `nextScoreKey` (the next fresh game, held values included). The
  restart question and the menu's key note use `restartScoreKey`: held values never move a game in progress.
- **`enabledIf`.** A condition takes its dependants' level but not their score flag: the dependant's own pair
  already holds its effective value, so a condition that changes no data changes no key.
- **User files** with a UTF-8 byte order mark are read as written (not as broken and moved aside).
- **Writes.** `SetEnabled`, `Move` and `SaveConfig` are gone. `EditEnable`, `EditMove`, `EditSetting` and
  `EditReset` re-read, change one thing and keep unknown keys. Broken files move to `mods.bad.json` and
  `mod-settings.bad.json`. Writes go through `Platform::WriteUserFileAtomic`. On the web, with no app running,
  it backs up straight away (review A14).
- **The state JSON** adds `dir`, `hasSettings`, `settingsFile`, and per setting `enabled`, `pending` and
  `noPercent`.

### M2, M3: restart and the menu
- **No skins** (decision 2): the look lives in `framework/ModMenu.cpp`.
- **Keys and focus.** Esc, Enter, Tab, Backspace and Space act on the character they type, so a key that closes
  the menu never reaches the board (review A1). While the key that closed it is held, its repeats (and the text
  they type) are dropped in `HandleEvent`; its release goes on. The menu closes in `KillDialog`'s order and takes
  the focus back if the game moves it (review A16).
- **Focus model.** Tab steps through every focusable control. On a mod page each row has one keyboard control;
  its −/+ buttons, slider, arrows and text field are mouse targets. A click on a setting's label focuses the
  row, and switches a bool.
- **Restart timing.** The menu's Restart takes effect at its next Update, outside its no-random scope.
- **rundesk** runs its child in a job object and returns when the whole job has ended. It kills nothing
  (review A15).
- **Script commands.** `modmenu restart confirm` was added. `modmenu page` opens the menu. There are also
  `keydown`/`keyup`, and `#` inside `"..."` is no longer a comment (review A17).
- **The restart question** needs the game's `HasGameInProgress`. Big Money overrides it (M6, 8.3); Bejeweled
  (M5) and BookWorm (M7) do not yet, so there Restart restarts at once. The game in progress is still saved as on
  Quit.
- **Not done.** A relaunch from a folder with non-ASCII characters: the file layer's ANSI calls fail there first
  (review A15).

### M4: the web pages
- **The C API** (4.5) is `byopop_userdata_restore`, `byopop_mods_open`, `byopop_mods_state`, `byopop_mods_edit`
  and `byopop_mods_rescan` in `platform/WebApi.cpp`. `byopop_menu` and `byopop_restart` are in
  `framework/ModMenu.cpp`, since they need the running game. All are listed in `-sEXPORTED_FUNCTIONS` by
  `cmake/ByopopWeb.cmake` (review A3). Changes to the design's API:
  - `byopop_menu(on)` takes 1, 0 or −1 (toggle). It returns 1 or 0 (open or not), or −1 when no game runs.
  - `byopop_restart(confirmed)` returns `{"result": "started"|"confirm"|"blocked"|"none", "message"}`, so the
    page can ask the game's question (review A2).
  - `Module.onModsChanged` is posted with `setTimeout`, never called inside an edit, and several changes make
    one call (review A14).
- **`web/modpanel.js`** is appended to every game's JS with `--post-js` and leaves `Module.byopop`. It contains:
  - the Mods panel;
  - `Session`, for restarts, autostart and the rescue;
  - the file and zip helpers the pages use for game files too.

  The panel reads everything through the C API. The pages lost their copies of the mod scan, the order and the
  gameplay tags.
- **The panel** (decision 4):
  - It lists the installed mods with their state, tags and errors, and has **Add mod folder**,
    **Add mod .zip** and **Remove**.
  - Before the game starts it also has **Play without mods**.
  - While the game runs and something waits, it shows "Restart to apply: ..." with **Restart game**.
  - Upload paths that would leave the mod folder are dropped, as in the page's game-file import.
- **Removing a mod.**
  - **Not in use:** the folder is deleted at once and the mod is forgotten. The new edit
    `{"op": "forget"}` (`ModManager::EditForget`) takes its id out of `order` and `disabled` in `mods.json`, so
    the same mod added later starts on. Its values in `mod-settings.json` stay.
  - **In use by the running game:** it is turned off and queued in localStorage
    (`byopop-pending-remove:<game>`). The page deletes and forgets it at the next boot. The panel offers
    **Keep** until then, and turning the mod on again in the in-game menu counts as Keep (the page drops a
    queued mod that the model shows on, at every change and again at boot).
- **Adding over a mod in use** (review A14) stages the new version in `<persist>/mods-incoming/<folder>`. It
  replaces the folder at the next boot. Adding a mod also cancels a queued removal of its folder.
- **Opening the menu and the panel.** The tool bar's **Mods** button opens the in-game menu while the game runs,
  and the panel before that. This replaces the design's "panel and menu together" (5.1). The menu's list page
  has **Add or remove mods** on the web, which opens the panel over it (`Module.onModPanel`).
- **Input around the panel** (review A2):
  - Keys typed in a panel or on the tool bar stop there. F2 on the tool bar still reaches the game, and moves the
    focus to the game so the menu it opens gets the keys that follow. Fullscreen gives the focus back to the game.
  - Esc and F2 close a panel. Opening a panel leaves fullscreen (the panels are outside the stage).
  - While a panel is open, a press on the game closes the panel and reaches nothing else; its release and click
    are dropped too, and the next press on the game ends that in any case.
- **The rescue** when a mod stops the game from starting:
  - The page marks a start with mods in localStorage (`byopop-starting:<game>`; a start without any writes
    nothing). The mark is cleared when the game has loaded (`Module.onGameLoaded`, a new `EM_JS` call in
    `SexyAppBase::DoUpdateFrames`) or quits. A mark found at the next boot means that start never finished: the
    page does not autostart, and offers **Play without mods**.
  - `Module.onAbort` shows "The game stopped" with **Play without mods**, which reloads and starts once with
    `-nomods`.
  - With mods off, the menu says so, and Restart game starts with them again.
- **Screen readers.** `Module.onModMenuFocus` fills a polite live region that the panel adds to the page.
- **Big Money's page** has the same reduction. It differs only in its storage folder, lock, game files and
  colours.
- **Not done.** Native has no rescue before start except `-nomods` (review A15).

---

This design covers a shared in-game mod menu for every game, on Windows and on the web. Mods can offer settings, the player can turn mods off, and the menu can restart the game. It answers the user's request of 2026-10-19: "Add support for a mod menu to the web ports and Windows. Mods can offer configurable values to the menu and be disabled from there. The mod menu could also restart the game".

The facts below were checked against these commits:
- **`code-mods` at 0376759:** `platform/ModManager.*`, `platform/WebPersist.*`, `framework/SexyAppBase.*`, `Dialog.cpp`, `MTRand.h`, `WidgetManager.cpp`, `web/shell.html`, `games/bejeweled/src/*` and `games/bejeweled/docs/CODE_MODS_DESIGN.md`.
- **Big Money, `claude/bigmoney-1912c8` at 7e57e76**, read with `git show`. This tip already contains `ModIndicators.*`.
- **BookWorm, `claude/bookworm-eb36ec` at f1c2524.** No game code is committed there yet.

Line references are to those commits. The proposed home of this document is `docs/MOD_MENU_DESIGN.md`, which is shared documentation owned by the coordinator.

---

## 1. Overview and principles

### 1.1 Scope

- **A mod menu in every game**, native and web. It shows:
  - the installed mods in load order;
  - an on/off switch and an order for each mod;
  - each mod's status and errors;
  - one page of settings per mod.

  A **Restart** button applies the changes that need a new process.
- **A `settings` block in `mod.json`.** These are typed values: bool, int, float, choice, text and colour. The menu shows them, and they flow into the mod's own data (`rules`, `tunables`, `features`, `files`, ...).
- **Per-user storage of the values** in `<user dir>/mod-settings.json`, next to `mods.json`. The web shell's Mods panel and the in-game menu stay in sync.
- **Restart:**
  - Native: after the normal shutdown, the exe is started again.
  - Web: after the normal shutdown and the storage sync, the page reloads.
- **Games:**
  - Bejeweled: complete, with code mods.
  - Big Money: complete, data-only mods.
  - BookWorm: in progress.

  The shared parts live in `framework/`, `platform/` and `web/`. Each game adds a skin and a few hooks.

Not in scope:
- Installing or removing mods natively. Mods stay folders the user manages.
- Swapping art without a restart (listed as later work in 6.1).
- Scripting.

### 1.2 Principles

1. **One model, several views.**
   - `Platform::ModManager` holds the mods, their order, their enabled flags and their setting values, in two states: applied and desired.
   - The in-game menu, the web panel (through an exported C API) and the test switches are all views of that model.
   - JavaScript gets no second implementation of ordering, tagging or validation.
2. **Without opening the menu, nothing changes.** There are:
   - no new files in the user dir;
   - no rewritten `mods.json` or `registry.ini`;
   - no new log lines;
   - no widgets, images or fonts created;
   - no `Rand()` draws.

   Every oracle case stays identical. That includes the examples case's prepared `mods.json` (`compare.sh:245-260, 334-336`) and the livesplit runs (`compare.sh:948-974`).
3. **Values are data.**
   - A setting's value is substituted into the mod's own JSON before anything reads it.
   - The engine learns no new mechanism. A value that sets `rules.match.minRun` cannot be told apart from a mod that wrote the same number.
4. **Where a value lands decides what it does.**
   - Two things are derived from where a value's placeholders sit: whether it needs a restart, and whether it splits high scores.
   - The mod author's word is not taken on trust. A mod can only make a value stricter, never looser.
5. **Gameplay values get their own high scores; cosmetic values never do.** A mod whose values are all at their defaults keeps today's score key.
6. **A game in progress is never lost.**
   - Every restart goes through the game's own shutdown save.
   - A board keeps the save name it started with.
   - A value that would strand a saved game either waits, or the menu says plainly where the game went and how to get it back.
7. **Restarts are explicit.** Nothing restarts by itself. The menu always says what is pending.
8. **Writes happen only on a user change, only in the user dir, and by merging.** Every write:
   - re-reads the file;
   - changes one thing;
   - keeps unknown keys;
   - writes atomically;
   - marks the web backup dirty.
9. **To the game, the menu is a modal dialog.**
   - It pauses play exactly as Options does. The pause and unpause are recorded as `PAUSE`/`UNPAUSE` in replays.
   - It draws no random numbers.
10. **One framework-level menu, with optional skins.**
    - There is one menu for every game. Its default look is built from drawing primitives and a built-in font, so a new game has it before it has any art.
    - A game may skin it with its own dialog art and fonts.
    - The menu must work with a pointer alone: touch on the web, and the 3DS idea in `docs/MOD_ROADMAP.md`. That roadmap already plans for "the mod menu".

### 1.3 Decisions

| Topic | Decision | Why |
|---|---|---|
| Binding a value to data | `{"$setting": "<id>"}` placeholders anywhere in the data keys, with optional `map` and `as`. There are no JSON-pointer `targets`. | A placeholder is visible where it is used and works inside a `rules.json` file. It can add or remove a key (through `null`) and can map a choice to a whole sub-tree. Pointers into a separate file break silently when that file changes (array indices), and they need a value already present at the target. |
| Where values are stored | `<user dir>/mod-settings.json`, a file separate from `mods.json` | The shell's writer drops unknown keys of `mods.json` (`web/shell.html:547-552`). `ModManager::SaveConfig` writes only `order` and `disabled` (`ModManager.cpp:138-158`). An old page or an old build would wipe values kept in `mods.json`. |
| Menu class | A `Widget` that adds itself as a base modal and calls `ModalOpen()`/`ModalClose()` itself. It is not a `Dialog` and is not in the dialog map. | Building a `Dialog` with a NULL button image permanently recolours the button text of every later dialog (`Dialog.cpp:84-93`). Dialog-map entries also reach game code: `WinDMApp::KillDialog` gives the board focus when the map empties (`WinDMApp.cpp:569-580`), and YES/NO presses arrive in the game's `ButtonDepress` as 2000+id and 3000+id (`SexyAppBase.cpp:744-758`). |
| Hotkey | F2, checked in `SexyAppBase::HandleEvent` after the scripted-input filter | Scripted runs never see real input (`SexyAppBase.cpp:1724-1746`). F-keys type no character, so no game reacts to them today; Bejeweled pauses on any typed character (`BoardRules.cpp:588-594`). F2 has no default action in the major browsers. |
| Native restart | Start the exe again after `delete anApp` in `main` | A restart inside the process is blocked by process-wide state: the static app in `main` (`main.cpp:250`); the singletons `ModManager`, `VFS`, `Mixer` and `gGameConfig`; the image-scale and patch globals; function-static string caches (`TitleScreen.cpp:89, 108, 155`); `gDialogColors`; and the LiveSplit statics. |
| Web restart | Normal shutdown, then wait for the IndexedDB sync, then reload | `callMain` runs only once per page, and the main loop never returns (`SexyAppBase.cpp:2380`). Statics and the GL context persist. |
| Web panel | Kept, but rebuilt on an exported C API over the same `ModManager`. One shared `web/modpanel.js` serves every game. | This removes the JavaScript copies of ordering and gameplay tagging. The shell's ordering ignores `priority` (`shell.html:440-461`), while ModManager sorts by priority first (`ModManager.cpp:101-105`); its gameplay tagging is a second copy too (`shell.html:413-439`). The panel and the menu can no longer disagree, and Big Money's copy of the shell shrinks. |
| Score key | The existing entry `id@version`, plus `?sid=value&...` for each score-affecting setting whose value differs from its default | `example-relaxed` keeps `savegame-58ebd9d7.dat`, and each gameplay combination gets its own tables. |
| When values apply | Derived per setting (3.5): at the next game (Bejeweled `rules`, `tunables` and `features`), at restart (assets, the enabled set, the order, and every value in games without next-game support), or live (only values that game code reads itself) | This follows what each game actually re-reads (6.1). |
| Font | A built-in Latin-1 bitmap font written in the repo (`framework/BuiltinFont.*`) | The framework has no font of its own: SysFont is rejected (`ResourceManager.cpp:867-870`). Writing our own font avoids a download and any licence question. |

---

## 2. Architecture

### 2.1 Layers

```
 web/shell.html + web/modpanel.js  (HTML Mods panel, add/remove, restart, autostart)
     | ccall byopop_mods_state/_edit/_rescan, byopop_menu, byopop_restart   ^ Module.onModsChanged
     v                                                                      | Module.onGameExit({restart})
 framework: SexyAppBase (F2, OpenModMenu, RequestRestart, script "modmenu", exit info)
            ModMenu (pages, controls) --> ModMenuSkin (default: primitives + BuiltinFont; per game)
 platform:  ModManager  applied state: VFS roots, remaps, images, tunables, strings, score key
                        desired state: scan, mods.json, mod-settings.json, edits, pending, next key
            ModSettings (schema, validation, values, placeholders, key entries)   SDL-free
            Relaunch (native), WebPersist (backup, sync), Json
 games:     skin, Options button, restart arguments, messages, next-game commit (Bejeweled)
```

### 2.2 Applied and desired state

- **Applied** is what this process runs with.
  - `Refresh()` sets it at start (`main.cpp:225`).
  - After that, only the next-game commit changes it (6.2).
  - VFS roots, remaps, images, strings, fonts and the mod sign all come from the applied state.
- **Desired** is what the files say: the mods on disk, `mods.json` and `mod-settings.json`.
  - The menu and the panel show and edit the desired state.
  - Where desired differs from applied, the change is **pending**, with the earliest level at which it can apply (6.1).
- **Change notification.**
  - `GetChangeSerial()` increases on every edit or rescan.
  - The open menu redraws when it changes.
  - On the web, an `EM_JS` call tells the panel.
- **Games declare what they re-read** with `ModManager::SetNextGameKeys(bool)`.
  - Bejeweled sets it to true, because it re-merges rules, tunables and features at every NewGame.
  - The default is false. Everything else is then restart-level.

### 2.3 Files

| File | Kind | Owner | Contents |
|---|---|---|---|
| `platform/ModSettings.h/.cpp` | new, SDL-free | coordinator | Schema parsing and validation, value normalisation, placeholder resolution, level and score derivation, key entries |
| `platform/ModManager.*` | changed | coordinator | Covered in the five rows below |
| ↳ applying | | | `Apply` reads effective manifests. Adds `CommitNextGameValues`, `GetLiveValue` and `SetNextGameKeys`. |
| ↳ desired state | | | Scan plus `mods.json` and `mod-settings.json`; edits written by merging |
| ↳ pending | | | Pending summary and the next score key |
| ↳ notification | | | Change serial |
| ↳ writing and state | | | `SaveConfig` keeps unknown keys. Produces the state JSON for the panel and for `-dumpmods`. |
| `platform/UserFiles.*` | new | coordinator | `WriteUserFileAtomic`; the "user data changed" callback |
| `platform/Relaunch.*` | new | coordinator | `Relaunch(args)` (Windows `CreateProcessW`) |
| `platform/WebApi.cpp` | new, web only | coordinator | The `EMSCRIPTEN_KEEPALIVE` functions (4.5) |
| `framework/ModMenu.*` | new | coordinator | The widget, pages, controls, scrolling, keyboard, text editing, confirmations |
| `framework/ModMenuSkin.*` | new | coordinator | The skin interface and the default skin |
| `framework/BuiltinFont.*` | new | coordinator | A Latin-1 bitmap font (a `Font` subclass) |
| `framework/SexyAppBase.*` | changed | coordinator | F2, `OpenModMenu`, `RequestRestart` and its virtuals (6.3), the script command `modmenu`, exit info on the web. `UTF8ToLatin1` moves from file-static (`SexyAppBase.cpp:70`) to `Common`. |
| `web/modpanel.js` | new | coordinator | The Mods panel for every game, built on the C API, linked with `--post-js` |
| `web/shell.html` | changed | coordinator | Uses `modpanel.js`, restores the backup before reading, handles restart and autostart. The old panel code and `bejSyncUserData` (`:283`) go. |
| `cmake/ByopopWeb.cmake` | new | coordinator | One function for every game's web link options: shell, `--post-js`, `EXPORTED_RUNTIME_METHODS` gains `ccall` and `UTF8ToString` |
| `tools/rundesk/rundesk.c` | changed | coordinator | Waits for every process in its job, not only its child (`:90`) |
| `tools/modsettingstest/`, `tools/fixtures/mods/settings-demo/` | new | coordinator | Unit tests, and a fixture mod that is JSON only |
| `games/bejeweled/src/*` | changed | Bejeweled session | 8.2 |
| `games/bigmoney/src/*`, `games/bigmoney/web/shell.html` | changed | Big Money session | 8.3 |
| `games/bookworm/*` | changed | BookWorm session | 8.4 |

---

## 3. `settings` in mod.json

### 3.1 Example

This is the fixture mod; mask rows are shortened.

```jsonc
{
  "id": "settings-demo", "name": "Settings Demo", "version": "1.0",
  "settings": [
    { "type": "section", "label": "Play" },
    { "id": "minRun", "label": "Shortest match", "type": "int", "default": 3, "min": 2, "max": 5,
      "help": "Gems in a line needed for a match." },
    { "id": "undo", "label": "Undo", "type": "bool", "default": false },
    { "id": "undoDepth", "label": "Undo depth", "type": "int", "default": 1, "min": 1, "max": 10,
      "enabledIf": { "setting": "undo", "equals": true } },
    { "id": "shape", "label": "Board", "type": "choice", "default": "full",
      "choices": [ { "value": "full", "label": "8 x 8" }, { "value": "diamond", "label": "Diamond" } ] },
    { "id": "replays", "label": "Record replays", "type": "bool", "default": false },
    { "type": "section", "label": "Look" },
    { "id": "praise", "label": "Praise text", "type": "text", "default": "EXCELLENT!", "maxLength": 16,
      "charset": "upper" },
    { "id": "music", "label": "Music", "type": "choice", "default": "original",
      "choices": [ { "value": "original", "label": "Original" }, { "value": "remix", "label": "Remix" } ] }
  ],
  "rules": {
    "match": { "minRun": { "$setting": "minRun" } },
    "undo": { "$setting": "undo",
              "map": { "true": { "enabled": true, "depth": { "$setting": "undoDepth" } }, "false": null } },
    "board": { "$setting": "shape", "map": { "full": null, "diamond": { "mask": ["...##...", "..."] } } }
  },
  "features": { "replays": { "$setting": "replays" } },
  "strings": { "EXCELLENT": { "$setting": "praise" } },
  "files": { "music/1984-network.s3m": { "$setting": "music",
             "map": { "original": null, "remix": "extra/remix.ogg" } } }
}
```

How each setting behaves in Bejeweled:

| Setting(s) | Applies | Scores |
|---|---|---|
| `minRun`, `undo`, `undoDepth`, `shape` | At the next game | Score-affecting |
| `replays` | At the next game | Not score-affecting |
| `praise`, `music` | At restart | Cosmetic, not score-affecting |

### 3.2 Setting keys

| Key | Types | Required | Meaning |
|---|---|---|---|
| `id` | all except section | yes | `[A-Za-z][A-Za-z0-9_]{0,31}`, unique within the mod. It keys the stored value, so never reuse an id for a different meaning. |
| `type` | | yes | `bool`, `int`, `float`, `choice`, `text`, `color`, or `section` (a heading row with no value) |
| `label` | all | yes | UTF-8, at most 40 characters |
| `default` | all except section | yes | Must be valid for the type and range |
| `help` | all | no | At most 200 characters. Shown in the help line when the row is focused, and as the web panel's description. |
| `min`, `max` | int, float | float: yes; int: no (−1,000,000..1,000,000) | Inclusive |
| `step` | int, float | float: yes; int: no (1) | Values snap to `min + k·step`. At most 100,000 steps. |
| `unit` | int, float | no | A short suffix such as `s` or `%`, at most 8 characters |
| `decimals` | float | no (taken from `step`) | Number of digits shown |
| `choices` | choice | yes | 2..32 entries of the form `{ "value": <string, number or bool>, "label": "..." }`, with unique values |
| `maxLength` | text | no (24) | 1..64 characters, counted after conversion to Latin-1 |
| `charset` | text | no (`any`) | `any` (printable Latin-1); `upper` (typing gives capitals, because Bejeweled's header font has no lower case, `Res.h:123-127`); `digits`; `name` (letters, digits, space, `-`, `_`) |
| `alpha` | color | no (false) | Stores `#rrggbbaa` instead of `#rrggbb` |
| `apply` | all | no | `live`, `nextGame` or `restart`: when the author wants the value applied. If the derived level (3.5) is later, the derived level wins, with a warning. |
| `affectsScores` | all | no | `true` adds the value to the score key where it would not otherwise count, for example a data file the game reads, such as a word list. `false` is ignored. |
| `enabledIf` | all | no | `{ "setting": "<an earlier bool or choice>", "equals": <value> }`, or `"notEquals"` in place of `"equals"`. While the condition is false, the row is greyed out and the value resolves to its default, for the score key too. |

A mod may have at most 64 settings.

### 3.3 Types

| Type | Stored as | Control in the menu | Canonical text (score key and `map` lookup) |
|---|---|---|---|
| bool | `true` / `false` | Checkbox, plus "On"/"Off" | `true` / `false` |
| int | integer | `−` value `+`, plus a slider when the range has at most 200 steps | Decimal |
| float | number | Slider, plus `−` value `+` | Key: the 16 hex digits of the IEEE bits of the snapped value. No `map`. |
| choice | The choice's `value` | `‹ label ›`, which cycles | A string as is; a number in decimal; a bool as `true`/`false` |
| text | String (UTF-8 in the file) | Edit field | Key: the Latin-1 bytes, with `%`, `&`, `=` and `?` percent-escaped. No `map`. |
| color | Lower-case `#rrggbb` / `#rrggbbaa` | Swatch, hex field, and `‹ ›` through a 16-colour palette | The text itself. No `map`. |

Both platforms compute `min + k·step` in IEEE double without fast-math (CODE_MODS_DESIGN 6.1). So the snapped float and its bits are identical natively and in wasm, and no `printf` formatting takes part in the key.

### 3.4 Placeholders

| Form | Result |
|---|---|
| `{"$setting": "minRun"}` | The value itself |
| `{"$setting": "shape", "map": {"full": null, "diamond": {...}, "*": ...}}` | The entry for the value's canonical text, otherwise the `"*"` entry. A mapped entry may contain placeholders of its own. |
| `{"$setting": "tint", "as": "rgb"}` | A colour as `[r,g,b]`. Also `"rgba"` (`[r,g,b,a]`) and `"argb"` (one integer). |
| `{"$setting": "speed", "as": "int"}` | A number rounded half away from zero. `"string"` gives the canonical text instead. |

**Where placeholders are allowed:**
- inside `tunables`, `rules` and `features`, including the JSON files that `tunables` and `rules` name;
- in member values of `files`, `images` and `strings`;
- in `imageScale`;
- in elements of `imagePatches` and `imageOps`.

They are an error in `id`, `name`, `version`, `author`, `description`, `priority`, `affectsScores` and `settings`.

**Resolution:**
1. **When it runs.**
   - `ModSettings::Resolve` runs in `ModManager::Apply`, and again in the next-game commit (6.2).
   - It runs over each enabled mod's manifest, after the `tunables` and `rules` files are read (`ModManager.cpp:281-321`). The result is the *effective manifest*.
   - Everything later reads only the effective manifest: VFS remaps (`:186-213`), patches, strings, `FlattenInto`, `mRulesJson`, `mFeaturesJson` and `GameConfig::ResolveRules`.
2. **What a placeholder replaces.**
   - A placeholder is an object whose only keys are `$setting`, `map` and `as`.
   - It replaces itself, whether it is a member value or an array element.
   - There is no interpolation inside strings.
3. **What a `null` result means:**
   - Under `rules`: the existing reset to the default (CODE_MODS_DESIGN 3.5 rule 4).
   - Under `features`: the feature is off.
   - A `null` member of `files`, `images` or `strings`: the member is removed.
   - A `null` element of `imagePatches` or `imageOps`: the element is removed.
4. **Errors.** Each of these is an error:
   - a setting that is not declared;
   - a `map` that has neither the value nor `"*"`;
   - `as` used with the wrong type;
   - a placeholder in a forbidden key;
   - nesting deeper than 8 levels.

   On an error, the top-level key that holds the placeholder (`rules`, `files`, ...) is ignored for that mod, as a mod whose rules fail their own checks is treated (CODE_MODS_DESIGN 3.5 rule 7). The error is shown in the menu and the panel, and in `mods-status.json` when that file is written.
5. **Mods without settings.** A mod without `settings` is never touched: its effective manifest is its manifest.

### 3.5 When a value applies (the derived level)

| Where the value is used | Level |
|---|---|
| Nowhere in the data: game code reads it with `ModManager::GetLiveValue(mod, id)` | `apply` if given, otherwise `live` |
| `features` | `nextGame` in a game with `SetNextGameKeys(true)`, otherwise `restart` |
| `rules`, `tunables` | `nextGame` / `restart`, as for `features` |
| `files`, `images`, `imageScale`, `imagePatches`, `imageOps`, `strings` | `restart` |

- A value's level is the latest of all its uses, and of its own `apply` if that is later.
- Enabling, disabling and reordering mods always need a restart. These change three things:
  - the VFS roots;
  - the mod sign drawn into the fonts at load (Bejeweled `WinDMApp.cpp:990-991`, Big Money `WinBMApp.cpp:628-629`);
  - the score key.

### 3.6 Validation

| Check | On failure |
|---|---|
| `settings` is an array | The whole block is ignored, with an error. Any placeholders then fail under rule 4 of 3.4. |
| The `id` format; unique ids; a known `type`; `label` present; at most 64 settings | The setting is dropped, with an error |
| `default` valid for the type, inside `min..max` and on a step; `min ≤ max`; `step > 0`; float has `min`, `max` and `step`; at most 100,000 steps | The setting is dropped |
| choices: 2..32 entries, unique scalar values, each with a label; the default is one of them | The setting is dropped |
| text: `maxLength` 1..64 and the default fits it and its charset; color: the format | The setting is dropped |
| `enabledIf` names an earlier bool or choice, and its value is valid for that setting | The condition is ignored, with a warning |
| `apply` is looser than the derived level | The derived level is used, with a warning |
| `map` on a float, text or colour; `map` keys that cannot match (an unknown choice, a bool key other than `true`/`false`) | An error, under rule 4 of 3.4 |

**Stored values** (4.1) are normalised as they are read. Normalising never writes the file; it is rewritten only when the user changes something.

| Stored value | Result |
|---|---|
| Wrong type | The default, with a warning |
| Outside the range | Clamped, with a warning |
| Float off a step | Snapped to the nearest step |
| A choice that no longer exists | The default, with a warning |
| Text too long | Cut to `maxLength` |
| Characters outside the charset | Replaced with `?` |

### 3.7 Score key and RulesId

- **Which settings affect scores.** A setting is *score-affecting* when any of these holds:
  - one of its placeholders lies under `tunables` or `rules`;
  - one lies in the `files` member for `data/diff.txt`;
  - it says `"affectsScores": true`.

  A mod with a score-affecting setting is score-affecting. A mod with `tunables` or `rules` already is (`ModManager.cpp:63`).
- **Key entries.** The local key is FNV-1a over the entries in load order (`ModManager.cpp`, `HashKey`; sorted until the integration onto main, where a reorder in the menu could change the rules under an unchanged key, and Bejeweled then refused and deleted the v2 save kept under it). With one score-affecting mod, or several already in id order, the key is the same as the sorted one. Each score-affecting mod's entry is:
  - `id@version` when all its score-affecting settings are at their defaults (after `enabledIf`). This is today's entry.
  - Otherwise `id@version?` followed by `sid=canonical` pairs joined with `&`, sorted by `sid`. For example: `example-relaxed@1.0?hintPenalty=25`.
- **Consequences.**
  - `example-relaxed` keeps `savegame-58ebd9d7.dat` (CODE_MODS_DESIGN 3.7), whether or not it gains a settings block, as long as the values stay at their defaults.
  - Every gameplay combination gets its own save, its own high-score table and its own `-seeded` table (`GameConfigMods.cpp:381-391`).
  - Cosmetic values never change the key: `strings`, `files` other than `diff.txt`, images and `features`.
- **RulesId** (Bejeweled) needs no change.
  - It is computed from the merged effective rules (CODE_MODS_DESIGN 3.7), so gameplay values change it, and seed and leaderboard boards follow it.
  - Values under `features` stay outside it, as features always have.
  - Replays carry their rules, so a replay plays back under its recorded values whatever the current settings are.
- **The "changes gameplay" tag** in the panel and the menu is `AffectsScores()` of the desired state, computed in C++. The JavaScript copy in `shell.html:413-439` goes.
- **The desired-state key** (`nextScoreKey`) is computed the same way, over the desired enabled set and values.
  - It needs the mod that `data/diff.txt` resolves to. `ModCatalog::DiffOwner()` follows the same rule as `VFS::Resolve` (the highest-priority `files` remap wins, then the highest-priority file).
  - A unit test checks that `DiffOwner()` agrees with `VFS::Resolve` on the fixtures.

---

## 4. Storage and sync

### 4.1 `mod-settings.json`

```jsonc
// <user dir>/mod-settings.json: written only when the user changes a value
{
  "v": 1,
  "mods": { "settings-demo": { "minRun": 2, "praise": "SUPER!" } },  // committed: what a start applies
  "next": { "settings-demo": { "undo": true } }    // Bejeweled only: held for the next fresh game (6.2)
}
```

- **What is stored.** Only values that differ from their defaults are stored. Setting a value back to its default removes its key, and an empty mod object is removed. When an author later changes a default, users who never touched that value follow the new default.
- **Unknown content is kept.** Entries for mods that are not installed are kept, so a mod that is removed for a while keeps its values. Unknown top-level keys are also kept.
- **A missing file** means every value is at its default. Nothing is written and nothing is logged.
- **An unreadable file:**
  - All values are at their defaults, and the menu shows a warning.
  - The first write after that moves the broken file to `mod-settings.bad.json` in the user dir.
- **A newer version.** When `"v"` is greater than 1, the file is read-only and the menu says "made by a newer version".

### 4.2 `mods.json`

- The format stays `{order, disabled}`. Unknown keys are kept, and so is member order (`Json` keeps object members in order).
- **Enable and disable** add or remove the id in `disabled`.
- **Move:**
  - The order shown is the one ModManager uses: priority first, then the saved order, then new mods appended (`ModManager.cpp:97-105`).
  - A mod swaps only with a neighbour of the same priority, so ▲ and ▼ are disabled at a priority boundary. Today `Move` swaps across priorities (`ModManager.cpp:125-136`), and the next start silently undoes that.
  - The write keeps the slots of ids in `order` that are not on disk.

### 4.3 How a write happens

Every edit runs these steps:
1. Re-read the file.
2. Apply one operation.
3. If the bytes are unchanged, stop.
4. Otherwise call `Platform::WriteUserFileAtomic(rel, bytes)`:
   - write `<name>.tmp`, then replace the file (Windows `MoveFileExW(MOVEFILE_REPLACE_EXISTING)`; elsewhere `rename`);
   - call `Platform::SyncUserData()`;
   - run the "user data changed" callback. `SexyAppBase` installs this callback to set `mBackupDirty` (`SexyAppBase.cpp:1231-1235`).

The callback also fixes a gap that exists today: `ModManager::WriteStatus` syncs, but does not mark the backup (`ModManager.cpp:388-415`). Nothing is written at startup.

### 4.4 Web: before the game starts

**The problem.** `boot()` reads `mods.json` (`shell.html:577, 604-605`) before `main()` restores the localStorage backup (`main.cpp:219`). So the panel can show, and save, an older `mods.json` than the one the game will use.

**The fix.**
- After mounting, the shell first calls `byopop_userdata_restore(userDir)`.
- This is `RestoreUserDataBackup`. It is idempotent: restored files take the backup's time stamps, so `main`'s second call changes nothing (`WebPersist.cpp:89-149`).
- Only after that does the shell call `byopop_mods_open(userDir, modsDir)`. This sets the folders and scans the desired state, without running `Apply`.

The exported functions work before `callMain`, because the runtime is already initialised (`-sINVOKE_RUN=0`, `games/bejeweled/CMakeLists.txt:24-28`).

### 4.5 Web: the C API, used by the panel and the tests

| Function | When | What it does |
|---|---|---|
| `byopop_userdata_restore(dir)` | At boot, before any read | 4.4 |
| `byopop_mods_open(userDir, modsDir)` | Before `callMain` | Sets the folders and scans the desired state |
| `byopop_mods_state()` | Any time | Returns the state JSON (below) |
| `byopop_mods_edit(json)` | Any time | Takes `{op: "enable"\|"move"\|"set"\|"reset", mod, on?, delta?, setting?, value?}`. Returns `""` or an error message. |
| `byopop_mods_rescan()` | After a mod is added or removed | Rescans the mod folders |
| `byopop_menu(on)` | While the game runs | Opens or closes the in-game menu |
| `byopop_restart()` | While the game runs | Calls `RequestRestart()`: the panel's "Apply and restart" |

Events from the game to the page:
- `Module.onModsChanged()`
- `Module.onModMenuFocus(text)`: for the live region, 5.6
- `Module.onGameExit(info)`: 6.5

```jsonc
{ "running": true, "nomods": false, "scoreKey": "58ebd9d7", "nextScoreKey": "1c0a9e4f",
  "pending": { "restart": ["HD Gems turned off"], "nextGame": ["Shortest match"] },
  "mods": [ { "id": "settings-demo", "name": "Settings Demo", "version": "1.0", "author": "", "description": "",
              "priority": 0, "enabled": true, "applied": true, "pending": null, "scores": true, "features": true,
              "errors": [], "warnings": [],
              "settings": [ { "id": "minRun", "label": "Shortest match", "type": "int", "min": 2, "max": 5,
                              "step": 1, "default": 3, "value": 2, "applied": 3, "level": "nextGame",
                              "scores": true, "enabled": true, "help": "..." } ] } ] }
```

**While the game runs**, the panel and the in-game menu edit one in-memory model:
- After each edit, the model calls `Module.onModsChanged`, and the panel re-renders.
- The open in-game menu sees the change serial move.
- So there is no two-writer problem.

**Adding and removing mods** stay in JavaScript, because they need the file pickers. After either, the panel calls `byopop_mods_rescan()`.
- A new mod shows as installed, applying after a restart.
- Removing a mod that the running game applied first disables it. Its folder is deleted at the next boot, before `byopop_mods_open`. The removal is queued in the shell's localStorage key `byopop-pending-remove:<game>`. The game may still read files from that folder (`ReadDiffValues` runs at every Load, `GameConfigMods.cpp:22-39`).
- Removing a mod that is not applied deletes it at once.

### 4.6 Native: hand edits and two processes

- Users can still hand-edit both files. The menu re-reads them when it opens and before each write.
- Natively there is no single-instance check (`SexyAppBase.cpp:3149-3152`), so two processes can run at once.
  - Merging per operation keeps both processes' changes.
  - Each process keeps its own applied state.

---

## 5. The in-game menu

### 5.1 Opening and closing

- **F2** toggles the menu.
  - It must be pressed without modifiers and must not be a key repeat.
  - It is checked in `HandleEvent`'s `SDL_KEYDOWN` case, after the scripted-input filter and before Alt+Enter and widget dispatch (`SexyAppBase.cpp:1889-1931`). F2's key-up is swallowed too.
  - It works from the end of `Init` until `Shutdown`, on the title screen and in play.
- **A game button** (8.2, 8.3) can open the menu over Options. Closing the menu returns to Options.
- **Web:**
  - While the game runs, the shell's Mods button opens the HTML panel and the in-game menu together (`byopop_menu(1)`). The game is then paused and canvas input is blocked while the panel is up.
  - Closing the panel closes the menu if the panel opened it.
  - Before the game starts, the Mods button opens only the panel.
- **Scripts:** `modmenu open` (7.2).
- **Closing:**
  - F2;
  - Esc on the list page;
  - the Close button.

  Clicks on the dimmed backdrop do nothing, to avoid accidental closes on touch.

### 5.2 Pages

**List page.** Default skin, logical 640x480. The panel is `Rect(40, 24, 560, 432)`.

```
+------------------------------------------------------------------+
| MODS                                           F2 or Esc: close  |
| [x] HD Gems 1.0                                       [^][v]     |
|     by you · Sharper gem sprites                                 |
| [x] Settings Demo 1.0    SCORES  FEATURES      [^][v] [Settings] |
|     ! rules: match.minRun 2 needs 5 gem colours                  |
| [ ] Fake Money! Deluxe 1.0   RESTART                  [^][v]     |
|     ...                                                     (#)  |
| ! the game's messages (the message slot, two lines)              |
|------------------------------------------------------------------|
| Restart to apply: HD Gems turned off.     [Restart game] [Close] |
+------------------------------------------------------------------+
```

- **Each row** (44 px tall) has:
  - a checkbox;
  - the name and version;
  - badges:
    - SCORES: separate high scores;
    - FEATURES;
    - ERROR;
    - RESTART or NEXT GAME, while the row has a pending change.
  - a second line: author · description, or the first message;
  - ▲ / ▼, disabled across a priority boundary;
  - "Settings", only when the mod has settings.

  Six rows are visible at once, with a scrollbar 12 px wide.
- **Banner rows** at the top:
  - "Mods are off for this run (-nomods)";
  - "mod-settings.json could not be read".
- **The message slot**, under the content on every page (34 px, two lines, reserved even when empty). It holds the
  page's messages: on the list, the game's messages about every mod (`GetModMessages` with no mod, such as the
  placeholder banner); on a mod's page, the mod's errors and then its warnings, the shared ones and the game's.
  One message may take both lines. With several, the first two get a line each, and the second ends with
  "+N more". The rows therefore sit at the same place in every game, scrolled or not, whatever a game has to say,
  so the menu's test scripts are shared (`tools/modmenutest`).
- **Empty state:**
  - Native: "No mods installed. Mods are folders in <mods dir>."
  - Web: "Add mods in the Mods panel."

**Mod page.** Each mod with settings has one.

```
| < Back    SETTINGS DEMO 1.0                              by ...  |
| Description, wrapped, at most 3 lines                            |
| PLAY                                                             |
| Shortest match       SCORES  [-]  2  [+]  ===o====   NEXT GAME   |
| Undo                 SCORES  [x] On                              |
| Undo depth (greyed)  SCORES  [-]  1  [+]                         |
| Board                SCORES  [<] Diamond [>]                     |
| LOOK                                                             |
| Praise text                  [SUPER!__________]      RESTART     |
| ! errors and warnings (the message slot)                         |
|------------------------------------------------------------------|
| Gems in a line needed for a match.          (help of focused row)|
| [Reset to defaults]                  [Restart game] [Back]       |
```

- Setting rows are 30 px tall; nine are visible. The label column is 230 px and the control column 270 px.
- A SCORES marker shows on every score-affecting setting.
- The pending badge shows until the value is applied.

**No Apply or Cancel button.**
- Every change is saved at once, as Options applies its volume sliders live (`OptionsDialog.cpp:16-22`).
- "Reset to defaults" asks first.
- Confirmations replace the footer inline, never as a second dialog:
  - Restart: 6.3.
  - Reset: "Reset all settings of Settings Demo?" [Reset] [Cancel].

**Footer texts:**
- "Restart to apply: HD Gems turned off, Music."
- "Applies to your next game: Shortest match."
- "Waits for a fresh game: your saved game keeps the current settings."
- **A key change made while a game is saved or in progress** shows this note at the moment of the change: "Gameplay changes use separate high scores. Your current game stays saved with the current mods and continues when you switch back."

**"Restart game"** is always enabled, so the menu can also simply restart the game.

### 5.3 Look

```cpp
class ModMenuSkin                                // framework/ModMenuSkin.h; the default skin is this class
{
public:
	enum FontRole  { FONT_TITLE, FONT_BODY, FONT_SMALL, FONT_BUTTON };
	enum ColorRole { COLOR_TEXT, COLOR_DIM, COLOR_ACCENT, COLOR_ERROR, COLOR_WARN, COLOR_BADGE };
	enum State     { STATE_NORMAL, STATE_HOVER, STATE_DOWN, STATE_DISABLED, STATE_FOCUS = 0x10 };
	virtual ~ModMenuSkin();
	virtual Font*	GetFont(FontRole theRole);
	virtual Color	GetColor(ColorRole theRole);
	virtual Rect	GetPanelRect(int theWidth, int theHeight);
	virtual int		GetRowHeight(bool theListPage);
	virtual void	DrawBackdrop(Graphics* g, const Rect& theScreen);
	virtual void	DrawPanel(Graphics* g, const Rect& theRect);
	virtual void	DrawButton(Graphics* g, const Rect& theRect, const SexyString& theLabel, int theState);
	virtual void	DrawCheckbox(Graphics* g, const Rect& theRect, bool isChecked, int theState);
	virtual void	DrawSlider(Graphics* g, const Rect& theTrack, double theFraction, int theState);
	virtual void	DrawField(Graphics* g, const Rect& theRect, int theState);
	virtual void	DrawScrollbar(Graphics* g, const Rect& theRect, double thePos, double theSize, int theState);
	virtual void	DrawFocus(Graphics* g, const Rect& theRect);
	virtual void	PlayClick();
};
// SexyAppBase: virtual ModMenuSkin* CreateModMenuSkin();   // default: new ModMenuSkin()
```

**Default skin:**
- Black backdrop at alpha 160.
- Panel `#1e2230` with a 1 px `#8a93b8` border.
- Text white, dim `#b7bdd2`, focus ring `#ffd34d`, error `#ff6b6b`, warning `#ffb347`.
- Drawn only with `FillRect`, `DrawRect` and `DrawLine`.
- The default skin and the built-in font are created when the menu first opens.

**Built-in font:**
- A 12 px cell with an 8 px cap height and proportional advances.
- ASCII 32..126 is drawn by hand. Latin-1 160..255 is composed from a base glyph and a diacritic table.
- Bold is made by drawing twice, one pixel apart.
- All glyphs are drawn white into one `MemoryImage` and tinted with colorize.
- At render scale N the font scales with nearest filtering.

**Per-game skins:**
- A game skin overrides fonts and the drawing of the frame, buttons, checkbox and slider with the game's own art.
- Each part falls back to the default when its image is NULL. Bejeweled's dialog art loads only in `LoadingThreadProc` (`Res.h:58`).
- The layout stays the framework's. A skin may change only the metrics: the panel rectangle, row heights and insets.

**Text handling:**
- Mod text is UTF-8 and is converted with `UTF8ToLatin1`. Characters outside Latin-1 become `?`.
- A string with a character the skin's font lacks (zero width in its `ImageFont`) is drawn in the built-in font.
- Long text is cut with "..." and shown in full in the help line when its row is focused.
- The menu's own words come from `GetString("MODMENU_...", default)`, so a game's properties or a strings mod can translate them.

### 5.4 Modality, pausing, focus and RNG

- **Opening** runs, in order:
  1. `AddWidget`;
  2. `BringToFront`;
  3. `mWidgetManager->AddBaseModal(menu)` (`WidgetManager.cpp:250-265`), which takes focus from the board;
  4. `SetFocus(menu)`;
  5. `ModalOpen()`.
- **Closing** runs `RemoveBaseModal` (which restores the previous focus, `:267-300`), then `RemoveWidget`, then `ModalClose()`.
- **Pausing** works through the game's own modal hooks:
  - Bejeweled pauses through `Pause(mTimedMode)` (`WinDMApp.cpp:323-348`). The pause is recorded as PAUSE/UNPAUSE.
  - Big Money pauses only its Action type (`WinBMApp.cpp:230-241`).
  - Both games count pauses (`BoardRules.cpp:88-127`; Big Money `Board.cpp:611-619`), so opening the menu over Options nests correctly.
- **Staying on top.** If something adds a widget above the menu while it is open, the menu raises itself again in its Update.
- **The menu is not in the dialog map**, so code that counts dialogs is unaffected.
- **During a replay watch** the menu behaves as Options does. `Board::Pause` ignores playback boards (`BoardRules.cpp:90`).
- **No randomness.**
  - The menu never calls `Rand()`.
  - Its event handlers, Update and Draw run inside `MTAutoDisallowRand` (`MTRand.h:48-51`), so a debug build asserts on any draw.
  - A release build is checked by a test (7.3).

### 5.5 Input

| Key | List page | Mod page | While editing text |
|---|---|---|---|
| Up / Down | Previous / next row | Previous / next setting | — |
| Left / Right | — | Change the value: toggle, ±step, cycle a choice, step through the palette. Shift: ±10 steps. | Move the caret |
| Home / End, PgUp / PgDn | First / last row, scroll | Same | Caret to start / end |
| Space | Toggle the mod on or off | Toggle a bool, or activate | Types a space |
| Enter | Open the mod's page | Activate; start or commit a text edit | Commit |
| Alt+Up / Alt+Down | Move the mod in the order | — | — |
| Delete | — | Reset the focused setting | Delete forward |
| Tab / Shift+Tab | Next / previous control in the row | Same | — |
| Esc | Close the menu | Back to the list | Cancel the edit |
| F2 | Close the menu | Close the menu | Commit, then close |

- **Mouse:**
  - click;
  - the wheel scrolls;
  - drag a slider's thumb;
  - press on a list and move more than 6 px vertically to scroll instead of clicking.

  Hovering highlights controls, and the framework's hand cursor shows over them.
- **Touch (web):**
  - SDL turns touches into mouse events, so all of the above works.
  - Hit targets are at least 24 px.
  - The HTML panel is the comfortable touch view.
- **Text fields** take `KeyChar` and `SDL_TEXTINPUT` (`SexyAppBase.cpp:1933-1941`). They are limited to their `charset` and `maxLength`.

### 5.6 Accessibility

- **Keyboard and pointer:**
  - Everything works from the keyboard alone, and everything works from a pointer alone.
  - The 2 px focus ring is always visible.
  - Esc always backs out.
- **Contrast and state:**
  - The default skin has 15:1 contrast (white on `#1e2230`).
  - State is never shown by colour alone: checkboxes have a mark and "On"/"Off", and badges are words.
- **Nothing moves by itself:** no timing, no animation, no blinking caret.
- **Web screen readers:**
  - The HTML panel is the screen-reader view: real form controls with labels, help through `aria-describedby`, and a live region for pending changes.
  - The in-game menu also reports its focused item through `Module.onModMenuFocus(text)` into a polite live region in the shell.

---

## 6. Apply and restart

### 6.1 What applies when

| Change | Bejeweled | Big Money, BookWorm | Why |
|---|---|---|---|
| Enable, disable, reorder; a mod added or removed on disk | restart | restart | VFS roots, the mod sign drawn into the fonts, the score key |
| A value used in `files`, `images`, `imageScale`, `imagePatches`, `imageOps`, `strings` | restart | restart | Loaded once. Bejeweled: images `WinDMApp.cpp:754-761, 964-1017`; sounds `:971-985`; music `:770-771`; fonts `:763-764, 987-991`; strings `:743-748` and `TitleScreen.cpp:89, 108, 155`. Big Money loads everything in Init. |
| A value in the `files` remap of `data/diff.txt` | restart | restart | The values are copied into the app once (`WinDMApp.cpp:949-962`) |
| A value used in `rules`, `tunables` | next fresh game (6.2) | restart | Bejeweled calls `GameConfig::Load` at every NewGame (`WinDMApp.cpp:240`), and the board snapshots its rules (`BoardVariant.cpp:31`). Big Money reads no rules. |
| A value used in `features` | next fresh game | restart | Same reason. Features never change the key. |
| A value with no data use (`GetLiveValue`) | live | live | The code asks for the value when it needs it |

Later work, not designed here: swapping art and strings without a restart. Images would be re-decoded into the same `Image` objects. The blocker is derived images (shrunken gems, timer bars, the mod sign in the font) and cached strings.

### 6.2 Bejeweled: next-game values

1. **Holding.**
   - A pending next-game value is held in memory and in `mod-settings.json` under `"next"`.
   - So a quit before the next game keeps both the saved game and the pending value.
2. **The commit.**
   - `GameConfig::Load()` (`GameConfigMods.cpp:300-334`) starts with `ModManager::CommitNextGameValues(canCommit)`.
   - The game passes `canCommit = true` only when no game would be stranded:
     - the outgoing board is not in progress, and
     - no save exists under the current key.
   - A commit:
     - moves `"next"` into `"mods"`;
     - re-resolves `rules`, `tunables` and `features`;
     - recomputes the score key;
     - writes the file once.
   - With nothing pending, the commit returns at once, so behaviour is identical.
   - Startup is safe too: at startup the save exists until "Continue?" is answered, so nothing commits.
3. **Each `Board` snapshots its save and sidecar names** in its constructor (`port(variant)`).
   - `SaveGame`, `~Board` (`Board.cpp:602-605`), `WinDMApp::Shutdown` (`:169-184`) and `GetShutdownSaves` (`:189-208`) use the snapshot.
   - The reason: `NewGame` deletes the old board deferred (`WinDMApp.cpp:233-236`) and calls `Load` right after (`:240`). A static name would therefore put the old game's save under the new key.
   - `HighScoreWidget` already keeps its own file name (`HighScoreWidget.cpp:176, 218`).
4. **Audit every user of `GetScoreKey()` and `ScopedFileName`:**
   - `Board.cpp:27-30`;
   - `HighScoreWidget.cpp:75-76`;
   - `GameConfigMods.cpp:351, 381-391`;
   - `LiveSplitObserver.cpp:367`.
5. **The mod sign.** `IsLiveSplitOnly` (`GameConfigMods.cpp:271-298`) reads the effective manifest and ignores `settings`. A livesplit mod that gains options stays sign-free (Q18 of the code-mods design).

### 6.3 The restart (shared)

`SexyAppBase::RequestRestart()` runs these steps:
1. **`CanRestartNow(&reason)`**, a virtual that defaults to true.
   - Bejeweled and Big Money return false while their record-score dialog is open, because restarting there would drop the score, as quitting there does.
   - The reason is shown in the footer.
2. **A confirmation**, shown only when `HasGameInProgress()` is true.
   - "Restart now? Your game is saved and you can continue it after the restart."
   - When `nextScoreKey != scoreKey`, it says instead: "...stays saved with the current mods and continues when you switch back to them."
   - With no game in progress, the restart happens at once.
3. **The save.** It sets `mRestartRequested` and calls `Shutdown()`. The game's own save path then runs exactly as on Quit:
   - Bejeweled: `WinDMApp::Shutdown`.
   - Big Money: `~WinBMApp` natively, and `ShutdownHook` on the web (`WinBMApp.cpp:133-139, 1067-1073`).

   The registry is written (`SexyAppBase.cpp:1360-1377`).
4. **Test runs.** In a test run (`BYOPOP_TEST_RUN`, or `-script`/autoplay active) it logs `restart: <args>` and does not relaunch. The exception is `BYOPOP_RELAUNCH_TEST_ARGS` (7.2).

New virtuals on `SexyAppBase`, each with a default:
- `CanRestartNow`
- `HasGameInProgress`
- `GetRestartArgs`
- `GetModMessages(std::vector<ModMessage>*)`: game messages per mod, such as Bejeweled's rules status.

### 6.4 Native relaunch

Each game's `main` gains three lines:

```cpp
anApp->Shutdown();
bool restart = anApp->mRestartRequested; std::vector<std::string> args; anApp->GetRestartArgs(&args);
delete anApp;
if (restart) Platform::Relaunch(args);
```

**`Relaunch`:**
- Uses `GetModuleFileNameW`.
- Converts the UTF-8 arguments (they arrive through `SDL2main`) to UTF-16 with MSVCRT quoting.
- Calls `CreateProcessW` with the current directory and no inherited handles.
- On failure it shows: "Could not restart <game>. Your game was saved; start it again."

| Argument | Relaunched | Why |
|---|---|---|
| `-gamedir`, `-userdir`, `-modsdir`, `-changedir` | yes | The same install and saves |
| `-seed=N`, `-renderscale=N` | yes | User choices for the session |
| `-replaydir` (Bejeweled) | yes | |
| `-gameseed=N` (Bejeweled) | Only while the seed is still pending (`gGameConfig.mPendingSeed != 0`) | A used seed would be played again |
| `-nomods` | no (open question 8) | The restart is how the menu applies mods |
| `-script`, `-fast`, `-trace`, `-autoplay*`, `-tracehash`, `-replay`, `-replayquit`, `-livesplitlog`, `-codeccheck`, `-dumprules`, `-forcevariant`, anything else | no | Test hooks |

- **The keep list:** the framework keeps its own keep list, and games add theirs in `GetRestartArgs`.
- **Window placement:** the new process restores `PreferredX/Y` and `ScreenMode` from the registry (`SexyAppBase.cpp:998-1009`), so it opens where the old window was.
- **LiveSplit:** the process exit resets a running run (the ASL's `exit`, `BejeweledDeluxe-byopop.asl:23-29`). This is documented in the livesplit README.
- **rundesk:** it puts its child in a job object, waits until the job is empty and ends whatever is left. Today it waits only for the child (`rundesk.c:90`).

### 6.5 Web restart

1. Steps 1 to 3 of 6.3 run. The values were already written and synced when they were edited, and each write marked the backup dirty.
2. **The main loop ends.** `MainLoopFinished` deletes the board, which saves the game. Then the backup is forced, and `sexyapp_notify_exit` runs (`SexyAppBase.cpp:2409-2416`). That call now passes `{restart: true, dropped: ["-gameseed"]}`.
3. **The shell's `onGameExit(info)`:**
   - suspends audio;
   - shows "Restarting…" instead of "Thanks for playing";
   - waits for `sexySyncBusy` and `sexySyncPending` to clear (`shell.html:227-242`);
   - sets `sessionStorage['byopop-autostart']`;
   - removes `seed` from the URL with `history.replaceState` when `-gameseed` was dropped, and `replay` (Big Money's
     page) when `-replay` was (`Session.restartPage`, `web/modpanel.js`);
   - calls `location.reload()`.

   `?silent=1` stays in the URL.
4. **Boot after the reload:**
   - The backup is restored first (4.4).
   - With the autostart flag set, the game starts at once, without the Play overlay.
   - Audio stays silent until the first click or key (`Audio.cpp:156-170`), and the status line says so.
5. **The lock race.** The page's Web Lock (`shell.html:250-266`) is freed on unload. A second tab that is already waiting takes the lock first, and this page then shows "Already open". This is accepted and documented; it is what a manual reload does today.

### 6.6 Games in progress

| Case | What happens |
|---|---|
| Restart, score key unchanged | The game is saved at shutdown, and "Continue your game?" is offered after the restart. |
| Restart, key changed (a gameplay mod toggled, or a restart-level gameplay value) | The game is saved under the old key. The confirmation and the change-time note say so. The game comes back when those mods or values are switched back. |
| A next-game gameplay value while a game is in progress or saved (Bejeweled) | Held back (6.2); it never strands the game. |
| Quit, or the tab closed, with restart-level changes pending | The changes are already in the files and apply at the next start. If they change the key, the note at the time of the change said where the game goes. |
| Page killed without unload events | The localStorage backup holds the save and both settings files (`WebPersist.h:4-13`). |
| Relaunch fails | The game was saved before the attempt, and a message says to start it again. |

---

## 7. Fidelity and tests

### 7.1 Why nothing changes without the menu

- **The hotkey** is on the real-input path only. Scripted runs drop real input first (`SexyAppBase.cpp:1724-1746`). No oracle or autoplay script sends `key F2`. They send click, down, up, move, rclick, `char`, and (Big Money) `key BACK` and `key RETURN`.
- **Nothing is created** before the menu first opens: no menu, no skin, no font.
- **Loading is unchanged** for mods without `settings`:
  - The effective manifest is the manifest, so the tunables, rules and features JSON, the RulesId and the score key are identical.
  - A missing `mod-settings.json` gives no read error, no log line and no write.
- **Options buttons** appear only when an installed mod has settings (8.2, 8.3). None of the oracle's mods have settings: `example-*`, `livesplit`, `true-gem`, `mines-trilliant`, `bejeweled2-*`, `hd4x-textures`, and Big Money's `fake-money`. This matters because the `seed_keyed` case opens Options with a tunables mod enabled.
- **The next-game commit** returns at once when nothing is pending. Each board's snapshot names equal the static names, so the files are identical.
- **The web exit notification** gains an argument. The non-restart path of the shell is unchanged.
- **The proofs:**
  - Bejeweled: `compare.sh --variants default,forcevariant,novariants`.
  - Big Money: its script and autoplay comparisons.
  - BookWorm: its own tests, once they exist.
  - `fwtest`.

### 7.2 Script commands and switches

These are framework commands in `RunScript`, so every game has them:

```
<n> modmenu open | close
<n> modmenu page <modId> | list
<n> modmenu enable <modId> 0|1
<n> modmenu move <modId> -1|1
<n> modmenu set <modId> <settingId> <value as JSON>     e.g. 3, true, "diamond", "#ff8800"
<n> modmenu reset <modId>
<n> modmenu restart                                     test run: shutdown save + "restart: <args>" log
<n> modmenu dump <file>                                 the state JSON, as byopop_mods_state
```

- **They use the UI's own functions.** Edits write the user dir, and page changes redraw.
- **Keys and clicks.** Keys inside the menu use the existing `key UP|DOWN|LEFT|RIGHT|RETURN|ESCAPE` and `char x`. Clicks use `click x y` at the layout coordinates of 5.2.
- **`key F2` deliberately does not toggle the menu.** It goes to `WidgetManager::KeyDown`, not to `HandleEvent`. The hotkey is tested in `fwtest`, which feeds SDL events into `HandleEvent`.
- **`-dumpmods`** prints the state JSON and exits before the window opens, like `-dumprules`.
- **`BYOPOP_RELAUNCH_TEST_ARGS`.** In a test run, setting this variable makes the game really relaunch, with the kept arguments plus these, for example a second `-script` that ends in `quit`. The relaunched process stays under the same rundesk.

### 7.3 Tests

| Test | Where | Checks |
|---|---|---|
| `modsettingstest` | native, and Node (wasm) | Every row of 3.6 and the stored-value rules; placeholder resolution (map, as, the null rules, nesting, errors); level derivation; score-key vectors, including `example-relaxed@1.0` giving 58ebd9d7 with and without a defaults-only settings block; float bits equal on native and wasm; `DiffOwner` agrees with `VFS::Resolve` |
| Oracle, the existing cases | `compare.sh` (all three variants); Big Money's scripts | Identity |
| `modmenu_screens` | Bejeweled and Big Money, with the fixture mod in a temporary mods dir | Screenshots of the list, a mod page, confirmations, the menu over Options and on the title screen, in the default skin (`-modmenuskin=default`) and the game skin |
| `modmenu_edit` | scripts | After `set`/`enable`/`move`/`reset`: the exact bytes of `mod-settings.json` and `mods.json`; unknown keys kept; no write for a no-op; nothing written in the mods dir (`compare.sh:364-373`) |
| `modmenu_pause` | Bejeweled | Opening and closing the menu mid-game gives the same `-trace`/`-tracehash` as opening and closing Options at the same updates; `Sexy::GetRandCount()` is unchanged; a recording that uses the menu verifies in `bejeweled_verify` |
| `modmenu_keys` | scripts | Keyboard only: navigate, change each type, edit text, every Esc path |
| `modmenu_nextgame` | Bejeweled | **Mid-game change:** a value set mid-game leaves the running trace unchanged. **New Game after game over:** new RulesId in `mods-status.json`, new key files. **Mode switch with a game in progress:** the value is held, and the save is under its old name; Continue is offered. |
| `modmenu_restart` | Bejeweled, Big Money | **First run:** `modmenu restart` mid-game writes the shutdown save, and the log has the kept arguments. **Second run with those arguments:** the Continue dialog appears and the value is applied. **Real relaunch:** with `BYOPOP_RELAUNCH_TEST_ARGS` under rundesk (job wait). **Folder name:** also run from a folder with non-ASCII characters. |
| `fwtest` hotkey | fwtest | An F2 key-down through `HandleEvent` toggles the menu. With modifiers, as a repeat, or in script mode, it does not. |
| Web | Browser pane, `?silent=1`, both games | **Before start:** the panel's state equals native `-dumpmods` for the same mods. **In game:** F2 opens the menu (screenshot); a toggle in the panel shows in the menu and the other way round. **Restart:** reload, autostart, Continue. **Persistence:** a tab closed without unload events keeps a changed value; `?seed=` is kept or dropped per 6.5. |
| `check_rng_use.py` | static | Covers `framework/ModMenu*.cpp`, `BuiltinFont.cpp` and `platform/ModSettings.cpp`: no `Rand` |

---

## 8. Per-game integration

### 8.1 Shared (coordinator)

Everything in 2.3 that is marked as coordinator-owned:
- the model;
- the menu and its one look: the default skin, drawn with primitives and the built-in font (decision 2);
- the restart plumbing and relaunch;
- the script commands and `-dumpmods`;
- the C API, `modpanel.js` and `cmake/ByopopWeb.cmake`;
- rundesk;
- the unit tests and the fixture mod.

A shared `web/modpanel.js` is a step towards the survey's shell template (G6, `docs/POPCAP_GAMES_SURVEY.md:667`).

### 8.2 Bejeweled (Bejeweled session)

- **Look:** the shared default skin, as in every game (decision 2). The menu draws no Bejeweled art, fonts or
  sounds, so nothing waits for loading.
- **Options:** a "MODS" `DialogButton` with a new id under 1000, in the register slot (`mX+30+bw`, `mY+166`; CODE_MODS_DESIGN 2.6). The slot shows:
  - "EXTRAS" when an Extras feature is on (stage 13). The Extras dialog then gets a "Mods..." button.
  - Otherwise "MODS", when an **enabled** mod has settings. Bejeweled gates on an enabled mod, not on an
    installed one as Big Money does (8.3): the oracle's examples case has `example-settings` installed but off
    (compare.sh lists every mod outside its six as disabled), and its Options screens must not change.
  - Otherwise nothing: the original row.
- **The Extras gate** must also leave out livesplit-only (an amendment to CODE_MODS_DESIGN 2.6). LiveSplit adds nothing to Extras, and the livesplit oracle runs open Options.
- **Next-game values and per-board names** (6.2; decision 3). Main sets the next-game filter
  (`ModManager::SetNextGameFilter`, As built M0/M1) to the paths it re-reads at a new game, so gameplay and
  feature values apply at the next fresh game without a restart.
- **`GetModMessages`:** the rules errors, dropped mods and warnings of `RulesStatus` per mod (`GameConfigMods.cpp:209-253`).
- **When `mods-status.json` is written** (unchanged rule), its entries for enabled mods gain `values`.
- **`GetRestartArgs`:** `-replaydir`, and `-gameseed` only while it is pending.
- **`CanRestartNow`:** false while `DIALOG_RECORD_SCORE` is open.
- **`HasGameInProgress`:** `mBoard && mBoard->IsGameInProgress()`.
- **Main:** the relaunch lines and `-dumpmods`.
- **Shell:** the JavaScript for tags, `diff.txt` detection and status goes, because the API provides them. Seeds and replays stay.
- **Known gaps from the integration onto main (2026-10-19), for M5:**
  - On the web, Bejeweled's rule errors, dropped mods and warnings (`mods-status.json`, including stage 5's
    warnings) show nowhere until `GetModMessages` above: main's shell read that file, the shared panel reads only
    the state JSON. CODE_MODS_DESIGN 3.5 rule 8 still describes the old shell.
  - `bejeweled_verify rulesid` and `bv_rules_id` (`games/bejeweled/src/headless/Verifier.cpp`, `RulesIdJson`) merge a
    mod.json's raw `tunables` and `rules`, so a mod with `$setting` placeholders gets a wrong RulesId with no error.
    Resolve the placeholders at their defaults first (`ModSettings::Parse`, `Analyze`, `Resolve`; add
    `platform/ModSettings.cpp` and `platform/Text.cpp` to the verifier's sources) and report setting errors in
    its `errors`.
- **As built (M5, 4facce2, merged at e679312):**
  - Options has the MODS button in the register slot (`mX+30+bw`, `mY+166`) while an enabled mod has settings. Extras
    (stage 13) is not built yet, so the EXTRAS case waits for it.
  - The next-game filter is `GameConfig::IsNextGamePath` (`GameConfigMods.cpp`). It covers every path under `rules`,
    `tunables` and `features` except `features.livesplit`, because the LiveSplit observer's state outlives a board.
    Those values commit at `NewGame`, and an untouched board is made again with them (modmenutest's Bejeweled
    nextgame scripts).
  - A mod's settings page shows the game's `GetModMessages`: the rules errors, dropped mods and warnings. That
    closes the first known gap above.
  - `bejeweled_verify` resolves `$setting` placeholders at their defaults (`ModSettings` `Analyze` and `Resolve` in
    `Verifier.cpp`) and links `platform/ModSettings.cpp` and `platform/Text.cpp`. That closes the second.
  - The four hooks say `override`: `HasGameInProgress`, `CanRestartNow`, `GetRestartArgs` and `GetModMessages`.

### 8.3 Big Money (Big Money session; built in M6, d4fe95b)

M6 was built on a merge of main (ed588ec), so it has the trunk's `GetShutdownSaves` and stages 2b and 3.

- **Look:** the shared default skin, as in every game (decision 2). The menu draws no Big Money art or fonts.
- **Options:** while any installed mod, on or off, has settings, the Register slot (created but never laid out
  in this build, `OptionsDialog.h`) holds a MODS button (`OPTIONS_MODS`). It opens the menu over Options, which
  stays open. Without such a mod the row is the original. Bejeweled gates on an enabled mod instead (8.2).
- **Pausing:** the menu inherits `ModalOpen`, which pauses only the Action type. The other types have no timer, so nothing advances without input.
- **Scores:** the save and the high-score files were already scoped by the score key since 9538925, through Big
  Money's own `ScopedUserFileName` (`ModRules.h`; `Board.cpp`, `HighScoreWidget.cpp`), so a gameplay setting gets
  its own tables. Nothing changes while the key is empty.
- **Values:** all values are restart-level. Big Money sets no next-game filter.
- **Effective manifests:** `ModRules` reads each enabled mod's effective manifest (the settings' values put in),
  so a value that leaves a key out (`$omit`) counts. `settings` itself is a meta key.
- **`HasGameInProgress`:** `CanSaveGame()`, a game the shutdown would save (never a replay).
- **`CanRestartNow`:** false while the record-score dialog (`DIALOG_RECORD_SCORE`) is open: the high-score name is
  being entered.
- **`GetRestartArgs`:** the framework's, plus `-replaydir` only when it was given (without it the folder follows
  the user dir again), and `-gameseed` while that seed still waits for its game. Not `-replay`.
- **Main:** the relaunch lines and `-dumpmods`.
- **Web shell** (M4):
  - It uses `modpanel.js` through `byopop_web_game`, with the restart and autostart paths.
  - It keeps `/bmpersist` and its lock name `big-money-deluxe-instance`.
- **The `$` mod sign** (`WinBMApp.cpp:628-629`) is unchanged.

### 8.4 BookWorm (BookWorm session, M7)

BookWorm runs (milestones 2 and 3, merged at 1b89ee2). The framework already gives it the menu: F2 opens it, in
the shared look, on any screen. M7 adds:
- **Look:** none of its own: the shared default skin, as in every game (decision 2).
- **Main:** the relaunch lines (`TakeRelaunch`, `Platform::Relaunch`) and `-dumpmods`. Until then the menu's
  Restart game shuts BookWorm down, saved as on Quit, and does not start it again.
- **Web:** the game links through `byopop_web_game` (`cmake/ByopopWeb.cmake`), so its page gets `modpanel.js`,
  the C API, restarts and autostart. Today `games/bookworm/CMakeLists.txt` sets its own link options and
  `games/bookworm/web/shell.html` has its own Mods panel.
- **Options:** a MODS button only if the game has a free slot, and only when a mod has settings.
- **Scores:**
  - Mods that change the word list or other play data must count for the score key. Their settings use `"affectsScores": true`, or BookWorm marks those data paths the way Bejeweled marks `diff.txt`.
  - Its save and high-score files are scoped with the key.
- All values are restart-level.
- The menu needs only `SexyAppBase`, `WidgetManager` and `Graphics`, which the 2004 compatibility layer shares.

---

## 9. Implementation plan

Sizes are new or changed lines.

| # | Stage | Owner | Contents | Tests | Size |
|---|---|---|---|---|---|
| M0 | Groundwork | coordinator | `SaveConfig` re-reads, merges and keeps unknown keys; atomic user-file writes and the backup-dirty callback (also for `WriteStatus`); both shells' `saveModConfig` keep unknown keys; `byopop_userdata_restore` before the shell reads; `UTF8ToLatin1` moved to `Common` | Oracle; a `mods.json` with an unknown key survives a panel save; a status write reaches the backup | ~200 |
| M1 | Settings model | coordinator | `ModSettings`; effective manifests in `Apply`; desired state, edits, `mod-settings.json`, pending changes, the next key; score-key entries; `-dumpmods` | `modsettingstest` (native and Node); oracle identical; the `example-relaxed` key | ~1,300 (≈450 of it tests) |
| M2 | Restart | coordinator, plus 3 lines in each game's main | `RequestRestart` and its virtuals; `Relaunch`; web exit info; shell restart and autostart; rundesk job wait; `modmenu restart` | The restart rows of 7.3; oracle | ~450 |
| M3 | Menu | coordinator | `BuiltinFont`, the one look (the default skin; no per-game skins, decision 2), `ModMenu`, F2, the `modmenu` commands, strings | screens, edit, keys and pause tests in both games; the fwtest hotkey test; RNG | ~2,100 |
| M4 | Web views | coordinator, plus the Big Money session for its shell | The C API, `modpanel.js`, both shells, `cmake/ByopopWeb.cmake` | The web row of 7.3 | ~900 (the shells lose more JS than this adds) |
| M5 | Bejeweled | Bejeweled session | The Options slot rule (MODS while an enabled mod has settings), messages, restart arguments, `CanRestartNow`, `HasGameInProgress`, next-game values with per-board names; the shared look | Oracle ×3 (the examples case's Options screens unchanged); the `modmenu_*` tests; `modmenu_nextgame`; replay verify | ~550 |
| M6 | Big Money | Big Money session, after a trunk merge | **Built (d4fe95b):** Options MODS while an installed mod has settings, effective manifests in `ModRules`, `HasGameInProgress`, `CanRestartNow`, `GetRestartArgs`; the shared look. The scoped files (9538925), the main lines and the shell were already in | `modmenutest bigmoney`; its screenshot regression; autoplay | ~350 (built: ~80) |
| M7 | BookWorm | BookWorm session (it runs since milestone 2) | The main lines, the web link through `byopop_web_game` and `modpanel.js`, the scores rule for word lists; the shared look | Its own tests; `modmenutest` once it covers BookWorm | ~200 |
| M8 | Docs | coordinator and the games | `docs/MOD_MENU.md` for mod authors (settings, placeholders, levels, scores); README Mods sections (F2, `mod-settings.json`); the schema comment in `ModManager.h` | — | ~300 |

**Total:** about 6,350 lines.

**Order:**
- M0, then M1.
- Then M2 and M3, in either order.
- Then M4.
- Then M5 and M6, which can run in parallel.
- Then M7.

The next-game part of M5 can be split off if the user chooses restart-only (open question 6).

---

## 10. Risks

| Risk | Mitigation |
|---|---|
| A gated button or a new code path changes an oracle screen | Buttons appear only with an installed mod that has settings; every hook is inert without settings; the oracle runs in all three variants |
| High-score tables multiply, one per combination of values | Intended for gameplay values. Defaults keep today's key, and the SCORES marker shows which settings split the tables. |
| A saved game seems lost after a key change | Next-game values wait (6.2); the change-time note and the restart confirmation say where the game is; switching back brings it back |
| The menu draws a random number or changes play | No `Rand` (static check and counter test); the menu's modality equals Options' (trace test, replay verify) |
| Web: a waiting tab takes the lock on reload | Documented; the restarted page waits behind "Already open", as after a manual reload |
| Web: edits made before start are lost to a stale backup | `byopop_userdata_restore` before any read (4.4) |
| Two writers: panel and menu, hand edits, two native processes | One in-memory model on the web; re-read, merge and write per operation; atomic replace |
| Relaunch fails (a non-ASCII path, antivirus) | Wide-character `GetModuleFileNameW` and `CreateProcessW`; a test from a non-ASCII folder; a message box; the game is saved first |
| The built-in font is hard to read on large windows | Game skins use the games' fonts; the default font is crisp at integer render scales; open question 3 |
| A skin draws art that has not loaded yet | Every skin part falls back to the default when its image is NULL |
| Mod authors misjudge when a value applies | Derived levels are shown as badges; `docs/MOD_MENU.md`; a warning when `apply` is looser than the derived level |
| An old build reads a mod that has placeholders | It reports the key as invalid and ignores it, and never crashes. All games ship from one trunk. |
| Big Money's branch is behind the trunk | M6 starts with a trunk merge |
| A test run relaunches and leaves processes behind | No relaunch in test runs unless `BYOPOP_RELAUNCH_TEST_ARGS` is set; rundesk waits for and ends its whole job |

---

## 11. Open questions for the user

Recommendations are in brackets.

1. **Hotkey.** [F2 without modifiers, in every game and on every platform. On the web, the shell's Mods button also opens the menu; it covers Mac laptops, where F2 needs Fn, and touch. A MODS button in Options where a game has a free slot.]

   Alternatives considered:
   - F1 opens browser help in some browsers.
   - F10 is the Windows menu key.
   - Esc and letters: Bejeweled pauses on every typed character, and letters are typed into name fields.
   - Browsers take several Ctrl and Alt chords; Firefox uses Ctrl+M to mute a tab.
2. **Look.** [Per-game skins built from each game's own dialog art and fonts, for example Bejeweled's dialog box and Big Money's green-on-black dialogs. The shared default skin serves games without a skin and anything a skin lacks.] Alternative: the default skin everywhere, a single "port" look that is clearly separate from the original game and is less work.
3. **Built-in font.** [Write our own 12 px Latin-1 bitmap font in the repo.] Alternative: a public-domain or OFL pixel font, which needs your OK to download.
4. **Web shell panel.** [Keep it, rebuilt on the game's own model. It stays:
   - the place to add and remove mods;
   - the rescue when a mod stops the game from starting;
   - the accessible and touch-friendly view.

   While the game runs, it opens together with the in-game menu.] Alternative: reduce it to adding and removing mods.
5. **What restarts by itself and what asks.**
   - [Nothing restarts by itself.
   - "Restart game" asks only when a game is in progress, and says whether that game continues after the restart or waits for its mods. With no game in progress it restarts at once.
   - Closing the menu with changes pending does not ask: the changes are saved and apply at the next start.]
6. **Next-game values in Bejeweled.** [Yes: gameplay and feature values apply at the next fresh game without a restart. They are held back while a saved game exists.] Alternative: a restart for everything, which is simpler and drops 6.2.
7. **The Options button.** [Show it only when an installed mod has settings. This keeps every oracle screen identical.] Alternative: show it whenever a mod is enabled. The `seed_keyed` Options screenshot then changes, and the change is accepted as intended.
8. **`-nomods` on restart.** [Drop it. The menu says "Mods are off for this run" at the top, and Restart applies the mods.]
9. **Removing mods from the in-game menu.** [No. Natively, mods are folders you manage; on the web, the panel's Remove stays.]
10. **Big Money high scores.** [Scope Big Money's save and high-score files with the score key now. Nothing changes until a Big Money mod changes play, and then its gameplay settings get their own tables.]

---

## Appendix A. Adversarial check of MOD_MENU_DESIGN against the code

I read the code only; nothing was built or run.
- **Trunk:** `code-mods` at 0376759.
- **Big Money:** `claude/bigmoney-1912c8` at **9538925**, which is newer than the 7e57e76 the design cites.
- **SDL:** `E:/retools/emsdk/upstream/emscripten/cache/ports/sdl2/SDL-release-2.32.10`.

### A. Corrections, most severe first

#### A1. Closing the menu with Esc or Enter pauses the Bejeweled board
- **What happens:** `HandleEvent` sends `KeyDown(ESCAPE)` to the widget manager. Right after it, it sends `KeyChar(0x1B)` (`SexyAppBase.cpp:1927-1929`). The same happens for Enter (`\r`), Backspace and Tab.
- If the menu closes on the key-down, `RemoveBaseModal` has already given focus back to the board. The key-char then reaches `Board::KeyChar`, which toggles a user pause on any character (`BoardRules.cpp:588-594`). That pause is recorded as `PAUSE`.
- The script command `key ESCAPE` does the same thing: key-down, key-char, key-up (`SexyAppBase.cpp:2955-2961`).
- **Fix:** act on the key-char for Esc and Enter, the way `EditWidget` does, or defer the close to the menu's `Update`.
- **Close order:** mirror `KillDialog`'s order: `RemoveWidget`, then `ModalClose`, then `RemoveBaseModal` (`SexyAppBase.cpp:675-682`).

#### A2. Web: SDL takes keys from the whole window
- No `SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT` is set, so keydown, keyup and keypress are registered on `window` (`SDL_emscriptenevents.c:984-991`).
- SDL calls `preventDefault`:
  - on every keypress, because text input is on (`SexyAppBase.cpp:2042`; handler at `:852-857`);
  - on Backspace, Tab, arrows, F-keys and Ctrl chords at keydown (`:833-849`).
- **Consequences while the game runs:**
  - The HTML panel's text, number and colour fields cannot be typed into.
  - Tab, Backspace and the arrows do not work in the panel.
  - Every key typed in the panel also reaches the game. With both UIs open, Space toggles a mod and Delete resets a setting in the in-game menu.
- **Fix:** the shell stops propagation of key events from the panel's root (a capture listener for keydown, keyup and keypress). The panel then handles F2 and Esc itself.
- **"Canvas input is blocked"** has no mechanism yet; one has to be specified.
- **Opening both UIs:** the slide-over panel (`width: min(440px,100%)`, `shell.html:84-85`) covers the right side of the in-game menu, where its footer and the Restart confirmation sit. Recommendations:
  - while the panel is open, the game shows only a modal "paused – Mods panel open" state;
  - `byopop_restart(confirmed)` returns any confirmation text to the panel.

#### A3. The C API would not be linked
- `platform/*.cpp` is globbed into the **STATIC** library `sexyframework` (`framework/CMakeLists.txt:6-11`).
- `EMSCRIPTEN_KEEPALIVE` functions that only JavaScript references are never pulled out of the archive.
- **Fix:** list them in `-sEXPORTED_FUNCTIONS`, which replaces the default, so `_main` must be added too. Alternatively, compile `WebApi.cpp` into each game target.

#### A4. The resolution order in `Apply` is impossible as written
- The `files` remaps are processed at `ModManager.cpp:186-213`, the `diff.txt` owner at `:215-218`, and images and patches at `:228-253`. All of that runs *before* the `tunables`/`rules` files are read at `:274-321`.
- **Fix:** restructure `Apply` in this order:
  1. read the referenced files;
  2. resolve the placeholders;
  3. then remaps, the `diff.txt` owner, patches and merges.
- `LoadManifest` computes `mAffectsScores` from the *raw* manifest (`:63`), and `mHasRules`/`mHasFeatures` use `man.Has()` (`:299-300`).
- Compute every flag from the **effective** manifest:
  - `AffectsScores`, `HasRules`, `HasFeatures` and `HasRulesOrFeatures` (which decides whether `mods-status.json` is written);
  - `IsLiveSplitOnly`, which today returns false for any extra key such as `settings` (`GameConfigMods.cpp:271-298`).
- Otherwise a mod whose rules resolve to `null` at its defaults still splits the scores and writes the status file.

#### A5. `nextGame` is wrong for rule keys that need art generated at load
- `board.gemColors` (3..12, `GameRules.h:116-117`) needs gem-sheet rows and shrink images built in `LoadingThreadProc`:
  - today there are 7 fixed rows (`WinDMApp.cpp:1001-1016`);
  - stage 8 plans `max(7, gemColors)` rows and `EnsureGemSheetRows` (`CODE_MODS_DESIGN.md:352`).
- A placeholder in `rules.board.gemColors` raised at the next game would draw past the generated art.
- **Fix:** replace `SetNextGameKeys(bool)` with a game-supplied predicate per key, or a list of load-time keys. Alternatively, the commit checks the loaded capacity and otherwise holds the value for a restart.

#### A6. `null` under `rules` is "reset to default, removing another mod's rule"
- This is rule 4 of `CODE_MODS_DESIGN.md` 3.5.
- In the design's own example, `"undo": {...map: {"false": null}}` and `"board": {...map: {"full": null}}` actively wipe any lower-priority mod's undo and board shape.
- **Fix:** add an "omit" result for placeholders (for example a `"$omit"` sentinel) that removes the member, and keep `null` for an intentional reset.

#### A7. A user's text can reach a printf format string
- `Board.cpp:230` runs `StrFormat(mApp->GetString("LEVEL").c_str(), mLevel + 2)`.
- A `text` setting placed under `strings` (charset `any` allows `%`), or any strings mod, can inject `%s` or `%n` and crash the game.
- **Fix:** validate overrides of keys whose base value contains conversions: they must keep the same conversion sequence. Also forbid `%` in text values that land in `strings`.

#### A8. Edits to the desired state must never touch `mMods`
- `GameConfig::ResolveRules` re-reads `GetMods()` and `mEnabled` at **every** `NewGame` (`GameConfigMods.cpp:44-57`).
- Today's `SetEnabled`/`Move` mutate `mMods` (`ModManager.cpp:118-136`). Wiring them up, or any edit that does the same, would merge a "restart-pending" mod's rules at the next game, while the VFS roots and the score key stay stale.
- **Fix:** keep the desired state separate from `mMods`; retire or rename those two calls.
- **The commit:**
  - `CommitNextGameValues` must not call `Apply`, which clears the VFS roots, image patches and scale overrides (`:174-184`).
  - It must rebuild the flat `mTunables` map as well as the per-mod JSON: Big Money reads `GetTunable` in each `Board` constructor (`Board.cpp:3629, 3691-3740` @9538925).

#### A9. The board created at startup misses next-game values
- `TitleScreenIsFinished` creates the board before the player does anything (`WinDMApp.cpp:931-939`).
- "Continue? No" deletes the save, but the fresh board keeps the old rules (`:855-858`).
- So a value set before the first move, or after declining Continue, applies one game late, while the menu says "applies to your next game".
- **Fix:** when values are pending and the board is untouched (`mIsNewGame`) with no save, call `NewGame()` again. It draws `Rand()`, but only when something is pending, so the oracle is unaffected. Run it outside the menu's `MTAutoDisallowRand` scope.

#### A10. The Big Money facts are out of date (tip 9538925, "gameplay tunables, separate high scores, set seed")
- **Tunables:** Big Money already reads tunables (`ModRules.h/.cpp`, `Board::LoadTunables` per board) and ships `mods/small-change` and `mods/high-roller`.
- **Scoped files:**
  - It already has `ScopedUserFileName` for `savegame.dat` (`Board.cpp:215`) and for the high scores (`HighScoreWidget`'s `mFileName`).
  - So open question 10 and the "adopts a shared `ScopedUserFileName`" part of 8.3 are moot; the work is now to move that helper into `platform/`.
- **Seeds:** `-gameseed` and `?seed` exist in Big Money's `main.cpp` and its `shell.html`. So 8.3 needs:
  - `GetRestartArgs` to keep `-gameseed` only while `mPendingSeed != 0`;
  - the shell to drop `seed` from the URL once the seed is used.
- **Next-game values** are now cheap for Big Money, because tunables are read per board. But `Board::GetSaveGameFileName` is static, so a per-board name snapshot would be needed first.
- **Other details:**
  - The mod sign is at `WinBMApp.cpp:630`.
  - `RecordScoreDialog` (id 3) is **non-modal**.

#### A11. Holes in the score key
1. **A score-affecting value applied live.** `affectsScores: true` together with `GetLiveValue` would change the key mid-game:
   - `HighScoreWidget`, LiveSplit `VANILLA` and the static save names all read the current key;
   - replays do not carry live values.

   Force such a setting to at least `nextGame` (or `restart`), and forbid gameplay use of live values.
2. **Canonical text needs escaping and uniqueness.**
   - Choice strings are used "as is", so `&`, `=`, `?` and `;` can forge entries. The entries are joined with `;` in the FNV hash (`ModManager.cpp:337`). Text escaping also omits `;`.
   - The number `1` and the string `"1"`, or `true` and `"true"`, produce the same canonical text, which breaks both the key and `map` lookups.
   - Require choice values to be unique by canonical text, and restrict them to ids, integers and bools (no floats, so no printf).
3. **The "a mod with a score-affecting setting is score-affecting" rule.** It adds `id@version` at the defaults even for a mod that was not in the key before. Example: a mod whose only link is `files["data/diff.txt"]` mapped to `null` by default. This breaks principle 5.
   - Derive it from the effective manifest instead: a score-affecting setting off its default, or today's rules.
4. **Detecting the `diff.txt` remap.** It must use `Lower(Normalize(key))`, as `VFS::SetRemaps` does (`VFS.cpp:205`). Otherwise `"Data\\Diff.txt"` slips through.
   - Better: make gameplay data paths game-registered, for example `ModManager::SetGameplayFiles({...})`. Today `"data/diff.txt"` is hard-coded in `platform/`, and BookWorm's word list needs the same rule.
5. **No protection beyond a 32-bit FNV.**
   - The save has no rules check: `LoadGame` loads whatever `savegame-<key>.dat` holds.
   - A collision between value combinations, or an author changing a default without bumping `version`, resumes a save under different rules.
   - Consider storing the RulesId or the key text in the sidecar or save trailer and checking it on Continue.
6. **Integer ranges.**
   - `as:"argb"` yields up to 4294967295, and int `min`/`max` are unbounded.
   - `Json::AsInt` is `(int)mNumber`, which is undefined out of range and may trap in wasm (CODE_MODS_DESIGN 6.1).
   - Limit ints to int32, make `argb` signed, and clamp before casts.

#### A12. Float snapping does not match the JSON literal
- `min + k*step` in double gives `0.30000000000000004` for 3 × 0.1, not 0.3.
- So a float setting yields a different RulesId from a mod that writes `0.3`, which breaks principle 3. The "default on a step" check also rejects ordinary decimal defaults.
- **Fix:** snap in scaled integers: `(round(min·10^d) + k·stepUnits) / 10^d`. A division of exact integers is correctly rounded, so it equals `strtod` of the decimal text. Use `k`, or that value, for the key.

#### A13. Bounds and security of mod-provided values
- **Range against the rule's own range.** A setting whose range exceeds the target field's range (for example `match.minRun` 2..5, `GameRules.cpp:107`) gets the mod's whole `rules` dropped at the next game (rule 7).
  - Check setting ranges against `gFields` at load, with a warning, and validate the desired state through a game hook. `GetModMessages` covers only the applied state, so the user should learn before the next game.
- **Text in path positions.** Text and colour values in `files`, `images` or `imagePatches.image` become paths. `VFS::Normalize` keeps a leading `..` (`VFS.cpp:47-52`), so the path can leave the mod folder.
  - Allow only choice→`map` to author-listed paths in path positions, and reject paths that leave the mod root.
- **Non-string values under `strings`.** These become `""` (`kv.second.AsString()`, `ModManager.cpp:258`). Require `as:"string"`, or convert scalars.

#### A14. Web backup and IDBFS
- **Edits made before `callMain`:**
  - Nothing refreshes the localStorage backup (`SexyAppBase` is not running), so an edit is lost if the tab dies before the IndexedDB sync finishes.
  - A user-dir file **deleted** before start is brought back by main's `RestoreUserDataBackup`: a missing file has mtime −1, so the backup's copy wins (`WebPersist.cpp` restore loop).
  - **Fix:** `WriteUserFileAtomic` on the web calls `BackupUserData` itself when no app is running. The menu and panel never delete user files; they write empty objects instead.
- **`importMod` over an applied mod.** It runs `rmTree` and rewrites an applied mod's folder in place while the game runs (`shell.html:553-576`). `ReadDiffValues` re-reads at every Load, and the VFS directory cache goes stale. Treat it as a queued remove plus add.
- **The pending-removal queue** must clear an entry when a mod with the same folder name is re-imported. Otherwise the next boot deletes the new import.
- **`Module.onModsChanged` re-entrancy.** It is called through `EM_JS` *inside* an edit. A handler that calls back into `byopop_mods_state` re-enters `ModManager` mid-operation. Post it asynchronously.
- **Other notes:**
  - A reload leaves fullscreen.
  - The autostart key should be named per game.

#### A15. Native relaunch
- **Foreground focus.** `SexyAppBase::Shutdown` hides the window at once (`SexyAppBase.cpp:1372-1373`), so by the time `Relaunch` runs this process no longer owns the foreground. The new window may open behind other windows.
  - Call `AllowSetForegroundWindow(ASFW_ANY)` in `RequestRestart` before `Shutdown`. This needs verifying on Windows.
- **`-seed=N` is a framework test hook** (`SexyAppBase.cpp:2849-2853`). Keeping it replays the same boards after a restart; drop it.
- **Test-run detection** should check `BEJ_TEST_DESKTOP` as well as `BYOPOP_TEST_RUN`, as `OpenURL` does (`:796`).
- **Path encoding on Windows.**
  - Windows file I/O is ANSI: `FindFirstFileA` (`VFS.cpp:84-85`) and `fopen` (`:275-280`) with UTF-8 paths from SDL. Non-ASCII user, exe or mods paths already fail today.
  - So the planned "relaunch from a non-ASCII folder" test will fail for an unrelated reason.
  - `WriteUserFileAtomic`'s `MoveFileExW` must use the same encoding as its own temporary-file write. Better: fix the platform layer to use wide APIs, or a UTF-8 activeCodePage manifest.
- **No rescue before start.** Natively there is no UI before the game starts. Offer a safe mode: `mLastShutdownWasGraceful` from `InProgress` (`SexyAppBase.cpp:1171-1175`) could offer starting with `-nomods` after a crash.
- **rundesk job wait.** rundesk also wraps whole bash scripts, so "end whatever is left in the job" must not kill helpers a script backgrounds on purpose.

#### A16. Framework UI
- **Built-in font filtering.** `DDInterface::PrepareTexture` forces linear filtering whenever `mRenderScale > 1` (unless pixel-perfect) or the image's `mScale > 1` (`DDInterface.cpp:592`). So "nearest filtering at render scale N" needs a nearest flag per image, or a glyph sheet generated at the render scale.
- **Characters outside Latin-1.** `UTF8ToLatin1` silently drops them (`SexyAppBase.cpp:70-127`); it does not turn them into `?` as the design says.
- **Focus theft.** `WinDMApp::KillDialog` gives the board focus when the dialog map empties (`WinDMApp.cpp:569-580`). For example, `URLOpenSucceeded` kills `DIALOG_OPENING_BROWSER`. The menu, which is not in the map, must take focus back in `Update`, not only its z-order.
- **Text settings on touch.** SDL2 on the web shows no on-screen keyboard, so text settings cannot be edited in-game on touch devices. "Everything works from a pointer" holds only through the HTML panel.

#### A17. Script commands and tests
- `LoadScript` cuts everything after `#` (`SexyAppBase.cpp:2895`), so `modmenu set ... "#ff8800"` breaks. Use another colour syntax, or quote-aware comment handling.
- `key` cannot hold a modifier: it sends key-down and key-up together. Shift+arrows, Alt+Up/Down and Shift+Tab are therefore untestable. Add `keydown`/`keyup <name>` commands.
- Mod ids with spaces (folder names) cannot be addressed by whitespace-split arguments.
- `games/bejeweled/tools/check_rng_use.py` is owned by the Bejeweled session. A check that covers `framework/` belongs in `tools/`.
- The examples case runs with the real `games/bejeweled/mods`. Adding `settings` to any of those mods (the design suggests `example-relaxed` might get one) makes the MODS button appear in oracle screens.
- The "MODS" gate should use the start-time `GetMods()`, which reads nothing from disk when Options opens. With `-nomods`, that list is empty.
- If any LiveSplit global hotkey is bound to F2, it also opens the menu. A configurable hotkey would help.
- Two mod folders with the same manifest `id` collide in `mods.json`, `mod-settings.json` and `SetEnabled`. Flag the duplicate as an error.

### B. Confirmed against the code
- **Mod management:**
  - There is no in-game mod UI, and nothing calls `SetEnabled`, `Move` or `SaveConfig`.
  - `SaveConfig` drops unknown keys, as does the shell's `saveModConfig` (`shell.html:547-552`).
  - `WriteStatus` syncs but never sets `mBackupDirty` (`ModManager.cpp:388-415` versus `SexyAppBase::SyncUserData` `:1231-1235`).
  - `Move` swaps across priorities, and the shell's ordering ignores `priority`.
- **Dialogs and focus:**
  - A `Dialog` with a NULL button image recolours the button text of every later dialog (`Dialog.cpp:84-92`); `EnsureFonts` is empty.
  - `DialogButtonPress`/`Depress` turn YES and NO into ids 2000+ and 3000+; `AddDialog` calls `AddBaseModal` then `ModalOpen`.
  - `AddBaseModal` takes focus from the board.
  - `WatchController` holds playback while any base modal is open (`WatchController.cpp:269`).
- **Input and scripts:**
  - The scripted-input filter is at `SexyAppBase.cpp:1724-1746`, and F-keys type no character.
  - Bejeweled pauses on any character; Big Money pauses on space only and counts pauses (`Board.cpp:618`, with a guard on `STATE_PLAYING`).
  - Big Money's modal hooks pause only the Action type.
  - The script `key` command goes only to the WidgetManager, so it cannot reach an F2 check placed in `HandleEvent`.
- **Web lifecycle:**
  - `callMain` runs once and the main loop never returns.
  - On exit: `MainLoopFinished`, then a forced backup, then `onGameExit` (`:2400-2408`).
  - The restore is idempotent through `FS.utime`, apart from the deletion case in A14.
  - The Web Lock reload race exists as the design describes.
- **Saves and scores:**
  - `Board::GetSaveGameFileName` is static and reads the current key, while `HighScoreWidget` keeps its own `mFileName`.
  - `NewGame` deletes the old board deferred, then calls `Load` (`WinDMApp.cpp:233-240`).
  - Bejeweled saves in `WinDMApp::Shutdown`; Big Money saves in `~WinBMApp`, or in `ShutdownHook` on the web.
- **Native and build:**
  - Native has no single-instance check.
  - `-msse2 -mfpmath=sse` is PUBLIC on `sexyframework`, so `platform/` code is covered.
  - `MTAutoDisallowRand` is counted, so nested scopes are safe.
- **Line references:** Big Money's refs should be re-pinned to 9538925.
