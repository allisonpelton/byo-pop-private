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

Screens, ported and checked (the engine test reads all 27 layouts of the archive into ScreenLayouts, 469 named nodes,
every font of the layouts loaded):
`ImageNode`, `FadeContainer` (0x429b70: groups and screens), `Selector` (0x426f10: one child shown, cross-fades),
`Button` and `ImageButton` (0x432600, 0x429c70: hover scaling, flashing, enter and press sounds, listeners told under
the button's action; a press is told on mouse down), `ScreenLayout` (0x428640: named nodes, passes a pressed action to
its ScreenListener, the controller's +0xc), `Layout` (the reader 0x42c7f0 with image, imageButton, group and include),
`Sound.h` (the two SoundSystem slots the buttons call; App::getSoundSystem is the global 0x49bfc0), `TextItem`
(0x427680: lines split at a literal backslash-n, aligned, underlined or struck through; setNumber puts commas in,
setTime gives h:mm:ss) and the text element 0x42ef20, `CheckBox` (0x483dc8: four images, a press flips it) and
`TextButton` (0x42c2c0: a text node whose font follows the mouse) with their elements 0x42d6f0 and 0x42e2b0, and
textSub 0x4300c0. Still to port of the reader: particleSystem 0x42f760 (7 uses; the particle code 0x430d20-0x435d50,
ParticleSystem 0x42b920, ParticleStream 0x433390). The particleSystem element reads x, y (0), file (""), name ("ParticleSystem")
and warmup (int, 0): a new ParticleSystem (0xbc bytes, constructor 0x42b920) goes into the top group, then 0x42b7a0
(load the file), setPosition, 0x42b6e0, setName, and 0x42b6b0(warmup) when warmup is set. The dumps are in
`re/wordharmony/work/psystem.c` (0x42b4c0-0x42c1e0), `particles.c` (0x430d20-0x434ce0) and `particles2.c`
(0x435300-0x435d50). After the particles: sound (SimpleSoundDX 0x4425f0-0x444970) and music (0x424160-0x426000),
then the controller and the game code.

ParticleSystem as read (0x42b920, vtable 0x483b70, a Container named "ParticleSystem"): +0xa8 running, +0xac
duration (-1), +0xb0 time left. Its children are the streams. start 0x42b6e0: each stream 0x432880, running = 1,
time left = duration when duration > 0. stop 0x42b730: each stream 0x4328d0, running = 0. isDone 0x42b760: every
stream's 0x432900. setDuration 0x42b6a0. warmup 0x42b6b0(float s): Container::update with origin (0,0) and s
seconds. load 0x42b7a0(file): the particle XML handler (constructor 0x433870, 0x4337f0 sets its system) parses the
file from the archive. update 0x42b8c0: when running with a duration, the time counts down and stop runs below 0;
then Container::update. A pool (0x42bb20, vtable 0x483c68, Container: +0xa8 file, n systems of the file) has
0x42ba00 all done and 0x42ba40 a done system or a new one. Point helpers: 0x42b520 scale, 0x42b540 add, 0x42b560
subtract, 0x42b590 rotate by degrees, 0x42b5d0 normalise, 0x42b610 "%f,%f". 0x42bd10: a random float between a
range's two ends (rand() * 3.051851e-05). The sounds file's handler (0x42bdf0, element "sound": name, file, vol,
pitch, count, category) feeds 0x426760 of the object at its +0x14 (the sound part). ParticleStream as read so far (constructor
0x433390, vtable 0x484518, a Container; its particles are its children): +0x148 a string, +0x16c a vector, +0x17c a
vector of counted objects (released in the destructor 0x432940), +0x18c emitting, +0x194 delay, +0x198 delay left,
+0x19c duration, +0x1a0 time left, +0x1a4 from App 0x436880(name) (0x432ad0). start 0x432880: delay left = delay
when delay > 0; with a duration, time left = duration (emitting stays as it is), else emitting = 1. stop 0x4328d0:
emitting = 0. isDone 0x432900: not emitting, time left <= 0 and no particles. layout 0x4328e0: empty bounds. Still to
read: 0x432e10, 0x4331b0 (update and emit, likely), 0x433390, the particle XML handler 0x4337f0-0x433ef0 and
0x430d20/0x430ee0 (0x430ee0 is large). Read, to be checked against the disassembly before writing: update 0x4331b0
removes particles whose +0xd8 (life left) went below 0 (clearParent, erase), counts time left down to start
emitting, the delay left down to stop it, accumulates rate (+0xa8) x seconds onto +0x188 and emits one particle per
whole unit while +0xac (a cap on the particle count, 0 none) allows, at the stream's position (or the origin when
+0x190 is set), then Container::update. emit 0x432e10(x, y): up to 25 tries of a random index (0x42bce0) into the
+0x17c templates, takes one whose slot 8 returns 1, and sets it up from the stream's ranges (pairs of floats at
+0xb0 ... +0x130, each drawn with 0x42bd10), a rotation by a random angle (0x42b590), an optional motion listener
(0x431930, 0x3c bytes) when the ranges at +0xf0/+0x100 are not zero, visible, +0x138/+0x13c copied, then addChild.

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

## Next pieces, as read so far

The controller (vtable 0x480a14: 0 dtor 0x4073d0, 4 configure 0x402eb0, 8 init 0x406b30, 0xc shutdown 0x401750, 0x10
update 0x409060, 0x14 0x4070a0; a one-slot listener vtable 0x480a10 at +0xc, 0x407580). init seeds rand with
GetTickCount, makes the sound system (0x443ca0, from the window handle, App slot 0x74 stores it) and a 0x14-byte
object at +0x710 (0x426ac0), sets up a beta play-time reporter (0x42a090; an online feature, left out of the port),
then runs the screensaver (+0x1fc set) or 0x4055f0(this, 0). The screen layout handler (constructor 0x42c7f0, a
group stack at +0x18) dispatches elements in 0x430910: image 0x430280, text 0x42ef20, imageButton 0x42c920,
checkBox 0x42d6f0, textButton 0x42e2b0, group 0x42fa90, particleSystem 0x42f760, textSub 0x4300c0, include
0x42fed0; the end of a group pops (0x42c4f0, "Mis-matched end group"). `re/wordharmony/work/layout.c` and
`controller.c` hold the dumps.

Images: the image interface (the image's +0xc, vtable 0x48aba8) has 0 getWidth, 4 getHeight, 8 nothing, 0xc
setPivot(mode: 1 = the centre, w/2 and h/2 in integers), 0x10 setPivot(x, y), 0x14 getPivot (integers), 0x18
setAlphaMode(mode, threshold byte), 0x1c getAlphaMode. The pivot and the alpha mode belong to the cached image, so
every node of that image shares them. Modes: 0 opaque (JPEGs, PNGs without alpha), 1 alpha test (PNGs with alpha get
1 with threshold 0; the layout's alphaType="alphaTest" sets 1 with alphaThresh, default 5), 2 blend (masked images,
alphaType="alphaBlend"). The draw 0x45e2d0: a colour offset uses 0x45d5a0; mode 2 or an opacity other than 1 the
blend 0x45ce90; mode 1 the alpha test 0x45c780; mode 0 the plain 0x45c070 (the pivot and the rotation are applied
inside the blitters; how mode 1 treats partial alpha is not yet read). The image node (0x435d50, 0x94 bytes, vtable
0x484858): a counted image at +0x88, own flag DRAWABLE, name "Image"; draw 0x435d20 draws the image when visible;
layout 0x435c60: bounds = position - pivot to position - pivot + size, then scaled by |scale| (after the offset).
The layout's attribute getters take the map by value: 0x42abd0 string, 0x42ad20 int (atol), 0x42aeb0 float; the
scratch `attrs.py <start> <end>` lists each read with its name and default. The image element 0x430280: file
("" = none), name ("_Image_"), x, y (0), opacity, scaleX, scaleY (1), alphaType, alphaThresh (5), pivot
("upperleft"; "center" centres the image's pivot); the node goes to the screen's slot 0xe8 and to the top group.

## Reading the decompilation

Two scratch helpers (not in the repository) print functions: `fn.py <addr>...` (whole functions) and `fr.py <lo> <hi>`
(a range, compacted: no declarations or SEH bookkeeping), `vt.py <vtable>...` prints vtables side by side, and
`dis.sh <start> <end>` disassembles with llvm-objdump (E:/retools/llvm-mingw-*/bin) where Ghidra loses x87 float
code. They
live in the session's scratchpad; any copy works on `re/wordharmony/gh/all.c`.

Common helpers: 0x4024a0 string assign(const char*, n); 0x4023b0 string assign(string, pos, n); 0x436480 string
destructor; 0x4019b0 string compare(pos, n, const char*, n2); 0x435f20 App::instance; 0x401000 an empty function
(inlined destructors); 0x44648d `__purecall`; 0x445fa8 operator new.
