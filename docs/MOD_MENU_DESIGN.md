# Mod menu: design

The mod menu is built: `framework/ModMenu.*`, `platform/ModSettings.*`, `platform/ModManager.*` and `web/modpanel.js`.
The guide for mod authors is [MOD_MENU.md](MOD_MENU.md). This document is the design the code was built from, and the
code cites its section numbers. It has three parts:
- **The user's decisions** (just below), taken after the design and its review. They win over the design.
- **As built**: where the shared code differs from the design.
- **The design** (sections 1-8), as written before anything was built. Its line references (`file.cpp:NNN`) are to the
  code of 2026-10-19 and are not kept up to date.

## The user's decisions (2026-10-19)

1. **Hotkey.** F2, as designed. Also the web page's Mods button, and a MODS or Options button where a game has
   a free slot.
2. **Look.** One shared look in every game: the design's default look, drawn with rectangles and the built-in
   font. There are no per-game skins, so 5.3's `ModMenuSkin` and the skin rows of 8.2-8.4 are dropped.
3. **Bejeweled.** Gameplay and feature values apply at the next fresh game without a restart (6.2). They are
   held back while a saved game exists. Art, enabling, disabling and the order still need a restart.
4. **The web panel** only adds and removes mods: import, remove, and the rescue when a mod stops the game from
   starting. Turning mods on and off, the order and the settings are in the in-game menu. This replaces the
   design's rebuilt full panel (4.5, 5.1).

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
  new game, so keys that need art made at load stay restart-level.
- **Other game hooks.** `SetGameplayFiles` (default `data/diff.txt`) and `SetDataCheck`, whose
  messages appear as warnings in the state JSON.
- **`$omit`.** `{"$omit": true}` removes a member or element. `null` keeps its meaning in the data.
- **`as: "string"`** gives the text the menu shows (a float with its decimals), not the canonical key text.
- **Ranges.** Ints are limited to int32. The 100,000-step limit applies to floats only. Floats are snapped in
  scaled integers, so a value equals its JSON literal.
- **Choice values** must be ids (`[A-Za-z0-9_.-]{1,32}`), int32 whole numbers or bools, unique by canonical text
 . The canonical text of a text value also escapes `;` and non-printable bytes.
- **Paths and strings**:
  - text and colour values cannot be placed directly in `files`, `images`, `imagePatches`, `imageOps` or a
    `strings` list of files; they need a mapped choice, bool or int;
  - a map entry in those places may not name a file outside the mod folder (`../`): an error. A mod's own
    literal paths are not checked, as before (nothing changes for a mod without settings);
  - a text setting used under `strings` cannot contain `%`;
  - `LoadProperties` checks the keys whose game text has printf conversions (Bejeweled's `LEVEL`, Big Money's
    two rank texts): a mod text for one with other conversions is replaced by the game's text, with a log line.
    Texts the game only draws, and keys a mod adds, are taken as they are;
  - under `strings`, a number or bool from `as` or a map entry becomes its JSON text, and a map entry that is an
    object or a list is an error.
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
  it backs up straight away.
- **The state JSON** adds `dir`, `hasSettings`, `settingsFile`, and per setting `enabled`, `pending` and
  `noPercent`.

### M2, M3: restart and the menu
- **No skins** (decision 2): the look lives in `framework/ModMenu.cpp`.
- **Keys and focus.** Esc, Enter, Tab, Backspace and Space act on the character they type, so a key that closes
  the menu never reaches the board. While the key that closed it is held, its repeats (and the text
  they type) are dropped in `HandleEvent`; its release goes on. The menu closes in `KillDialog`'s order and takes
  the focus back if the game moves it.
- **Focus model.** Tab steps through every focusable control. On a mod page each row has one keyboard control;
  its −/+ buttons, slider, arrows, text field and a bool's checkbox are mouse targets. Only a click on the
  checkbox switches a bool, or a mod on the list: a click on a setting's label or a mod's row only focuses
  it, and the footer shows its help or description (the user, 2026-10-25: label and row clicks switched too
  easily).
- **Restart timing.** The menu's Restart takes effect at its next Update, outside its no-random scope.
- **rundesk** runs its child in a job object and returns when the whole job has ended. It kills nothing
 .
- **Script commands.** `modmenu restart confirm` was added. `modmenu page` opens the menu. There are also
  `keydown`/`keyup`, and `#` inside `"..."` is no longer a comment.
- **The restart question** needs the game's `HasGameInProgress`. Bejeweled and Big Money override it (8.2, 8.3); in a
  game that does not, Restart restarts at once. The game in progress is still saved as on Quit.
- **Not done.** A relaunch from a folder with non-ASCII characters: the file layer's ANSI calls fail there first
 .

### M4: the web pages
- **The C API** (4.5) is `byopop_userdata_restore`, `byopop_mods_open`, `byopop_mods_state`, `byopop_mods_edit`
  and `byopop_mods_rescan` in `platform/WebApi.cpp`. `byopop_menu` and `byopop_restart` are in
  `framework/ModMenu.cpp`, since they need the running game. All are listed in `-sEXPORTED_FUNCTIONS` by
  `cmake/ByopopWeb.cmake`. Changes to the design's API:
  - `byopop_menu(on)` takes 1, 0 or −1 (toggle). It returns 1 or 0 (open or not), or −1 when no game runs.
  - `byopop_restart(confirmed)` returns `{"result": "started"|"confirm"|"blocked"|"none", "message"}`, so the
    page can ask the game's question.
  - `Module.onModsChanged` is posted with `setTimeout`, never called inside an edit, and several changes make
    one call.
- **`web/modpanel.js`** is appended to every game's JS with `--post-js` and leaves `Module.byopop`. It contains:
  - the Mods panel;
  - `Session`, for restarts, autostart and the rescue;
  - the file and zip helpers the pages use for game files too.

  The panel reads everything through the C API. The pages lost their copies of the mod scan, the order and the
  gameplay tags.
- **The panel** (decision 4):
  - It lists the installed mods with their state, tags and errors, and has **Add mod folder**,
    **Add mod .zip** and **Remove**. One upload may hold many mods (a whole mods folder): every folder in it that
    holds a `mod.json` or one of the game's folders is a mod, the outer one where they nest (`importMods`).
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
- **Adding over a mod in use** stages the new version in `<persist>/mods-incoming/<folder>`. It
  replaces the folder at the next boot. Adding a mod also cancels a queued removal of its folder.
- **Opening the menu and the panel.** The tool bar's **Mods** button opens the in-game menu while the game runs,
  and the panel before that. This replaces the design's "panel and menu together" (5.1). The menu's list page
  has **Add or remove mods** on the web, which opens the panel over it (`Module.onModPanel`).
- **Input around the panel**:
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
- **Not done.** Native has no rescue before start except `-nomods`.

---

This design covers a shared in-game mod menu for every game, on Windows and on the web. Mods can offer settings, the player can turn mods off, and the menu can restart the game. It answers the user's request of 2026-10-19: "Add support for a mod menu to the web ports and Windows. Mods can offer configurable values to the menu and be disabled from there. The mod menu could also restart the game".
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
| [x] HD Gems 1.0                                [^][v] [About]    |
|     by you · Sharper gem sprites                                 |
| [x] Settings Demo 1.0    SCORES  FEATURES      [^][v] [Settings] |
|     ! rules: match.minRun 2 needs 5 gem colours                  |
| [ ] Fake Money! Deluxe 1.0   RESTART           [^][v] [About]    |
|     ...                                                     (#)  |
| ! the game's messages (the message slot, two lines)              |
|------------------------------------------------------------------|
| Restart to apply: HD Gems turned off.     [Restart game] [Close] |
+------------------------------------------------------------------+
```

- **Each row** (44 px tall) has:
  - a checkbox, the only part a click switches (the box and 4 px around it); a click elsewhere on the row
    focuses it, and the footer shows its description;
  - the name and version;
  - badges:
    - SCORES: separate high scores;
    - FEATURES;
    - ERROR;
    - RESTART or NEXT GAME, while the row has a pending change.
  - a second line: author · description, or the first message;
  - ▲ / ▼, disabled across a priority boundary;
  - "Settings", or "About" for a mod without settings; both open the mod's page, as Enter does.

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

**Mod page.** Every mod has one, with its whole description (the page scrolls); a mod without settings says
"This mod has no settings." under it.

```
| < Back    SETTINGS DEMO 1.0                              by ...  |
| Description, wrapped, all of it                                  |
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
- **The footer's help** is the focused row's: a setting's help, or on the list a mod's description or first
  message. It takes both lines while nothing is pending, and one line above the pending text otherwise.

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

  Hovering highlights controls, and the framework's hand cursor shows over what a click changes (not over a
  mod's row or a setting's label, which a click only focuses).
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
| `-nomods` | no | The restart is how the menu applies mods |
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

A shared `web/modpanel.js` is a step towards one web shell that every game shares.

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
- **As built (M5, 4facce2, merged at e679312):**
  - Options has the MODS button in the register slot (`mX+30+bw`, `mY+166`) while an enabled mod has settings. Extras
    (stage 13) is not built yet, so the EXTRAS case waits for it.
  - The next-game filter is `GameConfig::IsNextGamePath` (`GameConfigMods.cpp`). It covers every path under `rules`,
    `tunables` and `features` except `features.livesplit`, because the LiveSplit observer's state outlives a board.
    Those values commit at `NewGame`, and an untouched board is made again with them (modmenutest's Bejeweled
    nextgame scripts).
  - A mod's settings page shows the game's `GetModMessages`: the rules errors, dropped mods and warnings.
  - `bejeweled_verify` resolves `$setting` placeholders at their defaults (`ModSettings` `Analyze` and `Resolve` in
    `Verifier.cpp`) and links `platform/ModSettings.cpp` and `platform/Text.cpp`.
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
- **Web (done by the coordinator, with the shared page):** the game links through `byopop_web_game`
  (`cmake/ByopopWeb.cmake`), and its page is every game's `web/shell.html` with `games/bookworm/web/page.js`. So it
  has `modpanel.js`, the C API, restarts and autostart, and its own Mods panel is gone.
- **Options:** a MODS button only if the game has a free slot, and only when a mod has settings.
- **Scores:**
  - Mods that change the word list or other play data must count for the score key. Their settings use `"affectsScores": true`, or BookWorm marks those data paths the way Bejeweled marks `diff.txt`.
  - Its save and high-score files are scoped with the key.
- All values are restart-level.
- The menu needs only `SexyAppBase`, `WidgetManager` and `Graphics`, which the 2004 compatibility layer shares.
