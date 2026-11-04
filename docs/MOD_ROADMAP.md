# Mod roadmap

Gameplay mod ideas to implement once the decompiled game and the mod menu are fully working. Each needs
a hook in the translated `Board` code; the translation keeps the original behaviour as the default and
reads these parameters from mod tunables instead of hard-coding them.

## Kinds of mod

Moved here from CLAUDE.md on 2026-11-01, word for word; CLAUDE.md keeps the rule's first sentence.

- **Mods (the user, 2026-10-26):** code that makes a game moddable makes things configurable (rules, settings and
  their ranges in `mod.json` and the mod menu); it never just overwrites the original, which plays unchanged with
  no mod on. Every mod is data: a feature's code lives in the port, and a mod's `features` or `rules` turns it on.
  So each feature the port adds also ships a mod that turns it on. There are three kinds of mod:
  - An **example** mod has `example` in its name (`example-flame-gems`, `example-twist`). It shows how a single
    feature can be configured, for playing with its settings and finding which features work well together. It
    starts off (`"defaultEnabled": false`).
  - A **feature** mod gives the player one feature under its own name (BookWorm's `widescreen` or `undo`). Its
    settings are in the mod menu.
  - A **curated** mod gives the player a tuned experience, much like a fan game: features, settings and art
    chosen to play well together. It never copies a game mode from another game (the user, 2026-10-26): it always
    has a twist of its own that makes a player want to pick it over the game it draws on. A port may implement
    features from other games so that they can be recombined in new ways, and it is fine if a player can combine them
    into something close to another game's mode; a curated mod itself is never just that mode. Bejeweled 2's
    Twilight alone in Bejeweled is not interesting, but with other tunables it can be.

| Idea | What it needs in the engine |
|---|---|
| Allow invalid moves | Swap rule flag: keep a swap that makes no match instead of swapping back (and decide whether it counts as a move / costs time). |
| More gem colors | Gem-type count above 7: extra rows in the gem sheet (image patch or a taller `gemsheet6`), colour list for effects that tint by gem, RNG `% n` with the new count. |
| Board size and arbitrary shapes | Width/height parameters and a cell mask (holes) instead of the fixed 8x8 grid; layout/centering, gravity that skips holes, refill per column, hit testing. **Playable** in variant builds since Bejeweled's stage 7: `board.cols`/`board.rows` 3 to 16, `board.mask`, `board.levels` (a shape per level); dirt cells play as holes until stage 7b (CODE_MODS_DESIGN.md 4.3). |
| Dirt tiles (Jewel Quest) | Cells that hold no gem and block falling gems. A match next to one removes a layer, and gems slide past diagonally, alternating left and right. Design: games/bejeweled/docs/CODE_MODS_DESIGN.md 4.14. |
| Twilight gravity (Bejeweled 2) | Bejeweled 2's secret mode: gravity flips each time the board comes to rest after a move (never after a swap that makes no match), and once per board fill, so new gems fall and rise in turn (the opening board rises). Needs direction-aware falling, refill, fill, fall-out and game-over fall, a saved flip count, slower falls, a mirrored half-brightness backdrop and sounds 2 semitones lower; Normal and Timed. Design: games/bejeweled/docs/CODE_MODS_DESIGN.md 4.16. **Playable** since Bejeweled's stage 7c: `board.gravity.direction` (down, up, alternate), `board.gravity.fallAccel`, `look.backdrop`, `look.soundPitch` (the shared SoundManager::SetDefaultPitch). |
| Chuzzle-style play | New move type: drag a whole row or column with wrap-around; match check after release, revert if no match (unless invalid moves are allowed). |
| Bejeweled Twist-style play | New move type: rotate a 2x2 group clockwise around a grid intersection; cursor between cells. |
| Jewel Quest-style "gold board" mode | Per-cell state layer under the gems (plain / gold); every cell covered by a scored run turns gold, and the level is won when all cells are gold, instead of filling the bonus bar (Jewel Quest also gives each board a time limit and odd board shapes, so it builds on the board-shape and timer hooks). Needs: the cell layer saved with the game, gold tile art drawn in place of the checkerboard (mod-supplied, e.g. a `goldtile` image), a progress display (cells left), and level rules (time limit, what happens on no moves). |
| Custom match patterns | Pluggable match finder: minimum run length (e.g. 2), shapes (L of three), diagonal lines; scoring per pattern. |
| Flame and lightning gems | Special gem kinds created by 4- and 5-matches, with their own sprites/animations, explosion (3x3) and row+column clears, cascades through the existing scoring. |
| Set seed | All game randomness is `Sexy::Rand()` (MT19937), seeded from the tick count in `SexyAppBase::Init`; expose the seed (options / command line / URL) and show it on the game-over screen. |
| Save and watch replays | Not a player feature in the original, but its framework had a developer demo recorder (`-record`/`-play`/`-demofile`; seed + timestamped input). The port drops that code; a replay = seed + mode + per-tick input log, played back through the same 100 Hz update loop. Needs a replay browser UI. |
| Online leaderboards | Host the web build on a Cloudflare Worker (static assets + API) with a D1 database for scores, one board per mode and per gameplay mod set (the existing ModManager score key). Submissions carry a replay; the Worker verifies it by re-running the game logic headlessly (the game compiled to WASM without SDL video/audio, fed the replay's seed and input log at 100 Hz) and accepts the score only if it reproduces. Needs: the replay format (see "Save and watch replays"), a headless build target, rate limiting, and a name/identity scheme. A Cloudflare account is connected to this environment (D1/Workers tools), so setup can be done from here when we get to it. **Not planned:** the user decided on 2026-10-24 that the player has no online features. |
| Undo move | Snapshot board + score/level/timers + RNG state before each swap (the save-game writer already serializes the board); restore on undo. Decide on scoring (separate high scores, like other gameplay mods). **Playable** in variant builds since Bejeweled's stage 6 (CODE_MODS_DESIGN.md 4.12, 6.7). |
| Auto LiveSplit support (desktop) | A tagged, read-only memory block in desktop builds, filled after each update while `features.livesplit` is on, and a published ASL script that finds it: start at GO, split at each wormhole or Score Attack threshold, reset when the run's game ends, turned on with LiveSplit's Activate. Real time only, as speedrun.com times Bejeweled Deluxe. Publishing, the licence, contacting HyperNervie (who lists a Bejeweled 1 splitter as coming soon) and the registry entry wait for the user's approval. Design: games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15. |
| Steven Universe gems | The seven gems redrawn as gems from the show (Yellow Diamond, Pearl, Sapphire, Ruby, Amethyst, Jasper, Peridot), in the show's flat cel-shaded look, with spin and hover frames and the title-screen gems. Images only, from a generator like the Bejeweled 2 gems; its output is fan art and stays git-ignored. No engine change. Design: games/bejeweled/docs/CODE_MODS_DESIGN.md 4.17. |
| Code-drawn art (no image files) | Every image and bitmap font the game loads drawn by code at load time (gems, board textures, frame and HUD, title screen, dialogs, effects, cursors, fonts) under the original names and sizes, so the game runs without image files and is sharp at any window size, with its draw code unchanged. Needs an image source in the loader ahead of mods and game files (shared), a small anti-aliased rasteriser and a vector font. Design: games/bejeweled/docs/CODE_MODS_DESIGN.md 4.18. |
| Detailed game-over stats | A page of the game's numbers at game over (runs, moves, swaps back, hints, undos, levels), from an observer with hooks in the board, and a sidecar chunk of its own. Bejeweled's stage 17, `features.gameStats` (CODE_MODS_DESIGN.md 4.19). |
| Game modes from mod combinations | After all the mods: combinations of mods and settings that make fun, balanced modes, each tuned with the autoplay bot and measured "fun" numbers over many seeded games (choice, cascades, tension, pace, balance, frustration). Bejeweled's CODE_MODS_DESIGN.md 9.2, after the stages table. |
| Arrow gems | Some gems are arrows pointing up, down, left or right. After every settled move each one steps a cell that way, trading places with the gem there; an arrow that would step off the board ends the game, so it has to be matched first. A four-way take on Bejeweled 3's Butterflies (none of that mode's art or texts), asked for by the user on 2026-10-25. Bejeweled's stage 18, after stage 17 (CODE_MODS_DESIGN.md 4.20). |
| Sweet Tooth look (maybe) | Bejeweled restyled to look like Sweet Tooth To Go, PopCap's candy match-three made for Pogo, which the user decided on 2026-10-25 not to port, though they may want this look instead. Images only. Like the Steven Universe gems, its art comes from a local generator and stays git-ignored, since it would recreate another game's look. No engine change. Not yet planned. |

## Fix mods

The user, 2026-11-04: "I think we'll want the default way to serve games on the site be to include a bunch of mods
making changes that fix bugs in the original or make it more performant (without doing things like Bejeweled's Lean
mod) by default. The user can turn them off for a more 'true' experience. That way these don't have to be
'deviations' and can instead be features."

A **fix** mod is a fourth kind. It fixes a bug of the original, or stops the original wasting the player's time or
the machine's, and otherwise leaves the game as it was: its look, rules, sounds and pace of play. TipTop's loading bar
is the first candidate: it loads one of its 229 items a frame, so it fills at a steady crawl however fast they load
(2.3 seconds in the port, about 3.8 in the original on a 60 Hz screen, where a frame waits for the vertical blank). A
mod that trades the original's look or feel for speed or memory, like Bejeweled's `lean` with its lower frame rate,
is not a fix mod and starts off as before.

The site starts each game with its fix mods on, and the player can turn them off to play the original as it was. The
port itself stays the unmodified original, so a native build without the mods, the oracle and the frozen baselines
all play the original, and the rules of CLAUDE.md ("Port first, unmodified", "Done means identical") still hold for
the port. Like the other mods, fix mods are made once every decompilation is finished. Until then a session that
finds a candidate keeps the original's behaviour in the port and lists the candidate in its game's notes under "Fix
mod candidates": what the original does, how to see it, and what the fix would change.

## Standard mods for every game

The user approved this list on 2026-10-26 as the mod plan of every game. A port is first the original game, unmodified
and working like its exe; it adds nothing for later mods, not even hooks. In the game's mod phase (which waits until every
PopCap game works in the browser, CLAUDE.md "Order of work") its session goes through this list, makes each mod that fits
the game, and says in the game's README why it skipped any. Games that already have mods (Bejeweled, Big Money, BookWorm,
Noah's Ark) make none of these until the user says every decomp runs in the browser to their satisfaction (2026-10-26).
Each is a feature mod (or an example mod for a single
setting), under the mod rules in CLAUDE.md. Where a game already has one, it is named in brackets.

The user's first six:

1. LiveSplit: an autosplitter for the desktop builds, from a tagged memory block and a script LiveSplit runs (Bejeweled,
   Big Money, BookWorm).
2. Widescreen: 16:9, with the session deciding what goes in the extra space (BookWorm).
3. High-res textures: an upscaled image pack generated locally from the player's own files, never committed (Bejeweled).
4. Replays: a seed plus the input of each update, played back through the same update loop (Bejeweled, Big Money,
   BookWorm).
5. Seeded RNG: the seed shown at game over and settable from the options, the command line or the URL.
6. Bot scoring: a bot that plays games without a window and scores them, for tuning curated mods, where the game suits it.

Speedrunning and competition:

7. Replay checker: a saved replay replayed without a window to confirm its score, so scores can be shared as files
   without online features.
8. Timer and input display on screen, for speedruns in the browser, where LiveSplit cannot read the game.
9. Daily board: the same seed for everyone on a date, with its own high scores (BookWorm, Noah's Ark).

Practice and help:

10. Undo, in turn-based games (Bejeweled, BookWorm, Noah's Ark).
11. Hints or a solver (BookWorm's word solver, Noah's Ark's move helper, Big Money's puzzle solver).
12. Level select: start at any level or stage, with separate scores.
13. Game stats: the game's numbers at game over, and lifetime totals (Bejeweled's stage 17, BookWorm's word journal).
14. Speed control: a slower or faster game clock.

Accessibility:

15. Colour-blind help: symbols or other palettes for anything the game tells apart only by colour.
16. Reduced motion: screen shake, flashes and heavy particle bursts turned off.
17. Other controls: keyboard-only or gamepad play for mouse games, touch for phone browsers, remappable keys.
18. Larger text and a high-contrast interface.

Rules and content:

19. Rule settings: timers, scoring, lives and the difficulty curve as settings in the mod menu; every curated mod builds
    on them.
20. A relaxed variant: no timer, or no game over.
21. Custom levels: a documented level format, so a mod can add levels, in level-based games.
22. Randomizer: a shuffled level order or generated levels, in level-based games.
23. Developer leftovers: cheats, debug keys and unused content found in the exe, as an example mod.
24. Translations: a table that replaces the game's texts by id, so a mod can carry a language. Translations of a game's
    texts are derived from them, so they are made locally and never committed.
25. Local achievements, for games whose versions had Steam achievements, shown and saved locally (their names read from
    the player's exe like other game texts).
26. Two players taking turns or competing, where the game's rules allow it.

Presentation:

27. Pixel-perfect scaling and an optional scanline filter, for the 640x480 games.
28. Smooth frames on high-refresh screens: frames drawn between updates without changing the update clock, so replays
    stay exact.
29. Jukebox: the player chooses the music, or the game's other tracks play on other levels.

## Nintendo 3DS port (after Bejeweled works fully with mods)

Attempt a native 3DS build of Bejeweled; the goal is to get it running in an emulator. First research how
native 3DS ports have been done before (for example the Super Mario 64 decompilation's 3DS port, and other
homebrew ports built with devkitPro's devkitARM / libctru / citro3d, or SDL2's 3DS backend), then pick the
approach. Known constraints to plan for: the 640x480 game against 400x240 (top) + 320x240 (bottom touch)
screens, 64 MB RAM on the original 3DS (more on the New 3DS), touch instead of mouse, GPU and audio APIs
different from SDL's desktop backends, and asset size/format limits. The SDL-based platform layer and the
game-agnostic framework port are the parts to reuse.

## After Bejeweled: more PopCap games

Once Bejeweled Deluxe and all of the mods above are done and working:

1. **Big Money Deluxe** - decompile, port to WASM, and develop mods for it. Installed at
   `A:\SteamLibrary\steamapps\common\Big Money Deluxe`. `WinBM.exe` (2006-08-21) is the same PopCap DRM
   wrapper (SexyAppFramework RTTI), so the unwrapping tool (`tools/extract_game_exe.py`, private), the
   Ghidra pipeline (`tools/ghidra_scripts`, `tools/ghidra_headless.sh`) and the framework-identification
   approach carry over. A separate `BigMoney.exe` (2005-11-14, no framework RTTI) needs a look first.
2. **Other PopCap games**, in the order agreed on 2026-10-18 (since replaced: the plan in
   [POPCAP_GAMES_SURVEY.md](POPCAP_GAMES_SURVEY.md), section 2, "The plan for the remaining games", with each game's
   effort and prerequisites in its section 5, "The remaining games"):
   1. BookWorm Deluxe. It is now also the first second-game target, so it takes on the multi-game
      layout groundwork.
   2. Typer Shark! Deluxe, which uses the same 2004 framework.
   3. Bookworm Adventures Deluxe + Volume 2.
   4. The Sprout-engine line: Feeding Frenzy 2 + Talismania, then Iggle Pop and Feeding Frenzy Deluxe (added 2026-10-25). Pizza Frenzy now comes
      first in it (no references, 2026-10-26).
   5. Bejeweled 2 Deluxe.
   6. The Peggle family, decompiled without Thunderball; Deluxe and Nights stay unpublished (2026-10-26).
   7. The remaining classic titles: Rocket Mania, Hammer Heads, Heavy Weapon, Dynomite, AstroPop.
   8. The newer frameworks: Bejeweled 3, Bejeweled Twist, Zuma's Revenge.

   **Decided 2026-10-26 (COORDINATION.md, "Which games, and where they are published"; POPCAP_GAMES_SURVEY.md
   section 2, "The plan for the remaining games"):** every game may be decompiled, with no other project as a
   reference, except Plants vs. Zombies, Insaniquarium and Zuma Deluxe, whose fan communities are left to their own work, and
   Chuzzle's Christmas Edition. Games with another PC decompilation in progress or an active modding community (Peggle
   Deluxe and Nights) are built but not published. Pizza Frenzy and Chuzzle Deluxe are back in the order.

## Very far out: the Build-a-Lot series

Decompilations of the full Build-a-Lot series (HipSoft, not PopCap, so a different engine and no
SexyAppFramework to lean on; the general pipeline - unwrap, Ghidra, RTTI, translate onto an SDL port of
the engine - still applies). Installed in `A:\SteamLibrary\steamapps\common`: Build-a-lot Bundle - 4 in 1,
Build-a-lot -- The Elizabethan Era, Build-a-Lot- On Vacation, Build-a-Lot- Fairy Tales, Build-a-lot
Mysteries, Build-a-lot 10. Survey them (engine, protection, shared code between entries) before starting.

To make that cheap, the shared pieces stay game-agnostic: the portable SexyAppFramework (`framework/`),
the platform layer and mod system (`platform/`), the web shell, and the RE tooling. Game code lives in its
own folder (`games/bejeweled/src` for Bejeweled).

Already shipped as data-only mods: HD gem art (`example-crystal-gems`), single-sprite replacement
(`mines-trilliant`, `true-gem`), string overrides, Bejeweled 2 audio (`bejeweled2-audio`, generated locally).
