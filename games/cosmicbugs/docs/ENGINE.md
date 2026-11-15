# Retro64's engine (Cosmic Bugs, 2007)

Cosmic Bugs runs on Retro64's own engine, which Water Bugs shares. It is plain C over global state: no classes
with virtual methods, no RTTI, only structs with type and state fields that the game's code switches on. The exe
is MSVC 2005 with a DirectDraw screen, DirectInput, and BASS for sound and MO3 music. This map is written from our
own Ghidra export (`re/cosmicbugs/`, made by `tools/ghidra_pass.sh`); `docs/symbols/*.jsonl` holds the names given
so far, and the port's `engines/retro64/` (shared with Water Bugs, `docs/ENGINES.md`) translates the engine function by
function, each with its address in a comment.

## Reading the export

`tools/ghidra_pass.sh` runs Ghidra headless with three post scripts. `CosmicFpuHelpers.java` gives the MSVC
floating-point helpers their real signatures (`__ftol2_sse` takes ST0 and returns EAX; `_CIcos`, `_CIsin`,
`_CIsqrt` and `_CIatan2` work on the x87 stack), without which every float-to-int conversion lost its argument.
`CosmicTailCalls.java` gives the blit wrappers (0x429110..0x429d30) the parameter count of the blitter they jump
to, since Ghidra cut their arguments short. `CosmicCommitParams.java` keeps parameters found by the decompiler.
`tools/decread.py <addr>|<from-to>` prints decompiled functions and `tools/dis.sh <from> <to>` disassembles (bare
hex, no 0x). `tools/exeread.py <va>...` prints the bytes and string at a virtual address of the user's exe.

## Start-up and the main loop

WinMain calls GameMain (0x41ee60). It sets the game file to `animations\bugs.eco`, seeds the random generators,
fills the sine and cosine tables (InitSinCos 0x42b9a0: 361 entries each, the value times 1024, from an angle in
degrees times 3.14159/180), registers the screen-lost and screen-changed callbacks, and makes the 800x600 back
buffer in the screen's format with a 40x40 grid of dirty rectangles. It loads `art8.bbk` raw, draws the loading
image centred, loads `art.bbk` converted to the screen's format, and takes its fonts from the banks. It then reads
the game file (LoadGameFile 0x41e9b0) and enters the loop. Each pass of the loop pumps the window's messages, takes
the elapsed milliseconds (clamped: a negative step sleeps 1 ms, one over 0x200 repeats the last), steps the game
once per elapsed millisecond (GameStep 0x40d6a0), draws (GameDraw 0x40ce80) and presents.

The original's screen is 16-bit (it asks DirectDraw for 16 bits per pixel and never changes it; windowed, it
uses 555). The port's screen is 16-bit 565, so only the engine's 8- and 16-bit blitters are translated.

## Data files

`.eco` names stand for `.xml` files: LoadGameFile swaps the extension and reads the tags until `/game`.
`bugs.xml` holds the menus, its `next_file` names `level1.eco`, and `local.xml` holds the localised strings. The
tags are `animation`, `sound`, `random_levels`, `level`, `enemy` (with `emitter` inside), `font`, `song`,
`next_file` and `level_set`. All of them are plain XML read by the engine's own reader (R64Xml: Xml_NextTag
0x42d3c0 reads one tag with its attributes and, for an opening tag, the text after it; Xml_ReadToken 0x42d2d0
reads whitespace-separated tokens of that text, skipping `//` and `\\` comments).

The art is in two banks. A `.bbk` is XML: `BANK datafile="..."`, an `IMAGELIST` of `IMAGE` tags (name, offset
and size in the `.dat`, bits per pixel and the colour masks, width, height, pitch, the original size and the
offset of the cropped image within it, a palette number) and a `PALETTELIST` of `PALETTE` tags whose text is
r g b triples. The `.dat` is every image's pixels one after another. `art8` holds 730 8-bit images (the fonts,
the loading image) and three palettes; Bank_Load mode 0 keeps them as stored and converts each palette to the
screen's format. `art` holds 219 32-bit images; mode 1 converts each to the screen's format and gives it a mask
of its alpha bytes. An 8-bit image is its own mask, and an image named `mask_<name>` becomes the mask of the image
named `<name>`.

Animations are FLIC files (`animations/X.flc`), each with an optional `mask_X.flc` whose palette's red channel
is the mask; the exe has its own FLIC decoder (0x40c1a0 open, 0x40c690 next frame, 0x40c890 close).

## Images

An Image (0x128 bytes, R64.h) is a pixel buffer with its format, a clip rectangle, the original size and offset
from the bank's cropping, a handle (the point drawn at the given position), an optional palette and an optional
mask. Its mask may carry runs (Image_BuildRle 0x425150, Rle_Build 0x42dfa0): per row, counts of transparent,
blended (with their alpha bytes) and solid pixels, which the RLE blitters walk.

The drawing methods (0x423740..0x424e10) clip with ClipBlit (0x42bad0) and pick a blitter through a wrapper per
destination depth (0x429110..0x429d30): plain copy, copy through the mask, a colour through the mask (with or
without a fade), blend, alpha (0xff copies), alpha faded, additive, rotated, and for 8-bit images the same through
the palette. The runs are taken only when the whole image is drawn and the mask has no offset. The blend of a
component is `((((m&s)-(m&d))*a>>8)+(d&m))&m` on the masked pixel. A few quirks are kept: some solid runs blend at
0xff instead of copying, the faded variants at 0xff blend twice, and the rotated blitters plot each pixel also on
the row above when its y truncates there.

## Lists

The engine keeps its objects in doubly linked lists of {current, head, tail} with nodes {next, prev}; the banks
use MSVC std::lists, read by position.

## Drawing order and dirty rectangles

The game does not draw its sprites at once: it queues them with DrawList_Add (0x421070, up to 2000 items of 0x44
bytes at 0x6e6540) and DrawList_Flush (0x4211d0) sorts and draws them. The sort is a comb sort by depth then by a
sequence value (the game passes the object's address), followed by the CRT's qsort by depth alone with a comparator
that never answers 0; the order of equal depths therefore depends on the MSVC 2005 qsort's algorithm, which the
port translates from the exe (CrtQsort, 0x444c50). Each item swaps its palette into the image for the draw and
names one of seven blits (plain, alpha, blend, colour fill, faded alpha, faded fill, additive).

The back buffer is copied to the screen through a grid of 40x40 cells (Dirty, 0x4214b0..0x421e00), each with the
rectangles that changed in it. GameMain uses one bounding rectangle per cell (mode 1); in the other mode a cell
keeps up to maxRects rectangles, merging two when the union is no larger than their sum, and becomes one whole-cell
rectangle when full. Dirty_CopyTo copies each rectangle and reports it to the screen (0x428f50).

## Fonts

A Font (0xd6c bytes, 0x421e40..0x423440) is a glyph image per character code, read from a bank by a name prefix
followed by the code (Font_LoadFromBank) or cut from one strip image at its empty mask columns (Font_StripCut, which
adds a space half as wide as the first glyph). It draws onto a target buffer with a palette (the 8-bit faces), a
spacing, an optional clip, and, in Font_DrawBounded, a '^' code that picks one of the palettes stored by character,
an outline palette drawn a pixel up, down, left and right first, a texture drawn through the glyphs' masks and a
mask target that collects the glyphs' masks. Glyphs with a mask on a screen over 8 bits are drawn faded at the
font's alpha. The game's fonts are the `font` tags of its files (`face` is the prefix, `8bit` picks art8.bbk).

## FLIC

The exe reads FLI and FLC files itself (0x40c1a0 open, 0x40c690 next frame, 0x40c890 close): the whole file is kept
in memory behind a 0x14-byte handle and each call decodes one frame onto an 8-bit buffer, with palette chunks going
to a global 256-colour palette (0x6e57a0). It handles COLOR_256 and COLOR_64 alike without scaling (and moves the
index only by each packet's skip), BYTE_RUN (taking each row's packet count from its count byte), DELTA_FLI and
DELTA_FLC; after the ring frame it goes on with the second frame. The port's decoder is R64Flic.

## The game's animations and objects

An Animation (0x5dcc bytes, 0x401480..0x402850) is an `animation` tag: a run of `num_frames` bank images starting at
`start_frame`, or a FLIC (`X.eco` stands for `animations/X.flc`) whose mask FLIC `mask_X.flc` gives each frame a mask
from its palette's red channel, or a TGA (the game ships none). FLIC animations keep the file's palette and ten
tinted copies of it that recolour the green-dominant entries (ColourShift 0x4204c0); `rotation` turns square frames
by 90, 180 or 270 degrees when loaded. The animations are a global list (0x46f35c) searched by name.

The game's objects share a base of 0xb0 bytes (constructor 0x40f060, destructor 0x40f0f0): the list links, a kind
and state, the position and the previous one, the velocity, a depth, a colour and alpha, a tint index, the current
image, the draw-list kind, an age and lifetime, two child lists (one of particle emitters, one of child objects),
the animation with its frame, frame count, frame time, timer and step, and flags for looping, ping-pong and a
shadow. The base update (0x40ee80) moves the object, steps its animation (state 2 fades it out), ages it, and
updates its children, removing those whose update returns 0; the base draw (0x40eac0) queues the image (and its
shadow) on the draw list. Every walk over a list goes on from the node after the one that replaced a removed node,
so that node is skipped for the frame; the port keeps this (List_Each).

## Input

The window procedure (0x42a7a0) fills the input that the game reads (0x4289d0..0x428b40): the three mouse buttons
held, a press latch for each that Mouse_Pressed (0x428a00) sets once per press and a release clears, the keys held
by virtual-key code, and a ring of the last 100 keys pressed (Key_Pop 0x428aa0 takes the newest). A key's slot is 1
from its press until Key_Pressed (0x428b40) reads it, then 2 until its release; Key_Down (0x428b20) is the held test.
The game's own keys (space, Enter, Esc and '.') go through Key_Pressed, so holding one acts once. The game samples
them once per pass of the loop, after PumpMessages has handled every waiting message, so a button pressed and let
go between two passes is never seen. Before queueing a key the procedure acts on four of its own: F5 turns the
sound on or off, F6 the music, F7 switches between the window and the full screen, F8 between stretched and
centred. Activating the window clears the keys held and resumes the music; deactivating it pauses the sound, and
while the window is inactive PumpMessages (0x42a070) waits instead of returning, so the game stops, unless
0x70819c is 1 (only the demo's power-ups set it). The pointer is read with Mouse_Update (0x429e40), in the game's
800x600 coordinates; the game also moves it (it pins it to the ship while a line is fired).

The engine's text entry (R64TextInput, 0x42bda0..0x42c190) edits one line from the key ring into its own image,
with a font, a fill colour, an outline, a case mode and an Enter callback; the name requesters and the Hall of Fame's
name use it.

## The screen, the settings and the saves

The screen functions (0x428e70..0x428ff0) wrap DirectDraw: the clock (TimeMs, timeGetTime), opening a link (OpenURL
0x428e90, which the game does at exit when its URL is set), the mode (full screen or a window, stretched or
centred), locking the screen for GameDraw's copy and presenting it. A change of mode is applied by PumpMessages: it
keeps the pointer's position, reopens the screen, calls the screen-changed callback (the game's, 0x41e7d0, marks the
whole screen dirty) and writes the settings.

The settings are `cp.cfg` in the save folder (LoadConfig 0x42a340, SaveConfig 0x429db0): one `CP_CONFIG` tag with
`fullscreen`, `desktopResolution` (the centred mode), `wave` and `music`, defaulting to the full screen, stretched,
with sound and music. The players, their progress and the high scores are `cb.dat` (0xbf9c bytes; LoadProfiles in
the port's CBMain.cpp has the order): the eight high scores of 0x24 bytes come first, then the current player's
level reached, a placeholder field (the original's trial time, now only kept for the format) and the shooter mode.

## Sound and music

Sound and music go through BASS. Sound_Start (0x42ad70) opens the device at 44100 Hz (22050 if that fails) and sets
the overall volume to 60 of 100. Samples (Sample_Load 0x42b370) are kept by index and played by Sound_Play
(0x429fb0); the game keeps its own list of named sounds (SoundList_Load 0x428770, SoundList_Find 0x428650) whose
lookup answers 0, the first sound, for a name it lacks. The music is one MO3 module at a time: Music_Load (0x429030)
loads it looping with BASS's volume ramping, Music_Play (0x429090) starts it from the beginning, Music_Resume
(0x4290c0) goes on, and Music_SetVolume (0x429000) takes 0..256 and gives BASS a percentage. Each level set names
its song, and a `song` tag of a game file loads one at once (the title's).

The port puts all of this behind one interface, `r64::SysHost` in `engines/retro64/R64Sys.h` (the files, the screen, the
clock, the pointer, the sounds, the music, links, the save folder and the player's name), which the port's host
(`src/CosmicBugsApp.cpp`) implements over the platform and the headless runner (`test/gamerun.cpp`) over plain
memory.
