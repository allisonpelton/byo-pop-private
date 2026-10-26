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

## Port progress

Built and checked (`wordharmony_enginetest` parses all 52 XML files of the archive, 7366 elements): `engine/Object.h`
(Interface, Object, Ref), `Exception.h`, `Geometry.h`, `Graphics.h/.cpp` (render states; the image draw is pure, the
host's subclass implements it), `Font.h/.cpp`, `Xml.h/.cpp` (the parser 0x43b1f0 translated state by state, the
attribute getters), `Stream.h/.cpp` (memory streams: the host reads whole files), `ResourceManager.h/.cpp` (the
archive loader without the MD5 checks, the resource manager), `Host.h` (what the Windows layer got from Windows),
`Node.h/.cpp` (Node, Container, Window: every slot of the three vtables) and `App.h/.cpp` (the caches, the node map,
the image loader's mask rule, the frame 0x4542e0). The engine test: `TARGET=wordharmony_enginetest bash tools/build.sh
native`, then `build/native/rundesk.exe bash -c "build/native/wordharmony_enginetest.exe assets/wordharmony"`.

Next: the game's controller (created by 0x407370, 0x71c bytes, constructor 0x406980; its slots: 4 configure the
AppConfig at app+0x88, 8 init, 0xc shutdown, 0x10 update), then the screen layout (0x42c4f0-0x431000) and the
widgets it makes, the image node (0x435d50), TextItem, buttons, particles, sound and music, the host app in src/.
The run loop 0x4552b0: frame period 1000 / fps (fps = config +0x90); the root window is the Windows window class
(vtable 0x48a528, a Window subclass that owns the HWND). The App's slots: 0x0c is 0x427000, 0x64 a stopwatch in
seconds (+0xb8), 0x6c and 0x70 not yet read. The image loader (0x457510) reads a PNG or JPEG and, when
`<dir>_<name>.png` exists, takes its alpha from that mask (0x45ea10; which channel is not yet checked).

Node vtable (0x484b50, 27 slots), by offset: 0 dtor, 4 getX, 8 getY, 0xc getPosition, 0x10 getAbsPosition, 0x14
setPosition(Point), 0x18 setPosition(x, y), 0x1c setScale, 0x20 setRotation, 0x24 getBounds, 0x28 setOpacity, 0x2c
getOpacity, 0x30 setFont, 0x34 setName, 0x38 getName, 0x3c applyTransform(Graphics), 0x40 contains(Point), 0x44
hitTest(Point), 0x48 hitTest(flags, Point), 0x4c toLocal, 0x50 parent's toLocal, 0x54 setVisible, 0x58 isVisible,
0x5c setEnabled, 0x60 isEnabled, 0x64 and 0x68 empty (start and stop: containers forward them to the children).
Container (0x481ca8) and Window (0x484a08) add: 0x6c draw, 0x70 update(UpdateInfo: origin Point, then the seconds
elapsed at +8, clamped to 0.33), 0x74 layout, 0x78 dump, 0x7c/0x80 mouse down (public, protected), 0x84/0x88 up,
0x8c/0x90 right down, 0x94/0x98 right up, 0x9c mouse move, 0xa0/0xa4 enter, 0xa8/0xac exit, 0xb0 setParent, 0xb4
clearParent, 0xb8 getParent, 0xbc setFlags (propagates up), 0xc0 clearFlags, 0xc4 notify listeners, 0xc8 removeChild,
0xcc setClipRect, 0xd0 addChild, 0xd4 removeAllChildren. Flags: 1 drawable, 2 takes the mouse, 8 needs layout, 0x10
remove me, 0x14 needs update. The root window's mouse events (0x4387f0...) hit-test with flag 2, update the hover
node (0x4386c0: exit on the old, enter on the new) and call the hit node's handler; key events go to the handlers
in +0xb0, last first, until one returns true.

## Reading the decompilation

Two scratch helpers (not in the repository) print functions: `fn.py <addr>...` (whole functions) and `fr.py <lo> <hi>`
(a range, compacted: no declarations or SEH bookkeeping), `vt.py <vtable>...` prints vtables side by side, and
`dis.sh <start> <end>` disassembles with llvm-objdump (E:/retools/llvm-mingw-*/bin) where Ghidra loses x87 float
code. They
live in the session's scratchpad; any copy works on `re/wordharmony/gh/all.c`.

Common helpers: 0x4024a0 string assign(const char*, n); 0x4023b0 string assign(string, pos, n); 0x436480 string
destructor; 0x4019b0 string compare(pos, n, const char*, n2); 0x435f20 App::instance; 0x401000 an empty function
(inlined destructors); 0x44648d `__purecall`; 0x445fa8 operator new.
