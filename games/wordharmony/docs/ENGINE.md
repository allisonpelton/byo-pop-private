# Word Harmony's engine: Sprout's first-generation `engine`

Word Harmony (Sprout Games, published by PopCap; the build inside `WordHarmony.exe` was linked on 2006-02-22 with
Visual Studio .NET 2003, internal name WordBall, "Release-2.4.3") does not run on the SexyAppFramework. It runs on
Sprout Games' own engine, the C++ namespace `engine`, which Sprout also used for Pizza Frenzy and Iggle Pop (Sprout's
"first generation"; Feeding Frenzy 2 and Talismania use a later, rewritten one). This file describes that engine as
Word Harmony's binary has it. Addresses are those of the carved game exe (`re/wordharmony/bin/WordHarmonyGame.exe`, see
README.md); nothing here comes from another project's decompilation.

## How it relates to the other Sprout games

The engine is shared, but this is an older build of it. Of the 198 strings that the engine's code refers to, 170
appear in Pizza Frenzy's and in Iggle Pop's game exes too (the XML parser's messages, the archive loader's, the
DirectDraw display's, the sound system's, the screen layout's attribute names). Only about a fifth of the engine's
bytes are identical to Pizza Frenzy's, though (relocation-masked function bodies, the survey's `funcsim.py`): the
functions were rebuilt and changed between the two. Word Harmony has three parts the other two lack: a layered music
system (`musicLayer`, `bridge`, `finalOnly`, `playFinal`...), ACM and WAV readers for its WAV sounds, and a crash
reporter that writes a minidump. So the engine port here is meant to become shared code once a second Sprout game
is ported; it is kept apart from the game for that reason (`engine/`, no game code in it).

## The binary's layout

The code section runs from 0x401000 to 0x47f50a, 3,456 functions after the vtable pass. In link order:

- 0x401000-0x424000: the game (screens, the board of letter balls and cubes, levels, word lists, high scores, the
  registry), about 145 KB.
- 0x424000-0x446000: the engine proper, about 123 KB: the music system, text items, the frame-rate display, value
  parsing, sound categories, the screen layout parser and its widgets (image buttons, text buttons, check boxes,
  groups, includes), particle systems and streams, images, the application base class, windows and nodes, the XML
  parser, the render context, fonts, the resource manager, the archive loader, the DirectSound sound system, the
  ACM and WAV readers, file streams.
- 0x446000-0x454a40: the C runtime (Ghidra names it: "Visual Studio 2003 Release").
- 0x454a40-0x45ee00: the engine's Windows layer: the crash handler, the Windows application (WinMain is 0x455820,
  the main loop 0x4552b0, the window procedure 0x454510, the window class is named "Gatsu"), the DirectDraw display,
  its surfaces and the image loader (PNG with a separate `<name>_<suffix>.png` alpha mask, and JPEG).
- 0x45ee00-0x480000: libpng 1.2.5, zlib 1.1.4 and the JPEG decoder.

RTTI exists only for `engine::Object`, `engine::Interface` and `engine::Exception` (as exception types); every other
class is unnamed in the binary. The 133 vtables were found by `tools/vtables.py` (runs of code pointers in .rdata
whose first entry the code references); 328 virtual functions were only reachable through them and were created in
Ghidra by the pass script.

## Objects and references

Every engine class derives virtually from a reference-counted base, Java style. `engine::Interface` is the virtual
base with three slots (add a reference, release one, read the count); `engine::Object` (constructor 0x435c10) holds the
count at +8 and implements them (0x435bd0 adds, 0x435be0 releases and deletes at zero, 0x435c00 reads). An object is
`{vfptr, vbptr, ...fields..., Interface}`, and every reference held in a field or a container is counted: the
decompilation's `*(*(p+4)+4)+4+p` is the step from an object to its Interface part through the vbtable, followed by
slot 0 (add) or 1 (release). The port keeps the counting with an intrusive `Ref<T>`.

`App::instance()` (0x435f20) returns the one application object (0x49bfbc).

## Nodes, containers and windows

The screen is a tree of nodes. The node base class (constructor 0x43a3c0, vtable 0x484b50, 27 slots, 0x88 bytes
before the virtual base) holds: the parent (+0x0c, not counted; a parent holds a reference to each child), a name
(+0x10, a `std::string`; naming a node registers it in the application's name map, 0x437f60), the position (+0x2c),
the absolute position (+0x34, the parent's plus its own, 0x439bd0), the scale (+0x3c, 1,1), the rotation (+0x44), the
opacity (+0x48, 1.0), a counted reference at +0x4c (slot 12 sets it; it is handed to the render state, 0x43c980: the
font), a colour offset (+0x50, three floats), two flag words (+0x5c the node's own, +0x60 its own or'ed with its
children's, so a parent knows a descendant wants an update or a redraw), visible (+0x64) and enabled (+0x65), the bounds
(+0x68, four floats) and a vector of listeners (+0x7c), each called with an event code and the node (0x439e60).

The container (constructor 0x40f110, vtable 0x481ca8, 54 slots) adds the children (+0x88, a vector of counted
references) and a clip rectangle (+0x98). It forwards the per-frame update (slot 0x70, 0x439510: it removes children
flagged for removal, flag 0x10, and updates those flagged 0x14), the layout pass (slot 0x74, 0x439210: the bounds are
the union of the children's, scaled and moved by its own), drawing (slot 0x6c, 0x4392b0: for each visible child, push
the render state, apply the child's transform (slot 0x3c, 0x439df0: translate, scale, rotate, opacity, colour, font),
draw it, pop), hit testing (slots 0x44 and 0x48: the topmost child under a point, optionally with a flag), and the
input slots (0x7c-0xac: mouse down, up, move, key down, key up, character...), which call one another in pairs: each
public slot calls a protected one that subclasses override.

The window (constructor 0x438e70, vtable 0x484a08, named "Window") is the container at the root of a screen: it keeps
the node under the mouse and the one with the focus, a vector of counted overlays (+0xb0) and the mouse position.
The application owns one root window (+0xa8) and forwards the window procedure's input to it (0x4387f0 mouse down,
0x438860 mouse up, 0x4389b0 mouse move, 0x438770 key down, 0x4387b0 key up).

## The render context

Drawing goes through a render context (base vtable 0x484cf8) holding a deque of 0x40-byte render states (0x43d0a0
pushes the first; 0x43d120 pushes a copy of the top, 0x43ce10 pops). A state is a translation (+0x00), a scale (+0x08),
a rotation (+0x10, added), an opacity (+0x14, multiplied), a colour (+0x18), a colour offset (+0x24, added), the font
(+0x30, counted) and a bit field (+0x34: the text's alignment among other things). The context's pure slots draw an
image with the current state; slot 3 (0x43cba0) draws a string glyph by glyph through them. The Windows layer
implements it over DirectDraw surfaces (constructor 0x45dcb0, vtable 0x48ac14) with Sprout's own software blitters
(0x457830-0x45dcb0). Its image draw (slot 0, 0x45e2d0) adds the state's translation to the position, rounds it to whole
pixels according to the state's snapping bits (both coordinates, only one, or neither), and picks a blitter: a colour
offset blit when the state's colour offset is not zero, a plain or colour-keyed blit when the opacity is 1, and an
alpha blit (opacity times 255) otherwise; each takes the scale and the rotation. Slots 5 and 6 fill a rectangle with the
state's colour and opacity, slot 7 draws a line. The port maps all of these onto the framework's blits (matrix blits
with a colour), so its pixels can differ slightly from the original's.

## Fonts

A font (0x1c40 bytes, constructor 0x43d770, loaded from XML by 0x43db80) is a bitmap font: one image and, for each of
the 256 characters, the A, B and C widths (+0x14, +0x414, +0x814) and the glyph's rectangle in the image (+0xc14),
then the height, the ascent, the width of a space and the overlaps. The XML (`res/fonts/*.xml` in the archive) names
the image and lists `<glyph char= a= b= c=>`.

## The application and its loop

The application base class (constructor 0x4382f0, vtable 0x484948, 32 slots) keeps the name map, the resource
manager, the image and font caches (0x437b20 and 0x437be0 load an image or reuse the cached one, 0x437dc0 and 0x437e90
the same for fonts), the root window and the timing. The Windows application (constructor 0x455050, vtable
0x48a398) adds the window, the display and the sound system. WinMain creates it and runs it (0x4552b0): it reads the
configuration, opens the archive (0x43eae0), creates the window ("Gatsu") and the display, loads DbgHelp for the
crash handler, and then loops: it handles the window's messages, and whenever a frame's time has passed it runs one
frame (0x4542e0). A frame measures the time since the last one (as a fraction of the frame period, at most 0.33),
lets the game's controller update, updates the root window (slot 0x70) and lays it out (slot 0x74), and, when the
display is ready, draws the root window into a render context over the back buffer (slot 0x6c) and presents it. There
are no modal loops: the port runs one frame per framework update, without ASYNCIFY.

## Files

All game data is in `WHArchive.saf`, a Sprout archive: the header `FFAS`, the version 1 and the offset of the table
of contents; the table holds a version, a keyed MD5 of the rest of the table and the entries (offset, size, keyed MD5
of the data, the name's length and the name). The keyed MD5 is MD5 with its four starting words increased by 11, 71, 37
and 97 times 0x53414646; both the table's and the files' digests verify. The data is neither compressed nor encrypted.
The archive loader (0x43f1f0-0x4417c0) reads it; the resource manager (0x43e640) looks files up in it. Next to it, the
game reads loose files: `res/sounds` and `res/music` (WAV), `res/splash` and `res/buttons`.

Inside the archive are 52 XML files (screens, levels, particle systems, music, sounds, fonts), 117 PNGs, 5 JPEGs and
two text files with the word lists. Screens are XML documents (`<screen>` with `image`, `imageButton`, `text`,
`textButton`, `checkBox`, `group`, `include`, `textSub` and `particleSystem` elements and attributes such as
`x`, `y`, `pivot`, `scaleX`, `opacity`, `action`, `overSound`), parsed by the screen layout code (0x42c4f0-0x431000)
into nodes.

## Sound

Sounds are WAV files played through DirectSound (`SimpleSoundDX.cpp`, `SoundHandleDX.cpp`,
`StaticSoundHandleDX.cpp`), read by `WaveFileReader.cpp` or, for compressed WAVs, through the ACM
(`AcmFileReader.cpp`); larger ones stream through a buffer that is refilled. The music is not a module or a stream
but layers of short WAV phrases (`res/music`), arranged by the music system (0x424160-0x426000) from
`config/music.xml` and the other music configurations.
