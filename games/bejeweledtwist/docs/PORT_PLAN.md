# Bejeweled Twist: port plan

## The binary

The game is `BejeweledTwist.exe` v1.0.3.7482, linked 2008-10-17 with Visual Studio 2005 (partial LTCG), on an internal
mid-2008 generation of SexyAppFramework. Both of the user's copies hold the same inner image: the retail one inside
PopCap's CDProtect launcher and the Steam one inside the 2008 DRM wrapper (the Steam copy is patched in three places,
survey section 5 item 7). The analysis uses the retail one, carved by `tools/extract_game_exe.py` into
`re/bejeweledtwist/bin/BejeweledTwist.exe` (4,984,832 bytes); the staging script carves the same image into
`assets/bejeweledtwist/bejeweledtwist.exe`, where the port reads the game's texts by address.

The Ghidra project is `E:/retools/projects/bejeweledtwist_game`, made by `tools/ghidra_import.sh` with `MSVC70=1` (which
only turns off the noreturn discovery: on this exe it marks `_free` noreturn and cuts every function short at its first
free, as it did on BookWorm Deluxe) and `CLASS_RECOVERY=0`. The export is in `re/bejeweledtwist/gh/` (21,091 functions,
746 vtables). RTTI is present, so the classes are placed by the slots of their vtables; the functions are not yet named.

## Where things are

Visual C++ keeps each object file's functions together, so the classes sit in runs (by the middle of each class's own
vtable slots, `tools/classmap.py`):

| Range | What |
|---|---|
| 0x401000-0x4bd000 | SAF: D3DTester, DDImage, Graphics, Image, ImageFont, SysFont, XMLParser, PropertiesParser, InternetManager, ResourceManager, DescParser, EncodingParser, SexyAppBase (0x493db0-0x4a8420), SexyApp |
| 0x4bd000-0x4d3000 | SAF: the 3D layer's interfaces (D3DInterface, D3DObject, WorldGraphicsListener), the widgets, Dialog, FlashWidget |
| 0x4d3000-0x4d8600 | PopAnim (PopAnimModParser, PASpriteInst) |
| 0x4e5900-0x4e9900 | FEAST's CfgCompiler (only for `compat.cfg`; dropped) |
| 0x4f1000-0x500000 | ImageLib, the sound managers (DSound, BASS, FMOD), the music interfaces |
| 0x500000-0x60d000 | shaders' interfaces, DDInterface, BigInt, and the libraries (zlib, libpng, libjpeg, Ogg Vorbis and others; to be told apart) |
| 0x60d000-0x633000 | D3D7Interface, DXSurface7/8/9, D3D9Interface, D3D8Interface, the meshes |
| 0x634000-0x636000 | PakInterface |
| 0x660000-0x694000 | the visualizers (Zen mode's backgrounds and the menu's, about 30 classes) |
| 0x694000-0x6a3d00 | HighScoresPanel, HelpPanel, NewUserPanel, Menu, MenuButtonWidget, OptionsPanel, ProfilePanel, AwardTelly |
| 0x6a3d00-0x6d6900 | Board (about 200 KB) |
| 0x6d9000-0x6e7000 | ChallengeInfoParser, ChallengeButton, ChallengeMenu |
| 0x6e7000-0x6f5000 | Credits, CustomBassMusicInterface, CustomResourceManager, CustomSound* |
| 0x6f5000-0x70e500 | Effect, EffectsManager, EndStats, FlyingText, FruitAwardScreen, GameOverShow, Logo, Gates, GemSoul, RankBadge, GoodTube, GraphicsRecorder |
| 0x70e500-0x718000 | Hyperspace (the level-up warp, the 3D sidebar, the planets, the saucer, the world cube) |
| 0x71c600-0x737a00 | LevelUpShow, LoaderScreen, ModeButton, Mandala, OrbitText, SpeedPath, Piece, Points, PointsManager, PuzzleUI, RankBonusWidget |
| 0x739b00-0x75e500 | RegisterDialog (trial, left out), Shockwaves, SpeedText, Travellers, Tutorial, the Twist widgets and dialog, VizManager, Viz2D, WheelSpin |
| 0x75e900-0x768d00 | WinTwistApp (the app), TwistP3DListener, WorkerThread |
| 0x769000- | the C runtime |

## Rendering

The original draws through Direct3D 7, 8 or 9 (a P3D mesh renderer, fixed-function "shaders" written as classes, and
ps_2_0/vs_2_0 shaders for the sidebar, the cubes, the whiteout and the dialogs), and it has a real 2D mode of its own:
`-noacceleration` or the Options switch, which the game warns against. In 2D it stays at 800x600, uses the `2DOnly`
resources (flat pictures for the planet and the space backdrop) and turns the visualizers off. The framework of this
repository draws in software.

So the port goes in two steps. First the game in its own 2D mode, which the software framework can draw as the
original does; that is a complete game, and it is compared with the original run with `-noacceleration`. Then the 3D
mode, which is what the user plays: it needs a renderer for meshes and the shader effects (in software, or a GPU path
through SDL/WebGL in `framework/`, a shared change agreed with the coordinator first).

## Milestones

1. The analysis: the export, the class map above, symbols for the app, the board and the widgets (`docs/symbols/`).
2. 2D mode, boot to the main menu: WinTwistApp, the loader, CustomResourceManager (the three art sizes), the logo, the
   menu and its panels, the Twist dialogs, options, profiles (with the site's players, `framework/WebProfiles.h`, and
   the site settings, `framework/WebSettings.h`), the music (the two MO3 modules with their offsets file, libopenmpt).
3. 2D mode, the game: Board, Piece, the effects, the points, the level-up show, game over, the end stats, high scores,
   in each mode (Classic, Blitz, Zen and the challenge modes of `challenges.xml`), with every RNG draw in its order.
4. The web build (port 18794).
5. 3D mode: the meshes, the sidebar, Hyperspace, the planets, the visualizers.
6. Done means identical: the deviations listed, a frozen baseline with a regression check, every leak of the original
   fixed, and a screen-by-screen comparison with the original, natively and in the browser.

## Which copy's files

`main.pak` is the same in both copies, as are 41 other files; they differ in `properties/` (the coordinator's comparison,
2026-10-31). The Steam copy's `resources.xml` (2018-06-23) moves the 47 Zen visualizer drops (`zendrop*`) from
`images\1200\NonResize\visualizers\` to `images\NonResize\visualizers\`, changes four sound volumes (chainx1,
challengecompleted, gem_countdown, trans_engineignition) and the flags of two images; its `partner.xml` names Steam as
the partner, drops registration and sets the marketing mode `LevelLock` (empty in retail). The retail `compat.cfg` is
the original (the Steam one has a local edit), and the retail folder's WineD3D wrapper DLLs and `crash.txt` are not
part of the game.

The staging script takes the retail copy first, so the port follows the retail properties for now: they belong to the
image being decompiled. In the retail `resources.xml` every path resolves in `main.pak` except the 47 zendrop ones,
which are only in `images\NonResize\visualizers\`. Whether the retail game finds them anyway depends on how
CustomResourceManager rewrites the art folders (to be read); in 2D mode the visualizers are off, so it matters from
milestone 5. Before the final comparison the port must decide which copy's properties it matches (the one the user
plays), since `LevelLock` and the volumes change what a player sees and hears.
