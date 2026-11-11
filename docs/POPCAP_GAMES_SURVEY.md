# PopCap games survey: the facts about each game, and the plan for those still to port

The facts about each PopCap game on the user's machines, and a few that PopCap published or that run on its
engines, with the plan for the games still to port. The rules (which games, in what order, how many sessions at
once, which model) are in [COORDINATION.md](COORDINATION.md), "Which games, in what order"; this document gives
the facts behind them. Six of the first games went through the full Ghidra pipeline; the others were read
statically or through `tools/triage_popcap.py`, so their code sizes are address-range estimates. Bejeweled
Deluxe, the first port, is the baseline every effort is measured against.

## 1. Where things stand

Twenty-five games are ported, each in its own folder under `games/`; the
[progress table in README.md](../README.md#games) holds each one's status and cost, and its own docs hold what
its port found. Here a ported game keeps only its row in section 4. Fifteen games remain (section 2), and none
of them is decompiled yet.

Not ported, and why:

| Games | Reason |
|---|---|
| Plants vs. Zombies, Insaniquarium Deluxe, Zuma Deluxe | large fan communities with finished decompilations, many ports and active modding: left to their own work (the user, 2026-10-26) |
| Peggle Extreme, Peggle World of Warcraft Edition | they become settings of the Peggle Deluxe and Nights ports (section 2) |
| Chuzzle: Christmas Edition | mostly changes Chuzzle's art |
| The Wizard's Pen and every other hidden-object game (Mystery P.I., Amazing Adventures, the Escape games, Vacation Quest, Hidden Identity: Chicago Blackout) | the user ruled the genre out, since it has almost no replay value (2026-10-25) |
| Sweet Tooth To Go | a Bejeweled mod with its look may come instead ([MOD_ROADMAP.md](MOD_ROADMAP.md)) |
| every game PopCap released after EA bought it in July 2011 (the Hasbro board games, Garden Warfare, Plants vs. Zombies: Replanted) | out; Bejeweled 3 (December 2010) is older and stays |
| web-only, Facebook and mobile games, Bejeweled Live, Peggle 2, the compilations | never on the list |

## 2. The plan for the remaining games

### The order

When the remaining games start, how many run at once and which model each gets are rules of
[COORDINATION.md](COORDINATION.md) ("Which games, in what order", "Model selection"). The table lists them in
that order: the easier ones down to Feeding Frenzy, then the harder ones from Chuzzle on. A game that builds on
another's engine goes after it. Efforts are the estimates without references (3.3).

| # | Game | Model | Published | Effort | Engine, and what it needs first |
|---|---|---|---|---|---|
| 1 | Venice Deluxe (5.1) | Sonnet | yes | ~5x | SAF ~1.3 of Bejeweled's year, with PakInterface; no game RTTI |
| 2 | Heavy Weapon Deluxe (5.2) | Opus | yes | ~5x | a 2005 framework no port has, between BookWorm's and Big Money's; no game RTTI; SWF cutscenes; MO3 |
| 3 | Hammer Heads Deluxe (5.3) | Sonnet | yes | ~6x | Big Money's 1.2 framework; TinyXML object files; an SWF how-to-play |
| 4 | Pizza Frenzy (5.7) | Opus | yes | ~8x | Sprout's first-generation engine, from Word Harmony's port; no RTTI |
| 5 | Iggle Pop Deluxe (5.8) | Sonnet | yes | ~5-6x after Pizza Frenzy | Pizza Frenzy's engine build (72% of its functions identical); a Mac build with class names |
| 6 | Feeding Frenzy Deluxe (5.9) | Sonnet | yes | small to medium after Iggle Pop | the same engine in an older build; a Mac build with class names |
| 7 | Chuzzle Deluxe (5.10) | Opus | yes | ~8-12x | Raptisoft's own engine, no RTTI; a Mac build with names |
| 8 | Feeding Frenzy 2 Deluxe (5.11) | Opus | yes | ~10x | Sprout's second-generation engine, which no port has; `.theora` animations |
| 9 | Peggle Deluxe (5.12) | Sonnet | no | ~12x | Bejeweled's framework generation; JPEG 2000 and MO3; a Mac build |
| 10 | Peggle Nights (5.13) | Sonnet | no | ~4-5x after Deluxe | Peggle Deluxe's port; a Mac build |
| 11 | Bejeweled 3 (5.14) | Opus | yes | ~30x | the post-2010 "prime" framework; builds the layer Zuma's Revenge reuses; a Mac build |
| 12 | Banana Bugs (5.15) | Opus | yes | ~15x | a public-generation SAF with PakInterface, BASS and JPEG 2000; game RTTI |
| 13 | Gyromancer (5.16) | Opus | yes | large | Square Enix's own engine |
| 14 | Zuma's Revenge! (5.17) | Opus | web only | ~35x | the 2009 internal framework; Bejeweled 3's layer and Bookworm Adventures' JeffLib; a Mac build |
| 15 | Talismania Deluxe (5.18) | Sonnet | yes | ~12-16x after Feeding Frenzy 2 | Feeding Frenzy 2's engine |

### Where a port is published

A game with another PC decompilation in progress, or an active modding community, is still built, but published
neither in the GitHub snapshot nor on the website (the user, 2026-10-26; COORDINATION.md, "Which games, and where
they are published"). Decompilations of a phone or console build don't count, and neither do a handful of mods
or one person's tool. In the "Published" column, "yes" is the snapshot and the website; "no" is neither, with the
game's folder in `.publishignore`; "web only" is the website, with the folder in `.publishignore`. Zuma's Revenge
is the one "web only" game, while the user learns more about a PC decompilation in progress (2026-10-26). Every
ported game is published. Appendix B has the evidence.

### Peggle Extreme and Peggle World of Warcraft Edition

The user took these two out of the order on 2026-10-26: a comparison of their programs and paks with Deluxe's
and Nights' found no gameplay change, only presentation. Extreme adds a headcrab on Bjorn and Striders to the
main menu, its logo to the loading screen, the Heavy popping in after a good shot, four voice lines each for a
won and a lost level, and a teleport sound. World of Warcraft Edition is Nights cut down to two masters, with an
animated main menu, a dragon loading screen and its own texts. What their own code draws becomes settings of the
Peggle Deluxe and Nights ports, unpublished with them. Theme mods of the original games with their sounds, menus
and art are already made, outside the repository.

### After PopCap's games

Other studios' games follow (the user, 2026-10-25); each needs the user's own copy. Octogeddon comes first
(2026-11-01), World of Goo is wanted among them, and the three Cartoon Network games on Unity (2026-10-27) come
with them. Glass Tower comes near the end and the Tasty Planet series after it (2026-11-01). Appendix A has the
facts.

## 3. Method and findings

### 3.1 The Bejeweled method

[RE_NOTES.md](../games/bejeweled/docs/RE_NOTES.md) describes it. **Carve** the real game PE out of PopCap's DRM
wrapper (`tools/extract_game_exe.py`; it and `tools/triage_popcap.py` are private tools). **Analyse** it with
Ghidra 12 headless, which recovers C++ classes from RTTI and exports every decompiled function into the
git-ignored `re/<game>/` (`tools/ghidra_import.sh`). **Identify the framework** by matching functions against the
public SexyAppFramework trees (1.2, 1.3, 1.34): of Bejeweled's 1 MB `.text`, only about 60 KB was game code.
**Translate** the game classes function by function onto the portable SAF 1.3 in `framework/`, each function
annotated with its original address.

Later ports added two habits. Names carry over from an earlier port's export by byte matching
(`tools/funcmatch.py`), and each game keeps its names in `games/<game>/docs/symbols/`. A game not on the
SexyAppFramework gets its engine translated apart from the game, on the same platform layer.

### 3.2 What makes a good candidate

In order of how much each affects the effort: the game runs on a SexyAppFramework generation with public source
(older builds need a compatibility layer, which exists up to April 2004; other engines lose the framework step
entirely); its game classes have RTTI (Heavy Weapon, Venice, Chuzzle, Pizza Frenzy, Iggle Pop and Feeding Frenzy
have none); it uses no private middleware (PopLua, PopAnim, PIEffect, PIL, AELib, JeffLib and the 2008+ 3D
renderers have no public source; 3.6 says which are rebuilt); its game code is small (Bejeweled's is about 60 KB
and 250 functions, most others 3 to 30 times that); and its data is in formats the port already reads.

### 3.3 Verdicts and effort

| Verdict | Meaning |
|---|---|
| **DIRECT** | The method works as it is: a framework generation with public source, game classes with RTTI, and only decoders shared with other games as extra work. |
| **EXTRA WORK** | The method works, but the game also needs one of: a compatibility layer for a pre-public framework, manual class recovery (no game RTTI), a private PopCap library, Flash, or a 3D/shader backend. |
| **HARD** | The framework step fails (the engine is not SAF and no port has it), or the code is very large. |

Effort is relative to Bejeweled Deluxe = 1x (about 60 KB of game code, 250 functions, 12.7K decompiled lines),
estimated from game-code size plus the extra subsystems, without references to other projects (CLAUDE.md, "No
references"); treat each as ±50%. [TOKEN_ECONOMY.md](TOKEN_ECONOMY.md), "Estimates against costs", sets the
estimates beside what the first ports cost.

### 3.4 Findings that apply to many games

<!-- private: **Protection.** Of the first 26 games, none needs real unpacking. Most builds carry the unencrypted game inside one of four PopCap wrappers or launchers, which the private tool copies it out of; Bookworm Adventures Deluxe, Bookworm Adventures Vol. 2, Peggle WoW and Bejeweled 3 have no wrapper. Two games found later, Banana Bugs and Gyromancer's Steam exe, are encrypted; both now have a clear image (section 5). -->

**Protection.** Of the first 26 games, none needs real unpacking. There are four wrapper variants, plus a few games with
no wrapper:

| Wrapper | Games | How to get the image |
|---|---|---|
| 2006-08-21/22 PopCap DRM wrapper (VC7.1, Flash trial UI), marker `!popcapdrmprotect!` | AstroPop, Big Money (`WinBM.exe`), Chuzzle, Dynomite, Hammer Heads, Heavy Weapon, Iggle Pop, Pizza Frenzy, Rocket Mania, Talismania, Typer Shark | `extract_game_exe.py` unchanged |
| 2007-08-10 SAF + Lua + Flash wrapper (the same wrapper as Bejeweled's `WinBej.exe`) | Bejeweled 2, Feeding Frenzy 2, Insaniquarium, Peggle Deluxe, Peggle Extreme, Zuma Deluxe | unchanged |
| 2008-10-08 wrapper | Bejeweled Twist (Steam), Peggle Nights, Plants vs. Zombies, Zuma's Revenge | unchanged |
| PopCap "CDProtect" launcher, marker `!popcapcdprotect!` | BookWorm Deluxe 1.03 (retail); Bejeweled Twist (retail, ends at `!popcapcdprotend!`) | `extract_game_exe.py` accepts this marker too |
| no wrapper | Bookworm Adventures Deluxe, Bookworm Adventures Vol. 2, Peggle WoW, Bejeweled 3 | load the exe directly |
| PopCap 2010 DRM launcher: `<Game>.exe` beside an encrypted `<Game>.dat` | Banana Bugs | `extract_popcap2010.py` decrypts the `.dat` with the key in its header ([5.15](#515-banana-bugs)) |

Two games found later are encrypted: Banana Bugs behind PopCap's 2010 DRM and Gyromancer behind Steam's DRM stub
([5.15](#515-banana-bugs), [5.16](#516-gyromancer)). A cracked copy of Gyromancer's Steam build has since given a
clear image ([5.16](#516-gyromancer)), and Banana Bugs' launcher keeps everything its `.dat` needs inside the file,
so the private tool above writes the clear exe without running anything ([5.15](#515-banana-bugs)).

Bejeweled's wrapper, as measured: `WinBej.exe` (2,972,712 bytes, SHA-256
`a16fa32d6603f12843fce14153eecb5278cc907cc46aa862248229f5563b8376`) holds the game image at file offset `0x17b1a2`,
length `0x15aa86` (1,419,910 bytes, SHA-256 `6257a77f0e54f65b5cdbe36faf1ee6872189a700b05e5ed608ae545c0649466e`).

Typer Shark's `WinTS.exe` (1,860,002 bytes) is the 2006-08-22 wrapper, with Flash trial screens and the Steam2
API; the game PE runs from 0x591a2 to the end of the file (1,495,040 bytes).


<!-- /private -->

The `.luc` Lua files in the `drm/` folders belong to the wrappers, not the games, so Venice needs no PopLua.

**Ghidra on VS.NET 2002 (MSVC 7.0) binaries.** `tools/ghidra_import.sh` and `tools/rtti_msvc70.py` handle four
problems; of the games still to port, Heavy Weapon and Chuzzle are VS.NET 2002 builds.
- Ghidra 12's RTTI analyzer and `RecoverClassesFromRTTIScript` recover **no classes**: MSVC 7.0 writes 24-byte
  BaseClassDescriptors without the `pClassDescriptor` field, and Ghidra rejects them. `tools/rtti_msvc70.py` reads
  the RTTI itself and writes symbols that `ApplySymbols.java` applies (`SYMBOLS=` in `ghidra_import.sh`).
- The "Non-Returning Functions - Discovered" analyzer marks the `_free`/`operator delete` thunks as noreturn,
  which truncated about 2,225 call sites in BookWorm's first export. `MSVC70=1` disables it
  (`tools/ghidra_scripts/DisableNoReturnDiscovery.java`).
- `ApplySymbols.java` failed under the pipeline's JDK 21 with "class file version 67.0" when a class compiled by
  JDK 23 was cached in Ghidra's script bundle.
- `analyzeHeadless.bat` breaks `-scriptPath` on `;`, so all scripts are in one directory, `tools/ghidra_scripts/`.

On a large binary (Bookworm Adventures) `RecoverClassesFromRTTIScript` was still running after 50 minutes;
`CLASS_RECOVERY=0` skips it, and auto-analysis still names the RTTI classes and vtables.

### 3.5 Framework generations, and where each is ported

From vtable slot counts:

| Game | Game PE build | Widget slots | Dialog slots | Other markers | Where it is ported |
|---|---|---|---|---|---|
| Noah's Ark, Mummy Maze, Seven Seas, Atomica | 2002 to early 2003 | | | the first-generation SexyApp | `games/noahsark/docs/FRAMEWORK_2002.md`, and the `FRAMEWORK_2003.md` of Seven Seas and Atomica |
| Dynomite | 2003-07 | 43 | 48 | no WidgetContainer; SexyApp (61 slots) with no SexyAppBase | `games/dynomite/docs/FRAMEWORK_2003.md` |
| Rocket Mania | 2003-09 | 44 | 49 | VC6; SexyApp 62 slots | `games/rocketmania/docs/FRAMEWORK_2003.md` |
| AstroPop, Typer Shark, BookWorm Deluxe; Pixelus, Alchemy | 2004-04 | 47 | 52 | no WidgetContainer, monolithic SexyApp | [FRAMEWORK_2004.md](FRAMEWORK_2004.md) and `framework/compat2004` (the library `sexycompat2004`) |
| Heavy Weapon | 2005-02 | 48 | 54 | SexyAppBase split out (80 slots); FlashWidget | none yet |
| Big Money, Hammer Heads | 2005-11, 2006-05 | 54 | 60 | WidgetContainer has 15 slots: exactly the public 1.2 tree ([sandboxmo-de/SexyAppFramework-v1.2](https://github.com/sandboxmo-de/SexyAppFramework-v1.2)) | `games/bigmoney/docs/FRAMEWORK_12.md` |
| Bejeweled, Bejeweled 2 | 2007-04 | 70 | 76 | WidgetContainer 30; Bejeweled 2 is identical to Bejeweled in every measured class | `framework/` itself, with [FRAMEWORK_NOTES.md](FRAMEWORK_NOTES.md) |

The other SAF games are of the 1.3 generation and run on `framework/` as it is: Zuma Deluxe, Insaniquarium, the
Peggle family, Plants vs. Zombies, Venice, both Bookworm Adventures games and, as its image showed, Banana Bugs.
Bejeweled Twist (2008), Zuma's Revenge (2009) and Bejeweled 3 (2011) use later internal generations with no
public source. Twist's is ported: its D3D9 interface became the framework's GPU path (`framework/GPU2D.h`,
`GPUInterface.h`, `GPUDisplay.h`), and its 3D drawing is in `games/bejeweledtwist/src/`.

Of the engines that are not SAF, Sprout's first generation is ported in Word Harmony (`games/wordharmony/engine/`,
described in its `docs/ENGINE.md`), and Pizza Frenzy, Iggle Pop and Feeding Frenzy start from it. Sprout's second
generation (Feeding Frenzy 2, Talismania), Raptisoft's engine (Chuzzle) and Square Enix's (Gyromancer) have no
port. The other engines ported so far (TipTop's, Ning Po's, Retro64's, Jolly Bear's, New Crayon's) serve no
remaining game.

### 3.6 Shared components

Each is built once and serves several games. Most live in the shared code; a game library lives in the folder of
the game that needed it first until a second game moves it to shared code.

| Component | Built, and where | Remaining games that need it |
|---|---|---|
| MO3 music with order-position jumps | libopenmpt (`third_party/libopenmpt`) through `platform/openmpt/OpenMPTPlayer.h` (the library `sexyopenmpt`). Only MO3 goes through it; S3M, IT and XM stay on libxmp-lite. | Heavy Weapon, Chuzzle, Feeding Frenzy 2, Peggle Deluxe and Nights, Bejeweled 3, Zuma's Revenge |
| JPEG 2000 (`.jp2`) | OpenJPEG (`third_party/openjpeg`) through `platform/openjpeg/JP2Decoder.h` (`sexyopenjpeg`), decoding at run time. Its rounding was changed to match the originals' decoder for the 5/3 wavelet; [BYO-POP.md](../third_party/openjpeg/BYO-POP.md) asks the Peggles, Zuma's Revenge and Bejeweled 3 to check their own files first. | Peggle Deluxe (217 files) and Nights (273), Zuma's Revenge (2,633), Bejeweled 3 (4,557), Banana Bugs |
| Flash SWF | A player of our own for the SWF 7 and 8 cutscenes of both Bookworm Adventures games, `games/bookwormadv/swf/` (copied in `games/bookwormadv2/swf/`). | Heavy Weapon (3 cutscenes), Hammer Heads (how-to-play) |
| Pre-1.2 framework layers | Built up to April 2004 (3.5). | Heavy Weapon (its 2005 layout, not yet mapped) |
| PopAnim (`.pam`) player | v1 in `games/bookwormadv/popanim/`, v1-3 in `games/bookwormadv2/popanim/`, and Volume 2's with Twist's two extra scales in `games/bejeweledtwist/popanim/`. | Bejeweled 3, Zuma's Revenge |
| JeffLib | The parts Bookworm Adventures uses: `games/bookwormadv/jefflib/`. | Zuma's Revenge (13 classes) |
| 3D and shaders | For Twist: the framework's GPU path (`framework/GPU2D.h`, `GPUInterface.h`, `GPUInterfaceGL.cpp`, `GPUDisplay.h`), and Twist's `.p3d` meshes and 17 shaders as GLSL in `games/bejeweledtwist/src/` (`TwistD3D`, `TwistShaders.cpp`). | Bejeweled 3 (RenderDevice3D, `.d3dfx` effects), Zuma's Revenge (`.fx` effects) |
| PIEffect, PIL, AELib, CurvedVal | CurvedVal is Twist's (`games/bejeweledtwist/src/CurvedVal.cpp`); the others are not built. FEAST and CfgCompiler only compile `compat.cfg` (GPU rules) and can be dropped. | Bejeweled 3, Zuma's Revenge |
| Sprout engine, `.saf` archives (keyed MD5), `.theora` sprite animations | First generation: Word Harmony's port of an older build, `games/wordharmony/engine/`, kept free of game code so that it can move to shared code; its `ResourceManager` reads `.saf` archives. Second generation and `.theora`: not built (libtheora is not in `third_party/`). | Pizza Frenzy, Iggle Pop, Feeding Frenzy (first generation); Feeding Frenzy 2 and Talismania (second, with `.theora`) |
| Resource-ID tables | No shared tool: a port with a `resources.xml` generates its `Res` table with a script of its own (`games/bookwormadv/tools/resgen.py`, `games/bejeweledtwist/tools/resgen.py`); a port whose names are hard-coded writes the table from the exe. | Hammer Heads and the Peggles (`resources.xml`) |

Built and needed by no remaining game: Sun `.au` sounds (`platform/Audio.cpp`), the PopLua VM with its LuaLib and
LuaApp bindings (`games/bookwormadv/poplua/`, `lualib/`, `luaapp/`), and the prefix-compressed word lists of the
BookWorm ports.

## 4. All games at a glance

"Build" is the link date of the game PE and its compiler. "Effort" is without references, and for a ported game
it is the estimate made before its port. Rows are grouped by framework family and by build date within a group;
a remaining game's row names its section, which has its code size.

| Game | Build | Framework / engine | Effort | Status |
|---|---|---|---|---|
| Mummy Maze Deluxe | 2002-06 | first-generation SexyApp | ~1-1.5x | [ported](../games/mummymaze/docs/README.md) |
| Noah's Ark Deluxe | 2002-10, VC6 | first-generation SexyApp | ~1-1.5x | [ported](../games/noahsark/docs/README.md) |
| Seven Seas Deluxe | 2003-01, VC6 | first-generation SexyApp | ~1-1.5x | [ported](../games/sevenseas/docs/README.md) |
| Atomica Deluxe | 2003-03, VS2002 | 2003 SAF | ~1.5x | [ported](../games/atomica/docs/README.md) |
| Dynomite Deluxe (Raptisoft) | 2003-07-08, VS2002 | pre-public 2003 SAF | ~4.5x | [ported](../games/dynomite/docs/README.md) |
| Rocket Mania Deluxe (Nuclide) | 2003-09-08, VC6 | pre-public 2003 SAF | ~3.5x | [ported](../games/rocketmania/docs/README.md) |
| AstroPop Deluxe | 2004-04-06, VS2002 | pre-public Apr 2004 SAF | ~7-8x | [ported](../games/astropop/docs/IDENTICAL_PLAY.md) |
| BookWorm Deluxe 1.03 | 2004-04-15, VS2002 | pre-public early-2004 SAF | ~3-3.5x | [ported](../games/bookworm/docs/README.md) |
| Typer Shark! Deluxe 1.02 | 2004-04-20, VS2002 | pre-public Apr 2004 SAF | ~5x | [ported](../games/typershark/docs/GAME_NOTES.md) |
| Pixelus Deluxe (Nuclide) | 2004-07-29 | BookWorm's 2004 SAF, plus Nuclide's own layer | ~3x | [ported](../games/pixelus/docs/README.md) |
| Alchemy Deluxe 1.6 | 2004-08-23 | BookWorm's 2004 SAF | ~2-3x | [ported](../games/alchemy/docs/README.md) |
| Heavy Weapon Deluxe | 2005-02-16, VS2002 | pre-1.2 SAF (early 2005), FlashWidget | ~5x | to port (5.2) |
| Big Money Deluxe | 2005-11-14, VS2002 | public SAF 1.2 layout | ~2x | [ported](../games/bigmoney/docs/README.md) |
| Hammer Heads Deluxe (Nuclide) | 2006-05-17, VS2003 | SAF 1.2 layout + FlashWidget; TinyXML | ~6x | to port (5.3) |
| Zuma Deluxe | 2007-04-04, VS2005 | SAF ~1.3 | | not decompiled |
| Bejeweled 2 Deluxe | 2007-04-11, VS2005 LTCG | SAF ~1.3, same vtables as Bejeweled | ~6x | [ported](../games/bejeweled2/docs/IDENTICAL_PLAY.md) |
| Bejeweled Deluxe 1.87 (baseline) | 2007-04-13, VS2005 LTCG | SAF ~1.3 | 1x | [ported](../games/bejeweled/README.md) |
| Venice Deluxe | 2007-06-06 | SAF ~1.3 with PakInterface | ~5x | to port (5.1) |
| Bookworm Adventures Deluxe | 2007-06-20, VS2005 | SAF ~1.3 + PopAnim, PopLua, JeffLib | ~10-15x | [ported](../games/bookwormadv/docs/README.md) |
| Insaniquarium Deluxe 1.1 | 2007-06-28, VS2005 | SAF 1.3 | ~8-10x | not decompiled |
| Peggle Deluxe 1.01 | 2007-07-16, VS2005 | SAF ~1.3 | ~12x | to port (5.12) |
| Peggle Extreme | 2007-08-22, VS2005 | SAF ~1.3 | ~1x after Deluxe | not decompiled: settings of Deluxe |
| Peggle Nights | 2008-09-26, VS2005 LTCG | SAF ~1.3 | ~4-5x after Deluxe | to port (5.13) |
| Bejeweled Twist 1.0.3 | 2008-10-17, VS2005 partial LTCG | internal mid-2008 SAF with D3D7/8/9, shaders, P3D meshes; PopAnim | ~25x | [ported](../games/bejeweledtwist/docs/NOTES.md) |
| Peggle World of Warcraft Edition | 2009-06-19, VS2005 LTCG | SAF ~1.3 (Nights branch) | ~1-2x after Nights | not decompiled: settings of Nights |
| Bookworm Adventures Vol. 2 | 2009-07-22, VS2005 | SAF 1.3 generation + PopAnim v1-3 | ~15x alone; ~4-6x after Vol. 1 | [ported](../games/bookwormadv2/docs/README.md) |
| Zuma's Revenge! | 2009-08-14, VS2005 LTCG | internal 2009 SAF: RenderDevice3D, D3D8/9, `.fx` effects; JeffLib, PIL, AELib, PIEffect, PopAnim | ~35x | to port (5.17) |
| Banana Bugs | 2010-04-09, VS2005 | public-generation SAF; PakInterface, BASS, JPEG 2000 | ~15x | to port (5.15) |
| Plants vs. Zombies GOTY 1.2.0.1096 | 2010-12-10, VS2008 | internal SAF (~2009-10), TodLib | | not decompiled |
| Bejeweled 3 | 2011-10-26, VS2008 LTCG | post-2010 "prime" framework; PopAnim, PIEffect, CurvedVal | ~30x | to port (5.14) |
| Ning Po Mahjong Deluxe | 2002-09 | its own engine | ~2-3x | [ported](../games/ningpo/docs/README.md) |
| TipTop Deluxe | 2003-02 | its own engine | ~2-3x | [ported](../games/tiptop/docs/README.md) |
| Chuzzle Deluxe (Raptisoft) | 2005-05-10, VS2002 | Raptisoft's engine | ~8-12x | to port (5.10) |
| Feeding Frenzy Deluxe (Sprout) | 2005-07 | Sprout first generation, an older build | small to medium after Iggle Pop | to port (5.9) |
| Pizza Frenzy (Sprout) | 2006-02-21, VS2003 | Sprout first generation | ~8x | to port (5.7) |
| Iggle Pop Deluxe (Sprout) | 2006-02-21, VS2003 | Sprout first generation | ~5-6x after Pizza Frenzy | to port (5.8) |
| Word Harmony (Sprout) | 2006-02-22, VS2003 | Sprout first generation, an older build | ~3-4x | [ported](../games/wordharmony/docs/README.md) |
| Feeding Frenzy 2 Deluxe (Sprout) | 2006-02-24, VS2005 | Sprout second generation; sigslot, libtheora | ~10x | to port (5.11) |
| Talismania Deluxe (RocketJump) | 2006-07-28, VS2005 LTCG | Sprout second generation | ~20-25x alone; ~12-16x after Feeding Frenzy 2 | to port (5.18) |
| Mahjong Escape: Ancient China (Jolly Bear) | 2006-08 | Jolly Bear Games' engine, DirectDraw | small after Ancient Japan | [ported](../games/mahjongchina/docs/README.md) |
| Mahjong Escape: Ancient Japan (Jolly Bear) | 2006-11 | Jolly Bear Games' engine, DirectDraw | ~4-5x | [ported](../games/mahjongjapan/docs/README.md) |
| Bonnie's Bookstore Deluxe (New Crayon) | 2006-11-10, VS2003 | New Crayon's own engine on SDL 1.2 and FMOD 3 | ~4-6x | [ported](../games/bonnies/README.md) |
| Water Bugs (Retro64) | 2007-10 | Retro64's own engine, DirectDraw | small after Cosmic Bugs | [ported](../games/waterbugs/docs/README.md) |
| Cosmic Bugs (Retro64) | 2007-11 | Retro64's own engine, DirectDraw | ~2x | [ported](../games/cosmicbugs/docs/README.md) |
| Gyromancer (PopCap with Square Enix) | 2009-09 | Square Enix's own engine | large | to port (5.16) |

Chuzzle: Christmas Edition, The Wizard's Pen and Sweet Tooth To Go were never triaged. No game was found to be
impossible.

## 5. The remaining games

One section for each game, in the order of section 2; the numbers are those of the first survey, so some are
missing. The image each port starts from, carved where the game is wrapped, is in the main checkout's
`re/<game>/bin/`. Where a Mac build exists it is in `re/<game>/mac/` with a demangled list of its function and
class names (CLAUDE.md, "Mac versions"; git-ignored, so a worktree copies the folder in): its names and class
layout guide the port, and the PC exe stays the authority for behaviour. Published means the rule of section 2.

### 5.1 Venice Deluxe

**Sonnet, published, ~5x.** One of the user's installers (2026-10-24). The game inside the wrapper `Venice.exe`
is 1,978,368 bytes, linked 2007-06-06, with about 1.59 MB of code. It is on SAF of the ~1.3 generation with
PakInterface, of Bejeweled's year: its RTTI names 43 `Sexy::` classes and the pak and library classes but no
game class, so the game's classes are found from their constructors, as Alchemy's were. Whether its vtables
match Bejeweled's, as Bejeweled 2's do, is the first thing to measure, since `framework/` is Bejeweled's. Data:
`main.pak` (XOR 0xF7; `tools/popcap_pak.py` reads it) with 861 PNG, XML, JPG and OGG files; no `resources.xml`.
It has no PopLua (checked 2026-11-04: its 15 `.luc` scripts are the wrapper's); the ~5x estimate assumed PopLua
and stands as a first guess.

### 5.2 Heavy Weapon Deluxe

**Opus, published, ~5x, EXTRA WORK.** The game inside the wrapper is 1,519,616 bytes, linked 2005-02-16 with
VS2002: ~300 KB / ~690 functions of game code. Its framework is pre-1.2, early 2005 (Widget 48 slots, Dialog 54,
SexyAppBase split out at 80, NativeDisplay, FlashWidget), which no port has. It sits between BookWorm's April
2004 snapshot ([FRAMEWORK_2004.md](FRAMEWORK_2004.md), `framework/compat2004`) and Big Money's 1.2
(`games/bigmoney/docs/FRAMEWORK_12.md`), both built with the same compiler, so byte matching against both ports'
exports (`tools/funcmatch.py`) should name most of it. RTTI covers the framework only: about 75 game vtables have
no names (many are 9-slot enemy and projectile classes), and names come from strings and the data XML; source
files appear to be linked alphabetically. Data: `data/*.xml` (levels, waves, bosses, craft, survival), which make
behaviour easy to verify; `Music/AtomicTank.mo3` and 2 OGG; 3 SWF cutscenes, for Bookworm Adventures' Flash
player; `*.dmo` demo recordings.

### 5.3 Hammer Heads Deluxe

**Sonnet, published, ~6x, EXTRA WORK.** The game inside the wrapper is 1,703,936 bytes, linked 2006-05-17 with
VS2003, by Nuclide (working title "Cubby Catch"); its DRM IPC client must be stubbed. It is on the 1.2 layout
plus FlashWidget and ListWidget: Big Money's port (`games/bigmoney/`, `docs/FRAMEWORK_12.md`) is the one to copy.
Whether it shares code with Nuclide's earlier Rocket Mania and Pixelus, both ported, has not been checked. Game
code is ~418 KB / ~1,890 functions, of which ~80 KB is editor and property UI and ~26 KB generated resource code;
RTTI names 120 game classes, with a full in-game level editor and property system. Data: `resources.xml` (363
IDs); `boards/*.xml`, TinyXML documents of serialized objects (`<object Class="class Sexy::Cubby" ...>`), so the
property system and object factory must be reproduced; `boards/*.csv`; `gnomedata/*.txt`;
`images/howtoplay.swf`. TinyXML (zlib licence) is the game's own open library, so the port can use it.

### 5.7 Pizza Frenzy

**Opus, published, ~8x as the first game on Word Harmony's engine port, EXTRA WORK.** The game inside the
wrapper is 1,268,224 bytes (v5.8.30.1, Sprout Games), linked 2006-02-21 with VS2003: Sprout's first-generation
engine (~265 KB) and ~455 KB of game code, with no RTTI (about 322 unnamed vtables). Data: `PizzaFrenzy.saf`
(6.0 MB, FFAS format, 1,081 entries) and loose `splash/`. It starts from Word Harmony's port of an older build of
the engine, `games/wordharmony/engine/` (`docs/ENGINE.md`), whose `ResourceManager` already reads `.saf` archives:
170 of the 198 strings its engine code refers to appear in Pizza Frenzy and Iggle Pop too, but only about a
fifth of its bytes are identical, the functions having been rebuilt and changed. Moving that engine to shared
code, once this second Sprout game is on it, is a shared change for the coordinator. There is no Mac build of
Pizza Frenzy here, but Iggle Pop shares its engine build, and Iggle Pop's and Feeding Frenzy's Mac builds name
the engine's classes (5.8, 5.9).

### 5.8 Iggle Pop Deluxe

**Sonnet, published, ~5-6x after Pizza Frenzy, EXTRA WORK.** The game inside the wrapper is 1,359,872 bytes
(v6.2.17.1, internal name FuzzleFrenzy), linked the same day as Pizza Frenzy on the same engine build: after
masking relocations, 651 of 900 engine-region functions (72%) are byte-identical, as are 85% of the DirectDraw
functions and 87% of the CRT. Game code is ~480 KB / ~1,200 functions with no RTTI (about 386 unnamed vtables);
names come from the XML class and aiClass names and from the Mac build. Data: `IgglePop.saf` (6.9 MB, 1,628
entries), fully XML-driven.

The Mac build, `re/igglepop/mac/` (Iggle Pop Deluxe 1.0.0, `igglepop-bin`), has a stripped symbol table but
GCC's RTTI, which names 399 classes with their bases: 43 of the engine's (nine with the same names in Word
Harmony's port), 30 of its SDL and OpenGL driver, sound and signal libraries, and the game's own, down to nested
state classes. `IgglePop.symbols.txt` gives each class's typeinfo and vtables with their addresses (`rttinames.py`
beside it makes the list): the classes and the order of their virtual functions, but not the functions. Plan:
link Pizza Frenzy's engine, name the shared engine functions by byte matching, take class names and bases from
the Mac build, and translate only the game code.

<!-- private -->

The user's first Mac download (`IgglePop.dmg`, 2026-10-31) keeps the game in `Contents/Resources/Iggle Pop!.payload`
behind the `MacProtectRun` launcher. The second (`Iggle_Pop_Deluxe_1.0.1_Full.dmg`, 2026-11-04) is a cracked copy
whose `igglepop-bin` is that payload with each 16-byte block reversed and the last, partial block left as it is:
byte for byte the same binary, saved here. Its `Info.plist` was edited to say 1.0.1, so the saved one is PopCap's
1.0.0. Feeding Frenzy's two downloads are built the same way.

<!-- /private -->

### 5.9 Feeding Frenzy Deluxe

**Sonnet, published, small to medium after Iggle Pop.** From the user's Popcap Game Collection (2026-10-25): a
plain exe with no wrapper (992,976 bytes, built 2005-07, 640 KB of code) beside a signed `properties/partner.xml`,
an 8 MB `FFArchive.saf` and loose WAV, music and menu files. It is Sprout's first generation in a build older
than Pizza Frenzy's; its RTTI names only three `engine` classes, but source file names and the PDB name are left
in. The Mac build, `re/feedingfrenzy/mac/` (Feeding Frenzy Deluxe 1.0.0, saved as `FeedingFrenzy`), is stripped
as Iggle Pop's and names 148 classes with their bases, 87 of them also in Iggle Pop's build: most of the engine,
and a layer above it that both games use (`FadeContainer`, `ParticleSystem`, `MusicSystem`, `ScreenLayout`,
`TextItem`...). Its copyright runs to 2007, so it may be a later build than the PC exe. Plan: copy Pizza
Frenzy's and Iggle Pop's engine and find where this older build differs, as Word Harmony's port did.

### 5.10 Chuzzle Deluxe

**Opus, published, ~8-12x, HARD.** The first of the harder games. The game inside the wrapper `Chuzzle.exe` is
1,105,920 bytes, linked 2005-05-10 with VS2002: ~300 KB of game and ~320 KB of engine. The engine is **not SAF**
but Raptisoft's own, with no public source: App, Graphics (DirectDraw7/Direct3D7 with a software fallback),
Sprite, TextureMosaic/TextureSplatter, Sound, Music (BASS/MO3), Font (it reads SAF ImageFont `.txt`
descriptors), GadgetManager, ProfileManager and others; Dynomite, Raptisoft's 2003 game, runs on SAF and shares
nothing with it. There is effectively no RTTI (`/GR-`): about 129 unnamed vtables, 27 `__FILE__` strings, config
keys and dialog names. The Mac build, `re/chuzzle/mac/` (Chuzzle Deluxe 1.0.0, a name list of about 5,100
lines), names the engine's and the game's classes, which turns the class recovery that made Chuzzle HARD into a
matter of matching. Data is loose: textures with `_Alpha` masks, ImageFont fonts, OGG, `music/Chuzzle.mo3` with
`Jukebox.xml`, `data/gamedata.cfg` tuning, `data/mindbenders.txt` puzzles; the `cached/*.cached` texture caches
can be ignored. Plan: translate the engine apart from the game, as a small layer (Sprite, Font, Sound, Music,
Gadget) over the platform code rather than the D3D7 internals.

### 5.11 Feeding Frenzy 2 Deluxe

**Opus, published, ~10x, EXTRA WORK.** The game inside the wrapper `FeedingFrenzyTwo.exe` is 1,917,952 bytes,
linked 2006-02-24 with VS2005: ~470 KB of game and ~570 KB of engine, with full RTTI (581 classes, about 430 of
them the game's). The engine is Sprout's second generation, which no port has; 5.18 describes it, and Talismania
shares 59 of its 62 engine classes, so it is built here for both. Word Harmony's first-generation engine shares
its formats and many names (the `.saf` archive, ScreenLayout, the XML data), but the code was rewritten between
the generations. Data: `FF2.saf` (14.6 MB, 1,933 entries: PNG, JPG, OGG and 363 XML with 74 stages) with 29
`.theora` sprite animations (frame ranges in `animation.xml`; the container is described in 5.18), which need
libtheora, the game's own open library, not yet in `third_party/`; loose `music/FF2.mo3`.

### 5.12 Peggle Deluxe

**Sonnet, unpublished, ~12x, DIRECT.** The game inside the wrapper `Peggle.exe` is 2,842,624 bytes (v1.01),
linked 2007-07-16 with VS2005: ~750 KB of game code in about 70 classes, on SAF ~1.3, Bejeweled's generation,
with ModVal tunable strings left in. Data: `main.pak` (11 MB) with 217 `.jp2` images (check their wavelet first,
3.6), 62 binary `levels/*.dat` files, ConfigParser `.cfg` files and fonts; loose `music/PeggleBeats.mo3` and 2
OGG. The Mac build is `re/peggle/deluxe/mac/` (Peggle 1.0.5, a name list of about 12,300 lines), and Ghidra
exports of all four Peggle images (2026-10-26) are in `re/peggle/<edition>/gh/` (deluxe, extreme, nights, wow).
Unpublished: another decompilation is in progress and the modding is strong (Appendix B), so `games/peggle*` is
in `.publishignore`. Extreme's presentation becomes settings of this port.

### 5.13 Peggle Nights

**Sonnet, unpublished, ~4-5x after Deluxe, DIRECT.** The game inside the wrapper `PeggleNights.exe` is 3,342,336
bytes, linked 2008-09-26 with VS2005 LTCG: ~700-950 KB in 81 game classes, the Deluxe set plus screens and a
level editor. Only about 6% of bytes are identical to Deluxe, which reflects the LTCG rebuild; the class
structure and names carry over from Deluxe's port. The framework is SAF ~1.3 plus EncodingParser and OLE
drag-and-drop for level-pack import, which needs another way in for the web build. Data: `main.pak` (17.9 MB)
with 273 `.jp2` and 70 levels; 16 MO3; `levelpacks/*.pak`. Peggle WoW is a branch of the Nights code (PDB path
`ThunderballNights\WoW`) with no wrapper, 32% of its bytes identical to Nights; its presentation becomes
settings of this port. The Mac build is `re/peggle/nights/mac/` (Peggle Nights 1.0.4, about 10,100 lines of
names). Unpublished: the most active modding community of any game left (Appendix B).

### 5.14 Bejeweled 3

**Opus, published, ~30x, EXTRA WORK.** `Bejeweled3.exe` is the game itself, with no wrapper (5,484,400 bytes),
linked 2011-10-26 with VS2008 SP1 LTCG, a later build of a game that came out in December 2010; it imports
`steam_api` (achievements, leaderboards) and WinInet. Game code is ~1.0-1.13 MB in 151 classes (a Board subclass
for each mode, quest mini-game boards, about 25 Goal and 20 Badge classes, BinauralManager) plus ~0.6 MB of
framework; `properties/default.xml` AUTOGEN comments name 45 source files and 171 `Class::Method:line` entries.
The framework is the post-2010 "prime" one: a driver layer (IAppDriver, IGraphicsDriver, IAudioDriver,
IFileDriver, IGamepadDriver...), RenderDevice3D, RenderEffect (`.d3dfx`), a Graphics3D camera and meshes, PopAnim,
PIEffect, CurvedVal, ResStreamsManager, FEAST and XInput. Data: `main.pak` (90 MB) with 4,557 `.jp2`, 64 `.pam`,
118 `.ppf`, 12 `.p3d` + 5 `.p3danim`, 14 `.d3dfx`, `.cfg` files and one MO3 suite; loose `binaural/*.sbg` (SBaGen
programs for Zen mode), ambient OGG and affirmation texts. Twist's port has the PopAnim player, CurvedVal,
`.p3d` meshes, GLSL shaders and the framework's GPU path (3.6); new here are the driver layer, RenderDevice3D
with its `.d3dfx` effects, PIEffect (`.ppf`) and the 3D animations, the layer Zuma's Revenge reuses. The Mac
build, `re/bejeweled3/mac/` (version 1.1.12), has a short name list (about 2,000 lines): framework classes and
some of the game's, the quest goals among them.

### 5.15 Banana Bugs

**Opus, published, ~15x, DIRECT.** Added on 2026-10-25.

- **Copy:** installed in `C:\Program Files (x86)\PopCap Games\Banana Bugs`; the installer (`BananaBugsSetup.exe`,
  82 MB) is unpacked, unrun, in `re/_installers/BananaBugsSetup/`.
- **Image:** the only game here behind PopCap's 2010 DRM: `BananaBugs.exe` is the launcher, and the game is the
  encrypted `BananaBugs.dat`. A clear image, made from the installed copy on 2026-11-04, is
  `re/bananabugs/bin/BananaBugsGame.exe`, with a `README.txt` on where each file came from; how the scheme works
  and the tool that undoes it stay private. It was linked 2010-04-09 with VS2005 (v1.0.0.7, 2,413,936 bytes): a
  1.87 MB `.text` with 6,946 functions in Ghidra, libraries included (the export is in `re/bananabugs/game/`),
  and 170 RTTI names, about 85 of them the game's own classes (the bugs and their `BugBrain` AIs, twenty `Bonus`
  classes, the board, the map and its buildings).
- **Engine:** despite the name and the rope-walling play it is not Retro64's engine but PopCap's public
  SexyAppFramework generation (`SexyAppBase`, `DDInterface`, `D3DInterface`, `WidgetManager`, `ResourceManager`,
  `BassMusicInterface`, `PakInterface`): `Main.pak` (62 MB), 120 OGG sounds, BASS, and `j2k-codec.dll` for JPEG
  2000 images, which `platform/openjpeg` replaces.
- **Effort:** the framework is public and the game classes have RTTI, so the extra work is the size alone, over
  a megabyte of game code, on a par with Bookworm Adventures Vol. 2's. The launcher and its licensing client are
  left out of the port.
- **Animated cursors on the site (the user, 2026-11-04).** The game's cursors are animated (its own `Cursor`
  class and `images/cursor_anim`). Over the canvas the port shows them with no site work. Two places need work
  once the web build runs: the game page copies the cursor to the rest of the page only every 100 ms
  (`framework/WebCursor.cpp`, `web/shell.html`'s `pollCursor`), and the site cursor (`web/cursors.js`,
  WEB_PLAYER.md's "The site's cursor") keeps one image per kind while browsers animate no cursor image, so a set
  would keep its frames and the site would swap them on a timer (check that the cursor still changes while the
  mouse is still, keep to the 128 px limit, keep the cost small). Both are shared code: the Banana Bugs session
  sends the coordinator a spec.
- **Other copies (2026-11-04):** PopCap's own installer of June 2010 (identical files), Shockwave.com's
  installer and Big Fish Games' build in a repack from the Internet Archive stay in the Downloads folder. None
  holds a clear game image, so nothing was saved from them.

<!-- private -->

**The 2010 DRM scheme and the tool (2026-11-04).** Read from the launcher's code in `re/bananabugs/launcher/`,
with nothing run. `FUN_004c6780` opens the `.dat`, reads a 0x44-byte header and decrypts the rest in memory, then
writes the result to `popcapgame1.exe` in a temp folder and runs it. The header is a zero dword, the SHA-256 of
the clear exe (bytes 4..36) and the AES-256 key (bytes 36..68): the key is in the file, which is why the unbought
game plays its trial offline. The cipher is AES-256 in CBC mode with the first 16 bytes as the IV and ciphertext
stealing for the partial last block. The plaintext is a container: `PCWE`, the exe's size, its SHA-256 again, a
chunk count, then chunks of an 11-byte header (a zero flag byte, two unused bytes, offset, length) and data. The
35 chunks are shuffled; the launcher orders them by offset, insists they tile the exe without gaps, and compares
their hash with both stored hashes. `tools/extract_popcap2010.py <Game.dat> <out.exe>` (in `.publishignore`) does
the same in pure Python in about two seconds, and refuses to write unless both hashes match.

**The other copies.** Shockwave's and the repack's `BananaBugs.exe` are the game itself (linked 2010-04-09,
v1.0.0.7, the same virtual section sizes as the clear image) wrapped by Armadillo (Silicon Realms), with the
game's sections compressed and encrypted inside `.pdata`: a second route to the image that would have needed a
dumper or a downloaded unpacker, and that now serves only as a check on the clear image.

<!-- /private -->

### 5.16 Gyromancer

**Opus, published, large.** Added on 2026-10-25.

- **Copy:** Steam, `E:/SteamLibrary/steamapps/common/Gyromancer` (2009-09). A puzzle RPG made by PopCap with
  Square Enix, which published it, on battles like Bejeweled Twist's.
- **Engine:** not Twist's or any PopCap framework: no SexyApp class strings at all, and 80 game classes of
  Square Enix's own engine (`CGM_*` game modes, `CData*` tables, effects). Its data is in its own archives
  (`.bjm`, `.cab`, `.dat`).
- **Image:** Steam's DRM stub has encrypted the Steam exe's code section. A clear image of the same build, found
  on 2026-11-04, is kept as `re/gyromancer/bin/Gyromancer_clear.exe` beside the Steam exe, with a README.

<!-- private -->

**How the clear image differs from the Steam exe.** Two of the user's downloads hold the same cracked exe
(1,671,168 bytes, dated 2009-11-18): the `Gyromancer\` folder of the repack in 5.15 and a separate
`Gyromancer.rar`. It is the Steam exe with Valve's stub taken off: the same link date (2009-09-18), version
1.0.0.1, PDB path and checksum field; `.data` and `.rsrc` byte-identical; `.rdata` different only in the import
descriptors; the `.bind` section and Valve's signature gone; `.text` decrypted. `steam_api.dll` was dropped from
the imports, and the code jumps over its calls into Steam's API, all between 0x418100 and 0x4181c0
(`re/gyromancer/bin/README.txt` lists the sites). Apart from them the image should be the original code, though
the Steam exe's encrypted `.text` cannot be used to check it byte for byte. Its 99 RTTI names are the same as the
Steam exe's, and both downloads' data files are identical to Steam's.

<!-- /private -->

### 5.17 Zuma's Revenge!

**Opus, web only, ~35x, HARD.** The game inside the wrapper `ZumasRevenge.exe` is 6,651,904 bytes, linked
2009-08-14 with VS2005 LTCG (internal name Zuma2); it imports `steam_api` (achievements) and WinInet. Its `.text`
is 5.3 MB, the largest of the first 26 games: ~1.15-1.75 MB of game code in 63 polymorphic classes (Board, Ball,
Gun, CurveMgr, ten Boss subclasses, TikiTemple, MapScreen, shader effects) plus 0.6-1 MB of non-public
libraries, with LTCG blurring the boundary (~0.9 MB of rasterizer code sits between game classes). The framework
is an internal 2009 generation with RenderDevice/RenderDevice3D, D3D8/9, DXSurface, `.fx` effects through a
dynamically loaded d3dx9, D3D meshes, PopAnim, PIEffect, CfgCompiler + FEAST and MoreGamesAd/Flash. Libraries
without official source: JeffLib (13 classes; Bookworm Adventures' port has the parts it used), PIL (21 classes)
and AELib (`.pax` After Effects compositions). Data: `main.pak` (106 MB) with 2,633 `.jp2` across three
resolution sets, 21 `.pam`, 75 `.ppf`, 10 `.pax`, font caches and one segmented MO3; loose `levels/*/*.dat`
("CURV" v15 curves); seven `.fx` effects that need GLSL rewrites. It comes after Bejeweled 3, reusing its layer
and JeffLib. The Mac build is `re/zumasrevenge/mac/` (Zuma's Revenge 1.0, about 13,600 lines of names). Web
only: a PC decompilation is in progress elsewhere (Appendix B), so `games/zuma*` is in `.publishignore`, but the
web build may go on the website.

### 5.18 Talismania Deluxe

**Sonnet, published, ~12-16x after Feeding Frenzy 2; HARD at about 20-25x on its own.** It comes last, once
Feeding Frenzy 2 has built the second-generation engine.

**Install and image.**
- The install is `A:\SteamLibrary\steamapps\common\Talismania Deluxe`.
<!-- private: - `Talismania.exe` is the 2006-08-22 DRM wrapper. The game PE is carved to `re/talismania/bin/TalismaniaGame.exe`. -->
- `Talismania.exe` (4,678,050 bytes) is the 2006-08-22 DRM wrapper. The game PE is at 0x591a2 (4,313,088 bytes), carved to `re/talismania/bin/TalismaniaGame.exe`.
<!-- /private -->
- The game PE was linked 2006-07-28 with VS2005 RTM and LTCG (392 LTCG objects). Its internal name is "Atlantis"
  (`AtlantisGameMgr.cpp`).
- The game talks to the wrapper over registered-message IPC (`PopCapDRM_*`), and saves go to the registry plus a
  hashed blob. Both need stubbing or replacing.

**Developer and engine.** By RocketJump Games (Wes Cumberland, the credited sole programmer), with design and
production from ex-Sprout Iggle Pop staff. The engine is **Sprout Games' second-generation engine**, not
SexyAppFramework: the namespaces `engine` (62 classes: a Java-style Object, Reader, Runnable, Thread, Scene,
ScreenLayout, ArchiveLoader/ArchiveToc, ParticleSystem, Splat, QDParser), `dxdriver` (16), `simplesound` and
`simplesounddx` (12), `shape` (collision), sigslot for events, and about 70 global framework classes such as
GenericApplication, Theme, SplinePath, SurfaceCombiner, PixelCombiner, StateMgr, SoundMgr and BassMusicPlayer.
Feeding Frenzy 2 has almost the same engine (59/62, 16/16, 12/12 and about 70 shared framework classes).
Rendering is 800x600 DirectDraw 7 through a software DrawContext with BltFx, PixelFx and WaveFx effects. The
public libraries are sigslot (public domain), zlib 1.1.4, libpng 1.2.5, libjpeg 6b, libogg/libvorbis and
libtheora; BASS is a closed DLL; the engine has its own XML parser.

**Code.** 34,453 functions, 2.97 MB, with full RTTI (841 type descriptors, 836 vtables). The game part
(~0x401000-0x5f6000) has 325 game classes with 4,813 RTTI-attributed methods (384 KB, about 100K decompiled
lines), 7,795 unattributed functions (1.18 MB, about two thirds of them template duplicates; about 520 KB after
deduplication) and 655 checked-STL functions. The realistic translation target is **150-200K decompiled lines**,
against Bejeweled's 12.7K; the engine adds about 100K, including the software blitters. The game has 12 cell
types, paths, pots and the Gameboard, the GameLevel, BonusLevel and AtlantisChapter state machines, map and
building construction, a narrator and journal, the HUD, and about 40 `*ClassLoader` XML data loaders.

**Data formats.**
- **`Talismania.saf`** (11.5 MB) is a **Sprout Archive File**, decoded and checked byte for byte in the survey:
  - Header: `FFAS` (0x53414646 LE), u16 1, u16 0, u32 TOC offset.
  - TOC: u16 1, u16 0, a 16-byte keyed MD5 of the rest of the TOC, u32 count, then entries
    `{u32 off, u32 size, keyed-MD5(file)[16], u16 namelen, name\0}`.
  - Keyed MD5 is standard MD5 with the IV shifted by the seed 0x53414646 times {11, 71, 37, 97}. Both the TOC and
    per-file digests verify, so archives can be repacked.
  - Data is uncompressed and unencrypted. Word Harmony's engine reads the same format
    (`games/wordharmony/engine/ResourceManager.h`), without checking the digests, as its original does not.
- **Archive contents** (2,529 entries): 1,056 PNG and 935 JPG (914 with `_name.png` masks); 448 XML files with
  about 180 element types; 73 OGG; and 14 `.theora` sprite animations. Each animation is a sequence of serialized
  `ogg_packet` headers (`{i32 bytes, i32 b_o_s, i32 e_o_s, i64 granulepos, i64 packetno}` followed by the
  payload), and alpha comes from `_alpha.N.png` frames.
- **Loose files:** `music/*.ogg` (6 BASS streams), `splash/` and `properties/partner.xml`. A log string ("FOUND
  Talismania.saf, USING ARCHIVE") suggests the engine also reads loose files.

**The SAF tool.** Talismania's session also publishes a working version of the SAF Extractor/Packer, a tool for
these archives, to the user's fork, https://github.com/allisonpelton/Popcap_.SAF_Extractor-Packer (the user,
2026-11-04; COORDINATION.md, "Publishing").

**Plan.** Translate the game classes on Feeding Frenzy 2's engine, starting from the RTTI-named state machines
(GameLevel, AtlantisChapter, the CBaseCell family, Gameboard) and the `*ClassLoader` XML parsers, which must
reproduce every attribute exactly. Merge each JPEG with its mask; play `.theora` through libtheora and the music
through the platform's OGG path instead of BASS; draw at 800x600; keep saves in files instead of the registry
and the hash blob; and make the loading thread (`ILoadingThread`) synchronous or cooperative for WASM. What
makes it large: 325 classes plus about 1.2 MB of unattributed code, much of it template bloat to tell apart from
real logic, LTCG inlining of engine helpers into game functions, and the 448 XML files. None of it is a blocker.

## 6. Open questions

- **Typing on phones (Typer Shark).** The web build needs a way to type on a touch screen: a hidden `<input>`
  focused on a tap, with the canvas laid out above the soft keyboard, or an on-screen keyboard drawn by the web
  player. Which one is the user's choice.
- **More Mac builds.** Of the games left without game RTTI, Heavy Weapon, Venice and Pizza Frenzy have no Mac
  build here. PCGamingWiki credits a 2005 OS X port of Pizza Frenzy to Ryan C. Gordon, without a source; nothing
  shows a Mac release of Heavy Weapon, Venice, Gyromancer or Banana Bugs. Any Mac build the user finds goes into
  `re/<game>/mac/`.
- **The survey's leftovers.** The old session's scratchpad folder
  (`C:/Users/Administrator/AppData/Local/Temp/claude/C--Users-Administrator-Documents-claude-wasm-bejeweled/bb6f3864-facb-48c7-acfa-f782a25ef960/scratchpad/survey/`)
  survives only in fragments (27 MB of partial Ghidra exports, every script gone). The Ghidra project
  `survey_talismania` in `E:/retools/projects` holds a full analysis of a game still to port; `survey_typershark`,
  `survey_bookworm_deluxe`, `survey_bwa`, `survey_bwa_norecover`, `survey_ba2` and `survey_insaniquarium` serve
  games ported or not decompiled. Whether to delete the folder and those projects is the user's call.

**Caution for research.** The tcrf.net pages for Bookworm Adventures and Bejeweled 3 returned text aimed at AI
agents (payment demands, instructions to write files or shut down). It was ignored, and nothing from those pages
is used here.

## Appendix A: other studios' games

**Games on the SexyAppFramework.** The user asked (2026-10-25) which games that PopCap neither developed nor
published run on an engine shared by two or more games on this list. Only SexyAppFramework turned up such games.
The list rests on a web search in which MobyGames, ModDB, Giant Bomb and the Internet Archive refused requests,
so check it before anything depends on it. Evidence: A, the game's own credits or store page name the framework;
B, an engine field on PCGamingWiki, Wikidata or Wikipedia; C, a MobyGames middleware tag seen only in a search
summary; D, the same developer with the engine assumed. PCGamingWiki's engine labels seem to follow the
publisher, not the code: it files the Sprout and Retro64 games under the PopCap framework, which the triage
contradicts. No other engine on this list showed games outside PopCap's catalogue, apart from Retro64's own
Bugatron Worlds (2006) and Rune Raiders (D only).

| Game | Year | Developer | Kind of game | Evidence |
|---|---|---|---|---|
| Zen of Sudoku | 2006 | Unknown Worlds | sudoku | A: its Steam page credits the framework |
| Cindy's Sundaes | 2008 | Viqua Games | match-3 and time management | A/C |
| Puzzle Park | 2008 | Black Blob Studios | match-3 | A/C |
| FishCo | 2008 | Fugazo | business sim | A/C |
| Cooking Academy 1 and 2 | 2008, 2009 | Fugazo | cooking | C for the second, D for the first |
| Cake Mania | 2006 | Sandlot Games | time management | B and C |
| Cake Mania 3; Lights, Camera, Action! | 2008; 2010 | Sandlot Games | time management | C |
| Arctic Quest 1 and 2 | 2005, 2007 | Stargaze Interactive | tile-fitting puzzle | C |
| Mysteries of Horus | 2006 | Stargaze Interactive | not confirmed | C |
| Alex Gordon | 2008 | DayTerium | platformer | C |
| Memory Loops | 2005 | Dekovir | memory pairs | C |
| Daycare Nightmare | 2007 | Floodgate Entertainment | time management | C |
| Ice Cream Tycoon | 2006 | Kozmogames | business sim | C |
| Operation Mania | 2008 | Magellan Interactive, for Pogo | time management | C |
| Winter in New York | 2019 | PixQuake | tile matching | C: credits "PopCap Framework 1.3" |
| Winter Magic Factory | 2017-19 | PixQuake | match-3 | C (snippet only) and D |
| The Far Kingdoms: Age of Solitaire | 2015-18 | Lazy Turtle | solitaire | C (snippet only) |
| World of Goo | 2008 | 2D Boy | physics puzzle | B, weak: Wikipedia lists the framework among its tools, probably in part |

Eight hidden-object games on the framework were left out by the user's rule against the genre. No decompilation
or source release turned up for any of these. **Decision (the user, 2026-10-25):** the other studios' games come
at the very end, after every PopCap game, and World of Goo is wanted among them; which of the others to port is
decided then.

**Octogeddon, first of them (the user, 2026-11-01):** "Spiritually, it's a PopCap game." By All Yes Good; the
user's Steam copy is app 525620 (705 MB). It is a 4.8 MB 32-bit `octogeddon.exe` on Direct3D 9, DirectSound and
XInput, with `steam_api.dll`, on the studio's own C++ framework (`AppLib`, with `AnimLib`, `Json` and `Pak`).
MSVC RTTI names 447 classes, the game's own among them. Its data is one 700 MB archive, `data.ayg`, whose header
starts "AYGP". The Ghidra route fits, with SDL in place of Direct3D 9 and DirectSound.

**Three Unity games (the user, 2026-10-27):** Cartoon Network games whose Steam copies are Unity Mono builds,
with the game's own code in `Assembly-CSharp.dll` and no IL2CPP.

| Game | Year | Developer | Unity | Install | Notes |
|---|---|---|---|---|---|
| Steven Universe: Save the Light | 2017 | Grumpyface Studios | 2018.1.0f2, 32-bit | 4.9 GB | 3D; Steamworks.NET |
| Steven Universe: Unleash the Light | 2019 | Grumpyface Studios | 2020.3.12f1, 64-bit | 1.7 GB | 3D; Steamworks.NET |
| OK K.O.! Let's Play Heroes | 2018 | Capybara Games | 5.6.1f1, 64-bit | 5.7 GB | FMOD Studio, InControl |

The Bejeweled method does not fit them, and the user doesn't want the engine decompiled ("I'd just love to play
them in my browser and make modding easier"). Modding uses BepInEx with Harmony patches on the native game.
Browser play comes from a local WebGL rebuild of the player's own copy: its assets ripped into a Unity project,
the decompiled scripts added, and the project built for WebGL in the matching editor. Such a build holds the
publisher's assets and code, so it stays private and never goes on the website. Unleash the Light is the best
first try (2020.3 LTS, no FMOD); the weak points are shaders recovered from a build, the WebGL memory limit and
FMOD's web libraries. Every tool (AssetRipper, Unity Hub and the editor, a .NET decompiler) is a
download that needs the user's OK, and the user accepts Unity's license themselves.

**Glass Tower, near the end (the user, 2026-11-01):** a discontinued Android game that won't run on modern
Android. The user's copy is an APK in the Downloads folder (version 1.0.10, 40.7 MB): a Java shell with 2016-era
ad and Play services libraries, which are left out, and the game as a 5.9 MB native library built only for the
old 32-bit `armeabi` ABI; its levels are XML files with PNG maps. The user would like native, browser and
Android versions; the Ghidra route fits, on ARM code, and an Android build would be a new target.

**The Tasty Planet series, after Glass Tower (the user, 2026-11-01):** Dingo Games' series. The user doesn't own
every game yet. Three are in the Steam library on A: (Tasty Planet, Tasty Planet 2, and Tasty Planet Forever,
app 946010, 78 MB). Forever is a 4.4 MB native exe, `tastyplanet4.exe`, on SDL2 and SDL2_mixer with
`steam_api.dll`, its data in `assets/`; the port can keep SDL2. The two older games were not looked at.

## Appendix B: other projects and modding communities

Checked on 2026-10-26 for the publishing rule (section 2). "Active" means mods or tools since late 2024:
"strong" is many authors or regular releases, "thin" a handful, often from one person. Dates are the last
activity seen. "None found" means GitHub, GameBanana and Nexus showed nothing; Reddit, ModDB and Discord servers
were not read. No port reads any of these projects (CLAUDE.md, "No references"); they matter only for where a
port is published.

| Game | Other decompilation or remake | Modding | Result |
|---|---|---|---|
| Peggle Deluxe | [Thunderball](https://github.com/teampopwork/Thunderball), matching decompilation, 39% (2026-10-18); Xbox 360 recompilations | strong: the [haggle](https://github.com/PeggleCommunity/haggle) mod kit (2026-08), a few GameBanana mods | unpublished |
| Peggle Nights | none of its own (the Xbox 360 recompilations include it) | strong: [GameBanana](https://gamebanana.com/games/6197), 82 mods, nine in 2026 from eight people (newest 2026-10-01); [PeggleEdit](https://github.com/IntelOrca/PeggleEdit) level editor (2026-03) | unpublished |
| Zuma Deluxe | [CircleShootApp](https://github.com/alula/CircleShootApp), finished and playable (quiet since 2025-12), with active ports and a browser build | strong: [GameBanana](https://gamebanana.com/games/8223), eight mods in 2026 from four people (six from one); new level tools in 2026-10 | not decompiled |
| Pizza Frenzy | one unlicensed upload of a decompilation (2026-10-03), nothing since | none found | published |
| Chuzzle Deluxe | a repository whose 13 game files are empty (2026-04); an iOS recompilation | dormant (one mod, 2024) | published |
| Bejeweled 3, Zuma's Revenge | console or phone builds being recompiled or decompiled (2026-10), which don't count | thin | published (Zuma's Revenge: the next row) |
| Zuma's Revenge (PC) | a decompilation in C++ by a developer in China, written without AI, reported to the user on 2026-10-26; a clip showed it crashing at the main menu | | web only, until the user learns more |
| Heavy Weapon, Feeding Frenzy 2 | Heavy Weapon has a reimplementation for the Vita | thin | published |
| every other remaining game | none found | none found | published |
| the ported games | Alchemy: flamel, a web remake written from scratch (2026-10); the others none | thin or none (a few GameBanana mods, one person's tools) | published |

Plants vs. Zombies and Insaniquarium were not rechecked: both have finished decompilations, many ports and large
communities.

## Appendix C: where the copies are

The remaining games' copies, all read only:
- **Steam, `A:/SteamLibrary/steamapps/common/`:** Bejeweled 3, Chuzzle Deluxe, Feeding Frenzy 2 Deluxe, Hammer
  Heads Deluxe, Heavy Weapon Deluxe, Iggle Pop Deluxe, Peggle Deluxe, Peggle Extreme, Peggle Nights, Pizza
  Frenzy, Talismania Deluxe and Zuma's Revenge, and among the other studios' games World of Goo, Octogeddon and
  the Tasty Planet games.
- **Steam, `E:/SteamLibrary/steamapps/common/`:** Gyromancer, a second Peggle Extreme, and the three Cartoon
  Network games.
- **`C:/Program Files (x86)/PopCap Games/`:** Venice Deluxe, Banana Bugs and Peggle World of Warcraft Edition.
  Venice's and Banana Bugs' installers are also unpacked, unrun, into `re/_installers/`.
- **`C:/Program Files (x86)/Popcap Game Collection/`:** Feeding Frenzy Deluxe.
- **The Downloads folder:** Glass Tower's APK, the other Banana Bugs installers and the Mac downloads of Iggle
  Pop and Feeding Frenzy.

In the main checkout (git-ignored), `re/<game>/bin/` holds the exe or carved image of every remaining game,
`re/peggle/<edition>/` the four Peggle images with their Ghidra exports, and `re/<game>/mac/` the Mac builds.
