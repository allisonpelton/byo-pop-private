# Working on this repo (byo-pop)

One repository for every game: shared `framework/`, `platform/`, `web/`, `tools/` and `docs/`, and one
folder per game under `games/<game>/` (Bejeweled: `games/bejeweled`). Game files are staged into
`assets/<game>/` and Ghidra exports go to `re/<game>/`; both are git-ignored and must never be committed.

## Running the game in tests

Every test run of a game exe must be invisible, silent and low priority, because the user is working on
the same machine. A plain run opens a window, raises it over whatever the user is doing and plays sound.

- Start every game exe (`bejeweled.exe`, `fwtest.exe`, `smoke.exe`, frozen exes under `build/baseline*`,
  scratch builds) through `build/native/rundesk.exe`, or run the script that starts them under it:
  `build/native/rundesk.exe bash my-loop.sh`. rundesk puts the command and all its children on a private
  desktop, gives them SDL's silent audio driver (always: it replaces any `SDL_AUDIODRIVER` you set) and
  below-normal priority. Build it with `cmake --build build/native --target rundesk`.
- `games/bejeweled/tools/oracle/compare.sh` already re-runs itself under rundesk, and so does
  `games/bejeweled/tools/livesplit/run_tests.sh`. `build/native/lsprobe.exe` starts the game it probes:
  run it under rundesk too.
- Open the web builds in the Browser pane only with `?silent=1`, e.g.
  `http://127.0.0.1:18766/bejeweled.html?silent=1`: the pane may be hidden, and without it the game plays
  its music and sounds on the user's speakers. The status line ends in "silent" when it is on.
- Run at most 3 game processes at a time, and never add artificial CPU load.
- Do not leave anything running: loops started in the background must end before you finish, and a stopped
  task's child processes must be killed.
- Stop only processes you started, identified by a command line inside your own folder or worktree. Several
  sessions share this machine, so never stop a game exe by its name alone.
- Never run `-script`/`-autoplay` runs on the user's desktop. `ORACLE_VISIBLE=1` exists only for the user.
