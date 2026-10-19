# Big Money Deluxe: translation plan

How the original's game code (`INVENTORY.md`, behaviour in `GAME_NOTES.md`) becomes the
port in `games/bigmoney/src/`, on the shared portable SexyAppFramework and platform layer.

## 1. Approach

The same method as Bejeweled (`games/bejeweled/src`, `games/bejeweled/docs/GAME_NOTES.md`):

1. **Analysis per module group.** Behaviour specs with class layouts at their original offsets, every
   function, and every random draw in call order. Five groups: Board rules, Board drawing plus
   MrMoney/Points/OverlayWidget, the app (WinBMApp, MoneyDialog, LockableButton, resources), the screens
   (TitleScreen, StartButton, GameSelector, HelpWidget, CreditsDialog), and the dialogs (high scores,
   name entry, options). They are consolidated in GAME_NOTES.md.
2. **Translation, function by function.**
   - Each function is marked `// 0xADDR`, and each class names its original size and vtable.
   - Fields keep the original order, with the offset in a comment. Code never depends on the offsets,
     because the port's base classes have other sizes.
   - Deviations are marked `// port:`.
3. **Integration**: one CMake target (`bigmoney`, native and web), built with the rest of the repository.
4. **Verification** (section 6).

## 2. Classes and files

| Original class (source file) | sizeof | Port file(s) | Notes |
|---|---|---|---|
| WinBMApp (WinBMApp.cpp) | 0x828 | `WinBMApp.h/.cpp`, `Res.h` | static global app object; resource tables are hard-coded in the exe (71 images, 48 sounds, 14 songs of one `.it`, 13 fonts from 5 descriptors) |
| Board (Board.cpp) | (board notes) | `Board.h`, `Board.cpp` (rules), `BoardDraw.cpp` (drawing) | Board.cpp is split in two files because it is the largest module (9.7K decompiled lines) |
| Piece (Board.cpp) | | `Piece.h` | a coin / bag / bomb cell |
| MrMoney, Points, OverlayWidget | | `MrMoney.*`, `Points.*`, `OverlayWidget.*` | widgets drawn over the board |
| MoneyDialog, LockableButton | 0x158, 0x120 | `MoneyDialog.*`, `LockableButton.*` | MoneyDialog is the base of the game's dialogs (running lights) |
| TitleScreen, StartButton | 0x118, 0x138 | `TitleScreen.*`, `StartButton.*` | |
| GameSelector, HelpWidget, CreditsDialog | 0xe0, 0xa0, 0x168 | `GameSelector.*`, `HelpWidget.*`, `CreditsDialog.*` | game type (Action / Strategy / Puzzle) and skill 1..3; per-mode how-to-play; credits over drifting clouds |
| HighScoreWidget, HighScoreDialog, RecordScoreDialog, OptionsDialog | 0x304, 0x160, 0x168, 0x198 | same names | `hiscores.dat` is owned by HighScoreWidget |
| (none) | | `Compat12.h/.cpp` | 1.2 / MSVC behaviour the port's framework or host C library does differently (section 4) |
| (none) | | `main.cpp` | finds the game data, sets up the VFS, user dir and mods, runs the app |

**Not ported:**
- RegisterDialog, UpdateCheckDialog, InternetManager and BetaSupport.
- The trial, registration, update-check, internet high-score and Zylom branches. The Steam build is
  registered (`properties/partner.xml` NoReg=true, DontUpdate=true), so the port always takes the
  registered path. Each dropped branch keeps a one-line `// 0xADDR ... not ported` note.

## 3. Platform mapping

| Original | Port |
|---|---|
| DirectDraw / D3D7 drawing (DDImage, DDInterface) | the port's SDL2 renderer behind the same Graphics / Image / DDImage API |
| DirectSound sounds (`.ogg`, `.au`), Ogg decoded by Tremor (SAF 1.2 `USE_OGG_LIB`) | `SDLSoundManager` + `Platform::Mixer`. The Ogg files use Vorbis floor 0 (2002 encoder), which stb_vorbis cannot read, so they go through the same Tremor (`third_party/tremor`). `.au` mu-law is decoded with the framework's own table (`Mixer::LoadSoundData`) |
| BASS 2.0 music (one `.it` module, songs by order position) | `SDLMusicInterface` + libxmp-lite, which supports `PlayMusic` / `FadeIn` order offsets (`MAKELONG(order, row)`) |
| `HKLM\SOFTWARE\PopCap\BigMoney` (direct Win32 calls in WinBMApp) | the framework's emulated registry (`registry.ini` in the user dir), same value names |
| `savegame.dat`, `hiscores.dat` next to the exe | the user dir via the framework's file helpers; byte formats unchanged, so original files load |
| loading thread | synchronous `LoadingThreadProc` (as in Bejeweled; required on the web) |
| `srand(GetTickCount())` in 1.2's SexyAppBase::Init; game `rand()` calls | `CrtSRand`/`CrtRand` in `Compat12.h`: MSVC's LCG, identical on native and web; seeded by WinBMApp::Init (tick count, or `-seed=N`) |
| Win32 cursors (images 0-3) | framework custom cursors |
| `WinMain` | `main.cpp` (`Init`, `Start`, `Shutdown` on one app object, as the original) |

## 4. Framework differences: SexyAppFramework 1.2 vs the port (1.3)

The port's framework was written for Bejeweled (a 1.3-generation build). The 1.2 sources were compared
with it for every class Big Money uses:

| Area | Difference | Handling |
|---|---|---|
| vtable layout / field offsets | 1.2 slot order and layouts (FRAMEWORK_12.md); Big Money's build also lacks two virtuals and four SexyAppBase fields | analysis only: the translation calls methods by name |
| Mouse / key virtuals | identical overload sets (`MouseDown(x,y,clickCount)`, 4-argument forms) | none |
| Dialog, DialogButton | 1.3 looks up `ID_YES`/`ID_OK`... strings (falling back to the same texts); DialogButton's 1.3 over-fade is off by default | none |
| ButtonWidget | image-less buttons: 1.2 hard-codes the outline and label colours; 1.3 reads them from `mColors` with the same defaults | none unless the game sets custom colours on a plain button (checked per translation) |
| Slider | 1.3 clicking the track jumps the thumb; 1.3 `SetValue` clamps | `Slider12` |
| EditWidget | Latin-1 word characters, tablet caret | none |
| ImageFont | 1.3 adds `LayerSetHeight` and integer colour values; same rendering | none; every command in Big Money's descriptors is supported |
| Graphics::WriteString colour codes `^RRGGBB^` | 1.3 adds `^oldclr^` and fixes `^^` at the end of a string | none (no such strings) |
| SysFont fallbacks | the port has no SysFont; the game always sets ImageFonts | none |
| C runtime `rand()` | host `rand()` differs (Emscripten) | `CrtRand` |

## 5. Data and mods

- Assets are staged from the player's own install with `games/bigmoney/tools/stage_bigmoney_assets.py`
  into the git-ignored `assets/bigmoney/`. The web build imports the player's folder into its own
  IndexedDB folder, `/bmpersist`.
- Data mods work as in Bejeweled through the shared VFS and ModManager: replacement images (including
  `@2x` HD art), sounds, music, fonts and `properties/default.xml` strings. Mods live in `mods-bigmoney/`
  next to the exe, or in the web shell's Mods panel.
- Code mods for Big Money come after the port is complete (MOD_ROADMAP "After Bejeweled").

## 6. Verification

- **Scripted native runs under `build/native/rundesk.exe`** (CLAUDE.md), using the framework's
  `-seed=N -script=<file> -fast` hooks and `screenshot` commands. The runs cover:
  - the title screen through the selector into each mode;
  - play: clicks on coin groups, bag drops, level-up and game over;
  - a high-score entry, options and help;
  - save on quit and continue.
- Screenshots are compared against the behaviour spec: positions, frames and texts.
- Determinism: two runs with the same seed and script must give identical screenshots.
- The original exe is not run (BASS sound cannot be silenced, and it goes through the DRM). If a
  behaviour can only be settled by watching the original, the user is asked.
- `savegame.dat` / `hiscores.dat` written by the port must parse with the original's format; the
  user's existing `hiscores.dat` must load. That is checked with a copy in a scratch user dir; the
  install is never touched.
- Web: the same build in the Browser pane with `?silent=1`. Checks: importing the game folder, saves
  surviving a reload, and the single-instance lock.
