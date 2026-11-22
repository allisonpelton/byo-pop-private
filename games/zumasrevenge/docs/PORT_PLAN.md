# Zuma's Revenge!: port plan

## The binary

The Steam copy (`A:/SteamLibrary/steamapps/common/Zuma's Revenge`) starts the 2008-10-08 wrapper `ZumasRevenge.exe`;
the game inside it is `re/zumasrevenge/bin/ZumasRevengeGame.exe` (6,651,904 bytes), linked 2009-08-14 with Visual
Studio 2005 and link-time code generation (its internal name is Zuma2). Link-time code generation passes `this` and
arguments in whatever register suits the caller, so the decompiler shows many calls without their arguments
(`std_string_assign_n()` with the text in a register): read such a call in the caller's assembly (`zr.py asm`). The
exe imports `steam_api.dll` (achievements and statistics), `bass.dll` (sound and music), `j2k-codec.dll` (JPEG 2000)
and WinInet (the crash upload and the internet manager).

The export the port reads is `re/zumasrevenge/gh2` (project `zumasrevenge_nr`, made with `MSVC70=1 CLASS_RECOVERY=0`,
`tools/ghidra_import.sh`): 27,555 functions. The older export `re/zumasrevenge/gh` (project `zumasrevenge_game`) was made
with Ghidra's noreturn discovery on, which cut 9,178 functions short at a call it wrongly took as never returning; the new
one has 85 such warnings. RTTI class recovery is not run: it went 9.7 hours on this exe without finishing.

Names, in the order `tools/zr.py` lays them over each other:

- the Mac build (`re/zumasrevenge/mac/ZumasRevenge`, version 1.0, Intel i386, symbols kept): `tools/macmatch.py` pairs
  1,692 functions (944 of the game's 3,560 Mac functions, 567 of the framework's 1,648), in
  `re/zumasrevenge/macmatch/zumasrevenge.match.jsonl`;
- Bejeweled Twist (VS2005 too): `tools/funcmatch.py` pairs 2,929 functions (1,541 byte for byte), 688 of them named by
  Twist's port, in `re/zumasrevenge/macmatch/twist.match.jsonl`;
- the session's own names, `re/zumasrevenge/names/*.jsonl`.

All of these are the user's game and stay git-ignored under `re/`.

## The framework

The exe's framework is PopCap's 2009 internal generation: SexyAppBase and SexyApp with a registry and properties layer,
RenderDevice/RenderDevice3D over Direct3D 8 and 9, DXSurface, `.fx` effects through a dynamically loaded d3dx9, D3D
meshes, the ResourceManager of ResourceMan2's manifests, PopAnim, PIEffect, CurvedVal, CfgCompiler with FEAST (only for
`compat.cfg`, the graphics cards' rules, which the port drops as Twist's and Bejeweled 3's do), the BASS music
interface, MoreGamesAd (an outbound button: left out) and the crash reporter (left out). Beside it the game links three
libraries with no public source: JeffLib (helpers: dialogs, the user selection, the animator, the sound
player, the bonus text, XML config, oscillators, the command console), PIL (a particle library: systems, emitters,
particle types, forces, deflectors, waypoints, colour keys) and AELib (`.pax` After Effects compositions).

The port puts the game on the shared `framework/` and platform layer, as every SexyAppFramework port does, and uses
`engines/prime` for what this generation shares with Bejeweled 3 (CurvedVal, PopAnim, PIEffect, the hermite and spline
maths; `docs/ENGINES.md`). Each function there carries both games' addresses; where this game's version differs, the
game sets which. A change to `engines/prime` is a shared change, told to Bejeweled 3's session first. JeffLib has two
copies in the Bookworm Adventures ports (`games/bookwormadv/jefflib`, `games/bookwormadv2/jefflib`): this game's version
is later and larger, and moving one copy to `engines/jefflib` for all three goes to the coordinator as a spec when the
game first needs it. PIL and AELib only this game has; they live in `src/pil` and `src/aelib`.

## Rendering

The game has two display paths. With 3D acceleration off it runs at 800x600 with the 600 art on the DirectDraw path
(DDImage and the software blitters). With it on it draws through RenderDevice3D, and only then offers "Hi-Res
(1920x1200)", the 1200 art in a widescreen layout (WidescreenBoardWidget). The shader effects (lava, water,
distortion, the shadow canopies, fog) need the 3D device; what the 2D path shows in their place is still to be read. The manifest names every picture under `images\1200\`; the art size swaps the folder.
The port goes as Bejeweled 3's does: first the whole game at 800x600 with the 600 art, compared with the original run
in that mode, then the 3D mode and Hi-Res on the framework's GPU path with the seven `.fx` effects as GLSL.

## Milestones

1. The analysis: the export without noreturn discovery, the Mac and Twist names, `tools/zr.py`; the class map; the
   boot read through (GameApp's constructor, Init, InitHook, the loading thread, the registry).
2. Boot to the main menu at 800x600: GameApp, the resource manager (ResourceMan2's manifest, the art-size folders), the
   loading screen, the main menu, the profiles and JeffLib's user dialogs, the options, the music (one segmented MO3
   through libopenmpt) and sounds.
3. The game: LevelMgr and the levels (`levels/*/*.dat`, "CURV" version 15 curves), CurveMgr and the way points, Board,
   Ball, Gun, the bullets and power-ups, the scoring, the adventure map, the challenge (Gauntlet) mode, Iron Frog, the ten
   bosses and the Tiki Temple, every random draw in its order; a bot behind the `move` command, and the standard test
   commands (`docs/TEST_COMMANDS.md`).
4. The web build.
5. 3D mode and Hi-Res.
6. The first playable merge: every mode played by the bot through real input, natively and on the web.
7. Then every phase but modding, in `docs/COORDINATION.md`'s order.

From the first draft the port follows `docs/CODE_STANDARDS.md`, leaves out the registration paths, the crash reporter,
the online statistics and every button that leads out of the game, and translates no function the export shows nothing
reaches (each listed by address in the notes).

## Which copy's files

The Steam copy is the only one. `main.pak` (XOR 0xF7) holds everything but `levels/` and `properties/partner.xml`;
`tools/stage_zumasrevenge_assets.py` unpacks it into `assets/zumasrevenge/` with every name in lower case, with the
loose folders and the unwrapped exe (for its texts and tables, never run by the port).
