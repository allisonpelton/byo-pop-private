# Working on this repo (byo-pop)

One repository for every game: shared `framework/`, `platform/`, `web/`, `tools/` and `docs/`, and one
folder per game under `games/<game>/` (Bejeweled: `games/bejeweled`). Game files are staged into
`assets/<game>/` and Ghidra exports go to `re/<game>/`; both are git-ignored and must never be committed.
The coordinator's own rules (planning, usage, which games and when) are in `docs/COORDINATION.md`.

## Running games in tests

Every test run of a game exe must be invisible, silent and low priority, because the user is working on
the same machine. A plain run opens a window, raises it over whatever the user is doing and plays sound.

- Start every game exe (`bejeweled.exe`, `fwtest.exe`, `smoke.exe`, frozen exes under `build/baseline*`,
  scratch builds) through `build/native/rundesk.exe`, or run the script that starts them under it:
  `build/native/rundesk.exe bash my-loop.sh`. rundesk puts the command and all its children on a private
  desktop, gives them SDL's silent audio driver (whatever `SDL_AUDIODRIVER` says) and below-normal priority. Build it
  with `cmake --build build/native --target rundesk`. `tools/check.sh`, `games/bejeweled/tools/oracle/compare.sh` and
  `games/bejeweled/tools/livesplit/run_tests.sh` re-run themselves under it; run `build/native/lsprobe.exe`, which
  starts the game it probes, under it too.
- Headless runs need no rundesk: Bejeweled's verifier (`build/native/bejeweled_verify.exe`, and its wasm under Node)
  and a web build under Node (`node games/bejeweled/tools/verify/wasmrun.js game build/web/bejeweled.js ...`) open no
  window, play no sound and open no URL. A web build under Node still counts as a game process.
- **Originals** run only through `tools/origrun` (`docs/ORIGRUN.md`: what it does, its status table for each game, its
  gaps), from a copy under build/, invisibly under rundesk; an install folder is never written to or run in place.
  origrun is silent by default (rundesk silences only SDL): its stand-in also serves the DirectSound a copy asks COM
  for (BASS in Alchemy, Pixelus and Rocket Mania) and mutes the copy's audio sessions as it loads, so nothing reaches
  the speakers, and a run's RESULT line says `SOUND LEAK` if anything did. A copy keeps the stand-in its last run
  placed, so a worktree merges the trunk (2026-11-03 or later) and rebuilds origrun before running originals. The user allows sound and changed saves ("You are welcome
  to run the originals with sound on and mess with my save data", 2026-10-31; the saves they care about are backed
  up), and any change to PopCap games' own registry values without restoring them ("You can mess with the registry
  entries for PopCap games all you like. I promise.", 2026-11-01); the rest of the registry stays untouched. origrun
  may hook a copy's time reads and timed waits to run it faster than real time ("Yes, hook them", 2026-11-02; "Yes,
  waits too", 2026-11-03), its GetCursorPos so that it returns origrun's scripted mouse position ("Yes, hook it",
  2026-11-03), and its MessageBox and MessageBeep calls so that a message box is answered without showing or sounding
  ("Yes, hook them", 2026-11-03); any other hook of a Windows function inside a copy needs the user's OK first
  (`docs/ORIGRUN.md`). Patch a copy as needed (many are not activated): how a protection is taken apart goes only in
  private blocks, and the patching tools in `.publishignore`. origrun answers a copy's message boxes with no window
  and its MessageBeep calls with no sound, unless `--msgbox show` (a box then rings, as do boxes called through
  GetProcAddress, `--sound real` runs and games that never load the stand-in; `docs/ORIGRUN.md`). It has no guard
  against URL or program launches, so a script never clicks a link, "Buy" or "More Games" button. When a run really
  needs real sound, the user turns the speakers off for a short stretch or overnight. An original counts as a game
  process.
- Open web builds in the Browser pane only with `?silent=1`, e.g. `http://127.0.0.1:18766/bejeweled.html?silent=1`
  (the status line then ends in "silent"): the pane may be hidden, and without it the game plays its music and sounds
  on the user's speakers. The terms card is on in every build but skips itself on a development host (127.0.0.1,
  localhost, [::1], *.localhost, *.test, file://), so local tests never meet it; `?terms=1` shows it there. On the
  deployed site an agent skips it with the test key from the git-ignored `secrets/web-testkey.txt`, which is never
  written into chat, commits or docs; either skip counts no play (`docs/WEB_PLAYER.md`, "The terms"). The site goes
  on a public host only when the user says so.
- Under rundesk the framework never opens a URL (it logs "OpenURL in a test run"), but builds and frozen exes from
  before that guard do: a click on a game's "more games" link brings up the user's Steam client. Random or generated
  input never clicks link buttons (Options dialog, "buy" or "more games" screens) in any build.
- **Uncapped, one core each (the user, 2026-11-02):** "As much as possible, allow programs to run at uncapped speed on
  a single thread, to speed up development." A port runs with `-fast` (its virtual clock: no waiting, a draw every few
  updates) in every test run that does not need real time, in a session's own comparison scripts as in check.sh's
  suites; an original runs on origrun's virtual clock once it has one (`docs/ORIGRUN.md`). Real time only where the run
  needs it: LiveSplit's real-time runs, sound and focus checks, the Browser pane. Before relying on `-fast`, check that
  it leaves the run's results unchanged (a game that draws random numbers while drawing would not). Speed comes from
  not waiting, never from threads: no game's code is made multithreaded to run faster (the user, 2026-11-02).
- Run at most 3 game processes at a time, and never add artificial CPU load. While the user sleeps the machine may run
  up to 8, counted across every session (the user, 2026-11-07: "Up to 8 while I sleep"; that night until 09:00 local
  time on 2026-11-08): count the game processes already running on the machine before starting more. The oracle runs 3 at a time by default
  (`--jobs`, at most 3): start no other game tests while it runs.
- Only one oracle runs on the machine at a time. To wait for another, look for processes that run the oracle script
  (a command line like `bash .../compare.sh ...`) and exclude your own process tree. Never put the wait loop in the
  same shell command that later starts compare.sh: that shell's own command line contains "compare.sh", so it waits
  on itself forever and blocks every other agent. How to run the oracle (its variants, cache and partial runs):
  `docs/BUILDS.md`, "Running the oracle".
- Leave nothing running: loops started in the background end before you finish. Stopping a task can leave its loop's
  shells alive and still starting runs (Ning Po, 2026-11-03), so afterwards look for processes whose command line is
  inside your folder and stop them by id.
- End a turn waiting only on something that wakes you: a background command or a Monitor that ends by itself and
  notifies you. A run you watch in the Browser pane never wakes you, and a page in a hidden pane runs no
  requestAnimationFrame unless it is shimmed, so it can stand still for good. A session that ended its turn on "waiting
  for the web tour" sat idle for a day (2026-11-03). Stop only processes you started, identified by a command line inside your own folder or worktree;
  several sessions share this machine, so never stop a game exe by its name alone.
- Never run `-script`/`-autoplay` runs on the user's desktop. `ORACLE_VISIBLE=1` exists only for the user.

## Token economy (the user's rules, 2026-10-24)

Every call re-reads the whole context, so what costs is context times calls, and a cache that expires while waiting
(`docs/TOKEN_ECONOMY.md` has the measurements).
- No workflows or parallel agents unless the user asks for them; mechanical work goes to one Sonnet agent.
- A subagent never runs anything longer than about 4 minutes (the oracle, whole test suites, full builds,
  wait loops): its 5-minute cache expires and its whole context is written again. The owning session runs
  those, in the background, and waits for the notification instead of polling.
- Read narrowly: grep, then the range you need; the section of a doc, not the doc. Send long output to a
  log file and print a summary line.
- **Batch reads (the user, 2026-11-01, a trial).** Most calls are a single read, often several in a row
  (`docs/TOKEN_ECONOMY.md`, "Calls"). When you already know the next few things you need (a function and its caller, a
  header and its .cpp, the export and the port side by side), get them in one call: several tool calls in one message,
  or one Bash command with the reads separated by `echo ---`.
- Test with `tools/check.sh <suite>...` (`--help` lists them): one line per suite, logs in files.
- Give an agent the files and sections it needs, so it need not read the design docs to find them.

## Sessions, ownership and git

- **One session per game** owns `games/<game>/` on its own branch and worktree: the decompilation, all of its
  testing, and its mods. Game sessions do not edit another game's folder.
- **The coordinator session** (`local_dba6c364-63c7-4e08-8ad1-0d086d673e52`, in the main checkout) owns the shared
  code: `framework/`, `platform/`, `third_party/`, `web/`, `tools/`, `cmake/`, `docs/` and the top-level files. It
  builds shared libraries and decompilation tooling, and it merges each game's branch into the trunk at that game's
  milestones, after checking that every game still builds and passes its own tests.
- **Shared changes:** a game session that needs one sends the coordinator a message (SendMessage to the id above)
  with a precise spec: what, why, which files, how to test. A tiny shared fix it cannot wait for goes in its own
  commit titled `shared: ...`, so the coordinator can review it and carry it over. A game session also messages the
  coordinator when a milestone is ready to merge. The user directs the game-specific work.
- **A new worktree** lacks the git-ignored game files, baselines, oracle cache and generated mods: run
  `tools/setup_worktree.sh` in it once (copies from the main checkout, no junctions). Cut it from the local `main`
  (the trunk), never from `origin/main`: that holds only the publish snapshots, which share no history with `main`, so
  a branch made from one cannot merge (`git rebase --onto main <snapshot>` fixes one). Game branches merge the trunk
  in regularly.
- **Mark overrides.** Port-side game code that overrides a framework virtual (hooks such as `GetShutdownSaves`,
  `HandleScriptCommand`, `OpenURL`) must say `override`, so a renamed or changed hook breaks the build instead of
  silently never being called.
- **Commits:** game sessions may commit on their own branch at verified milestones without asking the user each time.
  Only the coordinator commits on the trunk. Nobody merges into `master`. Commits end with the trailer
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Use the user's email only for authorship, never anywhere
  else.
- **Never push.** Only the coordinator publishes, and only as snapshots (the user, 2026-10-25):
  `python tools/publish.py --push` pushes one commit holding the trunk's tree without the paths in `.publishignore`
  and the `<!-- private -->` blocks of text files (`docs/PUBLISHING.md`). How to take a game's protection apart
  (offsets in a wrapper, unwrapping commands, how a launcher checks its copy) goes only inside private blocks, and
  tools that do it go into `.publishignore`.
- **Game files:** never commit or redistribute game files or anything derived from them (staged assets, Ghidra
  exports, word lists and other game data, mods built from another game's files, close recreations of another game's
  art): keep them git-ignored. Docs and code comments refer to a game's texts by id and paraphrase; a few words of
  quote to identify something are fine, tables or long passages of the game's strings are not. Downloads of any kind
  need the user's explicit OK first (name, source, size).

## Making a port

- **Port first, unmodified (the user, 2026-10-26):** a port is the original game, unmodified and working like its exe;
  it adds nothing for later mods, not even hooks (the site's own hooks for player names and settings are the
  exception). No new mods unless the user requests one ("no new mods for anything unless someone requests it",
  2026-10-30); fixes and suggestions for the mods that exist wait in `docs/MOD_ROADMAP.md` until every
  decompilation is finished. Code that makes a game moddable makes things configurable and never overwrites the
  original (`docs/MOD_ROADMAP.md`, "Kinds of mod"). The site will serve each game with its fixes mod on, one toggle a
  fix, and a game is done only once that mod is made (the user, 2026-11-04): a bug of the original, or a wait that
  only costs the player time, is kept in the port and fixed in the mod (listed until then under "Fix candidates" in
  the game's notes), and with the mod on a game plays the same on every run. Memory leaks are still fixed in the
  port itself, and so is a bug of the original that breaks play: controls that stop working, a soft lock, a crash, a
  lost save (the user, 2026-11-07, of Seven Seas' dead squares: "That sort of bug shouldn't be part of a fix mod"),
  listed in the game's notes with its deviations (`docs/MOD_ROADMAP.md`, "The fixes mod").
- **Done means identical (the user, 2026-10-27):** "A game is done when it plays identically to the original in the
  browser and natively." No port is hurried to get through the list. A port's own suite matching its earlier output
  proves nothing about the original, and the hidden, silent pane never changes focus or plays music: before its final
  merge, a session compares its game with the original exe screen by screen (the original run under rundesk at the
  same points; every screen, dialog and text, music and sounds by their state, losing and regaining the focus),
  natively and in the web build, and fixes every difference. The screenshots are compared by `tools/shotdiff`
  (`docs/SHOTDIFF.md`), not by eye: the session reads its summary, looks only at the crops of the boxes it flags, and
  judges every flagged difference itself, fixing or explaining each one, timing included: it lines up the input with
  the game's updates, or masks animation that runs on its own clock and lists each mask with its reason. The user's
  review page holds only what a session truly cannot judge, and the user sees it only once it holds every game. Every
  game is tested as Bejeweled is: its deviations listed in its notes, each without a visible effect; every RNG draw
  kept in its order; a frozen baseline of the port with a regression check that proves later fixes and mods leave
  unmodded play unchanged. Every memory leak of the original is fixed: memory the original still uses after "leaking"
  it (Bejeweled's FillBoard orphans) is freed after its last possible use. Autoplay, script commands and runs under
  Node never pass through the real input path, so before its final merge a session also plays the game with real mouse
  and key events only: through the page in the Browser pane (`?silent=1`, requestAnimationFrame shimmed while hidden)
  and through the window natively under rundesk (posted input, as visrun does), from launch through the tutorial and a
  whole game of every mode, in 2D and 3D where both exist, windowed and in full screen. Twist passed every comparison
  and still could not twist a gem in the browser (the user, 2026-11-05). Sessions never play a game themselves (the
  user, 2026-11-07: "the sessions are not allowed to play any games themselves"; "Yes, use drivers"): code plays, a
  driver script that reads the game's state from its log, chooses the moves (a solver or bot where the game has one)
  and sends them as those real events, a minigame or a whole game in a call or two. The session writes the driver and
  judges its results, and looks at a screenshot only to check what is drawn, never to pick a move. Natively, posted
  clicks reach SDL only as one burst of move, press and release (`games/sevenseas/tools/realinput.py`); a held press
  or drag waits for the shared tool. A driver reaches the state it needs (a level, a boss, a locked mode) with the
  game's standard test commands instead of playing up to it (`docs/TEST_COMMANDS.md`; the user, 2026-11-07: "Yes,
  add them"): every port has them, reachable only from a test, and lists them in its notes. The move a driver makes
  comes from the port's own bot through the `move` command, so the driver stays a thin loop and a later bot mod reuses
  the bot (the user, 2026-11-07: "the scripts that play the games should be developed in a way that their code can be
  reused for a mod that adds a bot").
- **Two phases: plays identical, then looks identical (the user, 2026-11-01).** The first phase is the game's logic:
  with the same input, a series of games plays out the same in the original and the port (the same boards and random
  draws, moves, scores, levels, timers and endings). Check it by the game's state (origrun's `--watch` values at an
  update, the port's own logs) more than by pictures. When a series of games plays out the same, the first phase is
  done. The second phase is everything else in the bullet above: screen by screen with shotdiff, sound, focus and the
  web build. A graphics fault noticed during the first phase may be fixed then. Passes already running do both phases;
  every other game gets the first phase only, until every game plays identical (`docs/COORDINATION.md`).
- **Oddities are not chased (the user, 2026-11-01):** "If the original game behaves weirdly, I don't *necessarily* want
  to preserve that", and no session should spin its wheels on something obscure. A fix is the original's own logic,
  found in our export and kept small and plain. When a difference would need silly code (values measured from runs,
  leftover or uninitialised state, the original's own run-to-run variation), or its cause is not found after about an
  hour of looking, the session stops and writes it under "Oddities" in the game's notes: what the original does, how
  to see it, and what it would take. Then it moves on. Oddities wait for later, perhaps a write-up or videos of each
  game's quirks.
- **A report for each pass (the user, 2026-11-01):** a session that finishes a phase of a game's identical-play pass
  writes `games/<game>/docs/IDENTICAL_PLAY.md` for the user, or adds the phase to it, before the phase's last merge. It
  covers what was compared and how (the games played; in the second phase the screens, dialogs, sound, focus and the
  web build). It lists every change the pass made: the difference, its cause in the
  original, what the port does now, and the commit. It lists every oddity skipped and why, how to see each one, and the
  deviations that remain. Write it in prose, with a table for the list of changes if that reads better.
- **Order of the work (the user, 2026-11-01):** first every fault the user reported and anything that makes a game
  hard to test, then the identical-play pass. A performance pass (the user, 2026-11-03), then a readability pass
  (`docs/CODE_STANDARDS.md`) and taking out code the ports do not need (registration and the like) come once every game
  is done (`docs/COORDINATION.md`).
- **No references (the user, 2026-10-26):** no port reads another project's decompilation, reimplementation, mod
  kit, class layouts or address maps (Thunderball, haggle, WinFish, CircleShootApp, PvZ-Portable, the Pizza Frenzy
  decompilation and the like); work from our own Ghidra export. PopCap's released SexyAppFramework source, the open
  libraries a game itself uses and descriptions of file formats are fine.
- **Mac versions (the user, 2026-11-01):** PopCap's own Mac builds are not references. Where the user found one, it
  sits in the main checkout's `re/<game>/mac/` (`re/peggle/<edition>/mac/` for the Peggles; git-ignored, so a worktree
  copies the folder in) with a demangled `<name>.symbols.txt` of its function and class names. Its names and class
  layout guide the port, but the PC exe stays the authority for behaviour, since the Mac build may be another
  version. No symbol list goes into the repo; a few names as examples are fine. A game decompiled before its Mac
  version came (BookWorm, Bejeweled, Big Money, Alchemy, Seven Seas, Mummy Maze) also gets a report: its session
  notes the commit it starts from before it first opens the Mac files, logs each thing the Mac build shows the port
  got wrong while it makes its fixes, and once its fixes are done writes `games/<game>/docs/MAC_COMPARISON.md` and
  sends it to the coordinator. The report says how close the decompilation was (game functions found and named
  rightly, class boundaries and inheritance, functions merged, split or put in the wrong class, wrong types or
  constants), what the AI got wrong and which of those had a visible effect, and the lessons for future
  decompilations. Behaviour faults it finds are fixed with the other fixes; naming and structure corrections wait for
  the readability pass.
- **Sonnet sessions** translate loops in their original shape (a test-then-increment loop does not become a for
  loop), check what a comment claims about the original before writing it, and build with AddressSanitizer at the
  first sign of a crash or memory corruption (`docs/COORDINATION.md`, "Model selection").
