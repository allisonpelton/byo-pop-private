# Bejeweled 3: port plan

## The binary

`Bejeweled3.exe` (5,484,400 bytes, the Steam copy in `A:/SteamLibrary/steamapps/common/Bejeweled 3`, not wrapped) was
linked 2011-10-26 with Visual Studio 2008 SP1 and link-time code generation, so many functions take `this` or their
arguments in registers other than the usual ones (`unaff_EDI` and the like in the decompiler) and a class's functions
are scattered rather than kept in one run. It imports `steam_api.dll` (achievements, leaderboards), `bass.dll` (music
and sound) and `j2k-codec.dll` (JPEG 2000).

The export is `re/bejeweled3/gh` (the coordinator's, made with Ghidra's noreturn discovery on, which marks `operator
delete` noreturn and cuts every function short at its first delete) and `re/bejeweled3/gh2`, made again with
`MSVC70=1 CLASS_RECOVERY=0` (`tools/ghidra_import.sh`, project `bejeweled3_nr`), which this port reads. The session's
names are in `re/bejeweled3/names/*.jsonl` (git-ignored, beside the export), read by `tools/re3.py`.

The Mac build (`re/bejeweled3/mac/Bejeweled3`, version 1.1.12, Intel i386) is mostly stripped: its symbol table names
2,191 functions, most of them inline templates and a few framework and quest classes. `tools/macmatch.py` pairs 38 of
them with the PC export (15 by strings, 18 by vtables, 5 by calls; 2 of the framework's 102 named functions and 36 of
the game's 752). Zuma's Revenge's Mac build kept its symbols and has the same framework: paired with this exe it
names 399 functions, 234 of them the framework's (PopAnim, PIEffect, the resource manager, `re/bejeweled3/macmatch_zr`).
So the names come from those, the PC exe's own RTTI (746 `Sexy::` vtables), the 171 `Class::Method:line` keys and 45
source names in `properties/default.xml`, its strings, and PopCap's released 1.3 framework for the framework's API. `tools/funcmatch.py` against Twist (VS2005) pairs 1,131 functions, 163 of them named
by Twist's port: the compilers differ, so few framework bodies match byte for byte.

## The framework

The exe's framework is PopCap's post-2010 internal "prime" generation: a driver layer under SexyAppBase
(`IAppDriver`/`WindowsAppDriver`, `IGraphicsDriver`/`WindowsGraphicsDriver`, `IAudioDriver`, `IFileDriver`,
`IGamepadDriver` with XInput, `IProfileDriver`, `IResStreamsDriver`, save-game contexts), `RenderDevice3D` with D3D8 and
D3D9 behind it, `RenderEffect` (`.d3dfx`, compiled effects), `Graphics3D` with cameras and `.p3d` meshes, `DeviceImage`,
`PopAnim` (`.pam` versions 4 and 5), `PIEffect` (`.ppf` particles), `CurvedVal`, `ResStreamsManager`, a resource
manager reading ResourceGen3's manifest version 2 (composite groups by art size, `PopAnim`, `PIEffect`,
`RenderEffect` and `File` resources), and FEAST with `CfgCompiler` (only for `compat.cfg`, the graphics cards' rules,
which the port drops as Twist's did).

The port puts the game on the shared `framework/` and platform layer, as every SexyAppFramework port does; the driver
layer is the original's way to reach Windows and has no counterpart in the port. What the game needs from the prime
generation that `framework/` lacks goes into `engines/prime` (library `prime`), kept free of game code so that Zuma's
Revenge, whose 2009 framework has the same layer, links it too (`docs/ENGINES.md`): CurvedVal, PopAnim, PIEffect, the
manifest-2 resource manager and, for 3D, RenderDevice3D's drawing, the effects as GLSL and the meshes. It is merged
into main on its own as soon as it stands.

## Rendering

As in Twist, the game has a 2D mode of its own: with 3D acceleration off (the Options box, or a card that fails the
checks) it runs at 800x600 with the 600 art, shows the `2DOnly` group's flat pictures in place of the animated
backgrounds, and refuses higher resolutions. The port goes the same way: first the whole game in 2D mode on the
framework's software and GPU 2D paths, compared with the original run in 2D, then the 3D mode the original defaults
to (the effects, the meshes, hyperspace and the animated backgrounds) on the framework's GPU path.

## Milestones

1. The analysis: the export without noreturn discovery, the class map, names for the app, the board and the widgets.
2. `engines/prime`: CurvedVal, PopAnim 4 and 5, PIEffect, the manifest-2 resource manager, with headless tests of
   each against the game's files; merged into main early for Zuma's Revenge.
3. 2D mode, boot to the main menu: Bej3App, the loader screen, the main menu, profiles, options, music.
4. 2D mode, the game: Board and its subclasses (Classic, Zen, Lightning, Quest and its mini-games, Butterflies,
   Diamond Mine, Ice Storm, Poker and the secret modes), the goals, the badges, the dialogs, with every random draw in
   its order; a bot behind the `move` command.
5. The web build.
6. 3D mode.
7. The first playable merge: every mode played by the bot through real input, natively and on the web.
8. Then every phase but modding, in `docs/COORDINATION.md`'s order.

## Which copy's files

The Steam copy is the only one the user has. `main.pak` holds everything but `properties/partner.xml`, the Zen mode's
`binaural/`, `ambient/` and `affirmations/` folders and `compat.cfg`. `tools/stage_bejeweled3_assets.py` unpacks it
into `assets/bejeweled3/` with every name in lower case.
