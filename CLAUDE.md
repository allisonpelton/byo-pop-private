# Working on this repo (byo-pop)

One repository for every game: shared `framework/`, `platform/`, `engines/`, `web/`, `tools/` and `docs/`, and one
folder per game under `games/<game>/`. Game files are staged into `assets/<game>/` and Ghidra exports go to
`re/<game>/`; both are git-ignored and never committed. The coordinator's rules are in `docs/COORDINATION.md`.
`docs/RULES.md` holds every rule below in full, with the user's words and dates: read its section when a rule's edge
matters.

## Running games in tests

The user works on this machine, so every test run of a game is invisible, silent and low priority.
- Start every game exe (ports, `fwtest`, `smoke`, `lsprobe`, frozen baselines, scratch builds) through
  `build/native/rundesk.exe`, or a script under it: `build/native/rundesk.exe bash my-loop.sh` (a private desktop,
  SDL's silent audio driver, below-normal priority; build it with `cmake --build build/native --target rundesk`).
  `tools/check.sh` and the oracle and LiveSplit scripts re-run themselves under it. Headless runs (Bejeweled's
  verifier, a web build under Node) need no rundesk but still count as game processes.
- **Originals** run only through `tools/origrun` (`docs/ORIGRUN.md`), from a copy under build/, never in an install
  folder; origrun puts the game under rundesk itself (never wrap it) and silences it (a RESULT line says `SOUND LEAK`
  otherwise). Merge the trunk and rebuild origrun before running originals. The user allows sound, changed saves and
  any change to PopCap games' own registry values; the rest of the registry stays untouched. The hooks the user has
  approved are listed in `docs/ORIGRUN.md`; any other hook of a Windows function needs the user's OK first. Patch
  copies as needed; how a protection is taken apart goes only in private blocks, and patching tools in `.publishignore`.
- Open web builds in the Browser pane only with `?silent=1`. The terms card skips itself on development hosts; on the
  deployed site the test key in the git-ignored `secrets/web-testkey.txt` skips it, and that key is never written into
  chat, commits or docs. The site goes on a public host only when the user says so.
- No run ever clicks a link, "Buy" or "More Games" button, in any build or original: old builds and originals open the
  user's browser or Steam.
- **Uncapped, one core each:** a port runs with `-fast` in every test that does not need real time (check first that
  `-fast` leaves the results unchanged), an original on origrun's virtual clock. Real time only for LiveSplit, sound
  and focus checks and the Browser pane. Speed comes from not waiting, never from threads.
- At most 6 game processes on the machine across every session, and at most 3 for one check or script (`--jobs`).
  Count first: `python tools/gamecount.py --limit 6` exits 1 when there is no room. Never add artificial CPU load.
- One oracle at a time (`docs/BUILDS.md`, "Running the oracle"). A wait loop whose own command line holds "compare.sh"
  waits on itself forever: run it from a script file, or exclude your own process tree.
- **Disk:** C: is small. Delete a run's outputs once their result is in the game's notes, and keep a worktree's
  `build/` under about 3 GB, checked before each merge request.
- Leave nothing running: afterwards look for processes whose command line is inside your folder and stop them by id.
  Stop only processes you started, never a game exe by name alone. End a turn waiting only on something that wakes you
  (a background command, a Monitor that ends by itself); a hidden pane runs no requestAnimationFrame unless shimmed.
- Never run `-script`/`-autoplay` runs on the user's desktop (`ORACLE_VISIBLE=1` is the user's).
- **Overnight, 22:00 to 08:00 local:** any game, port or original, and any test program that opens a window, may run on
  the visible desktop or in Windows Sandbox (`--sandbox-vgpu`, `--sandbox-connect`) with sound, scripted runs too; a
  run started then ends by 08:00. Everything else holds: 6 processes, no link clicks, the user's cursor and keyboard
  never touched (input posted to the game's window), window, registry and saves put back, nothing left running.

## Token economy

Every call re-reads the whole context (`docs/TOKEN_ECONOMY.md`).
- No workflows or parallel agents unless the user asks; mechanical work goes to one Sonnet agent, given the files and
  sections it needs.
- A subagent never runs anything longer than about 4 minutes (the oracle, whole suites, full builds, wait loops): the
  owning session runs those in the background and waits for the notification instead of polling.
- Read narrowly (grep, then the range), and batch the reads you already know you need into one call. Send long output
  to a log file and print a summary line. Test with `tools/check.sh <suite>...` (`--help` lists them).
- Sessions keep their conversation and compact on their own at the full window; nobody clears them or writes handoff
  notes. Monitors notify when a run ends, not on progress.

## Sessions, ownership and git

- **One session per game** owns `games/<game>/` on its own branch and worktree (decompilation, testing, mods) and
  edits no other game's folder. The user directs the game-specific work.
- **The coordinator** (`local_dba6c364-63c7-4e08-8ad1-0d086d673e52`, the main checkout) owns `framework/`,
  `platform/`, `third_party/`, `engines/`, `web/`, `tools/`, `cmake/`, `docs/` and the top-level files, and merges
  each game's branch at its milestones after checking every game still builds and passes.
- **Shared changes:** send the coordinator a precise spec (what, why, which files, how to test). A tiny shared fix that
  cannot wait goes in its own commit titled `shared: ...`. Before a merge request, merge the trunk in and run
  `tools/check.sh --changed main` and `tools/check.sh build` (tools included).
- **Shared engines:** sibling games on one engine keep one copy under `engines/<library>/` (`docs/ENGINES.md`). Each
  function's comment gives every game's address; where versions differ, each game sets which. A change there is
  shared: `tools/check.sh --changed` runs every game that uses it, each frozen baseline unchanged.
- **A new worktree:** run `tools/setup_worktree.sh` once. Cut it from the local `main`, never `origin/main`, which holds
  only publish snapshots with no shared history (`git rebase --onto main <snapshot>` fixes one). Merge the trunk in
  regularly.
- **Mark overrides:** port code that overrides a framework virtual says `override`.
- **Commits:** game sessions commit on their own branch at verified milestones; only the coordinator commits on the
  trunk; nobody merges into `master`. A commit ends with a trailer naming the model that wrote it, such as
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>` (or Claude Sonnet 5.5, Claude Fable 5.2). The user's email
  is for authorship only.
- **Never push.** Only the coordinator publishes, as snapshots without `.publishignore`'s paths and `<!-- private -->`
  blocks (`docs/PUBLISHING.md`), and keeps the backups.
- **Game files:** never commit or redistribute game files or anything derived from them (staged assets, exports, word
  lists and other game data, mods built from another game's files, close recreations of its art). Docs and comments
  name a game's texts by id and paraphrase. Downloads of any kind need the user's explicit OK first (name, source,
  size).

## Making a port

The steps and their order are in `docs/COORDINATION.md` ("Which games, in what order"), the mods in
`docs/MOD_ROADMAP.md`.
- **Port first, unmodified:** a port is the original game working like its exe, with nothing added for later mods
  (the site's hooks and an autosplitter's inert state excepted). No new mods unless the user asks. A bug of the
  original, or a wait that only costs the player time, stays and is fixed in the fixes mod ("Fix candidates" in the
  notes until then). The port itself fixes memory leaks and bugs that break play (dead controls, a soft lock, a crash,
  a lost save), and loads at the machine's speed; each is listed with the deviations. Fixes and lite changes are read
  through `Platform::FixOn` and `LiteOn` (`platform/GameOptions.h`).
- **Done means identical, in two phases:** first the logic (with the same input, a series of games plays out the same:
  boards, every random draw in order, moves, scores, timers, endings; checked by state more than by pictures), then the
  looks (every screen, dialog and text, sound by state, losing and regaining the focus, natively and on the web). Shots
  are compared with `tools/shotdiff` (`docs/SHOTDIFF.md`), and every flagged difference is judged and fixed or
  explained, timing included; masks are listed with their reasons. Each game keeps its deviations listed, each without
  a visible effect, and a frozen baseline with a regression check.
- **Real input before the final merge:** through the page in the Browser pane and the native window under rundesk
  (posted input, `docs/REAL_INPUT.md`), from launch through the tutorial and a whole game of every mode, 2D and 3D,
  windowed and full screen, plus the page check (`tools/web/pagecheck.js`). Sessions never play games themselves: a
  driver plays, taking its moves from the port's bot through the `move` command and reaching states with the standard
  test commands (`docs/TEST_COMMANDS.md`). A screenshot checks what is drawn, never picks a move.
- **Oddities are not chased:** a fix is the original's own logic from our export, small and plain. When a difference
  would need silly code, or its cause is not found within about an hour, write it under "Oddities" in the game's notes
  (what it does, how to see it, what it would take) and move on.
- **A report for each pass:** `games/<game>/docs/IDENTICAL_PLAY.md`, in prose: what was compared and how, every change
  (the difference, its cause, the fix, the commit), every oddity skipped and the deviations left.
- **Registration and outbound buttons are removed, not disabled:** the registered path becomes the only one, with no
  flag, widget, string or image left that only the other used (a save keeps a field only for its format). Every button
  that leads out of the game goes too, its neighbours closing the gap.
- **Readability (`docs/CODE_STANDARDS.md`):** no unknown or unnamed variable, field, parameter, global or constant. A
  new port writes to the standards from its first draft and leaves untranslated what the export shows nothing reaches.
- **Done means every phase but modding:** the user's faults, both phases, registration and outbound buttons out, the
  fixes mod, readability, coverage, performance (processor time at or below the original's, measured like for like),
  the lite mod, unneeded code out, the Collection check for a Steam port, and the site's features (Music, soundboard
  presets, an autosplitter where speedrun.com has a board). Never hurry a game. A session may speed up its port early,
  under the performance pass's rules: how it works changes, never what it does, and no threads.
- **No references:** never read another project's decompilation, reimplementation, mod kit, class layouts or address
  maps; work from our own Ghidra export. PopCap's released SexyAppFramework source, the libraries a game uses and
  file-format descriptions are fine.
- **Mac builds:** where PopCap's Mac build exists (`re/<game>/mac/`), a new decomp first takes its names, types and class
  boundaries for the PC export (`tools/macmatch.py`, `docs/MACMATCH.md`). The PC exe stays the authority for
  behaviour, and no symbol list goes into the repo.
- **Sonnet sessions** translate loops in their original shape, check what a comment claims about the original before
  writing it, and build with AddressSanitizer at the first sign of memory corruption.
