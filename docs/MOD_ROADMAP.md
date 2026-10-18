# Mod roadmap

Gameplay mod ideas to implement once the decompiled game and the mod menu are fully working. Each needs
a hook in the translated `Board` code; the translation keeps the original behaviour as the default and
reads these parameters from mod tunables instead of hard-coding them.

| Idea | What it needs in the engine |
|---|---|
| Allow invalid moves | Swap rule flag: keep a swap that makes no match instead of swapping back (and decide whether it counts as a move / costs time). |
| More gem colors | Gem-type count above 7: extra rows in the gem sheet (image patch or a taller `gemsheet6`), colour list for effects that tint by gem, RNG `% n` with the new count. |
| Board size and arbitrary shapes | Width/height parameters and a cell mask (holes) instead of the fixed 8x8 grid; layout/centering, gravity that skips holes, refill per column, hit testing. |
| Dirt tiles (Jewel Quest) | Cells that hold no gem and block falling gems. A match next to one removes a layer, and gems slide past diagonally, alternating left and right. Design: docs/CODE_MODS_DESIGN.md 4.14. |
| Chuzzle-style play | New move type: drag a whole row or column with wrap-around; match check after release, revert if no match (unless invalid moves are allowed). |
| Bejeweled Twist-style play | New move type: rotate a 2x2 group clockwise around a grid intersection; cursor between cells. |
| Jewel Quest-style "gold board" mode | Per-cell state layer under the gems (plain / gold); every cell covered by a scored run turns gold, and the level is won when all cells are gold, instead of filling the bonus bar (Jewel Quest also gives each board a time limit and odd board shapes, so it builds on the board-shape and timer hooks). Needs: the cell layer saved with the game, gold tile art drawn in place of the checkerboard (mod-supplied, e.g. a `goldtile` image), a progress display (cells left), and level rules (time limit, what happens on no moves). |
| Custom match patterns | Pluggable match finder: minimum run length (e.g. 2), shapes (L of three), diagonal lines; scoring per pattern. |
| Flame and lightning gems | Special gem kinds created by 4- and 5-matches, with their own sprites/animations, explosion (3x3) and row+column clears, cascades through the existing scoring. |
| Set seed | All game randomness is `Sexy::Rand()` (MT19937), seeded from the tick count in `SexyAppBase::Init`; expose the seed (options / command line / URL) and show it on the game-over screen. |
| Save and watch replays | Not a player feature in the original, but its framework had a developer demo recorder (`-record`/`-play`/`-demofile`; seed + timestamped input). The port drops that code; a replay = seed + mode + per-tick input log, played back through the same 100 Hz update loop. Needs a replay browser UI. |
| Online leaderboards | Host the web build on a Cloudflare Worker (static assets + API) with a D1 database for scores, one board per mode and per gameplay mod set (the existing ModManager score key). Submissions carry a replay; the Worker verifies it by re-running the game logic headlessly (the game compiled to WASM without SDL video/audio, fed the replay's seed and input log at 100 Hz) and accepts the score only if it reproduces. Needs: the replay format (see "Save and watch replays"), a headless build target, rate limiting, and a name/identity scheme. A Cloudflare account is connected to this environment (D1/Workers tools), so setup can be done from here when we get to it. |
| Undo move | Snapshot board + score/level/timers + RNG state before each swap (the save-game writer already serializes the board); restore on undo. Decide on scoring (separate high scores, like other gameplay mods). |

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
   wrapper (`!popcapdrmprotect!` at 0x53000, SexyAppFramework RTTI), so `tools/extract_game_exe.py`, the
   Ghidra pipeline (`tools/ghidra_scripts`, `tools/ghidra_headless.sh`) and the framework-identification
   approach carry over. A separate `BigMoney.exe` (2005-11-14, no framework RTTI) needs a look first.
2. **Other PopCap games**, in the order agreed on 2026-10-18 (details, effort and prerequisites in
   [POPCAP_GAMES_SURVEY.md](POPCAP_GAMES_SURVEY.md), section 4):
   1. BookWorm Deluxe. It is now also the first second-game target, so it takes on the multi-game
      layout groundwork (survey G6).
   2. Typer Shark! Deluxe, which uses the same 2004 framework.
   3. Bookworm Adventures Deluxe + Volume 2.
   4. The Sprout-engine line: Feeding Frenzy 2 + Talismania, then Iggle Pop. The public Pizza Frenzy
      decompilation serves as an engine reference only.
   5. Bejeweled 2 Deluxe.
   6. The Peggle family. Thunderball is a partial decompilation (about 39%), so Peggle stays in the order.
   7. The remaining classic titles: Rocket Mania, Hammer Heads, Heavy Weapon, Dynomite, AstroPop.
   8. The newer frameworks: Bejeweled 3, Bejeweled Twist, Zuma's Revenge.
   9. Chuzzle Deluxe.

   **Skipped for now:** games that others have already decompiled. Revisit them after every other
   game is done:
   - Insaniquarium Deluxe ([Vindirect/WinFish](https://github.com/Vindirect/WinFish))
   - Zuma Deluxe ([alula/CircleShootApp](https://github.com/alula/CircleShootApp))
   - Plants vs. Zombies ([wszqkzqk/PvZ-Portable](https://github.com/wszqkzqk/PvZ-Portable))
   - Pizza Frenzy ([PopCap-Decompilations/PizzaFrenzy-Decompilation](https://github.com/PopCap-Decompilations/PizzaFrenzy-Decompilation))

## Very far out: the Build-a-Lot series

Decompilations of the full Build-a-Lot series (HipSoft, not PopCap, so a different engine and no
SexyAppFramework to lean on; the general pipeline - unwrap, Ghidra, RTTI, translate onto an SDL port of
the engine - still applies). Installed in `A:\SteamLibrary\steamapps\common`: Build-a-lot Bundle - 4 in 1,
Build-a-lot -- The Elizabethan Era, Build-a-Lot- On Vacation, Build-a-Lot- Fairy Tales, Build-a-lot
Mysteries, Build-a-lot 10. Survey them (engine, protection, shared code between entries) before starting.

To make that cheap, the shared pieces stay game-agnostic: the portable SexyAppFramework (`src/SexyAppFramework`),
the platform layer and mod system (`src/platform`), the web shell, and the RE tooling. Game code lives in its
own folder (`src/game` for Bejeweled).

Already shipped as data-only mods: HD gem art (`example-crystal-gems`), single-sprite replacement
(`mines-trilliant`, `true-gem`), string overrides, Bejeweled 2 audio (`bejeweled2-audio`, generated locally).
