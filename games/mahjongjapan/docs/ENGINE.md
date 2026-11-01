# Jolly Bear's engine in MahjongEscapeAJ.exe

This is the map of the engine under Mahjong Escape: Ancient Japan, recovered from the exe's vtables, constructors
and calls (the exe has no RTTI and no symbols, so every name here is the port's). Ancient China is the same code with
other resources, so its port reuses this map. Addresses are virtual addresses in `MahjongEscapeAJ.exe` (image base
0x400000, linked 2006-11-08 by MSVC 6; build string "Oct 27 2006 11:11:20"). `symbols/` has the names as JSONL for
the Ghidra pass.

## The binary

| Range | What it is |
|---|---|
| 0x401000-0x417680 | The game (about 89 KB, 609 functions). |
| 0x417680-0x41e1c5 | The C runtime (unaligned functions): `operator delete` 0x417680, `srand` 0x417d95, `rand` 0x417d9f (MSVC's LCG), `fopen` 0x417be7, `fclose` 0x417b1a, the entry 0x417dbd. |
| 0x41e240-0x42cdb0 | Jolly Bear's engine above the device: the object tree, layers, the manager, fonts, files, settings, the app. |
| 0x42cdb0-0x438000 | The device: DirectDraw/Direct3D 7 surfaces and textures, image loading, blitters, cursors, BASS sound and music. The port replaces this layer with the platform (see "The device boundary"). |
| 0x438000-0x447000 | libjpeg (the "JPEGMEM" source manager) and the PNG decoder. Not ported: stb_image decodes both. |
| 0x447000-0x514000 | A large float-heavy library reached only through the CPU dispatch table 0x527fc8 (13 stubs at 0x42fd00-0x42fdc0, chosen by a CPUID check at 0x42fdd0/0x42fe30): software blitters in generic, MMX and SSE variants. Not ported. |
| 0x514000-0x51c000 | More C runtime and STL. |

`WinMain` is 0x429050. It takes the app object from the global `gApp` (0x527ac0, set by the app's constructor),
registers the window class (0x429780; window procedure 0x42ff30), sets up the folders and the error log
(0x429800, "Error.txt"), then calls the app's virtual `Init` (+4) and `Run` (+0).

The game's resources come from two modules: the exe (`gApp+4`, its HINSTANCE) and `Resource.dll` (`gApp+0x30`).
Strings are loaded with `LoadStringA` from either (0x423570 exe, 0x423590 Resource.dll, both into a CString through
0x4235b0). Typed resources ("JPG", "PNG", "LAYOUTS", "TXT", "WAVE", "TGA", "DATA") are read with
FindResourceA/LoadResource (the text reader 0x425470; images 0x4326e0).

## Library code that becomes the C++ library

- 0x421600-0x422020: MSVC 6 `std::basic_string` internals (`_Tidy`, `_Grow`, `assign`, the `invalid vector<T>
  subscript` throw 0x421600).
- 0x422320-0x4238e0: an MFC-style reference-counted string ("CString"): data pointer with a header of refcount,
  length and capacity before it. Constructor 0x422320, destructor 0x4224c0, `=(const char*)` 0x422660,
  `=(const CString&)` 0x422600, `operator+` 0x4227d0, `Format` 0x422e10 (varargs wrappers 0x4231d0, 0x4231f0 for a
  string id of the exe, 0x423260 of Resource.dll), `Tokenize`-like split 0x422a00, `Mid` 0x422ce0, `Left` 0x422d80,
  `Right`/`Mid(n)` 0x422cb0, `FindOneOf` 0x422b70, `TrimRight` 0x4234c0, `TrimLeft` 0x423510, `Insert(char)`
  0x423660, `Delete` 0x423700, `Insert(str)` 0x423760, `GetLength` 0x4238c0, `IsEmpty` 0x4238d0, `c_str` 0x4238e0,
  `atoi` of it 0x422ae0, `+=char` 0x422920, number formatting with the locale's separators 0x4232d0, seconds as
  "H:mm:ss" 0x423450 (through 0x4233d0).
  The port uses `std::string` for both.

## The object tree

**Object** (vtable 0x51df04, 22 entries; constructor 0x41ec10, destructor 0x41ed00): every visible thing. The
constructor reseeds the C runtime's generator with `srand(GetTickCount())`, as do several others (the RNG draws of a
game depend on the clock, as in the original).

| Offset | Field |
|---|---|
| +0x04 | the manager (0x42a350 family) |
| +0x08 | the layer it is in |
| +0x0c | its parent Object (0 for a layer's top object) |
| +0x10, +0x14 | position relative to the parent |
| +0x18 | dirty rect, the union of everything drawn this frame (`AddDirty` 0x41f910, only when +0x39) |
| +0x28 | the clip region: a linked list of rects (rect, then next at +0x10) |
| +0x30, +0x34 | the clip region's tail and own copy (0x41f0b0 rebuilds them, offset to the object) |
| +0x38 | uses a clip region; +0x39 tracks dirty rects |
| +0x40..+0x48 | children (a vector of Object*) |
| +0x50..+0x58 | attached things with their own virtuals (+0x0c update, +0x10 draw, +0x14 pause) |
| +0x5c | draw attachments after itself; +0x5d visible, +0x5e shown by the parent; +0x5f enabled, +0x60 enabled by the parent |
| +0x64 | z order (0x41f090 tells the manager, 0x42b5f0) |
| +0x68 | alpha 0..255 |
| +0x6c | the time it started (its clock: `GetTime` 0x4201e0 is the manager's time minus it, paused time excluded); +0x70 pause start; +0x74 paused |
| +0x78, +0x7c | elapsed since the last update, and the time of it (0x420240) |
| +0x80..+0x86 | registered with the manager for: updates (0x42a350/0x42a360), keys of the layer (0x420a60/0x42a3b0), keys, mouse (0x42a400), mouse moves (0x42a460), wheel? (0x42a4c0), timers (0x42a520) |
| +0x87 | ignores input while disabled |
| +0x88 | a user flag |
| +0x8c | a list of timers (`AddTimer` 0x420360: id, period, count; fired through the virtual +0x48) |

Virtuals: +0x00 deleting destructor, +0x04 Draw, +0x08 Init (after `Create` 0x41f930), +0x0c Clear, +0x10..+0x34
input (0x420000 maps WM_LBUTTONDOWN.. to +0x14 down, +0x18 up, +0x1c double click, +0x20 right down, +0x24 right
double?, +0x28 right up; mouse move +0x2c, +0x30, +0x34), +0x38..+0x40 keys, +0x44 Update, +0x48 OnTimer, +0x4c
OnVisible, +0x50 OnEnabled.

Drawing (all clipped to the clip region and summed into the dirty rect): `DrawImage(x, y, imageId, srcRect, ...)`
0x41f190 and `DrawImageTinted` 0x41f220 (by image id, through the manager's image table 0x42c770), `DrawSurface`
0x41f1e0 (an image pointer), the shared worker 0x41f270 (scaled by percent with MulDiv, alpha multiplied by the
layer's and its own), `FillRect` 0x41f6a0, `FillRectAlpha` 0x41f770, `Line` 0x41f810, `Frame` 0x41f8a0.

**Button** (vtable 0x51de24, 28 entries; constructor 0x41e240(image, frames, ?, noAutoFrames)): an Object with an
image strip (+0x90 image id, +0x94 command id, +0x98 frame count, +0xa0 frame width, +0xa4 height), a label (font
+0xb0, text +0xa8, colours and offsets +0xb4..+0xcc), hover sound +0xd4, enabled +0xd1. Its states draw frames
0..3 (normal, hover, pressed, disabled: 0x41e560, 0x41e660, 0x41e6d0, 0x41e810). Subclasses: a toggle 0x41ea40
(vtable 0x51de94, +0xe4 checked, +0xec group), 0x424ae0 (vtable 0x51dfbc, a check box with 6 frames), the link
button 0x425ba0 (vtable 0x51e088: image 0x38 is the "more games" link with string 0x42; it opens its URL with
ShellExecuteA, 0x425cb0).

**RadioGroup** (0x422020, vtable 0x51df8c): a vector of toggles (+8) and the selected id (+0x14, 0x422260).

**Layer** (vtable 0x51df5c, 10 entries; constructor 0x420840(modal, ?)): a screen or dialog: top objects (+0x0c
vector; `Add` 0x420ac0, `AddFocusable` 0x420c80), focus ring (+0x38 vector, +0x44 index; next 0x421160, previous
0x4211f0, set 0x421280), alpha (+0x24) with fade in/out state (+0x2c: 1 in, 2 out; 0x420d50, step 0x420d80), modal
(+0x20) with a darkening that grows by the step up to 0x80 (+0x28, 0x420d10), +0x48 darkens, +0x49 and +0x4a flags,
+0x4c keyboard listeners. `Draw` 0x420cb0. Subclass 0x425320 (vtable 0x51e03c) is the splash screen (0x425370 loads
"SplashLogo" and adds the logo object 0x42f650).

**Animation** (0x425640, vtable 0x51e068, base 0x42eed0): a frame strip played on a timer with modes (+0x4c: 0 its
own virtual, 1 loop 0x425a50, 2 ping-pong 0x425a70, 3 random ping-pong 0x425aa0), random delays between plays
(+0x54..+0x58 ms, 0xfa..0x4e2 by default), alpha +0x74. Update 0x4258a0, Draw 0x425990.

## Fonts and text

**Font** (vtable 0x51df90; constructor 0x4238f0; load 0x423970(image, txt id, module)). The metrics are a TXT
resource of lines: `H:height,ascent,?,space width,...` makes the glyph table (0x42efc0), `K...` adds kerning pairs
(0x42f2e0), `S...` glyph sizes (0x42f0f0). Glyph lookup 0x42f4b0 (char, next char) gives row, x, width and kerning;
the font image holds one row per glyph line, +0x1c the line height, +0x20 the ascent, +0x24 the spacing, +0x28 the
space width, +0x2c..+0x58 up to 10 tab stops (0x4248b0). Width 0x423c00 (percent scale), fit 0x423d80, per-character
offsets 0x423e20, draw 0x423ef0 (left), 0x4240a0 (centred), 0x4240f0 (right), multi-line 0x424130/0x4242a0, wrapped
0x4244a0 (word breaks at ".,:; ", 0x424760).

## Files and the binary stream

**Stream** (vtable 0x51e02c; 0x424d30): a file (`fopen` mode 0x527ac4 write 1 / 0x527ac8 read 2) or a memory
buffer, with a running checksum (0x42f5e0) over every byte; writes bytes 0x424ec0, reads 0x424f60, big-endian
16/32-bit (0x425100, 0x425160, 0x4251b0), bool 0x425240, strings 0x424fd0/0x425050, and the checksum check 0x425280
(the file ends with the complement of the sum).

**TextResource** (0x425430, vtable 0x51e064): a TXT resource as lines (0x425470 load as ANSI 1 or UTF-16 2; 0x425510
next line).

## Not yet mapped

The app class (vtable 0x51e3c4, constructor 0x4290c0(string ids 0x40, 0x41); the game's app 0x425e90 derives from
it), the settings (0x426060 reads SoundsOn, SoundsVolume, MusicOn, MusicVolume, FullScreen, WideScreen,
ActivateLinks... from the registry under Software\JollyBear), the manager (0x42a0d0 and the 0x42a350-0x42c300
family: per-frame loop, input dispatch, the image table, the cursor 0x42c8f0/0x42c9a0 with frames of image 0x46-0x4a),
the window classes 0x51e298/0x51e2f4/0x51e350 (23 entries), and the device below 0x42cdb0.

## The device boundary

The engine and the game call these device functions and nothing else below 0x42cdb0 (the port implements them on
the platform): the image table (0x432430 get by id, 0x4326e0 load by id/type/module, 0x433aa0 size), blits
(0x4338c0, 0x433900 tinted, 0x433870 to a rect), fills (0x42dff0 solid, 0x42e120 alpha), the font tables
(0x42efc0, 0x42f0f0, 0x42f2e0, 0x42f4b0, 0x42f580), the checksum (0x42f5e0, 0x42f610), the cursor (0x430990,
0x434e60, 0x434f00), the window (0x4308f0, 0x430940, 0x4309f0, 0x430b00), the screen (0x431a70, 0x432000,
0x432350, 0x4328f0, 0x432a00, 0x432a30, 0x432be0, 0x432e90, 0x433380, 0x433490, 0x4334b0, 0x433530, 0x433740,
0x4337a0, 0x433810, 0x434dc0), and the sound object at manager+0xb0 (0x436620 init, 0x436b20, 0x436b50, 0x436b70
play, 0x436d50, 0x436de0, 0x436e50, 0x436ec0 sound volume, 0x436f60, 0x437010, 0x4371a0, 0x437300, 0x4373a0,
0x437400 music volume, 0x437470, 0x4374b0).

The screen is 800x600 (0x42cb10 tests the cursor against it; 0x42cb90 maps a stretched or widescreen window back to
it).
