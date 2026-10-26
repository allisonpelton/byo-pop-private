# Working notes (Word Harmony port)

The state of the work, for picking it up again (after a compaction or in a later session).

## Done

<!-- private: -->
The game exe is carved out of `WordHarmony.exe` (PopCap's DRM wrapper) with `python tools/extract_game_exe.py
<install>/WordHarmony.exe -o re/wordharmony/bin/WordHarmonyGame.exe` (671,744 bytes).
<!-- /private -->

The game exe `re/wordharmony/bin/WordHarmonyGame.exe` is analysed in the Ghidra project `wordharmony_game` (`E:/retools/projects`) with `CLASS_RECOVERY=0`, and exported to
`re/wordharmony/gh` (`all.c`, `functions.jsonl`, `strings.jsonl`). `tools/vtables.py` lists the vtables
(`re/wordharmony/vtables.txt`) and `tools/ghidra_pass.sh` created the virtual functions, gave the indirect calls their
argument counts and exported again. The archive is extracted for reading to `re/wordharmony/saf` (git-ignored).
`tools/stage_wordharmony_assets.py` stages `assets/wordharmony/` (WHArchive.saf and res/). ENGINE.md has what is
known of the engine.

The coordinator was told (2026-10-26) that the engine is Sprout's first generation and shared with Pizza Frenzy and
Iggle Pop; the plan is to keep it in `engine/`, free of game code, so it can move to shared code later.

## Plan

The engine runs one frame per call (no modal loops), so the port hosts it from a framework widget like Ning Po's
(`games/ningpo/src/NingPoApp.*`), without ASYNCIFY: the widget's Update runs the frame (0x4542e0), its Draw blits the
800x600 screen. `engine/` is the engine translated (namespace `engine`), mapped onto the framework and `platform/`
through a host interface; `src/` is the game. STL code in the binary (map, list, deque, vector, string internals)
is not translated: the port uses the standard containers.

Order: Object and Ref; the render context over the framework's MemoryImage (scaled, rotated, alpha and colour
blits); nodes, containers, windows; images and fonts; the XML parser (0x43b1f0, translate it: the data has quirks such
as a repeated attribute); the screen layout and its widgets; the archive and the resource manager; particles; sound
and the music layers; the application; then the game from 0x401000 up.

## Reading the decompilation

Two scratch helpers (not in the repository) print functions: `fn.py <addr>...` (whole functions) and `fr.py <lo> <hi>`
(a range, compacted: no declarations or SEH bookkeeping), and `vt.py <vtable>...` prints vtables side by side. They
live in the session's scratchpad; any copy works on `re/wordharmony/gh/all.c`.

Common helpers: 0x4024a0 string assign(const char*, n); 0x4023b0 string assign(string, pos, n); 0x436480 string
destructor; 0x4019b0 string compare(pos, n, const char*, n2); 0x435f20 App::instance; 0x401000 an empty function
(inlined destructors); 0x44648d `__purecall`; 0x445fa8 operator new.
