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
  out of the game removed; its fixes mod; the coverage check; the performance pass; its lite mod; the other code it
  does not need
  taken out; the readability pass; for a game ported only from its Steam copy, the check against its PopCap Game
  Collection version (the user's ISO in their Downloads folder), its build working with that version's files too (also
  for a game ported from another copy when the Collection's is another build: BookWorm's and Word Harmony's Collection
  exes differ from their retail ones, so players with the Collection need the check, 2026-11-11; where every file
  matches, as Ning Po's and Water Bugs' exes do by size, the check is that comparison and one port run; the ISO's
  file groups are `<Title>Component`, listed with `E:/retools/unshield-build/unshield.exe g data1.cab`); and
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
- **A new port writes to the standards from its first draft (the user, 2026-11-12: "Yes, add it").** Of what the
  eleven smaller games spent after their first playable merge (about $2,200 at Opus prices), about half went on the
  checks against the original (identical play, screens and sound, coverage, the Collection), a fifth on performance and
  a sixth on passes that rework the first write (readability, unneeded code, registration out); Noah's Ark had the
  cheapest port and the dearest finish of them. So a new port, from its first draft:
  - takes the Mac build's names before translating (`docs/MACMATCH.md`, as before) and names everything else to
    `docs/CODE_STANDARDS.md` as it goes, so `tools/namecheck.py` passes at its first merge;
  - leaves out the registration paths and every button that leads out of the game (CLAUDE.md, "Order of the work");
  - does not translate a function the export shows nothing can reach (no caller, no vtable slot, no table or callback
    holding it), and lists each one by its address in the notes, so the coverage check confirms it;
  - uses the framework's fast paths and avoids the shared costs listed under "A performance pass" below (pictures
    decoded at their first use, sounds at their first play, loading in one go);
  - translates rigorously: loops in their original shape, every random draw in its order, the original's arithmetic.
  The steps keep their order, since speed-ups and removals are proved against the frozen baseline of a port that
  already plays identically, but each is shorter: the readability step is namecheck's run and a read of what it flags,
  the unneeded-code step is what the coverage check finds, and the performance pass is the measurement and the game's
  own hot spots. One session takes the game through every step; each brief for a new port says so.
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
  (like Bejeweled's `lean`). One that computes faster but may play differently is listed under "Lite candidates" for
  the lite mod (`docs/MOD_ROADMAP.md`, "The lite mod"). No threads. A session may speed up its own port at any time
  under these rules ("Sessions
  should be allowed to improve performance early on so they can test with -fast and run even faster", 2026-11-08).
  **The target (the user, 2026-11-11):** "I understand why our framework increases RAM usage on every game. Is there
  an equivalent justification for some games being worse on CPU? If not, that should be an enforced target." There is
  none: the framework adds a fixed amount of memory to every process, but a port's extra processor time has always
  come from code that does more work than it needs to. So a port's processor time is at most its original's before the
  pass is done, measured like for like:
  - the same stretch of play;
  - the original paced as on a player's 60 Hz screen, since one that waits for a vertical blank spins on the hidden
    desktop;
  - sound counted the same way on both sides. origrun's stand-in takes the original's DirectSound, and its MIDI device
    calls may be skipped, so the original's number may hold no mixing or synthesis. Measure the port the same way, or
    count its mixer and synthesizer apart.
  - **Cycle counts, not GetProcessTimes or GetThreadTimes.** Count processor time as the threads' cycle counts
    (QueryThreadCycleTime, summed over the process's threads; `games/tiptop/tools/perf/cpuprof2.py` at seconds 0
    reads them without disturbing the run). Windows charges process and thread times by sampling at the 15.6 ms clock
    tick: a frame loop doing exactly 0.60 s of work was charged 0.59 to 0.72 s in five runs, while its cycle count
    stayed within 4% (2026-11-11).
  - **The player's exe, with no test machinery running.** There is no separate player build: the shipped exe is the
    Release build, and its test code costs a few comparisons an update while no flag turns it on. During the measured
    stretch, nothing of it may run:
    - input comes from a `-script` read at start, or from posted real input;
    - no `-commands` channel, which is polled every update;
    - no per-update state log;
    - no in-process bot, unless the original runs the same bot code. Otherwise subtract the bot's cycles.
    The load figures may use the channel and say so.
  - **Alternating runs.** Alternate the two sides' runs and take the medians of at least five pairs.
    `python -I tools/pincore.py -- <command>` runs each on a performance core of its own: no efficiency core, and no
    hyperthread shared with another pinned run.
  - **The same window on both sides.** origrun runs an original windowed at its own size (`--reg FullScreen=0`). A port
    started with a fresh userdir takes its original's defaults, often full screen, and under rundesk that is the
    whole 3840x2160 desktop at render scale 4 (Mahjong, 2026-11-12); a port's window also starts at twice the game's
    size where it fits. So the port is measured windowed (FullScreen=0 in its settings) at the game's own size in
    pixels (`-windowscale=1`), and its log's display line is checked ("display: client WxH px, logical WxH, render
    scale N, windowed|full screen, renderer <name>", logged when the window is made and at each size or mode change;
    docs/TEST_COMMANDS.md, "The window line"). The default window's cost is measured as well, beside it, since that
    is what a player gets. Where a session needs to know which part of a frame costs what, `-cyclesplit` in the port
    and `origrun --count` in the original split update, draw and present on both sides (docs/TEST_COMMANDS.md,
    "Picking the functions"); per-update figures compare, per-call ones do not, since a port presents in step with
    the display and an original's Blt does not wait (BookWorm, 2026-11-12).
  - **The renderer a player gets.** On the visible desktop SDL picks Direct3D 9; on rundesk's private desktop it
    gets Direct3D 11 (TipTop and AstroPop, 2026-11-12). TipTop's bench put a frame's present at 2.5 to 2.9 M cycles
    on the visible desktop (D3D9) against 2.0 to 2.6 M on rundesk's (D3D11), and a window-surface present at 0.9 to
    1.2 M on both. So name the renderer each run used, and compare a port and its original on one desktop.
  - **origrun's own cost.** Until 223d9bdf the stand-in listed the copy's modules up to once each 15 ms from any hooked
    call, which in an original that asks for the cursor every 10 ms added about 1 G cycles in 20 s (Typer Shark: its
    earlier "0.76 of the original" was this). Now it looks only after the loader maps a DLL. A count of an original made
    before that merge is high by up to that much when the original polls the cursor often, so such a game measures again.
  - **The update counter.** Mummy Maze's original keeps three counters side by side, sleeps at app+0x328, draws at
    +0x32c and updates at +0x330, and its origrun counter had been the sleeps, which run 1 to 11% behind the updates
    (2026-11-12). Other originals of that framework's age may keep the same three, so a session checks that the counter
    it times input and per-update figures by is the updates.
  Where the excess is a shared cost, the session sends the spec, and the game waits for the framework's fix instead of
  being let off.
  Shared costs are found once and fixed in the framework for every game; the list below holds them.
- **Shared costs waiting for a fix:**
  - Loading GIFs: decoded to 32 bits (`_stbi__out_gif_code`) and quantized back to 8 (`Sexy::Quantize8Bit`, a binary
    search a pixel; a cache of the last colour gives the identical table), about a quarter of a load's processor time,
    0.15 to 0.2 s of a load in any game with GIFs (Typer Shark, AstroPop, GAME_NOTES section 11). With the JPEG and
    Vorbis decoders and Platform::DecodeImage, the load's decoding is about 12% of Bejeweled 2's 3D run without waiting.
  - imagehlp.dll, 2.4 to 7.6% of a run's cycles, from a call site not yet found (Typer Shark, AstroPop, Bejeweled 2).
  - The 2D path on SDL's software renderer (`DDInterface.cpp`, `mIs3D` false, about lines 784 to 833): every sprite an
    SDL_RenderCopyEx or SDL_RenderGeometry blended in software, 15 to 25 ms a drawn frame in Bejeweled 2's 2D mode (81%
    of a core where the original takes 18%, 83.6% of the cycles in SDL2.dll, 31 s for a `-drawevery` test run). It
    reaches a player without an accelerated renderer and every 2D test run. Drawing the dirty widgets into one CPU
    surface with the framework's own blitters and uploading it once a frame, as SexyAppFramework's DirectDraw path does,
    would cut it; test with `games/bejeweled2/tools/profile/pairs.sh` and `tools/check.sh --changed` (2026-11-12).
    Bejeweled 2 counted the draw calls on its idle board (2026-11-11, cycle counts, five pinned pairs): 93.7% of a core
    against the original's 12.7%, 98% of it inside SDL's draw calls, a 14.5 ms frame of about 65 opaque sprites at 183
    us each. Two causes. A window of 1,280 by 960 gives a render scale of 2, so every sprite goes through SDL's
    software stretch; and an image with no transparency is still blended. Three changes, prototyped in a scratch build:
    an image with neither alpha nor transparent pixels, drawn at full alpha in normal mode, as a copy
    (`SDL_BLENDMODE_NONE`; frame 13.2 to 7.8 ms, the shot identical); drawing at the logical size on the software
    renderer and presenting with a nearest-neighbour stretch at an integer ratio (frame 2.6 ms, the board 23.8%; pixel
    doubling instead of smoothing, so a change of look the user decides); and drawing at most at the display's rate,
    skipping a frame with nothing dirty, as the surface presenter below does. The first is on the trunk (e19af272, with
    fec8a8a5 keeping images in forced mode blended); the third goes into the presenter, and the second waits until those
    are measured. The `-commands` poll, a stat of the file every update (0.34 M cycles a call, an eighth of the main
    thread in a run that uses the channel), is now Platform::FileSizeWatch, an open handle and a directory watch, about
    49 k cycles a call (fa73221e).
  - A relative `-userdir` (`./u3/`) is not found where the same folder by its absolute path is (Bejeweled 2): harmless
    for players, a trap for scripts.
  - Memory: the Direct3D 11 device (20 to 25 MB and about 0.2 s of start); every image's pixels kept after upload (19
    MB of Bejeweled's 55, 20.6 of Typer Shark's 85); about 54 MB in every process before it reads a game file;
    libopenmpt decoding a module whole (51 MB of Ancient Japan's 146), and an OGG tune the same (Ning Po's 5.6 MB),
    where a streaming render would hold one buffer; Mahjong Escape's engine keeping each image's decoded bits beside its 32-bit copy;
    in 2D, a picture loaded as 8-bit pixels and a palette kept as 32-bit pixels and a texture once drawn (Bejeweled 2's
    private bytes go from 59 to 101 MB at a game's start, the original's from 40 to 43; memreport with PERF_MEM=1).
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
    GPU, 10 to 13 ms a frame at 640x480 against 0.26 ms with Direct3D 11. A partial upload helps a game whose dirty
    rectangle is most of the screen little (87% on Cosmic Bugs' board); a streaming or 16-bit texture would cut the bytes.
    In Word Harmony the frame's present is about 35% of what is left after its pass (the whole 1.9 MB screen picture
    to a static texture, then the copy to the target and the window); SDL's other drivers cost the same or more. A
    rectangle upload fed by a list of changed rectangles the game gives the screen image would help it; test with its
    `tools/profile/pair.sh` against the original (2026-11-11). TipTop measured the other side: its screen uploaded as
    tiles (128x96, 320x240 or 64x48, about 6 of 25 changed a frame) gained nothing, since an upload call costs 0.3 to
    0.45 ms whatever its size, so a rectangle upload pays only where it stays one call a frame (2026-11-11).
    Mahjong measured the way out (2026-11-11, `build/presenttest` in its worktree). It sent 800x600 software frames at
    40 a second and counted cycles over 15 s, with these results:

    | Presenter | Cycles in 15 s |
    |---|---|
    | The framework's renderer path | 1.6 to 2.3 G |
    | No vsync | 1.7 to 2.1 G |
    | SDL's software renderer | 1.2 G |
    | The window surface (SDL_UpdateWindowSurface), whole frame | 0.68 to 0.80 G |
    | The window surface with a dirty rectangle of 15% | 0.26 to 0.28 G |
    | The window surface with a dirty rectangle of 3% | 0.12 to 0.16 G |

    A partial texture upload does not help, since the driver's cost is per present. So a software-surface presenter,
    opt-in by the app and given the frame's dirty rectangle, is the fix for every software-drawn port. It is on the
    trunk as SurfaceDisplay (a299640e, built by the Mahjong session; framework/SurfaceDisplay.h says how an app takes
    it): Bonnie's uses it, the Mahjongs' engine with its own merge; TipTop, Word Harmony, Dynomite and the Retro64 engine (Cosmic Bugs builds
    it for both Retro64 games) take it in their performance passes. Cosmic Bugs keeps its own partial-upload work off
    the trunk.
  - Not a cost but a shared look (Typer Shark, 2026-11-12): the 2004 framework's Direct3D 9 blends straight alpha, and
    DDInterface uploads premultiplied textures whenever the renderer has custom blend modes (`SetupBlendModes`), which
    moves the edges of pictures with transparent texels by up to 44 levels in 3D. Typer Shark switches to the
    framework's straight fallback modes at BeginFrame while 3D is on (`TyperSharkDisplay`, WinWSApp.cpp); its 2D needs the
    premultiplied ones. A DDInterface flag for straight alpha in 3D, like `gWidgetsDrawWithoutFastStretch`, would serve
    the other 2004-framework games once one of them shows the same edges.
  - Presenting in step with the display. The framework asks for vsync whenever `-fast` is off (`InitDDInterface`),
    and the driver spends cycles in the wait: in AstroPop's 3D, SDL_RenderPresent took 1.35 M cycles a frame with vsync
    against 0.52 M without, 46% of the main thread, and the driver's second thread about 1.4 M more; about 70% of the
    port's 3D stretch was the present path and under 15% the game's code (2026-11-12, instrumented scratch build). The
    original does not wait for the display in a window. A window's frames are composed by the desktop, so presenting
    without vsync should not tear; a candidate is no vsync with presents capped at the display's rate by the frame
    loop's own timing (Bejeweled 2's pace code), measured against vsync and checked for even pacing. Typer Shark
    measured the same path (2026-11-12, 2,000 updates of its adventure recording, five pinned pairs on the visible
    desktop). The Direct3D 11 driver thread is about 60% of the port's cycles, 1.7 to 2.7 M a presented frame whatever
    is drawn. It falls when the machine is busy (3.2 to 3.6 G in 20 s on a quiet machine, 1.8 to 2.5 G on a busy one)
    while the original's cycles rise, so a player's quiet machine is the port's worst case. Vsync off costs more, SDL's
    Direct3D 9 costs 8.3 G and OpenGL 4.3 G, and drawing straight into the back buffer at render scale 1 (no screen
    texture pass) saved nothing. Mummy Maze's port draws 37 frames a second against the original's 53, since each
    present waits for the display and two updates run between draws. AstroPop measured the cap against vsync in 3D
    at 1x (GAME_NOTES 11d, 2026-11-12, `-presentlog` for the intervals): no clear gain (105% of the original with the
    cap, 114% with vsync, about +-30% a run), and the cap's presents are lumpy, quantised to the loop's 10 ms updates,
    so keep vsync on the GPU path. Per present its present call plus driver thread cost 1.9 to 2.4 M cycles against the
    original's 1.2 M; the driver thread runs the frame's ~120 draw calls (one per texture change), so only fewer,
    larger submissions cut it (the next item). The port's mixer thread, about 0.13 M an update, has no counterpart in
    the original's process.
  - Draw calls (Typer Shark, 2026-11-12). `DDInterface::DrawTexturedQuad` makes one SDL call a quad: 593 a frame for
    about 20 texture changes, DrawScreen 466 M cycles in 20 s. Joining consecutive quads of one texture into one
    SDL_RenderGeometry call would take about half of that, some 6% of the port. The draw order stays, and every port's
    frozen baseline must hold.
  - A cycle-weighted EIP sampler charges a thread's bursts to the wait it stands in (54% of Cosmic Bugs' cycles on
    `ZwWaitForSingleObject`); timers around the calls with `QueryThreadCycleTime` find the real split. Cosmic Bugs'
    `tools/perf/cpuprof.py` has `CALLERS=1` (the exe function and the DLL on the stack).
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
