# Bonnie's Bookstore's engine

Bonnie's Bookstore Deluxe (New Crayon Games, published by PopCap; the game exe inside `BonniesBookstore.exe` was linked
on 2006-11-10 with Visual Studio .NET 2003, linker 7.10) does not run on the SexyAppFramework, and none of its logic is
scripted: there is no Lua or other VM in the exe, and `Data/Bonnie.pak` holds only bitmap fonts. Everything is C++ in
the exe, on New Crayon's own small engine, which sits on SDL 1.2 (a 32-bit software surface), SDL_image (PNG and JPEG)
and FMOD 3 (samples and the module music). This file describes that engine as the binary has it. Addresses are those of
the game exe inside `BonniesBookstore.exe` (its image base 0x400000); nothing here comes from another project.

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

## The port

`engine/` is this engine translated, function by function with the original's addresses, and `game/` the game on top
of it. Where the original called SDL 1.2, SDL_image, FMOD 3 or Windows, the engine calls `bonnies::Host`
(`engine/Host.h`), one interface named after the calls it replaces. `src/BonniesHost.cpp` implements it on the
shared framework and platform layer:

- **Loop.** The game keeps its own loop (`bonniesMain`, and one in each modal dialog). `SexyAppBase::DoMainLoop` runs
  it, and the framework runs whenever the game sleeps (`Host::sleep`, the original's `Sleep` while it waited for the
  next frame) or pumps its events, at most every 100 ms then. On the web the sleep yields to the browser (ASYNCIFY).
- **Screen.** The engine draws into an 800x600 buffer of `0x00RRGGBB` pixels, as into SDL 1.2's software surface;
  the widget presents it through a `MemoryImage`. The game's cursors (32x32 data and mask bits) become the
  framework's image cursor.
- **Input.** The widget turns the framework's input into SDL 1.2's events: keysyms, a key-down's character
  (`SDL_EnableUNICODE`), the mouse in screen pixels with buttons 1 to 3, the focus. A typed character without a key
  (a script's `char`) comes as a key of its own. A test run has the input focus, as the user's window would.
- **Files.** Game files are read through `Platform::VFS` (case-insensitive); `SavedGames/` is the user's folder, read
  first and written there.
- **Sound.** FMOD's samples are the platform mixer's sounds and its channels the mixer's voices (made paused, started
  by `FSOUND_SetPaused`, with FMOD's 0-255 volume and frequency). The music, `Sound/music.oxm`, is an XM with Ogg
  Vorbis samples, which libxmp-lite cannot read: libopenmpt plays it (`Platform::RegisterOpenMPTOggXM`).
- **The exe.** `src/main.cpp` reads the game out of the player's `BonniesBookstore.exe` (the distributor's wrapper
  holds it after its marker), runs what its static initializers do to its data (`engine/Exe.cpp`) and copies the
  letter odds and the books out of it before the game starts. All of the game's data is read from it at run time.
- **Full screen.** `Data/Portal.cfg` can start the game in full screen. On the web the page's button does that
  (a browser enters full screen only on a click), so the game's request is ignored there.
- **Test hooks.** `-seed=N` seeds the game's `rand` instead of the time (`seedRandom`), so a script's boards repeat.
  A script's `focus 0` and `focus 1` take the window's focus away and give it back (the action game pauses).

### Comparing with the original

`tools/origrun` runs a copy of the original (the game exe placed in a copy of the install folder) hidden, and
`games/bonnies/tools/origcmp.py` compares its captures with the port's screenshots. origrun's captures of this SDL 1.2
window come out resampled by a constant sub-pixel offset (each pixel mixes 0.2 of its left neighbour and 0.4 of the
one above, fitted on the PopCap logo to within 2 levels), so the script applies the same mix to the port's frame
first. On 2026-11-01 the logos, the main menu (but the hanging sign, which swings), the new player dialog, the
difficulty choice and the first story page matched to within 3 levels; the later story pages differ only in the
arrow's pulse; the first board's fixed tutorial letters and its hint boxes are in the same cells (the other letters
come from `rand` seeded with the time, so they differ between any two runs).
