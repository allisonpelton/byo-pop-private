# Memory and processor time

What the native game holds in memory and where its processor time goes, measured on 2026-10-26, and what the `lean`
mod (`features.lean`, CODE_MODS_DESIGN.md 4.21) saves. The machine: Windows 11, a GTX 1080 with NVIDIA's Direct3D 11
driver, the native i686 build.

## 1. How it was measured

Three scripts in `tools/profile` do the measuring, all run under rundesk from the worktree root. `prof_run.py` lets
the skilled bot play 30 seconds of a Normal game, logs its input as a `-script` file, then plays that script again in
real time under `memprof.py`. `memprof.py` samples the game's working set, private bytes and processor time, and
takes snapshots of its resident pages by owner: each DLL and the exe, private memory, mapped files. The script also
carries the game's own `memreport` command (`src/testhooks/MemReport.h`), which lists what the game holds itself:
each image's pixels and caches, the sounds' samples, and the heap's blocks by size. `cpuprof.py` samples the threads'
instruction pointers about a thousand times a second, weighted by the cycles each thread used, and names the
functions from the exe's symbol table.

Two traps cost a measurement each. Under rundesk the window is never shown, but the game still draws, so the frame
counts are real: the replay drew 1,774 frames in 30 seconds. A saved game left by an earlier run brings up "continue
old game?", though, and then nothing moves and almost nothing is drawn. `prof_run.py` deletes the saves before each
run.

## 2. Where the memory goes

Sections 2 to 4 are the profile as it was made, before the framework changes of section 5. While playing, the
process has a working set of 57 to 68 MB and about 57 MB of private bytes. The working set by owner, from a snapshot
15 seconds into the game:

| Owner | Resident |
|---|---|
| The game's heap | 27.6 MB |
| Other private memory (thread stacks, the driver's allocations) | about 4 MB |
| Windows DLLs (51 of them) | 10.7 MB |
| The GPU driver and Direct3D (8 DLLs) | 9.3 MB |
| Mapped files | 2.3 MB |
| The game's exe and its DLLs | 2.4 MB |

The heap, from the game's own report:

| In the heap | Size |
|---|---|
| Images' pixels (108 images, 4 bytes a pixel) | 11.3 MB |
| Sounds' samples (21 sounds) | 7.4 MB |
| The framework's premultiply buffer | 2.4 MB |
| Everything else (music, fonts, strings, widgets, the driver's small blocks) | about 7 MB |

The game's files are 1.6 MB on disk, so nearly all of this is decoded data. Every image has 4 bytes a pixel, though
most of the files have 256 colours or fewer. The largest are the level-complete black hole (2304 x 256, 2.3 MB),
the gem sheet (2.2 MB), the board's two 640 x 480 backgrounds (this level's and the next one's, 2.4 MB), the title
screen (1.2 MB), the background art the board's backgrounds are made from (1.2 MB) and the 25 colours of the timer
bar (0.8 MB). The framework keeps every image's pixels after the GPU has its texture, since a texture cannot be read
back. The sounds are 24.6 seconds of Ogg Vorbis at 44.1 kHz, mono or stereo, which the mixer keeps as stereo float at
48 kHz, 8 bytes a frame. The premultiply buffer is where the framework premultiplies an image's pixels before an
upload. It grows to the largest image uploaded, the gem sheet, and stays that size; it is 2.4 MB rather than 2.2,
because a vector grows by doubling.

## 3. Where the time goes

A real game takes 4 to 6 % of one core at about 58 frames a second. Almost none of it is the game's code. In the
sampled profile 88 % of the cycles are in ntdll.dll, which for a 32-bit process under 64-bit Windows means system
calls (waiting, presenting a frame, the WOW64 layer between them). Another 4 % are the GPU driver and 2 % Windows'
window and message calls. The game's own code is about 2 %, and most of that is loading: decoding the GIF, PNG and
JPEG images and the Ogg sounds, 0.4 seconds at the start. So the cost is per frame and in the system and driver, and
drawing fewer frames is what saves time, not faster game code.

SDL's software renderer is no way out. It used a whole core, since it has no vsync and blends every pixel in
software, and it kept a second copy of every texture in the heap (54 MB of private memory instead of 32).

## 4. The lean mod

`features.lean` has two settings, both in the `lean` mod. Compact images (on by default) palettizes every loaded image
that has 256 colours or fewer, when loading ends: 1 byte a pixel and a colour table instead of 4 bytes a pixel.
SexyAppFramework's `Palletize` is lossless; it keeps an exact table or leaves the image alone. The textures upload
from the table, and an image that is drawn into or read later gets its pixels back. The title screen's picture is
let go once the title is gone, since nothing else draws it. The frame rate setting caps the frames drawn in a second
(30 by default; 50, 20 or as the original). The game still updates 100 times a second, so it plays at the same speed.

| Rules | Heap | Images' pixels | Processor time |
|---|---|---|---|
| The original | 28.3 MB | 11.3 MB | 4 to 6 % |
| lean, frame rate as the original | 24.1 MB | 7.2 MB | 5.7 % |
| lean, 50 frames a second | 24.1 MB | 7.2 MB | 2.9 % |
| lean, 30 frames a second (the default) | 24.1 MB | 7.2 MB | 1.9 % |
| lean, 20 frames a second | 24.1 MB | 7.2 MB | 1.7 % |

Processor time is a share of one core, and it varies by about a point from run to run. The gem sheet keeps its
4 bytes a pixel, since it has more than 256 colours once its alpha mask is applied, and so does the background art, a
JPEG. The board's two backgrounds are made from it during play, after loading, and are drawn into at every level.

## 5. What the framework saved

The rest of the heap was the framework's to change. Three changes were proposed to the coordinator, and two of them
have been in `main` since 2026-10-26:

- **Smaller sounds** (in `main`). Each sound is kept as 16-bit samples at its file's own rate, with its first one or
  two channels, and a voice resamples it as it mixes. The 21 sounds take 2.35 MB instead of 7.4. Keeping the Ogg data
  and decoding a sound as it plays would make it about 0.3 MB.
- **The premultiply buffer** (in `main`). Images are converted and uploaded 64K pixels at a time, so the buffer stays
  at 256 KB instead of 2.4 MB.
- **Pixels after upload** (not for now). SexyAppFramework let an image drop its pixels once the GPU had them
  (`PurgeBits`); the port keeps them, since a texture cannot be read back. An image loaded from a file could drop
  them and load the file again when they are needed (a lost device, a second texture filter, a software draw from
  it), which would free the gem sheet's 2.2 MB and most of the other images' pixels. The coordinator left it for now:
  reloading has to reproduce exactly what loading did, some code draws into images after they load, and no game's
  heap has been shown to be at risk.

The same moment of the same game as section 2, 15 seconds in:

| Rules | Heap before | Heap with the two changes |
|---|---|---|
| The original | 27.6 MB | 21.6 MB |
| lean, the default settings | 24.1 MB | 17.2 MB |

Dropping the pixels after upload as well would take the lean heap to about 12 MB, and the game's own data (images,
sounds and music) to about 5 MB.

## 6. Why not 3 MB

The game's data could come down to a few megabytes, but the process cannot. Windows' DLLs alone are 11 MB of its
working set, and Direct3D with the GPU driver another 9 MB, besides the driver's private allocations. A window with a
Direct3D device costs more than 3 MB before the game loads anything. Most of those pages are shared: a DLL is in
memory once for every process that uses it, so the working set Task Manager shows overstates what the game costs the
machine. Private bytes are the fairer measure of the game's own use, and they are what the lean mod and the framework
changes above bring down.
