# Coordination: the coordinator's rules

The user's rules for planning and running the work across sessions, which only the coordinator acts on. CLAUDE.md
holds the rules every session follows; this file does not repeat them.

## Usage

- **Weekly cap, a hard limit (the user, 2026-10-26, 2026-11-03, 2026-11-10):** at 97% of the account's weekly usage
  (all models, or the window of the model the sessions run on) work stops: "I only need 3% usage in reserve for the
  week". The coordinator then starts no session and sends none on, tells every session at work to commit and end its
  turn, and merges nothing that needs a suite run; the README refresh is the one thing it still does. It checks
  `mcp__ccd_session_mgmt__get_usage` hourly while sessions work, warns them at 95%, and checks again before every
  message that sends a session on ("You ran up the usage all the way to 100% despite my 97% limit", 2026-11-10).
- **Flat out to the cap (the user, 2026-11-07):** the work runs at full pace up to that limit.
- **Effort (the user, 2026-10-27, a trial):** game sessions run at medium effort to save usage.

## Sessions

- **What the coordinator does (the user, 2026-10-24, and since):** it relays the user's notes on a game to that game's
  session in the user's words, sends sessions on through the phases below, merges their milestones, and messages them
  when a rule changes (a running session reads CLAUDE.md again only at its next start or compaction). It looks into a
  session's code only when the user asks or when the work breaks something for others.
- **Model selection (the user, 2026-10-25):** a game whose port can largely copy another's (the same framework
  generation and engine, or a sequel after its first game) runs on Sonnet; a game that needs an engine or framework
  layer no port has yet (PopLua and PopAnim, the Sprout engines, the non-SAF engines, Chuzzle, Twist, Bejeweled 3,
  Zuma's Revenge) runs on Opus. Before merging a Sonnet port the coordinator compares about ten translated functions
  with the decompilation, weighted to the largest and most loop-heavy, and sends the fixes back; if a Sonnet game's
  review finds many faults or its port runs far over estimate, games of its kind move to Opus. Fable 5.2 ran every
  session for one evening (2026-11-10): a phase cost about what it does on Sonnet but took about three times the share
  of the all-models window, so the sessions went back ("Yes, switch back", 2026-11-11). Fable suits short stretches of
  judgement-heavy work, not many sessions at once.
- **How many at once (the user, 2026-11-11):** "Too many sessions, some crashed. Set a comfortable limit". At most 8
  sessions in a turn besides the coordinator ("Based on CPU and RAM usage it might be ok to run an extra session or
  two"); a session idle while its own background run goes on does not count ("Only count active sessions"). Before
  waking one the coordinator also checks the machine: under 6 GB free or over 80% of the commit limit means waiting.
  The limit of 6 game processes (CLAUDE.md) holds for all of them, so more sessions mean longer waits for a game slot.
  A session past the limit waits idle on a background wait for its own file (`until [ -f <main
  checkout>/build/slots/<name>.go ]; do sleep 60; done; cat` the file), which the coordinator writes with any news
  when a slot frees; that wake costs no message. A crashed session comes back only into a free slot.
- **Finished games' sessions are archived (the user, 2026-11-04):** to free the boot drive, a done game's session is
  archived and its worktree removed, after what only the worktree held (its `re/<game>/` exports and analysis, its
  baselines in `build/`) is copied into the main checkout. A later pass reuses the archived session (unarchived, it
  stays linked) or starts fresh from `main`.

## Publishing

- **The SAF Extractor/Packer (the user, 2026-11-04):** when Talismania's turn comes, its session clones the user's
  fork https://github.com/allisonpelton/Popcap_.SAF_Extractor-Packer (the user's OK for that download), makes it read
  and write `.saf` archives correctly (`docs/POPCAP_GAMES_SURVEY.md`, "Talismania Deluxe"), keeps its author's credit
  and licence, and pushes the working version to that fork: the one place a session pushes to.
- **origrun and the framework as projects of their own (the user, 2026-11-08):** "Note publishing origrun and the
  framework on SDL2 + wasm as a task to tackle at any point after the decomps are complete." Each gets its own
  repository and README: origrun with its SexyApp-era assumptions said plainly, its fixed paths made options and its
  protection handling left out (as `.publishignore` does now); the framework (`framework/`, `platform/`) under PopCap's
  framework licence with nothing taken from a game.
- **Which games are published (the user, 2026-10-26):** every game may be decompiled except Plants vs. Zombies,
  Insaniquarium and Zuma Deluxe, whose fan communities we leave to their work. A game with another PC decompilation in
  progress or an active modding community is built but unpublished (its folder in `.publishignore`, off the website);
  "web only" (Zuma's Revenge) keeps the folder in `.publishignore` but puts the game on the website. The "Published"
  column of `docs/POPCAP_GAMES_SURVEY.md` section 2 says which.
- Before each publish the coordinator checks the trunk for game files and game text.

## Which games, in what order

- **Order of the work on the merged games (the user, 2026-11-01, 2026-11-07, 2026-11-08):** each game is taken
  through CLAUDE.md's steps ("Order of the work", "Done means identical"): the user's faults and anything that makes it
  hard to test; phase one, plays identical; phase two, looks identical, with registration and every button that leads
  out of the game removed; its fixes mod; the coverage check; the performance pass; the other code it does not need
  taken out; the readability pass; for a game ported only from its Steam copy, the check against its PopCap Game
  Collection version (the user's ISO in their Downloads folder), its build working with that version's files too; and
  its features on the site. The registered original is the reference for every comparison; each removal is listed in
  the game's notes and proved by its frozen baseline, and how a protection was taken apart stays in private blocks.
- **What a phase two needs to merge (2026-11-03):** sound compared against the original (origrun's `--sound wav` log
  against the port's BYOPOP_SOUNDLOG; music a library mixes, through that library's stand-in: games/bigmoney/tools/
  bassproxy for BASS, games/mummymaze/tools/fmodproxy for FMOD 3), and the web build's pictures, shotdiffed against
  native, not just its log. Each brief says so.
- **Done means every phase but modding (the user, 2026-11-08, 2026-11-11):** a game is done, and may go on the live
  site, only after every step above, its features on the site included: its tunes placed, grouped and split in Music,
  and its soundboard's presets ("All these accessory features should be added to our definition of done"). The site
  session builds those and the game's session answers its questions. The games done on 2026-12-31 go live then (the
  site going public is still the user's word); the rest follow when done. Modding waits: new mods and the changes in
  `docs/MOD_ROADMAP.md`.
- **The ports still to make (the user, 2026-11-01, 2026-11-08):** none starts until every current game has had its
  readability pass ("Move up readability passes to come before additional decomps"). Then the easier ones side by side
  (by `docs/POPCAP_GAMES_SURVEY.md` section 2: Venice, Heavy Weapon, Hammer Heads, then Pizza Frenzy with Iggle Pop and
  Feeding Frenzy after it), and the harder ones from Chuzzle two at a time, in the survey's order (Feeding Frenzy 2,
  Peggle Deluxe and Peggle Nights, Bejeweled 3, Banana Bugs, Gyromancer, Zuma's Revenge, Talismania after Feeding
  Frenzy 2), each taken all the way through. The first pair is watched for trouble: "When we get to it, see if running
  two-by-two causes issues. If not, continue with two". A game that builds on another's engine waits for that game; a
  slot the next game cannot use yet goes to the cheapest remaining one. Within a step, the lowest-effort games go first
  ("Start with lowest effort", 2026-11-03).
- **The readability pass (the user, 2026-11-08, 2026-11-10):** "NO unknown/unnamed variables." Bejeweled set the
  standards, Twist tested them, and the user approved them ("new standards approved"): `docs/CODE_STANDARDS.md`, with
  `tools/namecheck.py` to prove "no unknowns" by a run. A readable port changes nothing it does and keeps each
  function's address for the coverage check.
- **A coverage check after the identical-play passes (the user, 2026-11-02: "Queue it"):** which of the original's
  functions the comparison runs reach, and whether the port has each export function's counterpart, named by its
  address; a function no run reaches is read against the export or given a run that reaches it (`docs/COVERAGE.md`).
  reccmp, which the user asked about, does not fit: it scores code rebuilt with the original compiler.
- **A performance pass before the readability pass (the user, 2026-11-03):** "There's got to be a few things to speed
  up in most games." Each original and its native port are measured over the same scripted play (the original through
  origrun under rundesk; the port as `games/bejeweled/docs/PROFILE.md` and `tools/profile` do it: working set, private
  bytes, processor time and where it goes), and the web build's memory and frame time; on a busy machine, per-thread
  cycle counters (QueryThreadCycleTime) and alternating runs, and the coordinator gives a measuring slot when few game
  processes run. Then the session speeds up what the profile shows, natively and on the web: how the port works
  changes, never what it does, proved by the frozen baseline. One that changes what the player sees is a toggle in the
  fixes mod when it keeps the original's look and only stops wasting time, otherwise a mod of its own that starts off
  (like Bejeweled's `lean`). No threads. A session may speed up its own port at any time under these rules ("Sessions
  should be allowed to improve performance early on so they can test with -fast and run even faster", 2026-11-08).
  Shared costs are found once and fixed in the framework for every game; the list below holds them.
- **Shared costs waiting for a fix:**
  - Loading GIFs: decoded to 32 bits (`_stbi__out_gif_code`) and quantized back to 8 (`Sexy::Quantize8Bit`, a binary
    search a pixel; a cache of the last colour gives the identical table), about a quarter of a load's processor time,
    0.15 to 0.2 s of a load in any game with GIFs (Typer Shark, AstroPop, GAME_NOTES section 11).
  - imagehlp.dll, 5 to 7.6% of a run's cycles, from a call site not yet found (Typer Shark, AstroPop).
  - Memory: the Direct3D 11 device (20 to 25 MB and about 0.2 s of start); every image's pixels kept after upload (19
    MB of Bejeweled's 55, 20.6 of Typer Shark's 85); about 54 MB in every process before it reads a game file;
    libopenmpt decoding a module whole (51 MB of Ancient Japan's 146), and an OGG tune the same (Ning Po's 5.6 MB),
    where a streaming render would hold one buffer; Mahjong Escape's engine keeping each image's decoded bits beside its 32-bit copy.
  - A memory jump beneath the heap: Twist's 2D level-up raises private memory by about 165 MB for ten seconds (the
    original flat), AstroPop's game start by about 60 MB for a few seconds, so it is likely the display layer. Count
    texture creations there in the first second of an AstroPop game, or hook malloc with a `--wrap` link.
  - `DDInterface::FillScanLines` sends every row to SDL as its own rectangle: about 21,000 a frame in Atomica's tunnel,
    about 58 MB in the driver and over a quarter of the game's processor time. Joining rows that touch may round a
    right edge differently at some scales; test with `games/atomica/tools/profile/pairs.sh` and a shot every 40
    updates of its scenarios s2, a1 and g5.
  - SDL_RenderPresent, a driver cost per presented frame (69% of Alchemy's main thread); only a frame cap would cut
    it, a visible change, so a mod. The whole-screen SDL_UpdateTexture of a software-drawn game (7% of Alchemy's main
    thread): uploading only a MemoryImage's changed rectangle would cut it. SexyApp::Init's window and device, 250 to
    310 ms of Noah's Ark's 0.6 s start (no threads, so no fix found). SDL's software renderer, the fallback without a
    GPU, 10 to 13 ms a frame at 640x480 against 0.26 ms with Direct3D 11.
  - For profiles, `tools/profile/callprof.py` puts 86% of samples in ntdll whatever the thread does; per-thread cycle
    counters are the way.
- **Memory broken down, and a software path to test (the user, 2026-11-11):** "I'd want the more granular performance
  reports then": every game's memory split into libraries, sound, graphics and game state, for the port and where
  possible the original, in the README's performance table (port against original: load time, processor time and
  memory, 2026-11-10). The framework answers a standard `memreport` test command, and a tool walks a process's memory
  regions, which works on an original too. "Software path sounds interesting for a test, at least": a port drawing
  through SDL's software renderer with no Direct3D device, measured against the default; no default changes without
  the user.
- **After every game is done: gamepads (the user, 2026-11-10):** "Once all games are done, I want to add native gamepad
  support to every game. I also want to support the Wii Remote in particular." Shared framework work with a layout per
  game. Also then: an installer that puts a native port beside its Steam original, started by a launch option
  (`docs/STEAM_LAUNCH.md`).
<!-- private -->
- **Building the web port in the browser (the user, 2026-11-04):** "Let's try it on the simplest possible game and
  see how long it takes to build in the browser and how much work it adds for each game", saving to localStorage or a
  connected Google Drive, to avoid any infringement. Not started; it belongs with the work before the website is ready.
  Drive (read on 2026-11-11, developers.google.com): the `drive.appdata` scope gives a hidden folder that only the site
  can read, which cannot be shared, and which Google deletes when the player removes the site from their Drive;
  `drive.file` reaches only the files the site made or the player picked. Both are non-sensitive scopes: no security
  assessment, no user cap, no fee. The page talks to Drive itself, so nothing passes through the site's servers. Showing
  the site's name and logo on Google's consent screen takes brand verification (free, two or three business days): the
  domain proved in Search Console, a homepage anyone can open, a privacy policy on that domain saying how Google data is
  used. The broad `drive` and `drive.readonly` scopes are restricted and need a paid security assessment; never ask for
  them. The files count against the player's own quota (15 GB free).
<!-- /private -->

## The site

- What the user asks for while testing is done when asked; each note is tracked until a merge ships it. Desktop 95 is
  the only look ("drop the alt skins, including the plain one", 2026-11-09), and new site work is built and checked in
  it alone. Its pixel text waits for a font drawn for the project ("Draw our own later", 2026-11-08); Microsoft's font
  is never shipped. XP and perhaps Vista styles of the same desktop wait until "all the site functionality is exactly as
  I want it" (2026-11-09). The chat is a test with made-up messages; its real version waits for funding.

## Mods

- **No new mods unless requested (the user, 2026-10-30):** "no new mods for anything unless someone requests it." The
  standard mods and each game's ideas in `docs/MOD_ROADMAP.md` become the first entries of each game's feedback page on
  the website, where players suggest and vote and the user decides ("Implementation of the mod could even be automated
  based on a comment getting enough upvotes (and my approval, of course)."). Fixes for the mods that exist wait in
  `docs/MOD_ROADMAP.md` until every decompilation is finished. The fixes mod is not modding: `docs/MOD_ROADMAP.md`,
  "The fixes mod".
- **A goal for Twist when modding resumes (the user, 2026-11-03):** "produce a TAS that scores >5.1 million in Blitz
  mode and save a replay of it that can be recorded and uploaded."
