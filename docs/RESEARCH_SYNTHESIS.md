# Bejeweled Deluxe 1.87: plan for a decompiled, moddable C++ port with native and WebAssembly builds

This plan merges the four research notes (dated 2026-10-18). I re-checked the most important local facts by reading the user's install and the extracted inner game image; nothing was modified. Those facts are marked **[checked]**. Everything else comes from the notes with their URLs. Anything still unconfirmed is marked **UNVERIFIED**.

Scripts used for the checks are in `C:\Users\Administrator\AppData\Local\Temp\claude\C--Users-Administrator-Documents-claude-wasm-bejeweled\bb6f3864-facb-48c7-acfa-f782a25ef960\scratchpad\synth\`:
- `rtti.py` compares RTTI, imports and Rich headers.
- `strs.py` greps strings.

---

## 0. Facts about the binary [checked]

### The two PE images inside WinBej.exe

**Outer image: `WinBej.exe` is only the DRM/Steam wrapper.**
- Size 2,972,712 bytes. SHA-256 `a16fa32d6603f12843fce14153eecb5278cc907cc46aa862248229f5563b8376`.
- TimeDateStamp 2007-08-10 22:59:23 UTC. Linker 8.0.
- RTTI found only here: DRMApp, LuaApp, PopLua, LuaWidget and the other Lua* types, OggDecoder, PNGDecoder, CircularBuffer, WinInetHTTPTransfer.
- Strings: `SteamStartup`, `steam.dll`, `-steamuser`.

**Inner image: the real game is a second PE appended to the file.**
<!-- private -->
- Location: file offset `0x17b1a2`, length `0x15aa86` (1,419,910 bytes).
<!-- /private -->
- SHA-256 `6257a77f0e54f65b5cdbe36faf1ee6872189a700b05e5ed608ae545c0649466e`.
- ImageBase 0x400000. TimeDateStamp 2007-04-13 20:28:30 UTC. Not encrypted.
- `.text` is 0x10404c bytes.
- Inner RTTI has 89 types:
  - **Game classes:** WinDMApp, Board, TitleScreen, HighScoreDialog, HighScoreWidget, OptionsDialog, RecordScoreDialog, RegisterDialog, UpdateCheckDialog, BetaSupport, InternetManager.
  - **Framework classes:**
    - App and widgets: SexyAppBase, SexyApp, WidgetManager, WidgetContainer, Widget, ButtonWidget, Checkbox, Slider, Dialog, DialogButton, EditWidget, HyperlinkWidget.
    - Graphics and fonts: Graphics, MemoryImage, DDImage, DDInterface, D3DInterface, D3DTester, NativeDisplay, ImageFont, FontData, SysFont, ImageLib::Image.
    - Resources and parsing: ResourceManager (with ImageRes, FontRes, SoundRes), PropertiesParser, XMLParser, DescParser, Buffer.
    - Audio: MusicInterface, BassMusicInterface, FModMusicInterface, DSoundManager.
    - Other: HTTPTransfer, PakInterface, PakInterfaceBase.

### Compiler and libraries
- **Compiler:** the Rich-header entries are build 50727, which is VS2005 / VC 8.0 with linker 8.0.
  - A few objects are build 4035. These are probably prebuilt SDK `.lib` files (inference).
  - The Rich header cannot distinguish VS2005 RTM (.42) from SP1 (.762).
  - This is the same compiler family as Peggle Deluxe, which Thunderball rebuilds with 14.00.50727.762.
- **CRT:** statically linked; there is no msvcr80 import.
- **Imports:** KERNEL32, USER32, GDI32, ADVAPI32 (Reg*), WINMM (timeGetTime, PlaySoundA, mixer*), WSOCK32, SHELL32, ole32, OLEAUT32.
- **Loaded at runtime instead of imported:** ddraw, d3d8, `.\bass.dll`, fmod.
- **Shipped `bass.dll`:** BASS **2.3** (Un4seen, ©1999-2006).

### Framework generation: about 1.3
- The inner image contains:
  - `main.pak` and PakInterface;
  - the image extensions `.tga/.jpg/.png/.gif/.j2k/.jp2`;
  - the font commands `CreateLayerFrom` and `LayerPointRange`.
- There is **no** resources.xml string and **no** ModVal string.

### Hard-coded paths
- Paths are lower-case with backslashes, while the files on disk use mixed case:
  - The exe says `music\1980-autonomus.s3m`; the file is `1980-Autonomus.s3m`.
  - Others: `images\`, `sounds\`, `data\diff.txt`, `data\afont1.txt` and the other font descriptors, `properties\default.xml`, `properties\partner.xml`.
- **Consequence:** the port must accept `\` as a path separator and look files up case-insensitively.

### Persistence
- Save files: `userdata\hiscores.dat` and `@userdata\savegame.dat`, inside SAF's app-data folder.
- Settings go to the registry under `PopCap\Bejeweled`; `Software\PopCap\Beta` belongs to BetaSupport.
- PCGamingWiki says saves live in `%PROGRAMDATA%\Steam\Bejeweled\userdata\`. That folder does not exist on this machine.

### Built-in SAF demo recorder
- Command-line flags present in the inner image: `-record`, `-play`, `-playnum`, `-recnum`, `-demofile`.
- Also present: `-changedir`, `-crash`, `-screensaver`, `-version`.

### Assets
- About 1.6 MB in total:
  - `images/`: 56 files, 621 KB.
  - `sounds/`: 25 sounds, each as both .ogg and .mp3, 560 KB.
  - `music/`: 3 × .s3m, 392 KB.
  - `data/`: 6 .txt files.
  - `properties/`: default.xml, partner.xml and partner.xml.sig.
- There is no `.pak`. Also present are `drm/`, `drm.swf` and `Flash.ocx`, which make up the DRM UI.
- Hashes for an asset validator:
  - `data/diff.txt` `127c55f16e064c243c29fc66a69f5352d552f436f86cb90fa5eeb2e8148c88f0`
  - `images/gemsheet6.png` `4d560fb26e5dd44aaca4f9c5ba517caa1de2faf22bfcbd725f1f4ba51ecbf043`

### PvZ-Portable framework (checked in the local clone)
- `src/SexyAppFramework` has **no includes of PvZ game code** (Lawn/PvzpLib/TodLib), so it can be lifted out on its own.
- Rendering:
  - It draws straight to the window's drawable at full resolution.
  - The viewport is **letterboxed to 4:3** and uses an ortho projection in logical coordinates (GLInterface.cpp around lines 1193-1257). An HD texture is therefore shown at window resolution. Bejeweled's 640×480 is 4:3.
- Images: its ImageLib keeps the GIF decoder. It **changes the probe order to `.png, .jpg, .gif, .tga`**, whereas SAF 1.3 uses tga, jpg, png, gif, j2k, jp2.
- Audio: SDL-Mixer-X is vendored as a submodule with modplug, xmp, midi, opus and flac switched off. libopenmpt is the only tracker backend.
- WASM link flags (CMakeLists):
  ```
  -sUSE_SDL=2 -sUSE_ZLIB=1 -sUSE_LIBPNG=1 -sUSE_LIBJPEG=1 -fexceptions -sASYNCIFY -sALLOW_MEMORY_GROWTH=1
  -sMAXIMUM_MEMORY=1073741824 -sGROWABLE_ARRAYBUFFERS=0 -sFORCE_FILESYSTEM=1
  -sEXPORTED_RUNTIME_METHODS=["FS","callMain"] -sINVOKE_RUN=0 -lidbfs.js --shell-file wasm/shell.html
  ```
- `wasm/build-wasm.sh --deps` builds libopenmpt with `emmake make CONFIG=emscripten`. That needs bash and GNU make.
- `shell.html` loads `jszip@3` from jsDelivr with no exact version pinned.

---

## 1. Contradictions between the notes

### Resolved

| # | Conflict | Resolution |
|---|---|---|
| 1 | The task context says WinBej was "built 2007-08-10"; the game note says the game PE is dated 2007-04-13. | Both are right. 08-10 is the **outer DRM wrapper**; 04-13 is the **inner game**. All reverse engineering targets the inner PE. |
| 2 | The game note says the binary lacks `CreateLayerFrom`/`LayerPointRange` and so predates 1.3. The decomps note says it is about 1.3 because of PakInterface. | **The game note is wrong.** Both strings are present in the inner PE, as are `main.pak`, PakInterface and j2k/jp2. The inner framework is a 1.3-era internal build from April 2007, which matches the third-party extension evidence that 1.3 existed by 2007-08 (https://tdjdextensions.sourceforge.net/index_en.html). |
| 3 | The frameworks note says CircularBuffer, PNGDecoder, OggDecoder, PopLua, LuaApp, DRMApp, LuaWidget and SteamStartup "must be rebuilt or replaced". | All of them are **only in the outer DRM wrapper**, so they are not needed. The game still needs these non-public classes: BetaSupport, InternetManager (both to be stubbed) and the dialogs. |
| 4 | The tooling note's headless Ghidra example imports `WinBej.exe`. | That would analyse only the DRM wrapper. Import the extracted **inner** PE (section 3). |
| 5 | The tooling note recommends an S3M player of Emscripten's SDL2_mixer port (libmodplug) on web and SDL2_mixer 2.8.2 (libxmp) on MinGW. PvZ-Portable uses SDL-Mixer-X with libopenmpt. | Use **one decoder everywhere: libopenmpt through PvZ-Portable's vendored SDL-Mixer-X**. Native and web then sound the same, and SDL-Mixer-X provides `Mix_ModMusicStreamJumpToOrder` for SAF's music-order offsets. |
| 6 | The tooling note recommends i686 native builds "for struct-offset parity". | Struct offsets only matter if new code is mixed into the original process, as in OpenRCT2/re3-style DLL replacement. A source-level port should target **x86_64 natively and wasm32 on the web**. Keep i686 only for the optional hook DLL (section 9.6). |
| 7 | The notes list PvZ-Portable's WASM flags differently from each other. | Both lists were incomplete. The authoritative list is in section 0, taken from CMakeLists.txt. |
| 8 | PvZ-Portable's history start: 2023-07-30 or 2023-07-31. | A time-zone difference; it doesn't matter. |
| 9 | The readme says a Time Trial level-2 set is worth 30 points; the code gives 25. | **The code wins.** "Vanilla" behaviour means behaviour as written in the code. StrategyWiki's table agrees with the code. |
| 10 | SAF 1.3's image probe order differs from PvZ-Portable's. | No vanilla asset has two colour files with the same stem and different formats, so vanilla is unaffected. For mods, restore the SAF 1.3 order (tga, jpg, png, gif) inside the VFS so the result is deterministic. |

### Still open (resolve early in Ghidra)

1. **Frame time.** Does WinDMApp change `mFrameTime` (SAF default 10 ms = 100 Hz) or `mVSyncUpdates`? PCGamingWiki's "capped at 100 FPS" suggests it keeps the default.
2. **RNG seeding.** Board uses CRT `rand()` (`rand() % 7` at 0x4cab3d). In public SAF 1.3, demo playback seeds only Sexy's MTRand (`SRand(mRandSeed)`); CRT `srand(GetTickCount())` runs regardless (SexyAppBase.cpp lines 6110-6126). Whether WinDMApp seeds the CRT deterministically is **unknown**, and it decides whether original `-record` demos can be replayed.
3. **SysFont.** Does game code actually construct SysFont? Its vtable may only be linked in through ResourceManager. PvZ-Portable has no SysFont.
4. **Blocking dialogs.** Does the game use blocking `Dialog::WaitForResult`/`DoDialog`? This decides whether `-sASYNCIFY` can be dropped.
5. **Music order offsets.** Does it call `PlayMusic(id, offset)` with non-zero order offsets?
6. **FPU precision.** Is D3D7 or DirectDraw the default renderer? SAF 1.3 never sets `DDSCL_FPUPRESERVE`. Hypothesis, not checked against Microsoft docs: when the D3D7 path is active, the x87 FPU may be switched to single precision, which would change Time Trial drain accumulation.
7. **Wrapper arguments.** Does the Steam wrapper pass `-record` etc. through to the inner game?
8. **Gameplay details:**
   - what the Board state values mean;
   - what triggers the Time Trial reshuffle;
   - gem fall and swap speeds;
   - what `L mod 4` controls;
   - what Board+0x220 is;
   - whether a bad move pauses the timer (wiki.gg says yes);
   - the size of the high-score table;
   - the formats of hiscores.dat and savegame.dat.
9. **Mac build with symbols.** Does a Mac Bejeweled Deluxe build with symbols exist? (PopSilicon lists Bejeweled 2/3, not 1.)
10. **Function ID coverage.** Does Ghidra's `vsOlder_x86.fidbf` cover VS2005? This is inferred from the file name only.

---

## 2. Methodology and why

**Recommendation: a functional, non-matching decompilation of the *game classes only*, written against a reused portable SexyAppFramework (SAF). Skip the DRM wrapper completely.**

Matching is optional and per-function (section 8). Incremental replacement inside the running original (DLL injection) is not used.

| Precedent | What they did | What we take from it |
|---|---|---|
| Zuma Deluxe — alula/CircleShootApp https://github.com/alula/CircleShootApp (MIT changes) | Game code rebuilt on the **public SAF 1.3** with a few patches; framework not decompiled; **DRM not reverse-engineered**; Mac exe used to cross-reference names; MSVC 7.0 identified from the Rich header | The closest template: same company, same era, same framework. Its derivative Zuma Portable (https://github.com/kyle-sylvestre/CircleShootApp, https://ksylvestre.itch.io/zuma-portable) **already runs a second PopCap game on the PvZ-Portable framework** |
| Peggle Deluxe — teampopwork/Thunderball https://github.com/teampopwork/Thunderball (MIT) | Shared Ghidra server, Mac debug symbols, **reccmp** with MSVC 8.0 SP1 (https://github.com/bananapizzuh/msvc80-sp1) | Same compiler as ours. Its toolchain is the option if we ever want function matching |
| Insaniquarium — Vindirect/WinFish https://github.com/Vindirect/WinFish | Decompiled from the exe alone with mostly guessed names, and still "fully playable"; ported to PopLib/SDL3 by https://github.com/SaMeiers/insaniquarium-port (AGPL), which uses scripted patches and never edits the decomp | Proves an exe-only, symbol-less approach is enough for a game this small |
| PvZ: Patoke https://github.com/Patoke/re-plants-vs-zombies (CC0) → headshot2017 https://github.com/headshot2017/re-plants-vs-zombies → **wszqkzqk/PvZ-Portable** https://github.com/wszqkzqk/PvZ-Portable (LGPL-3.0+) | Built from the 0.9.9 PDB decomp (https://github.com/ruslan831/PlantsVsZombies-decompilation; matching variant https://github.com/bluisblu/pvz), with `//0xADDR` comments on functions; SAF ported to SDL2 + GLES2 + SDL-Mixer-X/libopenmpt + **Emscripten**; live at https://wszqkzqk.github.io/PvZ-Portable/pvz-portable.html | **The only SAF with a working WASM build.** We reuse its framework and copy its asset-import/IDBFS pattern and its address-comment habit |
| sm64 → sm64ex https://github.com/n64decomp/sm64, https://github.com/sm64pc/sm64ex | Matching decomp first, then a separate port with `TARGET_WEB` and **texture packs** | Precedent for "swap the graphics" |
| LEGO Island: https://github.com/isledecomp/isle → https://github.com/isledecomp/isle-portable (reccmp: https://github.com/isledecomp/reccmp, AGPL) | Matching decomp, then a separate SDL3 tree with a web build (isle.pizza) and "extensions" | Keep reverse-engineering artefacts separate from the portable tree |
| devilution → devilutionX / diabloweb https://github.com/diasurgical/devilution | The user supplies DIABDAT.MPQ; the web build hosts only the shareware data | Never host the game data |
| re3 (https://git.dawidpotocki.com/mirror/re3), OpenRCT2 (https://github.com/kevinburke/OpenRCT2 → https://github.com/OpenRCT2/OpenRCT2) | Replaced functions one at a time inside the original exe via a DLL | Not needed: only about 11 game classes, and the framework source already exists |

**Why non-matching.**
- The goal is a moddable WASM port, not byte-identical proof.
- Every comparable project that wanted a port shipped a separate non-matching tree (isle → isle-portable, devilution → devilutionX, sm64 → sm64ex).
- Behavioural fidelity is checked with deterministic replay and state diffs instead (section 8).

**Why reuse a framework rather than decompile it.**
- The 1.3 framework source is public (https://github.com/PeggleCommunity/PopCapFramework, branches `main` and `v1.34`; https://github.com/LolHacksRule/sexy; SourceForge zip at https://sourceforge.net/projects/popcapframework/).
- A portable derivative of it already has a WASM build.
- Framework code in the inner exe only needs to be *identified* (through Function ID), so the game code can be separated from it.

**No Bejeweled 1 decompilation exists anywhere.** GitHub searches for "bejeweled decomp", "bejeweled decompilation" and "WinDMApp" found only unrelated WP7/C# projects:
- https://github.com/bognarit80/BejeweledLivePlusDecomp-MonoGame
- Anonymous1212144/PopCapCSDecomp
- LolHacksRule/Prime (https://github.com/LolHacksRule/Prime)

The fan clone https://github.com/LDinos/bejeweled-1-clone has no licence and its numbers are only partly right. Use it as a behaviour reference only.

---

## 3. Toolchain (versions verified by the tooling note, 2026-10-17/18)

### Reverse engineering

| Tool | Version | License | Install |
|---|---|---|---|
| Ghidra | **12.1.4** (2026-09-21), zip SHA-256 `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db` | Apache-2.0 | https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.1.4_build/ghidra_12.1.4_PUBLIC_20260921.zip |
| JDK | Temurin 21 (21.0.4.7 is already installed; current release 21.0.12.1) | GPLv2+CE | already present, or https://github.com/adoptium/temurin21-binaries/releases/download/jdk-21.0.12.1%2B1/OpenJDK21U-jdk_x64_windows_hotspot_21.0.12.1_1.zip |
| PyGhidra | 3.1.0 (bundled) | Apache-2.0 | from Ghidra's `pypkg/dist`; type stubs `ghidra-stubs==12.1.4` |
| pefile / capstone | 2024.8.26 / 5.0.9 (the versions in the scratch venv used for the checks) | MIT / BSD | `pip install pefile==2024.8.26 capstone==5.0.9` |
| MSVC 8.0 SP1, optional | 14.00.50727.762 | Microsoft EULA | https://github.com/bananapizzuh/msvc80-sp1 (Thunderball's source); also https://github.com/widberg/msvc8.0. Used only to build Function ID databases or for reccmp. Whether it includes `libcmt.lib` is **UNVERIFIED** |
| reccmp, optional | — | AGPL-3.0 (tool only; does not affect our code's license) | https://github.com/isledecomp/reccmp |
| Others, not recommended as primary | IDA Free (cloud decompiler, no scripting; https://hex-rays.com/ida-free), Binary Ninja Free (no API; https://binary.ninja/free), Cutter 2.5.0 / rz-ghidra 0.9.0, OOAnalyzer (https://github.com/cmu-sei/pharos, Linux/Docker; its Kaiju import only builds up to Ghidra 12.1.3, https://github.com/CERTCC/kaiju/releases/tag/260910), Diaphora (needs full IDA, https://github.com/joxeankoret/diaphora), https://github.com/threatrack/ghidra-fid-generator | | |

Install commands (cmd):
```bat
mkdir C:\tools & cd /d C:\tools
curl -L -o ghidra.zip https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.1.4_build/ghidra_12.1.4_PUBLIC_20260921.zip
certutil -hashfile ghidra.zip SHA256
tar -xf ghidra.zip -C C:\tools
set "JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-21.0.4.7-hotspot"
set PY_PYTHON=3.12
py -3.12 -m venv C:\re\venv && C:\re\venv\Scripts\pip install pefile==2024.8.26 capstone==5.0.9 pyghidra==3.1.0
```
`PY_PYTHON=3.12` is needed because `pyghidraRun.bat` calls `py`, which might otherwise pick Python 3.8.

### Build

| Tool | Version | Install |
|---|---|---|
| CMake / Ninja | 4.4.4 / 1.13.2 | `py -3.12 -m pip install --user cmake==4.4.4 ninja==1.13.2` (winget: `Kitware.CMake`, `Ninja-build.Ninja`). CMake 4 refuses `cmake_minimum_required` below 3.5; PvZ-Portable declares `3.12...4.0`, so it is fine |
| Emscripten | **6.0.12** (2026-10-08). The 6.x tools are `.exe` launchers. Minimum browsers: Chrome 85, Firefox 79, Safari 14.1 | `git clone https://github.com/emscripten-core/emsdk.git C:\tools\emsdk && cd /d C:\tools\emsdk && emsdk install 6.0.12 && emsdk activate 6.0.12 && emsdk_env.bat` |
| Native compiler, primary | **MSYS2 UCRT64**: SDL2 2.32.10-1 and the rest. PvZ-Portable supports MinGW (`BUILD_STATIC`), and its vcpkg triplets also allow MSVC | installer from https://www.msys2.org/, then `pacman -S --needed make git mingw-w64-ucrt-x86_64-{toolchain,cmake,ninja,SDL2,zlib,libpng,libjpeg-turbo,libopenmpt}`. The package name `libopenmpt` is **UNVERIFIED** (check with `pacman -Ss openmpt`). Follow PvZ-Portable's README for the exact dependencies of its vendored SDL-Mixer-X |
| Native compiler, alternative | llvm-mingw 20261006 (LLVM 23.1.3) with the SDL2 2.32.10 / SDL2_image 2.8.12 / SDL2_mixer 2.8.2 MinGW dev zips | https://github.com/mstorsjo/llvm-mingw/releases/download/20261006/llvm-mingw-20261006-ucrt-x86_64.zip; https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-devel-2.32.10-mingw.zip (plus SDL_image and SDL_mixer). The MinGW mixer zip has no libxmp.dll, which doesn't matter because we use libopenmpt |
| libopenmpt | 0.8.9 (2026-08-19), BSD-3. Needs Emscripten ≥3.1.51 | built by `wasm/build-wasm.sh --deps`. Run it from an MSYS2 bash with emsdk activated (it needs GNU make), or in Docker. The image tag `emscripten/emsdk:6.0.12` is **UNVERIFIED**. Prebuilt alternative: https://lib.openmpt.org/libopenmpt/download/ |
| Libraries | nlohmann/json v3.12.0 (MIT; we build with `-fexceptions`, so exceptions are fine). Lua 5.4.9 (MIT, https://www.lua.org/ftp/), vendored and compiled as C on both targets. 5.5.1 is newer, but whether binding libraries support 5.5 is **UNVERIFIED**. stb_image v2.30 is only a fallback. fflate 0.8.3 (MIT) or a pinned JSZip for web ZIP import | |
| CI | GitHub Actions; PvZ-Portable uses `emscripten-core/setup-emsdk@v16` | |

---

## 4. Decompilation procedure (steps P0-P2)

### P0 — Extract the game image, reproducibly
<!-- private: - Copy the game image out of WinBej.exe with a private tool (not published), checking both files against their SHA-256, into `re/bin/inner.exe`. -->
- Write `tools/extract_inner.py`. It should:
  1. Check WinBej.exe against SHA-256 `a16fa32d…8376`.
  2. Slice `[0x17b1a2 : 0x17b1a2+0x15aa86]`.
  3. Check the slice against `6257a77f…466e`.
  4. Write it to `re/bin/inner.exe`.
<!-- /private -->
- `re/bin/` is git-ignored. The working copy already exists at `…\scratchpad\bin\inner.exe`.
- Never commit WinBej.exe, inner.exe, bass.dll or any asset.

### P1 — Ghidra analysis
1. **Import and analyse headlessly.** "Windows x86 PE RTTI Analyzer" and "Demangler Microsoft" are on by default.
   ```bat
   C:\tools\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat C:\re\proj Bejeweled -import C:\re\bin\inner.exe -overwrite -max-cpu 16 ^
     -scriptPath C:\re\ghidra_scripts -postScript RecoverClassesFromRTTIScript.java -log C:\re\logs\headless.log -scriptlog C:\re\logs\scripts.log
   ```
   - `RecoverClassesFromRTTIScript` (a PROTOTYPE script) recovers vtables, constructors, destructors and class structs for all 89 types.
   - After editing class structs, run `ApplyClassFunctionSignatureUpdatesScript` and `ApplyClassFunctionDefinitionUpdatesScript`.
2. **Label the CRT and libraries with Function ID.**
   - Apply the bundled `vsOlder_x86.fidbf` (VS2005 coverage **UNVERIFIED**).
   - Then **build our own FidDb**: compile PeggleCommunity/PopCapFramework `main` (1.3) plus its bundled zlib, libpng, jpeg and Tremor with MSVC 8.0 SP1, Release `/O2`. The exact original flags are unknown, so try a few variants. Then run *Tools → Function ID → Populate FidDb from programs*.
   - Use **BSim** (built into Ghidra) as a fuzzy-match fallback.
   - Expect most of the about 1 MB `.text` to be framework, CRT and libraries. The game code is what's left over.
3. **Anchor the game code** (addresses from the game note):
   - `data\diff.txt` loader → WinDMApp at 0x4d82d2.
   - Sound table at 0x5362e0 (21 entries).
   - Board level-start at 0x4cae29 and `Board::Update` at 0x4cdb3b.
   - Scoring at 0x4cd390–0x4cd659 (jump table 0x4cd667).
   - Hint at 0x4cad2f, gem spawn at 0x4cab3d.
   - Legacy ImageFont constructor at 0x42bfa0 (called from 0x4d8531 and 0x4d8590).
   - Game code found so far clusters in **0x4ca000–0x4d9000**; confirm the full extent from the game classes' vtables.
   - Map each vtable slot to SAF 1.3 `Widget`/`Dialog` virtual methods in header order.
4. **Recover types.** Define the `Board` struct from the known offsets:

   | Offset | Field |
   |---|---|
   | +0x8c | level, 0-based |
   | +0x94 | 8×8 gem pointers |
   | +0x194 | state |
   | +0x198 | Time Trial flag |
   | +0x1d8 | drawn bar |
   | +0x1ec | score |
   | +0x1f0 | multiplier |
   | +0x1fc | gems cleared |
   | +0x200 | set counter |
   | +0x204 | cascade depth |
   | +0x210 | longest cascade |
   | +0x214 | best move |
   | +0x220 | unknown |
   | +0x270 | level size |
   | +0x278 | drained |
   | +0x280 | drain rate |
   | +0x288 | drain acceleration |
   | +0x304 | freeze counter |

   - On the app object, +0x728..+0x780 hold the diff.txt values.
   - Get the base-class (Widget) size by compiling SAF 1.3 with VC8 and printing `sizeof(Widget)`, or by reading the constructors.
5. **Export.** Use the per-function `ExportDecompiledC.java` from the tooling note. Its API calls were checked against the 12.1.4 source, but it has **never been compiled or run**. The alternative is the built-in `CppExporter`.
   - Write output to `re/decomp/` (private).
   - Re-export without re-analysing: `-process inner.exe -noanalysis -postScript ExportDecompiledC.java <dir>`.
6. **Bookkeeping.**
   - `re/symbols.csv`: address, name, class, status.
   - Patoke-style `// 0x4cae29` comment above every reimplemented function.
   - Optional: a shared Ghidra server, as Thunderball does.

### P2 — Reimplementation order
1. WinDMApp: init, image and font load list, sound table, diff.txt, registry settings, save/continue.
2. TitleScreen and OptionsDialog, plus music.
3. Board in Normal mode.
4. Board in Time Trial mode.
5. HighScoreWidget, HighScoreDialog and RecordScoreDialog.

Rules while doing it:
- Keep vanilla behaviour exact, including bugs.
- Put deliberate fixes behind `BEJ_FIX_BUGS`, following the `DO_FIX_BUGS` / `FIX_BUGS` precedent from PvZ-Portable and re3.
- Reimplement **MSVC's `rand`** exactly, with a per-thread seed (VC8's CRT is always multithreaded, initial seed 1):
  ```cpp
  holdrand = holdrand*214013 + 2531011; return (holdrand >> 16) & 0x7FFF;
  ```
- `_ftol` truncates toward zero, so a plain `(int)` cast matches it.

---

## 5. Port architecture

### Repository layout (private repo)
```
bejeweled-port/
  CMakeLists.txt
  extern/sexy/          PvZ-Portable src/SexyAppFramework @ pinned commit (tag 0.2.5, 2026-10-10) + SDL-Mixer-X submodule
  src/game/             WinDMApp, Board, TitleScreen, OptionsDialog, HighScoreDialog, HighScoreWidget, RecordScoreDialog
  src/compat/           MsvcRand, Win32/registry key mapping, SysFont shim, stubs (Register/UpdateCheck/Beta/Internet)
  src/mod/              Vfs, ModManifest, Tunables, Layout, ImageScale, LuaHost (optional)
  data/port/            port-owned defaults: tunables.json, layout.json (vanilla values)
  web/                  shell.html, importer.js (folder/ZIP → MEMFS/IDBFS), mods UI
  re/                   ghidra_scripts/, symbols.csv, notes  (no binaries; re/bin, re/decomp git-ignored)
  tools/                extract_inner.py, verify_assets.py, demo/state-diff tools
  tests/                unit (no assets) + golden (needs BEJ_GAME_DIR)
```

### Class plan

| Class | Plan |
|---|---|
| WinDMApp, Board, TitleScreen, OptionsDialog, HighScoreDialog, HighScoreWidget, RecordScoreDialog | **Reimplement** from the decompilation |
| RegisterDialog, UpdateCheckDialog, BetaSupport, InternetManager, HTTPTransfer | **Stub or remove**: no registration, update checks or network. Nag and trial paths compiled out |
| SexyAppBase/SexyApp, Widget/WidgetContainer/WidgetManager, ButtonWidget, Checkbox, Slider, Dialog, DialogButton, EditWidget, HyperlinkWidget, ScrollbarWidget, ListWidget, Graphics, Image, MemoryImage, SharedImage, ImageFont/FontData, ImageLib, PakInterface, XMLParser, PropertiesParser, DescParser, Buffer, ResourceManager, MTRand | **Reuse** from the PvZ-Portable fork, keeping namespace `Sexy` and the original names |
| DDInterface, DDImage, D3DInterface, D3DTester, NativeDisplay | Already replaced in PvZ-Portable by GLInterface/GLImage (SDL2 + GLES2, GL 2.1 fallback, optional ANGLE) |
| BassMusicInterface, FModMusicInterface | Replaced by SDLMusicInterface. Add an adapter if game calls rely on BASS-specific behaviour |
| DSoundManager, DSoundInstance | Replaced by SDLSoundManager/SDLSoundInstance |
| SysFont | Shim **only if** open item 3 shows game code uses it. Options: an ImageFont fallback, or stb_truetype / SDL2_ttf with a bundled OFL font |
| Outer-only: DRMApp, LuaApp, PopLua, Lua*Widget, OggDecoder, PNGDecoder, CircularBuffer, WinInetHTTPTransfer, SteamStartup | **Not ported** |

### Changes to the forked framework
1. **VFS hook.** Add a VFS in `PakInterface::FOpen`/`FindFirst`, consulted *before* pak records. Stock SAF checks pak records first and then `fopen` (source/PakLib/PakInterface.cpp in https://github.com/LolHacksRule/sexy). Also add `Vfs::Resolve()` for code that opens paths directly. `SDLMusicInterface::LoadMusic` calls `Mix_LoadMUS(path)`, so switch it to `Mix_LoadMUS_RW` over VFS bytes.
2. **Path normalisation.** Accept `\`, ignore case (PvZ-Portable's `fcaseopen` exists), strip a leading `./`.
3. **Image probe order.** Restore SAF 1.3's order (tga, jpg, png, gif) and keep the alpha-mask rules:
   - look for `_name` first, then `name_`;
   - the mask's blue/low byte becomes alpha;
   - a mask with no colour image becomes `gAlphaComposeColor` 0xFFFFFF with that alpha.
4. **Redraw every frame.** After `GLInterface::Flush`, the GL back buffer is undefined, so call `mWidgetManager->MarkAllDirty()` every frame. SAF 1.3 only redrew dirty widgets, and Bejeweled code may rely on that.
5. **Registry.** Map registry keys to `RegEmu`, stored in a file under the save directory.
   - `GetAppDataFolder()` → native `%APPDATA%\<port-name>\`, web `/saves`.
   - Keep the original `userdata\hiscores.dat` and `savegame.dat` names and formats so saves can be imported.
6. **Hyperlinks** (HyperlinkWidget, `ShellExecute`) → `SDL_OpenURL`, or stub them.
7. **HD texture scaling** (section 7.4).
8. **Hot reload** in dev builds: re-decode every SharedImage and font, and reload tunables, on F5.

### Main loop
- Keep SAF's fixed 100 Hz update (`mFrameTime = 10`, pending open item 1), with draws decoupled from updates.
- Web: PvZ-Portable's `emscripten_set_main_loop(EmscriptenMainLoopCallback, 0, 1)` runs every 10 ms update that is due, then one draw, per animation frame.
- Keep `-sASYNCIFY` until open item 4 is answered. If there are no blocking `WaitForResult` calls, drop it for a smaller, faster build.
- Rendering:
  - The logical canvas is 640×480. GLInterface letterboxes to 4:3 at the drawable's resolution.
  - Offer nearest-neighbour vs linear filtering, plus integer scaling as an option.

### Licensing

**Framework: PopCap Games Framework License v1.1** (`PopCap Framework License.md` in the SAF repos).
- Ship the full license text.
- Include this acknowledgment in the docs or in-game: "This product includes portions of the PopCap Games Framework, © 2005-2009 PopCap Games, Inc. All rights reserved. (http://popcapframework.sourceforge.net/)"
- Do not use "PopCap" in the product name or to promote the product.
- "Bejeweled" is an EA/PopCap trademark as well, so choose a neutral project name.

**PvZ-Portable fork: LGPL-3.0-or-later AND LicenseRef-PopCap** (per-file SPDX headers).
- For a statically linked WASM or exe build, comply by publishing the modified framework source with relinkable objects, or simply the whole source.
- Its README rejects "code generated directly through reverse engineering", so all Bejeweled work stays in our own repo and is never sent upstream.

**Projects whose code to avoid:**
- PopLib (https://github.com/teampopwork/PopLib): AGPL-3.0, archived, renamed API.
- insaniquarium-port and Insaniquarium-Enhanced: AGPL.
- LDinos clone: no licence.
- Can be borrowed with attribution: Patoke (CC0) and the MIT projects alula, Thunderball and WinFish.

**Third-party libraries:**
- SDL2: zlib. libopenmpt: BSD-3. Lua and nlohmann/json: MIT. stb: public domain / MIT.
- SDL-Mixer-X: core is zlib; the licenses of the backends we enable are **UNVERIFIED**, so audit the enabled ones.
- BASS and FMOD are commercial, have no WASM build, and are not used.

**The game logic itself is copyrighted** by PopCap/EA. Reconstructed code is a derivative work and comes with no grant of rights.
- PopCap decomps (PvZ, Zuma, Peggle, Insaniquarium) are public today, but that is not permission.
- Take-Two DMCA'd re3/reVC and sued 14 contributors (https://videogameschronicle.com/news/take-two-is-suing-the-creators-of-a-gta-3-and-vice-city-reverse-engineering-projects).
- Recommendation: a **private repo**, no assets, no original binaries. Public release is the user's decision.
- Nothing here breaks or bypasses the Steam/PopCap DRM: the port simply doesn't include that wrapper, as in alula's Zuma. This is not legal advice.

---

## 6. Audio plan

**What the original uses.**
- Music: BassMusicInterface through BASS 2.3. The binary contains BASS_MusicLoad, BASS_MusicGetOrders, BASS_MusicGetOrderPosition and BASS_ChannelPlay.
- Sound effects: DSoundManager. `.ogg` goes through Tremor in SAF 1.3, and `.mp3` copies exist too.
- WINMM `mixer*` functions are imported, apparently for volume control. Ignore them in the port.

**Port: one stack on every platform.**
- **SFX:** `SDLSoundManager` loads `.ogg` first and falls back to `.mp3`. That covers the 21-entry sound table: gotset2, gotsetbig2, combo22–52, bad2, gemongem2, explode2, click2, select, menuclick, multishot, go1, levelup1, gameover1, firecrackle, warning1, whirlpool1, excellent1.
  - Keep the original's instance semantics: whirlpool1 looks like it loops, and warning1 repeats every `max(80, frac×1600)` ticks.
- **Music:** the three `.s3m` modules (Skaven) play through **SDL-Mixer-X + libopenmpt** on native and WASM.
  - Map `MusicInterface::PlayMusic(id, offset, noLoop)` order offsets to `Mix_ModMusicStreamJumpToOrder`, and positions to `Mix_ModMusicStreamGetOrder`.
  - Keep SAF fade-in/fade-out, driven from `MusicInterface::Update()` inside `UpdateFrames`.
  - Expose libopenmpt's interpolation setting (`render.interpolationfilter_length`) so it can be tuned against BASS 2.3 by ear.
- **Avoid mixing players.** Emscripten's built-in `sdl2_mixer` port (SDL2_mixer release-2.8.0, `mod` → libmodplug) is a zero-build fallback only. It lacks the order-jump API (only `Mix_SetMusicPosition`) and would sound different from native.
- **Browsers:** the AudioContext has to start from a user gesture. The "pick your game folder" click is one; also resume on the first input. Whether SDL2's Emscripten audio backend resumes the context automatically is **UNVERIFIED**.
- **Fallback if libopenmpt for Emscripten becomes a problem:** convert at build time, as Zuma-TAS's `tools/build_assets.py` does (https://github.com/9933fa/Zuma-Deluxe-TAS): openmpt123 → ogg, run locally on the user's files. Order-jump semantics are then lost.

---

## 7. Assets and mod system

### 7.1 Asset handling (never redistribute)

**Native**
- Look for the game data in this order:
  1. `--game-dir`;
  2. the exe's own directory;
  3. Steam auto-detection: `HKCU\Software\Valve\Steam\SteamPath` + `steamapps\libraryfolders.vdf` → `steamapps\common\Bejeweled Deluxe` (app 3350, https://store.steampowered.com/app/3350/).
- The game directory is opened read-only and never written to.

**Web**
- First run shows "Select your Bejeweled Deluxe folder" (`<input type=file webkitdirectory multiple>`), or "Select a ZIP" (fflate, or a *pinned* JSZip).
- Copy only an allowlist: `images/`, `data/`, `sounds/`, `music/`, `properties/default.xml` (and optionally `partner.xml`).
  - Never copy `WinBej.exe`, `bass.dll`, `Flash.ocx`, `drm*`.
- Write files into MEMFS `/game` (`FS.mkdirTree` + `FS.writeFile`). With the user's opt-in "remember", persist them in IDBFS (about 1.6 MB).
  - Offer a "Forget game files" button.
  - Chromium-only extra: the File System Access API can re-read a remembered folder.
- Validate with `verify_assets.py` / JS: required-file list plus SHA-256 of `data/diff.txt` and `images/gemsheet6.png` (section 0). A mismatch warns rather than blocks.
- Nothing is uploaded. The hosted site contains only `.wasm`, `.js` and `.html`, the PvZ-Portable model.
- Saves:
  - IDBFS mounted at `/saves`, either `FS.mount(IDBFS,{autoPersist:true},'/saves')` (Emscripten ≥6.0.0) or `FS.syncfs(false)` after each write.
  - ZIP export/import of saves.
  - Optional import of original saves from `%PROGRAMDATA%\Steam\Bejeweled\userdata\` (https://www.pcgamingwiki.com/wiki/Bejeweled), once the formats are reverse-engineered.
- Emscripten docs: https://emscripten.org/docs/api_reference/Filesystem-API.html

**Repo hygiene:** git-ignore all game files. CI builds with no assets. Golden tests run only when `BEJ_GAME_DIR` is set.

### 7.2 Overlay VFS

Layers, from highest to lowest priority:
1. dev override directory (`--override-dir`; on web, a "live folder");
2. enabled mods in the user's load order;
3. port defaults (`data/port/`);
4. base game (`/game`, or the install directory).

Resolution rules:
- **Logical paths:** lower-case, `/`-separated, no leading `./`. Backslashes coming from game code are normalised.
- **Extensionless image loads** (the SAF convention): pick the **highest layer that has any colour file with that stem**, using the SAF 1.3 probe order inside that layer.
- **Alpha masks:** look up `_name` and `name_` **in that same layer only**. Without this rule, a mod's RGBA `gemsheet6.png` would receive the base game's `_gemsheet6.gif` mask. If the mod image has embedded alpha and no mask, keep its alpha.
- **Fonts:** `data/*.txt` descriptors resolve `LayerSetImage` relative to the descriptor's directory. Both formats stay supported:
  - script format (`Define`/`CreateLayer`/`LayerSet*`/`SetCharMap`);
  - legacy format (`<spaceWidth> <ascent>` followed by `<char> <width>` pairs).
- **Mod archives:** a mod is a folder or a `.zip`. Natively, mount zips read-only with minizip/zlib. On the web, unzip into `/mods/<id>/` and persist in IDBFS.

### 7.3 Manifest (`mods/<id>/mod.json`, parsed with nlohmann/json)
```json
{ "id": "hd-gems", "name": "HD Gems", "version": "1.0.0", "authors": ["…"],
  "targets": "bejeweled-deluxe-1.87", "priority": 100, "loadAfter": [],
  "images":   { "images/gemsheet6": { "scale": 2 } },
  "tunables": "tunables.json", "layout": "layout.json",
  "strings":  ["properties/strings.xml"], "scripts": ["main.lua"],
  "affectsScores": false }
```
- The enabled list and load order live in `/saves/mods.json`.
- UI: a "Mods" dialog in Options, plus `--mod <dir>` / `--mods a,b` on native.
- Mods that set `affectsScores: true`, or that change gameplay tunables, record high scores in a **separate table keyed by the mod-set hash**.

### 7.4 Graphics overrides (the user's main goal)
- **Same-size replacement:** drop in files under `images/`. This works from milestone M1 onward.
- **HD packs** (`"scale": N`):
  - The loader keeps the full-resolution texture but reports the *logical* `mWidth/mHeight = physical / N`.
  - GLImage computes UVs from logical source rectangles, so the hard-coded rectangles keep working.
  - Example: `checkertextures2.png` uses 51×51 cells at a 102 px step (5 light/dark pairs). `blackhole.gif` is probably 9 frames of 256×256 (inferred).
  - It is rendered at window resolution thanks to GLInterface's drawable-resolution letterbox (checked).
  - **Caveat:** CPU-side MemoryImage paths assume logical size equals pixel size: `Palletize` for fonts, colourise, `GetBits` readback. Those paths need scale-aware code and tests.
- **Layout data:** move every hard-coded sprite-sheet geometry and UI position found during reverse engineering into `data/port/layout.json` (vanilla defaults, overridable). Examples: gem cell size and frame counts, checker texture count (`L mod 5`), black-hole frames, timer-bar width 340 px (`timerbar.png` is 342×25), drawn-bar speed of 2 px per update.

### 7.5 Tunables and data
- **`data/diff.txt`** stays the source of the level curve and can be overridden as a file. Its lines (verified in code by the game note):
  - Normal level size: `need = (500 + 150·L) × mult`
  - Normal multiplier: `mult = 1.0 + 0.5·L`
  - Time Trial range: `range = (250 + 10·L) × multTT`
  - Time Trial multiplier: `multTT = 2.0 + 0.5·L`
  - Drain rate: `rate = (1.0 + 2.0·L) × mult`
  - Drain acceleration: `accel = (0.01 + 0.005·L) × mult`
- **`tunables.json`** exposes the remaining constants with vanilla defaults and validated ranges:
  - set base values `[10,20,30,40,60,80,110,160,210]`;
  - length bonus thresholds 3 and 5;
  - EXCELLENT threshold `120×mult`;
  - hint penalty `50×mult`;
  - gem types 7 (capped by the sprite-sheet rows);
  - freeze ticks 300/200;
  - Time Trial warning threshold 15%, with repeat bounds 80/1600 ticks.
- The board stays 8×8 in version 1, because it is a fixed array in the original.
- **Strings:** `properties/*.xml` from mods are merged by `id` on top of `default.xml` (PropertiesParser format). `partner.xml.sig` is ignored.

### 7.6 Optional Lua (phase 3)
- Lua 5.4.9, vendored and identical on native and WASM. Sandboxed: no `io`, `os` or `package` loaders.
- Hooks run only inside the fixed 100 Hz `Update`, which keeps replays deterministic:
  - `onLevelStart(level, mode)`
  - `onSetScored(info) → points`
  - `onGemSpawn(x, y, type) → type`
  - `onDraw(g)` (overlay images and text)
  - get/set for tunables
- Any mod with a script is flagged `affectsScores`.
- Precedents: isle-portable "extensions", sm64ex texture packs, PvZ-Portable-Mods (https://github.com/CypressVillage/PvZ-Portable-Mods), haggle's DLL loader (https://github.com/PeggleCommunity/haggle).

---

## 8. Verification strategy

1. **Unit tests (no assets).** Formulas and tables from section 7.5 against the reverse-engineered values:
   - Normal level sizes, levels 1-4: **500, 975, 1600, 2375**. This matches wiki.gg's `(50+15n)(10+5n)`, https://bejeweled.wiki.gg/wiki/Level_Bar.
   - Time Trial ranges: **500, 650, 810**; drain rates 2, 7.5, 15 pts/s.
   - Scoring table and length bonus (4 gems +10, 5 → +20, 6 → +40, 7 → +60).
   - Combo sound `min(depth−1,4)+2`; hint `score −= trunc(50×mult)` with a floor of 0.
   - Time Trial bar `170 + (progress·170)/range`; TIME_UP when `progress ≤ −range`.
   - MSVC `rand` sequence from seed 1.
2. **Deterministic record and replay in the port.** Record input, the RNG seed and the tick number, PvZ-Portable `.dmo` / Zuma-TAS style. Golden replays assert final score, level, statistics and per-tick Board state hashes.
3. **Differential testing against the original.** Optional, and depends on open items 2, 6 and 7.
   - Use the original's built-in SAF recorder (`-record` / `-demofile=`) if the Steam wrapper passes arguments through and CRT seeding is deterministic.
   - Otherwise, attach x32dbg to the running game launched by Steam (version not researched). Log the `srand` seed and dump Board fields at the offsets in section 4 P1 each tick; then compare the port's dump.
   - Check the x87 control word with 3D acceleration on and off (open item 6).
   - Pixel diffs are not used at this step; they are item 4 below.
4. **Screenshot comparisons.** Same seed and inputs, 640×480 captures of the title screen, a board, level-up and the high-score screens from both. Compare with a perceptual tolerance, since D3D7/DirectDraw differ from GLES.
5. **Assets and mods.**
   - Every vanilla image loads with the correct alpha (masks, `.PNG`/`.GIF` upper-case names).
   - All 5 fonts measure strings identically: legacy glyph widths sum to the image widths 150, 249 and 1403 px.
   - All 3 S3M files play and loop.
   - VFS tests: precedence, same-layer masks, case and backslash handling, zip mounts, HD-scale rendering.
6. **Optional per-function matching.** reccmp with MSVC 8.0 SP1 on Board's translation units, Thunderball style. It is only worth doing where behaviour is disputed.
7. **CI:** MSYS2 native build plus `setup-emsdk@v16` WASM build, and unit tests, all with no assets.
8. **References for behaviour checks:**
   - readme v1.87, archived at https://web.archive.org/web/20100612032438/http://popcap.com/faq/bejeweled/1033/pc/readme.html
   - https://strategywiki.org/wiki/Bejeweled
   - https://bejeweled.wiki.gg/wiki/%22The_Voice%22
   - https://bejeweled.wiki.gg/wiki/Hint_button
   - https://bejeweled.wiki.gg/wiki/Diamond_Mine_(game)
   - https://www.speedrun.com/bejeweled1
   - https://en.wikipedia.org/wiki/Bejeweled_(video_game)
   - The code is authoritative wherever these disagree with it.

---

## 9. Milestones

| Milestone | Work | Done when |
|---|---|---|
| **M0** | Tools; `extract_inner.py`; Ghidra project with RTTI classes; Function ID (bundled plus our VC8 SAF 1.3 database); `symbols.csv`; resolve open items 1–5 | Framework functions labelled; game functions listed |
| **M1** | Fork PvZ-Portable's framework into `extern/sexy`; VFS; probe-order and mask rules; native and WASM "asset viewer" app that loads the user's folder and shows every image and font and plays every sound and module | Proves the rendering, alpha, fonts, S3M-in-browser and import pipeline before any game code exists |
| **M2** | WinDMApp + TitleScreen + OptionsDialog; RegEmu settings; music | Title flow works on both targets |
| **M3** | Board, Normal mode | Unit and golden replay tests pass |
| **M4** | Time Trial, hints, no-moves, high scores, save/continue; formats compatible with the original | |
| **M5** | Mods: manifests, tunables, layout.json, HD scale, hot reload, Mods dialog, separate score tables | |
| **M6** | Web polish (importer, IDBFS remember/forget, mod ZIP import, save export); optional Lua; optional differential testing against the original | |
| **9.6 (optional, not on the critical path)** | Overrides for the *original* game: it already reads loose files, so replacing files works today. A non-destructive overlay could be a proxy `bass.dll` (the inner game does `LoadLibrary(".\bass.dll")`) built with i686 llvm-mingw + MinHook v1.3.4 (BSD-2), redirecting file opens to a `mods\` folder (haggle style) | Whether the DRM wrapper tolerates a proxy `bass.dll` is **UNVERIFIED** |

---

## 10. Remaining sources from the notes (not cited above)

**Framework source**
- https://www.gamedeveloper.com/pc/popcap-games-releases-open-source-framework (2005-03-09 release)
- https://web.archive.org/web/20070216100105/http://www.mechanism8.com/popcap/docs/html/_sexy_app_8cpp-source.html
- Variants:
  - https://github.com/sandboxmo-de/SexyAppFramework-v1.2 (plus sandboxmo-de/msvc70_saf_builder)
  - https://github.com/LolHacksRule/popcap (OSFramework)
  - https://github.com/sureandrew/SexyAppFramework (game note line references: SexyAppBase.cpp line 186, ImageFont.cpp lines 246-940 and 1026-1070, ImageLib.cpp lines 1554-1660)
  - https://github.com/AdrUlb/SexyAppFramework
  - https://github.com/teampopwork/SexyApp-Delta
  - https://github.com/keestux/tuxcap
  - https://codeberg.org/midzer/PvZ-x64

**Other projects**
- PvZ family: lvplay2/PvZ-Decomp, person6423/pvz-vanilla-recon, dnartz/PvZ-Emulator
- Zuma: GalaxyShad/Zuma-Deluxe-HD
- Insaniquarium: wefalltomorrow/Insaniquarium-Enhanced, PopCap-Decompilations/Insaniquarium-Decompilation
- Other PopCap titles: TC999/Chuzzle-Deluxe, PopCap-Decompilations/PizzaFrenzy-Decompilation, Chillaxhson/heavy-weapon-deluxe-vita, ProjectReversio/BejeweledBlitzPC
- https://github.com/JacksonDam/PopSilicon (runs PopCap Mac builds, a cross-reference source)
- devilutionX / d07RiV/diabloweb

**Tooling**
- Ghidra:
  - https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_12.1.4_build
  - https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_12.1.4_build/GhidraDocs/GettingStarted.md
  - https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_12.1.4_build/Ghidra/Configurations/Public_Release/src/global/docs/WhatsNew.md
  - https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_12.1.4_build/Ghidra/Features/PyGhidra/src/main/py/README.md
  - https://github.com/NationalSecurityAgency/ghidra-data/tree/master/FunctionID
  - https://github.com/threatrack/ghidra-fidb-repo
- JDK: https://api.adoptium.net/v3/assets/latest/21/hotspot?os=windows&architecture=x64&image_type=jdk
- Emscripten:
  - https://github.com/emscripten-core/emscripten/blob/main/ChangeLog.md
  - https://github.com/emscripten-core/emscripten/tree/6.0.12/tools/ports
- Toolchains and packages:
  - https://github.com/mstorsjo/llvm-mingw/releases/tag/20261006
  - https://packages.msys2.org
- Libraries:
  - https://github.com/libxmp/libxmp (4.7.4, MIT)
  - https://github.com/martincameron/micromod (ibxm/micromod, BSD-3)
  - https://github.com/nothings/stb