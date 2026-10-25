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
- While working on a few cases, run only those (it saves time and tokens): `compare.sh --cases <glob>,...`,
  e.g. `--cases "shape_*,rules_inert"` (matched against the whole case name), with any `--variants`. It runs the
  matching cases, the cases compare.py compares them with, and the runs they read (Bejeweled's stage 7 cases,
  three ways: 164 s instead of about 950 s). Its report says it was partial and expect-update refuses it: the
  whole oracle is still the check before a commit.
- Do not leave anything running: loops started in the background must end before you finish, and a stopped
  task's child processes must be killed.
- Stop only processes you started, identified by a command line inside your own folder or worktree. Several
  sessions share this machine, so never stop a game exe by its name alone.
- Never run `-script`/`-autoplay` runs on the user's desktop. `ORACLE_VISIBLE=1` exists only for the user.

## Token economy (the user's rules, 2026-10-24)

Usage limits ran out in 36 hours of one week; `docs/TOKEN_ECONOMY.md` has the measurements. Every call
re-reads the whole context, so what costs is context times calls, and a cache that expires while waiting.
- No workflows or parallel agents unless the user asks for them; mechanical work goes to one Sonnet agent.
- A subagent never runs anything longer than about 4 minutes (the oracle, whole test suites, full builds,
  wait loops): its 5-minute cache expires and its whole context is written again. The owning session runs
  those, in the background, and waits for the notification instead of polling.
- Read narrowly: grep, then the range you need; the section of a doc, not the doc. Send long output to a
  log file and print a summary line.
- Test with `tools/check.sh <suite>...` (`--help` lists them): one line per suite, logs in files.
- Give an agent the files and sections it needs, so it need not read the design docs to find them.

## Sessions, ownership and git (the user's rules)

The work is split across Claude sessions (the user's design, 2026-10-19):
- **One session per game** owns `games/<game>/` on its own branch and worktree: the decompilation, all of its
  testing, and its mods.
- **The coordinator session** (session id `local_dba6c364-63c7-4e08-8ad1-0d086d673e52` since the folder
  rename of 2026-10-24, in the main checkout) owns the shared code: `framework/`, `platform/`, `third_party/`, `web/`, `tools/`, `cmake/`,
  `docs/` and the top-level files. It builds shared libraries and decompilation tooling, and it integrates:
  it merges each game's branch into the trunk at that game's milestones, after checking that every game
  still builds and passes its own tests.
- **Shared changes:** a game session that needs one sends the coordinator a message (SendMessage to the id
  above) with a precise spec: what, why, which files, how to test. A tiny shared fix it cannot wait for
  goes in its own commit titled `shared: ...`, so the coordinator can review it and carry it over. Game
  sessions do not edit another game's folder.
- **Directing sessions (the user, 2026-10-24):** the user tells the game sessions about game-specific changes;
  the coordinator does not direct or check on them. A game session messages the coordinator when it needs it:
  a shared change, a milestone ready to merge, or its mods finished (the push waits for that). The coordinator
  looks at a session's work only when the user asks, or when it breaks something for other sessions (shared
  code, the trunk, the machine's test rules). It does message the sessions when the way they are instructed
  changes (a new rule like this one), since a running session reads CLAUDE.md again only at its next start
  or compaction.
- **A new worktree** lacks the git-ignored game files, baselines, oracle cache and generated mods: run
  `tools/setup_worktree.sh` in it once (copies from the main checkout, no junctions).
- **Trunk:** `main` (named `code-mods` until 2026-10-19, after Bejeweled's code-mods stages 4 and 5). Game branches
  merge the trunk in regularly.
- **Mark overrides.** Port-side game code that overrides a framework virtual (hooks such as
  `GetShutdownSaves`, `HandleScriptCommand`, `OpenURL`) must say `override`, so a renamed or changed hook
  breaks the build instead of silently never being called. (A rename once left Big Money's web backup
  without the game in progress.)

Git and publishing:
- Game sessions may commit on their own branch at verified milestones without asking the user each time.
  Only the coordinator commits on the trunk. Nobody merges into `master`.
- Never push. The user decided when the repository is first pushed (2026-10-24): when the Bejeweled, Big Money
  and BookWorm sessions have finished their mods, before any new decompilation starts. The coordinator does
  that, after auditing the whole history for game files.
- **Order of work (the user, 2026-10-24):** those three sessions finish their mods; then the push; then one
  session alone decompiles the simplest remaining game, and its performance is measured against the earlier
  games; only if that shows an improvement do up to three sessions run at once again. New game sessions list
  the mods worth making but make none without the user's approval, and no further game gets mods until every
  PopCap game works in the browser.
- **Sonnet trial (the user, 2026-10-25):** that next game's session runs on Sonnet. Once its port works, it
  brainstorms mods for the game and makes the interesting ones: the user approved mods for this one game, the one
  exception to the rule above. The coordinator measures it against the earlier games (`docs/TOKEN_ECONOMY.md`):
  tokens, time, and where Sonnet struggled. If Sonnet does well in most of it (fewer tokens, not absurdly long), it
  is the default for games that can largely copy another game's port, with Opus for the kinds of work where it
  showed deficits. Complex games (Zuma's Revenge, Bejeweled 3) stay on Opus.
- Never commit or redistribute game files or anything derived from them (staged assets, Ghidra exports,
  word lists and other game data, mods built from another game's files, close recreations of another
  game's art): keep them git-ignored. Docs and code comments refer to a game's texts by id and paraphrase;
  a few words of quote to identify something are fine, tables or long passages of the game's strings are
  not. Downloads of any kind need the user's explicit OK first (name,
  source, size).
- Commits end with the trailer `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Use the user's
  email only for authorship, never anywhere else.
