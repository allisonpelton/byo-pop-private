# Feeding Frenzy 2: working notes

The session's own notes while porting: the plan, where the work stands, and what the export taught that is not yet in
code. `GAME_NOTES.md` will hold the finished port's description.

## Sources

- The game's files: `python -I games/feedingfrenzy2/tools/stage_feedingfrenzy2_assets.py` copies `FF2.saf`, `music/`,
  `splash/` and `properties/` from the Steam copy into `assets/feedingfrenzy2/`.
- The exe: `re/feedingfrenzy2/bin/FeedingFrenzy2Game.exe` (main checkout, git-ignored), the game PE carved out of
  the Steam wrapper; linked 2006-02-24 with VS2005, full RTTI.
- The export: `re/feedingfrenzy2/gh/` (`all.c`, `functions.jsonl`, `vtables.jsonl`, `strings.jsonl`, `decomp/`).
- RTTI: `python -I tools/rtti_msvc70.py <exe> re/feedingfrenzy2/rtti/ff2 --functions re/feedingfrenzy2/gh/functions.jsonl`
  writes `ff2.classes.json` (581 classes, each with its bases and vtables). Talismania's, for the shared set, goes to
  `re/feedingfrenzy2/talismania/tal.classes.json`.

## Tools

- `tools/fn.py <addr>...`: a function, compacted (`--raw` as Ghidra wrote it, `--asm` disassembled by llvm-objdump,
  `--info` its callers, callees and strings, `--vt` a vtable read from the exe). Ghidra loses `this` in most
  `__EH_prolog3` functions (it shows up as `in_stack_00000004` or a shifted `param_1`), so read the disassembly
  wherever a body's first lines look wrong.
- `tools/classdump.py <class>`: a class's bases and vtables, each slot's function marked `=` where the primary base's
  own is kept, and the functions that store its vtables (constructors and destructors).

## The engine's scope

Sprout's second-generation engine goes in `engines/sprout2` (library `sprout2`), shared with Talismania. What is in
it: the 151 classes both games' RTTI names (namespaces `engine`, `dxdriver`, `simplesound`, `simplesounddx`, the
sigslot bases, and about 70 global classes such as `GenericApplication`, `Theme`, `ScreenLayout`, `SoundMgr`,
`BassMusicPlayer`, the particles, paths, splats and pixel combiners). What only Feeding Frenzy 2 has stays in
`games/feedingfrenzy2` (the `shape` namespace, `FadeContainer`, `MusicSystem`, `GenericOptionScreen`,
`GenericHighScoreScreen` among them) until Talismania's session finds it needs one.

## The object model

- `engine::Interface` is a virtual base with three slots (addRef, release, and a third that returns the count);
  `engine::Object` implements them over an interlocked count at +8 (a release to 0 calls the object's deleting
  destructor). An object is laid out as its own vtable at +0, the vbtable at +4, its fields, and the `Interface`
  part last.
- Every counted reference is added and released by hand in the original, through the vbtable.
- Ghidra's class recovery names many functions after the wrong class (`engine::Object::Object_Constructor_or_Destructor`
  at 0x438700 is the engine's frame), so a name in the export is a hint.

## The main loop

- `WinMain` 0x4a3960 makes the `dxdriver::EngineImpl` (0x4e8 bytes, 0x494e30) into the globals 0x5ac654 and 0x5ab9e4,
  runs 0x4383b0 and deletes it (slot 0x10c).
- 0x4383b0 is the engine's run: the window message (`Notify`), the single-instance mutex, the screensaver arguments
  (the strings at 0x56130c, 0x560564, 0x561320, 0x561324), the window, then a loop of PeekMessage and the frame
  (0x438700) whenever the clock (slot 0xe4) passes the next frame's time, `Sleep(0)` otherwise.
- The clock (0x438310) reads QueryPerformanceCounter, then switches to `rdtsc` (0x4c1e10) once calibrated; origrun
  cannot hook `rdtsc`, which matters for the identical-play runs.

## Startup

- The run (0x4383b0) reads the command line into the engine (+0x190), takes the game's name from the engine's slot 0,
  registers the "Notify" message and the single-instance mutex (a second copy posts Notify and leaves), and handles the
  screensaver letters `s` (run), `c` (configure), `p` and `l` (preview); a plain start is the `s` path without the
  screensaver's window style.
- 0x49bc60 makes the game's application (`FeedingFrenzyTwo`, 0x378 bytes, the factory 0x49e800) into the engine's
  +0x288 and has it fill the configuration at +0x1c0 (its slot 4, 0x48c080): the archive `FF2.saf` added with loose
  files off, the string table `resource/baseStrings.xml` (its `vendor` and `appName`), the options, the title
  "Feeding Frenzy 2 - Shipwreck Showdown", 60 frames a second and 800x600. A configuration without a name throws
  "Invalid configuration.". The application's slot 8 (0x408050) then starts the game.

## Images

- The surface loader (0x45f530) decodes `.jpg` with libjpeg 6b's defaults and `.png` with libpng; a file `_name.png`
  beside `name.jpg` or `name.png` is the alpha mask (slot 0x2c, 0x463580). Without a mask, a PNG whose colour type has
  an alpha channel is drawn blended (blend mode 2) and its colours premultiplied (0x4754b0, rounded by
  (t + (t >> 8)) >> 8 with t = c * a + 0x80); everything else is opaque (mode 0).
- The archive's PNG images are all 8-bit RGBA; its masks are grey (1, 8 and 16 bits), grey-valued RGB or palette, a
  palette with tRNS (`_infoframe`) and RGBA ones, whose alpha is the mask's alpha. libpng's rgb_to_gray weights
  (6968, 23434, 2366) give a grey pixel its own value, so its gamma tables never matter for the game's masks.

## Done

- `engines/sprout2`: objects and references, sigslot, streams, the archive and resource manager, the engine's
  file, XML, image and font slots, components and containers, the draw context's state stack and text, fonts,
  surfaces with their loader and masks, the XML parser and its parameters. `host/` has EngineImpl (the disk files,
  the log, the clock).
- `games/feedingfrenzy2/src`: the app skeleton (a window, the engine made, frames run on the clock; nothing drawn yet).
