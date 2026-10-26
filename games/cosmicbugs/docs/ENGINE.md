# Retro64's engine (Cosmic Bugs, 2007)

Cosmic Bugs runs on Retro64's own engine, which Water Bugs shares. It is plain C over global state: no classes
with virtual methods, no RTTI, only structs with type and state fields that the game's code switches on. The exe
is MSVC 2005 with a DirectDraw screen, DirectInput, and BASS for sound and MO3 music. This map is written from our
own Ghidra export (`re/cosmicbugs/`, made by `tools/ghidra_pass.sh`); `docs/symbols/*.jsonl` holds the names given
so far, and the port's `engine/` translates the engine function by function, each with its address in a comment.

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
