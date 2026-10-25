# Bejeweled Deluxe

A decompiled, moddable reimplementation of **Bejeweled Deluxe 1.87** (PopCap Games, 2001; Steam build)
that runs natively on Windows and in the browser as WebAssembly. It is the first game of
[byo-pop](../../README.md). You need your own copy of the game: no PopCap assets are included in this
repository or in its builds.

- **Game code** (`games/bejeweled/src`) is a function-by-function C++ translation of the original game
  classes (`Board`, `WinDMApp`, `TitleScreen`, high-score and options dialogs). Every function is
  annotated with its address in the original executable, and gameplay (scoring, RNG order, timing,
  animations, save and high-score file formats) follows the binary.
- **Framework** (`framework/`, shared) is PopCap's public SexyAppFramework 1.3 with a new SDL2 backend:
  GPU rendering (so mods can ship high-resolution art), a software mixer with libxmp-lite for the
  tracker music, and a 100 Hz fixed update loop identical to the original.
- **Mods** (`platform/`, shared) overlay any game file, add HD images, patch single sprites, remap music
  and sounds, change strings and gameplay tunables, and keep separate high scores when gameplay changes.

## Getting the game files

The port reads the `images`, `data`, `sounds`, `music` and `properties` folders of an installed copy.

- **Native**: the executable finds a Steam install automatically, or run it with
  `-gamedir="<Bejeweled Deluxe folder>"`. For development, `python games/bejeweled/tools/stage_assets.py`
  copies those folders into `assets/bejeweled/` (git-ignored), where `build/native/bejeweled.exe` finds
  them by itself.
- **Web**: the page asks for the game folder (or a .zip of it) on first start and copies the needed files
  into the browser's storage. Nothing is uploaded.

## Building and running

See the [byo-pop README](../../README.md#building) for the toolchain. The build makes
`build/native/bejeweled.exe` and `build/web/bejeweled.html` (+ .js, .wasm). To try the web build, serve
`build/web` over HTTP and open `bejeweled.html`; automated tests open `bejeweled.html?silent=1`, which
plays no sound.

On the web, the game files, mods and saves stay in the browser's storage for that site (IndexedDB).
Saving works as in the original: quitting saves a game in progress, and the next start offers
"Continue old game?". Quit Game closes the game and offers Play again. A browser tab can also be
closed, reloaded or crash without the game hearing about it. So the page also keeps an up-to-date
copy, in localStorage, of what quitting would save at that moment, and the next start restores it.

Command-line options (native; the web shell passes the equivalent paths):

| Option | Meaning |
|---|---|
| `-gamedir=<dir>` | game data folder |
| `-userdir=<dir>` | saves, settings (`registry.ini`) and `mods.json` |
| `-modsdir=<dir>` | mods folder (default: `mods` next to the executable) |
| `-nomods` | start without mods (once: the mod menu's Restart game starts with them again) |
| `-dumpmods` | print the mods, their settings and what is waiting as JSON (the mod menu's model), then exit |
| `-seed=<n>` | fixed random seed (default: time based, like the original) |
| `-gameseed=<n>` | play the next game on seed n (1 to 4294967295): the same board every time. The seed waits until you play it: a mode switch or New Game before your first match, or a resumed saved game, leaves it for the next game; your first match or a game over uses it up. On the web: `bejeweled.html?seed=<n>`. The game-over screen shows the seed, and the scores go to a separate high-score table |
| `-script=<file>`, `-fast`, `-trace` | scripted input and screenshots for automated tests |
| `-livesplitlog=<file>` | with the `livesplit` mod (Windows): one line per write of the LiveSplit block, for tests |

## Mods

A mod is a folder in the mods directory. It contains files at the same paths the game uses
(`images/`, `sounds/`, `music/`, `data/`, `properties/`), plus an optional `mod.json`. When two mods
change the same file, the one higher in the load order wins.

Press **F2** (or, on the web, click **Mods** while the game runs) for the mod menu. It turns mods on and
off, changes their order and the settings a mod offers, and restarts the game when a change needs it. New
mods are enabled automatically. On the web, the **Mods** panel adds mod folders or .zip files and removes
them; the menu's **Add or remove mods** opens it during play. The menu keeps its choices in
`<userdir>/mods.json` and `<userdir>/mod-settings.json`, which can also be edited by hand. Mod settings are
described in [docs/MOD_MENU.md](../../docs/MOD_MENU.md); `mods/example-settings` is an example.

In Bejeweled, a setting used only in `rules`, `tunables` or `features` applies at the next fresh game, without a
restart (except `features.livesplit`). While a game is in progress or saved, the value waits, so that game keeps
its rules and its high-score table; a board nobody has played yet is made again with it. While an enabled mod
has settings, Options shows a **Mods** button that opens the menu. Restart game asks first while a game is in
progress (it is saved and offered again), keeps `-replaydir` and a `-gameseed` that has not been played yet,
and waits while a high-score name is being entered. The menu also shows each mod's rule errors and warnings.

```jsonc
{
  "id": "my-mod", "name": "My Mod", "version": "1.0", "author": "you",
  "description": "What it changes",

  // HD art: put images/gemsheet6@2x.png next to (or instead of) the 1x file, or declare a scale
  "imageScale": 2,
  "images": { "images/gemsheet6": { "scale": 2 } },

  // replace part of an image (here: one gem's two animation rows) without shipping the whole sheet
  "imagePatches": [ { "target": "images/gemsheet6", "image": "patches/gem@2x.png", "x": 0, "y": 520 } ],

  // play other files for game paths; music can start/loop anywhere and use Ogg instead of S3M
  "files": {
    "music/1984-network.s3m": { "file": "music/track.ogg", "start": 0, "loopStart": 12.5, "volume": 0.8 },
    "sounds/go1.ogg": "voice/go.ogg"
  },

  // replace on-screen text (ids from properties/default.xml), or list extra properties files
  "strings": { "EXCELLENT": "AWESOME!" },

  // gameplay tunables (see games/bejeweled/src/GameConfig.h); mods that use them get their own
  // high-score table
  "tunables": { "scoring": { "hintPenalty": 25, "setValues": [10, 20, 30, 40, 60, 80, 110, 160, 210] },
                "board": { "gemColors": 6 } }
}
```

Images follow the SexyAppFramework rules: `name.png`/`.jpg`/`.gif` with an optional `_name.*` alpha mask.
A mod's mask is only applied to the mod's own image, and a PNG with transparency needs no mask.

### Included mods and generators

The mods live in `games/bejeweled/mods/`; the generators write there by default.

| Mod | How to get it |
|---|---|
| `example-crystal-gems`: new HD gem art (original artwork) | `python games/bejeweled/tools/make_example_mods.py` |
| `example-voice-lines`: string overrides | same script |
| `example-relaxed`: cheaper hints, separate high scores | same script |
| `example-settings`: the hint cost, when the Timed bar flashes, replays and the big-move message, set in the mod menu ([docs/MOD_MENU.md](../../docs/MOD_MENU.md)) | in the repository |
| `livesplit`: LiveSplit auto splitter support (Windows): the block the auto splitter reads, 12 split templates and the script ([tools/livesplit](tools/livesplit/README.md)); changes nothing in play or on screen | same script (`--only livesplit`) |
| `mines-trilliant`: orange gem as a trilliant cut shaped like the Colorado School of Mines triangle | `python games/bejeweled/tools/make_trilliant_mod.py` |
| `true-gem`: every gem becomes a badly recoloured "True..." meme card, the logo reads BE TRUE LED, EXCELLENT! becomes TRUE... and NO MOVES! becomes NO TRUE... | `python games/bejeweled/tools/make_true_mod.py --image <picture>` |
| `hd4x-textures`: every image at 4x, upscaled from your own game files with Real-ESRGAN (needs [realesrgan-ncnn-vulkan](https://github.com/xinntao/Real-ESRGAN/releases) and a Vulkan GPU). Shows its detail in a large window or fullscreen | `python games/bejeweled/tools/make_hd4x_mod.py` |
| `bejeweled2-audio`: Bejeweled 2's music and sound effects (needs your Bejeweled 2 Deluxe install and an ffmpeg build with libopenmpt) | `python games/bejeweled/tools/make_bej2_audio_mod.py` |
| `bejeweled2-gems`: the seven gems redrawn in the style of Bejeweled 2, HD (@4x) with spin and hover animations, and the same gems over the five on the title screen (leave those out with `--no-title`). New procedural art that reads no game files, but a close recreation of another game's look | `python games/bejeweled/tools/make_bej2_gems_mod.py` |

Mods built from other games' files or third-party pictures, or closely recreating another game's art, are generated locally and git-ignored.

When any mod is enabled, two small signs show it: hints get a fourth sparkle, and the G in banner text
(GO!, ...) gets a crossbar. A mod that only turns on LiveSplit support shows neither, since it changes
nothing in play.

HD art (`name@2x.png` .. `name@4x.png`) also reaches images the game builds at run time, such as the
board background, the banner text, the shrinking gems and (at render scales above 1x) the hovered gem. Software drawing repeats each draw on the HD
pixels whenever HD art is involved (`MemoryImage::BeginHDMirror`).

Planned gameplay mods (board shapes, other move types, special gems, replays, ...) are listed in
[docs/MOD_ROADMAP.md](../../docs/MOD_ROADMAP.md); their design and the staged plan are in
[docs/CODE_MODS_DESIGN.md](docs/CODE_MODS_DESIGN.md). Some already play: swaps that make no match can stay
(`"moves": {"allowInvalid": true}`, with the `noMoves` rule), and undo (`"undo": {"enabled": true}`, with `depth`,
`max`, `penalty`, `inTimed`, `restoreTime` and `restoreRng`). With undo, **z**, **u** or **Backspace**, or a click on
the red orb left of the timer bar, takes the last move back. The orb lights up while an undo is possible and shows
how many are left when `max` limits them. Repeating the move refills the board the same way, so undo cannot reroll
the new gems.

## Tests

- `build/native/rulestest.exe` (and `build/web/rulestest.js` under Node): the code-mod rules fixtures
  and RulesId vectors (`games/bejeweled/tools/rulestest`); native and wasm must print the same vectors.
- `games/bejeweled/tools/oracle/compare.sh`: the regression oracle. It plays scripted and bot games with
  the frozen baseline exes in `build/baseline*/` and the candidate (`build/native/bejeweled.exe`) and
  compares logs, saves, screenshots and state hashes. It runs every game on an invisible desktop
  (`tools/rundesk`); see the repository's `CLAUDE.md` for the rules on test runs. Two more candidates
  must match the baselines too: the game with every variant path forced on
  (`compare.sh --candidate-args -forcevariant --out build/oracle-forcevariant`) and the build without
  them (`tools/build.sh novariants`, then
  `compare.sh --candidate build/native-novariants/bejeweled.exe --out build/oracle-novariants`).
  `compare.sh --variants default,forcevariant,novariants` runs all three in one go. It runs three games at
  a time (`--jobs`) and keeps the frozen baselines' runs in `build/oracle-cache`, so after the first run
  only the candidates play; `--no-cache --jobs 1` runs everything now, one game at a time.
- `build/web-test/` (`tools/build.sh web-test`): the web build for automated browser tests, served on
  port 18766 by `.claude/launch.json`'s `web-test` server and opened with `?silent=1`. It is a different
  origin from `build/web` (port 18765), so test runs keep their browser storage apart from your own.
- `games/bejeweled/tools/check_rng_use.py` and `list_variant_hooks.py`: the randomness rules and the
  code-mod hooks in the translated code.
- `build/native/bejeweled_verify.exe` and `build/web/bejeweled_verify.wasm`: the headless replay verifier,
  the game's own board code without window, sound or game files (`games/bejeweled/src/headless`).
  `bejeweled_verify <replay.bjr> [-trace]` plays a replay as the game's `-replay` does and prints its
  verdict as JSON; `bejeweled_verify difftest --seeds 1..500` plays bot games on the plain and the
  `-forcevariant` board side by side. `node games/bejeweled/tools/verify/wasmrun.js` runs the wasm verifier
  under Node, and the game's web build with the native test switches (`-script`, `-tracehash`,
  `-autoplay`, ...); `games/bejeweled/tools/verify/run_tests.sh` holds the verifier's own tests (its broken and
  extreme replays come from `craft_replays.py`), and the oracle checks that the game, both verifiers and the web
  build agree.
- `games/bejeweled/tools/livesplit/run_tests.sh`: the LiveSplit auto splitter without LiveSplit: games
  probed from outside by `build/native/lsprobe.exe` as the script reads them, the script compiled as
  LiveSplit compiles it (the .NET Framework's C# 5 compiler) and its start, split and reset decisions in
  27 scenarios ([tools/livesplit/README.md](tools/livesplit/README.md)).

## How it was reverse engineered

See [docs/RE_NOTES.md](docs/RE_NOTES.md). In short:

1. `WinBej.exe` is PopCap's DRM wrapper; the real game is an unencrypted PE appended to it.
   `python tools/extract_game_exe.py` carves it out to `re/bejeweled/bin/BejeweledGame.exe`.
2. Ghidra 12 analyses it headlessly, recovers the C++ classes from RTTI and exports every decompiled
   function (`tools/ghidra_scripts/ExportAll.java`, run by `tools/ghidra_headless.sh`).
3. The framework functions the game calls were identified against the public SexyAppFramework source
   ([docs/FRAMEWORK_NOTES.md](../../docs/FRAMEWORK_NOTES.md)); the game classes were analysed into a
   behaviour spec ([docs/GAME_NOTES.md](docs/GAME_NOTES.md)). Recovered names are applied back to Ghidra
   with `tools/ghidra_scripts/ApplySymbols.java` (`re/bejeweled/symbols/*.jsonl`).
4. The game code was translated against that spec and checked function by function against the
   decompilation and disassembly.

`re/bejeweled/` (decompiler output) is git-ignored; regenerate it with the tools above. The research
behind the approach (comparable projects such as Zuma's CircleShootApp and PvZ-Portable) is summarized
in [docs/RESEARCH_SYNTHESIS.md](../../docs/RESEARCH_SYNTHESIS.md).

## Licenses and credits

See the [byo-pop README](../../README.md#licenses-and-credits). Bejeweled is a trademark of Electronic
Arts / PopCap Games; this project is not affiliated with or endorsed by them, and the game's code, art,
music and sounds remain their property.
