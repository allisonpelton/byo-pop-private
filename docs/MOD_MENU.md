# Mod settings and the mod menu

This is the guide for mod authors. It covers how a mod offers values the player can set in the mod menu, how
those values reach the mod's data, when a change takes effect, and when it gives the player separate high
scores. The design and its reasons are in [MOD_MENU_DESIGN.md](MOD_MENU_DESIGN.md). The code is
`platform/ModSettings.*` (the settings), `platform/ModManager.*` (the model and the files) and
`framework/ModMenu.*` (the menu). Every game built on the shared framework has the same menu and reads the
same `settings` block.

## The mod menu for players

- **Opening it.** Press **F2** in any game, natively or in the browser, on the title screen or during play.
  On the web, the tool bar's **Mods** button also opens it while the game runs. F2 works without Ctrl, Shift
  or Alt, and holding it down does not repeat. In Big Money, Options also has a **MODS** button while an
  installed mod, on or off, has settings. Bejeweled's, which its own menu integration adds, shows while an
  enabled mod has settings. The menu looks the same in every game.
- **Closing it.** Press F2 again, press Esc on the list, or use **Close**.
- **Pausing.** While the menu is open the game is paused, as it is while Options is open.
- **The list page** shows every installed mod in load order:
  - an on/off box;
  - ▲ and ▼ to move the mod (only among mods of the same `priority`);
  - **Settings**, for a mod with settings;
  - badges: SCORES (the mod gives separate high scores), FEATURES (it turns on port features), and RESTART
    or NEXT GAME while a change of it waits.
- **A mod's page** has one row per setting. A setting with a SCORES mark splits the high scores when it is not
  at its default. **Reset to defaults** asks first.
- **Saving.** Every change is saved at once. The footer says what still waits:
  - "Restart to apply: ..." for changes that need a restart;
  - "Applies to your next game: ..." for values the game reads at its next new game.
- **Restart game.** This button restarts the game; the game in progress is saved as on Quit. In a game that
  reports a game in progress (`SexyAppBase::HasGameInProgress`: Big Money does, while its game can be saved;
  Bejeweled's own menu integration adds it), it asks first and says where that game goes: it can be continued
  after the restart, or, when the restart changes the score key, it stays saved with the current mods and
  continues when the player switches back to them. Elsewhere it restarts at once and says nothing, and a game
  saved under the old key is offered again only once the mods are switched back. Nothing restarts by itself:
  changes left pending apply at the next start.
- **Turning off a broken mod.** Start without mods (`-nomods` natively, **Play without mods** on the web),
  then turn the mod off in the menu and use Restart game. The menu says "Mods are off for this run". The
  restart starts with mods again.
- **Adding and removing mods.**
  - Natively, mods are folders in the mods folder, and the menu does not add or remove them.
  - On the web, the page's **Mods** panel adds mods (a folder or a .zip) and removes them. The menu's
    **Add or remove mods** button opens that panel during play.
  - A mod the running game uses is removed, or replaced by a new version, when the game next starts.

### Keys

| Key | List page | A mod's page | Editing text |
|---|---|---|---|
| Up, Down | Row | Setting | — |
| Left, Right | Next/previous control | Change the value (Shift: 10 steps) | Move the caret |
| Home, End, PgUp, PgDn | First, last, a page | Same | Caret to start/end |
| Space | Mod on/off | Switch a bool, press a button | Types a space |
| Enter | Open the mod's page (a mod without settings: on/off) | Press; start or finish editing text | Finish |
| Alt+Up, Alt+Down | Move the mod | — | — |
| Delete | — | Setting back to its default | Delete forward |
| Tab, Shift+Tab | Next/previous control | Same | Finish, then next |
| Esc | Close the menu | Back to the list | Cancel the edit |
| F2 | Close the menu | Close the menu | Finish, then close |

The mouse works on everything: click, the wheel, and dragging sliders and the list. On touch screens the
browser turns taps into clicks. Text settings need a keyboard: without one they cannot be edited.

### Files in the user dir

| File | Contents | Written |
|---|---|---|
| `mods.json` | `{"order": [...], "disabled": [...], "enabled": [...]}`: the load order, the mods turned off, and the mods turned on whose `mod.json` says `"defaultEnabled": false`. Only choices that differ from a mod's default are listed, and `"disabled"` wins if both lists name a mod. Mods not listed are appended, and on unless they say `"defaultEnabled": false`. | When the player turns a mod on or off or moves it, and when the web panel removes a mod (its id leaves all three lists). |
| `mod-settings.json` | `{"v": 1, "mods": {"<mod id>": {"<setting id>": value}}, "next": {...}}`. Only values that differ from their defaults are stored. `"next"` holds values that wait for the next fresh game. | When the player changes a value. |
| `mods-status.json` | What the enabled mods' rules came to (Bejeweled) | At start, when an enabled mod has `rules` or `features` |

`mods.json` and `mod-settings.json` may be edited by hand while the game is closed. When the menu opens it
reads them again. Every change re-reads the file, changes one thing and keeps everything else, including keys
this version does not know. Values of a mod that is not installed are kept.

A file that cannot be read counts as empty, and the first change moves it aside (`mods.bad.json`,
`mod-settings.bad.json`); for `mod-settings.json` the menu also says so. A UTF-8 byte order mark at the start
(as Notepad and PowerShell write one) is fine. A `mod-settings.json` with a `"v"` above 1 was made by a newer
version: the menu shows its values but does not change them. In a game that applies no values at the next game,
values found under `"next"` count as set: the next start applies them.

## Declaring settings

A mod's `mod.json` gains a `settings` array. Each entry is one setting, or a section heading:

```jsonc
{
  "id": "example-settings", "name": "Settings Example", "version": "1.0",
  "settings": [
    { "type": "section", "label": "Play" },
    { "id": "hintPenalty", "label": "Hint cost", "type": "int", "default": 50, "min": 0, "max": 100, "step": 5,
      "help": "Points a hint costs, times the level's point multiplier." },
    { "id": "timeWarning", "label": "Timed: warn below", "type": "float", "default": 0.15,
      "min": 0.05, "max": 0.5, "step": 0.05 },
    { "type": "section", "label": "Look" },
    { "id": "excellent", "label": "Big move message", "type": "text", "default": "EXCELLENT!",
      "maxLength": 16, "charset": "upper" }
  ],
  "tunables": {
    "scoring": { "hintPenalty": { "$setting": "hintPenalty" } },
    "timing": { "warningFraction": { "$setting": "timeWarning" } }
  },
  "strings": { "EXCELLENT": { "$setting": "excellent" } }
}
```

The whole example is `games/bejeweled/mods/example-settings/mod.json`. Another example, which uses every
type and placeholder form, is `tools/fixtures/mods/settings-demo/mod.json`. It is a test fixture: the files it
names do not exist.

### Keys of a setting

| Key | Types | Needed | Meaning |
|---|---|---|---|
| `id` | all but section | yes | A letter, then up to 31 letters, digits or `_`; unique in the mod. It keys the stored value: do not reuse an id for a different meaning. |
| `type` | | yes | `bool`, `int`, `float`, `choice`, `text`, `color`, or `section` (a heading without a value) |
| `label` | all | yes | UTF-8, at most 40 characters (longer is cut, with a warning) |
| `default` | all but section | yes | Valid for the type, inside `min`..`max` and on a step |
| `help` | all | no | At most 200 characters, shown at the bottom of the menu while the row has the focus |
| `min`, `max`, `step` | int, float | float: yes; int: no (−1,000,000..1,000,000, step 1) | Inclusive. Ints are whole numbers within 32 bits. A float allows at most 100,000 steps. |
| `unit` | int, float | no | A short suffix such as `%` or `x`, at most 8 characters |
| `decimals` | float | no | Digits shown, 0..9 (default: as many as `step` has) |
| `choices` | choice | yes | 2..32 entries `{"value": v, "label": "..."}` |
| `maxLength` | text | no (24) | 1..64 characters |
| `charset` | text | no (`any`) | `any` (printable Latin-1), `upper` (letters become capitals), `digits`, `name` (letters, digits, space, `-`, `_`) |
| `alpha` | color | no (false) | The value is `#rrggbbaa` instead of `#rrggbb` |
| `apply` | all | no | `live`, `nextGame` or `restart`: when you want the value applied. It can only make that later than the data allows ("When a value applies"). |
| `affectsScores` | all | no | `true`: the value splits the high scores even where its use alone would not, e.g. a data file the game reads for play |
| `enabledIf` | all | no | `{"setting": "<an earlier bool or choice>", "equals": v}`, or `"notEquals"`. While false, the row is greyed out and the setting counts as its default. The condition applies no earlier than the setting; it splits the scores only through its own data uses. |

A mod has at most 64 settings.

### Types

| Type | Value | In the menu |
|---|---|---|
| `bool` | `true` / `false` | A box with On/Off |
| `int` | A whole number | − value +, and a slider up to 200 steps |
| `float` | A number on `min + k·step` | A slider and − value + |
| `choice` | One of the choices' `value`s | ‹ label ›, which goes round |
| `text` | A string (Latin-1 characters only) | A text field |
| `color` | `#rrggbb` or `#rrggbbaa`, lower case | A swatch, a hex field and ‹ › through 16 colours |

- A choice's `value` must be an id (letters, digits, `_`, `.`, `-`, at most 32), a whole number or `true`/`false`,
  and the values must be different.
- Floats are exact: the value of `"step": 0.05` at 3 steps from 0.05 is 0.2, the same number as `0.2` written in
  JSON. A float can have at most 9 decimals.

### What happens to wrong declarations

| Problem | Result |
|---|---|
| `settings` is not an array | The block is ignored, with an error; every placeholder then fails |
| A setting with a bad id, a used id, an unknown type, no label, a bad default or range, bad choices | That setting is left out, with an error |
| More than 64 settings | The rest are left out |
| `enabledIf` that does not name an earlier bool or choice and one of its values | The condition is ignored, with a warning |
| `apply` earlier than the data allows | The later level is used, with a warning |

Errors and warnings show on the mod's row and page in the menu, in the web panel and in `-dumpmods`.

## Placeholders: putting values into the data

A value reaches the mod's data through a placeholder: an object whose only keys are `$setting`, and optionally
`map` or `as`. The placeholder is replaced by the value before the game reads anything. A rule set by a
setting is therefore the same as a rule written directly in the file.

| Form | Becomes |
|---|---|
| `{"$setting": "minRun"}` | The value |
| `{"$setting": "shape", "map": {"full": ..., "diamond": ..., "*": ...}}` | The `map` entry for the value, or `"*"` when there is none. An entry may contain placeholders itself, up to 8 deep. |
| `{"$setting": "speed", "as": "int"}` | An int or float, rounded half away from zero |
| `{"$setting": "x", "as": "string"}` | The value as the menu shows it (a float with its decimals) |
| `{"$setting": "tint", "as": "rgb"}` | A colour as `[r, g, b]`; `"rgba"` adds alpha; `"argb"` gives one signed 32-bit number |

- **Map keys.** A `map` works on bool (`"true"`, `"false"`), int (the number, e.g. `"3"`) and choice settings
  (the value, e.g. `"diamond"`). A key that can never match is an error.
- **`$omit`.** A map entry `{"$omit": true}` removes the member or array element the placeholder stands for. Use
  it for "this setting adds nothing at this value". Do not use `null` for that.
- **`null`** keeps its meaning in the data:
  - under `rules` it resets the rule to the game's default, even if another mod set it;
  - under `features` it turns the feature off;
  - as the value of a member of `files`, `images` or `strings`, or of an element of `imagePatches` or
    `imageOps`, it removes that member or element.

```jsonc
"rules": {
  "match": { "minRun": { "$setting": "minRun" } },
  "undo": { "$setting": "undo",
            "map": { "true": { "enabled": true, "depth": { "$setting": "undoDepth" } }, "false": { "$omit": true } } },
  "board": { "$setting": "shape",
             "map": { "full": { "$omit": true }, "diamond": { "mask": [ "...##...", "..." ] } } }
},
"features": { "$setting": "replays", "map": { "true": { "replays": true }, "false": { "$omit": true } } },
"files": { "music/1984-network.s3m": { "$setting": "music", "map": { "original": null, "remix": "extra/remix.ogg" } } }
```

### Where placeholders may stand

- Anywhere inside `tunables`, `rules` and `features`, including the JSON files that `"tunables": "file.json"`
  and `"rules": "file.json"` name.
- As the value of a member of `files`, `images` and `strings` (not as the whole key).
- As `imageScale`, and inside the elements of `imagePatches` and `imageOps`.

Anywhere else (`id`, `name`, `version`, `priority`, `affectsScores`, ...) a placeholder is an error.

Some values are refused in some places:
- **File names.** A text or colour value cannot name a file (in `files`, `images`, `imagePatches`, `imageOps`,
  or a `strings` list of files). Use a choice or a bool with a `map` to file names you list. A map entry there
  may not name a file outside the mod's folder (`../`): that is an error. (The mod's own file names, outside
  placeholders, are taken as they always were.)
- **Texts with `%`.** A text setting used in `strings` cannot contain `%`: its default may not have one, the
  menu does not type one, and one in a stored value becomes `?`. The game passes some of its texts to `printf`
  (those with `%` codes, such as Bejeweled's `LEVEL`): a mod text for such a key must have the same `%` codes,
  or the game's text is used (the log says so). Every other text is used as the mod wrote it.
- **Numbers and bools in `strings`** become text: a setting's value as the menu shows it, and a number that
  `"as": "int"` or a map entry gives as JSON writes it. A map entry under `strings` that is an object or a list
  is an error.

When a placeholder is wrong (an unknown setting, a map without the value and without `"*"`, an `as` that does
not fit the type, a placeholder where none may stand, nesting deeper than 8), the whole top-level key that holds
it (`rules`, `files`, ...) is left out for that mod, and the menu shows the error.

## When a value applies

The game decides from where the value is used. You do not have to tell it.

| Where the value is used | When a change applies |
|---|---|
| Nowhere in the data: the game's code asks for it (`ModManager::GetLiveValue`) | At once (`live`) |
| `rules`, `tunables`, `features` | At the next fresh game, in a game that reads these at every new game (NEXT GAME); otherwise at a restart |
| `files`, `images`, `imageScale`, `imagePatches`, `imageOps`, `strings` | At a restart: art, sounds, music, fonts and texts are loaded once |

- **The latest use wins.** A value used in two places applies at the later of the two.
- **`apply`** can make a value apply later than its data needs, never earlier.
- **Turning a mod on or off, and the load order,** always need a restart.
- **Next-game values wait for a fresh game.** A NEXT GAME value is held while a game is in progress or saved,
  so a saved game is always continued with the values it was started with.
- **Some rule values always need a restart,** such as those that need art made at load (Bejeweled's gem
  count). The game says which data paths it reads at a new game; everything else waits for a restart.

The menu marks each pending change with RESTART or NEXT GAME, so the player always sees which.

## High scores and saves

High scores and the saved game are kept per score key. The key comes from the enabled mods that change play,
so that changed play never mixes with original scores.

- **A mod counts for the key** when it has `rules` or `tunables`, says `"affectsScores": true`, or replaces a
  file the game plays with (Bejeweled: `data/diff.txt`).
- **A setting counts** (SCORES in the menu) when it is used under `rules` or `tunables`, in the `files` remap of
  such a gameplay file, or says `"affectsScores": true`.
- **The key entry.** At their defaults, a mod's settings leave its entry as before (`id@version`), so adding a
  settings block to an existing mod keeps its players' scores and saved game. A score-affecting value away from
  its default adds itself to the entry, e.g. `example-relaxed@1.0?hintPenalty=25`, and every such combination
  gets its own high scores and saved game.
- **The order of score-affecting mods counts.** The key is taken over their entries in load order, so moving
  one of them past another gives a new key (when two mods set the same rule, the higher one wins, so the play
  can differ). Moving a mod past a cosmetic one changes nothing.
- **Cosmetic values never split scores.** These are values in `strings`, `images`, `files` other than gameplay
  files, and `features`.
- **A value that counts for the scores is never live.** It applies at the next game at the earliest.
- **Change `version` when you change the default of a SCORES setting.** At its defaults a mod's entry is
  `id@version` whatever the defaults are, so without a new version the high scores and a saved game played
  with the old default would carry on under the new one.

When the player changes something that moves the key a restart would apply while a game is in progress, the
menu says that the game stays saved with the current mods and continues when they switch back (in a game that
reports a game in progress, as for Restart game). A value held for the next fresh game moves no game in
progress, so it brings no such note.

## Checking a mod

- **`-dumpmods`** (native) prints the mod menu's model as JSON and exits: every mod with its state, errors,
  warnings, settings, values, levels, what waits, and the score keys: now (`scoreKey`), after a restart
  (`restartScoreKey`) and at the next fresh game, with the values held for it (`nextScoreKey`).
- **Script commands** (`-script=<file>`; syntax in `framework/SexyAppBase.h`) drive the menu in tests:
  `modmenu open`, `modmenu page <mod>`, `modmenu set <mod> <setting> <JSON value>`, `modmenu enable <mod> 0|1`,
  `modmenu move <mod> -1|1`, `modmenu reset <mod>`, `modmenu restart [confirm]` and `modmenu dump <file>`. A test
  run does not start the game again after a restart; it logs `restart: <arguments>`.
- `tools/modsettingstest` checks the settings rules, and `tools/modmenutest` checks the menu, in both games.

## For game code

A game uses a value that no data holds with `Platform::ModManager::Get().GetLiveValue(modId, settingId)`, which
returns the default until the player changes it. Live values apply at once. A game that re-reads its rules at
every new game declares which data paths it reads (`ModManager::SetNextGameFilter`) and commits waiting values
as it starts a fresh game (`CommitNextGameValues`). The restart hooks (`CanRestartNow`, `HasGameInProgress`,
`GetRestartArgs`, `GetModMessages`) are virtuals of `SexyAppBase` with defaults. These are covered in
[MOD_MENU_DESIGN.md](MOD_MENU_DESIGN.md) 6.
