# PopCap games survey: which other games can be decompiled and ported

Survey date: 2026-10-18. This survey covers the 26 PopCap and PopCap-published games installed on this machine, in 27 install folders (Bejeweled Twist is installed twice, from Steam and from a retail installer). Bejeweled Deluxe, the game this repository already ports, is the baseline.

The facts come from nine survey passes:
- Six ran the full Ghidra pipeline: BookWorm Deluxe, Bookworm Adventures Deluxe, Bookworm Adventures Volume 2, Insaniquarium Deluxe, Talismania Deluxe and Typer Shark! Deluxe.
- Three did static analysis of the other 20 games without Ghidra: PE and Rich headers, imports, RTTI, vtables, strings, link-order regions and pak listings. Their code sizes are therefore address-range estimates.

Nothing in any install folder was modified. Every GitHub repository cited below returned HTTP 200 on the survey date; three have been renamed, and their current URLs are given. The working files are listed in [Appendix B](#appendix-b-survey-artefacts).

Contents:
1. [Method and what makes a good candidate](#1-method-and-what-makes-a-good-candidate)
2. [All games at a glance](#2-all-games-at-a-glance)
3. [Priority games](#3-priority-games): [BookWorm Deluxe](#31-bookworm-deluxe), [Bookworm Adventures Deluxe](#32-bookworm-adventures-deluxe), [Bookworm Adventures Volume 2](#33-bookworm-adventures-volume-2), [Insaniquarium Deluxe](#34-insaniquarium-deluxe), [Talismania Deluxe](#35-talismania-deluxe), [Typer Shark! Deluxe](#36-typer-shark-deluxe)
4. [Recommended order](#4-recommended-order)
5. [Open questions and decisions for the user](#5-open-questions-and-decisions-for-the-user)
- [Appendix A: the other games](#appendix-a-the-other-games)
- [Appendix B: survey artefacts](#appendix-b-survey-artefacts)

---

## 1. Method and what makes a good candidate

### 1.1 The Bejeweled method

See [RE_NOTES.md](../games/bejeweled/docs/RE_NOTES.md) and the Bejeweled README section "[How it was reverse engineered](../games/bejeweled/README.md#how-it-was-reverse-engineered)". The four steps are:

1. **Carve.** The Steam exe is PopCap's DRM wrapper with the real game PE inside it. `tools/extract_game_exe.py` copies it out. It and `tools/triage_popcap.py` are private tools: they are not published.
2. **Analyse.** Ghidra 12 runs headless, recovers C++ classes from RTTI and exports every decompiled function (`tools/ghidra_import.sh`).
3. **Identify the framework.** Framework functions are matched against the public SexyAppFramework (SAF) trees (1.2, 1.3, 1.34). For Bejeweled, only about 60 KB of the 1 MB `.text` was game code; the rest was framework, libraries and CRT.
4. **Translate.** The game classes are translated function by function onto our portable SAF 1.3 (`framework/`, SDL2 + Emscripten), with every function annotated with its original address.

### 1.2 What makes a game a good candidate

These are listed roughly in order of how much they affect the effort:

1. **There is a public decompilation to build on.** The work then becomes integration onto our framework, a mod layer and an accuracy pass. This applies to Insaniquarium, Zuma Deluxe, Plants vs. Zombies, Pizza Frenzy and, partly, Peggle Deluxe.
2. **The game runs on a SexyAppFramework generation with public source** (1.2 or 1.3). Step 3 works, and the game's calls map directly onto our port. Older (2003-2005) builds need a compatibility layer. Non-SAF engines (Sprout, Raptisoft) lose step 3 entirely.
3. **The game classes have RTTI.** Class names and vtables then come for free. Dynomite, AstroPop, Heavy Weapon, Chuzzle, Pizza Frenzy, Iggle Pop and Plants vs. Zombies were built without game RTTI.
4. **There is no private middleware.** The PopLua scripting VM, PopAnim, PIEffect, PIL, AELib, JeffLib and the 2008+ 3D/shader renderers have no public source, so each has to be rebuilt.
5. **The game code is small.** Bejeweled's is about 60 KB and about 250 functions. Most other games are 3-30 times that.
6. **The data is in formats the port already reads**: loose files or `main.pak`, ImageFont `.txt` fonts, OGG, and S3M/IT tracker music. MO3 music, JPEG 2000 images, Flash cutscenes and custom archives each add a decoder or a conversion step.

Getting an analysable image is not a differentiator. None of the 26 games uses a commercial packer on its game image (see 1.4).

### 1.3 Verdicts and effort

| Verdict | Meaning |
|---|---|
| **DIRECT** | Steps 1-4 work as they are. The framework generation has public source, the game classes have RTTI, and the only extra work is decoders shared with other games (MO3, JPEG 2000). |
| **ALREADY DONE** | A public decompilation of the same game exists and is playable or nearly so. The remaining work is moving it onto our framework, adding mods and checking accuracy. |
| **EXTRA WORK** | The method works, but the game also needs one of: a compatibility layer for a pre-public framework, manual class recovery (no game RTTI), a private PopCap library, Flash, or a 3D/shader backend. |
| **HARD** | Step 3 fails (the engine is not SAF and has no public source), or the code is very large, and there is little or no outside reference. |

Effort is given relative to Bejeweled Deluxe = 1x. Bejeweled's game code is about 60 KB, about 250 functions and about 12.7K decompiled lines. Effort is estimated from game-code size plus the extra subsystems listed for each game. Treat every estimate as ±50%.

### 1.4 Findings that apply to many games

<!-- private: **Protection.** No game needs real unpacking. Most builds carry the unencrypted game inside one of four PopCap wrappers or launchers, which the private tool copies it out of; Bookworm Adventures Deluxe, Bookworm Adventures Vol. 2, Peggle WoW and Bejeweled 3 have no wrapper. -->

**Protection.** No game needs real unpacking. There are four wrapper variants, plus a few games with no wrapper:

| Wrapper | Games | How to get the image |
|---|---|---|
| 2006-08-21/22 PopCap DRM wrapper (VC7.1, Flash trial UI), marker `!popcapdrmprotect!` | AstroPop, Big Money (`WinBM.exe`), Chuzzle, Dynomite, Hammer Heads, Heavy Weapon, Iggle Pop, Pizza Frenzy, Rocket Mania, Talismania, Typer Shark | `extract_game_exe.py` unchanged |
| 2007-08-10 SAF + Lua + Flash wrapper (the same wrapper as Bejeweled's `WinBej.exe`) | Bejeweled 2, Feeding Frenzy 2, Insaniquarium, Peggle Deluxe, Peggle Extreme, Zuma Deluxe | unchanged |
| 2008-10-08 wrapper | Bejeweled Twist (Steam), Peggle Nights, Plants vs. Zombies, Zuma's Revenge | unchanged |
| PopCap "CDProtect" launcher, marker `!popcapcdprotect!` | BookWorm Deluxe 1.03 (retail); Bejeweled Twist (retail, ends at `!popcapcdprotend!`) | the extractor must accept the second marker (one-line change, not yet made) |
| no wrapper | Bookworm Adventures Deluxe, Bookworm Adventures Vol. 2, Peggle WoW, Bejeweled 3 | load the exe directly |

<!-- /private -->

Two other notes on protection:
- Big Money's `BigMoney.exe` is iWin's AutoWrap 2.12d build of the same 2005-11-14 link, under an Armadillo/SoftwarePassport-style protector. It is redundant and should be ignored.
- The `.luc` Lua files in the `drm/` folders belong to the wrappers, not the games.

**Ghidra on VS.NET 2002 (MSVC 7.0) binaries.** The BookWorm and Typer Shark passes found four problems. They probably also affect AstroPop, Dynomite, Heavy Weapon, Big Money and Chuzzle.
- Ghidra 12's RTTI analyzer and `RecoverClassesFromRTTIScript` recover **no classes**. MSVC 7.0 writes 24-byte BaseClassDescriptors without the `pClassDescriptor` field, and Ghidra rejects them. Workaround: a Python RTTI scanner feeding `ApplySymbols.java`. Two versions exist, `bookworm/scripts/vtables.py` and `typer-shark/scripts/rtti2sym.py`.
- The "Non-Returning Functions - Discovered" analyzer marks the `_free`/`operator delete` thunks as noreturn. This truncated about 2,225 call sites in BookWorm's first export. Workaround: disable the analyzer in a preScript (`bookworm/ghidra_scripts/DisableNoReturnDiscovery.java`).
- `tools/ghidra_scripts/ApplySymbols.java` fails under the pipeline's JDK 21 with "class file version 67.0", because a JDK 23-compiled class is cached in Ghidra's script bundle. The Typer Shark pass worked around it with renamed copies.
- `analyzeHeadless.bat` breaks `-scriptPath` on `;`, so all scripts have to be in a single directory.

Large binaries have their own problem. On Bookworm Adventures, `RecoverClassesFromRTTIScript` was still running after 50 minutes, so that export used `CLASS_RECOVERY=0` and assigned functions to modules by address region.

**Framework generations**, from vtable slot counts:

| Game | Game PE build | Widget slots | Dialog slots | Other markers |
|---|---|---|---|---|
| Dynomite | 2003-07 | 43 | 48 | no WidgetContainer; SexyApp (61 slots) with no SexyAppBase |
| Rocket Mania | 2003-09 | 44 | 49 | VC6; SexyApp 62 slots |
| AstroPop, Typer Shark, BookWorm Deluxe | 2004-04 | 47 (AstroPop) | 52 | no WidgetContainer, monolithic SexyApp. The AstroPop and Typer Shark framework RTTI sets are identical. |
| Heavy Weapon | 2005-02 | 48 | 54 | SexyAppBase split out (80 slots); FlashWidget |
| Big Money, Hammer Heads | 2005-11, 2006-05 | 54 | 60 | WidgetContainer has 15 slots. This is exactly the public 1.2 tree ([sandboxmo-de/SexyAppFramework-v1.2](https://github.com/sandboxmo-de/SexyAppFramework-v1.2); MSVC 7.0 build harness [sandboxmo-de/msvc70_saf_builder](https://github.com/sandboxmo-de/msvc70_saf_builder)). |
| Bejeweled, Bejeweled 2 | 2007-04 | 70 | 76 | WidgetContainer 30. Bejeweled 2 is identical to Bejeweled in every measured class, so [FRAMEWORK_NOTES.md](FRAMEWORK_NOTES.md) carries over. |

The other SAF games (Zuma, Insaniquarium, the Peggle family, Plants vs. Zombies and the two Bookworm Adventures games) are 1.3-generation. Bejeweled Twist (2008), Zuma's Revenge (2009) and Bejeweled 3 (2011) use later internal generations with no public source.

**Shared components.** Each of these is built once and then serves several games:

| Component | Needed by | Source or reference | Notes |
|---|---|---|---|
| MO3 music with order-position jumps | 17 games: BookWorm, Typer Shark, AstroPop, Heavy Weapon, Zuma, Insaniquarium, Bejeweled 2, all four Peggles, Bookworm Adventures 1, Chuzzle, Feeding Frenzy 2, Twist, Zuma's Revenge, Bejeweled 3 | libopenmpt (BSD-3, builds with Emscripten). `openmpt_module_set_position_order_row` maps directly onto `BASS_MusicPlayEx` positions. | The port's libxmp-lite cannot read MO3. Fallbacks: convert MO3 to IT in OpenMPT, keeping the orders, or render offline with ffmpeg + libopenmpt (which loses the order jumps). PvZ-Portable already uses libopenmpt. |
| Sun `.au` (8-bit µ-law) sounds | Big Money, Insaniquarium (15 files), Typer Shark (3), BookWorm (`blank.au`) | about 30 lines of code | `Mixer::LoadSoundData` only decodes Ogg and WAV. |
| JPEG 2000 (`.jp2`) | Bookworm Adventures (42 files), BA Vol. 2 (34), Peggle Deluxe (217), Extreme (132), Nights (273), WoW (130), Twist (2,364), Zuma's Revenge (2,633), Bejeweled 3 (4,557) | OpenJPEG, or a one-time conversion to PNG with Pillow while staging assets | The originals use the commercial `j2k-codec.dll`. Pillow decoded every BA file it was tried on. |
| Flash SWF | Bookworm Adventures (4 cutscenes), BA Vol. 2 (5 cutscenes + trailer), Heavy Weapon (3 cutscenes), Hammer Heads (how-to-play) | Ruffle in the web build, or pre-rendered video | All are SWF v7/v8 with AS2 and little scripting. |
| Pre-1.2 framework compatibility layer | Dynomite, Rocket Mania (2003); AstroPop, BookWorm, Typer Shark (2004); Heavy Weapon (2005) | Must be derived from the binaries; no public tree this old exists. alula/CircleShootApp shows how to map a 2003 game onto SAF 1.3 with small patches. | One slot/field map per framework generation. |
| PopAnim (`.pam`) player | Bookworm Adventures (v1), BA Vol. 2 (v1-3), Twist, Zuma's Revenge, Bejeweled 3. Bejeweled 2 links it but appears not to use it. | Format references: [PopGameTool/PopStudio](https://github.com/PopGameTool/PopStudio) (Apache-2.0, formerly YingFengTingYu/PopStudio_Old), [LambdaEd1th/pam-codec](https://github.com/LambdaEd1th/pam-codec) (AGPL), [twinstar6980/Twinning](https://github.com/twinstar6980/Twinning) (GPL, formerly TwinStar.ToolKit). Code: [LolHacksRule/Prime](https://github.com/LolHacksRule/Prime) (C++), [bognarit80/BejeweledLivePlusDecomp-MonoGame](https://github.com/bognarit80/BejeweledLivePlusDecomp-MonoGame) (C#). | Magic `0xBAF01954`. |
| PopLua VM + LuaLib/LuaApp bindings | Bookworm Adventures, BA Vol. 2 | Only stock Lua 5.0.2 (MIT) is public | see 3.2 |
| JeffLib | Bookworm Adventures, BA Vol. 2, Zuma's Revenge | C# version in the Bejeweled LIVE+ decompilation | |
| PIEffect, PIL, AELib, CurvedVal, 3D/shader renderer | Bejeweled 3, Zuma's Revenge. Twist needs shaders and P3D meshes but not PIEffect. | Bejeweled LIVE+ C# decompilation; LolHacksRule/Prime | FEAST/CfgCompiler only compile `compat.cfg` (GPU rules) and can be dropped. |
| Sprout engine, `.saf` archives (keyed MD5), `.theora` sprite animations | Pizza Frenzy, Iggle Pop (first-generation engine); Feeding Frenzy 2, Talismania (second generation) | [PopCap-Decompilations/PizzaFrenzy-Decompilation](https://github.com/PopCap-Decompilations/PizzaFrenzy-Decompilation) (first generation; no license) | see 3.5 |
| Prefix-compressed word lists | BookWorm (`wordlist.txt`), Bookworm Adventures (`data/compressed.txt`), BA Vol. 2 (the same plus `compressed_attribs.dat`) | trivial loader | One loader serves all three. |
| Resource-ID tables | Name lists hard-coded in the exe: BookWorm, Typer Shark, Big Money, Rocket Mania. `resources.xml` plus a generated `Res.cpp`: Insaniquarium, AstroPop, Hammer Heads, Bejeweled 2, Peggle, both Bookworm Adventures. | Regenerate from `resources.xml` rather than translating it. [LolHacksRule/PopCapResourceFilesGeneratedCPP](https://github.com/LolHacksRule/PopCapResourceFilesGeneratedCPP) already has Bejeweled 2, Peggle, Peggle Nights and Peggle WoW. | |

---

## 2. All games at a glance

"Build" is the link date of the game PE and its compiler. "Game code" is the game-specific part of `.text` (KB, function count). Rows are grouped by framework family.

| Game | Build | Getting the image | Framework / engine | Notable internal libs | Game code | Existing decomps | Verdict | Effort |
|---|---|---|---|---|---|---|---|---|
| **Bejeweled Deluxe 1.87** (baseline) | 2007-04-13, VS2005 LTCG | 2007 wrapper; carve | SAF ~1.3 | none | ~60 KB / ~250 fn | this repo | done | 1x |
| Dynomite Deluxe (Raptisoft) | 2003-07-08, VS2002 | 2006 wrapper; carve | pre-public 2003 SAF | none | ~262 KB / ~445 fn; **no game RTTI** | none ([level editor](https://github.com/iCTzzz/Dynomite-Deluxe-Level-Editor) only) | EXTRA WORK | ~4.5x |
| Rocket Mania Deluxe (Nuclide) | 2003-09-08, VC6 | 2006 wrapper; carve | pre-public 2003 SAF | none | ~195 KB / ~560 fn; RTTI | none (fan clones) | EXTRA WORK | ~3.5x |
| AstroPop Deluxe | 2004-04-06, VS2002 | 2006 wrapper; carve | pre-public Apr 2004 SAF | none | ~556 KB (67 KB generated) / ~1,030 fn; **no game RTTI** | none | EXTRA WORK | ~7-8x |
| **BookWorm Deluxe 1.03** | 2004-04-15, VS2002 | CDProtect launcher; carve (already done) | pre-public early-2004 SAF | none | ~170-190 KB / ~750 fn; RTTI (custom scan) | none | EXTRA WORK | ~3-3.5x |
| **Typer Shark! Deluxe 1.02** | 2004-04-20, VS2002 | 2006 wrapper; carve (already done) | pre-public Apr 2004 SAF | none | 379 KB / 1,219 fn; RTTI (custom scan) | none | EXTRA WORK | ~5x |
| Heavy Weapon Deluxe | 2005-02-16, VS2002 | 2006 wrapper; carve | pre-1.2 SAF (early 2005) | FlashWidget cutscenes | ~300 KB / ~690 fn; **no game RTTI** | [heavy-weapon-deluxe-vita](https://github.com/Chillaxhson/heavy-weapon-deluxe-vita) (reimplementation, not a decomp) | EXTRA WORK | ~5x |
| Big Money Deluxe | 2005-11-14, VS2002 | `WinBM.exe` 2006 wrapper; carve | public SAF 1.2 layout | none | ~130 KB / ~322 fn; RTTI | none | DIRECT (already planned) | ~2x |
| Hammer Heads Deluxe (Nuclide) | 2006-05-17, VS2003 | 2006 wrapper; carve; stub the DRM IPC client | SAF 1.2 layout + FlashWidget | TinyXML; Nuclide property system and level editor | ~418 KB (~300 KB gameplay) / ~1,890 fn; RTTI | none | EXTRA WORK | ~6x |
| Zuma Deluxe | 2007-04-04, VS2005 | 2007 wrapper; carve | SAF ~1.3 | none | ~360 KB; RTTI | [alula/CircleShootApp](https://github.com/alula/CircleShootApp): playable, built from the 2003 1.0.0 exe | ALREADY DONE | ~0.5-1x |
| **Insaniquarium Deluxe 1.1** | 2007-06-28, VS2005 | 2007 wrapper; carve (already done) | SAF 1.3 | none | ~497 KB / 1,568 fn; RTTI | [Vindirect/WinFish](https://github.com/Vindirect/WinFish): playable, plus many ports | ALREADY DONE | ~1-1.5x |
| Bejeweled 2 Deluxe | 2007-04-11, VS2005 LTCG | 2007 wrapper; carve | SAF ~1.3, same vtables as Bejeweled | PopAnim (linked, appears unused) | ~325-343 KB / ~1,450 fn; RTTI | none | DIRECT | ~6x |
| Peggle Deluxe 1.01 | 2007-07-16, VS2005 | 2007 wrapper; carve | SAF ~1.3 | none | ~750 KB; RTTI | [teampopwork/Thunderball](https://github.com/teampopwork/Thunderball): 38.9% of all functions implemented, MIT | DIRECT (note 1) | ~5x on Thunderball (~12x from scratch) |
| Peggle Extreme | 2007-08-22, VS2005 | 2007 wrapper; carve | SAF ~1.3 | none | ~635-760 KB; 84% of functions identical to Deluxe | none (Deluxe via Thunderball) | DIRECT | ~1x after Deluxe |
| Peggle Nights | 2008-09-26, VS2005 LTCG | 2008 wrapper; carve | SAF ~1.3 | none | ~700-950 KB; RTTI | none ([haggle](https://github.com/PeggleCommunity/haggle) mod SDK has Nights class layouts) | DIRECT | ~4-5x after Deluxe |
| Peggle World of Warcraft Edition | 2009-06-19, VS2005 LTCG | no wrapper | SAF ~1.3 (Nights branch) | none | ~660-880 KB; RTTI | none | DIRECT | ~1-2x after Nights |
| Plants vs. Zombies GOTY 1.2.0.1096 | 2010-12-10, VS2008 | 2008 wrapper; carve | internal SAF (~2009-10) | TodLib (rebuilt in the decomps) | >1 MB; **no game RTTI** | [wszqkzqk/PvZ-Portable](https://github.com/wszqkzqk/PvZ-Portable): about 100% playable, has a WASM build, LGPL-3.0 | ALREADY DONE | ~0.3-0.5x (fork) |
| **Bookworm Adventures Deluxe** | 2007-06-20, VS2005 | no wrapper | SAF ~1.3 + PopAnim | PopLua, LuaLib, LuaApp, JeffLib, XWidget (none public) | C++ ~588 KB / ~2,650 fn, plus 621 `.luc` scripts (~78K lines) | none (tools only) | EXTRA WORK | ~10-15x (engine shared with Vol. 2) |
| **Bookworm Adventures Vol. 2** | 2009-07-22, VS2005 | no wrapper | SAF 1.3 generation + PopAnim v1-3 | as Vol. 1 | C++ ~557 KB / ~1,994 fn, plus 462 `.luc` | none (tools only) | EXTRA WORK | ~15x alone; ~4-6x after Vol. 1 |
| Bejeweled Twist 1.0.3 | 2008-10-17, VS2005 partial LTCG | Steam: 2008 wrapper, but the inner exe is locally patched. Retail: CDProtect wrapper, clean image. | internal mid-2008 SAF with D3D7/8/9, shaders, P3D meshes | PopAnim, FEAST (droppable) | ~0.93-1.1 MB + ~0.4 MB framework; RTTI | none (patches and mods only) | EXTRA WORK | ~25x |
| Zuma's Revenge! | 2009-08-14, VS2005 LTCG | 2008 wrapper; carve; uses `steam_api` | internal 2009 SAF: RenderDevice3D, D3D8/9, `.fx` effects | JeffLib, PIL, AELib, PIEffect, PopAnim, FEAST | ~1.15-1.75 MB + 0.6-1 MB libraries; RTTI | [devcronx/zuma-s-revenge-64-bits](https://github.com/devcronx/zuma-s-revenge-64-bits) (C#, appears derived from the WP7 port; unverified, no license) | HARD | ~35x (~15-20x with C# references) |
| Bejeweled 3 | 2011-10-26, VS2008 LTCG | no wrapper; uses `steam_api` | post-2010 "prime" framework (drivers, RenderDevice3D, `.d3dfx`) | PopAnim, PIEffect, CurvedVal, FEAST | ~1.0-1.13 MB + ~0.6 MB framework; RTTI | [BejeweledLivePlusDecomp-MonoGame](https://github.com/bognarit80/BejeweledLivePlusDecomp-MonoGame) (C# of the WP7 LIVE+ port, MIT); [Prime](https://github.com/LolHacksRule/Prime) (framework, abandoned) | EXTRA WORK | ~30x (~10-15x with references) |
| Chuzzle Deluxe (Raptisoft) | 2005-05-10, VS2002 | 2006 wrapper; carve | Raptisoft engine (not SAF, no public source) | none | ~300 KB game + ~320 KB engine; **no RTTI** | [TC999/Chuzzle-Deluxe](https://github.com/TC999/Chuzzle-Deluxe) (empty placeholder files) | HARD | ~8-12x |
| Pizza Frenzy (Sprout) | 2006-02-21, VS2003 | 2006 wrapper; carve | Sprout engine, first generation | none | ~455 KB game + ~265 KB engine; **no RTTI** | [PizzaFrenzy-Decompilation](https://github.com/PopCap-Decompilations/PizzaFrenzy-Decompilation): full game and engine, addresses match, no license, not yet built | ALREADY DONE | ~1-1.5x |
| Iggle Pop Deluxe (Sprout) | 2006-02-21, VS2003 | 2006 wrapper; carve | Sprout first generation (72% of engine functions identical to Pizza Frenzy) | none | ~480 KB / ~1,200 fn; **no RTTI** | none | EXTRA WORK | ~6x after Pizza Frenzy |
| Feeding Frenzy 2 Deluxe (Sprout) | 2006-02-24, VS2005 | 2007 wrapper; carve | Sprout second generation | sigslot, libtheora (public) | ~470 KB game + ~570 KB engine; RTTI | none | EXTRA WORK | ~10x after Pizza Frenzy |
| **Talismania Deluxe** (RocketJump / Sprout engine) | 2006-07-28, VS2005 LTCG | 2006 wrapper; carve (already done) | Sprout second generation (almost identical to Feeding Frenzy 2) | sigslot, libtheora (public) | 150-200K decompiled lines (~12-16x) + engine; RTTI | none | HARD | ~20-25x (~12-16x once the FF2 engine exists) |

Bold = the user's priority games (section 3).

Note 1: Peggle Deluxe's survey pass rated it EXTRA WORK, citing JPEG 2000 and Thunderball being unfinished. The pass that covered Peggle Extreme, Nights and WoW rated them DIRECT, and they have the same requirements. This report uses DIRECT for the whole family and lists JPEG 2000 as a shared prerequisite.

**Totals:**
- DIRECT: 6 (Big Money, Bejeweled 2, the four Peggles).
- ALREADY DONE: 4 (Insaniquarium, Zuma Deluxe, Plants vs. Zombies, Pizza Frenzy).
- EXTRA WORK: 13.
- HARD: 3 (Talismania, Chuzzle, Zuma's Revenge).

No game was found to be impossible.

---

## 3. Priority games

### 3.1 BookWorm Deluxe

**Verdict: EXTRA WORK, about 3-3.5x Bejeweled. No public decompilation, source port or source leak exists.**

**Install and image.**
- The install is `C:\Program Files\PopCap Games\BookWorm Deluxe`: version 1.03, a retail/partner "CD-protect" edition with `partner.xml NoReg=True`.
<!-- private: - `Bookworm.exe` is not the game but PopCap's CDProtect launcher ("CDProtectRun") with the game inside it. -->
- `Bookworm.exe` (1,425,425 bytes) is not the game. It is a 106,496-byte launcher, "CDProtectRun". The game PE follows the 17-byte marker `!popcapcdprotect!` at 0x1a000; the game starts at 0x1a011 and runs to EOF (1,318,912 bytes).
- At runtime the launcher checks a hardware fingerprint (`hw.dat`, built from the MAC address and volume serial) or the original CD (`\cdinstall.dat`). It then copies the game bytes unchanged to a hidden `game.exe`, runs it and deletes it. So a static carve gives exactly the bytes the launcher runs.
<!-- /private -->
- The carve is done (`bookworm/bin/BookwormGame.exe`). The triage JSON's 33 KB `.text` figure measured the launcher, not the game.

**Build and framework.**
- The game PE was linked 2004-04-15 with MSVC 7.0 (VS.NET 2002, build 13.00.9466). That is the same toolchain as Zuma 1.0.0, the build alula/CircleShootApp decompiled.
- The framework is an **internal SAF from early 2004, older than any public release**:
  - a monolithic `Sexy::SexyApp`, with no `SexyAppBase`;
  - `Widget` with no `WidgetContainer`;
  - no `PakInterface`, so all files are loose;
  - DirectDraw7/D3D7, `DSoundManager` (OGG), `BassMusicInterface` (BASS 2.0, MO3 by order position) and `FModMusicInterface`;
  - ImageFont/DescParser;
  - `ResourceManager`, linked but unused.
- Every class name exists in the public trees, so functions can be identified by name and strings. Layouts and behaviour differ, however: Widget/Dialog/ButtonWidget fields, ImageFont, and the old SexyApp API.

**Libraries.** BookWorm uses none of PopCap's later internal libraries. It links ImageLib with libpng, IJG libjpeg 6b and zlib 1.1.3, and the VC7 iostreams (`FameWidget` uses `fstream`).

**RTTI.** There are 165 type descriptors. Ghidra recovered nothing, as described in 1.4. A Python RTTI scan fed into `ApplySymbols` named 849 functions.

**Game code.**
- The game region is 0x401000-0x437b80: 861 functions, 216.5 KB, including about 15 KB of iostream/STL code. The real game logic is about **170-190 KB in about 750 functions**.
- The largest parts are `BookWormApp` (44 KB: loading, profiles, registry, registration, music) and `Board` (43 KB: rules, burning tiles, scoring, bonus words, animations), followed by `FameWidget` (16 KB) and `BWGameSelector` (7 KB).
- The trial, registration, money and update dialogs and the internet high scores (about 15-20 KB) can be dropped.

**Data formats and port work.**

| File(s) | Format | Port work |
|---|---|---|
| `wordlist.txt` (767 KB) | Sorted and prefix-compressed. Each line is an optional decimal N (characters shared with the previous word; when omitted, the previous N is reused) followed by the suffix, e.g. `aa`, `2h` → aah, `3ed` → aahed. It holds 151,477 words of 2-12 letters, and the loader keeps only words of 3 or more letters. | Trivial loader. Bookworm Adventures' `compressed.txt` uses the same scheme. |
| `bonus_word_dictionary.txt` | 1,785 words | trivial |
| `popdefs.txt` | 2,249 lines of `WORD<TAB>(pos.) definition`, Latin-1 | trivial |
| `fonts/*.txt` | SAF ImageFont descriptors (several fonts are alpha-mask only) | already supported |
| `images/` (279 files) | GIF/JPG/PNG at 640x480, with `_name` alpha masks; sprite strips (`lex_*`, `tile_*`, `fx_*`) | A generated `Res.h`-style table, because names are hard-coded in `.rdata` and `BookWormApp` and there is no `resources.xml`. |
| `sounds/` | 45 `.ogg` plus `blank.au`. The `cached_*.wav` files are runtime caches. | AU decoder (or drop the file); ignore the caches |
| `music/Bookworm1.mo3` | One MO3 module by Skaven (2003), with sections chosen by order position: 0x00 main, 0x1B menu, 0x25 action, 0x33 danger, 0x36 high scores, 0x3B/0x3D game-over stingers, 0x3F silence, 0x41 action danger | libopenmpt, or an offline conversion to IT that keeps the orders |
| `properties/partner.xml` (+ `.sig`) | PropertiesParser switches (NoReg, DontUpdate, NoInternetScores, LoadingBarY...) | read or emulate |
| `players/`, `classic.dat`, `action.dat`, `HKLM\SOFTWARE\PopCap\BookWorm` | Profiles, saved games (including a converter for an older save format) and registry values | Reimplement on the port's storage and `registry.ini` |

**Existing work.** There is no decompilation. Related projects:
- [JacksonDam/PopSilicon](https://github.com/JacksonDam/PopSilicon) runs the Mac 1.0.5 binary on Apple silicon. It shows that a Mac build exists, which might be a second reference binary; whether its symbols are stripped is not known.
- [thelabcat/cookworm](https://github.com/thelabcat/cookworm) (Apache-2.0) and [cookworm-c](https://github.com/thelabcat/cookworm-c) edit `wordlist.txt` and `popdefs.txt`, so the community already mods these files.
- Fan clones only: [textagons](https://github.com/shadowFAQs/textagons), [Egghead](https://github.com/gabrianrchua/Egghead), [HonKuiMushiGD](https://github.com/jahu00/HonKuiMushiGD).
- The closest precedent is CircleShootApp: the same compiler and a pre-public framework, rebuilt on SAF 1.3 with patches that restore old behaviour.

**Plan.**
1. Tooling: make `extract_game_exe.py` and `triage_popcap.py` accept `!popcapcdprotect!`. Move the RTTI-scan → `ApplySymbols` step and the noreturn-analyzer preScript into `tools/`.
2. Reuse the existing export: 6,103 functions, 103 class namespaces, `game_region.txt` and `vtables_rtti.json`.
3. Write the **2004 framework compatibility map**: the vtable slots and fields of the old Widget, Dialog, ButtonWidget, ImageFont and SexyApp, mapped to our 1.3 API, using CircleShootApp's patches as a guide. Typer Shark and AstroPop reuse it.
4. Translate in this order:
   1. The `BookWormApp` load path: the dictionary, bonus-word and definition loaders, and the image, font and sound name tables.
   2. `Board`, `Piece`, `Lex` and the effects (`EffectMgr`, `EmberListMgr`, `Sparkle`, `FlyingPieceList`, `WordBalloon`, `WordScore`, `Points`).
   3. `FameWidget`, `LevelUpWidget`, `HelpWidget`, `HintDialog`, `OptionsDialog`, `TitleScreen`, `BWGameSelector` and the user dialogs.
   
   Drop `RegisterDialog`, `UpdateCheckDialog`, `MoneyDialog`, and the trial and internet high-score code.
5. Audio: libopenmpt with order-position playback (see 1.4).
6. Persistence: profiles, `classic.dat`/`action.dat`, and the registry values via `registry.ini`.
7. Mods: serve `wordlist.txt`, `bonus_word_dictionary.txt` and `popdefs.txt` through the VFS, keeping the format cookworm-compatible (custom dictionaries, other languages, definitions), plus tile and Lex art.

**Obstacles.** These are the second wrapper marker, Ghidra's VC7.0 problems (both already worked around), the 2004 framework drift, MO3 music, the hard-coded resource names, and the profiles, saves and registry code to replace. There is also more STL-heavy code than in Bejeweled (`std::string`/`list`/`map`/`set`, `fstream`), which makes the decompiler output noisier.

### 3.2 Bookworm Adventures Deluxe

**Verdict: EXTRA WORK.**
- About 10-15x Bejeweled for this volume, including the engine it shares with Volume 2.
- Both volumes together are about 20x.
- No public decompilation exists.
- This is the largest of the priority games, but one engine covers both volumes.

**Install and image.**
- The install is `C:\Program Files (x86)\PopCap Games\Bookworm Adventures Deluxe`.
- `BookwormAdventures.exe` (4,760,904 bytes; internal name WinBAD) is the game itself. It has no wrapper or packer; the overlay is only an Authenticode signature.
- The exe contains a PopCap DRM IPC client and the `drm/` UI. Both are dead code for a port, as are the upsell, MoreGames and UpdateCheck screens.

**Build and framework.**
- Linked 2007-06-20 with VS2005 (14.00.50727), plus some VC7.1 build 4035 static-library objects.
- SAF about 1.3, from a 2006-07 internal build. Its class set matches `E:/retools/refs/pcf`, and it has no XMLWriter, so it predates 1.34.
- It adds PopAnim, CustomResManager, MoreGamesAd and UpdateCheckDialog.

**Internal libraries.** None of these has public source:

| Library | Size | Contents |
|---|---|---|
| **PopLua** | ~157 KB | A Lua 5.0.2 fork with a C-like dialect: classes, `def`, `{}` blocks, `//` comments, `++`/`+=`, a `*t` length operator, 0-based `for(i<n)` loops and an int32 number subtype. The full compiler is in the exe, so the game compiles `.lua` files when it finds them. |
| **LuaLib** | ~120 KB | `LuaLinkedClass`, `LuaSupport`, the ProcComm remote debugger, and 358 `MethodWrapper<>` instantiations |
| **LuaApp** | ~260 KB | `LuaApp`, `LuaWidget` and `LuaWidgetTpl<>` wrappers for 8 widget types, `LuaImage`, `LuaFont`, `LuaResourceManager`, and the DProps property editor |
| **JeffLib** | ~104 KB | StateMachine/State, Animator, SoundPlayer, SexyBassMusicInterface, DataDrivenClass, plus developer dialogs |
| **XWidget::XLayout** | 17 KB | reads `data/layout_*.xml` |
| **PopAnim** | 25-49 KB | the `.pam` player |

**RTTI.** There are 605 type descriptors and 596 vtables. Ghidra's class recovery did not finish within 50 minutes, so the export uses `CLASS_RECOVERY=0` and assigns functions to modules by address region.

**Code.**
- 20,226 functions, 3.10 MB of code in total.
- The game C++ region is 0x401000-0x4c8000: 3,176 functions, 653 KB. After removing STL code, MethodWrappers and generated resource code, about **2,650 functions and 588 KB** are game-specific; about 790 of them are 200 bytes or larger.
- The largest classes are GameApp, Board, ConversationPanel, MainMenu, Credits, BookManager, TreasureScreen, TooltipWidget, HallOfFame and StatsBook.

**Scripts.**
- 621 `.luc` files, with 3,411 functions, 171,271 VM instructions and about 78K source lines. Full debug info is kept: line numbers and 11,221 local names.
- The largest are BattleEngine (5.8K lines), CreatureBaseClass, WordMasterData, Book, BookManager, Lex, LinkNSpell, WordMaster and main. There are 465 creature scripts.
- The scripts reach C++ through **608 distinct method names**.

**PopLua bytecode.** This was decoded in the survey and checked by disassembling BA1's `common/QRand.luc` against the `QRand.lua` source that ships with Volume 2:
- **Header:** ESC `Lua`, version byte 0x56, then `01 01 04 04 04 06 08 09 09 08` and a test double.
- **Per function:** source, line, an implicit-self flag (int), nups, nparams, vararg, maxstack, line info, locals, upvalues, constants, protos and code. Constant tags are 0 nil, 1 bool, 3 double, 4 int32 and 5 string.
- **49 opcodes:** the Lua 5.0 set with MOD and SIZ inserted, then ALTSELF, CONSTGLOBAL, CONSTTABLE, DEFGLOBAL, DEFTABLE, SETSELFORGLOBAL, GETSELFORGLOBAL, SELFORGLOBAL, CALLSELFORGLOBAL, TAILCALLSELFORGLOBAL, INT and BREAK, with CLOSURE moved to 48.
- **Decompilers:** stock Lua decompilers (unluac, ChunkSpy) fail on this bytecode.

**Data formats.**
- **`main.pak`:** 27.4 MB, 5,157 files, standard XOR 0xF7 (`tools/popcap_pak.py` reads it).
- **Scripts:**
  - `scripts/**/*.luc`;
  - `scripts/wordlists/*.luc`: themed bonus-word lists as Lua tables;
  - 166 plain-text `.lore` bestiary entries.
- **Dictionary:** `data/compressed.txt` has 167,624 words in the same prefix-compressed format as BookWorm. `data/defs.bin` is referenced by the exe but not shipped.
- **Images:**
  - 2,053 GIF, 1,769 JPG, 41 PNG and **42 JPEG 2000** (`.jp2`), with SAF alpha masks.
  - Decoded, all images together are about 176 MB of RGBA, but they load per resource group.
- **Animations:** 274 `.pam` files (PopAnim v1, 30 fps), 2,964 sprite images, and 4 PopAnim mod scripts (`.txt`).
- **Fonts and layout:**
  - 11 ImageFont descriptors;
  - `properties/resources.xml` (370 images, 105 sounds, 10 fonts);
  - `Book1-3.xml`, `tiles.xml`, `TimedMode.xml`, `*Treasures.xml`, `config.xml` (letter values);
  - XLayout `data/layout_*.xml`.
- **Audio:** 156 OGG and 4 WAV. All music is in `sounds/music.mo3`, with tracks chosen by order position.
- **Outside the pak:** 4 Flash 7 cutscenes in `images/cutscenes/*.swf`, the `drm/` folder, and `properties/partner.xml`.

**Existing work.** Only tools exist:
- [wxarmstrong/PopLua-Disassembler](https://github.com/wxarmstrong/PopLua-Disassembler): opcode names, with approximate operand types for the PopLua-only opcodes.
- [sharky564/BookwormAdventuresModding](https://github.com/sharky564/BookwormAdventuresModding) (MIT): repacks paks and patches bytecode. It cannot recompile a method; the README says the VM then crashes on load.
- [canairo/bookworm-adventures-unpacked](https://github.com/canairo/bookworm-adventures-unpacked): early RE notes and an asset dump.
- [franklindyer/BWA](https://github.com/franklindyer/BWA): a browser clone with its own art (AGPL-3.0 / CC-BY-SA-4.0).
- The [zenhax thread](https://www.zenhax.com/viewtopic.php@t=9339.html) on the `.luc` format.

**Plan.** This is one engine port for both volumes.
1. **PopLua first, as a standalone headless VM.**
   - Start from stock Lua 5.0.2 and apply the diffs read from the Ghidra decompile: undump, `luaV_execute` for the 49 opcodes, the int32 subtype and the extended stdlib. The 32-bit `size_t` chunk loader needs a patch for 64-bit native builds.
   - Then add the C-like lexer and parser, which is needed for source mods and because the game compiles `.lua` files itself.
   - Regression test: load and run all 621 BA1 and 462 BA2 `.luc` files, and compare against Volume 2's shipped `.lua`/`.luc` pairs.
2. Optionally, grow the scratch tools (`luc50.py`, `poplua.py`, `disasm.py`) into a PopLua decompiler. The debug info is intact, so the creature and battle logic would become editable source.
3. **Rewrite the binding layer on our framework.**
   - The parts to rewrite: the LuaLinkedClass/CreateLinkedObject protocol (`ClassLink.lua`), the MethodWrapper signatures (233 distinct bound methods in Vol. 2), the Lua widget templates, and LuaImage, LuaFont and LuaResourceManager.
   - Drop DProps and the ProcComm debugger.
   - The 608 method names the scripts call are the API checklist.
4. Port the JeffLib subset (StateMachine, Animator, SoundPlayer, SexyBassMusicInterface with MO3 order positions, DataDrivenClass) and XWidget::XLayout.
5. Write a PopAnim player for v1-3, using Volume 2's loader since it is the superset, plus PopAnimModParser.
6. **Translate the C++ game classes.**
   - GameApp, Board, BattleEngine, TileEngine, Tile, BookManager, StatsBook, TreasureScreen, ConversationPanel, then the menus and dialogs. Drop DRM, upsell, MoreGames, UpdateCheck and the developer dialogs.
   - Recommended: Volume 1 first, then Volume 2 as a delta behind build flags or a per-game module. The reverse order works too, because Volume 2's classes are close to a superset (see 3.3).
7. **Assets.** Convert JP2 to PNG while staging. Play the SWF cutscenes through Ruffle on the web or as pre-rendered video natively. Play MO3 through libopenmpt, and load resources synchronously on WASM.
8. **Mods.** The `.luc` creature, book, treasure and word-list scripts, the dictionary, and `properties/*.xml` are the natural mod surface. Gameplay is already script, so mods can ship PopLua source once step 1 includes the parser.

**Obstacles.**
- Four private libraries plus the PopLua VM. Every script depends on bit-exact VM semantics.
- About 590 KB of game C++.
- PopAnim.
- Flash cutscenes.
- JPEG 2000 images.
- About 176 MB of decoded images in total, which matters for the web and for the 3DS roadmap item.

### 3.3 Bookworm Adventures Volume 2

**Verdict: EXTRA WORK.** Alone it is about 15x Bejeweled (range 12-20x). Done after Volume 1 on the shared engine, it costs roughly 0.3-0.4 of the first volume, about 4-6x, mostly the changed and new game classes. No public decompilation exists. The plan is the shared plan in 3.2; this section gives the facts specific to Volume 2.

**Install and image.**
- The install is `A:\SteamLibrary\steamapps\common\Bookworm Adventures Volume 2`.
- `BookwormAdventuresVol2.exe` (4,146,504 bytes, v1.0.4.8465) is the game itself. There is no launcher, wrapper, packer or SteamStub, and there are no Steam imports. The PopCap DRM IPC strings in the exe are inactive.
- Other files in the folder:
  - `bass.dll`;
  - `j2k-codec.dll` (a commercial JPEG 2000 decoder);
  - `Flash.ocx`;
  - `GDF.dll` (Vista Game Explorer, not needed);
  - the uninstaller.

**Build and framework.**
- Linked 2009-07-22 with VS2005. Its Rich header has the same product mix as Bejeweled's game exe and Volume 1. As in Bejeweled, LTCG-style register passing (`unaff_` registers) shows up in about 28% of game functions.
- The framework is a 2008-09 internal build of the SAF 1.3 generation. It adds:
  - SWTri, a 327 KB generated software triangle rasterizer that the port does not need;
  - FlashWidget, EncodingParser and WinInetHTTPTransfer;
  - PopAnim, handling PAM v1-3.
- It uses the same internal libraries as Volume 1: PopLua, LuaLib, LuaApp, JeffLib and XWidget.

**RTTI.** There are 178 type descriptors: 122 `Sexy::`, 10 `JeffLib::` and 7 `LuaLib::`. The 358 `MethodWrapper<>` strings encode **233 distinct methods bound to Lua**, for example GameApp 31, LuaApp 23, Board 14, Tile 10 and Graphics 9. Ghidra class recovery found 8,871 virtual functions and 1,572 vtables, and the whole analysis took about 10 minutes.

**Code.**
- 16,596 functions; `.text` is 3.03 MB.
- Regions:

  | Region | Address range | Size |
  |---|---|---|
  | Game C++ | 0x401000-0x490080 | ~1,994 functions, 557 KB (real logic is perhaps 6-7x Bejeweled) |
  | `Res.cpp` | 0x490080-0x49fef0 | 61 KB (regenerable) |
  | XWidget + JeffLib | around 0x4aaf80-0x4c0000 | 117 KB |
  | PopAnim | 0x510c00-0x51be00 | 43 KB |
  | Lua binding glue | 0x5ffe00-0x659000 | 336 KB |
  | PopLua core | parser around 0x66c000-0x672000; `luaV_execute` at `FUN_0067b640`, a 7.9 KB 48-case switch | 146 KB |
  | CRT and STL | | 220 KB |

- Scripts: 462 `.luc` files (3,619 functions, about 203K instructions; `BattleEngine.luc` alone has 17K) plus 8 plain `.lua` files in `scripts/common`.

**How much it shares with Volume 1:**
- **Classes:** almost the same list.
  - Volume 2 drops CursorWidget, TextWidget, MoreGamesAd/Button, FModSoundInstance and two JeffLib developer dialogs.
  - It adds BookSelectScreen, EncodingParser, SecretListWidget and WinInetHTTPTransfer.
  - The two bind 228 and 233 Lua methods.
- **Bytecode:** the format and the 49-opcode set are the same, and one parser reads both games' scripts.
- **Content:** Volume 2's books are numbered 4-6, continuing Volume 1's 1-3.
- **Function match.** Masked instruction streams of Volume 2's game C++ compared against Volume 1:

  | | Identical | Near-identical | Same entry, changed | New |
  |---|---|---|---|---|
  | vs. Volume 1 | 28% | 19% | 33% | 19% |
  | vs. Bejeweled (baseline) | 7% | 1% | 25% | 66% |

  The engine layers are 51-73% identical or near-identical. Most of the PopLua core differences are in the parser and debugger.

**Data formats.**
- **`main.pak`:** 60 MB, 9,221 entries, standard XOR 0xF7.
- **Images:** 3,969 GIF, 3,940 JPG, 25 PNG and 34 JP2. OpenJPEG decodes all 34.
- **Animations:** 304 `.pam` files (v1/2/3), with mod `.txt` files.
- **Scripts:** 454 `.luc` files in the pak.
- **Other pak content:**
  - 145 `.lore` files;
  - 295 OGG;
  - ImageFont descriptors;
  - `properties/*.xml`: `resources.xml` with 550 resources, `Book0/4/5/6.xml`, treasures, tiles, TimedMode and config;
  - `data/*.xml`: layouts, hall of fame, credits.
- **Dictionary:** `data/compressed.txt` has 191,743 words in the same prefix format. New in Volume 2 is **`data/compressed_attribs.dat`**: one uint32 bitmask per word, behind IsNoun/IsVerb/IsAdjective.
- **Outside the pak:**
  - Music is **10 OGG tracks**, so this volume needs no MO3.
  - 5 SWF cutscenes plus `images/upsell/trailer.swf`, all SWF v7/v8 with AS2, only 2 DoAction tags and no sound, 170-500 frames at 12-15 fps. The trailer is upsell content and can be dropped.
  - `scripts/common/*.lua`/`.luc`: LuaApp base classes as PopLua source plus bytecode, which is the validation set for the VM.
  - `properties/partner.xml`.
- **Loading:** the original loads resources on a thread ("Total thread proc resources" in `lua.log`). WASM needs synchronous loading.

**Existing work.** The same tools as Volume 1, plus [Vazquez241/bookworm-adventures-2-es](https://github.com/Vazquez241/bookworm-adventures-2-es) (a Spanish translation data mod) and [PopGameTool/PopStudio](https://github.com/PopGameTool/PopStudio) as a PAM v1-6 format reference.

**Volume 2 work beyond the shared plan:**
- PAM v2/v3.
- The dictionary attribute file.
- A larger content set: 304 animations and roughly twice as many images.
- No MO3 work.

### 3.4 Insaniquarium Deluxe

**Verdict: ALREADY DONE.** Building on the existing decompilation (WinFish) costs about 1-1.5x Bejeweled; decompiling from scratch would be about 8-10x. No public WebAssembly build exists, so ours would be the first.

**Install and image.**
- The install is `A:\SteamLibrary\steamapps\common\Insaniquarium Deluxe`.
<!-- private: - `InsaniquariumDeluxe.exe` and `Insaniquarium.exe` are byte-identical 2007-08-10 DRM wrappers, and the unwrapping tool works on them unchanged. -->
- `InsaniquariumDeluxe.exe` and `Insaniquarium.exe` are byte-identical 2007-08-10 DRM wrappers. The `!popcapdrmprotect!` marker sits at 0x188000, exactly at the outer image end, and `extract_game_exe.py` works unchanged.
<!-- /private -->
- The game is `WinFish.exe` (2,170,880 bytes, FileVersion 1.1.0.0, ProductVersion "1.1 D"). It is already carved to `insaniquarium/bin/WinFishGame.exe`.
- `WinFish_Scr.exe` is the screensaver build of the same code, linked 3 minutes earlier with about 5 KB of different `.text` and no wrapper. `Insaniquarium.scr` is a 274 KB launcher stub. Neither is needed.

**Build and framework.**
- The game PE was linked 2007-06-28 with VS2005, plus a few VS2003 objects.
- It uses SAF 1.3: 33 of the 62 strings unique to pcf 1.3 are present, and none of the 17 unique to 1.34. Built-in pieces are DirectDraw7/D3D7, DSoundManager, BASS + FMOD, ResourceManager and the widget set.
- No internal libraries.
- RTTI is present: 210 classes with vtables, including 79 `Sexy::` game classes such as Fish, FishTypePet, Board, Alien, Coin, Breeder, ProfileMgr, WinFishApp and DataSync.

**Code.**
- The game region is 0x4D4540-0x553230: 1,568 functions, about 497 KB, including about 40 KB of generated resource-binding code.
- WinFish's game source is 150 files, 47,328 lines and about 1,150 member functions. Our Bejeweled `games/bejeweled/src` is 4,853 lines.

**Data formats.** All files are loose; there is no `main.pak`.
- **Resources:** `properties/resources.xml`.
- **Images:** 390 GIF and 35 JPG, with `_` alpha masks.
- **Fonts:** ImageFont descriptors in `data/`.
- **Sounds:** about 50 OGG plus **15 `.au` files** (8-bit µ-law, 8 kHz).
- **Music:** **3 MO3** files, played with order offsets 0, 1, 3, 5, 13, 22, 32, 45 and 54, with FadeIn/FadeOut.
- **Fish songs:** `fishsongs/*.txt` note sequences, played by pitch-shifting sounds. `AdjustPitch` already exists in our framework.
- **Saves:** binary DataSync files `userdata/users.dat`, `user%d.dat`, `highscores.dat` and `scr%d.dat`. The web build needs IDBFS.
- **Timing:** `WinFishApp` sets **mFrameTime = 28 ms (35.7 Hz)** at 0x54DA98, not SAF's 10 ms default.

**Existing work.** This is the most active PopCap decompilation scene. Projects that use WinFish code or port it:

| Project | What it is | Status | License |
|---|---|---|---|
| [Vindirect/WinFish](https://github.com/Vindirect/WinFish) | Functional (non-matching) decompilation of the 1.1 exe on vendored SAF 1.3 | "Fully playable, still has bugs"; names mostly guessed; about 30 fixes in Oct 2026 | No LICENSE file. The README says the game source is "provided as-is", modifications are MIT, and the framework is under the PopCap license. |
| [kyle-sylvestre/WinFish](https://github.com/kyle-sylvestre/WinFish) | Fork on [kyle-sylvestre/PvZ-Portable](https://github.com/kyle-sylvestre/PvZ-Portable)'s SDL2/OpenGL SAF | Android releases 0.1-0.4, about 20 more gameplay fixes; base of the Vita, Mac and co-op projects | as WinFish; the framework is LGPL-3.0 |
| [SaMeiers/insaniquarium-port](https://github.com/SaMeiers/insaniquarium-port) | WinFish on [SaMeiers/PopLib](https://github.com/SaMeiers/PopLib) (SDL3) + libopenmpt | Works on Windows, Android, Linux and PortMaster; documents 11 WinFish logic bugs | AGPL-3.0: read only, do not copy |
| [PopCap-Decompilations/Insaniquarium-Decompilation](https://github.com/PopCap-Decompilations/Insaniquarium-Decompilation) | Heavy rewrite of WinFish with about 230 original-address annotations | 2 commits (2026-10-03); its addresses match our Steam exe; build status unknown | none: read only |
| [wefalltomorrow/Insaniquarium-Enhanced](https://github.com/wefalltomorrow/Insaniquarium-Enhanced) | CI repackaging of the SaMeiers port | only compilation is verified | AGPL-3.0 |
| [ChanseyIsTheBest/insaniquarium_nx](https://github.com/ChanseyIsTheBest/insaniquarium_nx) | Switch port on the PvZ-Portable SAF | released | none stated |
| [KartingSackboy06/insaniquarium-vita](https://github.com/KartingSackboy06/insaniquarium-vita) | Vita so-loader of the Android build | released, 36 fps | none stated |
| [Jake-Vellora/insaniquarium-mac](https://github.com/Jake-Vellora/insaniquarium-mac) | Apple silicon build | working | MIT (own code) |
| [RAlexander777/Insaniquarium_Miyoo_Mini_Flip](https://github.com/RAlexander777/Insaniquarium_Miyoo_Mini_Flip) | Miyoo port | tested on hardware | AGPL-3.0 |
| [Insaniquarium-Co-Op/insaniquarium-coop](https://github.com/Insaniquarium-Co-Op/insaniquarium-coop) | Online co-op patches | working | MIT |
| [undecuple/insaniquarium-remastered](https://github.com/undecuple/insaniquarium-remastered) | Runtime hook mod for the unmodified Steam 1.1 exe | working | MIT |

Two more projects reimplement the game rather than use WinFish's code: [AmeerMH/insaniquarium-web](https://github.com/AmeerMH/insaniquarium-web), a TypeScript reimplementation that documents 75 of the 76 tuning constants with their addresses, and [AScriver/TurbofishDeluxe](https://github.com/AScriver/TurbofishDeluxe) (Rust, WIP). The addresses in PopCap-Decompilations and AmeerMH (0x413AD0, 0x4DB250, 0x4D8C10, 0x4ED471, 0x4E9291, 0x54DA98) all match our carved exe. WinFish targets the same 1.1 build as the Steam copy.

**Plan.**
1. **Vendor WinFish.** Use Vindirect/WinFish at `f919b3c`, or kyle-sylvestre's fork with its extra fixes, as a second game target (for example `games/insaniquarium`). Build it against our `framework/`; its vendored framework is near-stock pcf 1.3, the same API as ours. Whether vendoring is acceptable is a licensing decision for the user (section 5).
2. **Framework additions:**
   - libopenmpt, keeping the order offsets and fades;
   - a µ-law AU decoder;
   - a shim for `D3DInterface::PushTransform`/`PopTransform`, which HelpScreen credits and HighScoreScreen use (or force the software path);
   - a synchronous `WorkerThread` for fish-song loading on WASM;
   - Win32 stubs for about 10 sites: the registry, MessageBox, CreateDirectory, the screensaver installer, BetaSupport, and the update, register and trial code;
   - IDBFS saves;
   - keeping the 28 ms frame time data-driven.
3. **Accuracy pass.** Check Fish, FishTypePet, OtherTypePet, Board, Alien, Coin, Breeder, Food and Missle function by function against our Ghidra export, which has the RTTI classes named.
   - Annotate addresses the way `games/bejeweled/src` does.
   - Cross-check against SaMeiers' bug list, kyle-sylvestre's release notes and the PopCap-Decompilations address notes, reading those projects only.
4. **Mods.** Asset and audio overlays and string patches through the existing VFS and ModManager. Expose tunables (prices, hunger and alien timers, drop rates), starting from AmeerMH's documented constants.

**Obstacles.**
- Licensing.
- WinFish's known inaccuracies.
- CPU pixel effects that call `GetBits`/`BitsChanged` and upload textures every frame: `DrawInvisibleEffect`, the egg shards, and the hole and mask compositing. HD-art mods must keep those images consistent.
- 32-bit assumptions (`sizeof/4`), which are harmless on i686 and wasm32.
- Windows path-separator bugs in the save code.

### 3.5 Talismania Deluxe

**Verdict: HARD, about 20-25x Bejeweled on its own.** If the second-generation Sprout engine already exists (from a Feeding Frenzy 2 port), Talismania drops to about 12-16x and becomes EXTRA WORK. No public decompilation exists.

**Install and image.**
- The install is `A:\SteamLibrary\steamapps\common\Talismania Deluxe`.
<!-- private: - `Talismania.exe` is the 2006-08-22 DRM wrapper. The game PE is already carved. -->
- `Talismania.exe` (4,678,050 bytes) is the 2006-08-22 DRM wrapper. The game PE is at 0x591a2 (4,313,088 bytes) and is already carved.
<!-- /private -->
- The game PE was linked 2006-07-28 with VS2005 RTM and LTCG (`/GL`, 392 LTCG objects). Its internal name is "Atlantis" (`AtlantisGameMgr.cpp`).
- The game talks to the wrapper over registered-message IPC (`PopCapDRM_*`). Saves go to the registry plus a hashed blob. Both need stubbing or replacing.

**Developer and engine.**
- The splash shows **RocketJump Games**, Wes Cumberland's company; he is the credited sole programmer. Design (Del Chafe) and production (Ed Allard) came from ex-Sprout Iggle Pop staff, and PopCap published.
- The engine is **not SexyAppFramework** (there are no `Sexy::` classes). It is **Sprout Games' second-generation engine**:
  - namespaces `engine` (62 classes), `dxdriver` (16) and `simplesound`/`simplesounddx` (12);
  - about 70 global framework classes, such as GenericApplication, ScreenLayout, Theme, ParticleSystem, Splat, SplinePath, SurfaceCombiner, PixelCombiner, StateMgr, SoundMgr and BassMusicPlayer.
- Feeding Frenzy 2 has almost the same engine: 59/62 engine classes, 16/16 dxdriver, 12/12 simplesound and about 70 shared framework classes.
- Rendering is 800x600 DirectDraw 7 through a software DrawContext with BltFx, PixelFx and WaveFx effects. Events use sigslot.
- No public source exists for either Sprout engine generation. The only reconstruction is the first-generation engine inside PizzaFrenzy-Decompilation, which has no license.

**Libraries.** The public ones are sigslot (public domain), zlib 1.1.4, libpng 1.2.5, libjpeg 6b, libogg/libvorbis and libtheora. BASS is a closed DLL. The engine has its own XML parser.

**RTTI.** Full: 841 type descriptors and 836 vtables. Ghidra found 706 constructors, 636 destructors and 13,790 virtual functions, in 765 namespace directories.

**Code.**
- 34,453 functions, 2.97 MB.
- The game part (~0x401000-0x5f6000) has three components:
  - 325 game classes with 4,813 RTTI-attributed methods (384 KB, about 100K decompiled lines);
  - 7,795 unattributed functions (1.18 MB, about two thirds of them template duplicates; about 520 KB after deduplication);
  - 655 checked-STL functions.
- The realistic translation target is **150-200K decompiled lines**, against Bejeweled's 12.7K.
- The engine adds about 100K decompiled lines, including the software blitters.
- Game subsystems:
  - 12 cell types (Normal, Energy, Kraken, Medusa, Minotaur, Pandora, Siren, Spinner, AcidGlob, StarGlob and others);
  - paths and pots, and Gameboard;
  - the GameLevel, BonusLevel and AtlantisChapter state machines;
  - map and building construction;
  - the Midas narrator and the journal;
  - the HUD and UnlockProcessor;
  - about 40 `*ClassLoader` XML data loaders.

**Data formats.**
- **`Talismania.saf`** (11.5 MB) is a **Sprout Archive File**. The survey decoded it and checked it byte for byte:
  - Header: `FFAS` (0x53414646 LE), u16 1, u16 0, u32 TOC offset.
  - TOC: u16 1, u16 0, a 16-byte keyed MD5 of the rest of the TOC, u32 count, then entries `{u32 off, u32 size, keyed-MD5(file)[16], u16 namelen, name\0}`.
  - Keyed MD5 is standard MD5 with the IV shifted by the seed 0x53414646 times {11, 71, 37, 97}. Both the TOC and per-file digests verify, so **archives can be repacked for mods**.
  - Data is uncompressed and unencrypted.
- **Archive contents** (2,529 entries):
  - 1,056 PNG and 935 JPG (914 with `_name.png` masks);
  - 448 XML files with about 180 element types: boards, chapters, levels, cells, scoring, sounds, splats, strings, screens, themes, fonts (XML glyph metrics plus PNG), particle systems and spline paths;
  - 73 OGG;
  - 14 `.theora` sprite animations. Each is a sequence of serialized `ogg_packet` headers (`{i32 bytes, i32 b_o_s, i32 e_o_s, i64 granulepos, i64 packetno}` followed by the payload), and alpha comes from `_alpha.N.png` frames.
- **Loose files:** `music/*.ogg` (6 BASS streams), `splash/` and `properties/partner.xml`. The log string "FOUND Talismania.saf, USING ARCHIVE" suggests the engine also reads loose files, which suits mods.

**Existing work.** None for Talismania. Useful references:
- [PopCap-Decompilations/PizzaFrenzy-Decompilation](https://github.com/PopCap-Decompilations/PizzaFrenzy-Decompilation): the first-generation engine, including SAF/ArchiveToc with keyed MD5, Xml, ScreenLayoutParser, Font, Splat, ParticleSystem, SimpleSoundDX and Blitter.
- [amooose/Popcap_.SAF_Extractor-Packer](https://github.com/amooose/Popcap_.SAF_Extractor-Packer): its packer is unfinished because the checksum was unknown, which the survey has now solved.
- [SnoUweR/pizza-frenzy-saf-extractor](https://github.com/SnoUweR/pizza-frenzy-saf-extractor).
- [YoussefAminGomaa/FF2Theora](https://github.com/YoussefAminGomaa/FF2Theora): the same `.theora` container.

**Plan.** This goes through the Sprout line; see section 4.
1. **Pizza Frenzy pilot.** Build the existing decompilation on Windows against the user's `.saf`. If it plays, port its first-generation engine's platform layer (DDraw display, SimpleSoundDX, Win32 window, thread and registry, WinInet) to SDL2/Emscripten, reusing our `platform/` code but not SAF's API. That gives a working SAF-archive, XML, font, screen-layout, particle and splat stack.
2. **Second-generation engine.** Extend it with the dxdriver split, Scene, Modifier, BltFx, PixelFx, WaveFx and the DataSet loaders, using the RTTI and vtables of both Talismania and Feeding Frenzy 2. Plan the two games as a pair.
3. **Translate Talismania's game classes.** Start from the RTTI-named state machines (GameLevel, AtlantisChapter, the CBaseCell family, Gameboard) and the `*ClassLoader` XML parsers, which must reproduce every attribute exactly.
4. **Assets.**
   - SAF reader (`talismania/scripts/saf.py` is done).
   - Merge JPEG and mask.
   - `.theora` through libtheora (which builds for WASM), or pre-decoded PNG strips at staging.
   - Music through our existing OGG path instead of BASS.
   - 800x600 output.
   - File-based saves instead of the registry and the hash blob.
   - The loading thread (`ILoadingThread`) made synchronous or cooperative for WASM.
5. **Mods.** Loose-file overrides of the XML data cover most gameplay; SAF repacking is possible.

**Obstacles.**
- Step 3 of our method fails: there is no public engine source.
- The size: 325 classes plus about 1.2 MB of unattributed code, much of it template bloat that has to be told apart from real logic.
- LTCG inlining of engine helpers into game functions.
- 448 XML files whose loaders must be reproduced exactly.
- Theora animations.
- The loading thread.
- BASS, the registry with its hash check, and the DRM IPC.

None of these is a blocker: the image is not packed and has full RTTI. It is just large.

### 3.6 Typer Shark! Deluxe

**Verdict: EXTRA WORK, about 5x Bejeweled (range 4-6x). No public decompilation exists.**

**Install and image.**
- The install is `A:\SteamLibrary\steamapps\common\Typer Shark Deluxe`.
<!-- private: - `WinTS.exe` is the 2006-08-22 DRM wrapper, with Flash trial screens and the Steam2 API, and the unwrapping tool works on it unchanged. -->
- `WinTS.exe` (1,860,002 bytes) is the 2006-08-22 DRM wrapper, with Flash trial screens and the Steam2 API. The game PE runs from 0x591a2 to EOF (1,495,040 bytes), and `extract_game_exe.py` carves it unchanged.
<!-- /private -->
- The inner PE has InternalName WinTS, development name "WordShark", and ProductVersion 1.02, which TCRF lists as the last build. It was linked 2004-04-20 with VS.NET 2002 (7.0, build 9466).
- The old shareware code is still inside: RegisterDialog, KeyDialog, the BigInt key check, trial limits, BetaSupport, UpdateCheck and HTTP crash reports. `properties/partner.xml` (NoReg, DontUpdate, SteamId 3450) disables it, and a port drops it.

**Framework.**
- The game uses the same **pre-1.0 April 2004 SAF snapshot** as AstroPop (whose framework RTTI set differs only by MoreGamesAd/MoreGamesButton) and BookWorm (built 5 days earlier).
- It has:
  - a monolithic SexyApp, Widget without WidgetContainer, and no NativeDisplay;
  - DDraw/D3D7, DSoundManager, BASS 1.x plus FMOD;
  - ImageFont, XMLParser, PropertiesParser, DescParser;
  - ResourceManager (unused) and the widget set.
- The framework region is 0x4623f0-0x4c6000 (about 383 KB).
- There are no internal libraries. Game-side widgets (FadeButton, XWidget, TabbedWidget/TabPanel, TooltipWidget, SoundSlider, SexyString, WSTimer and a user-name edit filter) have no public source and are translated with the game.

**RTTI.** There are 196 type descriptors and 273 vftables, covering 62 game classes. Ghidra recovers nothing (see 1.4). `rtti2sym.py` + `ApplySymbols` and a re-export named 1,045 functions and placed 223 vftable labels. For example, `Board::vf20` is `KeyChar(char)`: it upper-cases a-z, and space pauses.

**Code.**
- The game is 0x401000-0x4623f0: **1,219 functions, 379 KB**, bloated by MSVC 7's inline `std::string` code. 494 functions are in class namespaces. The 725 non-virtual helpers (236 KB) are still unnamed, and need names from their debug strings (`UserProfileMgr::WriteDataToDisk` and similar).
- The largest pieces are:
  - UserProfileMgr save, load and registry code (~35 KB);
  - the difficulty and abyss XML config parser (11.7 KB);
  - WinWSApp (19 KB of virtual functions);
  - Board (26 KB of virtual functions: HUD, pause, level complete, KeyChar, Shark Zapper, treasure and bonus rounds);
  - MapScreen (8.4 KB) and TypingTutor (8.4 KB).
- The 62 classes cover:
  - screens: Title, Menu, Map, Board, HighScore, Stats, plus panels;
  - TypingTutor and LessonManager;
  - about 30 entity classes: sharks and their variants, piranhas, Mecha bosses, torpedoes, projectiles, clams, bubbles, BonusText and others;
  - 13 dialogs.

**Data formats.** All files are loose, with no `main.pak` and no `resources.xml`. The asset set totals about 6 MB.
- **Names:** image and sound names are hard-coded in WinWSApp, so they become a `Res.h`-style table.
- **Images:** 518 files (484 GIF, 262 of them alpha masks; 33 JPG; 1 PNG). Some names differ in case, which our case-insensitive VFS already handles.
- **Fonts:** 23 ImageFont descriptors in `data/`, already supported.
- **Sounds:** 34 OGG plus **3 µ-law `.au`** files.
- **Music:** `music/Wordshar.mo3` is MO3 v0 by Purple Motion (2002), with 9 sections chosen by order offset. Use libopenmpt, or pre-render the 9 sections to Ogg and use the port's existing track remap.
- **Levels:**
  - `easy/normal/hard/expert/xtreme-cfg.xml`, 5,083 lines each (Config, Default, Level, Wave and Enemy elements with speeds, counts, word difficulty and boss parameters);
  - `abyss.xml` for endless mode;
  - `Lesson0-18.xml` typing lessons, with non-standard tags such as `<1>` that the lenient XMLParser accepts.
- **Words:**
  - `twords.txt`: 16.7K lines in `#DIFF-3`..`#DIFF-21` sections;
  - `waves.txt`: `#LENGTH-3..7` sections;
  - `themes.txt`: 152 `#THEME` groups;
  - `gemnames.txt` and `shipnames.txt`.
- **Saves:** `users\user%d.dat` (Buffer format), `users\user.idx`, and `HKCU\Software\PopCap\TyperShark`, which the port's registry emulation covers.

**Existing work.** There is no decompilation. TCRF reports that the "Typer Shark Abyss" release, bundled with Typing Instructor Deluxe 17 ([archive.org item](https://archive.org/details/typing-istructor-deluxe-17-typer-shark-abyss), a 411 MB setup), contains a CVS folder with some source files. This is **unverified**: TCRF was behind a Cloudflare challenge. The other projects are fan clones ([tieuchanlong/TyperShark-Remake](https://github.com/tieuchanlong/TyperShark-Remake), [lprichar/typershark](https://github.com/lprichar/typershark), [kh4n9/Typer-Shark-Deluxe](https://github.com/kh4n9/Typer-Shark-Deluxe)). Several GitHub repositories named "...Typer-Shark-Deluxe-Crack" are spam or malware bait and must not be used.

**Plan.**
1. Use the class-named export (`typer-shark/ghidra`). Name the 725 global helpers from their debug strings.
2. Port WinWSApp's image, sound and font tables into a `Res.h`-style table.
3. On the 2004 framework compatibility layer built for BookWorm, translate in this order: the XML config and lesson parsers, the entity hierarchy, Board, MapScreen, then the dialogs.
4. Audio: the AU decoder, plus libopenmpt or pre-rendered sections.
5. **Input.**
   - Desktop is already handled: the port routes `SDL_TEXTINPUT` to `KeyChar`.
   - Mobile web needs one of two things. Either a hidden `<input>` focused on tap that forwards characters, with the canvas laid out above the soft keyboard, or an in-game on-screen keyboard; the tutor's `glo_*` key sprites could be reused.
   - Add a "touch" difficulty as a data mod of the `*-cfg.xml` speeds.
   - The finger-placement tutor makes no sense on touch.
6. Mods: word lists, themes, cfg XML and lessons are plain text, which makes custom word packs, other languages and difficulty curves easy.

**Obstacles.**
- Ghidra's VC7.0 RTTI failure (already worked around).
- About 1,200 game functions.
- Field offsets and slot meanings from the 2004 framework.
- MO3 and AU audio.
- Keyboard-only play.
- The registration, trial, beta, update and crash-report code to stub out.

---

## 4. Recommended order

> **Decision (2026-10-18):** the order below is accepted, except that the ALREADY DONE games
> (Insaniquarium, Zuma Deluxe, Plants vs. Zombies and Pizza Frenzy) are skipped for now and revisited
> after every other game is done. Consequences:
> - BookWorm Deluxe becomes the first new game target, so it carries the multi-game layout (G6) and the
>   AU decoder (G2).
> - The Sprout line starts with Feeding Frenzy 2 + Talismania and uses the Pizza Frenzy decompilation
>   only as an engine reference.
>
> [MOD_ROADMAP.md](MOD_ROADMAP.md) holds the resulting list.
>
> **Decision (2026-10-24), replacing the order below where they differ:** finish the BookWorm series
> (Deluxe, then Adventures and Adventures Vol. 2), then Bejeweled Twist. After those, take games in order
> of least predicted effort. Typer Shark and Talismania move to the end. Any game with an existing
> recompilation or decompilation is skipped (Zuma, Plants vs. Zombies, Insaniquarium, Pizza Frenzy).
> First, one small game from Appendix B is ported by a single agent as a measured experiment: its token
> use is compared with Big Money's and BookWorm's before several games run at once.
>
> **Decision (2026-10-25):** the experiment (Noah's Ark Deluxe) showed an improvement, so up to three games
> run at once. Bookworm Adventures Deluxe starts the BookWorm series, on Opus (CLAUDE.md, "Model selection").
> Volume 2 waits for its engine, and Twist for PopAnim, which Bookworm Adventures builds first, so the other two
> places go to the cheapest games: Seven Seas Deluxe and Atomica Deluxe (Appendix B), on Sonnet, which reuse
> Noah's Ark's 2002 framework.
>
> **Decision (2026-10-25, later), after Appendix C:** Chuzzle Deluxe is skipped (someone else has reportedly
> decompiled it), and so is its Christmas Edition. Alchemy Deluxe, Mummy Maze Deluxe, Banana Bugs, Gyromancer and
> Feeding Frenzy Deluxe are added. Gyromancer comes after Twist, whose battles it uses, and Feeding Frenzy joins the
> Sprout line. Every game PopCap released after EA bought it in
> 2011 is out. Appendix C has the rest.

Bejeweled Deluxe and then Big Money Deluxe are already planned ([MOD_ROADMAP.md](MOD_ROADMAP.md)). Big Money is DIRECT at about 2x. It builds the SAF 1.2 vtable map that Hammer Heads reuses, and its `.it` music already plays with libxmp-lite.

### Groundwork

These are small, shared pieces. Do them alongside or right after Big Money.

| | Task | Unlocks |
|---|---|---|
| G1 | Add **libopenmpt** to the audio layer, with order-position playback and fades. Decide whether it also replaces libxmp-lite for the S3M/IT games. | 17 MO3 games |
| G2 | **µ-law `.au` decoder** in `Mixer::LoadSoundData` | Big Money, Insaniquarium, Typer Shark, BookWorm |
| G3 | `extract_game_exe.py` and `triage_popcap.py`: accept `!popcapcdprotect!` | BookWorm 1.03, Twist retail |
| G4 | Ghidra pipeline fixes for MSVC 7.0 and VC6 binaries, moved into `tools/`: RTTI scan → `ApplySymbols`, the noreturn-analyzer preScript, a `CLASS_RECOVERY=0` option for very large binaries, the JDK class-version fix, a single script directory | all 2003-2005 titles, Bookworm Adventures |
| G5 | **JPEG 2000 → PNG** conversion during asset staging (Pillow/OpenJPEG), or an OpenJPEG decoder | both Bookworm Adventures games, the Peggle family, Bejeweled 3, Twist, Zuma's Revenge |
| G6 | Multi-game layout: game code per target (`games/<game>/`), per-game asset staging and web shell, shared framework, platform and mod code | every later game |

### Order

1. **Insaniquarium Deluxe** (priority; ALREADY DONE upstream; ~1-1.5x).
   - The cheapest priority game.
   - It proves the second-game-target structure (G6) with a codebase that is already known to work, and it exercises G1 and G2 on 3 MO3 files and 15 AU files.
2. **BookWorm Deluxe** (priority; ~3-3.5x).
   - The carve and a fixed, class-named Ghidra export already exist.
   - It builds the **2004 framework compatibility map**.
   - Its word-list format and dictionary code carry straight over to Bookworm Adventures.
3. **Typer Shark! Deluxe** (priority; ~5x).
   - It uses the same April 2004 framework snapshot as BookWorm (5 days apart), so it reuses the map from step 2.
   - New work: keyboard-only input and a touch keyboard.
4. **Bookworm Adventures Deluxe + Volume 2** (priority; ~20x for both).
   - Build the PopLua VM first. It can be tested headless against 1,083 real scripts before any UI exists.
   - The bindings, JeffLib, XWidget and PopAnim follow, then the game classes of Volume 1, then Volume 2 as a delta.
   - This step produces PopAnim, the SWF handling and JeffLib, which later games reuse.
5. **Sprout line, ending at Talismania** (priority; Talismania is HARD).
   1. **Pizza Frenzy** (~1-1.5x). Verify the existing decompilation and port its engine backend to SDL2/WASM.
   2. **Feeding Frenzy 2 + Talismania as a pair** (second-generation engine). Feeding Frenzy 2 is ~10x; Talismania is ~12-16x on top of the engine.
   3. **Iggle Pop** (~6x). Same engine build as Pizza Frenzy, with 72% of engine functions identical. It can also go right after Pizza Frenzy.
6. **Bejeweled 2 Deluxe** (DIRECT; ~6x).
   - The framework is identical to Bejeweled's, so this is the lowest-risk large title. It can be pulled forward at any point if priorities change.
   - PopAnim can be stubbed, or taken from step 4.
   - The repo already ships a Bejeweled 2 audio mod.
7. **Peggle family** (DIRECT). Order: Peggle Deluxe on Thunderball (~5x), Extreme (~1x), Nights (~4-5x), WoW (~1-2x). Needs G5. Thunderball is MIT and targets the same 1.01 Steam build; contributing upstream is an option.
8. **Remaining classic SAF titles.** Each reuses a framework map from earlier steps:

   | Game | Effort | Notes |
   |---|---|---|
   | Rocket Mania | ~3.5x | RTTI present; 2003 map |
   | Hammer Heads | ~6x | 1.2 map from Big Money; Flash how-to-play handled as in step 4; TinyXML |
   | Heavy Weapon | ~5x | no game RTTI; Flash cutscenes |
   | Dynomite | ~4.5x | no game RTTI; the oldest framework layout |
   | AstroPop | ~7-8x | no game RTTI; Typer Shark's framework; the largest classic title |

9. **Newer frameworks.**
   1. **Bejeweled 3** (~10-15x with the C# and Prime references). Build a shared "prime-compat" layer: PopAnim, PIEffect, CurvedVal, PIL/AELib and a GL RenderEffect backend.
   2. **Bejeweled Twist** (~25x). It shares PopAnim, P3D meshes and the shader concepts. Start with the D3D7/no-shader paths.
   3. **Zuma's Revenge** (HARD; ~15-20x with the references). It reuses the prime-compat layer and step 4's JeffLib.
10. **Chuzzle Deluxe** (HARD; ~8-12x). A separate Raptisoft engine with no RTTI. First check whether a Mac build with symbols is available (section 5).

**Side quests**, not on the critical path and doable at any time:
- **Zuma Deluxe** (~0.5-1x). Vendor CircleShootApp's game folder onto our SAF 1.3 port. hcsubser and kyle-sylvestre have already shown the SDL changes. It is a good warm-up between steps 1 and 2, because it uses the same vendored-decomp path on a second codebase.
- **Plants vs. Zombies** (~0.3-0.5x). Fork PvZ-Portable, which already runs in WASM, and add our mod layer. It does not use our framework. LGPL-3.0 obligations apply, and its README does not accept reverse-engineered contributions.

### Dependencies

```
Groundwork (libopenmpt, AU, extractor marker, Ghidra VC7 fixes, JP2 staging, multi-game layout)
├─ Big Money ─────────(SAF 1.2 map)──────────── Hammer Heads
├─ Insaniquarium (WinFish)          Zuma Deluxe (CircleShootApp)          PvZ (PvZ-Portable fork)
├─ BookWorm Deluxe ───(2004 framework map)───── Typer Shark ── AstroPop
│        │                                      (2003/2005 maps: Rocket Mania, Dynomite, Heavy Weapon)
│        └─(word lists)── Bookworm Adventures 1 ⇄ 2 ──(PopAnim, JeffLib, SWF)──┐
├─ Pizza Frenzy ── Iggle Pop                                                   │
│        └─(Sprout gen 2)── Feeding Frenzy 2 ⇄ Talismania                     │
├─ Bejeweled 2                                                                 │
├─ Peggle Deluxe (Thunderball) ── Extreme ── Nights ── WoW                     │
├─ Bejeweled 3 ──(prime-compat: PopAnim, PIEffect, GL effects)── Twist ── Zuma's Revenge
└─ Chuzzle (separate engine)
```

---

## 5. Open questions and decisions for the user

1. **Licensing of the public decompilations we would build on.** The repository is private, and [RESEARCH_SYNTHESIS.md](RESEARCH_SYNTHESIS.md) recommends keeping it so. Please decide which of these may be vendored and which are reference-only:

   | Project | License situation |
   |---|---|
   | WinFish, CircleShootApp | No LICENSE file. The READMEs say modifications are MIT and the decompiled code is "as-is". |
   | PizzaFrenzy-Decompilation, PopCap-Decompilations/Insaniquarium-Decompilation | No license at all. |
   | devcronx Zuma's Revenge C# | No license, and the repository ships game assets. |
   | PvZ-Portable | LGPL-3.0. |
   | Thunderball, Bejeweled LIVE+ C#, Prime | MIT. |
   | SaMeiers port, Insaniquarium-Enhanced, Miyoo port | AGPL. Reference only. |

2. **Insaniquarium base.** Use Vindirect upstream (the source of truth) or kyle-sylvestre's fork (about 20 more fixes, already on an SDL SAF)?
3. **Audio player.** Should libopenmpt (G1) replace libxmp-lite for every game, including Bejeweled's S3M music, so there is one player, or only handle MO3? Switching changes how Bejeweled's music sounds.
4. **Priority of Talismania.** It is HARD on its own. The recommended route goes through Pizza Frenzy and Feeding Frenzy 2. Is that detour acceptable, or should Talismania wait?
5. **Pizza Frenzy decompilation build.** Its project targets Visual Studio 2026 (v145 toolset, x86). Verifying it needs either Visual Studio Build Tools on this machine or a port of the project to our llvm-mingw toolchain. Its quality is unverified: it was a single bulk commit.
6. **Things only the user can provide or run.**
   - **Mac builds** of Chuzzle, BookWorm or Bejeweled 2, if available to you, might carry C++ symbols. CircleShootApp and Thunderball both got names from Mac builds, and for Chuzzle this could turn HARD into EXTRA WORK.
   - **The Steam BookWorm Deluxe 1.13** (2006, delisted in 2016) is not installed and is not needed. It would only add a `main.pak` build.
   - **Peggle Deluxe.** Thunderball's matching target is `%TEMP%\popcapgame1.exe`, the file the wrapper writes at runtime. Its hash differs from our carved image, so the wrapper probably patches the image when it writes it. To reuse Thunderball's reccmp setup, copy that file while the game is running.
   - **Typer Shark Abyss.** Checking TCRF's claim of leftover source means downloading the 411 MB Typing Instructor Deluxe 17 setup from archive.org. That needs your approval.
7. **Bejeweled Twist (Steam).** The Steam copy's inner exe has 3 local `jcc`→`jmp` patches (at 0x402df9, 0x403039 and 0x4a39c7). They skip the video-memory, unsupported-card and properties-signature checks, and the exe file is dated 2026-01-16. The retail copy is the clean image and should be used for analysis. No action is needed.
8. **Typer Shark on touch devices.** Choose between a hidden-input soft keyboard and an in-game on-screen keyboard, and decide whether to add a slower "touch" difficulty.
9. **3DS roadmap item.** It is already recorded in [MOD_ROADMAP.md](MOD_ROADMAP.md) ("Nintendo 3DS port (after Bejeweled works fully with mods)"). This survey did not research the Super Mario 64 3DS port.
   - **SAF-on-3DS precedents found along the way:**
     - [headshot2017/re-plants-vs-zombies](https://github.com/headshot2017/re-plants-vs-zombies) lists a Nintendo 3DS port as "In development, might not have enough memory for Old 3DS, might barely work on New 3DS".
     - [hello-lisai/PvZ-Portable-LuaJIT](https://github.com/hello-lisai/PvZ-Portable-LuaJIT) lists its 3DS target as discontinued.
   - **Fit of the surveyed games:**
     - Bejeweled (640x480, small assets) is the best first target, as planned.
     - Typer Shark would need an on-screen keyboard on the bottom screen, and the words would be hard to read at 400x240.
     - Bookworm Adventures has about 176 MB of decoded images against 64 MB (Old 3DS) or 124-178 MB (New 3DS) of app memory.
     - Talismania is 800x600 with a software blitter.
     - Insaniquarium's per-frame texture uploads are a performance risk, but its 35.7 Hz tick is light on the CPU.
10. **Survey artefacts and cleanup.**
    - The survey's working files live in the session scratchpad, which is temporary (Appendix B). Should the useful scripts be moved into `tools/` and the Ghidra exports into `re/` (git-ignored)? Useful scripts include the RTTI scanners, the SAF reader and keyed-MD5 tool, the PopLua parser and disassembler, and the function-similarity matchers.
    - The stale Ghidra project `E:/retools/projects/survey_bwa` (an abandoned class-recovery run) can be deleted.
    - So can the partial clones in the scratchpad's `survey/insaniquarium/repos/`.
11. **Unverified claims to resolve when each game comes up:**
    - Thunderball's playability.
    - Whether the Pizza Frenzy decompilation builds and plays.
    - The state of the devcronx Zuma's Revenge C#.
    - Whether the BookWorm and Chuzzle Mac binaries keep symbols.
    - TCRF's Typer Shark source claim.
    - Whether BJ2GC (Bejeweled 2 for GameCube) was clean-room or decompiled.
12. **Caution for future research.** The tcrf.net pages for Bookworm Adventures and Bejeweled 3 returned text aimed at AI agents (payment demands, instructions to write files or shut down). It was ignored, and nothing from those pages is used in this report.

---

## Appendix A: the other games

The key facts for the 20 non-priority games, in the order of the table in section 2. Each heading gives the verdict and the effort.

### Dynomite Deluxe: EXTRA WORK, ~4.5x
- **Image:** `Dynomite.exe` is the 2006-08-21 wrapper; the game inside it is 1,277,952 bytes. The game was linked 2003-07-08 with VS2002. Developer: Raptisoft (John Raptis), working title "Fossilized".
- **Framework:** the oldest layout in the batch: Widget 43 slots, Dialog 48, SexyApp 61, no WidgetContainer and no SexyAppBase.
- **RTTI:** framework only. Game classes have none; there are about 42 game vtables without COLs. Source file names come from `default.xml` comments (FossilizedApp.cpp, Museum.cpp, Tutor.cpp...).
- **Game code:** ~262 KB / ~445 functions. The largest function is the ~24 KB tutorial script.
- **Data:** `DEFAULT.EGG` (a 246 KB binary level pack), `Stomped/*.EGG` user puzzles, binary `.HS` high scores, `.it` music (libxmp-lite works), ImageFont, and 441 strings in `default.xml`.
- **Existing:** [iCTzzz/Dynomite-Deluxe-Level-Editor](https://github.com/iCTzzz/Dynomite-Deluxe-Level-Editor), a possible `.EGG` format reference.

### Rocket Mania Deluxe: EXTRA WORK, ~3.5x
- **Image:** the 2006-08-22 wrapper; the game inside it is 1,019,904 bytes. The game was linked 2003-09-08 with MSVC 6.0 (the only VC6 build in the batch). App class `WinTubeTricksApp`. Developer: Nuclide (Belgium).
- **Framework:** pre-public 2003 (Widget 44, Dialog 49, SexyApp 62, no SexyAppBase).
- **RTTI:** 23 game classes.
- **Game code:** ~195 KB / ~560 functions.
- **Data:** loose files. Music is OGG segments A..I per piece, assembled at runtime from a `PLAYLIST.txt` (In/Out timecodes) by the game's `MySound` class; easy to reimplement. Also voice OGGs and `Players/savea-c.dat`.
- **Existing:** clones only ([nemoraca/rocket-mania-clone](https://github.com/nemoraca/rocket-mania-clone), [ikanyu/Modified-Rocket-Mania](https://github.com/ikanyu/Modified-Rocket-Mania)).

### AstroPop Deluxe: EXTRA WORK, ~7-8x
- **Image:** `WinAP.exe` is the 2006-08-21 wrapper; the game inside it is 1,748,992 bytes. Internal name WinGD. The game was linked 2004-04-06 with VS2002.
- **Framework:** the same April 2004 snapshot as Typer Shark (Widget 47, Dialog 52), plus MoreGames ad classes.
- **RTTI:** framework only; there are about 39 game vtables without COLs.
- **Game code:** ~556 KB / ~1,030 functions, of which ~67 KB is generated resource-ID code (691 IDs) and should be regenerated, not translated. The largest classic title: story mode, 4 pilots, SUPA weapons, survival, demo playback.
- **Data:**
  - `resources.xml` (604 images) with non-standard attributes (static, delay, lastanimframe, `!program` paths);
  - `.xcf` bonus-level configs with `.sig` signatures, whose check should be dropped;
  - `music/music.mo3`;
  - `users/*.usr` profiles.
- **Existing:** none.

### Heavy Weapon Deluxe: EXTRA WORK, ~5x
- **Image:** the 2006-08-21 wrapper; the game inside it is 1,519,616 bytes. The game was linked 2005-02-16 with VS2002. Programmer: Josh Langley.
- **Framework:** pre-1.2 early 2005 (Widget 48, Dialog 54, SexyAppBase split out at 80 slots, NativeDisplay, FlashWidget).
- **RTTI:** framework only; there are about 75 game vtables without COLs, many of them 9-slot enemy and projectile classes. Names come from strings and the data XML. Source files appear to be linked alphabetically (the bosses Helicopter, Rainer, Robot, Worm and Wrecker sit at the end).
- **Game code:** ~300 KB / ~690 functions.
- **Data:**
  - `data/levels.xml`, `waves.xml`, `bosses.xml`, `craft.xml`, `survival0-9.xml`, which make behaviour easy to verify;
  - `Music/AtomicTank.mo3` plus 2 OGG;
  - 3 SWF cutscenes;
  - `*.dmo` demo recordings.
- **Existing:** [Chillaxhson/heavy-weapon-deluxe-vita](https://github.com/Chillaxhson/heavy-weapon-deluxe-vita), a reimplementation (not a decompilation) with v1.1.0 claiming all 19 missions. Its in-mission music is silent because it cannot read MO3, and there is no license. Useful as a behaviour reference.

### Big Money Deluxe: DIRECT, ~2x (already planned)
- **Image:** `WinBM.exe` is the 2006-08-21 wrapper; the game inside it is 1,245,184 bytes. The game was linked 2005-11-14 with VS2002 (no LTCG, so easier to read than Bejeweled).
- **The other exe:** `BigMoney.exe` is iWin's AutoWrap 2.12d build of the same link, under an Armadillo-style protector (`glcfg.date` decodes to `gameVendor=IW`). Ignore it.
- **Framework:** the public 1.2 layout (WidgetContainer 15, Widget 54, Dialog 60, ButtonWidget 57), with DescParser and NativeDisplay, and no PakInterface or FlashWidget.
- **RTTI:** 21 game classes (WinBMApp, Board, Piece, MrMoney, Points, GameSelector, dialogs).
- **Game code:** ~130 KB / ~322 functions, with the same skeleton as Bejeweled.
- **Data:** loose files with no `resources.xml`; ImageFont; `music/QualityTime_BigMoney.it` (libxmp-lite works); OGG/WAV/AU sounds; `default.xml` strings; binary `hiscores.dat` and `savegame.dat`.
- **Existing:** none.

### Hammer Heads Deluxe: EXTRA WORK, ~6x
- **Image:** the 2006-08-22 wrapper; the game inside it is 1,703,936 bytes. The game was linked 2006-05-17 with VS2003. Developer: Nuclide, working title "Cubby Catch". The game has a DRM IPC client that must be stubbed.
- **Framework:** 1.2 layout (as Big Money) plus FlashWidget and ListWidget.
- **RTTI:** 120 game classes, including about 25 Gnome* classes, plus a full in-game level editor and property system (Property*, PropertyWidget*, Tool*, WidgetStyle...).
- **Game code:** ~418 KB / ~1,890 functions, of which ~80 KB of editor and property UI can be dropped and ~26 KB is generated resource code.
- **Data:**
  - `resources.xml` (363 IDs);
  - `boards/*.xml`: TinyXML documents of serialized objects (`<object Class="class Sexy::Cubby" ...>`), so the property system and object factory must be reproduced;
  - `boards/*.csv` tables;
  - `gnomedata/*.txt`;
  - `images/howtoplay.swf`.
- **Libraries:** TinyXML (zlib license).
- **Existing:** none.

### Zuma Deluxe: ALREADY DONE, ~0.5-1x
- **Image:** `Zuma.exe` is the 2007-08-10 wrapper; the game inside it is 2,023,424 bytes. The game was linked 2007-04-04 with VS2005. This is a 2007 rebuild, **not** the 2003 MSVC 7.0 1.0.0 exe that CircleShootApp targets, so addresses do not line up. Gameplay and data are the same.
- **Framework:** SAF ~1.3 with PakInterface.
- **RTTI:** full; the game classes match CircleShootApp's. Steam-only additions: AdScreen, EyeBlaster ad classes, MoreGames, LevelLockScreen and others.
- **Game code:** ~360 KB.
- **Data:** loose files: `resources.xml`, `levels/levels.xml` plus per-level curve `.dat` files, 38 OGG, `music/zuma.mo3`. Folder-name case is mixed (`Blackswirley` vs `BlackSwirley-1.dat`), so lookups must be case-insensitive.
- **Existing:**
  - [alula/CircleShootApp](https://github.com/alula/CircleShootApp): "fully playable"; no LICENSE file, modifications MIT.
  - [kyle-sylvestre/CircleShootApp](https://github.com/kyle-sylvestre/CircleShootApp): "Zuma Portable" for Windows, macOS, Linux and Android, at [ksylvestre.itch.io/zuma-portable](https://ksylvestre.itch.io/zuma-portable).
  - [hcsubser/CircleShootApp-linux](https://github.com/hcsubser/CircleShootApp-linux): on an SDL2/OpenGL SAF, MIT.
  - [sashokey/circle-shoot-web](https://github.com/sashokey/circle-shoot-web): a WASM build that fetches HD and third-party assets rather than your install.
  - [9933fa/Zuma-Deluxe-TAS](https://github.com/9933fa/Zuma-Deluxe-TAS): a faithful JavaScript port.
  - [Fnava1/zuma-deluxe-miyoo](https://github.com/Fnava1/zuma-deluxe-miyoo).
  - [GalaxyShad/Zuma-Deluxe-HD](https://github.com/GalaxyShad/Zuma-Deluxe-HD): a C remake, not a decompilation.
- **Plan:** vendor CircleShootApp's game folder, build it against our framework, add the mod layer and case-insensitive lookup, and credit upstream.

### Bejeweled 2 Deluxe: DIRECT, ~6x
- **Image:** `WinBej2.exe` is the 2007-08-10 wrapper; the game inside it is 2,715,648 bytes. The game was linked 2007-04-11 with VS2005 LTCG, the same toolchain as Bejeweled. The app class is again `WinDMApp`.
- **Framework:** identical vtables to Bejeweled in every class measured, so FRAMEWORK_NOTES applies as it is.
- **RTTI:** 40 game classes (Board, MainMenu, GalaxyMap, Hyperspace, StarField, puzzle-mode classes, dialogs, WorkerThread, CustomResourceManager...).
- **Game code:** ~325-343 KB / ~1,450 functions; Board alone is ~100 KB.
- **Libraries:** PopAnim is linked but its constructors are only referenced from inside PopAnim, so stub it.
- **Data:**
  - `resources.xml` with an `sm_path` (low-res) extension;
  - `music.xml` order offsets into one MO3;
  - `config.xml`, `Backdrops.xml`, `galaxy.xml`;
  - puzzle files `puzzles/*/*.bpz` (magic `bf13 3f03`, with named fields) plus `.sol`;
  - `edata/*.txt` vertex and polygon lists for the galaxy and hyperspace effects.
- **Existing:**
  - No decompilation.
  - [PopSilicon](https://github.com/JacksonDam/PopSilicon) runs the Mac binary.
  - [myuu-151/BJ2GC](https://github.com/myuu-151/BJ2GC) recreates Classic mode on GameCube; the method is not stated.
  - [LolHacksRule/PopCapResourceFilesGeneratedCPP](https://github.com/LolHacksRule/PopCapResourceFilesGeneratedCPP) has Bejeweled 2's `Res.cpp`/`Res.h`.
  - [LDinos/Bejeweled-2-Puzzle-Editor](https://github.com/LDinos/Bejeweled-2-Puzzle-Editor) is a `.bpz` reference.
  - [bognarit80/Bej2Widescreen](https://github.com/bognarit80/Bej2Widescreen) is a widescreen mod.

### Peggle Deluxe: DIRECT on Thunderball, ~5x (~12x from scratch)
- **Image:** `Peggle.exe` is the 2007-08-10 wrapper; the game inside it is 2,842,624 bytes (v1.01). The game was linked 2007-07-16 with VS2005.
- **Framework:** SAF ~1.3, with ModVal tunable strings left in.
- **RTTI:** about 70 game classes (ThunderballApp, Board, Ball, Gun, LogicMgr, PhysObj, CharacterMgr, LevelEditor, Replay...).
- **Game code:** ~750 KB.
- **Data:** `main.pak` (11 MB) with 217 `.jp2` images, 62 binary `levels/*.dat` files, ConfigParser `.cfg` files and fonts. Loose: `music/PeggleBeats.mo3` and 2 OGG.
- **Existing:**
  - [teampopwork/Thunderball](https://github.com/teampopwork/Thunderball): a matching decompilation (reccmp, MSVC 8.0 SP1, Mac debug symbols) of the same 1.01 Steam English build. 38.90% implemented (2,558 of 6,576 functions, framework included), 91.28% accuracy, last pushed 2026-10-05. MIT plus the PopCap Framework License. Not stated to be playable.
  - [PeggleCommunity/haggle](https://github.com/PeggleCommunity/haggle): a mod SDK (MIT).
  - Empty or irrelevant: [Peggle-Reversed/Peggle-Deluxe](https://github.com/Peggle-Reversed/Peggle-Deluxe), [eddieburke/peggle-decompilation-rust](https://github.com/eddieburke/peggle-decompilation-rust) (empty), and the XBLA projects [SplaatSites/peggle](https://github.com/SplaatSites/peggle) and [JosueMora918/peggle360-decomp](https://github.com/JosueMora918/peggle360-decomp).
- **Plan:** use Thunderball's layouts and implemented functions, fill the gaps from our own Ghidra export (ideally upstreaming them), then build its game folder against our framework.

### Peggle Extreme: DIRECT, ~1x after Deluxe
- **Image:** `PeggleExtreme.exe` and `testapp.exe` are identical 2007-08-10 wrappers; the game inside it is 2,859,008 bytes. The game was linked 2007-08-22 with VS2005 and no LTCG (PDB path `ThunderballExtreme`).
- **Game code:** exactly the same 70-class set as Deluxe. After masking addresses, 84% of functions (50% of bytes) are identical to Deluxe. It is a light themed fork with Orange Box assets (headcrabs, walkers).
- **Data:** `main.pak` (10 MB) with 132 `.jp2`, 18 levels, `characters.cfg` and `stages.cfg`; `music/PeggleBeats.mo3`.
- **Plan:** build it as a variant of Deluxe, diffing strings, levels and characters against Deluxe.

### Peggle Nights: DIRECT, ~4-5x after Deluxe
- **Image:** `PeggleNights.exe` is the 2008-10-08 wrapper; the game inside it is 3,342,336 bytes. The game was linked 2008-09-26 with VS2005 LTCG.
- **Framework:** SAF ~1.3 plus EncodingParser, and OLE drag-and-drop for level-pack import, which needs a file-picker replacement.
- **Game code:** 81 game classes: the Deluxe set plus CreditsScreen, IntroScreen, InstituteLounge, LevelFileDialog, MyLevelEdit, ProgressScreen and others. ~700-950 KB. Only about 6% of bytes are identical to Deluxe, but that reflects the LTCG rebuild; the class structure and names carry over.
- **Data:** `main.pak` (17.9 MB) with 273 `.jp2` and 70 levels; 16 MO3; `levelpacks/*.pak`.
- **Existing:**
  - [haggle](https://github.com/PeggleCommunity/haggle) has Nights class layouts.
  - [JosephFiter/peggleonlinedecom](https://github.com/JosephFiter/peggleonlinedecom): a TypeScript remake of the duel mode.

### Peggle World of Warcraft Edition: DIRECT, ~1-2x after Nights
- **Image:** `PeggleWoW.exe` is the game itself, with no wrapper (a free promo). It was linked 2009-06-19 with VS2005 LTCG, as a branch of the Nights code (PDB path `ThunderballNights\WoW`). Because it has no wrapper, it is the easiest Nights-engine binary to load into Ghidra.
- **Game code:** 82 classes: the Nights set minus CreditsScreen, DownloadOrPlayDialog, ProgressScreen and UpsellScreen, plus AdDialog and particle emitters. 32% of bytes are identical to Nights.
- **Data:** `main.pak` (11 MB) with 130 `.jp2` and 12 levels; 3 MO3 plus 4 OGG.

### Plants vs. Zombies GOTY: ALREADY DONE, ~0.3-0.5x
- **Image:** the 2008-10-08 wrapper; the game inside it is 3,552,256 bytes (v1.2.0.1096). The game was linked 2010-12-10 with VS2008 and uses `steam_api` for achievements.
- **RTTI:** framework only; the game classes have none.
- **Libraries:** an internal SAF (~2009-10) plus TodLib (Reanimator, TodParticle, TodFoley), both reconstructed in the decompilations.
- **Data:** `main.pak` (45 MB) with 146 `.reanim` files, particle XML, 251 regenerable `.compiled` caches, PNG/JPG, 167 OGG and 2 MO3.
- **Existing:**
  - [wszqkzqk/PvZ-Portable](https://github.com/wszqkzqk/PvZ-Portable): about 100% playable on Linux, Windows, macOS, Android, iOS, Switch and **WASM** ([live demo](https://wszqkzqk.github.io/PvZ-Portable/pvz-portable.html)). The Steam 1.2.0.1096 build is "fully playable" with `scripts/make-compat-overrides.py`. LGPL-3.0-or-later.
  - [Patoke/re-plants-vs-zombies](https://github.com/Patoke/re-plants-vs-zombies) (CC0).
  - [headshot2017/re-plants-vs-zombies](https://github.com/headshot2017/re-plants-vs-zombies) (CC0, SDL; 3DS port in development).
  - [hello-lisai/PvZ-Portable-LuaJIT](https://github.com/hello-lisai/PvZ-Portable-LuaJIT): a mod API, as a design reference.
  - [ruslan831/PlantsVsZombies-decompilation](https://github.com/ruslan831/PlantsVsZombies-decompilation), [bluisblu/pvz](https://github.com/bluisblu/pvz) and [person6423/pvz-vanilla-recon](https://github.com/person6423/pvz-vanilla-recon).
- **Plan:** fork PvZ-Portable and graft our mods-first file override and manifest onto its resource loader. LuaJIT cannot run in WASM, so mods use PUC Lua or our own API.

### Bejeweled Twist: EXTRA WORK, ~25x
- **Image:** two builds.
  - The Steam `BejeweledTwist.exe` is the 2008-10-08 wrapper.
<!-- private:   - The retail `BejeweledTwist.exe` (`C:\Program Files (x86)\PopCap Games\Bejeweled Twist`) uses the CDProtect wrapper, and the unwrapping tool needs G3. -->
  - The retail `BejeweledTwist.exe` (`C:\Program Files (x86)\PopCap Games\Bejeweled Twist`) uses the CDProtect wrapper. The game sits at 0x611a1 and ends at `!popcapcdprotend!`, and the extractor needs G3.
<!-- /private -->
  - Both inner images are v1.0.3.7482, linked 2008-10-17. The Steam one is locally patched (section 5, item 7). Use the retail inner image.
- **Compiler:** VS2005 with partial LTCG. Codename "Zongo".
- **Framework:** an internal mid-2008 generation with D3D7/8/9 interfaces, DXSurface7/8/9, a P3D mesh renderer, 20 embedded ps_2_0/vs_2_0 shader blobs, GraphicsRecorder, WorkerThread, PopAnim, FEAST (only for `compat.cfg`, so droppable) and FlashWidget. It predates the RenderDevice abstraction.
- **RTTI:** 115 game classes, including about 25 menu/Zen visualizer classes (~190-230 KB, optional).
- **Game code:** ~0.93-1.1 MB plus ~0.35-0.5 MB of non-public framework.
- **Data:** `main.pak` (41 MB, 2,364 `.jp2`), 17 loose `.p3d` meshes, per-mode `config_<mode>.xml`, 2 MO3 with an offsets file. It uses ResourceGen3, so no generated `Res` sources exist.
- **Existing:** patches and mods only ([ha1vorsen/BejeweledTwist-CompatFix](https://github.com/ha1vorsen/BejeweledTwist-CompatFix), [the1812/Popcap-Patches](https://github.com/the1812/Popcap-Patches), and the [Bejeweled Twist Extender](https://bejeweled.fandom.com/wiki/Bejeweled_Twist_Extender)).
- **Plan:** do it after Bejeweled 3. Start with the D3D7/no-shader paths, rewrite the shaders in GLSL, and port the visualizers last or drop them.

### Zuma's Revenge!: HARD, ~35x (~15-20x with references)
- **Image:** `ZumasRevenge.exe` is the 2008-10-08 wrapper; the game inside it is 6,651,904 bytes. The game was linked 2009-08-14 with VS2005 LTCG (internal name Zuma2). It imports `steam_api` (achievements) and WinInet.
- **Size:** `.text` is 5.3 MB, the largest of all 26.
- **Framework:** an internal 2009 generation with RenderDevice/RenderDevice3D, D3D8/9, DXSurface, `.fx` effects through a dynamically loaded d3dx9, D3D meshes, PopAnim, PIEffect, CfgCompiler + FEAST, and MoreGamesAd/Flash.
- **Libraries:** JeffLib (13 classes), PIL (21 classes), AELib (`.pax` After Effects compositions). None has official source.
- **Game code:** 63 polymorphic game classes (Board, Ball, Gun, CurveMgr, ten Boss subclasses, TikiTemple, MapScreen, shader effects). ~1.15-1.75 MB, plus 0.6-1 MB of non-public libraries. LTCG blurs the boundary between game and framework, with ~0.9 MB of rasterizer code sitting between game classes.
- **Data:**
  - `main.pak` (106 MB) with 2,633 `.jp2` across three resolution sets, 21 `.pam`, 75 `.ppf` (PIEffect), 10 `.pax`, font caches and one segmented MO3;
  - loose `levels/*/*.dat` files ("CURV" v15 curves, the same family CircleShootApp reads);
  - seven `.fx` effects that need GLSL rewrites.
- **Existing:**
  - [devcronx/zuma-s-revenge-64-bits](https://github.com/devcronx/zuma-s-revenge-64-bits): appears to be the WP7 C# port being retargeted to .NET 8. 44 commits, not verified to build, no license, ships assets.
  - [alula/CircleShootApp issue #4](https://github.com/alula/CircleShootApp/issues/4) confirms no PC decompilation exists.
  - [exoduswalker/revengedit](https://github.com/exoduswalker/revengedit): a difficulty editor (formerly ShamblesSM/revengedit).
- **Plan:** do it last in the newer-framework line. Reuse the Bejeweled 3 prime-compat layer and Bookworm Adventures' JeffLib, take names from the C#, and use CircleShootApp's curve code as a reference.

### Bejeweled 3: EXTRA WORK, ~30x (~10-15x with references)
- **Image:** `Bejeweled3.exe` is the game itself, with no wrapper. It was linked 2011-10-26 with VS2008 SP1 LTCG and imports `steam_api` (achievements, leaderboards) and WinInet.
- **Framework:** the post-2010 "prime" framework:
  - a driver layer (IAppDriver, IGraphicsDriver, IAudioDriver, IFileDriver, IGamepadDriver...);
  - RenderDevice3D, RenderEffect (`.d3dfx`), a Graphics3D camera and meshes;
  - PopAnim, PIEffect, CurvedVal, ResStreamsManager, FEAST, XInput.
- **RTTI:** 151 game classes: Board subclasses for Classic, Speed, Butterfly, Dig, Quest, Zen, Poker and Inferno, quest mini-game boards, about 25 Goal classes, about 20 Badge classes, and BinauralManager. `properties/default.xml` AUTOGEN comments name 45 source files and 171 `Class::Method:line` entries.
- **Game code:** ~1.0-1.13 MB plus ~0.6 MB of framework.
- **Data:**
  - `main.pak` (90 MB) with 4,557 `.jp2`, 64 `.pam`, 118 `.ppf`, 12 `.p3d` + 5 `.p3danim`, 14 `.d3dfx`, quest/secret/speed `.cfg` files and one MO3 suite;
  - loose: `binaural/*.sbg` (SBaGen programs for Zen mode), ambient OGG and affirmation texts.
- **Existing:**
  - [bognarit80/BejeweledLivePlusDecomp-MonoGame](https://github.com/bognarit80/BejeweledLivePlusDecomp-MonoGame) (MIT): a C# decompile of the WP7 "Bejeweled LIVE+", which is essentially Bejeweled 3's code converted to C#. 688 files, with the boards, PIEffect, PopAnim, PIL, AELib, JeffLib and Graphics3D. About 139 of the PC's class names have matching files. Poker, Ice Storm, some quest boards and the Steam/D3D layers are missing.
  - [LolHacksRule/Prime](https://github.com/LolHacksRule/Prime): a C++ reconstruction of the prime framework, abandoned.
  - [ProjectReversio/BejeweledBlitzPC](https://github.com/ProjectReversio/BejeweledBlitzPC): an abandoned skeleton from the same engine family.
  - Also of interest: [Anonymous1212144/PopCapCSDecomp](https://github.com/Anonymous1212144/PopCapCSDecomp) and [ariidesu/Bejeweled2Live](https://github.com/ariidesu/Bejeweled2Live), WP7 C# decompilations of the mobile Bejeweled LIVE.
- **Plan:** build the shared prime-compat layer here, translate the game classes with the C# for names and logic, and verify against a Ghidra export of the PC binary.

### Chuzzle Deluxe: HARD, ~8-12x
- **Image:** `Chuzzle.exe` is the 2006-08-21 wrapper; the game inside it is 1,105,920 bytes. The game was linked 2005-05-10 with VS2002 (PDB path `c:\GameSrc\cpp\Raptisoft\Chuzzle`).
- **RTTI:** effectively none (`/GR-`): only 7 exception type names and about 129 vtables without COLs. Names come from 27 surviving `__FILE__` strings, config keys (`ClassicChuzzle::`, `SpeedChuzzle::`) and dialog names.
- **Engine:** **not SAF**. It is Raptisoft's own engine: App, Graphics (DirectDraw7/Direct3D7 with a software fallback), Sprite, TextureMosaic/TextureSplatter, Sound, Music (BASS/MO3), Font (it reads SAF ImageFont `.txt` descriptors), GadgetManager, ProfileManager and others. Its source is not public. Raptisoft's later "Robot Wants" engine is semi-public ([rwkcommunity/rwkgame](https://github.com/rwkcommunity/rwkgame), [PeerInfinity/kittyengine](https://github.com/PeerInfinity/kittyengine)), but how it relates to this engine is unverified.
- **Code:** game ~300 KB plus engine ~320 KB.
- **Data:** loose: textures with `_Alpha` masks, ImageFont fonts (already supported), 76 OGG, `music/Chuzzle.mo3` + `Jukebox.xml`, `data/gamedata.cfg` key=value tuning, `data/mindbenders.txt` puzzles. The `cached/*.cached` texture caches can be ignored.
- **Existing:**
  - [TC999/Chuzzle-Deluxe](https://github.com/TC999/Chuzzle-Deluxe) (GPL-3.0) is 13 empty `.cpp` placeholders next to an unrelated SAF copy.
  - [PopSilicon](https://github.com/JacksonDam/PopSilicon) runs the Mac 1.0.0 binary, so a Mac build exists. It might carry symbols (unverified).
- **Plan:** write a small Raptisoft-API shim (Sprite, Font, Sound, Music, Gadget) over our SDL backend rather than decompiling the D3D7 internals.

### Pizza Frenzy: ALREADY DONE, ~1-1.5x
- **Image:** the 2006-08-22 wrapper; the game inside it is 1,268,224 bytes (v5.8.30.1, Sprout Games). The game was linked 2006-02-21 with VS2003.
- **Engine:** Sprout first generation. No RTTI: 10 exception type names and about 322 vtables without names.
- **Code:** game ~455 KB, engine ~265 KB.
- **Data:** `PizzaFrenzy.saf` (6.0 MB, FFAS format, 1,081 entries: JPG, PNG, 147 XML, 73 OGG) and loose `splash/`.
- **Existing:** [PopCap-Decompilations/PizzaFrenzy-Decompilation](https://github.com/PopCap-Decompilations/PizzaFrenzy-Decompilation).
  - Much more than "minimal": 137 game files (~900 KB) in `source/PizzaFrenzy`, a 250-file reconstruction of the Sprout engine in `source/engine`, vendored libraries, and about 1,800 address-annotated functions.
  - Five functions and two data addresses were checked against our carved exe, and all matched.
  - It is Win32-only (DirectDraw, DirectSound, WinInet) with a VS2026 v145 x86 project. It was a single bulk push on 2026-10-03, is not stated to be playable, has not been built here, and has **no license**.
  - Also: [SnoUweR/pizza-frenzy-saf-extractor](https://github.com/SnoUweR/pizza-frenzy-saf-extractor).
- **Plan:** build it, then port the engine backend to SDL2/WASM and add per-file `.saf` overrides for mods. This is the entry point to all the Sprout titles.

### Iggle Pop Deluxe: EXTRA WORK, ~6x after Pizza Frenzy
- **Image:** the 2006-08-22 wrapper; the game inside it is 1,359,872 bytes (v6.2.17.1, internal name FuzzleFrenzy). The game was linked 2006-02-21 with VS2003, on the same day as Pizza Frenzy.
- **Engine:** the same Sprout engine build as Pizza Frenzy. After masking relocations, 651 of 900 engine-region functions (72%) are byte-identical, as are 85% of the DirectDraw functions and 87% of the CRT.
- **RTTI:** none: 10 exception type names and about 386 unnamed vtables. Names come from the XML class and aiClass names (ChainCounter, WarpHole, FreezePickup, GuardZoog...).
- **Game code:** ~480 KB / ~1,200 functions.
- **Data:** `IgglePop.saf` (6.9 MB, 1,628 entries: 161 extension-less level XML files, monster definitions, 74 screen layouts, 840 theme files, 96 OGG), fully XML-driven.
- **Existing:** none.
- **Plan:** link the ported Pizza Frenzy engine, auto-label the shared engine functions by masked-hash matching (`funcsim.py`), and translate only the game code.

### Feeding Frenzy 2 Deluxe: EXTRA WORK, ~10x after Pizza Frenzy
- **Image:** `FeedingFrenzyTwo.exe` is the 2007-08-10 wrapper; the game inside it is 1,917,952 bytes. The game was linked 2006-02-24 with VS2005 (PDB name `FeedingFrenzyTwo-saf-instrument`).
- **RTTI:** full: 597 type names and 581 classes, about 430 of them game classes (Fish, AIFish, PlayerFish, dozens of `*AI` and `*State` classes, Stage, GameHud...).
- **Engine:** Sprout second generation, almost identical to Talismania's: Java-style `engine` (Object, Reader, Runnable, Thread, Scene, ScreenLayout, ArchiveLoader/ArchiveToc, ParticleSystem, Splat, QDParser), `dxdriver`, `simplesound`, `shape` (collision) and sigslot. It shares 48 engine class names with the Pizza Frenzy decompilation.
- **Code:** game ~470 KB, engine ~570 KB.
- **Data:**
  - `FF2.saf` (14.6 MB, 1,933 entries): 1,239 PNG, 246 JPG, 363 XML (74 stages with spawn tables, actors, layouts, particle definitions, strings), 54 OGG and 29 `.theora` sprite animations (frame ranges in `animation.xml`);
  - loose: `music/FF2.mo3`.
- **Existing:** none.
- **Plan:** pair it with Talismania (see 3.5).

---

## Appendix B: survey artefacts

All survey work is under the session scratchpad. It is temporary; see question 10 in section 5. The root is `C:/Users/Administrator/AppData/Local/Temp/claude/C--Users-Administrator-Documents-claude-wasm-bejeweled/bb6f3864-facb-48c7-acfa-f782a25ef960/scratchpad/survey/`. The triage input was `.../scratchpad/popcap_triage.json`.

| Folder | Games | Contents |
|---|---|---|
| `bookworm/` | BookWorm Deluxe | `bin/BookwormGame.exe` (carved); `ghidra/` export (6,103 functions, `decomp/Sexy__*`); `vtables_rtti.json`, `class_symbols.jsonl`, `game_region.txt`; `scripts/` (`peinfo.py`, `rich.py`, `vtables.py`, `region.py`, `run_ghidra2.sh`); `ghidra_scripts/DisableNoReturnDiscovery.java`. Ghidra project: `E:/retools/projects/survey_bookworm_deluxe`. |
| `bookworm-adventures/` | Bookworm Adventures Deluxe | `ghidra_norecover/` export (20,379 functions); `pak/` (extracted); `tools/luc50.py`, `tools/disasm.py`; `vtables_quick.json`. Ghidra project: `survey_bwa_norecover`. `survey_bwa` is a stale, abandoned run and can be deleted. |
| `bookworm-adventures-2/` | Bookworm Adventures Vol. 2 (+ BA1 comparison) | `ghidra/` export (16,596 functions); `pak1/`, `pak2/`; `scripts/poplua.py`, `lucdiff.py`, `sweep.py`, `match.py`, `luareg.py`, `rtti.py`. Ghidra project: `survey_ba2`. |
| `insaniquarium/` | Insaniquarium Deluxe | `bin/WinFishGame.exe`; `ghidra/` export (9,194 functions); `r/` (clones of WinFish, the SaMeiers port, Enhanced and PopCap-Decompilations); `wf_tmp/`. `repos/` holds partial clones and can be deleted. Ghidra project: `survey_insaniquarium`. |
| `talismania/` | Talismania (+ FF2, Iggle Pop, Pizza Frenzy comparison exes) | `bin/TalismaniaGame.exe`; `saf/` (extracted); `ghidra/` export (34,453 functions); `scripts/saf.py`, `keyedmd5.py`, `vt.py`, `fnstats.py`. Ghidra project: `survey_talismania`. |
| `typer-shark/` | Typer Shark (+ the AstroPop inner exe) | `bin/TyperSharkGame.exe`; `ghidra/` (class-named export, 7,459 functions); `ghidra_pass1_noclasses/`; `rtti_symbols.jsonl`; `scripts/rtti2sym.py`, `ghidra_reexport.sh`, `vt.py`. Ghidra project: `survey_typershark`. |
| `classic-sexyapp/` | Big Money, Bejeweled 2, Rocket Mania, Hammer Heads, Heavy Weapon, Dynomite, AstroPop | `bin/*_game.exe`; `json/*.json` per-exe reports (plus `Bej1.json` as the baseline); `scripts/analyze.py`, `region.py`, `strx.py`; `bej2.dis`. |
| `newer-games/` | Zuma's Revenge, the Peggle siblings, Twist, Bejeweled 3 | `bin/` (carved images, plus BejDeluxe/PeggleDeluxe for calibration); `out/*.json`, `out/pak_*.txt`; `scripts/sizes.py`, `simil.py`, `srcfiles.py`. |
| `existing-and-other/` | Zuma, Peggle Deluxe, PvZ, Chuzzle, Feeding Frenzy 2, Iggle Pop, Pizza Frenzy | `bin/*_inner.exe`; `scripts/peinfo.py`, `layout.py`, `carry.py`, `funcsim.py` (masked function identity across binaries), `saf_list.py`; `out/chuzzle_layout.txt`. |

Scripts worth promoting into `tools/` when the corresponding game starts:
- `rtti2sym.py` / `vtables.py` and the noreturn preScript (G4);
- `saf.py` + `keyedmd5.py` (Sprout archives);
- `poplua.py` / `luc50.py` / `disasm.py` (PopLua);
- `funcsim.py` and `sweep.py` + `match.py` (cross-binary function matching).

## Appendix B: nine more games from the user's installers (2026-10-24)

The user's own installers, from their Downloads folder. None was run. All nine hold a Microsoft Cabinet:
four are self-extracting cabinets, and five are PopCap's own installer, a signed PE with an 8-byte header
and then the cabinet in its overlay (`!popcapinstallersig!` marks the code-signing certificate after it). The
cabinets were carved and unpacked with 7-Zip into the git-ignored `re/_installers/<installer>/`, then run
through `tools/triage_popcap.py`. No existing decompilation or recompilation was found for any of them
(two searches, 2026-10-24).

| Game | Exe (inner) | Built | Code | Framework | Data | Effort (Bejeweled = 1x) |
|---|---|---|---|---|---|---|
| Noah's Ark Deluxe | WinNoah.exe | 2002-10 | 552 KB | early SAF (48 class strings), no wrapper | GIF/JPG/OGG, one `.au` | **~1-1.5x**, the cheapest |
| Seven Seas Deluxe | WinSS.exe | 2003-01 | 596 KB | early SAF (34), no wrapper | GIF/JPG/PNG/OGG, `.it` music | **~1-1.5x** |
| Atomica Deluxe | Atomica.exe | 2003-03 | 708 KB | early SAF (30), no wrapper | GIF/JPG/OGG, `.it` music | ~1.5x |
| TipTop Deluxe | TipTop.exe | 2003-02 | 438 KB | not SAF (own engine) | two `.dat` archives | ~2-3x (the archive format first) |
| Ning Po Mahjong Deluxe | Ningpo.exe | 2002-09 | 396 KB | not SAF | one `ningpo.pyz` archive | ~2-3x (the archive format first) |
| Pixelus Deluxe | Pixelus.exe (DRM wrapper) | 2004-07 | 1.09 MB | SAF, BookWorm's generation (53) | 283 `.txt` levels, GIF/PNG/OGG | ~3x, with the 2004 compatibility library |
| Word Harmony | WordHarmony.exe (DRM wrapper) | 2006-02 | 517 KB | not SAF (an `engine` namespace) | WAV, two `.db` | ~3-4x |
| Venice Deluxe | Venice.exe (DRM wrapper) | 2007-06 | 1.59 MB | SAF with PakInterface, plus PopLua | `main.pak` (XOR 0xF7), 15 `.luc` scripts | ~5x (PopLua: see the Sprout notes) |
| Bonnie's Bookstore Deluxe | BonniesBookstore.exe (DRM wrapper) | 2006-11 | 349 KB | not SAF; PopLua `.luc` scripts | JPG/PNG/OGG, one `.pak` | ~4-6x |

Notes:
- The three 2002-2003 SAF games predate the Bejeweled Deluxe framework this port is built on only
  slightly, so the existing framework and platform layers should fit with small differences. That makes
  Noah's Ark or Seven Seas the natural first game for the measured experiment.
- "DRM wrapper" means `tools/extract_game_exe.py` carves the inner game image, as for BookWorm.
- The effort figures are first estimates from the triage alone (code size, framework, data formats), not
  from a Ghidra export.

## Appendix C: PopCap PC games missing from this survey (2026-10-25)

The user asked which PopCap games are still missing from the lists above, and ruled out hidden-object games for
ports, since they have almost no replay value. One research pass, mostly web search, compared the lists above with
Wikipedia's list of PopCap games, Backloggd, Steam and search results from MobyGames, PCGamingWiki and Giant Bomb.
It searched GitHub for decompilations or source releases of each title and found none. Those three sites refused
direct requests, and the Wayback Machine's copies of popcap.com's download catalogue could not be read, so the list
may still have gaps. None of these games is among the user's installers or installs found so far, so each needs
the user's own copy first. The effort column is a guess from the year, the studio and the games already surveyed,
not from a triage of the exe.

| Game | PC release | Developer | Kind of game | Likely effort | Notes |
|---|---|---|---|---|---|
| Alchemy Deluxe | 2001 or 2002 | PopCap | tile placement, Ishido-like | small: early SAF, as Noah's Ark | |
| Mummy Maze Deluxe | 2002 | PopCap | maze puzzle | small: early SAF | |
| Feeding Frenzy Deluxe | 2004; PopCap's Deluxe 2005 | Sprout | eat-or-be-eaten arcade | after the Sprout engine | Feeding Frenzy 2 is in section 2 |
| Cosmic Bugs | 2004 or 2005 | Retro64 | Qix-style area capture | unknown engine | PopCap published it from about 2006 |
| Water Bugs | 2005 | Retro64 | the same, a sequel | after Cosmic Bugs | |
| Mahjong Escape: Ancient Japan | 2006 | SpinTop | mahjong solitaire with a story | unknown engine | |
| Mahjong Escape: Ancient China | 2006 | SpinTop | a re-skin of Ancient Japan | small after Ancient Japan | |
| Chuzzle: Christmas Edition | 2008 | Raptisoft | a holiday re-skin of Chuzzle | small after Chuzzle | not certain PopCap sold it |
| The Wizard's Pen | 2008 | PopCap | uncover a picture and guess it | late SAF | sometimes called "reverse hidden object": the user's call |
| Gyromancer | 2009 | PopCap with Square Enix | puzzle RPG on Bejeweled Twist's battles | after Twist | Square Enix published it |
| Banana Bugs | 2010 | PopCap | arcade, a monkey ropes off bugs | late SAF | |
| Sweet Tooth To Go | 2003 | PopCap, for Pogo | candy match-three | small | not certain it was a PopCap download |

Out of scope, unless the user decides otherwise:
- The Hasbro board games of 2012-2013 (Monopoly, Risk, Battleship, The Game of Life). Other studios made them on
  their own engines (Battleship is Unity) under licence, and Wikipedia's PopCap list leaves them out.
- The Plants vs. Zombies Garden Warfare games (2014, 2016, 2019). These are online 3D shooters on EA's engine, not
  casual games.
- Plants vs. Zombies: Replanted (2025). This is a remaster that is still on sale, and the original is already
  covered by its existing decompilation.

Skipped as hidden-object games: the Mystery P.I. series (eight games), the Amazing Adventures series (five), the
Escape games (Rosecliff Island, the Emerald Star, Whisper Valley), Vacation Quest (the Hawaiian Islands,
Australia) and Hidden Identity: Chicago Blackout.

The research left out web-only, Facebook-only and mobile games (Diamond Mine, Bejeweled Blitz, Zuma Blitz and the
like), Bejeweled Live (Windows Phone and Windows 8), Peggle 2 (consoles only) and the compilations. Mystery
Solitaire: Secret Island appears on one list of SpinTop's games, but Mumbo Jumbo seems to have published it.

The user's decisions on this list (2026-10-25):
- **To port:** Alchemy Deluxe, Mummy Maze Deluxe, Banana Bugs, Gyromancer (after Twist) and Feeding Frenzy Deluxe
  (in the Sprout line; its 2004-2005 build may share the first-generation engine with Pizza Frenzy and Iggle Pop,
  not yet checked). Each needs the user's own copy first.
- **Waiting on the user's research:** Cosmic Bugs, Water Bugs and the two Mahjong Escape games.
- **Skipped:** Chuzzle: Christmas Edition, along with Chuzzle Deluxe, which someone else has reportedly decompiled.
  Sweet Tooth To Go is skipped too, though a Bejeweled mod with its look may come later (MOD_ROADMAP.md).
  The Wizard's Pen counts as a hidden-object game, so it is skipped.
- **Out:** every game PopCap released after EA bought it in July 2011, which covers the Hasbro games, the Garden
  Warfare games and Replanted. Bejeweled 3 (December 2010) predates the purchase and stays on the list.
