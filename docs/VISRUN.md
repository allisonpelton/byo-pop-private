# visrun: an original in 3D on the visible desktop

`tools/visrun.py` runs a PopCap original on the user's visible desktop, in a place of its own on the right half of the screen
(up to four runs at once, "The places" below), for what neither Windows Sandbox nor rundesk gives it: its Direct3D mode (`docs/ORIGRUN.md`, "Hardware acceleration"). It replaces the five game-local drivers
(Astro Pop, Bejeweled 2, Bookworm Adventures, Rocket Mania, Typer Shark) with one that reads a small per-game config and a
step file. Run it with `python -I`.

## When it may run

A run may start at any hour. The user lifted the time window on 2026-11-15: "Get rid of the time restrictions on ... running on my
visible desktop. Do what you want forever now". A run still has a deadline, as a safety stop for one that hangs: 14 hours
after it starts, and it ends 2 minutes before that. `--user-window-until HH:MM` on the command line sets an earlier stop
(the next HH:MM, at most 14 hours ahead; never a default, never in a config). `--dry-run`, `--selftest` and
`--help`, `--places` run at any hour. Before starting it checks `tools/gamecount.py --limit` (again just before the launch,
which can come minutes later), takes a place and its game lock (below), and waits until no other window has been on its
place for `--quiet` seconds (default 60).

The user has allowed four windows at once on the right half of the screen, none overlapping, 2D runs as well as 3D
("Right half, four", 2026-11-22, `docs/RULES.md`). That is what the places are.

## The places

**The layout.** The right half is the primary screen's, from x = width / 2, and its size is read from the screen (the process
is DPI aware, so the numbers are real pixels). It is cut into an even grid of places, each at least 840x680 (an 800x600 client
with its frame and title bar, 40x80 on top of the client), at most four, numbered row by row. A 3840x2160 screen has a right
half of 1920x2160 and **four places of 960x1080**, two by two: place 0 at (1920, 0), 1 at (2880, 0), 2 at (1920, 1080), 3 at
(2880, 1080). A 1920x1080 screen has a right half of 960x1080 and **one place** (960x1080; a second would be shorter than
680). A 2560x1440 screen has two, 1280x720 one above the other; a 1280x720 screen has none, and a run refuses.

**What a run takes.** A window whose client plus frame fits a place takes one place. A larger one takes two adjacent places
(side by side first, then one above the other; the pair's rectangle is its place); one that fits no pair refuses to start.
The window goes to the place's top left as soon as it exists and stays there, topmost and never activated; a config's `pos`
is ignored on the right half (the log says so) and can be dropped from older configs. The game is also started with that
position as its startup position, for games that let Windows choose theirs. If the window's frame turns out larger than the
place, the run stops before any input (exit 3).

**The locks.** Each place has a lock file `place<N>.lock` under `%LOCALAPPDATA%\byo-pop\visrun\`, next to origrun's sandbox lock
under `%LOCALAPPDATA%\byo-pop\`; every worktree sees them, which a lock under a worktree's `build/` would not give. A file
holds {pid, run, start, game_pid} and lives for the whole run. A run takes the first group of places (in order: single
places 0 to 3, then pairs) whose files are free (or whose holder's process is gone: the file is renamed away, which only one
taker wins, and deleted) and that no other window overlaps right now, so a place with the user's window on it is skipped for the
next. It takes all the places of its group or none. A second lock, `game-<registry key>.lock`, keeps two runs of one game
apart, since they would share its registry key and saves (two configs of one game, or the same game from two worktrees):
the later one waits. With no free place a run waits and says so in `visrun.log` ("waiting for a place (N s): ..." with the
reason for each group, every minute); it gives up at its deadline.

**The quiet wait.** With its place taken, the run waits until no window has overlapped the place's rectangle for `--quiet`
seconds. Any visible, not minimised, not cloaked window there blocks it, except the desktop and the taskbar: another game, an
older visrun's, a port's, the user's own (a browser, a terminal). It looks at nothing outside its place, so runs in other places
go on. The run's own windows never block it; the game's window is moved only into a place no other window overlapped for
that time, so it is never put over a game window it does not own. Its first moments are the one gap: from creation to the move
(a poll every 2 ms) the window is where the game made it, usually the screen's centre, which can reach into the left column.

**Shots.** A shot is still refused while any other visible window overlaps the game's window, and three refusals in a row end
the run (exit 4).

**`python -I tools/visrun.py --places`** lists each place with its rectangle, who holds it (run, pid, game pid, minutes), and
the windows that are not the holder's own and overlap it (class, title, pid), then the game locks held. It starts nothing.

**An older visrun** (another worktree that has not merged this) still takes its own `build/visrun/visrun.lock` and waits until no
game window (`MainWindow` or `SDL_app`) has been on the whole desktop for its `--quiet` seconds. That is safe: while new runs
hold the right half it waits, and it knows nothing of the places. The one gap is the race at its start: a run that begins
in the instant after the older one's last check can have its window put wherever the older config's `pos` says. The older one
holds no place, so `--places` shows its window only as an overlapping window; a new run never starts on a place it overlaps.

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
   `SW_SHOWNOACTIVATE` only covers a program's first `ShowWindow` call, and the originals activate their window in a later one: in
   the first live check of the places, both games took the foreground as their window opened. So a thread watches the
   foreground and, whenever the game's process owns it, hands it back to the window that had it at the start, or to the one the user has moved to since (within
   20 ms in the check; the log counts the hand-backs and refusals).
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
   A run that is stopped from outside (a stopped background task, a killed queue script) puts nothing back: whoever stops
   it runs `--restore` for its config at once, since until then the user's own saves and key are the staged ones. The next
   run refuses to start over the backup, but that may be hours later (Bookworm Adventures, 2026-11-22: a queue stopped at
   08:12 left the user's own save out of their WinBAD users folder until 10:51).

## The config (TOML)

| Key | Meaning |
|---|---|
| `name` | The run's tag, under `build/visrun/<name>/`. |
| `game` or `source_exe` | A `games.tsv` name that origrun stages, or a ready copy's exe. |
| `exe`, `carve_exe` | The exe's leaf name in the copy, and a carved exe to place as it. |
| `args` | The game's command-line arguments, a list (Bejeweled Twist's Blitz: `["-autoplay=blitz", "-4x3"]`). |
| `patches` | A patch list (origrun's format), applied with `--seed`. |
| `title`, `client` | A text the window's title holds, and the least client size of the game window (`[w, h]`). |
| `pos` | Ignored: the run's place decides where the window goes. |
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
  A `focus lose` stands until the file's own `focus regain`: visrun otherwise tells the game it has the focus again before
  each step, since the game never holds the foreground. A game that stops counting updates while unfocused (Bejeweled
  Twist) needs `t` or `wait` steps until the regain, not `at`.
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

On the virtual clock visrun reads the stand-in's clock block, `tools/origrun/vclock.h`'s `VcShared`, through a copy of
its layout in `tools/visrun.py`, and refuses a block whose version or size is not the one it knows: rebuild `dsound.dll`
from the same checkout (`tools/origrun/build.sh`). The selftest holds the copy to the offsets the compiler gives for the
header, and `tools/check.sh unit` runs it; a change to `VcShared` changes both. Before this check, visrun read a version 3
block with the version 2 offsets: the watch ring's head and tail were garbage, its drain wrote without end (17.6 GB of
`watch.log` in Heavy Weapon's 3D run, 2026-11-22) and the run never closed the game. A drain now takes at most one
ring's worth and logs a gap larger than that once.

The stand-in holds the game at its main thread's first wait (`ORIGRUN_CLOCK_HOLDSTART`), so the first step finds it
where it started. That wait can come before the window: Bookworm Adventures waits before it makes its window. Until the
window exists, visrun lets such a hold go on with the stand-in's window hold (`VC_HOLD_WINDOW`: the game runs to its
first wait with a window of its own up, visible, top-level and at least 64x64, and holds there), so the game stands at
its first wait after the window (the log's "window after" line counts how often it was let on). Stepping it wait by
wait, as visrun did first, cost a round trip a wait, about 100 ms with the machine busy. Without this, visrun waited for a window the held game could not make,
the stand-in gave up the hold at its 60 s cap and turned holds off, and the counter never moved (Bookworm Adventures'
runs from 2026-11-22, each with its window after about 62 s and the end at update 0).

## Lessons from the first live runs

Venice's 3D tour (2026-11-15) was the first long run, and it taught four things:
- **Windows that were centred overlapped another game's window** (every shot was refused, exit 4): the first runs gave every
  config a `pos`; now the place does it.
- **A long tour needs `--timeout 600`.**
- **`values` print unsigned.** A negative number shows as its 32-bit pattern: -2 is 4294967294.
- **Peek every update while an update number matters.** The loading thread moves the first running update, which was
  1132 in one run and 936 in another, so a script cannot assume it. A peek can also fall inside an update: the
  counter can step by more than one update's ticks between two peeks, and a value read per frame (Venice's `batch`)
  need not belong to the frame drawn.

## Checked without a desktop

`python -I tools/visrun.py --selftest` (the time window, expressions, window picking, overlap, forbidden regions, the layout
of origrun's cursor block, the places at 3840x2160 and 1920x1080, a window that needs two places, lock order and a dead holder's
lock, the quiet wait with fake windows in and out of a place; it starts no game) and `--dry-run` (a step file's plan; also checks the forbidden regions). The rest needs a first
run on the desktop.

## The live check of the places (2026-11-22)

Two originals ran at once from two visrun processes started four seconds apart, Venice (800x600, `tools/visrun/venice.toml`,
`--3d off`) and Bejeweled 2 (640x480, the game's own `bejeweled2-2d.toml` without its two placeholder lines), short step files
(a cue, then two shots), `--quiet 3`. The user's Claude window overlapped places 0 and 2 and an older visrun's Bookworm window
was on the right half too, so the runs took place 1 and place 3 without waiting; `--places` showed both held (run, pid, game
pid) with their game locks, and showed nothing held afterwards. Both windows were muted (the stand-in's log), both shot twice
without a refusal, the two games' registry keys and save folders were identical before and after, and no process or lock
file was left. A foreground poll every 0.2 s showed the user's window throughout; the hand-back thread above came from the
first attempt of this check, where the poll saw each game's window take the foreground for a while.
