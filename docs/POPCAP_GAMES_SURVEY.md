# PopCap games survey: the facts about each game, and the plan for those still to port

This document holds the facts about each PopCap game on the user's machines, with a few that PopCap published or that
run on its engines, and the plan for the games still to port. The rules it follows (which games, in what order, how many
sessions at once, which model) live in [COORDINATION.md](COORDINATION.md), "Which games, in what order"; this document
gives the facts behind them. Bejeweled Deluxe, the first game ported, is the baseline every effort is measured against.

The survey ran from 2026-10-18 to 2026-10-26. Nine passes on the first day covered the 26 games then installed: six ran
the full Ghidra pipeline (BookWorm Deluxe, Bookworm Adventures Deluxe and Volume 2, Insaniquarium Deluxe, Talismania
Deluxe and Typer Shark! Deluxe), and three did static analysis of the other 20 without Ghidra (PE and Rich headers,
imports, RTTI, vtables, strings, link-order regions and pak listings), so their code sizes are address-range estimates.
Later additions went through `tools/triage_popcap.py` only: the user's installers (2026-10-24), the PopCap games still
missing and the user's further copies (2026-10-25), and other projects and communities (2026-10-26). Facts added after
that are dated where they appear. Nothing in any install folder was modified. On 2026-11-04 the document was cleaned up:
what it said about games already ported or not decompiled shrank to their row in section 4, and the superseded plans
and decisions went.

Contents:
1. [Where things stand](#1-where-things-stand)
2. [The plan for the remaining games](#2-the-plan-for-the-remaining-games): [the order](#the-order),
   [where a port is published](#where-a-port-is-published),
   [Peggle Extreme and World of Warcraft Edition](#peggle-extreme-and-peggle-world-of-warcraft-edition),
   [after PopCap's games](#after-popcaps-games)
3. [Method and findings](#3-method-and-findings): [the Bejeweled method](#31-the-bejeweled-method),
   [what makes a good candidate](#32-what-makes-a-good-candidate), [verdicts and effort](#33-verdicts-and-effort),
   [findings that apply to many games](#34-findings-that-apply-to-many-games),
   [framework generations](#35-framework-generations-and-where-each-is-ported),
   [shared components](#36-shared-components)
4. [All games at a glance](#4-all-games-at-a-glance)
5. [The remaining games](#5-the-remaining-games): [Venice](#51-venice-deluxe), [Heavy Weapon](#52-heavy-weapon-deluxe),
   [Hammer Heads](#53-hammer-heads-deluxe), [Bejeweled 2](#54-bejeweled-2-deluxe), [AstroPop](#55-astropop-deluxe),
   [Typer Shark](#56-typer-shark-deluxe), [Pizza Frenzy](#57-pizza-frenzy), [Iggle Pop](#58-iggle-pop-deluxe),
   [Feeding Frenzy](#59-feeding-frenzy-deluxe), [Chuzzle](#510-chuzzle-deluxe),
   [Feeding Frenzy 2](#511-feeding-frenzy-2-deluxe), [Peggle Deluxe](#512-peggle-deluxe),
   [Peggle Nights](#513-peggle-nights), [Bejeweled 3](#514-bejeweled-3), [Banana Bugs](#515-banana-bugs),
   [Gyromancer](#516-gyromancer), [Zuma's Revenge](#517-zumas-revenge), [Talismania](#518-talismania-deluxe)
6. [Open questions](#6-open-questions)
- [Appendix A: other studios' games](#appendix-a-other-studios-games)
- [Appendix B: other projects and modding communities](#appendix-b-other-projects-and-modding-communities)
- [Appendix C: where the copies are](#appendix-c-where-the-copies-are)

---

## 1. Where things stand

Twenty-two games are ported, each in its own folder under `games/`: Bejeweled Deluxe, Big Money Deluxe, BookWorm
Deluxe, Noah's Ark Deluxe, Seven Seas Deluxe, Atomica Deluxe, Mummy Maze Deluxe, TipTop Deluxe, Bookworm Adventures
Deluxe, Ning Po Mahjong Deluxe, Pixelus Deluxe, Alchemy Deluxe, Word Harmony, Cosmic Bugs, Water Bugs, Rocket Mania
Deluxe, Dynomite Deluxe, Bookworm Adventures Volume 2, Mahjong Escape: Ancient Japan and Ancient China, Bonnie's
Bookstore Deluxe and Bejeweled Twist. The [progress table in README.md](../README.md#games) holds each one's status and
cost, and each game's own docs, linked from that table and from section 4, hold what its port found. Here a ported game
keeps only its row in section 4, and a fact a remaining game needs from it is given in that game's section.

Eighteen games remain; section 2 gives their order. These are not decompiled. Plants vs. Zombies, Insaniquarium Deluxe
and Zuma Deluxe have large fan communities, with finished decompilations, many ports and active modding, and the user
leaves them to their own work (2026-10-26). Peggle Extreme and Peggle World of Warcraft Edition become settings of the
Peggle Deluxe and Nights ports
([section 2](#peggle-extreme-and-peggle-world-of-warcraft-edition)). Chuzzle: Christmas Edition mostly changes Chuzzle's
art. The Wizard's Pen counts as a hidden-object game, and the user ruled out every hidden-object game, since they have
almost no replay value (2026-10-25): the Mystery P.I. series, the Amazing Adventures series, the Escape games, Vacation
Quest and Hidden Identity: Chicago Blackout among them. Sweet Tooth To Go, a candy match-three PopCap made for Pogo, is
not ported either, though a Bejeweled mod with its look may come instead ([MOD_ROADMAP.md](MOD_ROADMAP.md)). Every game
PopCap released after EA bought it in July 2011 is out, which covers the Hasbro board games of 2012-2013, the Garden
Warfare games and Plants vs. Zombies: Replanted; Bejeweled 3 (December 2010) is older and stays. Web-only, Facebook and
mobile games, Bejeweled Live, Peggle 2 (consoles only) and the compilations were never on the list.

---

## 2. The plan for the remaining games

### The order

Every ported game first finishes its identical-play pass, both phases ([COORDINATION.md](COORDINATION.md), "Which
games, in what order"). Then the remaining games follow the user's rule of 2026-11-01: "anything easier than Chuzzle can
run in parallel, and anything harder has to be done in sequence AFTER all easier games have been finished." Games 1 to 9
below, estimated at 4x to 8x, run side by side; games 10 to 18 run one at a time, in this order. A game that builds on
another game's engine goes after it: Iggle Pop and Feeding Frenzy after Pizza Frenzy, Peggle Nights after Peggle Deluxe,
Zuma's Revenge after Bejeweled 3, Talismania after Feeding Frenzy 2. The model follows COORDINATION.md's "Model
selection": Opus for a game that needs an engine or framework layer no port has yet, Sonnet for a game that can largely
copy another port. Efforts are the estimates without references ([3.3](#33-verdicts-and-effort)).

| # | Game | Model | Published | Effort | Engine, and what it needs first |
|---|---|---|---|---|---|
| 1 | [Venice Deluxe](#51-venice-deluxe) | Sonnet | yes | ~5x | SAF ~1.3 of Bejeweled's year, with PakInterface; no game RTTI |
| 2 | [Heavy Weapon Deluxe](#52-heavy-weapon-deluxe) | Opus | yes | ~5x | a 2005 framework no port has, between BookWorm's and Big Money's; no game RTTI; SWF cutscenes; MO3 |
| 3 | [Hammer Heads Deluxe](#53-hammer-heads-deluxe) | Sonnet | yes | ~6x | Big Money's 1.2 framework; TinyXML object files; an SWF how-to-play |
| 4 | [Bejeweled 2 Deluxe](#54-bejeweled-2-deluxe) | Sonnet | yes | ~6x | Bejeweled's framework as it is; MO3; a Mac build |
| 5 | [AstroPop Deluxe](#55-astropop-deluxe) | Sonnet | yes | ~7-8x | BookWorm's 2004 framework; no game RTTI; MO3 |
| 6 | [Typer Shark! Deluxe](#56-typer-shark-deluxe) | Sonnet | yes | ~5x | BookWorm's 2004 framework; MO3 and `.au` sounds; typing on phones |
| 7 | [Pizza Frenzy](#57-pizza-frenzy) | Opus | yes | ~8x | Sprout's first-generation engine, from Word Harmony's port; no RTTI |
| 8 | [Iggle Pop Deluxe](#58-iggle-pop-deluxe) | Sonnet | yes | ~5-6x after Pizza Frenzy | Pizza Frenzy's engine build (72% of its functions identical); a Mac build with class names |
| 9 | [Feeding Frenzy Deluxe](#59-feeding-frenzy-deluxe) | Sonnet | yes | small to medium after Iggle Pop | the same engine in an older build; a Mac build with class names |
| 10 | [Chuzzle Deluxe](#510-chuzzle-deluxe) | Opus | yes | ~8-12x | Raptisoft's own engine, no RTTI; a Mac build with names |
| 11 | [Feeding Frenzy 2 Deluxe](#511-feeding-frenzy-2-deluxe) | Opus | yes | ~10x | Sprout's second-generation engine, which no port has; `.theora` animations |
| 12 | [Peggle Deluxe](#512-peggle-deluxe) | Sonnet | no | ~12x | Bejeweled's framework generation; JPEG 2000 and MO3; a Mac build |
| 13 | [Peggle Nights](#513-peggle-nights) | Sonnet | no | ~4-5x after Deluxe | Peggle Deluxe's port; a Mac build |
| 14 | [Bejeweled 3](#514-bejeweled-3) | Opus | yes | ~30x | the post-2010 "prime" framework; builds the layer Zuma's Revenge reuses; a Mac build |
| 15 | [Banana Bugs](#515-banana-bugs) | Opus | yes | ~15x | a public-generation SAF with PakInterface, BASS and JPEG 2000; game RTTI; a clear image of the installed copy is in `re/bananabugs/bin/` |
| 16 | [Gyromancer](#516-gyromancer) | Opus | yes | large | Square Enix's own engine; a clear image of the Steam build is in `re/gyromancer/bin/` |
| 17 | [Zuma's Revenge!](#517-zumas-revenge) | Opus | web only | ~35x | the 2009 internal framework; Bejeweled 3's layer and Bookworm Adventures' JeffLib; a Mac build |
| 18 | [Talismania Deluxe](#518-talismania-deluxe) | Sonnet | yes | ~12-16x after Feeding Frenzy 2 | Feeding Frenzy 2's engine |

### Where a port is published

A game with another PC decompilation in progress, or an active modding community, is still built, but published neither
in the GitHub snapshot nor on the website (the user, 2026-10-26; COORDINATION.md, "Which games, and where they are
published"). Decompilations of a phone or console build don't count, and neither do a handful of mods or one person's
tool. The "Published" column applies this rule: "yes" is the GitHub snapshot and the website; "no" is neither, with the
game's folder in `.publishignore`; "web only" is the website, with the folder in `.publishignore`. Zuma's Revenge is the
one "web only" game: its source stays out of the snapshot while the user learns more about a PC decompilation in
progress (2026-10-26). Every ported game is published. Appendix B has the evidence for each game.

### Peggle Extreme and Peggle World of Warcraft Edition

The user took these two out of the order on 2026-10-26. A comparison of their programs and paks with Deluxe's and
Nights' found no gameplay change, only presentation. Extreme adds a headcrab on Bjorn and Striders to the main menu, its
logo to the loading screen, the Heavy popping in after a good shot, four voice lines each for a won and a lost level,
picked at random, and a teleport sound. World of Warcraft Edition is Nights cut down to two masters, with an animated
main menu, a dragon loading screen and its own texts. So the parts their own code draws become settings of the Peggle
Deluxe and Nights ports, unpublished with them. For the original games, theme mods with their sounds, menus and art are
already made, outside the repository.

### After PopCap's games

After the eighteen come other studios' games, at the user's decision of 2026-10-25; each needs the user's own copy.
Octogeddon comes first (2026-11-01), World of Goo is wanted among them, and the three Cartoon Network games on Unity
(2026-10-27) come with them. Glass Tower comes near the end, and the Tasty Planet series after it (2026-11-01). Which of
the other games on PopCap's framework to port is decided then. Appendix A has the facts.

---

## 3. Method and findings

### 3.1 The Bejeweled method

See [RE_NOTES.md](../games/bejeweled/docs/RE_NOTES.md) and the Bejeweled README section
"[How it was reverse engineered](../games/bejeweled/README.md#how-it-was-reverse-engineered)". The four steps are:

1. **Carve.** The Steam exe is PopCap's DRM wrapper with the real game PE inside it. `tools/extract_game_exe.py` copies
   it out. It and `tools/triage_popcap.py` are private tools: they are not published.
2. **Analyse.** Ghidra 12 runs headless, recovers C++ classes from RTTI and exports every decompiled function
   (`tools/ghidra_import.sh`) into the git-ignored `re/<game>/`.
3. **Identify the framework.** Framework functions are matched against the public SexyAppFramework (SAF) trees (1.2,
   1.3, 1.34). For Bejeweled, only about 60 KB of the 1 MB `.text` was game code; the rest was framework, libraries and
   CRT.
4. **Translate.** The game classes are translated function by function onto our portable SAF 1.3 (`framework/`, SDL2 +
   Emscripten), with every function annotated with its original address.

The later ports added two habits. Names carry over from an earlier port's export by byte matching (`tools/funcmatch.py`
pairs functions that are identical once addresses are masked), and each game keeps its names in
`games/<game>/docs/symbols/`. A game not on the SexyAppFramework gets its engine translated apart from the game, on the
same platform layer: Word Harmony's `engine/` folder, and the engine libraries of Ning Po, Cosmic Bugs and the Mahjong
Escapes.

### 3.2 What makes a good candidate

These are listed roughly in order of how much they affect the effort:

1. **The game runs on a SexyAppFramework generation with public source** (1.2 or 1.3). Step 3 works, and the game's
   calls map directly onto our port. Older (2002-2005) builds need a compatibility layer, which now exists up to April
   2004 ([3.5](#35-framework-generations-and-where-each-is-ported)). Non-SAF engines (Sprout, Raptisoft) lose step 3
   entirely.
2. **The game classes have RTTI.** Class names and vtables then come for free. AstroPop, Heavy Weapon, Venice, Chuzzle,
   Pizza Frenzy, Iggle Pop and Feeding Frenzy, among the games left, were built without game RTTI.
3. **There is no private middleware.** The PopLua scripting VM, PopAnim, PIEffect, PIL, AELib, JeffLib and the 2008+
   3D/shader renderers have no public source, so each has to be rebuilt. Several now are
   ([3.6](#36-shared-components)).
4. **The game code is small.** Bejeweled's is about 60 KB and about 250 functions. Most other games are 3-30 times that.
5. **The data is in formats the port already reads**: loose files or `main.pak`, ImageFont `.txt` fonts, OGG, and
   S3M/IT tracker music. MO3 music, JPEG 2000 images, Flash cutscenes and custom archives each add a decoder or a
   conversion step, and the first three are built now.

Getting an analysable image was not a differentiator for the first 26 games: none uses a commercial packer on its game
image ([3.4](#34-findings-that-apply-to-many-games)). Two games found later were the exceptions, Banana Bugs and
Gyromancer's Steam exe, but both now have a clear image (section 5).

### 3.3 Verdicts and effort

| Verdict | Meaning |
|---|---|
| **DIRECT** | Steps 1-4 work as they are. The framework generation has public source, the game classes have RTTI, and the only extra work is decoders shared with other games (MO3, JPEG 2000). |
| **EXTRA WORK** | The method works, but the game also needs one of: a compatibility layer for a pre-public framework, manual class recovery (no game RTTI), a private PopCap library, Flash, or a 3D/shader backend. |
| **HARD** | Step 3 fails (the engine is not SAF and no port has it), or the code is very large. |

Effort is given relative to Bejeweled Deluxe = 1x. Bejeweled's game code is about 60 KB, about 250 functions and about
12.7K decompiled lines. Effort is estimated from game-code size plus the extra subsystems listed for each game, and
every estimate is without references to other projects (CLAUDE.md, "No references"). Treat each as ±50%.
[TOKEN_ECONOMY.md](TOKEN_ECONOMY.md), "Estimates against costs", sets the estimates beside what the first ports cost:
each port pays a fixed cost besides its size, sequels on a shared engine were estimated well, and the penalties for a
game without RTTI and for an archive to decode first were set too high.

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

<!-- /private -->

The `.luc` Lua files in the `drm/` folders belong to the wrappers, not the games. Alchemy's and Venice's scripts are
all there, so neither game needs PopLua.

**Ghidra on VS.NET 2002 (MSVC 7.0) binaries.** The BookWorm and Typer Shark passes found four problems.
`tools/ghidra_import.sh` and `tools/rtti_msvc70.py` now handle them. Of the games still to port, Typer Shark, AstroPop,
Heavy Weapon and Chuzzle are VS.NET 2002 builds.
- Ghidra 12's RTTI analyzer and `RecoverClassesFromRTTIScript` recover **no classes**. MSVC 7.0 writes 24-byte
  BaseClassDescriptors without the `pClassDescriptor` field, and Ghidra rejects them. `tools/rtti_msvc70.py` reads the
  RTTI itself and writes symbols that `ApplySymbols.java` applies (`SYMBOLS=` in `ghidra_import.sh`).
- The "Non-Returning Functions - Discovered" analyzer marks the `_free`/`operator delete` thunks as noreturn. This
  truncated about 2,225 call sites in BookWorm's first export. `MSVC70=1` disables the analyzer with
  `tools/ghidra_scripts/DisableNoReturnDiscovery.java`.
- `ApplySymbols.java` failed under the pipeline's JDK 21 with "class file version 67.0", because a JDK 23-compiled class
  was cached in Ghidra's script bundle. The Typer Shark pass worked around it with renamed copies.
- `analyzeHeadless.bat` breaks `-scriptPath` on `;`, so all scripts are in one directory, `tools/ghidra_scripts/`.

Large binaries have their own problem. On Bookworm Adventures, `RecoverClassesFromRTTIScript` was still running after
50 minutes; `CLASS_RECOVERY=0` skips it, and auto-analysis still names the RTTI classes and vtables.

### 3.5 Framework generations, and where each is ported

From vtable slot counts:

| Game | Game PE build | Widget slots | Dialog slots | Other markers | Where it is ported |
|---|---|---|---|---|---|
| Dynomite | 2003-07 | 43 | 48 | no WidgetContainer; SexyApp (61 slots) with no SexyAppBase | Dynomite: `games/dynomite/docs/FRAMEWORK_2003.md` |
| Rocket Mania | 2003-09 | 44 | 49 | VC6; SexyApp 62 slots | Rocket Mania: `games/rocketmania/docs/FRAMEWORK_2003.md` |
| AstroPop, Typer Shark, BookWorm Deluxe | 2004-04 | 47 (AstroPop) | 52 | no WidgetContainer, monolithic SexyApp. The AstroPop and Typer Shark framework RTTI sets are identical. | BookWorm: [FRAMEWORK_2004.md](FRAMEWORK_2004.md) and `framework/compat2004` (the library `sexycompat2004`) |
| Heavy Weapon | 2005-02 | 48 | 54 | SexyAppBase split out (80 slots); FlashWidget | none yet |
| Big Money, Hammer Heads | 2005-11, 2006-05 | 54 | 60 | WidgetContainer has 15 slots. This is exactly the public 1.2 tree ([sandboxmo-de/SexyAppFramework-v1.2](https://github.com/sandboxmo-de/SexyAppFramework-v1.2)). | Big Money: `games/bigmoney/docs/FRAMEWORK_12.md` |
| Bejeweled, Bejeweled 2 | 2007-04 | 70 | 76 | WidgetContainer 30. Bejeweled 2 is identical to Bejeweled in every measured class. | Bejeweled: `framework/` itself, with [FRAMEWORK_NOTES.md](FRAMEWORK_NOTES.md) |

The first-generation SexyApp of 2002 and early 2003 (Noah's Ark, Mummy Maze, Seven Seas, Atomica) is mapped in
`games/noahsark/docs/FRAMEWORK_2002.md` and the `FRAMEWORK_2003.md` of Seven Seas and Atomica; no remaining game uses
it. Pixelus and Alchemy (2004) are on BookWorm's generation. FRAMEWORK_2004.md's "Reuse for Typer Shark and AstroPop"
gives the steps for the next 2004 game. The other SAF games are of the 1.3 generation: Zuma Deluxe, Insaniquarium, the
Peggle family, Plants vs. Zombies, Venice and both Bookworm Adventures games, whose ports run on `framework/` as it
is. Bejeweled Twist (2008), Zuma's Revenge (2009) and Bejeweled 3 (2011) use later internal generations with no public
source. Twist's is ported: its D3D9 interface became the framework's GPU path (`framework/GPU2D.h`, `GPUInterface.h`,
`GPUDisplay.h`), and its 3D drawing is in `games/bejeweledtwist/src/`. Banana Bugs, once its image was read, turned
out to be on the public generation too (`SexyAppBase`, `DDInterface`, `D3DInterface`, `WidgetManager`), not a 2010 one
near Bejeweled 3's as guessed from its launcher.

Of the engines that are not SAF, Sprout's first generation is ported in Word Harmony (`games/wordharmony/engine/`,
described in its `docs/ENGINE.md`), and Pizza Frenzy, Iggle Pop and Feeding Frenzy start from it. Sprout's second
generation (Feeding Frenzy 2, Talismania), Raptisoft's engine (Chuzzle) and Square Enix's (Gyromancer) have no port.
The other engines ported so far, TipTop's, Ning Po's, Retro64's, Jolly Bear's and New Crayon's, serve no remaining game.

### 3.6 Shared components

Each of these is built once and then serves several games. Most live in the shared code; the game libraries live in the
folder of the game that needed them first, until a second game moves them to shared code.

| Component | Built, and where | Remaining games that need it |
|---|---|---|
| MO3 music with order-position jumps | Built: libopenmpt (`third_party/libopenmpt`) through `platform/openmpt/OpenMPTPlayer.h` (the library `sexyopenmpt`). Only MO3 goes through it; S3M, IT and XM stay on libxmp-lite. | Typer Shark, AstroPop, Heavy Weapon, Bejeweled 2, Chuzzle, Feeding Frenzy 2, Peggle Deluxe and Nights, Bejeweled 3, Zuma's Revenge |
| Sun `.au` sounds | Built: `platform/Audio.cpp` decodes 8-bit µ-law and 8-bit linear `.au`. | Typer Shark (3 files) |
| JPEG 2000 (`.jp2`) | Built: OpenJPEG (`third_party/openjpeg`) through `platform/openjpeg/JP2Decoder.h` (`sexyopenjpeg`), decoding at run time. Its rounding was changed to match the originals' decoder for the 5/3 wavelet; [BYO-POP.md](../third_party/openjpeg/BYO-POP.md) asks the Peggles, Zuma's Revenge and Bejeweled 3 to check their own files first. | Peggle Deluxe (217 files) and Nights (273), Zuma's Revenge (2,633), Bejeweled 3 (4,557), Banana Bugs |
| Flash SWF | Built: a player of our own for the SWF 7 and 8 cutscenes of both Bookworm Adventures games, `games/bookwormadv/swf/` (copied in `games/bookwormadv2/swf/`). | Heavy Weapon (3 cutscenes), Hammer Heads (how-to-play) |
| Pre-1.2 framework layers | Built up to April 2004 ([3.5](#35-framework-generations-and-where-each-is-ported)). | AstroPop and Typer Shark (the 2004 layer); Heavy Weapon (its 2005 layout, not yet mapped) |
| PopAnim (`.pam`) player | Built: v1 in `games/bookwormadv/popanim/`, v1-3 in `games/bookwormadv2/popanim/`, and Volume 2's with Twist's two extra scales in `games/bejeweledtwist/popanim/`. | Bejeweled 3, Zuma's Revenge. Bejeweled 2 links it but appears not to use it. |
| PopLua VM + LuaLib/LuaApp bindings | Built: `games/bookwormadv/poplua/`, `lualib/` and `luaapp/` (Volume 2 has its own copies). | none: Venice's and Alchemy's scripts belong to the wrapper |
| JeffLib | Built, the parts Bookworm Adventures uses: `games/bookwormadv/jefflib/`. | Zuma's Revenge (13 classes) |
| 3D and shaders | Built for Twist: the framework's GPU path (`framework/GPU2D.h`, `GPUInterface.h`, `GPUInterfaceGL.cpp`, `GPUDisplay.h`), and Twist's `.p3d` meshes and its 17 shaders as GLSL in `games/bejeweledtwist/src/` (`TwistD3D`, `TwistShaders.cpp`). | Bejeweled 3 (RenderDevice3D, `.d3dfx` effects), Zuma's Revenge (`.fx` effects) |
| PIEffect, PIL, AELib, CurvedVal | CurvedVal is Twist's (`games/bejeweledtwist/src/CurvedVal.cpp`); the others are not built. FEAST and CfgCompiler only compile `compat.cfg` (GPU rules) and can be dropped. | Bejeweled 3, Zuma's Revenge |
| Sprout engine, `.saf` archives (keyed MD5), `.theora` sprite animations | First generation: Word Harmony's port of an older build, `games/wordharmony/engine/`, kept free of game code so that it can move to shared code; its `ResourceManager` reads `.saf` archives. Second generation and `.theora`: not built (libtheora is not in `third_party/`). | Pizza Frenzy, Iggle Pop, Feeding Frenzy (first generation); Feeding Frenzy 2 and Talismania (second generation, with `.theora`) |
| Prefix-compressed word lists | Built in the BookWorm and Bookworm Adventures ports. | none |
| Resource-ID tables | No shared tool. Ports with a `resources.xml` generate their `Res` tables with a script of their own (`games/bookwormadv/tools/resgen.py`, `games/bejeweledtwist/tools/resgen.py`); ports whose names are hard-coded write the table from the exe. | hard-coded names: Typer Shark; `resources.xml`: AstroPop, Hammer Heads, Bejeweled 2, the Peggles |

---

## 4. All games at a glance

"Build" is the link date of the game PE and its compiler. "Game code" is the game-specific part of `.text` (KB, function
count); for the games triaged later it is the whole code section ("in all"). "Effort" is without references, and for a
ported game it is the estimate made before its port. Rows are grouped by framework family, and by build date within a
group.

| Game | Build | Getting the image | Framework / engine | Notable internal libs | Game code | Verdict | Effort | Status |
|---|---|---|---|---|---|---|---|---|
| Mummy Maze Deluxe | 2002-06 | no wrapper | first-generation SexyApp (30 class strings) | none | 523 KB in all | | ~1-1.5x | [ported](../games/mummymaze/docs/README.md) |
| Noah's Ark Deluxe | 2002-10, VC6 | no wrapper | first-generation SexyApp (48 class strings) | none | 552 KB in all | | ~1-1.5x | [ported](../games/noahsark/docs/README.md) |
| Seven Seas Deluxe | 2003-01, VC6 | no wrapper | first-generation SexyApp (34 class strings) | none | 596 KB in all | | ~1-1.5x | [ported](../games/sevenseas/docs/README.md) |
| Atomica Deluxe | 2003-03, VS2002 | no wrapper | 2003 SAF (30 class strings) | none | 708 KB in all | | ~1.5x | [ported](../games/atomica/docs/README.md) |
| Dynomite Deluxe (Raptisoft) | 2003-07-08, VS2002 | 2006 wrapper; carve | pre-public 2003 SAF | none | ~262 KB / ~445 fn; **no game RTTI** | EXTRA WORK | ~4.5x | [ported](../games/dynomite/docs/README.md) |
| Rocket Mania Deluxe (Nuclide) | 2003-09-08, VC6 | 2006 wrapper; carve | pre-public 2003 SAF | none | ~195 KB / ~560 fn; RTTI | EXTRA WORK | ~3.5x | [ported](../games/rocketmania/docs/README.md) |
| AstroPop Deluxe | 2004-04-06, VS2002 | 2006 wrapper; carve | pre-public Apr 2004 SAF | none | ~556 KB (67 KB generated) / ~1,030 fn; **no game RTTI** | EXTRA WORK | ~7-8x | [to port](#55-astropop-deluxe) |
| BookWorm Deluxe 1.03 | 2004-04-15, VS2002 | CDProtect launcher; carve | pre-public early-2004 SAF | none | ~170-190 KB / ~750 fn; RTTI (custom scan) | EXTRA WORK | ~3-3.5x | [ported](../games/bookworm/docs/README.md) |
| Typer Shark! Deluxe 1.02 | 2004-04-20, VS2002 | 2006 wrapper; carve | pre-public Apr 2004 SAF | none | 379 KB / 1,219 fn; RTTI (custom scan) | EXTRA WORK | ~5x | [to port](#56-typer-shark-deluxe) |
| Pixelus Deluxe (Nuclide) | 2004-07-29 | DRM wrapper; carve | BookWorm's 2004 SAF, plus Nuclide's own layer | none | 1.09 MB in all | | ~3x | [ported](../games/pixelus/docs/README.md) |
| Alchemy Deluxe 1.6 | 2004-08-23 | 2007 DRM wrapper; carve | BookWorm's 2004 SAF | none | 900 KB in all; **no game RTTI** | | ~2-3x | [ported](../games/alchemy/docs/README.md) |
| Heavy Weapon Deluxe | 2005-02-16, VS2002 | 2006 wrapper; carve | pre-1.2 SAF (early 2005) | FlashWidget cutscenes | ~300 KB / ~690 fn; **no game RTTI** | EXTRA WORK | ~5x | [to port](#52-heavy-weapon-deluxe) |
| Big Money Deluxe | 2005-11-14, VS2002 | `WinBM.exe` 2006 wrapper; carve | public SAF 1.2 layout | none | ~130 KB / ~322 fn; RTTI | DIRECT | ~2x | [ported](../games/bigmoney/docs/README.md) |
| Hammer Heads Deluxe (Nuclide) | 2006-05-17, VS2003 | 2006 wrapper; carve; stub the DRM IPC client | SAF 1.2 layout + FlashWidget | TinyXML; Nuclide property system and level editor | ~418 KB (~300 KB gameplay) / ~1,890 fn; RTTI | EXTRA WORK | ~6x | [to port](#53-hammer-heads-deluxe) |
| Zuma Deluxe | 2007-04-04, VS2005 | 2007 wrapper; carve | SAF ~1.3 | none | ~360 KB; RTTI | | | not decompiled |
| Bejeweled 2 Deluxe | 2007-04-11, VS2005 LTCG | 2007 wrapper; carve | SAF ~1.3, same vtables as Bejeweled | PopAnim (linked, appears unused) | ~325-343 KB / ~1,450 fn; RTTI | DIRECT | ~6x | [to port](#54-bejeweled-2-deluxe) |
| Bejeweled Deluxe 1.87 (baseline) | 2007-04-13, VS2005 LTCG | 2007 wrapper; carve | SAF ~1.3 | none | ~60 KB / ~250 fn | | 1x | [ported](../games/bejeweled/README.md) |
| Venice Deluxe | 2007-06-06 | DRM wrapper; carve | SAF ~1.3 with PakInterface | none | 1.59 MB in all; **no game RTTI** | | ~5x | [to port](#51-venice-deluxe) |
| Bookworm Adventures Deluxe | 2007-06-20, VS2005 | no wrapper | SAF ~1.3 + PopAnim | PopLua, LuaLib, LuaApp, JeffLib, XWidget (none public) | C++ ~588 KB / ~2,650 fn, plus 621 `.luc` scripts (~78K lines) | EXTRA WORK | ~10-15x | [ported](../games/bookwormadv/docs/README.md) |
| Insaniquarium Deluxe 1.1 | 2007-06-28, VS2005 | 2007 wrapper; carve | SAF 1.3 | none | ~497 KB / 1,568 fn; RTTI | | ~8-10x | not decompiled |
| Peggle Deluxe 1.01 | 2007-07-16, VS2005 | 2007 wrapper; carve | SAF ~1.3 | none | ~750 KB; RTTI | DIRECT (note 1) | ~12x | [to port](#512-peggle-deluxe) |
| Peggle Extreme | 2007-08-22, VS2005 | 2007 wrapper; carve | SAF ~1.3 | none | ~635-760 KB; 84% of functions identical to Deluxe | DIRECT | ~1x after Deluxe | not decompiled: settings of Deluxe |
| Peggle Nights | 2008-09-26, VS2005 LTCG | 2008 wrapper; carve | SAF ~1.3 | none | ~700-950 KB; RTTI | DIRECT | ~4-5x after Deluxe | [to port](#513-peggle-nights) |
| Bejeweled Twist 1.0.3 | 2008-10-17, VS2005 partial LTCG | 2008 wrapper (Steam) or CDProtect launcher (retail); carve | internal mid-2008 SAF with D3D7/8/9, shaders, P3D meshes | PopAnim, FEAST (droppable) | ~0.93-1.1 MB + ~0.4 MB framework; RTTI | EXTRA WORK | ~25x | [ported](../games/bejeweledtwist/docs/NOTES.md) |
| Peggle World of Warcraft Edition | 2009-06-19, VS2005 LTCG | no wrapper | SAF ~1.3 (Nights branch) | none | ~660-880 KB; RTTI | DIRECT | ~1-2x after Nights | not decompiled: settings of Nights |
| Bookworm Adventures Vol. 2 | 2009-07-22, VS2005 | no wrapper | SAF 1.3 generation + PopAnim v1-3 | as Vol. 1 | C++ ~557 KB / ~1,994 fn, plus 462 `.luc` | EXTRA WORK | ~15x alone; ~4-6x after Vol. 1 | [ported](../games/bookwormadv2/docs/README.md) |
| Zuma's Revenge! | 2009-08-14, VS2005 LTCG | 2008 wrapper; carve; uses `steam_api` | internal 2009 SAF: RenderDevice3D, D3D8/9, `.fx` effects | JeffLib, PIL, AELib, PIEffect, PopAnim, FEAST | ~1.15-1.75 MB + 0.6-1 MB libraries; RTTI | HARD | ~35x | [to port](#517-zumas-revenge) |
| Banana Bugs | 2010-04-09, VS2005 | PopCap's 2010 DRM launcher; a clear image is in `re/bananabugs/bin/` | public-generation SAF (`SexyAppBase`, `DDInterface`, `D3DInterface`, `WidgetManager`) | PakInterface, BASS, JPEG 2000 (`j2k-codec.dll`) | ~1.87 MB `.text`, 6,946 fn in Ghidra with libraries; RTTI (about 85 game classes) | DIRECT | ~15x | [to port](#515-banana-bugs) |
| Plants vs. Zombies GOTY 1.2.0.1096 | 2010-12-10, VS2008 | 2008 wrapper; carve | internal SAF (~2009-10) | TodLib | >1 MB; **no game RTTI** | | | not decompiled |
| Bejeweled 3 | 2011-10-26, VS2008 LTCG | no wrapper; uses `steam_api` | post-2010 "prime" framework (drivers, RenderDevice3D, `.d3dfx`) | PopAnim, PIEffect, CurvedVal, FEAST | ~1.0-1.13 MB + ~0.6 MB framework; RTTI | EXTRA WORK | ~30x | [to port](#514-bejeweled-3) |
| Ning Po Mahjong Deluxe | 2002-09 | no wrapper | its own engine, not SAF | none | 396 KB in all; a `ningpo.pyz` archive | | ~2-3x | [ported](../games/ningpo/docs/README.md) |
| TipTop Deluxe | 2003-02 | no wrapper | its own engine, not SAF | none | 438 KB in all; two `.dat` archives | | ~2-3x | [ported](../games/tiptop/docs/README.md) |
| Chuzzle Deluxe (Raptisoft) | 2005-05-10, VS2002 | 2006 wrapper; carve | Raptisoft engine (not SAF, no public source) | none | ~300 KB game + ~320 KB engine; **no RTTI** | HARD | ~8-12x | [to port](#510-chuzzle-deluxe) |
| Feeding Frenzy Deluxe (Sprout) | 2005-07 | plain exe, no wrapper | Sprout first generation, an older build | none | 640 KB in all; RTTI names three engine classes | | small to medium after Iggle Pop | [to port](#59-feeding-frenzy-deluxe) |
| Pizza Frenzy (Sprout) | 2006-02-21, VS2003 | 2006 wrapper; carve | Sprout engine, first generation | none | ~455 KB game + ~265 KB engine; **no RTTI** | EXTRA WORK | ~8x | [to port](#57-pizza-frenzy) |
| Iggle Pop Deluxe (Sprout) | 2006-02-21, VS2003 | 2006 wrapper; carve | Sprout first generation (72% of engine functions identical to Pizza Frenzy) | none | ~480 KB / ~1,200 fn; **no RTTI** | EXTRA WORK | ~5-6x after Pizza Frenzy | [to port](#58-iggle-pop-deluxe) |
| Word Harmony (Sprout) | 2006-02-22, VS2003 | DRM wrapper; carve | Sprout first generation, an older build | none | 517 KB in all | | ~3-4x | [ported](../games/wordharmony/docs/README.md) |
| Feeding Frenzy 2 Deluxe (Sprout) | 2006-02-24, VS2005 | 2007 wrapper; carve | Sprout second generation | sigslot, libtheora (public) | ~470 KB game + ~570 KB engine; RTTI | EXTRA WORK | ~10x | [to port](#511-feeding-frenzy-2-deluxe) |
| Talismania Deluxe (RocketJump, Sprout engine) | 2006-07-28, VS2005 LTCG | 2006 wrapper; carve | Sprout second generation (almost identical to Feeding Frenzy 2) | sigslot, libtheora (public) | 150-200K decompiled lines + engine; RTTI | HARD | ~20-25x alone; ~12-16x after Feeding Frenzy 2 | [to port](#518-talismania-deluxe) |
| Mahjong Escape: Ancient China (Jolly Bear) | 2006-08 | plain exe, no wrapper | Jolly Bear Games' engine, DirectDraw | BASS, libjpeg | ~1.15 MB in all; no RTTI | | small after Ancient Japan | [ported](../games/mahjongchina/docs/README.md) |
| Mahjong Escape: Ancient Japan (Jolly Bear) | 2006-11 | plain exe, no wrapper | Jolly Bear Games' engine, DirectDraw | BASS, libjpeg | ~1.15 MB in all; no RTTI | | ~4-5x | [ported](../games/mahjongjapan/docs/README.md) |
| Bonnie's Bookstore Deluxe (New Crayon) | 2006-11-10, VS2003 | DRM wrapper; carve | New Crayon's own engine on SDL 1.2 and FMOD 3 | SDL_image | 349 KB in all | | ~4-6x | [ported](../games/bonnies/README.md) |
| Water Bugs (Retro64) | 2007-10 | plain exe, no wrapper | Retro64's own engine, DirectDraw | BASS | ~350 KB in all; no RTTI | | small after Cosmic Bugs | [ported](../games/waterbugs/docs/README.md) |
| Cosmic Bugs (Retro64) | 2007-11 | plain exe, no wrapper | Retro64's own engine, DirectDraw | BASS | ~350 KB in all; no RTTI | | ~2x | [ported](../games/cosmicbugs/docs/README.md) |
| Gyromancer (PopCap with Square Enix) | 2009-09 | Steam's DRM stub encrypts the Steam exe; a clear image is kept | Square Enix's own engine, not SAF | none known | 80 game classes | | large | [to port](#516-gyromancer) |

Note 1: Peggle Deluxe's survey pass rated it EXTRA WORK, citing JPEG 2000. The pass that covered Peggle Extreme, Nights
and WoW rated them DIRECT, and they have the same requirements. This report uses DIRECT for the whole family, with JPEG
2000 as a shared component (built since).

Two corrections from the ports and from 2026-11-04: Bonnie's Bookstore has no PopLua (the first triage listed it), and
neither does Venice, whose `.luc` scripts are the wrapper's. Chuzzle: Christmas Edition, The Wizard's Pen and Sweet
Tooth To Go were never triaged. No game was found to be impossible.

---

## 5. The remaining games

One section for each game, in the order of section 2. Where a Mac build exists it sits in the main checkout's
`re/<game>/mac/` with a demangled list of its function and class names (CLAUDE.md, "Mac versions"; git-ignored, so a
worktree copies the folder in). Its names and class layout guide the port, but the PC exe stays the authority for
behaviour. The image each port starts from, carved where the game is wrapped, is already in the main checkout's
`re/<game>/bin/` for every game (Appendix C).

### 5.1 Venice Deluxe

**Sonnet, published, ~5x.** One of the user's installers (2026-10-24).

- **Image:** `Venice.exe` is a PopCap DRM wrapper; the game inside it (1,978,368 bytes) was linked 2007-06-06. The
  first triage counted 1.59 MB of code.
- **Framework:** SAF of the ~1.3 generation with PakInterface, from the same year as Bejeweled's. Its RTTI names 43
  `Sexy::` classes (SexyAppBase, WidgetContainer, ActiveFontLayer, D3DTester and the like) and the pak and library
  classes, but no game class, so the game's classes are found from their constructors, as Alchemy's were. Whether its
  vtables match Bejeweled's, as Bejeweled 2's do, has not been measured; that is the first check, since `framework/` is
  Bejeweled's.
- **Data:** `main.pak` (XOR 0xF7, `tools/popcap_pak.py` reads it) holds 861 files: 432 PNG, 311 XML, 65 JPG and 49 OGG
  sounds and music. There is no `resources.xml`; `properties/` holds only `partner.xml`.
- **No PopLua (checked 2026-11-04).** The first triage listed PopLua and 15 `.luc` scripts. The scripts are in
  `drm/common/scripts/` and belong to the wrapper, and the game image has no Lua at all. The ~5x estimate assumed PopLua;
  without it, but with no game RTTI, it stands as a first guess.

### 5.2 Heavy Weapon Deluxe

**Opus, published, ~5x, EXTRA WORK.**

- **Image:** the 2006-08-21 wrapper; the game inside it is 1,519,616 bytes. The game was linked 2005-02-16 with VS2002.
  Programmer: Josh Langley.
- **Framework:** pre-1.2 early 2005 (Widget 48, Dialog 54, SexyAppBase split out at 80 slots, NativeDisplay,
  FlashWidget). No port has this generation. It sits between BookWorm's April 2004 snapshot
  ([FRAMEWORK_2004.md](FRAMEWORK_2004.md), `framework/compat2004`) and Big Money's 1.2
  (`games/bigmoney/docs/FRAMEWORK_12.md`), both built with the same compiler, so byte matching against both ports'
  exports (`tools/funcmatch.py`) should name most of it.
- **RTTI:** framework only; there are about 75 game vtables without COLs, many of them 9-slot enemy and projectile
  classes. Names come from strings and the data XML. Source files appear to be linked alphabetically (the bosses
  Helicopter, Rainer, Robot, Worm and Wrecker sit at the end).
- **Game code:** ~300 KB / ~690 functions.
- **Data:** `data/levels.xml`, `waves.xml`, `bosses.xml`, `craft.xml` and `survival0-9.xml`, which make behaviour easy
  to verify; `Music/AtomicTank.mo3` plus 2 OGG (MO3 through `platform/openmpt`); 3 SWF cutscenes, for Bookworm
  Adventures' Flash player (`games/bookwormadv/swf/`); `*.dmo` demo recordings.

### 5.3 Hammer Heads Deluxe

**Sonnet, published, ~6x, EXTRA WORK.**

- **Image:** the 2006-08-22 wrapper; the game inside it is 1,703,936 bytes. The game was linked 2006-05-17 with VS2003.
  Developer: Nuclide, working title "Cubby Catch". The game has a DRM IPC client that must be stubbed.
- **Framework:** the 1.2 layout, as Big Money's, plus FlashWidget and ListWidget. Big Money's port
  (`games/bigmoney/`, with its slot and field reference `docs/FRAMEWORK_12.md`) is the one to copy. Nuclide's earlier
  games, Rocket Mania and Pixelus, are ported too; whether Hammer Heads shares their Nuclide code has not been checked.
- **RTTI:** 120 game classes, including about 25 Gnome* classes, plus a full in-game level editor and property system
  (Property*, PropertyWidget*, Tool*, WidgetStyle...).
- **Game code:** ~418 KB / ~1,890 functions, of which ~80 KB is editor and property UI and ~26 KB generated resource
  code.
- **Data:** `resources.xml` (363 IDs); `boards/*.xml`, TinyXML documents of serialized objects
  (`<object Class="class Sexy::Cubby" ...>`), so the property system and object factory must be reproduced;
  `boards/*.csv` tables; `gnomedata/*.txt`; `images/howtoplay.swf` (Bookworm Adventures' Flash player).
- **Libraries:** TinyXML (zlib licence), the game's own open library, so the port can use it.

### 5.4 Bejeweled 2 Deluxe

**Sonnet, published, ~6x, DIRECT.**

- **Image:** `WinBej2.exe` is the 2007-08-10 wrapper; the game inside it is 2,715,648 bytes. The game was linked
  2007-04-11 with VS2005 LTCG, the same toolchain as Bejeweled. The app class is again `WinDMApp`.
- **Framework:** identical vtables to Bejeweled in every class measured, so `framework/` and
  [FRAMEWORK_NOTES.md](FRAMEWORK_NOTES.md) apply as they are, and Bejeweled's port (`games/bejeweled/`) is the nearest.
- **RTTI:** 40 game classes (Board, MainMenu, GalaxyMap, Hyperspace, StarField, puzzle-mode classes, dialogs,
  WorkerThread, CustomResourceManager...).
- **Game code:** ~325-343 KB / ~1,450 functions; Board alone is ~100 KB.
- **Libraries:** PopAnim is linked, but its constructors are only referenced from inside PopAnim, so it can be stubbed.
- **Data:** `resources.xml` with an `sm_path` (low-res) extension; `music.xml` order offsets into one MO3
  (`platform/openmpt`); `config.xml`, `Backdrops.xml`, `galaxy.xml`; puzzle files `puzzles/*/*.bpz` (magic `bf13 3f03`,
  with named fields) plus `.sol`; `edata/*.txt` vertex and polygon lists for the galaxy and hyperspace effects.
- **Mac build:** `re/bejeweled2/mac/`, Bejeweled 2 1.0.0, with name lists for the game and for its SexyApp framework.

### 5.5 AstroPop Deluxe

**Sonnet, published, ~7-8x, EXTRA WORK.**

- **Image:** `WinAP.exe` is the 2006-08-21 wrapper; the game inside it is 1,748,992 bytes. Internal name WinGD. The
  game was linked 2004-04-06 with VS2002.
- **Framework:** the same April 2004 snapshot as Typer Shark and BookWorm (Widget 47, Dialog 52), plus MoreGames ad
  classes. The port starts from BookWorm's: [FRAMEWORK_2004.md](FRAMEWORK_2004.md), "Reuse for Typer Shark and
  AstroPop", gives the steps, and `framework/compat2004` holds the old behaviours.
- **RTTI:** framework only; there are about 39 game vtables without COLs.
- **Game code:** ~556 KB / ~1,030 functions, of which ~67 KB is generated resource-ID code (691 IDs) that should be
  regenerated, not translated. It is the largest classic title: story mode, 4 pilots, SUPA weapons, survival and demo
  playback.
- **Data:** `resources.xml` (604 images) with non-standard attributes (static, delay, lastanimframe, `!program` paths);
  `.xcf` bonus-level configs with `.sig` signatures; `music/music.mo3` (`platform/openmpt`); `users/*.usr` profiles.

### 5.6 Typer Shark! Deluxe

**Sonnet, published, ~5x (range 4-6x), EXTRA WORK.**

**Install and image.**
- The install is `A:\SteamLibrary\steamapps\common\Typer Shark Deluxe`.
<!-- private: - `WinTS.exe` is the 2006-08-22 DRM wrapper, with Flash trial screens and the Steam2 API, and the unwrapping tool works on it unchanged. -->
- `WinTS.exe` (1,860,002 bytes) is the 2006-08-22 DRM wrapper, with Flash trial screens and the Steam2 API. The game PE runs from 0x591a2 to EOF (1,495,040 bytes), and `extract_game_exe.py` carves it unchanged.
<!-- /private -->
- The inner PE has InternalName WinTS, development name "WordShark", and ProductVersion 1.02. It was linked 2004-04-20
  with VS.NET 2002 (7.0, build 9466).
- The old shareware code is still inside: RegisterDialog, KeyDialog, the BigInt key check, trial limits, BetaSupport,
  UpdateCheck and HTTP crash reports. `properties/partner.xml` (NoReg, DontUpdate, SteamId 3450) disables it.

**Framework.** The game uses the same pre-1.0 April 2004 SAF snapshot as AstroPop (whose framework RTTI set differs only
by MoreGamesAd/MoreGamesButton) and BookWorm (built 5 days earlier): a monolithic SexyApp, Widget without
WidgetContainer, no NativeDisplay; DDraw/D3D7, DSoundManager, BASS 1.x plus FMOD; ImageFont, XMLParser,
PropertiesParser, DescParser; ResourceManager (unused) and the widget set. The framework region is 0x4623f0-0x4c6000
(about 383 KB). Byte matching against BookWorm's port pairs 1,018 of its 1,388 framework functions with Typer Shark's,
802 of them byte-identical ([FRAMEWORK_2004.md](FRAMEWORK_2004.md), "Reuse for Typer Shark and AstroPop"). There are no
internal libraries. Game-side widgets (FadeButton, XWidget, TabbedWidget/TabPanel, TooltipWidget, SoundSlider,
SexyString, WSTimer and a user-name edit filter) have no public source and are translated with the game.

**RTTI.** There are 196 type descriptors and 273 vftables, covering 62 game classes. Ghidra recovers nothing
([3.4](#34-findings-that-apply-to-many-games)); the survey's own scan and a re-export named 1,045 functions and placed
223 vftable labels. For example, `Board::vf20` is `KeyChar(char)`: it upper-cases a-z, and space pauses.

**Code.**
- The game is 0x401000-0x4623f0: **1,219 functions, 379 KB**, bloated by MSVC 7's inline `std::string` code. 494
  functions are in class namespaces. The 725 non-virtual helpers (236 KB) were left unnamed, and get names from their
  debug strings (`UserProfileMgr::WriteDataToDisk` and similar).
- The largest pieces are the UserProfileMgr save, load and registry code (~35 KB), the difficulty and abyss XML config
  parser (11.7 KB), WinWSApp (19 KB of virtual functions), Board (26 KB of virtual functions: HUD, pause, level
  complete, KeyChar, Shark Zapper, treasure and bonus rounds), MapScreen (8.4 KB) and TypingTutor (8.4 KB).
- The 62 classes cover the screens (Title, Menu, Map, Board, HighScore, Stats, plus panels), TypingTutor and
  LessonManager, about 30 entity classes (sharks and their variants, piranhas, Mecha bosses, torpedoes, projectiles,
  clams, bubbles, BonusText and others) and 13 dialogs.

**Data formats.** All files are loose, with no `main.pak` and no `resources.xml`. The asset set totals about 6 MB.
- **Names:** image and sound names are hard-coded in WinWSApp, so they become a `Res.h`-style table.
- **Images:** 518 files (484 GIF, 262 of them alpha masks; 33 JPG; 1 PNG). Some names differ in case, which the
  case-insensitive VFS already handles.
- **Fonts:** 23 ImageFont descriptors in `data/`, already supported.
- **Sounds:** 34 OGG plus **3 µ-law `.au`** files, which `platform/Audio.cpp` decodes.
- **Music:** `music/Wordshar.mo3` is MO3 v0 by Purple Motion (2002), with 9 sections chosen by order offset, played
  through `platform/openmpt`.
- **Levels:** `easy/normal/hard/expert/xtreme-cfg.xml`, 5,083 lines each (Config, Default, Level, Wave and Enemy
  elements with speeds, counts, word difficulty and boss parameters); `abyss.xml` for endless mode; `Lesson0-18.xml`
  typing lessons, with non-standard tags such as `<1>` that the lenient XMLParser accepts.
- **Words:** `twords.txt` (16.7K lines in `#DIFF-3`..`#DIFF-21` sections), `waves.txt` (`#LENGTH-3..7` sections),
  `themes.txt` (152 `#THEME` groups), `gemnames.txt` and `shipnames.txt`.
- **Saves:** `users\user%d.dat` (Buffer format), `users\user.idx`, and `HKCU\Software\PopCap\TyperShark`, which the
  port's registry emulation covers.

**Input.** Typing on a desktop already works: the framework routes `SDL_TEXTINPUT` to `KeyChar`
(`framework/SexyAppBase.cpp`). A phone's browser needs a way to type, which is open (section 6). The finger-placement
tutor makes no sense on a touch screen.

**Obstacles.** About 1,200 game functions; field offsets and slot meanings from the 2004 framework, where each class is
checked against the new binary before its offsets are trusted; keyboard-only play; and the registration, trial, beta,
update and crash-report code to stub out.

**Typer Shark Abyss.** Individual Software's 2006 release in Typing Instructor Deluxe 17 holds no source (section 6).
Its Wise installer carries two older builds of the game. One is the Abyss exe, linked 2006-10-15 with the same VS.NET
2002 compiler, unwrapped and without debug information. The other is PopCap's Typer Shark Deluxe 1.0 trial, build 141
of 2003-06-03, in a bundled PopCap installer. Both are the June 2003 code on the 2003 framework generation (Widget 43
slots, Dialog 48, SexyApp 61, as Dynomite), not 1.02's April 2004 one, so field offsets and slots differ throughout.
Abyss swaps the mode menu for a lesson-type list that loads Individual's word lists, and it drops NewUserDialog. Neither
build has symbols, a PDB path or extra debug strings; both have fewer than 1.02, without MODVAL, demo recording or the
profiler. All of 1.02's data files, sounds and music are byte-identical to the Abyss copies, as are 480 of its 518
images; the rest is rebranded art. The CVS metadata names the module `GameSrc/cpp/WinWS` and dates the data files'
revisions up to 2003-06-03. The two exes and the CVS files are kept in `re/typershark/abyss/` with a README. As a second
compile of most of the game code against another layout, they can confirm function boundaries and show what changed
between 1.0 and 1.02, but the port does not need them. The `Typer Shark.swf` beside the installer is a short preview of
Abyss for Typing Instructor's game menu.

Several GitHub repositories named "...Typer-Shark-Deluxe-Crack" are spam or malware bait and must not be used.

### 5.7 Pizza Frenzy

**Opus, published, ~8x as the first game on Word Harmony's engine port, EXTRA WORK.**

- **Image:** the 2006-08-22 wrapper; the game inside it is 1,268,224 bytes (v5.8.30.1, Sprout Games, PDB name
  `PizzaFrenzy.pdb`). The game was linked 2006-02-21 with VS2003.
- **Engine:** Sprout's first generation. No RTTI: 10 exception type names and about 322 vtables without names.
- **Code:** game ~455 KB, engine ~265 KB.
- **Data:** `PizzaFrenzy.saf` (6.0 MB, FFAS format, 1,081 entries: JPG, PNG, 147 XML, 73 OGG) and loose `splash/`.
- **Starting point:** Word Harmony's port of an older build of the same engine, `games/wordharmony/engine/`, which its
  `docs/ENGINE.md` describes. Of the 198 strings Word Harmony's engine code refers to, 170 appear in Pizza Frenzy and
  Iggle Pop too, but only about a fifth of its bytes are identical to Pizza Frenzy's: the functions were rebuilt and
  changed between the two. Its `ResourceManager` already reads `.saf` archives. It was kept free of game code so that
  it can move to shared code when a second Sprout game is ported, which is this one; moving it is a shared change for
  the coordinator.
- **Class names:** Pizza Frenzy has no Mac build here, but Iggle Pop's and Feeding Frenzy's name this engine's classes
  and their bases ([5.8](#58-iggle-pop-deluxe), [5.9](#59-feeding-frenzy-deluxe)). Iggle Pop's has 43 in the `engine`
  namespace, and nine of them have the same names in Word Harmony's port (`Object`, `ResourceManager`, `Font`, `Image`
  among them). Iggle Pop shares Pizza Frenzy's engine build, so its names serve this port too.

### 5.8 Iggle Pop Deluxe

**Sonnet, published, ~5-6x after Pizza Frenzy, EXTRA WORK.**

- **Image:** the 2006-08-22 wrapper; the game inside it is 1,359,872 bytes (v6.2.17.1, internal name FuzzleFrenzy). The
  game was linked 2006-02-21 with VS2003, on the same day as Pizza Frenzy.
- **Engine:** the same Sprout engine build as Pizza Frenzy. After masking relocations, 651 of 900 engine-region
  functions (72%) are byte-identical, as are 85% of the DirectDraw functions and 87% of the CRT.
- **RTTI:** none: 10 exception type names and about 386 unnamed vtables. Names come from the XML class and aiClass names
  (ChainCounter, WarpHole, FreezePickup, GuardZoog...), and now from the Mac build.
- **Game code:** ~480 KB / ~1,200 functions.
- **Data:** `IgglePop.saf` (6.9 MB, 1,628 entries: 161 extension-less level XML files, monster definitions, 74 screen
  layouts, 840 theme files, 96 OGG), fully XML-driven.
- **Mac build:** `re/igglepop/mac/`, Iggle Pop Deluxe 1.0.0 for the Mac, a universal PowerPC and Intel binary
  (`igglepop-bin`). Its symbol table is stripped to 17 names, none from C++, but GCC left its RTTI in. That names 399
  classes with their base classes: 43 of the engine's (`engine::Object`, `engine::ResourceManager`...), 30 of its SDL
  and OpenGL driver, sound and signal libraries, and the game's own, down to nested state classes such as
  `GameMgr::PlayState`. `IgglePop.symbols.txt` gives each class's typeinfo and vtables with their addresses; the
  `rttinames.py` beside it makes the list. So the Mac build names the classes and their virtual functions' order, but
  not the functions. Its engine draws through SDL and OpenGL where the PC's uses DirectDraw.
- **Plan:** link Pizza Frenzy's engine, name the shared engine functions by byte matching (`tools/funcmatch.py`), take
  class names and bases from the Mac build, and translate only the game code.

<!-- private -->

The user's first Mac download (`IgglePop.dmg`, 2026-10-31) keeps the game in `Contents/Resources/Iggle Pop!.payload`
behind the `MacProtectRun` launcher. The second (`Iggle_Pop_Deluxe_1.0.1_Full.dmg`, 2026-11-04) is a cracked copy whose
`igglepop-bin` is that payload with each 16-byte block reversed and the last, partial block left as it is: byte for
byte the same binary, saved here. Its `Info.plist` was edited to say 1.0.1, so the saved one is PopCap's 1.0.0 from
the first download. The first download also holds another build of `igglepop-bin` (5,083,564 bytes) in a leftover
`.svn` folder, with the same 399 class names. Feeding Frenzy's two downloads are built the same way.

<!-- /private -->

### 5.9 Feeding Frenzy Deluxe

**Sonnet, published, small to medium after Iggle Pop.** Found in the user's Popcap Game Collection (2026-10-25).

- **Image:** a plain exe with no wrapper (992,976 bytes, built 2005-07, 640 KB of code), with a signed
  `properties/partner.xml` beside it.
- **Engine:** Sprout's first generation, in a build older than Pizza Frenzy's. Its RTTI names only three `engine`
  classes (Exception, Interface and Object, as in Pizza Frenzy and Iggle Pop), but source file names
  (`SimpleSoundDX.cpp`, `WaveFileReader.cpp`) and the PDB name are left in.
- **Data:** an 8 MB `FFArchive.saf` plus loose WAV, music and menu files.
- **Mac build:** `re/feedingfrenzy/mac/`, Feeding Frenzy Deluxe 1.0.0 for the Mac, a universal PowerPC and Intel binary
  (`Feeding Frenzy`, saved as `FeedingFrenzy`). It is stripped the same way as Iggle Pop's (19 names, none from C++),
  and its RTTI names 148 classes with their bases: 36 of the engine's, 25 of the driver, sound and signal libraries,
  and the game's fish, bonus bubbles, screens and stages. The screensaver in the same bundle has the same 148. Of these
  names, 87 are also in Iggle Pop's build: most of the engine, and a layer above it that both games use (`FadeContainer`,
  `ParticleSystem`, `MusicSystem`, `ScreenLayout`, `TextItem`...). `FeedingFrenzy.symbols.txt` is made as Iggle Pop's.
  Its copyright runs to 2007, so it may be a later build than the PC exe of 2005, which stays the authority.
- **Plan:** copy Pizza Frenzy's and Iggle Pop's engine, and find where this older build differs, as Word Harmony's did.

### 5.10 Chuzzle Deluxe

**Opus, published, ~8-12x, HARD.** It starts the sequence: every game before it is easier.

- **Image:** `Chuzzle.exe` is the 2006-08-21 wrapper; the game inside it is 1,105,920 bytes. The game was linked
  2005-05-10 with VS2002 (PDB path `c:\GameSrc\cpp\Raptisoft\Chuzzle`).
- **RTTI:** effectively none (`/GR-`): only 7 exception type names and about 129 vtables without COLs. Names come from
  27 surviving `__FILE__` strings, config keys (`ClassicChuzzle::`, `SpeedChuzzle::`) and dialog names.
- **Mac build:** `re/chuzzle/mac/`, Chuzzle Deluxe 1.0.0 for the Mac, with a name list of about 5,100 lines that names
  the engine's and the game's classes (App, Board and Gadget among them). The survey hoped for one, since it turns the
  class recovery that made Chuzzle HARD into a matter of matching.
- **Engine:** **not SAF**, but Raptisoft's own: App, Graphics (DirectDraw7/Direct3D7 with a software fallback),
  Sprite, TextureMosaic/TextureSplatter, Sound, Music (BASS/MO3), Font (it reads SAF ImageFont `.txt` descriptors),
  GadgetManager, ProfileManager and others. Its source is not public. Dynomite, Raptisoft's 2003 game, is ported but runs
  on SAF, so it shares nothing with this engine but its author.
- **Code:** game ~300 KB plus engine ~320 KB.
- **Data:** loose files: textures with `_Alpha` masks, ImageFont fonts (already supported), 76 OGG, `music/Chuzzle.mo3`
  + `Jukebox.xml` (`platform/openmpt`), `data/gamedata.cfg` key=value tuning, `data/mindbenders.txt` puzzles. The
  `cached/*.cached` texture caches can be ignored.
- **Plan:** translate the engine apart from the game, as the other non-SAF ports did, as a small layer (Sprite, Font,
  Sound, Music, Gadget) over the platform code rather than the D3D7 internals.

### 5.11 Feeding Frenzy 2 Deluxe

**Opus, published, ~10x, EXTRA WORK.**

- **Image:** `FeedingFrenzyTwo.exe` is the 2007-08-10 wrapper; the game inside it is 1,917,952 bytes. The game was
  linked 2006-02-24 with VS2005 (PDB name `FeedingFrenzyTwo-saf-instrument`).
- **RTTI:** full: 597 type names and 581 classes, about 430 of them game classes (Fish, AIFish, PlayerFish, dozens of
  `*AI` and `*State` classes, Stage, GameHud...).
- **Engine:** Sprout's second generation, almost identical to Talismania's ([5.18](#518-talismania-deluxe) has the
  details): a Java-style `engine` (Object, Reader, Runnable, Thread, Scene, ScreenLayout, ArchiveLoader/ArchiveToc,
  ParticleSystem, Splat, QDParser), `dxdriver`, `simplesound`, `shape` (collision) and sigslot. No port has it. Word
  Harmony's first-generation engine shares its formats and many names (the `.saf` archive, ScreenLayout, the XML data),
  but the code was rewritten between the generations. Talismania shares 59 of its 62 engine classes, so the engine is
  built here for both.
- **Code:** game ~470 KB, engine ~570 KB.
- **Data:** `FF2.saf` (14.6 MB, 1,933 entries): 1,239 PNG, 246 JPG, 363 XML (74 stages with spawn tables, actors,
  layouts, particle definitions, strings), 54 OGG and 29 `.theora` sprite animations (frame ranges in `animation.xml`;
  the container is described under Talismania), which need libtheora, the game's own open library, not yet in
  `third_party/`. Loose: `music/FF2.mo3` (`platform/openmpt`).

### 5.12 Peggle Deluxe

**Sonnet, unpublished, ~12x, DIRECT.**

- **Image:** `Peggle.exe` is the 2007-08-10 wrapper; the game inside it is 2,842,624 bytes (v1.01). The game was linked
  2007-07-16 with VS2005.
- **Framework:** SAF ~1.3, Bejeweled's generation, with ModVal tunable strings left in.
- **RTTI:** about 70 game classes (ThunderballApp, Board, Ball, Gun, LogicMgr, PhysObj, CharacterMgr, LevelEditor,
  Replay...).
- **Game code:** ~750 KB.
- **Data:** `main.pak` (11 MB) with 217 `.jp2` images (`platform/openjpeg`; check their wavelet first, see
  [3.6](#36-shared-components)), 62 binary `levels/*.dat` files, ConfigParser `.cfg` files and fonts. Loose:
  `music/PeggleBeats.mo3` (`platform/openmpt`) and 2 OGG.
- **Mac build:** `re/peggle/deluxe/mac/`, Peggle 1.0.5 for the Mac (the PC build is 1.01), with a name list of about
  12,300 lines.
- **Exports:** Ghidra exports of all four Peggle images, made on 2026-10-26, already sit in `re/peggle/<edition>/gh/`
  (deluxe, extreme, nights, wow).
- **Unpublished:** another decompilation is in progress and the modding is strong (Appendix B), so `games/peggle*` is in
  `.publishignore`. Extreme's presentation becomes settings of this port
  ([section 2](#peggle-extreme-and-peggle-world-of-warcraft-edition)).

### 5.13 Peggle Nights

**Sonnet, unpublished, ~4-5x after Deluxe, DIRECT.**

- **Image:** `PeggleNights.exe` is the 2008-10-08 wrapper; the game inside it is 3,342,336 bytes. The game was linked
  2008-09-26 with VS2005 LTCG.
- **Framework:** SAF ~1.3 plus EncodingParser, and OLE drag-and-drop for level-pack import, which needs another way in
  for the web build.
- **Game code:** 81 game classes: the Deluxe set plus CreditsScreen, IntroScreen, InstituteLounge, LevelFileDialog,
  MyLevelEdit, ProgressScreen and others. ~700-950 KB. Only about 6% of bytes are identical to Deluxe, but that
  reflects the LTCG rebuild; the class structure and names carry over from Deluxe's port.
- **Data:** `main.pak` (17.9 MB) with 273 `.jp2` and 70 levels; 16 MO3; `levelpacks/*.pak`.
- **Peggle WoW:** its exe is a branch of the Nights code (PDB path `ThunderballNights\WoW`) with no wrapper: 82
  classes, the Nights set minus CreditsScreen, DownloadOrPlayDialog, ProgressScreen and UpsellScreen, plus AdDialog and
  particle emitters, with 32% of bytes identical to Nights. It is a second view of the same engine, and its presentation
  becomes settings of this port.
- **Mac build:** `re/peggle/nights/mac/`, Peggle Nights 1.0.4 for the Mac, with a name list of about 10,100 lines.
- **Unpublished:** the most active modding community of any game left (Appendix B).

### 5.14 Bejeweled 3

**Opus, published, ~30x, EXTRA WORK.**

- **Image:** `Bejeweled3.exe` is the game itself, with no wrapper (5,484,400 bytes). It was linked 2011-10-26 with
  VS2008 SP1 LTCG and imports `steam_api` (achievements, leaderboards) and WinInet. The game came out in December 2010,
  before EA bought PopCap; this is a later build.
- **Framework:** the post-2010 "prime" framework: a driver layer (IAppDriver, IGraphicsDriver, IAudioDriver,
  IFileDriver, IGamepadDriver...); RenderDevice3D, RenderEffect (`.d3dfx`), a Graphics3D camera and meshes; PopAnim,
  PIEffect, CurvedVal, ResStreamsManager, FEAST and XInput.
- **RTTI:** 151 game classes: Board subclasses for Classic, Speed, Butterfly, Dig, Quest, Zen, Poker and Inferno, quest
  mini-game boards, about 25 Goal classes, about 20 Badge classes, and BinauralManager. `properties/default.xml` AUTOGEN
  comments name 45 source files and 171 `Class::Method:line` entries.
- **Game code:** ~1.0-1.13 MB plus ~0.6 MB of framework.
- **Data:** `main.pak` (90 MB) with 4,557 `.jp2`, 64 `.pam`, 118 `.ppf`, 12 `.p3d` + 5 `.p3danim`, 14 `.d3dfx`,
  quest/secret/speed `.cfg` files and one MO3 suite; loose `binaural/*.sbg` (SBaGen programs for Zen mode), ambient
  OGG and affirmation texts.
- **Starting points:** Twist's port has the PopAnim player, CurvedVal, `.p3d` meshes, shaders rewritten in GLSL and the
  framework's GPU path ([3.6](#36-shared-components)); JPEG 2000 and MO3 are built. New here: the driver layer,
  RenderDevice3D with its `.d3dfx` effects, PIEffect (`.ppf`) and the 3D animations. This layer is what Zuma's Revenge
  reuses.
- **Mac build:** `re/bejeweled3/mac/`, version 1.1.12 for the Mac. Its name list is short (about 2,000 lines): framework
  classes and some of the game's, the quest goals among them.

### 5.15 Banana Bugs

**Opus, published, ~15x.** Added on 2026-10-25; its clear image was made on 2026-11-04.

- **Copy:** the installer (`BananaBugsSetup.exe`, 82 MB) was unpacked into `re/_installers/BananaBugsSetup/` without
  running it, and the game is installed in `C:\Program Files (x86)\PopCap Games\Banana Bugs`.
- **Engine:** despite the name and the rope-walling play it is not Retro64's engine but PopCap's own: `Main.pak`
  (62 MB, the usual PopCap pak), 120 OGG sounds, BASS, and `j2k-codec.dll` for JPEG 2000 images, which
  `platform/openjpeg` replaces. The image shows the framework is the public SexyAppFramework generation
  (`SexyAppBase`, `DDInterface`, `D3DInterface`, `WidgetManager`, `ResourceManager`, `BassMusicInterface`,
  `PakInterface`), which the ports already have, not a 2010 one near Bejeweled 3's as the launcher's date suggested.
- **Image:** it is the only game here behind PopCap's 2010 DRM. `BananaBugs.exe` (2010-05, readable, with RTTI) is the
  launcher, its licensing and update client and `XLayout.dll`, and the game itself is the encrypted `BananaBugs.dat`
  (2.4 MB, entropy 8.0). A clear image was made from the installed copy on 2026-11-04 and is kept as
  `re/bananabugs/bin/BananaBugsGame.exe` beside the copied files, with a `README.txt` on where each came from; how the
  scheme works and the tool that undoes it stay private. The image is the game exe itself: linked 2010-04-09 with
  VS2005, v1.0.0.7, 2,413,936 bytes, a 1.87 MB `.text` with 6,946 functions in Ghidra (libraries included; the export
  is in `re/bananabugs/game/`), imports from the system DLLs only (BASS and `j2k-codec.dll` are loaded at run time),
  and 170 RTTI names, about 85 of them the game's own classes (the bugs and their `BugBrain` AIs, twenty `Bonus`
  classes, the board, the map and its buildings), the rest the framework's. Its link date, version and section sizes
  match the Armadillo-wrapped Shockwave and Big Fish builds below.
- **Effort:** DIRECT, about 15x: the framework is public and the game classes have RTTI, so the extra work is the
  size alone, over a megabyte of game code on a par with Bookworm Adventures Vol. 2's and Zuma's Revenge's, with
  BASS and JPEG 2000 already built. The 2010 DRM launcher and its licensing client are left out of the port.
- **Two more installers (2026-11-04).** The user found two more, which stay in the Downloads folder; both were
  unpacked without running them. `bananabugssetup.exe` is PopCap's own installer, signed by PopCap Games in June 2010,
  and all 485 of its files are identical to our copy's. `InstallBananaBugs.exe` is Shockwave.com's installer, built in
  2010 and signed in 2015 by Defy Media, which then owned Shockwave. Its `Main.pak`, its 120 OGG sounds, `bass.dll`
  and `j2k-codec.dll` are identical to ours. Neither holds a clear game image, so nothing was saved.
- **A repack (2026-11-04).** A third copy, Big Fish Games' build of July 2010 in a repack of PopCap's games that the
  user downloaded from the Internet Archive (`PopCap Games Repacked.zip`, which stays in the Downloads folder), holds
  the same game exe as Shockwave's, still protected, beside a `Main.pak` identical to ours, so it gives no clear image
  either and nothing was saved.

<!-- private -->

**The 2010 DRM scheme and the tool (2026-11-04).** Read from the launcher's code in `re/bananabugs/launcher/`, with
nothing run. `FUN_004c6780` (the only caller of the "Unable to extract game executable" box) opens the `.dat`, reads a
0x44-byte header and decrypts the rest in memory, then writes the result to `popcapgame1.exe` in a temp folder
(`FUN_004c65a0` picks the path) and runs it. The header is a zero dword, the SHA-256 of the clear exe (bytes 4..36)
and the AES-256 key (bytes 36..68): the key is in the file, which is why the unbought game plays its trial offline.
Ghidra's stack layout put the key four bytes later, so the disassembly had to settle it. `FUN_00469510` is a textbook
AES-256 inverse key schedule (seven round constants, Td tables at 0x506d40, Te at 0x503d40, an AES-NI flag in the
schedule's last byte; Crypto++ 5.6's shape) and `FUN_004c6230` is CBC with the first 16 bytes as the IV and
ciphertext stealing for the partial last block: the second-to-last block is decrypted, its tail pads the partial
block, which is then decrypted as the second-to-last plaintext block, and the partial plaintext is the head of the
first result XORed with the partial ciphertext. The plaintext is a container: `PCWE`, the exe's size, its SHA-256
again, a chunk count, then chunks of an 11-byte header (a zero flag byte, two unused bytes, offset, length) and data.
The 35 chunks are shuffled; the launcher puts them in a `std::set` keyed by offset (VC9's node has the value before
its colour byte, so the value sits at the fourth dword) and insists they tile the exe without gaps, hashing them in
order (`FUN_00467c30`, `FUN_00469180`, `FUN_004692b0` are SHA-256) and comparing with both stored hashes.
`tools/extract_popcap2010.py <Game.dat> <out.exe>` (in `.publishignore`) does the same in pure Python, with no third
party module, in about two seconds, and refuses to write unless both hashes match. An earlier attempt had found the
AES and the header but used a home-made AES that decrypted wrongly, so it never saw `PCWE`. The Armadillo copies
below were not needed, only used as a check: the image's link date, version and virtual section sizes match them.

**How the two installers are built.** PopCap's is a VS2008 stub of 2010-05-18 with an LZX cabinet after it, which
7-Zip opens. Shockwave's is Wise's signing stub of 2007 (`SIGNSTUB.EXE`). Its code finds a second PE in its own
resources at file offset 0x1800, writes 0x11800 bytes of it to a temp file and runs that on the outer file. The inner
exe is a Wise 9.02 installer. Its data are raw deflate streams, each followed by its CRC-32 (a few one byte later),
from file offset 0x2565f; Python's `zlib` with `wbits=-15` walks all 220 of them from there. One of the first is the
installer's WSE script: Shockwave's "GAF" build `gaf.banana-bugs-pc.shockwave.com.32.en-us` of a 2010-07-29 game drop.
It installs a `KEYHOLED` folder, checks for a Shockwave Unlimited subscription, and offers a Yahoo toolbar, OpenCandy
and an Offercast installer.

The shortcuts point at `Banana Bugs.exe`, which by its version resource is Shockwave's Keyhole DRM Application 3.2
(2007-12-19). Beside it is `BananaBugs.exe` (2,367,488 bytes, v1.0.0.7, linked 2010-04-09), the game's own name. That
second exe is the game itself, not PopCap's launcher, and there is no `.dat`: its image has a 1.87 MB `.text`, 0x31f10
bytes of `.rdata` and 0xab97c of `.data`, where PopCap's launcher has a 1.0 MB `.text`. Both exes are wrapped by
Armadillo (Silicon Realms): linker version 83.82, the sections `.text1`, `.adata`, `.data1` and `.pdata`, and the
strings `ARMDEBUG=` and `ARMSPLASHOFF`. The game's three sections have no bytes in the file. They are compressed and
encrypted inside the 1.5 MB `.pdata` (entropy 8.0), and only the protector's own loader (boost and std RTTI) and the
resources are clear. So the Shockwave build was a second route to the image, through a widely known commercial
protector, which would have needed a dumper or a downloaded unpacker; PopCap's own scheme (above) needed neither, and
the Armadillo figures now serve only as a check on the clear image. The scripts that read the installers stayed in a
scratch folder, since deleted.

**The repack's exe.** The repack's `BananaBugs.exe` (3,936,256 bytes, dated 2010-07-20) is the same Armadillo-wrapped
game as Shockwave's: linked 2010-04-09, v1.0.0.7, linker version 83.82, its entry point in `.adata`, and the same
virtual sizes for `.text`, `.rdata` and `.data`, which again have no bytes in the file. Its `.pdata` is 3.0 MB where
Shockwave's is 1.5 MB, because here its first megabyte holds Armadillo's licensing engine in the clear (a second PE
with the key-entry, trial and hardware-locking screens), and the game's sections follow in about 2 MB at entropy 8.0.
Its 87 RTTI names are boost's, std's and that engine's own (an embedded IE window, `SecurityCertificate`), none of the
game's, and its imports are the loader's 181 functions from three system DLLs. PopCap's Authenticode signature lies
stale at the end of `.pdata`, where the header's security entry no longer points. Big Fish's launcher is not in the
repack (its `bfgstate.xml` names another exe), and whether the repacker changed the wrapper cannot be seen without
running it. So the repack is a third copy on the Armadillo route, not a way round it.

<!-- /private -->

### 5.16 Gyromancer

**Opus, published, large.** Added on 2026-10-25.

- **Copy:** Steam, `E:/SteamLibrary/steamapps/common/Gyromancer` (2009-09). A puzzle RPG made by PopCap with Square
  Enix, which published it, on battles like Bejeweled Twist's. `re/gyromancer/bin/` holds the Steam exe as it is and a
  clear image of the same build.
- **Engine:** not Twist's or any PopCap framework: no SexyApp class strings at all, and 80 game classes of Square Enix's
  own engine (`CGM_*` game modes, `CData*` tables, effects). Its data is in its own archives (`.bjm`, `.cab`, `.dat`).
  So it no longer depends on Twist.
- **Image:** Steam's DRM stub has encrypted the Steam exe's code section (entropy 8.0). A clear image of the same
  build was found on 2026-11-04. It is kept as `re/gyromancer/bin/Gyromancer_clear.exe` beside the Steam exe, with a
  README, so analysis no longer waits on a tool.

<!-- private -->

**How the clear image differs from the Steam exe.** Two of the user's downloads hold the same cracked exe (1,671,168
bytes, dated 2009-11-18): the `Gyromancer\` folder of the repack in [5.15](#515-banana-bugs) and a separate
`Gyromancer.rar`. It is the Steam exe with Valve's stub taken off. Its link date (2009-09-18), version 1.0.0.1, PDB
path and checksum field are the Steam exe's. Its `.data` and `.rsrc` are byte-identical, and its `.rdata` differs only
in the import descriptors. The `.bind` section and Valve's signature are gone, `.text` is decrypted (entropy 6.6, with
the usual MSVC 8 CRT start as its entry point), and the header's `VLV` marker now reads `THT`. `steam_api.dll` was
dropped from the imports, which leaves its six IAT slots unresolved. To make up for that, the code jumps over its
calls into Steam's API: two of those `call` instructions became short jumps, and two more jumps skip the blocks that
make the other calls, all between 0x418100 and 0x4181c0. `re/gyromancer/bin/README.txt` lists the sites. Apart from
them the image should be the original code, though the Steam exe's encrypted `.text` cannot be used to check it byte
for byte. Its 99 RTTI names are the same as the Steam exe's. Both downloads' data files are identical to Steam's. The
RAR adds a save from 2023 and someone's `Uninstal.exe`, which were not kept.

<!-- /private -->

### 5.17 Zuma's Revenge!

**Opus, web only, ~35x, HARD.**

- **Image:** `ZumasRevenge.exe` is the 2008-10-08 wrapper; the game inside it is 6,651,904 bytes. The game was linked
  2009-08-14 with VS2005 LTCG (internal name Zuma2). It imports `steam_api` (achievements) and WinInet.
- **Size:** `.text` is 5.3 MB, the largest of the first 26 games.
- **Framework:** an internal 2009 generation with RenderDevice/RenderDevice3D, D3D8/9, DXSurface, `.fx` effects through
  a dynamically loaded d3dx9, D3D meshes, PopAnim, PIEffect, CfgCompiler + FEAST, and MoreGamesAd/Flash.
- **Libraries:** JeffLib (13 classes; Bookworm Adventures' port has the parts it used, `games/bookwormadv/jefflib/`),
  PIL (21 classes) and AELib (`.pax` After Effects compositions). None has official source.
- **Game code:** 63 polymorphic game classes (Board, Ball, Gun, CurveMgr, ten Boss subclasses, TikiTemple, MapScreen,
  shader effects). ~1.15-1.75 MB, plus 0.6-1 MB of non-public libraries. LTCG blurs the boundary between game and
  framework, with ~0.9 MB of rasterizer code sitting between game classes.
- **Data:** `main.pak` (106 MB) with 2,633 `.jp2` across three resolution sets, 21 `.pam`, 75 `.ppf` (PIEffect), 10
  `.pax`, font caches and one segmented MO3; loose `levels/*/*.dat` files ("CURV" v15 curves); seven `.fx` effects that
  need GLSL rewrites.
- **Plan:** after Bejeweled 3, reusing its layer and JeffLib.
- **Mac build:** `re/zumasrevenge/mac/`, Zuma's Revenge 1.0 for the Mac, with a name list of about 13,600 lines.
- **Web only:** a PC decompilation is in progress elsewhere (Appendix B), so `games/zuma*` is in `.publishignore`, but
  the web build may go on the website.

### 5.18 Talismania Deluxe

**Sonnet, published, ~12-16x after Feeding Frenzy 2; HARD at about 20-25x on its own.** It comes last, once Feeding
Frenzy 2 has built the second-generation engine.

**Install and image.**
- The install is `A:\SteamLibrary\steamapps\common\Talismania Deluxe`.
<!-- private: - `Talismania.exe` is the 2006-08-22 DRM wrapper. The game PE is carved to `re/talismania/bin/TalismaniaGame.exe`. -->
- `Talismania.exe` (4,678,050 bytes) is the 2006-08-22 DRM wrapper. The game PE is at 0x591a2 (4,313,088 bytes), carved to `re/talismania/bin/TalismaniaGame.exe`.
<!-- /private -->
- The game PE was linked 2006-07-28 with VS2005 RTM and LTCG (`/GL`, 392 LTCG objects). Its internal name is "Atlantis"
  (`AtlantisGameMgr.cpp`).
- The game talks to the wrapper over registered-message IPC (`PopCapDRM_*`). Saves go to the registry plus a hashed
  blob. Both need stubbing or replacing.

**Developer and engine.** The splash shows **RocketJump Games**, Wes Cumberland's company; he is the credited sole
programmer. Design (Del Chafe) and production (Ed Allard) came from ex-Sprout Iggle Pop staff, and PopCap published.
The engine is **not SexyAppFramework** (there are no `Sexy::` classes) but **Sprout Games' second-generation engine**:
the namespaces `engine` (62 classes), `dxdriver` (16) and `simplesound`/`simplesounddx` (12), and about 70 global
framework classes such as GenericApplication, ScreenLayout, Theme, ParticleSystem, Splat, SplinePath, SurfaceCombiner,
PixelCombiner, StateMgr, SoundMgr and BassMusicPlayer. Feeding Frenzy 2 has almost the same engine: 59/62 engine
classes, 16/16 dxdriver, 12/12 simplesound and about 70 shared framework classes. Rendering is 800x600 DirectDraw 7
through a software DrawContext with BltFx, PixelFx and WaveFx effects. Events use sigslot.

**Libraries.** The public ones are sigslot (public domain), zlib 1.1.4, libpng 1.2.5, libjpeg 6b, libogg/libvorbis and
libtheora. BASS is a closed DLL. The engine has its own XML parser.

**RTTI.** Full: 841 type descriptors and 836 vtables. Ghidra found 706 constructors, 636 destructors and 13,790 virtual
functions, in 765 namespace directories.

**Code.**
- 34,453 functions, 2.97 MB.
- The game part (~0x401000-0x5f6000) has 325 game classes with 4,813 RTTI-attributed methods (384 KB, about 100K
  decompiled lines); 7,795 unattributed functions (1.18 MB, about two thirds of them template duplicates; about 520 KB
  after deduplication); and 655 checked-STL functions.
- The realistic translation target is **150-200K decompiled lines**, against Bejeweled's 12.7K. The engine adds about
  100K decompiled lines, including the software blitters.
- Game subsystems: 12 cell types (Normal, Energy, Kraken, Medusa, Minotaur, Pandora, Siren, Spinner, AcidGlob, StarGlob
  and others); paths and pots, and Gameboard; the GameLevel, BonusLevel and AtlantisChapter state machines; map and
  building construction; the Midas narrator and the journal; the HUD and UnlockProcessor; and about 40 `*ClassLoader`
  XML data loaders.

**Data formats.**
- **`Talismania.saf`** (11.5 MB) is a **Sprout Archive File**, decoded and checked byte for byte in the survey:
  - Header: `FFAS` (0x53414646 LE), u16 1, u16 0, u32 TOC offset.
  - TOC: u16 1, u16 0, a 16-byte keyed MD5 of the rest of the TOC, u32 count, then entries
    `{u32 off, u32 size, keyed-MD5(file)[16], u16 namelen, name\0}`.
  - Keyed MD5 is standard MD5 with the IV shifted by the seed 0x53414646 times {11, 71, 37, 97}. Both the TOC and
    per-file digests verify, so archives can be repacked.
  - Data is uncompressed and unencrypted. Word Harmony's engine reads the same format
    (`games/wordharmony/engine/ResourceManager.h`), without checking the digests, as its original does not.
- **Archive contents** (2,529 entries): 1,056 PNG and 935 JPG (914 with `_name.png` masks); 448 XML files with about 180
  element types (boards, chapters, levels, cells, scoring, sounds, splats, strings, screens, themes, fonts as XML glyph
  metrics plus PNG, particle systems and spline paths); 73 OGG; and 14 `.theora` sprite animations. Each animation is a
  sequence of serialized `ogg_packet` headers (`{i32 bytes, i32 b_o_s, i32 e_o_s, i64 granulepos, i64 packetno}`
  followed by the payload), and alpha comes from `_alpha.N.png` frames.
- **Loose files:** `music/*.ogg` (6 BASS streams), `splash/` and `properties/partner.xml`. The log string "FOUND
  Talismania.saf, USING ARCHIVE" suggests the engine also reads loose files.

**The SAF tool.** Talismania's session also publishes a working version of the SAF Extractor/Packer, a tool for these
archives, to the user's fork, https://github.com/allisonpelton/Popcap_.SAF_Extractor-Packer (the user, 2026-11-04;
COORDINATION.md, "Publishing").

**Plan.** Translate the game classes on Feeding Frenzy 2's engine, starting from the RTTI-named state machines
(GameLevel, AtlantisChapter, the CBaseCell family, Gameboard) and the `*ClassLoader` XML parsers, which must reproduce
every attribute exactly. For the assets: merge each JPEG with its mask; play `.theora` through libtheora; play the music
through the platform's OGG path instead of BASS; draw at 800x600; keep saves in files instead of the registry and the
hash blob; and make the loading thread (`ILoadingThread`) synchronous or cooperative for WASM.

**Obstacles.** Step 3 of the method fails, since no public engine source exists; the size, 325 classes plus about
1.2 MB of unattributed code, much of it template bloat that has to be told apart from real logic; LTCG inlining of
engine helpers into game functions; 448 XML files whose loaders must be reproduced exactly; Theora animations; the
loading thread; and BASS, the registry with its hash check, and the DRM IPC. None of these is a blocker: the image is
not packed and has full RTTI. It is just large.

---

## 6. Open questions

1. **Typer Shark's leftover source: there is none (checked 2026-11-04).** The "Typer Shark Abyss" release, bundled with
   Typing Instructor Deluxe 17 ([archive.org item](https://archive.org/details/typing-istructor-deluxe-17-typer-shark-abyss)),
   was downloaded and unpacked without running anything. Its "CVS folder" is PopCap's CVS working-copy metadata in five
   data folders: file names, revisions and dates of data files, and no code. The release has no `.cpp`, `.h`, project,
   map or PDB file. What it does hold is two older builds of the game, described in
   [5.6](#56-typer-shark-deluxe). No decision about source is left. The older builds are PopCap's own exes, like the
   Mac builds, which the project's rules already let a port use as a guide with the PC exe as the authority.
2. **Typing on phones (Typer Shark).** The web build needs a way to type on a touch screen: either a hidden `<input>`
   focused on a tap that forwards characters, with the canvas laid out above the soft keyboard, or an on-screen
   keyboard drawn by the web player. Which one is the user's choice.
3. **More Mac builds.** A Mac build names the classes of a game whose PC exe has no game RTTI, as Chuzzle's does. Of the
   games left, AstroPop, Heavy Weapon, Venice, Pizza Frenzy, Iggle Pop and Feeding Frenzy lack it. A web search
   (2026-11-04) found Mac releases of two: Iggle Pop Deluxe (OS X, January 2006, for PowerPC and Intel; PCGamingWiki
   says the Steam version runs on macOS) and Feeding Frenzy (OS X, 2007, Intel). Both were in PopCap's own bundle of
   its 13 Mac games. The user found both (2026-10-31), and then copies without the launcher's protection (2026-11-04)
   that hold the same game binaries, both universal PowerPC and Intel builds. Their symbol tables are stripped (17 and
   19 names, none from C++), but GCC left their RTTI in: Iggle Pop's names 399 classes and Feeding Frenzy's 148, with
   their base classes, Sprout's engine among them. Both are now in `re/<game>/mac/`
   ([5.8](#58-iggle-pop-deluxe), [5.9](#59-feeding-frenzy-deluxe)), and they name Pizza Frenzy's engine too. PCGamingWiki
   credits a 2005 OS X port of Pizza Frenzy to Ryan C. Gordon, without a source. Nothing shows a Mac release of
   AstroPop, Heavy Weapon, Venice, Gyromancer or Banana Bugs. Any Mac build the user finds goes into `re/<game>/mac/`.
4. **Both late images are in hand.** A clear image of Gyromancer's Steam build was found on 2026-11-04 and is kept in
   `re/gyromancer/bin/` ([5.16](#516-gyromancer)), and Banana Bugs' was made from the installed copy the same day
   and is kept in `re/bananabugs/bin/` ([5.15](#515-banana-bugs)); the how-to stays private. No image waits on a
   download any more.
5. **The survey's leftovers.** The old session's scratchpad folder
   (`C:/Users/Administrator/AppData/Local/Temp/claude/C--Users-Administrator-Documents-claude-wasm-bejeweled/bb6f3864-facb-48c7-acfa-f782a25ef960/scratchpad/survey/`)
   survives only in fragments: 1,058 files and 27 MB of partial Ghidra exports, with every script gone. The scripts
   worth keeping became `tools/rtti_msvc70.py`, `tools/ghidra_scripts/DisableNoReturnDiscovery.java`,
   `tools/funcmatch.py`, Word Harmony's `.saf` reader and Bookworm Adventures' PopLua tools. The Ghidra projects
   `survey_typershark` and `survey_talismania` in `E:/retools/projects` hold full analyses of two games still to port;
   `survey_bookworm_deluxe`, `survey_bwa`, `survey_bwa_norecover`, `survey_ba2` and `survey_insaniquarium` serve games
   ported or not decompiled, and `survey_bwa` is an abandoned run. Whether to delete the folder and those five projects
   is the user's call.

**Caution for future research.** The tcrf.net pages for Bookworm Adventures and Bejeweled 3 returned text aimed at AI
agents (payment demands, instructions to write files or shut down). It was ignored, and nothing from those pages is used
here.

---

## Appendix A: other studios' games

The user asked (2026-10-25) which games that PopCap neither developed nor published run on an engine shared by two or
more games on this list. A research agent searched the web for them. MobyGames, ModDB, Giant Bomb and the Internet
Archive refused its requests, so much of what follows rests on search-engine summaries of MobyGames pages and should be
checked before anything depends on it. In the evidence column, A means the game's own credits or store page name the
framework, B an engine field on PCGamingWiki, Wikidata or Wikipedia, C a MobyGames middleware tag seen only in a search
summary, and D the same developer with the engine assumed.

Only SexyAppFramework turned up such games. PopLua, Sprout's engine, Retro64's, SpinTop's and Nuclide's showed none
outside PopCap's catalogue, apart from Retro64's own Bugatron Worlds (2006) and Rune Raiders, both D only. PCGamingWiki
files the Sprout and Retro64 games under the PopCap framework, but the triage contradicts it: Feeding Frenzy Deluxe's
classes are Sprout's own `engine` namespace, and Cosmic and Water Bugs carry no SexyApp class names at all. Its engine
labels seem to follow the publisher, not the code.

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

Bunny Quest (2013) and Nuclide's Penguin Puzzle (2005) rest on even less. Eight hidden-object games on the framework
were found too and left out by the user's rule against the genre. MobyGames' framework group reportedly holds 94 games,
mostly PopCap's, and the agent could not open it, so more may exist. No decompilation or source release turned up for
any of these; the framework's own 1.3 source is public on SourceForge.

**Decision (the user, 2026-10-25):** the other studios' games come at the very end, after every PopCap game, and World
of Goo is wanted among them. Which of the others to port is decided then; each needs the user's own copy.

**Octogeddon, first of the other studios' games (the user, 2026-11-01):** "Spiritually, it's a PopCap game." It is by
All Yes Good (developer and publisher), and the user's Steam copy is app 525620 (705 MB). It doesn't use PopCap's
framework, so it is not on this appendix's shared engines:
- **Exe:** a 4.8 MB 32-bit `octogeddon.exe` on Direct3D 9, DirectSound and XInput, with `steam_api.dll`.
- **Framework:** the studio's own C++ framework (`AppLib`, with `AnimLib`, `Json` and `Pak`).
- **Class names:** MSVC RTTI names 447 classes, the game's own among them, so the names come free.
- **Data:** one 700 MB archive, `data.ayg`, whose header starts "AYGP".

The Ghidra route fits, with SDL in place of Direct3D 9 and DirectSound, as in the other ports. Placing it first among
these games, ahead of World of Goo, follows the user's remark; they may move it.

**Unity games wanted too (the user, 2026-10-27):** Steven Universe: Save the Light, Steven Universe: Unleash the Light
and OK K.O.! Let's Play Heroes, all three of them Cartoon Network games. They come at the end with the other studios'
games. The user's Steam copies are Unity Mono builds, with the game's own code in `Assembly-CSharp.dll` and no IL2CPP.

| Game | Year | Developer | Unity | Install | Notes |
|---|---|---|---|---|---|
| Steven Universe: Save the Light | 2017 | Grumpyface Studios | 2018.1.0f2, 32-bit | 4.9 GB | 3D; Steamworks.NET |
| Steven Universe: Unleash the Light | 2019 | Grumpyface Studios | 2020.3.12f1, 64-bit | 1.7 GB | 3D; Steamworks.NET |
| OK K.O.! Let's Play Heroes | 2018 | Capybara Games | 5.6.1f1, 64-bit | 5.7 GB | FMOD Studio, InControl |

The Bejeweled method does not fit them: their code calls the Unity engine for everything, and the framework covers none
of it. The user doesn't want the engine decompiled ("I'd just love to play them in my browser and make modding
easier"), so the plan has two parts. Modding uses BepInEx with Harmony patches on the native game, the usual way to mod
Unity Mono games. Browser play comes from a local WebGL rebuild of the player's own copy: its assets are ripped into a
Unity project, the decompiled scripts are added, and the project is built for WebGL in the matching editor. Such a
build holds the publisher's assets and code, so it stays private and never goes on the website.

Unleash the Light is the best first try: it is on 2020.3 LTS and uses no FMOD. The weak points are shaders recovered
from a build, the WebGL memory limit and FMOD's web libraries. Every tool (AssetRipper, Unity Hub and the editor, a .NET
decompiler) is a download that needs the user's OK, and the user accepts Unity's license themselves.

**Glass Tower, near the end (the user, 2026-11-01):** a discontinued Android game. The user's copy is an APK in their
Downloads folder (version 1.0.10, 40.7 MB), and it won't run on modern Android, such as the user's phone. The user would
like native, browser and Android versions. The APK holds a Java shell with 2016-era ad and Play services libraries, plus
the game itself as a 5.9 MB native library, built only for the old 32-bit `armeabi` ABI. That ABI is the likely reason it
won't install on current phones. Its levels are XML files with PNG maps. So the Ghidra route fits, on ARM code instead
of x86. The ad and Play services code is left out. An Android build would be a new target for the project.

**The Tasty Planet series, after Glass Tower (the user, 2026-11-01):** Dingo Games' series. The user doesn't own every
game yet and is waiting for the series to go on sale again; each port needs the user's own copy. Three are in the Steam
library on A: (Tasty Planet, Tasty Planet 2, and Tasty Planet Forever, app 946010, 78 MB). Tasty Planet Forever is a
4.4 MB native exe, `tastyplanet4.exe`, on SDL2 and SDL2_mixer, with `steam_api.dll`, its data in `assets/`. A native
SDL2 game suits the Ghidra route, and the port can keep SDL2 itself; the two older games were not looked at. Which games make up the series, and the order to port them in, is settled when it is their turn.

---

## Appendix B: other projects and modding communities

Checked on 2026-10-26 for the publishing rule ([section 2](#where-a-port-is-published)), in a research pass; the Peggle
Nights, Zuma Deluxe and Pizza Frenzy findings were rechecked by hand. "Active" means mods or tools since late 2024:
"strong" is many authors or regular releases, "thin" a handful, often from one person. Dates are the last activity seen.
A "none found" means GitHub, GameBanana and Nexus showed nothing; Reddit, ModDB and Discord servers were not read. No
port reads any of these projects (CLAUDE.md, "No references"); they matter only for where a port is published.

| Game | Other decompilation or remake | Modding | Result |
|---|---|---|---|
| Peggle Deluxe | [Thunderball](https://github.com/teampopwork/Thunderball), matching decompilation, 39% (2026-10-18); Xbox 360 recompilations | strong: the [haggle](https://github.com/PeggleCommunity/haggle) mod kit (2026-08), a few GameBanana mods | unpublished |
| Peggle Nights | none of its own (the Xbox 360 recompilations include it) | strong: [GameBanana](https://gamebanana.com/games/6197), 82 mods, nine in 2026 from eight people (newest 2026-10-01); [PeggleEdit](https://github.com/IntelOrca/PeggleEdit) level editor (2026-03) | unpublished |
| Zuma Deluxe | [CircleShootApp](https://github.com/alula/CircleShootApp), finished and playable (quiet since 2025-12), with active ports and a browser build | strong: [GameBanana](https://gamebanana.com/games/8223), eight mods in 2026 from four people (six from one); new level tools in 2026-10 | not decompiled |
| Pizza Frenzy | one unlicensed upload of a decompilation (2026-10-03), nothing since | none found | published |
| Chuzzle Deluxe | a repository whose 13 game files are empty (2026-04); an iOS recompilation | dormant (one mod, 2024) | published |
| Bejeweled 3, Zuma's Revenge, Bejeweled 2 | console or phone builds being recompiled or decompiled (2026-10), which don't count | thin | published (Zuma's Revenge: the next row) |
| Zuma's Revenge (PC) | a decompilation in C++ by a developer in China, written without AI, reported to the user on 2026-10-26; a clip showed it crashing at the main menu | | web only, until the user learns more |
| Bookworm Adventures 1 and 2 | none | thin: one person's mod builder (2026-07); a Spanish translation of Vol. 2 | published |
| Bejeweled Twist, Dynomite, Heavy Weapon, Feeding Frenzy 2 | Heavy Weapon has a reimplementation for the Vita | thin: a fix, a recovered official level editor, an input mod, an Archipelago client | published |
| Alchemy | flamel, a web remake written from scratch (2026-10) | none found | published |
| Bejeweled, Big Money, BookWorm | none | thin: a few GameBanana mods; BookWorm's word-list editor | published |
| every other game in the order | none found | none found | published |

Plants vs. Zombies and Insaniquarium were not rechecked: both have finished decompilations, many ports and large
communities, and neither is decompiled here.

---

## Appendix C: where the copies are

The remaining games' copies, all read only:
- **Steam, `A:/SteamLibrary/steamapps/common/`:** AstroPop Deluxe, Bejeweled 2 Deluxe, Bejeweled 3, Chuzzle Deluxe,
  Feeding Frenzy 2 Deluxe, Hammer Heads Deluxe, Heavy Weapon Deluxe, Iggle Pop Deluxe, Peggle Deluxe, Peggle Extreme,
  Peggle Nights, Pizza Frenzy, Talismania Deluxe, Typer Shark Deluxe and Zuma's Revenge, and among the other studios'
  games World of Goo, Octogeddon and the Tasty Planet games.
- **Steam, `E:/SteamLibrary/steamapps/common/`:** Gyromancer, a second Peggle Extreme, and the three Cartoon Network
  games.
- **`C:/Program Files (x86)/PopCap Games/`:** Venice Deluxe, Banana Bugs and Peggle World of Warcraft Edition. Venice's
  and Banana Bugs' installers are also unpacked, unrun, into `re/_installers/`.
- **`C:/Program Files (x86)/Popcap Game Collection/`:** Feeding Frenzy Deluxe.
- **The Downloads folder:** Glass Tower's APK, the two Banana Bugs installers of [5.15](#515-banana-bugs), and the Mac
  downloads of Iggle Pop and Feeding Frenzy.

In the main checkout (git-ignored), `re/<game>/bin/` holds the exe or carved image of every remaining game (Banana
Bugs' made from its `.dat` on 2026-11-04), `re/peggle/<edition>/` holds the four Peggle images with their Ghidra
exports, and `re/<game>/mac/` the Mac builds (section 5). The survey's own working files are in question 5 of
[section 6](#6-open-questions).
