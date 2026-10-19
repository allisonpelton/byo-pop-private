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
- Bejeweled's headless verifier (`build/native/bejeweled_verify.exe`, and its wasm under Node) and the web
  build under Node (`node games/bejeweled/tools/verify/wasmrun.js game build/web/bejeweled.js ...`) open no
  window, play no sound and open no URL, so they need no rundesk; a web build under Node still counts as a
  game process below.
- Open the web builds in the Browser pane only with `?silent=1`, e.g.
  `http://127.0.0.1:18766/bejeweled.html?silent=1`: the pane may be hidden, and without it the game plays
  its music and sounds on the user's speakers. The status line ends in "silent" when it is on.
- Driving an SDL web build in the Browser pane (found by the BookWorm and Big Money sessions):
  - The pane's `left_click` sends no `mousemove`, so SDL sees the click at the previous pointer position.
    Hover over the point first, or dispatch `mousemove`, `mousedown` and `mouseup` on the canvas from JS.
  - Its `type` and `key` actions produce no `keypress` events, so text never reaches the game. Dispatch
    `KeyboardEvent`s (`keydown`, `keypress`) to `window` from JS instead.
  - A hidden pane runs `requestAnimationFrame` at only a few frames a second: the game runs slowly but
    correctly. For testing only, replace it in the page with a 16 ms `setTimeout`.
- Under rundesk (and the oracle) the framework never opens a URL; it logs "OpenURL in a test run". Builds or
  frozen exes from before that guard do open them: a click on a game's "more games" link (a steam:// URL)
  brings up the user's Steam client on their desktop. Random or generated input must not click link
  buttons (Options dialog, "buy" or "more games" screens) in any build.
- Run at most 3 game processes at a time, and never add artificial CPU load. The oracle (below) runs 3 at a
  time by default (`--jobs`, at most 3): start no other game tests while it runs.
- Only one oracle may run on the machine at a time. To wait for others, look for processes that run the
  oracle script (a command line like `bash .../compare.sh ...`) and exclude your own process tree. Never
  put the wait loop in the same shell command that later starts compare.sh: that shell's own command line
  then contains "compare.sh", so it waits on itself forever and blocks every other agent.
- The oracle's three runs per change (default, `-forcevariant`, the novariants build) are one command:
  `games/bejeweled/tools/oracle/compare.sh --variants default,forcevariant,novariants` (into build/oracle,
  build/oracle-forcevariant, build/oracle-novariants; build the novariants exe first with
  `tools/build.sh novariants`). The frozen baselines' runs come from `build/oracle-cache` (keyed on the exes,
  inputs, game files and machine; see compare.sh's header); `--no-cache` makes every run now, `--jobs 1` runs
  one game at a time in the old order. The candidate is never cached, so the cache cannot hide a regression;
  delete `build/oracle-cache` when in doubt. A "Draw Count" note in a report is the machine's speed within a
  bound, not a difference (compare.py, DRAWS); compare.sh then drops the cache entries of the baseline runs
  involved, so the next oracle makes them again. If such a note repeats in the same case, check it with
  `--no-cache --jobs 1`.
- Do not leave anything running: loops started in the background must end before you finish, and a stopped
  task's child processes must be killed.
- Stop only processes you started, identified by a command line inside your own folder or worktree. Several
  sessions share this machine, so never stop a game exe by its name alone.
- Never run `-script`/`-autoplay` runs on the user's desktop. `ORACLE_VISIBLE=1` exists only for the user.

## Sessions, ownership and git (the user's rules)

The work is split across Claude sessions (the user's design, 2026-10-19):
- **One session per game** owns `games/<game>/` on its own branch and worktree: the decompilation, all of its
  testing, and its mods.
- **The coordinator session** (session id `local_69b2d317-1aff-4b2b-ae9f-c9883bcf6d97`, in the main
  checkout) owns the shared code: `framework/`, `platform/`, `third_party/`, `web/`, `tools/`, `cmake/`,
  `docs/` and the top-level files. It builds shared libraries and decompilation tooling, and it integrates:
  it merges each game's branch into the trunk at that game's milestones, after checking that every game
  still builds and passes its own tests.
- **Shared changes:** a game session that needs one sends the coordinator a message (SendMessage to the id
  above) with a precise spec: what, why, which files, how to test. A tiny shared fix it cannot wait for
  goes in its own commit titled `shared: ...`, so the coordinator can review it and carry it over. Game
  sessions do not edit another game's folder.
- **Trunk:** `code-mods` (to be renamed `main`). Game branches merge the trunk in regularly.
- **Mark overrides.** Port-side game code that overrides a framework virtual (hooks such as
  `GetShutdownSaves`, `HandleScriptCommand`, `OpenURL`) must say `override`, so a renamed or changed hook
  breaks the build instead of silently never being called. (A rename once left Big Money's web backup
  without the game in progress.)

Git and publishing:
- Game sessions may commit on their own branch at verified milestones without asking the user each time.
  Only the coordinator commits on the trunk. Nobody merges into `master`.
- Never push. The user decided when the repository is first pushed: once every Bejeweled mod is done and
  Big Money fully works natively and in the browser. The coordinator does that, after auditing the whole
  history for game files.
- Never commit or redistribute game files or anything derived from them (staged assets, Ghidra exports,
  word lists and other game data, mods built from another game's files, close recreations of another
  game's art): keep them git-ignored. Docs and code comments refer to a game's texts by id and paraphrase;
  a few words of quote to identify something are fine, tables or long passages of the game's strings are
  not. Downloads of any kind need the user's explicit OK first (name,
  source, size).
- Commits end with the trailer `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Use the user's
  email only for authorship, never anywhere else.
