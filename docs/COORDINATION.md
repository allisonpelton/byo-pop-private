# Coordination: the coordinator's rules

The user's rules for planning and running the work across sessions, which only the coordinator session acts on.
CLAUDE.md keeps the rules every session follows and points here (the user, 2026-11-01: CLAUDE.md was "larger than I
typically like").

## Usage

- **Weekly cap, a hard limit (the user, 2026-10-26; 2026-11-03; 2026-11-10):** when the account's weekly usage reaches
  97% (all models, or the window of the model the sessions run on), work stops there: "I only need 3% usage in reserve
  for the week" (2026-11-03). The coordinator starts no session, sends no "Begin" and sends no session on to a further
  phase or step; it tells every session at work to commit what it has and end its turn, stops the turn of any that goes
  on, and merges nothing that needs a suite run. The README refresh the user asked for at 97% is the one thing it still
  does. It checks `mcp__ccd_session_mgmt__get_usage` hourly while sessions work (a cron job), warns the sessions at
  95% to reach a commit, and checks again before every message that sends a session on. Merging and sending idle
  sessions on after that refresh used the reserve once: "You ran up the usage all the way to 100% despite my 97% limit"
  (the user, 2026-11-10).
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
- **Fable only, an experiment (the user, 2026-11-10):** "An experiment this week: ONLY use the new Fable 5.2. Use it
  for everything so I can explore its capabilities and make full use of the banked reset." Every session, the
  coordinator included, and every agent a session starts runs on Fable 5.2 (`claude-fable-5-2`; `model: "fable"` or
  no model for an Agent), and the model selection above is suspended until the user ends the experiment. The user
  wants to hear how productive Fable was compared with Opus when its weekly window is close to running out: the
  baseline and the measure are in the coordinator's memory (`fable-experiment`).
- **Finished games' sessions are archived (the user, 2026-11-04):** to free the boot drive, the session of a game that
  is done and identical is archived and its worktree removed. Every branch stays, and what only the worktree held goes
  into the main checkout first: the game's Ghidra exports and analysis in `re/<game>/` (scratch folders left out) and
  its baselines in `build/`. A later pass on such a game starts a fresh session from `main`, which copies its
  `re/<game>/` in from the main checkout.

## Publishing

- **The SAF Extractor/Packer (the user, 2026-11-04):** "The Talismania decomp session should publish a working version
  of the SAF Extractor/Packer. I forked the repo: https://github.com/allisonpelton/Popcap_.SAF_Extractor-Packer". So
  when Talismania's turn comes, its session clones that fork (the user's OK for that download), makes the tool read and
  write `.saf` archives correctly (the format is in `docs/POPCAP_GAMES_SURVEY.md`, "Talismania Deluxe"), keeps its
  author's credit and license, and pushes the working version to the fork. That fork is the one place outside the
  coordinator's snapshots that a session pushes to; byo-pop itself is never pushed by a game session.
- **origrun and the framework as projects of their own (the user, 2026-11-08):** "Note publishing origrun and the
  framework on SDL2 + wasm as a task to tackle at any point after the decomps are complete." Each becomes a repository
  of its own with its own README. origrun (`tools/origrun`, with rundesk) needs its SexyApp-era assumptions said
  plainly, its fixed paths to the repo and rundesk made options, and its protection handling left out, as
  `.publishignore` does now. The framework (`framework/` and `platform/`) is published under PopCap's framework licence
  (`framework/LICENSE.txt`) with nothing taken from a game. Not before every decomp is complete; any time after that.
- Before each publish the coordinator checks the trunk for game files and game text.

## Which games, in what order

- **New ports wait (the user, 2026-11-01):** "If we have to triage, Twist before identical gameplay on existing
  decomps. No new decomps until all existing ones play identically, except ~trivial ones (the other Mahjong game is the
  only one, I'm guessing)." The existing decompilations are every game ported so far. Twist's port and Ancient China's
  are made; the bar for the rest has since risen to every current game's readability pass ("The readability pass
  first").
- **The ports still to make (the user, 2026-11-01):** "anything easier than Chuzzle can run in parallel, and anything
  harder has to be done in sequence AFTER all easier games have been finished." By the survey's estimates
  (`docs/POPCAP_GAMES_SURVEY.md` section 2, "The plan for the remaining games"), the easier ones still to start are
  Venice, Heavy Weapon, Hammer Heads, and Pizza Frenzy with Iggle Pop and Feeding Frenzy after it (4x to 8x). Chuzzle
  (~8-12x) starts the harder ones, then in the survey's order Feeding Frenzy 2 (~10x), Peggle Deluxe (~12x) and Peggle
  Nights after it, Bejeweled 3 (~30x), Banana Bugs, Gyromancer, Zuma's Revenge (~35x) and Talismania (~12-16x after
  Feeding Frenzy 2). They start after every current game's readability pass, the easier group side by side and the
  harder ones two at a time after it (a game that builds on another's engine still after that game), each taken all the
  way through (translation, both phases of identical play, the finishing items and its readability pass). Side by side
  stands because the user dropped a one-at-a-time rule the day it was made (2026-11-08): "We'll nix the one-at-a-time
  rule then. The idea behind it was it'd allow inefficient token usage to be spotted sooner, overall saving time, but
  now we have a much better idea of procedure". Of the harder ones: "When we get to it, see if running two-by-two
  causes issues. If not, continue with two". So the first pair is watched for trouble (two sessions' builds and runs on
  the machine at once, the usage they take), and pairs go on unless it shows. A slot that the next game in the order
  cannot use yet, because it waits on another game's engine, goes to the cheapest remaining game.
- **Passes from the lowest effort up (the user, 2026-11-03):** "get going on identical play for more games. Start with
  lowest effort." A pass runs in the game's own session where one fits, from the lowest survey estimate up, and a
  session that finishes its phase frees its slot for the next game.
- **What a phase two needs to merge (2026-11-03):** sound compared against the original, and the web build's pictures
  compared, not just its log. Sound effects: origrun's `--sound wav` log on the host against the port's
  BYOPOP_SOUNDLOG, as Alchemy did. Music a library mixes into one stream: that library's calls, logged by a stand-in
  (games/bigmoney/tools/bassproxy for BASS, games/mummymaze/tools/fmodproxy for FMOD 3). Pictures: shots from the web
  build under Node, shotdiffed against native. Each brief says so.
- **Twelve at work, flat out, and three early ports (the user, 2026-11-07):** "No limit on concurrent sessions while
  getting all current games to their finished state", run flat out to the 97% cap ("Flat out to the cap"). After 21
  sessions at work filled the machine and two crashed: "Too many sessions, some crashed. Set a comfortable limit to
  avoid this happening again." The limit is 12 sessions at work besides the coordinator, where at work means in a turn
  or waiting on a run of its own. Each session's own process holds about 0.6 GB even while idle, and its builds,
  decompiles, games and sandboxes come on top in bursts. Before waking one, the coordinator also checks the machine:
  under 6 GB free, or over 80% of the commit limit, means waiting even below 12. A session past the limit waits idle
  with nothing running, on a background wait for its own file (`until [ -f <main checkout>/build/slots/<name>.go ]; do
  sleep 60; done; cat` the file). When a slot frees, the coordinator writes that file with any news for the session.
  Such a wake costs no message (the app's limit of 10 since the user last typed). A crashed session comes back only
  into a free slot. And "some of the remaining decomps are simple enough to run concurrently": Typer Shark, AstroPop
  and Bejeweled 2 started beside the finishing work, an exception to "no new port until all are done". The rest of the
  easier group waits as before.
- **After every game is done: gamepads (the user, 2026-11-10):** "Once all games are done, I want to add native gamepad
  support to every game. I also want to support the Wii Remote in particular." Not before; it is shared framework work
  with a layout per game.
- **The README's performance table (the user, 2026-11-10):** "Include in the readme a performance comparison table
  including load time, CPU, and memory usage for every game", port against original, from each game's performance
  numbers in its notes.
- **A shared cost found by Atomica's performance pass (2026-11-11):** `DDInterface::FillScanLines` hands every row to
  SDL as its own rectangle. Atomica's tunnel sends about 21,000 a frame, which costs about 58 MB of private bytes in
  the driver (under every renderer) and over a quarter of the game's processor time. Joining rows that touch could
  halve them but may round a right edge differently at some window scales, so no fix is chosen yet; to test one,
  `games/atomica/tools/profile/pairs.sh` for the figures and a screenshot every 40 updates of its scenarios s2, a1 and
  g5 before and after for the pixels.
- **A shared cost found by Twist's performance pass (2026-11-11):** in 2D each level-up's jump raises the port's
  private memory by about 165 MB for about ten seconds (240 MB to 405 MB, the original flat at 185 MB). It is not the
  game's heap (22 MB in blocks of 64 KB and more) and belongs to no module, so it is allocated beneath the game while
  the jump's software-drawn frames are uploaded. To find it: hook malloc with a --wrap link, or count texture
  creations in the display layer during a jump; `bejeweledtwist.exe -nod3d9 -fast -autoplay=classic -seed=1234` under
  its perfmin.py reaches the jump at about update 4000 to 5700.
- **Memory broken down, and a software path to test (the user, 2026-11-11):** "I'd want the more granular performance
  reports then": every game's memory split into libraries, sound, graphics and game state, for the port and where it can
  be read for the original, in the README's performance table. The framework answers a standard `memreport` test command
  (pictures, sounds and music by their bytes, the process's totals, and what it held before reading a game file), and a
  tool walks a process's memory regions (libraries, heap, mapped files), which works on an original too. "Software path
  sounds interesting for a test, at least": measure a port drawing through SDL's software renderer with no Direct3D
  device against the default (memory before any file, at the title and in play; processor time; frame time at the
  game's size and full screen at 4K; the same pictures). A test only: no default changes without the user.
- **Reports from the user's test notes:** each goes to its game's session with its next brief, in the user's words,
  and the session writes in its notes whatever it cannot fix. From the notes cleared on 2026-11-04: Bookworm
  Adventures, "Sound effects seem to come about a second after they should" (its pass found every sound at the
  original's update natively, so check the web build, where the user plays), and its smashed tile "looks wrong"
  (`testnotes/images/bookwormadv-smashed-tile.png`; never compared with that screenshot); every game, "In fullscreen
  the original freezes while it does not have the focus; the port keeps running" (`docs/FRAMEWORK_2004.md` on the
  focus; each phase two's focus check covers it, natively in full screen); Cosmic Bugs' yellow flicker and offset
  digits at a second launch were not reproduced (its IDENTICAL_PLAY.md says how it was looked for). From the notes of
  2026-11-07: Bejeweled Twist, the resolution cannot be changed, quitting from the pause menu makes the menu vanish
  instead of shrinking, the board reappears on quit, natively Classic shows a black screen (full screen), Zen's
  background is missing, and the patch on the user's Steam original misaligns the cursor and pillarboxes the window;
  Bookworm Adventures and Volume 2, every minigame ends in a Lua error ("Expected boolean in LuaClassRet")
  with no restart, and the sound stops after a focus loss just after a chapter boss; Volume 2 also lags while the
  curtains move, lacks Word Up's submit button natively, and its rainbow tiles never form a word; Bonnie's Bookstore,
  the site's player name is not filled in, and the music is missing or far too quiet; shared, late sound in every web
  build; the site, Twist's cursors listed apart from the others, Ning Po's volume slider dragging the window, chat with
  game icons that can sit right of the game, keep its collapser's size and fill the space below the game, a true footer
  with a one-sentence copyright notice, a pixel-perfect option, explicit state machines, and, last, ideas for original
  selectable skins in the spirit of PopCap and the web around 2010.
- **Plays identical first, for every game (the user, 2026-11-01):** identical play is two phases, "plays identical"
  and "looks identical" (CLAUDE.md). "Current games can still do both, but for remaining ones to review get all of them
  playing identical first." Every game not yet reviewed gets the first phase, and second phases start only once every
  game plays identical. "If a series of games play out the same in the original and port then the first phase should be
  considered done."
- **Order of the work on the merged games (the user, 2026-11-01):** "once all issues I mentioned are fixed, move on
  down through the game list. Once all games are done, then we can do code quality/readability, and finally figure out
  what code does not need to be present in the decomps (mostly registration, but other things are likely to turn up as
  well)." So first every fault the user reported, in every game, and anything that makes a game hard to test; then the
  identical-play pass above, game by game; then a readability pass to the project's code standards
  (`docs/CODE_STANDARDS.md`), which comes before any further decomp (below, "The readability pass first"); then, game
  by game (below, "Done means every phase but modding"), the coverage check and a performance pass (both below); last,
  taking out the other code the ports do not need. Registration code (trial, nag and unlock paths: "It is misleading
  and adds unnecessary bloat") moved up (the user, 2026-11-07: "Move up the removal of all registration-related code to
  be part of the current phase before beginning other decomps"). Each game takes it out before its final merge, as part
  of being finished, and no further decomp begins until every current game has; a port not yet translated leaves it out
  as it translates. The port plays as the registered original does, which stays the reference for every comparison.
  What goes is the code only an unregistered or trial copy reaches: the trial's clocks and counters, nags, the
  register, unlock and buy dialogs and their code checks, expiry, and the strings, images and registry values only
  those use. A button or menu item that exists only for them goes too, its neighbours closing the gap as below. Each
  removal is listed in the game's notes, with a word on any visible change, and the frozen baseline proves play
  unchanged (how a protection's own checks were taken apart stays in private blocks). Every button that leads out of a
  game comes out in the same phase (the user, 2026-11-07, asked whether it should come out now too: "Yes, now too"),
  "More Games" first (asked earlier that day whether it might open the site's listing: "Remove the button entirely, as
  requested in the past. Other buttons can be resized to avoid the appearance of a gap"), with the buttons beside it
  widened or moved to close the gap, and the code and images only it used go with it. The game's regression check
  proves each step leaves play unchanged. All of it comes before the website is ready. So does one more step (the user,
  2026-11-01; since 2026-11-08 part of each game's own finishing): each game ported only from its Steam copy is
  compared with its version on the PopCap Game Collection disc (the user's ISO in their Downloads folder), and its
  build must work with that version's files too. Planned alongside the website (the user, 2026-11-01): an installer
  that puts a native port beside its Steam original, started by a launch option, SMAPI-style (`docs/STEAM_LAUNCH.md`).
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
  day: "Which games, in what order".) Bejeweled Deluxe set the standards (`docs/CODE_STANDARDS.md`, with a checker in
  `tools/` that finds unnamed identifiers, so a pass proves "no unknowns" by a run instead of by eye), Bejeweled Twist,
  the largest port, tested them on a more complex game, and the user approved them on 2026-11-10 ("new standards
  approved"). Each finished game takes its pass in its own session, a readable port changing nothing it does (its
  frozen baseline, and the oracle for Bejeweled), and keeping each function's address so the coverage check can still
  pair it with the export.
- **A coverage check after the identical-play passes (the user, 2026-11-02: "Queue it").** What our comparisons miss
  is a slip in code no comparison run reaches: options, rare branches, error paths. So each game gets a coverage check
  once it plays identical: which of the original's functions its comparison runs reach, and for each function in our
  Ghidra export, whether the port has its counterpart, named by its address. A function the runs never reach is read
  against the export or given a run that reaches it. `docs/COVERAGE.md` has the tool (origrun's `--coverage` and
  `tools/coverage.py`) and the steps ("A game's whole check"). reccmp, the matching-decompilation tool the user asked
  about, does not fit: it scores how closely code rebuilt with the original compiler reproduces the exe's
  instructions, and our ports are built by clang and Emscripten on a portable framework.
- **A performance pass before the readability pass (the user, 2026-11-03):** "Slot in performance passes for the games
  before readability passes. There's got to be a few things to speed up in most games." It comes after the coverage
  check. First each original and its native port are measured over the same scripted play, on a quiet machine (no
  passes running): the original through origrun under rundesk, the port as Bejeweled's was
  (`games/bejeweled/docs/PROFILE.md`, `tools/profile`: working set, private bytes, processor time and where the time
  goes), and the web build's memory and frame time in a browser. Then each game's session speeds up what the profile
  shows, natively and on the web. A speed-up changes how the port works, never what it does: the same draws, the same
  random numbers, the same screens, proved by the game's frozen baseline. One that changes what the player sees is a
  mod: a toggle in the game's fixes mod, on by default on the site, when it keeps the original's look and only stops
  wasting time, and otherwise a mod of its own that starts off, like Bejeweled's `lean` with its fewer frames
  (`docs/MOD_ROADMAP.md`, "The fixes mod"). No game's code is made multithreaded (CLAUDE.md, "Uncapped, one core
  each"). Shared costs (the framework's drawing, the mixer, image loading) are found once and fixed in the framework
  for every game. Each game's notes record its numbers before and after. A session need not wait for the pass to speed
  up its own port (the user, 2026-11-08: "Sessions should be allowed to improve performance early on so they can test
  with -fast and run even faster"): a speed-up that makes its `-fast` runs quicker may come at any time, under these
  rules, and is listed in its notes with the numbers.
  Shared costs found so far, waiting to be fixed once for every game: SDL_RenderPresent, a driver cost per presented
  frame under every renderer (69% of Alchemy's main thread, 85-90% of a `-fast` Bejeweled game's cycles with making the
  device; it swings 0.4-2.2 G cycles a minute between plays of one build, so only alternating runs compare). Frames
  almost never repeat, so only a frame cap would cut it, and that is a visible change, a mod. The whole-screen
  SDL_UpdateTexture of a software-drawn game (7% of Alchemy's main thread, about 1 G cycles a minute in Mahjong
  Escape): uploading only a redrawn MemoryImage's changed rectangle, or a streaming texture, would cut it
  (games/alchemy/docs/GAME_NOTES.md section 8). SexyApp::Init's window and graphics device take 250-310 ms of Noah's
  Ark's 0.6 s start; creating the device on another thread is ruled out (no threads), and showing the window before
  the device is made only moves the wait. The framework keeps every image's pixels after upload, 19 MB of Bejeweled's
  55 MB (the original: 36 MB), and Mahjong Escape's engine keeps each image's decoded bits beside its 32-bit copy.
  Every game's process holds about 54 MB before it reads a game file. libopenmpt decodes a module whole (51 MB of
  Ancient Japan's 146 MB private), where a streaming render would hold one buffer. For profiles,
  tools/profile/callprof.py puts 86% of samples in ntdll whatever the thread does; per-thread cycle counters
  (QueryThreadCycleTime) are the way (games/mahjongjapan GAME_NOTES, "Performance").
- **Done means every phase but modding (the user, 2026-11-08):** "I've decided I want to go through every phase
  except modding before a game can be considered done and ready for the live site, and just publish whatever games are
  ready by the end of December. I'm in no hurry to get games out in a less-than-stellar state." A game is done only once
  it has had every step: the user's faults fixed; both phases of identical play, with the real-input play of CLAUDE.md
  ("Done means identical"); registration and the buttons that lead out of the game removed; its fixes mod; the
  readability pass; the coverage check; the performance pass; the other code it does not need taken out; and, for a
  game ported from its Steam copy, the check against its PopCap Game Collection version. The coverage check and the
  performance pass are steps of each game's own finishing, in the order above where a game has not begun its
  readability pass (coverage, then performance, then readability, as the user ordered on 2026-11-03), and after it for
  the pilots. A performance measurement needs a quiet machine, so the coordinator gives each game a measuring slot when
  few game processes run. Modding is what waits: new mods and the changes listed in `docs/MOD_ROADMAP.md`; the fixes
  mod is not modding here, since the site serves every game with it on. On the last day of December 2026 the games that
  are done go on the live site (the site going public is still the user's word), and the rest follow when each is done.
- **The site (the user, 2026-11-02 to 2026-11-09):** what the user asks for while testing is done when asked. The chat
  is built as a test with made-up messages; its real version waits for funding. Desktop 95 is the site's only look
  ("I've basically completely lost interest in the skins that aren't Windows 95", and later that day: "drop the alt
  skins, including the plain one", 2026-11-09): new site work is built and checked in Desktop 95 alone. Its pixelated
  text waits for a pixel font drawn for the project (the user, 2026-11-08: "Draw our own later"): browsers draw only
  TrueType and OpenType, so the skin's "MS Sans Serif" comes out as the smooth Microsoft Sans Serif, and Microsoft's
  font itself is never shipped. An XP look, and perhaps Vista's, waits until the site works as the user wants it (the
  user, 2026-11-09: "it would be once all the site functionality is exactly as I want it"): each a second style of the
  same desktop, as XP's own Display settings offered the XP style or the Classic one for the same windows. The
  desktop's behaviour is first parted from the 95 look (the code tests for "d95" where it means the desktop), then each
  style is its own paint, controls and icons of our own drawing; Vista's glass is measured against the frame time of
  games in other windows. The coordinator's guess on 2026-11-09: $30-60 for the parting, $100-150 for XP, $150-200 for
  Vista.
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
  Twist's original); memory leaks stay fixed in the port itself. A game is done only once its fixes mod is made; until
  then a session that finds a fix lists it in its game's notes under "Fix candidates".
