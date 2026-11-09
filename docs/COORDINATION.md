# Coordination: the coordinator's rules

The user's rules for planning and running the work across sessions, which only the coordinator session acts on. They
moved here from CLAUDE.md on 2026-11-01, word for word, to keep CLAUDE.md to what every session needs (the user:
CLAUDE.md was "larger than I typically like"). CLAUDE.md keeps the rules every session follows and points here.

## Usage

- **Weekly cap (the user, 2026-10-26; 2026-11-03):** when the account's weekly usage (all models) reaches 97%, the
  coordinator starts no new game session and sends no "Begin" ("I only need 3% usage in reserve for the week", the
  user, 2026-11-03; it was 85%); it checks with `mcp__ccd_session_mgmt__get_usage` before each one.
  The cap is for the identical-play sessions; site work goes ahead past it (the user, 2026-11-04). Game sessions already running carry on and do not watch the usage. The coordinator keeps merging their milestones
  and keeps the local test site ready: `build/web` built from `main`, with every finished game importing and playable.
  Site work runs alongside the game sessions (the user, 2026-11-01).
- **Effort (the user, 2026-10-27, a trial):** game sessions run at medium effort to save usage; the coordinator sets
  it on every session it starts.

## Sessions

- **Directing sessions (the user, 2026-10-24):** the user tells the game sessions about game-specific changes;
  the coordinator does not direct or check on them. A game session messages the coordinator when it needs it:
  a shared change, a milestone ready to merge, or its mods finished. The coordinator
  looks at a session's work only when the user asks, or when it breaks something for other sessions (shared
  code, the trunk, the machine's test rules). It does message the sessions when the way they are instructed
  changes (a new rule like this one), since a running session reads CLAUDE.md again only at its next start
  or compaction.
- **Model selection (the user's rule, decided after the Sonnet trial on 2026-10-25):** a game whose port can
  largely copy another game's (the same framework generation and engine: Noah's Ark's for Seven Seas and Atomica,
  BookWorm's for Pixelus and Typer Shark, Big Money's for Hammer Heads, Bejeweled's for Bejeweled 2, a sequel after
  its first game) runs on Sonnet. A game that needs an engine or framework layer no port has yet (Bookworm
  Adventures' PopLua and PopAnim, the Sprout engines, the non-SAF engines, Chuzzle, Twist, Bejeweled 3, Zuma's
  Revenge) runs on Opus. Opus checks Sonnet where it showed deficits: before merging a Sonnet port, the coordinator
  compares about ten translated functions with the decompilation, weighted to the largest and the most
  loop-heavy, and sends the fixes back. A Sonnet session translates loops in their original shape (a
  test-then-increment loop does not become a for loop), checks what a comment claims about the original before
  writing it, and builds with AddressSanitizer at the first sign of a crash or memory corruption. If a Sonnet
  game's review finds many faults, or its port runs far over its estimate, games of its kind move to Opus.

- **Finished games' sessions are archived (the user, 2026-11-04):** to free the boot drive, the sessions of the games
  that are done and identical were archived and their worktrees removed (about 62 GB), with the three abandoned
  worktrees and the five game sessions that never started (AstroPop, Bejeweled 2, Hammer Heads, Heavy Weapon, Venice).
  Every branch stays. What only those worktrees held is now in the main checkout: each game's Ghidra exports and
  analysis in `re/<game>/` (scratch folders left out), the Seven Seas and Bonnie's baselines in `build/`, and Twist's
  one uncommitted debug change as a patch in `build/worktree-leftovers/`. A later pass on one of those games (the
  performance and readability passes) starts a fresh session from `main`, which copies its `re/<game>/` in from the
  main checkout. So does each of the five games when its turn comes.

## Publishing

- **The SAF Extractor/Packer (the user, 2026-11-04):** "The Talismania decomp session should publish a working version
  of the SAF Extractor/Packer. I forked the repo: https://github.com/allisonpelton/Popcap_.SAF_Extractor-Packer". So
  when Talismania's turn comes, its session clones that fork (the user's OK for that download), makes the tool read and
  write `.saf` archives correctly (the format is in `docs/POPCAP_GAMES_SURVEY.md`, "Talismania Deluxe"), keeps its
  author's credit and license, and pushes the working version to the fork. That fork is the one place outside the
  coordinator's snapshots that a session pushes to; byo-pop itself is never pushed by a game session.
- **origrun and the framework as projects of their own (the user, 2026-11-08):** "Note publishing origrun and the
  framework on SDL2 + wasm as a task to tackle at any point after the decomps are complete." Each becomes a repository
  of its own with its own README. origrun (`tools/origrun`, with rundesk) is the harness for running a 2000s Windows game
  invisibly, silently and on a virtual clock, with scripted input, watched values and function coverage. It needs its
  SexyApp-era assumptions said plainly, its fixed paths to the repo and rundesk made options, and its protection
  handling left out, as `.publishignore` does now. The framework (`framework/` and `platform/`) is SexyAppFramework on
  SDL2 natively and in WebAssembly, published under PopCap's framework licence (`framework/LICENSE.txt`) with nothing
  taken from a game. Not before every decomp is complete; any time after that.
- Before each publish the coordinator checks the trunk for game files and game text.

## Which games, in what order

- **Order of work (the user, 2026-10-24; updated 2026-10-25):** the measured one-session experiment (Noah's Ark,
  `docs/TOKEN_ECONOMY.md`) showed an improvement, so up to three decompilation sessions run at once, besides
  Bejeweled's mods session. The order of games is the survey's decision of 2026-10-24 (recorded in
  `docs/POPCAP_GAMES_SURVEY.md` until 2026-11-04, now replaced by the bullets below): the BookWorm series, then
  Bejeweled Twist, then the least predicted effort first. A slot that the next game in that order cannot use yet,
  because it waits on another game's engine, goes to the cheapest remaining game. New game sessions list the mods
  worth making but make none without the user's approval, and no further game gets mods until every PopCap game works
  in the browser (Noah's Ark's mods were the one exception, part of the Sonnet trial).
- **The merged games first (the user, 2026-10-27):** the 18 games merged by 2026-10-27 are not done until they pass
  the identical-play comparison (CLAUDE.md, "Done means identical"); from the weekly reset (2026-11-01) they come
  first, and no new port starts until all 18 are done.
- **Triage and new ports (the user, 2026-11-01):** "If we have to triage, Twist before identical gameplay on existing
  decomps. No new decomps until all existing ones play identically, except ~trivial ones (the other Mahjong game is the
  only one, I'm guessing)." So when usage runs short, Bejeweled Twist's port comes before the identical-play passes.
  The existing decompilations are every game ported so far, Bonnie's Bookstore and Mahjong Escape: Ancient Japan
  included. No new port starts until all of them play identically. The one exception is a near-trivial port: so far
  that means only Mahjong Escape: Ancient China, which is Ancient Japan's code with other resources. Ancient Japan's
  own session ports it once Ancient Japan plays identically, so it copies the finished engine instead of one that is
  still changing (the user, 2026-11-01).
- **The ports still to make (the user, 2026-11-01):** they wait until every current game is fully done, both phases of
  identical play included. Then: "anything easier than Chuzzle can run in parallel, and anything harder has to be done
  in sequence AFTER all easier games have been finished." By the survey's estimates (`docs/POPCAP_GAMES_SURVEY.md`
  section 2, "The plan for the remaining games"), the easier ones, run side by side as before, are Venice, Heavy
  Weapon, Hammer Heads, Bejeweled 2, AstroPop, Typer Shark, and Pizza Frenzy with Iggle Pop and Feeding Frenzy after
  it (4x to 8x). Chuzzle (~8-12x) itself starts the sequence, then in the survey's order Feeding Frenzy 2 (~10x),
  Peggle Deluxe (~12x) and Peggle Nights after it, Bejeweled 3 (~30x), Banana Bugs, Gyromancer, Zuma's Revenge (~35x)
  and Talismania (~12-16x after Feeding Frenzy 2), one session at a time. This moves Typer Shark, which the decision
  of 2026-10-24 put at the end, into the easier group. Typer Shark, AstroPop and Bejeweled 2 go on. The rest start
  after every current game's readability pass, the easier group side by side and the harder ones two at a time after
  it (a game that builds on another's engine still after that game), each taken all the way through (translation, both
  phases of identical play, the finishing items and its readability pass). For most of 2026-11-08 none was to run side
  by side (the user: "In-progress decomps can continue, but for the remaining games, I want to tackle them in full
  one-at-a-time"), so that wasteful token use would show sooner; the user dropped that the same day: "We'll nix the
  one-at-a-time rule then. The idea behind it was it'd allow inefficient token usage to be spotted sooner, overall
  saving time, but now we have a much better idea of procedure". Of the harder ones, which the rule of 2026-11-01 ran
  one session at a time: "When we get to it, see if running two-by-two causes issues. If not, continue with two". So
  the first pair is watched for trouble (two sessions' builds and runs on the machine at once, the usage they take),
  and pairs go on unless it shows.
- **The passes after Twist (the user, 2026-11-03):** the passes waited until Bejeweled Twist was done, 3D mode included
  (the user, 2026-11-01), with Bejeweled Deluxe's the one exception. With Twist finished: "get going on identical play
  for more games. Start with lowest effort." So the first phase runs for every game that has had no pass, up to five
  sessions side by side ("Ok to run up to 5 identical play sessions at once", the user, 2026-11-03), each in the game's
  own session where one fits, from the lowest survey estimate up: Noah's Ark, Mummy Maze, Alchemy, Atomica and Big
  Money first (started 2026-11-03; Big Money in a new session), then Cosmic Bugs (on Water Bugs' engine, whose pass is
  done), TipTop and Ning Po, Pixelus, BookWorm, Word Harmony, Rocket Mania, Dynomite and Bookworm Adventures Volume 2.
  A session that finishes its phase frees its slot for the next game.
- **The second phases (2026-11-03):** with Bookworm Adventures Volume 2's merge (07c6bdb) every ported game plays
  identical, so the second phase runs for the fourteen games whose pass had only the first, in the same order and in
  the same sessions, up to five at once: Noah's Ark, Mummy Maze, Alchemy, Atomica and Big Money first (started
  2026-11-03, at 90% of the week), then Cosmic Bugs, TipTop, Ning Po, Pixelus, BookWorm, Word Harmony, Rocket Mania,
  Dynomite and Bookworm Adventures Volume 2. Bejeweled, Twist, Bonnie's, Bookworm Adventures, Water Bugs, Seven Seas and
  the two Mahjong Escapes did both phases in their passes. Ning Po's needs origrun's stand-in to stop stalling under
  `--sound stub` with `--clock virtual` (after the title music); its own session, which found the stall, fixes it in a
  `shared:` commit. The ports still to make (above) wait until these are done. A phase two merges only with sound
  compared against the original and the web build's pictures compared, not just its log (Mummy Maze's and Atomica's
  first reports lacked them, and Mummy Maze's turned up two real faults once done). Sound effects: origrun's
  `--sound wav` log on the host against the port's BYOPOP_SOUNDLOG, as Alchemy did. Music a library mixes into one
  stream: that library's calls, logged by a stand-in (games/bigmoney/tools/bassproxy for BASS,
  games/mummymaze/tools/fmodproxy for FMOD 3). Pictures: shots from the web build under Node, shotdiffed against
  native. Each brief says so. By 2026-11-04 seven were merged (Big Money, Alchemy, Mummy Maze, Cosmic Bugs, Atomica,
  TipTop, Noah's Ark), and the week stood at 96%; Ning Po, Pixelus, BookWorm, Word Harmony, Rocket Mania, Dynomite and
  Bookworm Adventures Volume 2 start after the weekly reset (2026-11-08 00:00 UTC), five at a time again. That week
  also makes each game's fixes mod (the user, 2026-11-04: "next week will have a bunch of that in addition to
  finishing phase 2 of identical play"; `docs/MOD_ROADMAP.md`, "The fixes mod").
- **Twelve at work, flat out, and three early ports (the user, 2026-11-07):** "No limit on concurrent sessions while
  getting all current games to their finished state", run flat out to the 97% cap ("Flat out to the cap"), until 21
  sessions at work filled the machine and two crashed (exit 0xC0000409, with 2.6 GB of 32 GB free, 60 of 67 GB
  committed and every core busy). Then: "Too many sessions, some crashed. Set a comfortable limit to avoid this
  happening again." The limit is 12 sessions at work besides the coordinator, where at work means in a turn or waiting
  on a run of its own. Each session's own process holds about 0.6 GB even while idle, and its builds, decompiles, games
  and sandboxes come on top in bursts. Before waking one, the coordinator also checks the machine: under 6 GB free, or
  over 80% of the commit limit, means waiting even below 12. A session past the limit waits idle with nothing running,
  on a background wait for its own file (`until [ -f <main checkout>/build/slots/<name>.go ]; do sleep 60; done; cat`
  the file). When a slot frees, the coordinator writes that file with any news for the session. Such a wake costs no
  message (the app's limit of 10 since the user last typed). A crashed session comes back only into a free slot. And
  "some of the remaining decomps are simple enough to run concurrently": Typer Shark, AstroPop and Bejeweled 2 start
  now, beside the finishing work, an exception to "no new port until all are done". The rest of the easier group waits
  as before.
  JPEG: IJG libjpeg, the library the games use, replaces stb_image for JPEG images ("Add IJG libjpeg"; the download of
  jpegsrc.v6b.tar.gz from ijg.org approved), so no game lists a JPEG deviation.
- **Reports still open from the user's test notes (cleared 2026-11-04):** each goes to its game's session with its
  next brief. Bookworm Adventures: "Sound effects seem to come about a second after they should." Its pass found every
  sound at the original's update natively, so check the web build, where the user plays. Its smashed tile "looks
  wrong" (`testnotes/images/bookwormadv-smashed-tile.png`): the countdown that ToColor hid is fixed (d63c8e5b), but
  the tile was never compared with that screenshot. Every game: "In fullscreen the original freezes while it does not
  have the focus; the port keeps running" (`docs/FRAMEWORK_2004.md` on the focus): each phase two's focus check covers
  it, natively in full screen. Cosmic Bugs' yellow flicker and offset digits at a second launch were not reproduced
  (its IDENTICAL_PLAY.md says how it was looked for). Bejeweled Twist (the user, 2026-11-05, once it was on the site):
  "I can't twist the gems on the web version (stuck in the tutorial)! And the native version doesn't run either."
  Natively it showed a black or blank screen with ScreenMode=1 (full screen) in its registry.ini, though it ran about
  3 minutes and closed cleanly; a run under rundesk with a fresh user folder started fine (GL ES 3.0, GPUDisplay).
  Twist's session fixes both at the start of the week from 2026-11-08, before its fixes mod.
- **The user's test notes of 2026-11-07, routed the same day:** each session has the user's words in its brief and
  writes in its notes whatever it cannot fix. Bejeweled Twist: in the browser the cursor does not follow the mouse and
  no gem can be twisted in Classic's tutorial, the resolution cannot be changed, quitting from the pause menu makes
  the menu vanish instead of shrinking, the board reappears on quit, and the best-move replay lacks parts of the score
  meter; natively Classic shows a black screen (full screen), Zen's background is missing, and the patch on the user's
  Steam original misaligns the cursor and pillarboxes the window. Seven Seas: clicks fail on some squares (column 4,
  rows 1 to 3), and the tutorial text is in the wrong place. Bookworm Adventures and Volume 2: every minigame ends in
  a Lua error ("Expected boolean in LuaClassRet") with no restart, and the sound stops after a focus loss just after a
  chapter boss. Volume 2 also lags while the curtains move, lacks Word Up's submit button natively, and its rainbow
  tiles never form a word. Bonnie's Bookstore: the site's player name is not filled in, and the music is missing or
  far too quiet. Shared, in a session of its own: late sound in every web build, a minimum native window size of the
  game's resolution, and borderless full screen in every native port by default. The site: Twist's cursors listed
  apart from the others, Ning Po's volume slider dragging the window, chat with game icons that can sit right of the
  game, keep its collapser's size and fill the space below the game, a true footer with a one-sentence copyright
  notice, a pixel-perfect option, explicit state machines, and, last, ideas for original selectable skins in the
  spirit of PopCap and the web around 2010.
- **Plays identical first, for every game (the user, 2026-11-01):** identical play is two phases, "plays identical"
  and "looks identical" (CLAUDE.md). "Current games can still do both, but for remaining ones to review get all of them
  playing identical first." So the passes running on 2026-11-01 (Bejeweled, Bonnie's, Bookworm Adventures, and Twist's
  when it comes) do both phases. Every game not yet reviewed gets the first phase, and their second phases start only
  once every game plays identical. "If a series of games play out the same in the original and port then the first
  phase should be considered done."
- **Order of the work on the merged games (the user, 2026-11-01):** "once all issues I mentioned are fixed, move on
  down through the game list. Once all games are done, then we can do code quality/readability, and finally figure out
  what code does not need to be present in the decomps (mostly registration, but other things are likely to turn up as
  well)." So first every fault the user reported, in every game, and anything that makes a game hard to test; then the
  identical-play pass above, game by game; then a readability pass to the project's code standards
  (`docs/CODE_STANDARDS.md`, written on one game and approved by the user first), moved up on 2026-11-08 to come
  before any further decomp (below, "The readability pass first"); then, game by game since 2026-11-08 (below, "Done
  means every phase but modding"), the coverage check and a performance pass (both below); last, taking out the other code the ports do not need. Registration code (trial, nag and unlock
  paths: "It is misleading and adds unnecessary bloat") moved up (the user, 2026-11-07: "Move up the removal of all
  registration-related code to be part of the current phase before beginning other decomps"). Each game takes it out
  before its final merge, as part of being finished, and no further decomp begins until every current game has; a
  port not yet translated (Typer Shark, AstroPop and Bejeweled 2 among them) leaves it out as it translates. The port
  plays as the registered original does, which stays the reference for every comparison. What goes is the code only an
  unregistered or trial copy reaches: the trial's clocks and counters, nags, the register, unlock and buy dialogs and
  their code checks, expiry, and the strings, images and registry values only those use. A button or menu item that
  exists only for them goes too, its neighbours closing the gap as below. Each removal is listed in the game's notes,
  with a word on any visible change, and the frozen baseline proves play unchanged (how a protection's own checks were
  taken apart stays in private blocks). Every button that leads out of a game comes out in the same phase (the
  user, 2026-11-07, asked whether it should come out now too: "Yes, now too"), "More Games" first (asked earlier that
  day whether it might open the site's listing: "Remove the button entirely, as requested in the past. Other buttons
  can be resized to avoid the appearance of a gap"), with the buttons beside it widened or moved to close the gap, and
  the code and images only it used go with it. The game's regression check proves each step leaves play unchanged. All of it comes before the website is ready. So does one more step (the
  user, 2026-11-01; since 2026-11-08 part of each game's own finishing): each game ported only from its Steam copy is compared with its version on the PopCap Game
  Collection disc (the user's ISO in their Downloads folder), and its build must work with that version's files too.
  Planned alongside the website (the user, 2026-11-01): an installer that puts a native port beside its Steam
  original, started by a launch option, SMAPI-style (`docs/STEAM_LAUNCH.md`).
<!-- private -->
- **Building the web port in the browser (the user's test notes, moved here 2026-11-04):** "I'm starting to feel more
  open to trying to build the WASM version in the browser (either saving it to localStorage or to a connected Google
  Drive) to completely avoid infringement. Let's try it on the simplest possible game and see how long it takes to
  build in the browser and how much work it adds for each game." Not started; it belongs with the work before the
  website is ready.
<!-- /private -->
- **The readability pass first (the user, 2026-11-08):** "Move up readability passes to come before additional
  decomps. In-progress decomps can continue, but for the remaining games, I want to tackle them in full
  one-at-a-time. Part of the readability pass: NO unknown/unnamed variables. Use one game to set the full standards
  for what constitutes readable and see that they actually make sense." (The one-at-a-time part was dropped the same
  day: "Which games, in what order".) Bejeweled Deluxe is the pilot: the first
  port, with the strongest proof that a change leaves play alone (its oracle against the original, beside its frozen
  baseline), and its Mac build's symbols for names. Its session writes `docs/CODE_STANDARDS.md` while applying it to
  the whole port, changes a rule wherever it fails in practice, and builds a checker in `tools/` that finds unnamed
  identifiers, so every later pass proves "no unknowns" by a run instead of by eye. The user found the first draft
  good to start (2026-11-08) and wants it tried on a more complex game before every game takes it: Bejeweled Twist,
  the largest port (about twice Bejeweled's code, both 2D and 3D, and no Mac build, so its names come from use), takes
  its pass second, once its current fixes are merged, and the standards change wherever they fail there. The user
  approves them after that; then each finished game takes its pass in its own session, a readable port changing nothing it does (its frozen
  baseline, and the oracle for Bejeweled), and keeping each function's address so the coverage check can still pair
  it with the export. The coverage check and the performance pass now come game by game (below, "Done means every
  phase but modding"); each finished game already records its performance numbers.
- **A coverage check after the identical-play passes (the user, 2026-11-02: "Queue it").** The user asked whether
  reccmp, the matching-decompilation tool, would help. It does not fit: it scores, function by function, how closely
  code rebuilt with the original compiler reproduces the exe's instructions, and our ports are built by clang and
  Emscripten on a portable framework, with SDL in place of DirectX and the leaks fixed, so its scores would be noise.
  What it would catch that our comparisons miss is a slip in code no comparison run reaches: options, rare branches,
  error paths. So each game gets a coverage check once it plays identical: which of the original's functions its
  comparison runs reach (origrun would record it), and for each function in our Ghidra export, whether the port has its
  counterpart, named by its address (today the ports name few: Bejeweled 154 distinct addresses, Twist 204). A function
  the runs never reach is read against the export or given a run that reaches it. The tool came on 2026-11-08
  (`docs/COVERAGE.md`: origrun's `--coverage` and `tools/coverage.py`). Its pilot, a minute of Bejeweled's original,
  recorded 1739 of the exe's 5900 functions and 60 of its 152 game functions, and found one the port has no counterpart
  for; one short run proves the tool, not the method, so Ning Po takes the first whole check (every run its comparisons
  make and short runs for what they miss, each gap judged, the cost noted), and the other games take theirs after it, at
  their place in each game's order (below). Ning Po's found no missing logic: of 272 functions reached with no
  counterpart, 32 were the tool's (fixed), 54 had their addresses placed and 185 became `none` lines, in about 15
  minutes of machine time (`docs/COVERAGE.md`, "A game's whole check"). The same evening (2026-11-08) Noah's Ark, Mummy
  Maze, Bonnie's, the two Mahjong Escapes, Alchemy, Atomica, Big Money and Water Bugs took theirs; Cosmic Bugs, TipTop,
  Pixelus, BookWorm, Word Harmony, Rocket Mania, Dynomite, Seven Seas and the two Bookworm Adventures follow as slots
  free, and Bejeweled and Twist take theirs after their readability passes.
- **A performance pass before the readability pass (the user, 2026-11-03):** "Slot in performance passes for the games
  before readability passes. There's got to be a few things to speed up in most games." It comes after the coverage
  check. First each original and its native port are measured over the same scripted play, on a quiet machine (no
  passes running): the original through origrun under rundesk, the port as Bejeweled's was
  (`games/bejeweled/docs/PROFILE.md`, `tools/profile`: working set, private bytes, processor time and where the time
  goes), and the web build's memory and frame time in a browser. Then each game's session speeds up what the profile
  shows, natively and on the web. A speed-up changes how the port works, never what it does: the same draws, the same
  random numbers, the same screens, proved by the game's frozen baseline. One that changes what the player sees is a
  mod: a toggle in the game's fixes mod, on by default on the site, when it keeps the original's look and only stops
  wasting time (TipTop's loading bar), and otherwise a mod of its own that starts off, like Bejeweled's `lean`
  with its fewer frames (`docs/MOD_ROADMAP.md`, "The fixes mod"). No game's code is made multithreaded
  (CLAUDE.md, "Uncapped, one core each"). Shared costs (the framework's drawing, the mixer, image loading) are found
  once and fixed in the framework for every game. Each game's notes record its numbers before and after. A session
  need not wait for the pass to speed up its own port (the user, 2026-11-08: "Sessions should be allowed to improve
  performance early on so they can test with -fast and run even faster"): a speed-up that makes its `-fast` runs
  quicker may come at any time, under these rules, and is listed in its notes with the numbers.
  Shared costs found so far, waiting to be fixed once for every game: SDL_RenderPresent takes 69% of Alchemy's main
  thread, a driver cost per presented frame under every renderer (Alchemy, 2026-11-09). Its frames almost never repeat
  (4 of 2,406 identical), so only a frame cap would cut it, and that is a visible change, a mod; the one shared fix
  proposed is uploading only a redrawn MemoryImage's changed rectangle (SDL_UpdateTexture, 7% of the main thread;
  games/alchemy/docs/GAME_NOTES.md section 8). SexyApp::Init's window and
  graphics device take 250–310 ms of Noah's Ark's 0.6 s start (Noah's Ark, 2026-11-09). Creating the device on
  another thread is ruled out (no threads); showing the window before the device is made only moves the wait.
  Bejeweled's profiles agree (2026-11-09): a -fast game spends 85-90% of its cycles presenting, in the driver and
  making the device, imagehlp's 5% among them (wintrust loads it for the driver's signature check when the device is
  made); the framework also keeps every image's pixels after upload, 19 MB of Bejeweled's 55 MB (the original: 36 MB).
- **Done means every phase but modding (the user, 2026-11-08):** "I've decided I want to go through every phase
  except modding before a game can be considered done and ready for the live site, and just publish whatever games are
  ready by the end of December. I'm in no hurry to get games out in a less-than-stellar state." A game is done only once
  it has had every step: the user's faults fixed; both phases of identical play, with the real-input play of CLAUDE.md
  ("Done means identical"); registration and the buttons that lead out of the game removed; its fixes mod; the
  readability pass, once the user approves the standards after Twist's pilot; the coverage check; the performance pass;
  the other code it does not need taken out; and, for a game ported from its Steam copy, the check against its PopCap
  Game Collection version. The coverage check and the performance pass, planned for after every game was done, become
  steps of each game's own finishing, in the order above where a game has not begun its readability pass (coverage,
  then performance, then readability, as the user ordered on 2026-11-03), and after it for the pilots. The coordinator
  builds the coverage tool first. A performance measurement needs a quiet machine, so the coordinator gives each game a
  measuring slot when few game processes run. Modding is what waits: new mods and the changes listed in
  `docs/MOD_ROADMAP.md`; the fixes mod is not modding here, since the site serves every game with it on. On the last
  day of December 2026 the games that are done go on the live site (the site going public is still the user's word),
  and the rest follow when each is done.
- **The site's requested work comes between Twist and the passes (the user, 2026-11-02):** "Slot in the remaining
  requested additions and changes to the site after finishing Twist and before the passes on existing decomps." Then:
  "Any site features that are easy, you can do now. Anything that requires designing an animation or SVG, or that
  requires a lot of code to get working right, save for later." Every item the user listed for the site was done
  by 2026-11-04. Done now, in a site session: F11 for full screen in every game; play time, last day played
  and number of plays on the front page, with "Play now" and "Add your files"; a bigger logo on the front page;
  "Quit Game" back to the home page; the player, save and settings managers as tabs with far less text (names until the
  icons are designed). After Twist and before any identical-play pass starts on a game decompiled before Twist (passes
  already running carry on): adding many games' files at once with the game inferred from the folder; a game folder
  dropped on the logo or on a game's tile; bottle-cap placeholder icons; the bottle cap and opener animation; the recycle
  bin; the managers' tab icons. They go from easiest to hardest, and all but the animation start now, beside Twist (the
user, 2026-11-03: "Tackle the site items now, actually. I decided they won't be too hard to at least do a rough version
of. Just save the animation for after Twist"): a game folder dropped on its own tile; many games' folders at once, each
game inferred from its folder, a choice where that is unclear and the rejected folders named; a folder dropped on the
logo, using that inference; the managers' tab icons; the bottle-cap placeholder icons; the recycle bin, since it takes
a dragged cap. The bottle cap and opener animation comes after Twist's last merge. Ning Po Mahjong's and TipTop's high-score names
were fixed (34e6da1, a6547b9). The chat
  is built as a test with made-up messages; its real version waits for funding. What the user asks for while testing is
  still done when asked. Desktop 95's pixelated text waits for a pixel font drawn for the project (the user, 2026-11-08: "Draw our own
  later"): every Windows install still has the bitmap MS Sans Serif (`sserife.fon`), but browsers draw only TrueType and
  OpenType, so the skin's "MS Sans Serif" comes out as the smooth Microsoft Sans Serif, and Microsoft's font itself is
  never shipped. Desktop 95 is the site's only look (the user, 2026-11-09: "I've basically completely lost interest in
  the skins that aren't Windows 95", and later that day: "drop the alt skins, including the plain one"): Demo Disc, Soda
  Fountain, Plain, the skin choice and the dark theme are taken out, and new site work is built and checked in Desktop 95
  alone. An XP look, and perhaps Vista's, waits until the site works as the user wants it (the user,
  2026-11-09: "it would be once all the site functionality is exactly as I want it"): each a second style of the same
  desktop, as XP's own Display settings offered the XP style or the Classic one for the same windows. The desktop's
  behaviour is first parted from the 95 look (the code tests for "d95" where it means the desktop), then each style
  is its own paint, controls and icons of our own drawing; Vista's glass is measured against the frame time of games in
  other windows. The coordinator's guess on 2026-11-09: $30-60 for the parting, $100-150 for XP, $150-200 for Vista.
- **Which games, and where they are published (the user, 2026-10-26):** every game may be decompiled except Plants vs.
  Zombies, Insaniquarium and Zuma Deluxe, whose fan communities we leave to their work. A game with another PC decompilation in
  progress or an active modding community is built but unpublished: its folder goes into `.publishignore` and it stays
  off the website. The "Published" column of `docs/POPCAP_GAMES_SURVEY.md` section 2 ("The plan for the remaining
  games") says which; "web only" (Zuma's Revenge, the user 2026-10-26) keeps the folder in `.publishignore` but puts
  the game on the website.

## Mods

- **A goal for Twist when modding resumes (the user, 2026-11-03):** "a goal for Twist: produce a TAS that scores >5.1
  million in Blitz mode and save a replay of it that can be recorded and uploaded. This is likely to require a lot of
  strategizing." It waits, like every mod, until the decompilations are finished.

- **No new mods unless requested (the user, 2026-10-30):** "no new mods for anything unless someone requests it."
  There is no mod phase that goes through a list: the 29 standard mods in `docs/MOD_ROADMAP.md` and each game's listed
  ideas become the first entries of a feedback page the website will have for each game, where players suggest mods
  and vote on them, and the user decides which to make ("Implementation of the mod could even be automated based on a
  comment getting enough upvotes (and my approval, of course)."). Fixes and suggestions for the mods that already
  exist are noted in `docs/MOD_ROADMAP.md` ("The user's notes on the mods that exist") and wait until every
  decompilation is finished.
- **The fixes mod (the user, 2026-11-04):** the site will serve each game with one mod of fixes on by default, each
  fix a toggle in it, and the player can turn single fixes or the whole mod off for the original as it was
  (`docs/MOD_ROADMAP.md`, "The fixes mod"). With it on, a game plays the same on every run (no 15-update jumps as in
  Twist's original); memory leaks stay fixed in the port itself. A game is done only once its fixes mod is made, so
  the week from 2026-11-08 makes them beside the second phases; until a game's mod is made, a session that finds a fix
  lists it in its game's notes under "Fix candidates".

## History

- **The review page (the user, 2026-11-01):** the pilot on Bookworm Adventures settled who judges shotdiff's flagged
  differences ("Basically everything appears to be a timing difference, so this tool still isn't usable"; then "Yes,
  switch"): the session judges every one itself (CLAUDE.md, "Done means identical").
