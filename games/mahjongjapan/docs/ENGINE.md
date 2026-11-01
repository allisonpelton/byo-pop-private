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
| 0x41e240-0x42cdb0 | Jolly Bear's engine above the device: the object tree, layers, the manager, fonts, files, settings, the app, and the widgets: the slider bar 0x426910 (vtable 0x51e298), the list box 0x426ac0 (0x51e2f4) and the text edit 0x427920 (0x51e350), which gets WM_KEYDOWN at +0x3c and WM_CHAR at +0x40. |
| 0x42cdb0-0x431a70 | Engine classes that sit on the device: the image (0x42cdb0), the font's character map (0x42efc0), the CRC32 (0x42f610), the logo screen (0x42f650), the window (0x42f990, procedure 0x42ff30), the slider bar's thumb (0x4309f0) and the crash handler (0x430b50-0x4319e0, not ported). |
| 0x431a70-0x438000 | The device: the display (DirectDraw/Direct3D 7, the image and font caches) and the sound object (BASS). The port replaces it with the platform (see "The device boundary"). |
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

## The app

The app class (vtable 0x51e3c4, constructor 0x4290c0 with the string ids 0x40 and 0x41) holds the instance (+4), the
command line (+0xc), the window class name (+0x18), the window object (+0x2c), `Resource.dll` (+0x30) and the
manager (+0x34, embedded). The game's app 0x425e90 (vtable 0x51e270) derives from it. Its `Init` 0x426060 takes a
mutex, reads the settings from the registry under `HKCU\Software\JollyBear\<game>` (SoundsOn 0x527ae0, SoundsVolume
0x5d8e28 default 100, MusicOn 0x527ae1, MusicVolume 0x5d8e2c default 50, WideScreen, FullScreen,
HardwareAcceleration; ActivateLinks from HKLM), loads `Resource.dll` (string 0x4a), starts the manager (0x42a5f0) and
pushes the first layer. `Save` 0x4266c0 writes the settings back. The registry helpers are 0x4291b0 (get int),
0x429230 (get string), 0x429360 (set int), 0x4293c0 (set or delete a string) and 0x429470 (get bool); the game key is
the app name with everything but letters, digits and spaces removed (0x429ac0). The folders (0x429800) are the
per-user application data folder (CSIDL 0x1c, else 0x1a) and the shared one (0x23, else 0x2e, else 0x1a), each with
`\JollyBear\<game>\` appended; command-line arguments are read by 0x429b90 (a number) and 0x429ca0 (a flag).

## The manager

The manager (constructor 0x42a0d0, vtable 0x51e3d8) is embedded in the app at +0x34. It owns the display (+4,
0x431a70), the window (+0x28), the frame period in milliseconds (+8) and the sound object (+0xb0, 0x436620).

Objects register in linked lists of nodes {object, next, prev} (node add 0x420710): +0x48 hover (removed by
0x42a360), +0x4c click (0x42a3b0), +0x50 move targets (0x42a400/0x42a410), +0x54 character listeners
(0x42a460/0x42a470), +0x58 key listeners (0x42a4c0/0x42a4d0), +0x5c per-frame updaters (0x42a520/0x42a530; each gets
`Update(clock)` 0x420240), +0x60 animators (0x42c060; stepped by 0x420d10 with dt/3) and +0x64 layers in transition
(stepped by 0x420d80 with dt*3/5).

The layer stack is a vector at +0x3c/+0x40/+0x44, top last. 0x42a9f0 pushes a layer (optionally animating it in),
0x42b0d0 replaces the top one, 0x42ac60 pops (0x42acb0 deleting at once, 0x42adb0 animating out), 0x42b2d0 deletes one
layer and 0x42b340 clears the stack. A screen change with a fade is 0x42ae80(new layer): state +0xc8 goes 1 (fade
out: alpha +0xcc from 255 down by dt/3), 3 (0x42aed0 clears the stack and pushes the layer held at +0xd0), 2 (fade in
up to 255; +0xd4 skips the first frame) and back to 0 in 0x42b0b0, which calls the new top layer's vt+0x20 ("shown").

The frame (0x42bd40) runs on the window's WM_TIMER (window vt+0x44, 0x42fc60) while the app is active (+0xd), not
paused (+0xc) and not suspended (+0xe): the game clock +0xa4 is now minus the start time +0x18 (0x42c140 adds paused
time to the start), `Update` 0x42be10 gets the clock and the real dt since the last frame, then the frame draws
(0x42a820) and sleeps until base +0xa8 plus count +0xac times the period. `Update` runs the fade state machine, the
animators, the transitions and the updaters, each list stopping when a layer was deleted (0x5d8e9d).

Drawing (0x42a820) happens only when the screen was invalidated (0x527ba0, set by 0x42a7f0): every layer from the
topmost opaque one (0x420a40) up draws itself (0x420cb0), the fade is a black fill (0x42c830) or an alpha fill
(0x42c810 with 255 minus +0xcc), and the display presents (0x432a00). With the hardware cursor off the frame draws
the software cursor (0x42c9a0: images 0x46, 0x47, 0x48, 0x49, 0x4a with hot spots (9,1), (1,1), (6,14), (9,1), (1,1)).
0x42c8f0 picks the cursor: 0 arrow, 1 hand, 2 and 3 the game's own, 4 wait.

Input arrives from the window procedure: mouse move 0x42b3b0 (a drag first, then the hover list by
`PtInRect(object+0x18)` and 0x420520 "enabled"; enter and leave go through 0x42b5c0 to 0x4205a0; the hovered object's
+0x64 is the cursor), left down 0x42b620 (message 0x201 to the click list or to the captured object +0x30), left
double 0x42b6e0 (0x203, else as a click), left up 0x42b840 (0x202; ends a drag), right down 0x42b910 (0x204), right
double 0x42b9b0 (0x206), right up 0x42ba70 (0x205), key down 0x42bc60 (WM_KEYDOWN to the +0x54 list until one takes it),
character 0x42bcb0 (WM_CHAR to all of +0x58; TAB moves the top layer's focus, 0x421160 forward and 0x4211f0 back with Shift) and
activation 0x42bb90. A drag starts with 0x42b7a0(object, flags, clamp rect): flag 1 moves the object inside the
rect (0x42bb10), flag 2 sends it the moves. 0x42cb90 maps a stretched window back to 800x600.

The manager's forwards to the device are 0x42c370-0x42c530 (sound) and 0x42c6b0-0x42c870 (display), listed below.
The clipboard is 0x42c550 (get) and 0x42c5c0 (set); 0x42c660 reads the mouse buttons, Shift and Ctrl.

## Engine classes on the device

**Image** (0x42cdb0, vtable 0x51e3fc over 0x51e410, 0x38 bytes): +4 width, +8 height, +0xc pixels (rows bottom-up,
as a DIB), +0x10 owns the pixels, +0x14 pitch, +0x18 bits (24 or 32), +0x1c colour key, +0x20 keyed, +0x21 prepared,
+0x24 texture tiles, +0x28/+0x2c a 32-bit copy, +0x34 the display that tracks it. 0x42cfc0(id, module) loads a
resource of type BITMAP, then "JPG", "TGA", "PNG" (0x42d080 decodes: BMP in place 0x42d110, JPEG to BGR 0x42d160,
TGA raw or RLE 0x42d320, PNG 0x42d580); 0x42d670 loads a file by extension; 0x42ecd0 from `Resource.dll`. The blit
0x42dae0(x, y, source rect, x scale %, y scale %, alpha) goes to the software path 0x42dcd0, clipped to 800x600,
scaling through a temporary buffer (0x42cec0). Fills are 0x42dff0(rect, colour) and 0x42e120(rect, alpha, colour).
0x42eb60 makes a blank 32-bit image, 0x42ebb0 fills it and 0x42ed70 saves it as a BMP.

**Character map** (0x42efc0, vtable 0x51e444): ranges of {first character, count, offsets, widths, kerning lists};
0x42f0f0 parses a "first,width,width,..." line, 0x42f2e0 a kerning line, 0x42f4b0 looks a character up, 0x42f580
tests it. **CRC32** 0x42f610 (table 0x527bc4), used by the checksummed files.

**Logo screen** (0x42f650, vtable 0x51e448): draws images 0x4f, 0x4d, 0x50, 0x37, 0x51, 0x4e and 0x3a, waits 2000 ms
(+0x90) and then starts the game (the app's vt+0x1c) or, when +0x9c is set, fades to layer 0x425320.

**Window** (0x42f990, vtable 0x51e4a0 over 0x4306c0, 0x51e568): the window procedure 0x42ff30 dispatches the
messages to its virtuals (WM_TIMER +0x44, mouse +0x24-+0x3c, keys +0x18/+0x20, activation +0x40). **Slider**
(0x4309f0, vtable 0x51e5dc, a Button): a thumb dragged between +0xe4 and +0xe8 that reports 0-100 (0x430ab0,
0x430b00).

## The device boundary

The port implements these on the platform; nothing else below 0x431a70 is called from above.

The display (0x431a70, vtable 0x51e674) caches images by id in a map at +0xbc and fonts at +0xcc: 0x432430 gets an
image by id (loading it from the given module, `Resource.dll` by default), 0x433380 loads one from a file under an id,
0x433490 preloads, 0x4334b0 frees, 0x433530 creates a blank 32-bit image it tracks (+0xe0), 0x433740 and 0x4337a0 make
untracked images from `Resource.dll` (the second with a colour key), 0x433810 stops tracking. The draws are 0x433870
(by id), 0x4338c0 (an image) and 0x433900 (an image with a colour key), all as (x, y, image, source rect or null,
x scale %, y scale %, alpha 0-255, out rect) through 0x433940; 0x433aa0 returns an image's size and format. 0x4326e0
gets a font by id. The rest is DirectDraw: mode set 0x432000, windowed or full screen 0x432360, begin 0x4328f0,
present 0x432a00 and 0x432a30, the cursor's saved background 0x434e60/0x434f00.

The sound object (0x436620, vtable 0x51e680) wraps BASS (`BASS_Init` 44100 Hz in 0x436b20). Sounds are WAVE
resources of `Resource.dll` kept by id in a map at +4: 0x436b70 loads one (looping or not), 0x436d50 plays it at the
sound volume +0x14 unless muted (+0x30), 0x436de0 stops it, 0x436e50 stops all but one, 0x436ec0 sets the volume,
0x436f60 pauses or resumes them (all or only the looping ones, except one). Music (MO3) is kept at +0x20: 0x437010
loads it, 0x4371a0 plays it (restart flag, fade-in time), 0x437300 sets a loop point through a BASS sync, 0x437360
stops, 0x4373a0 pauses, 0x437400 sets the music volume +0x18, 0x437470 slides it, and 0x4374b0 pauses everything
while the app is inactive.
