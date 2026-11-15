# visrun: an original in 3D on the visible desktop

`tools/visrun.py` runs one PopCap original on the user's visible desktop, for what neither Windows Sandbox nor rundesk
gives it: its Direct3D mode (`docs/ORIGRUN.md`, "Hardware acceleration"). It replaces the five game-local drivers
(Astro Pop, Bejeweled 2, Bookworm Adventures, Rocket Mania, Typer Shark) with one that reads a small per-game config and a
step file. Run it with `python -I`.

## When it may run

Only between 22:00 and 08:00 local time (CLAUDE.md, "Overnight"), or, when the user opens the desktop for a stretch of
the day, until the time they gave, written on that run's command line as `--user-window-until HH:MM` (at most 14 hours
ahead, never a default, never in a config). A run ends 2 minutes before the deadline. `--dry-run`, `--selftest` and
`--help` run at any hour. Before starting it checks `tools/gamecount.py --limit 6`, takes `build/visrun/visrun.lock` (one
visrun on the machine), and waits until no other game window (class `MainWindow` or `SDL_app`) has been on the desktop
for `--quiet` seconds (default 60).

## What it does

1. **Stages through origrun.** `origrun --game <name> --run visrun-<name> --copy-only` puts the install into
   `build/origrun/visrun-<name>/game` (`stage_root` in the config puts it on a short path, which Bookworm Adventures
   needs). A config with `source_exe` names a ready copy instead. visrun then makes a fresh working copy under
   `build/visrun/<name>/game` without dgVoodoo's files (the 3D mode needs no wrapper on the desktop), saves, logs or an old
   stand-in, takes the clean exe where an earlier origrun run patched it (`*.origrun-unpatched`) or places the exe named
   by `carve_exe` (a wrapped game's real exe, `tools/extract_game_exe.py`), applies the patch list (origrun's format,
   `{seed}` from `--seed`), and places `build/origrun-bin/dsound.dll`, origrun's stand-in. origrun itself only launches under
   rundesk, so the launch is visrun's own: it sets origrun's environment (`ORIGRUN_DIR`, `ORIGRUN_SOUND=stub`,
   `ORIGRUN_MSGBOX=hook`, `ORIGRUN_REGMAP=popcap`, `ORIGRUN_CURSOR=scripted`, and for `clock = "virtual"` the
   `ORIGRUN_CLOCK_*` block, with visrun as the clock's host). The stand-in's network block and launch guard come with it.
2. **Silent.** The run stops before any input unless the stand-in's `logs/sound.log` says the process's default audio
   session is muted (`require_muted`, default true).
3. **Starts without the focus** (`SW_SHOWNOACTIVATE`, below-normal priority, in a job object that ends the game if
   visrun dies). The window is kept topmost and never activated; it is told it has the focus by the messages Windows sends
   (checked before each step).
4. **Input is posted to the game's window only** (`PostMessage`): never `SetCursorPos`, `SendInput` or the user's keyboard.
   With `cursor = "scripted"` the stand-in's `GetCursorPos` reads the point visrun writes into origrun's shared block, so the
   user's real cursor does not matter to the game.
5. **Shots are `PrintWindow` of the game's own window**, never the screen, and refused while another visible window
   overlaps the game's (three refusals in a row end the run, exit 4). A shot waits until the game has drawn its update
   (the config's `drawn` conditions).
6. **Puts everything back.** The game's registry key (`reg_key`) is exported, deleted (the cached Direct3D test with it) and
   set to `reg_values` and the 3D switch; each folder of `save_dirs` is copied away, emptied, and filled from `save_seed`. At
   the end (or by `visrun.py CONFIG --restore` after an interrupted run, which `build/visrun/<name>/backup` marks) all are
   put back, and a key or folder that did not exist is removed again. The game is asked to close, and only if it will not,
   ended by its own process handle. visrun stops no other process, and the working copy is deleted (`--keep-copy` keeps it).

## The config (TOML)

| Key | Meaning |
|---|---|
| `name` | The run's tag, under `build/visrun/<name>/`. |
| `game` or `source_exe` | A `games.tsv` name that origrun stages, or a ready copy's exe. |
| `exe`, `carve_exe` | The exe's leaf name in the copy, and a carved exe to place as it. |
| `patches` | A patch list (origrun's format), applied with `--seed`. |
| `title`, `client` | A text the window's title holds, and the least client size of the game window (`[w, h]`). |
| `pos` | Optional `[x, y]` to put the window at. |
| `counter` | The update counter, origrun's expression: `0xA`, `[0xA]+0xB`, `[[0xA]+0xB]+0xC`. |
| `clock` | `"real"` (the process is suspended at each step; the update a step lands at is logged) or `"virtual"` (origrun's clock: update for update). `--clock` overrides it. |
| `update_ms` | One update's length, for `clip.json`. |
| `drawn` | Conditions that say the last update is drawn, e.g. `["[0xA]+0x45a:1 == 0"]` (a size suffix `:1` or `:4`, an operator `== != < <= > >= in`). |
| `flag3d` | A condition on the game's 3D flag, logged at the first step and at each change. |
| `reg_key`, `reg_values`, `reg_3d_value`, `reg_preset`, `reg_map` | The key under HKCU, values to set (integers are DWORDs), the value `--3d on/off` sets (`Is3D`), a `.reg` preset (HKLM PopCap read as HKCU), and `"popcap"` for the older games that keep their settings under HKLM. |
| `save_dirs`, `save_seed` | Folders to back up, empty and restore; `[[src, dst], ...]` files to copy in. |
| `values` | Expressions read and written to `steps.tsv` at every step. |
| `watch` | Up to four expressions for the virtual clock's `watch N: a -> b at update U` log. |
| `cues` | Named conditions for `waitcue` and `gate`. |
| `forbid` | `[[forbid]]` tables with `name` and `rect = [x, y, w, h]`: regions of the client area no click may fall in. |
| `cursor`, `require_muted`, `stage_root`, `work_root` | `"scripted"` or `"real"`; true or false; a short staging path; where the working copy goes. |

## The step file

origrun's script dialect, so a script made for origrun runs here, plus the extras below. One command per line, `#` starts a
comment. Coordinates are the game's client pixels.

- `at N <cmd>`, `at +N <cmd>` (counting from the last `mark`), `t <seconds> <cmd>` (since the window came), or a bare
  `<cmd>` (at once). `<cmd>` is `shot <name>`, `burst <name> <count> update <k>`, `peek`, `move x y`, `click x y`, `rclick`,
  `dclick`, `down`, `up`, `rdown`, `rup`, `key <vk>`, `text <chars>`, `focus lose|regain`.
- `waitcue <cue> [seconds]`, `waitfor <expr> <op> <value> [ms]`, `mark`, `wait <ms>` (real time; ignored on the virtual
  clock), `quit`. `waitshot` is accepted and ignored (visrun always waits for the window).
- `gate <cue> <wanted> [fade]`: wait for the cue (a title's "loaded" flag), then for the update `wanted` or the loaded update
  plus `fade`, and move every later `at` by the updates this took (Bejeweled 2's `gate`).
- `forbid <name> x y w h`: a region where no press or release may fall. The whole file is checked before anything starts and
  a click, `down` or `up` inside a region refuses the run (a move may hover). Put every link, Buy and More Games button of a
  screen in a config or a `forbid` line. origrun's `hw*`, `record` and `count` have no form here and stop the parse.

## Results

`<out>/visrun.log` (the window, the mute check, 3D flags seen, refusals), `steps.tsv` (wanted update, landed update, command,
arguments, seconds since the window, `values`; a shot's arguments are its name and size), `<name>.png` shots (the client
area; `--shape sandbox` puts it at (3, 26) of a canvas 6 and 29 pixels larger, as origrun's sandbox shots are), `burst`
clips with `clip.json`, `watch.log`. On the real clock the update a step lands at is only near the one asked for; replay
the landed updates in the port. Exit 0 done, 3 the run stopped (no window, not muted, a cue or update never came), 4 shots
refused three times.

## Checked without a desktop

`python -I tools/visrun.py --selftest` (the time window, expressions, window picking, overlap, forbidden regions, the layout
of origrun's cursor block) and `--dry-run` (a step file's plan; also checks the forbidden regions). The rest needs a first
run on the desktop.
