# Bonnie's Bookstore's engine

Bonnie's Bookstore Deluxe (New Crayon Games, published by PopCap; the game exe inside `BonniesBookstore.exe` was linked
on 2006-11-10 with Visual Studio .NET 2003, linker 7.10) does not run on the SexyAppFramework, and none of its logic is
scripted: there is no Lua or other VM in the exe, and `Data/Bonnie.pak` holds only bitmap fonts. Everything is C++ in
the exe, on New Crayon's own small engine, which sits on SDL 1.2 (a 32-bit software surface), SDL_image (PNG and JPEG)
and FMOD 3 (samples and the module music). This file describes that engine as the binary has it. Addresses are those of
the carved game exe (`re/bonnies/bin/BonniesGame.exe`, see README.md); nothing here comes from another project.

No other PopCap game of the queue uses it: Word Harmony, the other non-SAF word game of 2006, is on Sprout Games' engine
(`games/wordharmony/docs/ENGINE.md`), a DirectDraw engine with reference-counted nodes that shares nothing with this one.

## The binary's layout

The code section runs from 0x401000 to 0x4561fa. In link order:

- 0x401000-0x41d600: the game (the application class, its screens, the board, the dictionary, the high scores), about
  115 KB.
- 0x41d600-0x42f6f6: the engine, about 72 KB: the string class, the sound system over FMOD, the platform helpers (paths,
  Portal.cfg, the clipboard, the window), the application base classes and their screen stack, the resource manager,
  the image loader, the software renderer, the language table, file access through the pak, the widget classes, the
  floating texts and the event queue.
- 0x42f6f6-0x42f834: the import thunks (FMOD, SDL, SDL_image).
- 0x42f834-0x443430: the C runtime (Ghidra names it) and the exception unwind funclets.
- 0x443430-0x4561fa: 159 C++ static initializers (global strings and tables such as the fonts' descriptors) and the 77
  destructors they register with `atexit`. They are reached only through the CRT's `__initterm` table (0x466000-0x466284)
  and push immediates, so Ghidra's analysis does not find them; `games/bonnies/tools/ghidra/CreateFunctionsAt.java`
  creates them.

There is no RTTI. The 27 vtables (`games/bonnies/tools/vtables.py`) are: the application (0x4572f0, 29 slots) over the
engine's application (0x458c38) over the platform base (0x4597d0, 22 slots); 15 screen classes with a 7-slot base
(0x4574bc...0x458ad4); 8 widget classes with a 15-slot base (0x459900...0x459ad8); and a small Portal.cfg reader
(0x458be4).

## Coordinates

The renderer works in normalized screen coordinates: a position or size is a float from 0 to 1 across the 800x600
screen, and each primitive converts it with `__ftol(x * width + 0.5)` (0x424c00 and the others; `__ftol` truncates). So
almost every drawing call takes floats on the stack, which Ghidra's default analysis loses; the signatures in
`docs/symbols/01_stackparams.jsonl` (from `tools/stackparams.py`) restore them.

## Start-up and the main loop

The program's main (0x403b80, called from SDL's WinMain through 0x41f9b0) loads the language table, refuses a second
instance (the mutex "BonniesBookstoreMutex", with a message from the language table), creates the application
(0x401160, 0x2fc bytes, global 0x4752a0), the Portal.cfg settings (0x420830, 0x18 bytes, global 0x480b08) and the
options (0x404260, 0x78 bytes, global 0x475a68), calls the application's slot 0 (its initialisation), and runs:

    while (!quit) { start = SDL_GetTicks(); app->Update(); app->Draw(); wait until start + 16 ms; }

`quit` is 0x481a5c. Waiting (0x420770) pumps SDL's events and sleeps.

## The platform base (`PL_`, vtable 0x4597d0)

`PL_BaseInit` (0x42a690) sets the global platform object (0x481a58 = the application itself), loads the language table,
reads the desktop size, builds the atan2 table (0x41f7e0) and seeds `rand` from the time, then creates the engine's
singletons: the key/mouse state (0x481ac0, 0x150 bytes), the pak (0x481abc), the resource manager (0x480b1c), the
floating texts (0x481ab0), the event queue (0x481ab8), the widget manager (0x481ab4, 0x51c bytes), the graphics
(0x4818a8: SDL_Init, the window icon and caption, the 800x600x32 video mode) and the sound system (0x47c764); then sets
the arrow cursor and SDL's Unicode and key repeat (500 ms, 45 ms).

## The application and its screens (vtable 0x458c38)

The engine's application keeps a stack of screens (depth at +0x198, up to 20): each level has a result (+0x3c), a done
flag (+0x90), the screen (+0xe4) and whether it runs modally (+0x138). Pushing a screen (0x421260) adds it to the
widget manager and, when modal, runs a nested Update/Draw loop until the screen ends (0x421340 sets the result) or the
program quits; the result is returned. Screens (15 classes, base slots: 2 contains-point, 6 handle-event...) are
rectangles in normalized coordinates with a sorted list of widgets.

## Resources (0x480b1c)

Three lists (fonts 0, images 1, sounds 2) of `{object, release time, type, references, key, prev, next}`, keyed by the
name's hash (0x41f5e0: case-insensitive, seed 0x3f6ab295) plus 1000 times the type. A released resource stays loaded
1.5 s (images 50 s) and is freed by the periodic sweep (0x421db0).

## Images (0x422de0)

An image is `{width, height, 32, pitch, frame count, frames, pixels, SDL_Surface}`. The loader tries the extensions of
the table at 0x4818dc in `Graphics/`, from the pak first (by base name) and then from disk; a 24-bit image is widened
to 32 bits (as an alpha map, white, while a font is loading); a `<name>_Matte.png` beside it gives the alpha (inverted:
alpha = 255 - matte); R and B are swapped so a pixel is 0xAARRGGBB; `<name>_ims.txt` lists the frames (`xywh x y w h`
or `xywh_ref`, with `//` comments), one frame for the whole image without it.

## Files and the pak

Files open through 0x429ee0: by base name from `Data/Bonnie.pak` first (handle 0x3039), else from disk with `_open`.
The pak (0x42f0b0) is an int, 50 bytes, the entry count and `{hash of the upper-cased base name, offset, size}` per
entry; an entry is read sequentially after a seek to its offset.

## The language table (0x429040)

`Data/Language.txt` holds the game's texts as `<number> "<text>"` lines with `**` comments and the escapes `\n`, `\r`
(also a line feed), `\t` and `\"`. Two passes: the first finds the largest number, the second fills the table. A text is
looked up by number (0x428f70). The port reads the player's file at run time; no text is in the repository.

## Sound (0x47c764)

FMOD 3 at 44.1 kHz with 64 channels. Samples are `Sound/<name>.ogg`, played with a volume from the options and an
optional pitch, or delayed by a queue checked each frame (0x41f2b0). Music is one module, `Sound/music.xm`,
`music.mod` or `music.oxm` (the first that exists), played from an order and stopped after a given order; or a stream
(`.ogg`, `.mp3`, `.wav`) played as a sample.
