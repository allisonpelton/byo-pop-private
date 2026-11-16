# Memory and processor time

The ports used two to three times the memory of the original games: Big Money 124 MB of private bytes in its notes'
scripted minute against the original's 42 MB, Bejeweled 2 88 MB at the title against 40, Atomica 166 to 183 MB against
64 to 67. This is the framework work of 2026-11-14 that set out to bring that down without changing a pixel or a sound:
what the memory was, how it was measured, what each change saves in memory and processor time, and what was looked at
and left. The rule for every change was that every frozen baseline and every shot stays identical, that nothing runs on
a thread of ours, and that speed comes from not waiting. A second part, of 2026-11-15, brought Big Money to the
original's own figures and asked what a 2D game spends on Direct3D ("Big Money at the original's memory"). A third,
the same day, found why Atomica's processor time had grown fourfold on Direct3D 11 and chose the renderer again ("Which
renderer, and SDL's batching"). A fourth lets a port whose original starts in 2D keep the 2D flags on the GPU path ("The
2D flags on the GPU path").

## How it was measured

Private bytes are the system's own count of the memory a process has committed for itself (`PrivateUsage`, Task
Manager's "commit size"), read from outside the game every 50 ms; a run's peak is the largest of those readings. The
processor time is the game process's cycle count (`QueryProcessCycleTime`), all its threads together, the graphics
driver's and the audio thread's included. The game's own `memreport` command (`docs/TEST_COMMANDS.md`) splits the
memory it knows of: the pictures' pixels and textures, the sounds, the music, and the heaps' blocks by size.

Every port ran under rundesk, and the configurations of one comparison ran interleaved, run by run, because other
sessions' games shared the machine throughout (three to six game processes) and a run's processor time moved by a
fifth or more with that load. A comparison runs the same exe with the new code switched off, where a switch exists:

| Switch | What it does |
|---|---|
| `BYOPOP_KEEPBITS=1` | keeps every picture's pixels after its upload (step 1 off) |
| `BYOPOP_DROPCHECK=1` | checks each picture made again from its file against a hash of the pixels dropped, and logs any difference as `DROPCHECK mismatch`; `=2` also makes every picture again the moment it is dropped |
| `BYOPOP_NOBATCH=1` | draws every quad with its own SDL call and fills every span on its own (step 2 off) |
| `BYOPOP_NOUPLOADFLUSH=1` | leaves the uploads' commands to the next present (the upload flush off) |

**Runs with `-fast` measure builds against each other, not against the originals.** With `-fast` the port hands the
driver frames faster than the GPU shows them, and Direct3D 11's driver keeps memory for the queued ones: Big Money's
bot game with the same exe ended at 95 to 98 MB of private bytes with `-fast`, after a peak of 139 to 148 MB, and stayed
at 64 to 70 MB in real time. The heap was 31 to 32 MB in both. So the like-for-like figures below, those set beside an
original's, are real-time runs.

## Where a port's memory went

Big Money is the worked example, the game with the widest gap. The original holds 37 MB of private bytes at its title
and peaks at 42 MB in play (its notes' performance section). The port as merged held 84 MB at the title and peaked at
86 MB in a real-time bot game here (124 MB in the notes' scripted minute). Of that:

**The Direct3D 11 device takes about 30 MB** as soon as SDL's renderer exists. A test program that makes only a hidden
window and the renderer holds 29.7 MB; with SDL's OpenGL renderer it holds 70.8 MB, with SDL's software renderer 4.3
MB. Making the device also takes most of `MakeWindow`'s 300 to 350 ms at start: a bare `D3D11CreateDevice` takes 126 to
132 ms (the driver's loading), SDL's own setup about 45 ms more. Neither can be had back without another renderer.

**The textures cost nothing; their uploads did.** A texture lives on the GPU: a render target of 2560x1920, the screen
at render scale 4, costs 0.1 MB of private bytes. But SDL copies each upload from a staging texture that it releases at
once, the runtime keeps that copy until the GPU has run it, and the driver keeps the memory afterwards for later use. A
burst of uploads, such as a screen's first draw, left as many private bytes as it uploaded: 50 textures of 256x256
(12.5 MB) left 13.0 MB, and destroying all fifty gave none of it back. Sending the commands to the GPU after each strip
let the driver reuse the last strip's memory: the same burst then left 2.2 MB, in the same time.

**Every picture kept its pixels after its upload**, as a copy of what the texture holds: 20.4 MB at Big Money's title,
41.6 MB on Bejeweled 2's board.

**The music was loaded fourteen times.** Big Money loads its one module, 392 KB, once for each of its 14 tunes, and each
load made a libxmp module with its own patterns and samples and started its player: 17 MB of heap, where a session plays
about six of the tunes.

**The sounds are as the original's.** Every sound in these games is Ogg Vorbis but a handful of WAVs (the 8-bit ones,
in Cosmic Bugs, Dynomite, Hammer Heads, Water Bugs and Word Harmony, add up to under 0.1 MB), decoded to 16-bit samples
at their own rate as the original's DirectSound buffers held them: 5.4 MB in Big Money, 6.5 MB in Atomica.

## What changed, and what each change saves

The changes are in the order they were made, each in a commit of its own on `claude/fw-memory-5fbaa7`.

**1. Pixels dropped after upload** (`MemoryImage::DropBits`, as PopCap's `PurgeBits`). A picture loaded from a file keeps
no pixels once its texture is up. They are made again from the file, with the same alpha file or grid, when something
needs them: a lost device, a second filter's texture, a software draw, or `GetBits`, after which the picture keeps them.
Pictures drawn into after loading, made by a game's generator or carrying HD pixels keep theirs, and a palette picture
stays 8-bit until its upload. With `BYOPOP_DROPCHECK=2` no picture made again differed from the one dropped, in
Bejeweled 2's bot game or at any title of the set below. In the bot games of Bejeweled 2, AstroPop and Heavy Weapon
(300 moves with `-fast`, this step's build against the build before it) the pixels held at the end fell by 11 to 13 MB
(39.6 to 26.9 MB in Bejeweled 2) and the peaks by 6 to 10 MB, with the processor time within the runs' noise.

**2. Quads drawn in batches, spans joined.** SDL makes one draw call of each `SDL_RenderGeometry` and joins none, and on
Direct3D 11 the driver's work for the draw calls took half of the processor time of some games: 49 % of Typer Shark's
in 3D, 55 to 60 % of Venice's. The textured quads and triangles with one texture, blend mode and clip now go to SDL as
one call (`DDInterface::AddQuads`, `FlushQuads`), the same triangles in the same order; any other draw, a read of the
target, a clip or target change and any change to a texture flush them first. A call with a u or v outside 0..1 goes
to SDL on its own, as before: SDL 2 refuses such a call whole, and in a batch it took the quads gathered with it along
(Bejeweled 2's hyperspace tunnel in the 3D whirlpool, found when the transform stack came in; the whirlpool's shots
are now the same with and without the batch). A quad's vertices go through the transform stack's top matrix before
it is queued, so a push or pop needs no flush. Solid fills join their spans into rectangles. Only an accelerated renderer gathers quads: SDL's software renderer turns each call's pairs of triangles back
into copies. (SDL 2.32's Direct3D 11 renderer issues one `Draw` for each geometry command, `SDL_render_d3d11.c`; Typer
Shark's notes took it that SDL merges the calls of one texture, which its OpenGL renderers do and this one does not.)

Typer Shark in 3D, its performance pass's real-time scenario on rundesk's desktop, the same exe with and without
`BYOPOP_NOBATCH`, four pairs turn about: about 170 SDL draw calls a frame became 21, and the Direct3D 11 driver's worker
thread spent 1,174, 1,160, 1,444 and 1,597 M cycles in the 20 s stretch against 1,443, 1,865, 1,796 and 1,687 M, a
quarter less at the median (1,302 M against 1,741 M). The process as a whole was 3,961 M against 4,645 M at the median,
but its main thread spins in `SDL_RenderPresent` on a desktop with no vertical blank, so that figure is noisy there. A
game in 2D draws in software and sends one frame a present, so it never reaches the batch: with 3D off (as the scenario's
own save has it) no quad was gathered, and Bejeweled 2's, AstroPop's and Heavy Weapon's bot games moved by no more than
the machine's noise.

**3. Uploads sent to the GPU every 2 MB** (`DDInterface::NoteUpload`, `platform/GPUFlush.cpp`): every picture is
uploaded in strips of 256 KB, and after each 2 MB uploaded the immediate context's `Flush` sends the copies to the GPU.
Nothing waits for it. What it saves depends on how much a game uploads between two presents, since a present sends the
commands too. In real time, held at the first board, turn about with `BYOPOP_NOUPLOADFLUSH=1`, Bejeweled 2 peaked at
98.9 and 97.2 MB against 102.6 and 102.9 MB.

The interval was measured. A flush after every 256 KB saved a little more in Bejeweled 2 (93.1 to 95.9 MB in four runs)
but cost Bejeweled Twist's start 1.2 to 1.5 G cycles in each of its pairs, about 480 flushes for its 121 MB of pictures
at a millisecond each; every 2 MB cost Twist no more than the noise (11.95 and 10.98 G cycles against 11.28 and 10.84
G). At 256 KB, Heavy Weapon, Bejeweled Twist, AstroPop, Dynomite and Typer Shark were between 2 MB lower and 0.5 MB higher
without the flush, and Big Money, whose loading screen presents between its pictures, 1.3 MB higher with it (67.0 to
67.3 MB against 65.6 to 66.0 MB).

**4. A module's player made when its song is first needed** (`Mixer::OpenModule`): `LoadSong` keeps the module's file,
one copy for songs loaded from the same bytes, and the module is loaded and its player started the first time the song
is played or looked into. A first play costs about 2 ms; the 14 loads at Big Money's start take 8 ms instead of 28.

**5. GIFs through our own decoder** (`platform/GIFDecoder.cpp`), which follows stb_image's first-frame decode step by
step, its quirks included, and differs only in how it does it: a code's pixels are copied from where its prefix was
written instead of walked by recursion, straight into the 32-bit pixels. All 11,293 GIFs staged under `assets/` decode
to the same pixels and alpha flag as stb's; Mummy Maze's 237 take 45.8 ms instead of 87.9, all of them 2.1 s instead of
4.4.

### Big Money, like for like

Real-time bot games (seed 1, skill 3), memory reports every 750 updates, stopped after 5,250 updates, two pairs turn
about; the original's figures are from its notes (the title, and its scripted minute's peak).

| | Original | Port before | Port after |
|---|---|---|---|
| Private bytes at the title | 37 MB | 86.4, 86.5 MB | 64.4, 67.7 MB |
| Peak private bytes in play | 42 MB | 86.4, 86.4 MB | 67.0, 69.0 MB |
| Heap at the end | | 52.6, 52.5 MB | 30.8, 31.5 MB |
| Processor time of the run | | 16.4, 11.7 G cycles | 15.5, 10.8 G cycles |

The new build was the lower in both pairs, by 0.9 G cycles each time, with every step in it.

### Thirteen games at a first board

Each game started in real time, went to the first board of a mode with the `goto` command and was held there, with a
memory report at the end of the run. The build before (main as it was) and the build after (this branch with main
merged in) ran turn about, in two rounds with the order swapped. The originals' figures are from each game's notes,
measured in their own scenarios, so they stand beside these as a guide, not run for run.

| Game (mode) | Original (its notes) | Before | After | Pixels held, before and after |
|---|---|---|---|---|
| Bejeweled 2 (Classic) | 42.8 MB at the board | 103.5, 106.3 MB | 94.8, 94.1 MB | 38.7, 35.1 MB |
| Big Money (selector) | 37 MB at the title, 42 at the peak | 82.3, 82.2 MB | 62.1, 62.0 MB | 20.5, 18.3 MB |
| Bejeweled Twist (Classic) | 158 MB at the menu | 303.5, 287.4 MB | 270.8, 267.1 MB | 141.3, 121.2 MB |
| Dynomite | 60 MB at the title, 65 at the peak | 83.2, 82.2 MB | 76.9, 76.7 MB | 34.1, 31.1 MB |
| AstroPop (Classic) | 71 MB at the title, 82 to 86 at the peak | 122.6, 122.5 MB | 119.9, 117.4 MB | 51.6, 44.9 MB |
| Alchemy (Strategic) | 51 to 56 MB at the peak | 62.0, 61.8 MB | 58.2, 60.6 MB | 22.6, 21.4 MB |
| Seven Seas (Easy) | 41.5 to 51.2 MB at the peak | 61.3, 58.9 MB | 58.1, 57.2 MB | 18.7, 17.4 MB |
| Bookworm (Classic) | 39 to 41 MB at the title | 79.5, 77.2 MB | 76.3, 76.4 MB | 10.2, 9.0 MB |
| Atomica (Strategy) | 64 to 67 MB at the peak | 90.3, 94.1 MB | 88.4, 91.1 MB | 43.9, 42.6 MB |
| Bejeweled (Normal) | 36 MB | 46.5, 46.1 MB | 44.9, 44.4 MB | 9.0, 7.8 MB |
| Typer Shark (Adventure) | 49 to 51 MB in play | 73.1, 73.1 MB | 72.8, 74.7 MB | 19.6, 19.1 MB |
| Heavy Weapon (Campaign) | | 107.9, 109.2 MB | 109.0, 108.8 MB | 41.0, 39.9 MB |
| Venice (Journey) | | 36.8, 36.1 MB | 38.2, 36.3 MB | 5.5, 5.5 MB |

Before and after are the runs' peak private bytes. Ten of the thirteen held less after, Big Money and Bejeweled Twist by 20 MB
or more, Bejeweled 2 by 9 to 12 MB, Dynomite by 6 MB, the rest by 1 to 5 MB; Typer Shark, Heavy Weapon and Venice moved
by no more than their runs differ among themselves. The pixels column falls only by the pictures a first board has
drawn, since a picture keeps its pixels until its upload: most of every game's pictures are not drawn yet there. The
sounds were the same in both builds, as they should be.

The processor time of a held board did not move. The thirteen runs took 86.7 G cycles before against 79.1 G after in
the first round, where the build before ran first, and 71.6 G after against 68.8 G before in the second, where the build
after ran first: whichever ran first was the higher, by more than the builds differ.

## Big Money at the original's memory (2026-11-15)

After the first part the user asked whether Big Money could be brought to the original's memory, 37 MB of private bytes
at the title and 42 MB at the peak, and then whether these 2D games need Direct3D at all, for memory and for processor
time. Each step was measured in real time against the original's figures from Big Money's notes (its performance pass:
42.1 MB at the peak of its scripted minute, the bot's Action game at skill 1 with seed 21).

**The answers.** Big Money on the software screen in the original's own window (640x480) holds 36.2 to 36.7 MB at the
title and peaks at 40.7 to 41.8 MB in that minute, against the original's 37 and 42.1, once step 7 below had taken out
the cache that made it peak at 47 to 49 MB. In about one run in five Windows adds 3.7 MB of its own (the oddity under
"Open"). Without Direct3D a 2D game saves the device's 28 to 30 MB at every window size, but it does not save processor
time on this machine. At 1x the software screen costs the game about what the GPU path does (Dynomite's medians 9.8 G
cycles against 10.2, the pairs going either way); at 2x it costs about a sixth more (Dynomite 10.6 G against 9.1, Big
Money 18.6 against 15.6), GDI's stretch on top of the same software draws. What dwm and the kernel spend on either could
not be told apart on a machine that was never quiet. So the GPU stays the default above 1x, and a port may take the
software screen there for its memory ("Windows at 2x to 4x").

### What the original runs in

The original starts in 2D. In its export, MakeWindow (0x4896a0) and the DDInterface constructor (0x44e730) leave mIs3D
at 0 and mAutoEnable3D at 0. The registry's Is3D is read only when it is there, and the user's install has neither Is3D
nor a Test3D key. Under origrun on rundesk's desktop DirectDraw has no hardware (`docs/ORIGRUN.md`), so the original
draws in software there too. Its 37 and 42 MB therefore hold no graphics driver at all. The port started on the GPU
whatever the registry said, since the framework's wish for 3D defaulted to on.

### Where the Direct3D 11 device's memory goes

A test program that makes the device and nothing else, on this machine's GTX 1080 (driver 32.0.15.6094), holds 27.5 MB
of private bytes. The writable image of the user-mode driver, `nvwgf2um.dll`, is 18.6 MB of that, 18 MB of it the
driver's .data section; `nvgpucomp32.dll` adds 0.8 MB, the heaps 3.2 MB and other private regions 2.8 MB. Nothing a
program asks of the device moved this by more than 0.5 MB: single-threaded, no internal threading optimisations, feature
level 10_0 or 9_3, a frame latency of one, and a discard swap chain with one buffer were each tried. WARP, the software
rasteriser behind the same interface, holds 2.6 to 4.1 MB, and SDL's direct3d11 renderer 31.0 to 31.9 MB. The 28 MB is
the NVIDIA driver's own, and only another way of showing the frames avoids it.

Direct3D 9 is not that way. On rundesk's desktop its CreateDevice fails (D3DERR_NOTAVAILABLE) and SDL falls back to
Direct3D 11. On the visible desktop a bare Direct3D 9 device holds 20.3 to 22.4 MB, `nvd3dum.dll`'s writable image 10.9
MB of it. SDL's direct3d renderer started at 20.4 MB but held 94.4 MB after 120 frames of 200 copies each, its heap
grown to 72.5 MB, so it saves at most 10 MB at the start and gives that back under load. (The test program named the
renderer, so SDL drew unbatched; batched, in the games, Direct3D 9 held about what Direct3D 11 held, except in AstroPop's
3D: "Which renderer, and SDL's batching".) SDL's software renderer holds
4.0 to 4.5 MB. (The framework asks SDL for direct3d11 by name, so a port's GPU path is Direct3D 11 on every desktop.)

### 6. Pictures uploaded as they are loaded

A picture the resource manager loads (`ResourceManager::DoLoadImage`) now gets its texture at once in 3D and drops its
pixels then (`DDInterface::UploadLoadedImage`), so that the pictures loaded and not yet drawn hold none either: a texture
costs no private bytes. A draw that needs the pixels again (a software draw, a read of them, the other filter) makes them
from the file, as after any drop (step 1). A display that keeps textures of its own (`GPUDisplay`) does nothing here, and
the software screen has no textures. `BYOPOP_NOLOADUPLOAD=1` in a test run turns it off.

It reaches only the games that load through the framework's resource manager: of the thirteen, Bejeweled 2 and AstroPop
(Bejeweled Twist does too, but draws through `GPUDisplay`), and beyond them Bookworm Adventures, its sequel and Hammer
Heads. The older games (Atomica, Big Money, Bejeweled, Venice, Typer Shark and the rest) load each picture through
`SexyAppBase::GetImage` in their own code, which this does not touch ("Open"). Each was held at its first board in real
time, as in the table of thirteen, three rounds turn about with and without the upload:

| Game | Peak private bytes, with / without | Pictures' pixels, with / without | G cycles, medians, with / without |
|---|---|---|---|
| Bejeweled 2 | 86.4 to 87.1 / 96.1 to 98.8 MB | 8.0 / 26.9 MB | 12.2 / 12.7 |
| AstroPop | 102.8 to 104.1 / 121.8 to 124.4 MB | 9.6 / 39.6 MB | 17.5 / 18.6 |
| Bejeweled Twist (not reached, held while loading) | 250.5 to 258.4 / 256.2 to 259.1 MB | 121.2 / 121.2 MB | 9.4 / 8.9 |

Bejeweled 2 holds 10 MB less and AstroPop 19 MB less. Processor time did not rise: the pairs went either way (each game
lower with the upload in two of its three), and Bejeweled Twist, which the change does not reach, spread as much between
its own pairs. With `BYOPOP_DROPCHECK=2` (every dropped picture made again at once and compared) no picture differed:
Bejeweled 2 made 19 pictures again from their files at its first board (its gems among them) and AstroPop
41, each the same as the one dropped. Every frozen baseline stayed identical (`tools/check.sh --changed main`,
which ran these games, Bookworm Adventures, its sequel and Hammer Heads among them).

### The software screen for Big Money

Big Money's own session has since given it Dynomite's software screen (`SoftwareScreenDisplay`) at 1x; here it was
measured through a test switch (`SOFTWARE_SCREEN=on`) that does the same for a game that keeps DDInterface, windowed at
the game's size with the original's settings (`games/bigmoney/tools/origcmp/settings.txt`, `-windowscale=1`). Its
pictures then come from the 2004 blitters, as the original's 2D mode draws them, and nothing is uploaded: the window
shows the frames through its own surface, with no renderer and no Direct3D device.

At the title it held 36.2 to 36.7 MB, against the original's 37 MB and 62 to 65 MB on the GPU path. Of that, the heap
was 32.2 MB, the pictures' pixels 21.7 to 22.8 MB of it (nothing is uploaded, and every picture keeps its pixels for the
blitters), and the modules' writable images 0.9 MB. The visible desktop gave the same figure (36.4 against 36.7 MB on
rundesk's). But every run's peak, 46.9 to 49.0 MB (four runs, one of them on the visible desktop), came in the second
between the title and the first board, not in the minute itself (40.5 to 41.9 MB). That was step 7's.

### 7. Stretched copies dropped on the software screen

Memory reports every few updates through that second found 25 pictures more while the game selector was up, all 467
pixels wide and of different heights, 7.6 MB of pixels and 1.7 MB of their alpha caches, gone when the selector closed.
The selector rolls open by stretching the middle of its panel to a new height in every frame, and the software stretch
(`DDInterface::GetStretchCopy`, an earlier shared change: the source stretched into a picture of the destination's size,
then blitted, as the original's software DDImage does) kept each stretched picture in a cache of 64 pictures or 64 MB.
That cache was made for the GPU path, where each copy has a texture worth keeping. The original keeps none. The loading
screen's bar filled the same cache before the title.

A software screen now drops the stretched pictures not drawn in the last two frames after each present
(`SoftwareScreenDisplay::Present`, `DDInterface::DropStretchCopiesDrawnBy`); one drawn in every frame is kept, and the
GPU path keeps its cache as it was. In the same minute, in real time on rundesk's desktop and the visible one: the title
36.2 to 36.7 MB, the selector 40.0 to 40.3 MB instead of 46.9 to 49.0, the first board 38.3 MB, and the minute's peak
40.7 MB on the visible desktop, 41.6 MB on rundesk's before the run's own screenshot (which took it to 46.5 MB for a
moment) and 41.8 MB in three more runs on rundesk, against the original's 37 and 42.1. With a memory report at the same
updates, the selector's rise above the title fell from 10.2 MB to 3.8.

The drop costs no measurable processor time. Dynomite's scripted minute (its notes' `pairs.sh` input, 6,000 updates in
real time on a core of its own, three runs of each turn about; the second batch below) took 12.4 G cycles with it and
13.8 G without at 1x, and 12.4 G with it and 11.6 G without at 2x: the runs of one build differ by more.

Every display now also holds at most two sizes of one source rectangle at one sampling (2026-11-15,
`kStretchSizesPerKey`), at Big Money's session's request. A third size replaces the one drawn longest ago, so a picture
drawn at two sizes in a frame keeps both, and a stretch that grows or shrinks frame by frame holds two copies. Before, a
display with the software stretch that drops nothing after a present (Bejeweled Twist's) held one for each size, up to
64 pictures or 64 MB. The sampling is part of the key too (pixel centres or the corner, which Big Money's countdown
changes between draws), so a copy made one way is never drawn for the other. The cost falls on a game that draws one
rectangle at three or more sizes in every frame: it would make a copy at each draw. None is known, and every game's
checks pass unchanged. memreport's file now ends its pictures with a line for the cache (`stretched copies: N, at most M
of one source rectangle, X MB of pixels`). Through Big Money's selector rolling up as a game starts, with a memory report
every three updates at 1x and at 2x in software, the cache held three or four copies, at most two of one rectangle, 0.75
to 1.32 MB, and none after. On the software screen the drop after each present had already kept it there; the cap holds
it where nothing is dropped.

### Windows at 2x to 4x

Above 1x the software screen handed itself to the GPU wherever an accelerated renderer existed, because copying the
buffer into SDL's window surface with each pixel repeated cost more processor time than the GPU. So the usual window,
2x, still had the device's 30 MB. `SurfaceDisplay`, the display of the Mahjong games, Tip Top, Bonnie's, Cosmic Bugs,
Water Bugs and Word Harmony, shows its frames that way at 2x to 4x.

GDI's own stretch was measured against that copy and against the GPU: each changed rect went from the 1x buffer straight
to the window in one StretchDIBits with COLORONCOLOR (nearest) sampling, and the window kept no surface of its size. Its
frames were read back from the window and compared with the buffer at every present: none of 901 differed, at each size.
While this was measured, the Mahjong session's stretch mode reached the trunk (`framework/SurfaceDisplayGdi.cpp`, for
window sizes that are no whole scale): it stretches the whole buffer with the changed rects as the clip region, which
its session found as cheap as the rect alone and exact at any size. Surface mode at 2x to 4x now presents through that
same `GdiStretch` (`SurfaceDisplay::PresentScaledByGdi`), and this branch's own stretch went; `SURFACE_VERIFY=1` checks
those presents as it checks stretch mode's.

**On the visible desktop**, between 01:00 and 01:50 (night hours allow it): Big Money's scripted minute on the software
screen (the GPU path for the GPU rows), three rounds turn about with the order reversed in the middle one, each window
shown without taking the focus, at below-normal priority, the game on a performance core of its own
(`tools/pincore.py`). Over the minute: the game's processor time, dwm.exe's and the System process's (from the system's
process list, since neither can be opened from an unelevated shell), and the game's peak private bytes. Medians of the
three:

| Window | Path | Game | dwm | System | Sum | Peak private bytes | Refreshes showing one new frame |
|---|---|---|---|---|---|---|---|
| 1x | GPU | 6.0 s | 11.3 s | 28.5 s | 47.3 s | 69.8 MB | 97 % |
| 1x | software screen | 5.7 s | 10.0 s | 29.4 s | 46.3 s | 50.2 MB | 71 % |
| 2x | GPU | 5.3 s | 9.9 s | 35.4 s | 51.7 s | 77.2 MB | 90 % |
| 2x | repeated copy | 11.5 s | 10.8 s | 30.1 s | 52.4 s | 53.8 MB | 85 % |
| 2x | GDI stretch | 5.8 s | 11.5 s | 30.4 s | 48.3 s | 49.1 MB | 73 % |
| 3x | GPU | 6.4 s | 9.6 s | 30.7 s | 46.9 s | 89.9 MB | 93 % |
| 3x | repeated copy | 15.0 s | 8.0 s | 25.6 s | 48.0 s | 59.7 MB | 63 % |
| 3x | GDI stretch | 4.2 s | 7.7 s | 29.7 s | 41.5 s | 49.1 MB | 68 % |
| 4x | GPU | 5.6 s | 9.5 s | 34.8 s | 48.7 s | 108.5 MB | 90 % |
| 4x | repeated copy | 26.6 s | 8.7 s | 20.4 s | 54.4 s | 68.1 MB | 82 % |
| 4x | GDI stretch | 5.2 s | 7.5 s | 24.3 s | 41.3 s | 49.1 MB | 64 % |
| | no game (a minute idle) | | 9.9 s | 32.5 s | | | |

These runs predate step 7 and wrote a present log, which costs about 5 MB itself, so their peaks stand against each
other only.

**The whole machine's sum could not tell the paths apart.** The machine was not quiet: other sessions' games ran
throughout (one to six at the start of each run), and the machine was 42 to 99 % busy. With no game of ours the System
process took 20.7 to 38.3 s a minute and dwm 9.8 to 10.9 s, more than any path's own share, so the dwm, System and sum
columns move by 10 s between runs of one path. The sums at 2x were 51.7 s on the GPU and 48.3 s with GDI's stretch; at
3x 46.9 and 41.5 s; at 4x 48.7 and 41.3 s; every one of those differences is inside that noise. A quiet machine would be
needed to say what dwm and the kernel spend on each path, and none could be had while other sessions ran.

**On rundesk's desktop, pinned**, the like-for-like desktop of the originals' figures, the game's own processor time was
measured again as cycle counts (`QueryProcessCycleTime`), three runs of each turn about. Big Money's whole scripted run
(load, title, selector and the minute, about 91 s) on the software screen, and Dynomite's scripted minute. The first two
batches ran while the compositor wait below was the default, so their software rows wait for DWM; the third batch ran
with the framework's pace, the default then (the wait is the default again since the user chose it, below), and set the
wait against it:

| Batch | Game | Path | G cycles, median (range) | Peak private bytes |
|---|---|---|---|---|
| 1 | Big Money | GPU at 2x | 15.6 (14.6 to 16.9) | 67.4 to 70.2 MB |
| 1 | Big Money | software screen at 2x, GDI's stretch, the wait | 18.6 (16.2 to 21.1) | 39.8 to 43.9 MB |
| 1 | Big Money | software screen at 1x, the wait | 19.5 (19.0 to 23.2) | 41.8 MB |
| 2 | Dynomite | GPU at 2x | 9.3 (7.8 to 10.0) | |
| 2 | Dynomite | software screen at 2x, GDI's stretch, the wait | 12.4 (11.8 to 13.2) | |
| 2 | Dynomite | software screen at 2x, repeated copy, the wait | 23.6 (22.5 to 26.2) | |
| 2 | Dynomite | software screen at 1x, the wait | 12.4 (12.2 to 13.6) | |
| 3 | Dynomite | GPU at 1x | 10.2 (8.7 to 11.8) | |
| 3 | Dynomite | software screen at 1x | 9.8 (9.4 to 13.4) | |
| 3 | Dynomite | software screen at 1x, the wait | 11.6 (9.6 to 13.5) | |
| 3 | Dynomite | GPU at 2x | 9.1 (8.7 to 9.2) | |
| 3 | Dynomite | software screen at 2x, GDI's stretch | 10.6 (9.3 to 11.7) | |
| 3 | Dynomite | software screen at 2x, GDI's stretch, the wait | 11.3 (9.4 to 11.8) | |

GDI's stretch costs little in itself: either game's software screen costs about as much at 2x as at 1x (Dynomite's 8 %
more in the third batch), where the repeated copy cost Dynomite nearly twice as much. At 1x the software screen and the
GPU path cost about the same, the third batch's pairs going either way (software 13 % and 8 % above the GPU in two, 4 %
below in one), as on the visible desktop (5.7 s against 6.0). At 2x the GPU is cheaper, by about a sixth in either game
(Dynomite 9.1 G against 10.6, all three pairs; Big Money 15.6 against 18.6), and the visible desktop showed the same
more faintly (5.3 s against 5.8). One batch's runs spread by a third and the batches by more, for the machine was busy
with other sessions' games throughout, so only the pairs within a batch are set against each other.

Nor does this branch cost the software screen more than before it. Dynomite's frozen baseline (built 2026-11-12), the
trunk and this branch (with the wait, then the default), turn about at 1x, took 7.6 G (6.8 to 8.6), 7.4 G (6.6 to 8.1)
and 7.9 G (7.0 to 8.0) in a fourth batch, the baseline as much as the others; the 8.46 G of Dynomite's notes was a
quieter day's figure.

So `SurfaceDisplay` now stretches with GDI at 2x to 4x on Windows (`SURFACE_PRESENT=copy` in a test run gives the copy
back): those games have no GPU path there, and the stretch costs them half of the copy or less. If GDI fails, the frames
are copied into the window's surface from then on. The software screen stays on the GPU at 2x to 4x, as before, unless
the port asks for software there with `SoftwareScreenDisplay::Create`'s third argument (`theScaledInSoftware`), which
saves the device's 28 to 30 MB at the usual window for the processor time above. No port asks yet; the choice is each
game's session's.

### The compositor wait, the default

The last column of the visible desktop's table is the frame pacing. A vsync present waits for the vertical blank, so
each refresh shows one new frame; the window's surface has no vertical blank, and the framework's own pace (whole
milliseconds, placing the frames on the update loop's 10 ms steps) showed a frame twice or skipped one on a quarter to a
half of the refreshes. A scene that moves steadily, as the selector's roll or a falling piece, then moves unevenly.

Each present without a renderer on Windows can wait for the desktop compositor's next composition (`DwmFlush`,
`GdiStretch::WaitForComposition`), as a vsync present waits for the blank. The wait is skipped with `-fast`, `-drawevery`
and `-hold`, as the pace is, and before the game has loaded. A wait that fails, or that holds a frame for more than
250 ms (a compositor that is not composing), hands the frames back to the pace for good. The same minute again on the visible desktop, three runs of each, every present's time set against DWM's vertical
blanks (`DwmGetCompositionTimingInfo`; this display runs at 60.002 Hz):

| Window | Path | Refreshes showing one new frame | Interval between presents, median (5th to 95th percentile) | Peak private bytes |
|---|---|---|---|---|
| 2x | GPU, vsync | 100 % | 16.68 ms (14.92 to 18.37) | 78.0 MB |
| 1x | software screen, the pace | 51 % (41 to 66 % by run) | 19.3 ms (9.0 to 21.5) | 50.1 MB |
| 1x | software screen, waiting for DWM | 100 % | 16.67 ms (16.09 to 17.23) | 50.3 MB |
| 2x | GDI stretch, the pace | 65 % | 18.9 ms (8.1 to 24.3) | 48.9 MB |
| 2x | GDI stretch, waiting for DWM | 100 % (78 % in one run with the machine 91 % busy) | 16.65 ms (15.37 to 17.97) | 49.4 MB |
| 3x | GDI stretch, waiting for DWM | 100 % | 16.67 ms (15.50 to 17.84) | 49.4 MB |
| 4x | GDI stretch, waiting for DWM | 100 % | 16.67 ms (15.83 to 17.51) | 49.3 MB |

With the wait, the software paths show a moving scene as evenly as the vsync GPU path: one new frame each refresh, and
their presents' intervals spread less than the GPU's. On rundesk's desktop the wait paces as well (58 presents a second,
median 16.65 ms). These rounds ran one path after another while the machine went from 27 to 90 % busy, so their
processor times are not set beside each other here. The cost was measured on rundesk's desktop, pinned, turn about, in
Dynomite's scripted minute (the table's last rows): the wait took more of the game's processor time in all six pairs, by
1 to 18 %, about 2 % in the median pair (1x 11.6 G against 9.8, 2x 11.3 against 10.6 in the medians), the cost of the
call and of presents that now meet the compositor's work.

**The user made the wait the default on 2026-11-15** ("Make it the default"), for that evenness at about 2 % of the
game's processor time: every present to the window's surface on Windows (the software screen at 1x, `SurfaceDisplay` at
every size) waits for the compositor, with the skips and the fallback above. It blocks the main loop for up to a
refresh, as a vsync present does. `SURFACE_PACE=ms` in a test run gives the framework's millisecond pace back, to
measure against; elsewhere than Windows the pace stays. Since `-fast`, `-drawevery` and `-hold` skip the wait, scripted
runs and the frozen baselines are untouched by it (all unchanged after the switch). The fallback is for good: a session
in which one wait fails or holds a frame over 250 ms (a compositor that stops composing) stays on the pace until the
game is started again.

### The picture at each size

Shots of the scripted minute every 1,000 updates, compared pixel for pixel (`screenshotfull`, the window's own pixels):

| Display, size | Before this part | Now | The picture |
|---|---|---|---|
| Software screen (Dynomite, Big Money), 1x | software screen | the same | the same |
| Software screen, 2x to 4x windowed | GPU path | the same, unless the port asks for software | the same; with software asked for, the 1x software picture with each pixel repeated, where the GPU's picture differed from it by 38,000 to 94,000 pixels a frame at 1x scale (alpha blends rounding by one or two levels, at most 54 to 55, and a strip along Big Money's board top) and, at Big Money's title, by one scaled picture the GPU draws finer at the render scale (45,000 to 180,000 pixels, at most 81 to 122 levels) |
| SurfaceDisplay, 1x | window surface | the same | the same |
| SurfaceDisplay, 2x to 4x | the repeated copy | GDI's stretch | the same: every shot identical, and none of 901 frames read back from the window differed from the buffer |
| Any display, full screen at a size that is no whole multiple | GPU, linear filter | the same | the same |
| DDInterface (the GPU path), any size | GPU | the same | the same |

The GPU path's own 2x to 4x pictures in play are its own 1x picture enlarged, pixel for pixel; only at the title does
that one scaled picture come out finer.

### The 3D box

A game on the software screen whose original offers 3D in its Options must still be able to take the GPU. Big Money's
does not: its Options have full screen and custom cursors only, and its original reaches 3D only through the
developers' debug keys or an Is3D value it never writes, so it needs none of this (an earlier note of this part said
otherwise, from a slip in its notes). The hand-back is there for the ports whose originals do offer the box. With
`theOffers3D` (`SoftwareScreenDisplay::Create(theApp, true)`, or `SOFTWARE_SCREEN=3d` in a test run) the display says 3D
can be had (mCan3D) while an accelerated renderer can be made, and stays in software with mIs3D false. When the game
wants 3D (`Set3DAcclerated(true)`, or Is3D=1 in the registry at the start) the window gets its renderer, and if that is
accelerated the screen is drawn on the GPU from then on (`SoftwareScreenDisplay::NoteWant3D`, which `InitDDInterface`
and `Set3DAcclerated` call). 3D off later leaves the screen on the GPU with mIs3D false, since a window that has had the
Direct3D 11 renderer cannot show its surface again; the next start is in software. A game whose original starts in 2D
says so with the new `SexyAppBase::mDefault3D` (true by default), which `InitDDInterface` takes as the wish when the
registry has no Is3D. Checked in Big Money under the test switch (as a game at hand, not because it needs it): started
with Is3D 0 it is in software, and `3d on`
moves it to the GPU (`Is3DAccelerated 1, Is3DAccelerationSupported 1`); started with Is3D 1 it is on the GPU from the
first frame; with software asked for at 2x, `3d on` moves it the same way; with `SOFTWARE_SCREEN=on` (no 3D offered) `3d
on` answers `Is3DAccelerationSupported 0` and nothing changes.

### Full screen

Full screen on this 3840x2160 display is 4.5 times the game's 640x480, no whole multiple, so its picture needs the GPU's
filtered stretch and stays on the GPU. Nothing was measured there and nothing changed. Whether a full screen should use
the linear filter, nearest sampling, or whole factors with borders (4x with borders would let the software screen hold
full screen too) is the user's decision, and no default changed.

### What it costs

**GDI's stretch samples the nearest pixel only.** At whole factors that is the picture itself; it could not serve a full
screen that is no whole multiple. It is Windows only: elsewhere the repeated copy stays.

**Its processor time grows with the area changed**, as the copy's did, where the GPU's present costs the same whatever
changed. It stays far under the copy's, which it replaces.

**A port that asks for software at 2x to 4x pays the software draws' processor time** for the device's memory (above),
and its picture there becomes its 1x software picture enlarged, losing the GPU's blending and, in Big Money, the finer
drawing of one scaled picture at the title.

**What dwm and the kernel spend on each path is not known**, for the machine was never quiet (above).

**The window's surface was paced less evenly than a vsync present** while the framework's pace was the default: a moving
scene showed a frame twice or skipped one on a quarter to a half of the refreshes, on the software screen at 1x (Big
Money, Dynomite) as on `SurfaceDisplay` at every size. The compositor wait evens it for about 2 % of the game's processor
time and blocks the main loop for up to a refresh, as a vsync present does; the user made it the default on 2026-11-15
("The compositor wait, the default", above).

**A stretched picture is made again** when it is drawn after two frames without it, as the original made every one each
time it was drawn.

### The switches added

| Switch (test runs only) | What it does |
|---|---|
| `SOFTWARE_SCREEN=on` | gives a game that keeps DDInterface the software screen in its place, as Dynomite has it |
| `SOFTWARE_SCREEN=3d` | the same, offering 3D: Is3DAccelerationSupported is 1 while an accelerated renderer can be had, and 3D on hands the screen to the GPU |
| `SOFTSCREEN_SCALED=1` or `0` | a software screen stays in software at 2x to 4x (through GDI's stretch where it can), or takes the GPU there, whatever the port asked |
| `SURFACE_PRESENT=copy` | at 2x to 4x, the repeated copy into the window's surface in place of GDI's stretch, as before |
| `SURFACE_PACE=ms` | each present to the window's surface is held by the framework's millisecond pace, as before 2026-11-15, in place of the wait for DWM's next composition (Windows; the wait is the default) |
| `BYOPOP_NOLOADUPLOAD=1` | turns off the upload at load (step 6) |

The test command `3d on|off` ticks or clears the Options box through `Set3DAcclerated` and logs what the display made of
it. `-presentlog` now records the presents without a renderer too (as `surface`, `surface-gdi`, `stretch`, and each with
`-dwm` under the compositor wait), and writes the first present's counter (`first_counter`), so that the presents can be
set against DWM's vertical blanks.

### The other 2D ports on DDInterface

The same opt-in (a `CreateDDInterface` override returning `SoftwareScreenDisplay::Create`, under `#ifndef
__EMSCRIPTEN__`, with mDefault3D false where the original starts in 2D) suits any port whose original draws a 2D mode in
software and that keeps DDInterface now: Alchemy, Atomica, Bejeweled, Bejeweled 2, Bookworm, Bookworm Adventures and its
sequel, Hammer Heads, Mummy Maze, Ning Po, Noah's Ark, Pixelus, Rocket Mania, Seven Seas and Venice. AstroPop, Heavy
Weapon and Typer Shark have displays of their own (DDInterface subclasses), so theirs would start from those. At 1x it
saves the device's 28 to 30 MB; on this machine it costs processor time against the GPU path (above), which each session
weighs against its original's figures. Each session first finds which mode its original starts in from its own export,
as Big Money's was found (MakeWindow's mIs3D and the DDInterface constructor's mAutoEnable3D). Bejeweled 2 and Bejeweled
Twist draw their 3D modes through GPUDisplay, which is not this. The web build keeps DDInterface. Each opt-in is its own
session's.

## Which renderer, and SDL's batching (2026-11-15)

Atomica's session found that trunk commit `ccec8c20` (2026-11-11, which asks SDL for Direct3D 11 by name) made its
processor time two to four times what it was, nearly all of it in the NVIDIA Direct3D 11 driver's thread, while runs
with `SDL_RENDER_DRIVER=direct3d` cost what the older builds had cost. This part found why, measured the renderers
again, and changed the framework.

### What SDL really chose

The log's renderer name is SDL's own (`SDL_GetRendererInfo`, in both the `display:` and the `Renderer:` line). On
rundesk's desktop Direct3D 9's `CreateDevice` fails (`D3DERR_NOTAVAILABLE`), and SDL goes on through its list to Direct3D
11, so `SDL_RENDER_DRIVER=direct3d` there gives `renderer direct3d11`, as the log said. On the visible desktop it gives
Direct3D 9 (`nvd3dum.dll` loaded, `renderer direct3d`). Atomica's cheap runs were therefore Direct3D 11 as well.

### Why the same renderer cost eight times as much

SDL 2.32's `SDL_CreateRenderer` turns batching off for a renderer it was asked for by name through
`SDL_HINT_RENDER_DRIVER`, and leaves it on for one it found by going through its list. Unbatched, every draw call goes to
the driver as it is made, and a game that draws many small pictures a frame hands Direct3D 11's driver thread a submission
for each. Before `ccec8c20` no renderer was named, so a player's Direct3D 9 and a test run's Direct3D 11 were
both batched. With the hint, every GPU-drawn port has run unbatched since 2026-11-11, and so has every run that set
`SDL_RENDER_DRIVER` to compare renderers, AstroPop's figures behind that commit among them (all three of its renderers
were unbatched). `SDL_RENDER_DRIVER=direct3d` on rundesk's desktop named a renderer that failed, and SDL's search for
another kept batching on.

Batching changes no pixel. SDL sends its queue to the driver before a texture the queue uses is updated or locked, before
the render target changes, before pixels are read back and at every present, so everything reads what it would have read
unbatched. The framework's own upload flush (`Platform::FlushRendererCommands`) flushes the device's context, which holds
the uploads' copies whether SDL batches or not.

### The measurements

Each run is a game's process from the first update of a 20-second stretch of play to its end, in real time: Atomica's
idle board (the lite pass's scenario, the pointer still), Dynomite's time-trial minute from its first move, and AstroPop's
classic game (the performance pass's scenario) in 2D and 3D, all windowed at 1x. Processor time is the cycles of all the
process's threads, grouped by the module each thread started in; the whole machine adds dwm.exe's and the System
process's cycles over the same stretch. *Pinned* is the game's process on one performance core (both its logical
processors, claimed through `tools/pincore.py`'s lock); *unpinned* is the game free to use every core, as on a player's
machine. The configurations ran turn about, one game process at a time, two or three rounds; the machine had none to two
other game processes running.

**On the visible desktop** (05:50 to 06:50, where Direct3D 9 starts), G cycles of the game's process in the 20 seconds,
pinned / unpinned (medians):

| | Direct3D 11, unbatched (the trunk) | Direct3D 11, batched | Direct3D 9, batched | OpenGL, batched | software screen |
|---|---|---|---|---|---|
| Atomica (3 rounds) | 42.0 / 42.6 | 5.6 / 5.4 | 7.0 / 4.8 | 5.1 / 20.3 | |
| Dynomite (2) | 1.88 / 1.82 | 1.75 / 1.77 | 2.89 / 1.83 | 1.77 / 41.3 | 1.71 / 1.57 |
| AstroPop, 3D (2) | 2.26 / 2.13 | 1.89 / 1.83 | 3.67 / 2.06 | 1.98 / 36.6 | |
| AstroPop, 2D (1) | 2.45 / 2.38 | 2.34 / 2.21 | 3.71 / 2.08 | 2.05 / 8.36 | |

In Atomica unbatched, the driver's thread took 37 to 38 G of the 42 (one pinned run 26 of 31), pinned or not. Batched,
it took 1.3 to 1.6 G. Direct3D 9's driver thread took 1.2 to 1.4 G unpinned and 3.1 to 3.3 G pinned (Dynomite 1.1 against
2.4, AstroPop's 3D 1.3 against 3.0): sharing the game's core costs it more than double. OpenGL's driver runs on the game's
own thread, which spins there when other cores are free (Atomica 16 to 39 G in that thread unpinned, against 4.7 to 5.3
pinned).

Private bytes at the peak of the run, pinned / unpinned:

| | Direct3D 11, unbatched | Direct3D 11, batched | Direct3D 9, batched | OpenGL, batched | software screen |
|---|---|---|---|---|---|
| Atomica | 270 / 269 MB | 109 / 109 | 117 / 118 | 92 / 104 | |
| Dynomite | 96 / 94 | 75 / 77 | 74 / 76 | 96 / 84 | 49 / 48 |
| AstroPop, 3D | 86 / 85 | 84 / 84 | 135 / 137 | 88 / 97 | |
| AstroPop, 2D | 120 / 119 | 119 / 119 | 129 / 129 | 116 / 131 | |

Unbatched, Atomica's driver held 160 MB more at the peak. The process had 41 to 43 threads on Direct3D 11, 20 to 22 on
Direct3D 9, 9 to 12 on OpenGL and 4 or 5 on the software screen.

dwm.exe took 1.7 to 4.7 G in the 20 seconds whatever the renderer, moving with the time of the run more than with the
configuration (the software screen 1.2 to 1.7). The System process took 1.3 to 15 G between runs of one configuration, so
the whole machine's sum (game, dwm and System) separates configurations only where the game's own difference is several G,
as it is for unbatched Direct3D 11 and unpinned OpenGL. Both sums are in each run's record.

**On rundesk's desktop** (06:50 to 07:30, where every test runs and Direct3D 9 cannot start), the same, with
`SDL_RENDER_DRIVER=direct3d` in place of Direct3D 9: it fails and SDL finds Direct3D 11 with batching on, as in Atomica's
runs:

| | Direct3D 11, unbatched (the trunk) | Direct3D 11, batched | `direct3d` (Direct3D 11 found) | OpenGL, batched | software screen |
|---|---|---|---|---|---|
| Atomica (2 rounds) | 35.3 / 41.4 | 5.06 / 4.78 | 4.95 / 4.78 | 4.50 / 40.4 | |
| Dynomite (2) | 1.89 / 1.86 | 1.81 / 1.74 | 1.77 / 1.68 | 1.50 / 41.0 | 1.63 / 1.62 |
| AstroPop, 2D (1) | 2.36 / 2.25 | 2.14 / 2.01 | 2.09 / 2.04 | 1.70 / 7.81 | |

Atomica's peak was 267 to 277 MB unbatched and 111 to 112 MB batched, either way of asking; Dynomite's 98 against 79 to
81. Batched Direct3D 11 asked for by name and Direct3D 11 found after Direct3D 9 failed cost the same, so the whole of
Atomica's finding is SDL's batching. The machine stayed quiet through both batches (none to two other game processes, the
machine 3 to 25% busy), so a loaded machine was not measured; the earlier loaded runs of Atomica's session (four to six
other games) moved the unbatched driver thread between 8 and 33 G.

### The default

The framework keeps Direct3D 11 by name and turns SDL's batching back on (`DDInterface::Init`: `SDL_HINT_RENDER_BATCHING`
"1" at the default priority, so `SDL_RENDER_BATCHING` in the environment still decides). Batched Direct3D 11 was the
lowest or within a few per cent of the lowest in every case measured, 2D and 3D, and it is the only renderer whose cost
was the same on one core and on all of them. Direct3D 9 was as cheap on a free machine and dearer on one core, held 50
MB more in AstroPop's 3D, and cannot run on rundesk's desktop, where every test runs. OpenGL spins on a player's machine.
No per-case default is needed. On the light games (Dynomite, AstroPop) the batching saves 3 to 16%; on a game that draws
many small pictures a frame it saves most of its processor time and, in Atomica's case, 160 MB at the peak.

Every processor-time and memory figure a session took on a GPU-drawn port between 2026-11-11 and this merge was taken
unbatched, this file's GPU-path figures above among them, and a port that draws many pictures a frame will measure lower
now. Each session takes its figures again after the merge. The shots are the same, since batching changes no pixel: 230
screenshots of Atomica, Dynomite on the GPU path and AstroPop in 2D and 3D, taken through the same scripted play with
batching off and on, have the same decoded pixels, and `tools/check.sh --changed main` passes all 36 suites.

### Like for like, for a renderer whose cost lands in a driver thread

The driver's threads are the game's cost: a player's machine runs them, and the original's DirectDraw work runs in its
own process or the kernel too. So the figure is every thread of the process, as it has been. Pinning stays fair for the
default renderer, since batched Direct3D 11 cost the same on one core and on all of them (Atomica 5.6 and 5.4, Dynomite
1.75 and 1.77, AstroPop 1.89 and 1.83). It is not fair between renderers: Direct3D 9 pays double for its driver thread on
one core, and OpenGL's spin shows only when cores are free. A comparison of renderers is therefore made both ways, and a
game's figure against its original is taken pinned on its default renderer. dwm.exe and the System process are recorded
beside it, but their run-to-run spread is larger than most of the differences a session is after.

## The 2D flags on the GPU path (2026-11-15)

On the GPU path every port drew with 3D's flags, whatever its original did. DDInterface set mIs3D from the renderer at
every Init, and with it the widgets draw with the smooth stretch and the linear blend (`SetFastStretch(!is3D)`,
`SetLinearBlend(is3D)`), and the framework batches `AddQuads` and blends opaque pictures; with mIs3D false it draws as
PopCap's 2D mode does. Half the switch was there already. `SexyAppBase::mDefault3D` says what the original starts in,
`InitDDInterface` takes it as the wish when the registry holds no Is3D and sets mIs3D false for a 2D wish, and a 3D box
cleared in the Options does the same through `Set3DAcclerated`. What lost the wish was a change of mode that makes the
renderer again without the app: SurfaceDisplay's `RestoreRenderer` calls `DDInterface::Init` at a resize or at full
screen, and the software screen's hand-over to the GPU (`SoftwareScreenDisplay::DrawOnGpu`) set mIs3D true. Both now
take it from `DDInterface::mAppWants3D`, which `NoteWant3D` records; the app tells it at every display made and at every
change of the box. A game with mDefault3D false, or a player who cleared the box, so keeps the 2D flags through every
change of size and mode. On those two displays a cleared box used to come back on at the next such change, but no port
on them offers the box today, so no player met it.

mDefault3D stays true unless a port sets it, so no port's shots or baselines change until its own session opts in. The
session sets mDefault3D false in its app's constructor, takes its shots again on the GPU path (the 2x window, full screen
and the web) and judges each difference against its original. The cost is that its frozen baselines and shots taken on
the GPU path are taken again.

One path still gives 3D's flags. SurfaceDisplay's surface and stretch modes with no renderer (`EnteredNoRendererMode`)
set mIs3D true, because `DDImage::GetBits` reads the screen back only while there is a renderer or mIs3D is set. The
software screen does not take that path in software (it sets mIs3D false there and reads its own bits). A port on
SurfaceDisplay itself (Pizza Frenzy, Tip Top and the engines of their own) that wanted the 2D flags in those modes would
first need that read-back keyed on the display's mode rather than on mIs3D.

The test is `fwtest -corner -realinput=tools/fwtest/flags.txt` in `tools/check.sh unit`, on DDInterface and on the
software screen (`SOFTWARE_SCREEN=on SOFTSCREEN_SCALED=1`), 15 checks each: the 3D box cleared, full screen and back,
then ticked and two resizes. With the trunk's framework the software screen fails the two checks of the 2D flags after
full screen and back; DDInterface passes, as its resizes make no renderer.

The same run checks Big Money's request that the fast stretch's sampling a game chooses (`mFastStretchCentre`) holds
through `Cleanup`. `SoftwareScreen()`, which `Cleanup` and the constructor call, set it back to pixel centres; it now
leaves the game's choice alone and puts it back when the screen returns from the GPU, which samples by its own filter.
In the trunk `Cleanup` runs only when the display is deleted, so no change of mode had reset it and the trunk's code
passes these checks too: the change keeps a later caller of `Cleanup` from undoing the game's choice, and the checks
(sampling from the corner set once the display is made, then 1x, 2x and full screen) guard it.

### Which originals start in 2D

Read from each game's own export for this list where its notes did not already say. In a SexyAppFramework original the
D3DTester decides: the app's constructor setting mAutoEnable3D starts the game in 3D when the test passes, and a saved
Is3D wins over both. Every port is a SexyAppBase app (the engines of their own through a SexyAppBase host), so the
switch reaches each of them.

| The original | Ports | Where it is said |
|---|---|---|
| Starts in 2D: mAutoEnable3D never set, 3D only through a saved Is3D | Alchemy, Bejeweled, Big Money, Dynomite | Big Money's IDENTICAL_PLAY.md; Dynomite's GAME_NOTES.md (DirectDraw); Alchemy's and Bejeweled's exports (Bejeweled's likely: nothing in the export sets the flag) |
| Has no 3D mode | Atomica, Mummy Maze, Noah's Ark, Pizza Frenzy, Seven Seas; Bonnie's, Cosmic Bugs, Ning Po, Tip Top, Water Bugs, Word Harmony (engines of their own) | the notes of Seven Seas, Pizza Frenzy, Tip Top, Ning Po, Bonnie's, Cosmic Bugs and Word Harmony; the others' exports (no D3D tester, no Direct3D strings) |
| 3D when the card passes the test | AstroPop, Bookworm, Bookworm Adventures, Heavy Weapon, Pixelus, Rocket Mania, Typer Shark, Venice; Bejeweled 2 and Hammer Heads; Bookworm Adventures 2 | the exports' constructors (Bookworm's GAME_NOTES.md too); the two ports' translated constructors; inferred from its predecessor, with no export here |
| 3D by its own setting | Bejeweled Twist (Direct3D 9, 2D only with Is3D saved 0 or a failed test); Mahjong China and Japan (Direct3D 7, "HardwareAcceleration" 1 by default, 2D when their own test fails) | Twist's NOTES.md and IDENTICAL_PLAY.md; the Mahjong exports and DEVIATIONS.md |

The first two rows are the candidates for mDefault3D false. For the third, the test decides on the player's machine, so
the port's default of 3D stands. But the tester fails under rundesk and in Windows Sandbox (Heavy Weapon's notes give its
E_OUTOFMEMORY), so in every origrun comparison such an original runs its 2D mode while its port draws with 3D's flags.
A session comparing shots or processor time there runs its port with 3D off (Is3D 0 saved, or the `3d off` command) to
be like for like. Rocket Mania's notes call the constructor's flag mTest3D, but its store is to mAutoEnable3D, the byte
before it.

## A smooth stretch on the processor (2026-11-15)

A SurfaceDisplay game (Bonnie's, Cosmic Bugs, Tip Top, Water Bugs, Word Harmony, Pizza Frenzy, the two Mahjong Escapes)
or a game on the software screen (Dynomite, Big Money) shows a window of 1x to 4x its size without a renderer, but any
other size, a full screen on a display whose size is no whole multiple of the game's above all, went to the renderer and
brought the Direct3D 11 device back with it. Mahjong's stretch mode avoided that with GDI's nearest-neighbour stretch, a
different picture. This part draws the renderer's own picture on the processor, so those sizes need no Direct3D either.

### What the renderer shows

The renderer puts the frame into its screen texture at the render scale R (the window's scale rounded up, at most 4),
each pixel repeated R times each way with the nearest filter, and copies that texture into the presentation rect with the
linear filter (`DDInterface::Present`). `tools/stretchcal` draws test pictures (gradients, one-pixel lines, checkers,
noise, and pictures that change only from column to column or only from row to row) through those same SDL calls, reads
the back buffer before the present, and compares it with models of the filter. The card's arithmetic, as fitted on this
machine's NVIDIA card under Direct3D 11:

- The texture coordinate of output pixel i is (i + 0.5) times a step that the triangle setup makes in 32-bit floats: the
  reciprocal of the rect's area (width times height), times the rect's other side, the product rounded toward zero. The
  coordinate is then truncated to 21 fractional bits.
- The texel position, the coordinate times the texture's size less one half, is rounded half up to 8 fractional bits: its
  whole part picks the first texel and its fraction is the second texel's weight, in 256ths. The edges clamp.
- Where both axes blend, the product of the two weights is rounded half up to 8 bits and the other three weights follow
  from it, so that the four sum to 256.
- The weighted sum of each channel is brought to 12 bits (times 257/4096, rounding half up) and then to 8 bits rounding
  half down.

`framework/LinearStretch` does exactly that from the frame itself, with a table for the columns and one for the rows;
the repeated texture is never made. Most output pixels fall between two texels of one frame pixel and are copies; only
the seams between the frame's pixels blend (with SSE2, four at a time: "What it costs"), and a row whose taps equal the
row above is copied whole.

### How close it is

At most sizes, to the byte. Of 40 window sizes tried with 640x480 and 800x600 frames and the six patterns (480 cases),
35 sizes came out identical in every pattern, among them the four calibration sizes (3840x2160, 1920x1080, a 1000x750
window and 500x375, a factor below 1) and the common full-screen sizes 1280x720 to 3440x1440. At the other five,
presentation rects of 740x555, 666x499, 1357x1018, 601x451 and 1036x777, a few columns or rows of seams come out one
weight step off, which moves those pixels by one level (two where both axes blend in the checkers and noise patterns).
The cause is the card's reciprocal of the area: at those sizes it is one unit of the last place away from the correctly
rounded value, up at some sizes and down at others, and no rounding rule of the reciprocal or of the product fits every
size, so it is the hardware's own approximation. A different card (AMD, Intel) filters with its own arithmetic and would
need its own fit; the renderer's picture is then not the same as this card's either.

In a game: Bonnie's menu read back from the window at 1000x750, 1500x1000, 1920x1080 and 2560x1440 is identical to the
renderer's window at those sizes; at 1111x777 (rect 1036x777) 933 pixels of 863,247 differ and at 1357x1018 1,025 of
1,381,426, each by one level, on seam columns. Dynomite's software screen against its GPU path: identical at 1000x750,
1500x1000 and 1920x1080, one level at 1,728 and 769 pixels at the other two.

### How it is drawn

Stretch mode (`SurfaceDisplay`, `AllowStretch`) now has two flavours: the app's nearest-neighbour one when it allows it
(Mahjong's, off in its port), and otherwise the smooth one (`SurfaceDisplay::kLinearStretchByDefault`; `SURFACE_STRETCH=3`
gives it and `0` neither in a test run). `GdiStretch::PresentLinear` paints only the window pixels that read a changed
pixel of the frame: each changed rect is mapped through the tables (`LinearStretch::OutputColumns`, `OutputRows`) to the
exact run of columns and rows whose two taps touch it, which covers the seams it blends into, and rects that then overlap
are merged. Each part is drawn a band of rows at a time into a DIB section of under 1 MB (1,000 KB: 133 rows at 1920
wide, 66 at 3840) and copied into the window with BitBlt; no image of the window's size is kept. The whole window,
bars included, is painted at a change of size, at an expose, a restore and a regained focus (`ScreenInvalidated`), and
once a second as before. `SURFACE_VERIFY=1` compares a shadow bitmap painted the same way with the whole picture drawn
afresh after every present, and `=2` reads the window back too; Bonnie's at seven sizes, 72 frames, none differed.

The software screen takes it at any size other than 1x to 4x, unless the renderer is wanted for something else: a drawn
cursor, 3D, or a screen already on the GPU (`SoftwareScreenDisplay::WantsStretch`). At 2x to 4x the port's own choice
between the surface and the GPU stands.

The test command `screenshotwindow <file>` writes the window as shown (`DDInterface::ReadWindow`): the renderer's
present made again into its back buffer, or GDI's window read back, after any pending present.

### What it costs

Bonnie's on rundesk's desktop, pinned to one core, with the smooth stretch and the renderer (`SURFACE_STRETCH=3` and
`0`) turn about, both under the compositor wait: full screen at the desktop's 3840x2160 (the frame in a rect of
2880x2160 at render scale 4) and a window of 1920x1080 (1440x1080, render scale 2). The logos are whole-screen fades
(360 updates), then the static menu (900 updates) and a scripted minute of the tour. Medians of the game's processor
cycles (`-cyclesplit`) over five rounds, and its private bytes at the menu and at the end:

| Window | Path | Logos | Static menu | Minute | Private bytes |
|---|---|---|---|---|---|
| 3840x2160, full screen | smooth, on the processor | 0.83 G | 0.70 G | 8.59 G | 53.6 / 55.7 MB |
| 3840x2160, full screen | renderer (Direct3D 11) | 0.79 G | 0.83 G | 8.79 G | 86.5 / 88.6 MB |
| 1920x1080 | smooth, on the processor | 0.58 G | 0.78 G | 9.04 G | 56.4 / 58.1 MB |
| 1920x1080 | renderer (Direct3D 11) | 0.80 G | 0.88 G | 9.53 G | 86.0 / 88.5 MB |

**Memory:** about 30 MB less private bytes at both sizes, the Direct3D device and its driver (5 to 7 threads in the
game's process against 41 to 44) never being loaded; the stretch itself keeps its band of under 1 MB and its tables.

**Processor:** at or below the renderer's in a window of 1920x1080 and in the 4K minute, and about the renderer's in the
4K fades: 5 % more by the medians of the five rounds, and more in three pairs of five (2, 14, 18 and 24 % more, 5 %
less), with the rounds themselves 2 to 1 apart from the machine's load (the 4K logos took 0.76 to 1.32 G with the
smooth stretch and 0.69 to 1.07 G with the renderer). The logos kept their 3.5 s on both paths but once (3.8 s with the
smooth stretch in the busiest round). What is left over the renderer at 4K is the copy into the window: in a timed run
of the logos at 4K, `LinearStretch::Draw` took about 0.9 ns a pixel (the first SSE2 version, a quarter faster since) and GDI's BitBlt
into the window 0.6 ns, which no drawing can save; the renderer copies only the frame's changed pixels and the card does the
rest.

**The first draw was slower.** It worked a value out for each run of equal columns (a frame pixel's copies, or one seam)
and filled the run, a branch and a short loop for every two or three pixels; a whole 2880x2160 rect took 9 to 10.5 ms
from an 800x600 frame and 15 to 19 ms from a 640x480 one (`stretchcal -bench`), and the 4K fades cost 28 % more than the
renderer's, the logos running 0.4 and 0.5 s long in two rounds of three. Now each row takes its copy columns from a
table of the frame's pixels (from the frame's row, or, between two rows, from that row blended with the next for each
of the frame's columns once, four at a time), and the seams, about one column in three, are blended over them four at a
time with SSE2: each seam's two pixels read at once (the second is always the first's right neighbour), the weights
kept spread over the 16-bit lanes, and the level worked out in those lanes (`(s + ((s + 128) >> 8)) >> 4` is the
table's first step and `(q + ((2047 - q) >> 8)) >> 4` its second, both inside 16 bits for every sum). The same rect now
takes 3.3 ms from 800x600 and 5.1 ms from 640x480 (1080: 2.7 and 1.5 ms). `stretchcal -selftest` (in `check.sh unit`)
holds `Draw` to `DrawReference`, the arithmetic pixel by pixel: 6,300 cases, whole rects in bands and random parts of
them, five frame sizes, thirty windows and the six patterns with top bytes set, none differing, and every one of the
65,281 sums levelled as the table does; the card's comparison is still 0 differing.

**dwm.exe and the System process** were measured on the visible desktop with the first draw (a 1920x1080 window, not
activated, three rounds each), but other sessions' builds kept the machine 94 to 100 % busy throughout, so the wall times
there are the load's: the renderer's static menu took 19.3 s for 9 in one run, and the smooth minute 80 and 96 s in two,
the second handing its frames to the pace after a held wait. dwm's cycles a second were the same within the spread for
the static menu (0.28 to 0.36 G against the renderer's 0.30 to 0.35) and the minute (0.34 to 0.47 against 0.34 to 0.46),
and about 12 % higher with the smooth stretch in the logos (0.34 to 0.37 against 0.29 to 0.34). The System process's
cycles were the other sessions' and say nothing here. What dwm and the kernel spend on each path still wants a quiet
machine.

### The mod menu

Under SurfaceDisplay, F2's mod menu opened but nobody could see it: the display shows the app's frame, not what the
widgets drew, and in renderer mode the frame was copied over the widgets at the present. Now, while the framework's own
widget is open (`SexyAppBase::OverlayWidget`), renderer mode copies the frame into the screen texture at `BeginFrame`,
under the widgets, and surface and stretch mode draw the menu in software over a copy of the frame (a `MemoryImage` of
the screen's size, kept only while the menu is open) and show that copy whole; the app's buffer is not written. Nothing
of the game updates while the menu is open, so the frame under it is still. The software screen already drew every widget
into its screen and is unchanged.

F2 was posted to nine games' windows under rundesk (Pizza Frenzy has no game files staged here), with window shots
before, open and after: Bonnie's, Cosmic Bugs, Tip Top, Water Bugs, Word Harmony and the two Mahjong Escapes at 2x, at
1111x777 smooth and on the renderer, and full screen at 3840x2160 (Tip Top stayed at 2x there); Dynomite and Big Money at
1x, at 2x and 1111x777 on the GPU (their default with 3D on), full screen, and at 1111x777 and full screen smooth
(`SOFTSCREEN_SCALED=1`, 3D off). The menu showed in every one and left nothing when closed; what differed afterwards
was the game's own animation. `screenshotwindow` read the software screen on the GPU as black, since `SurfaceDisplay`'s
readback copied the software buffer, not kept up to date there, over the screen texture first;
`SoftwareScreenDisplay::ReadWindow` now reads the texture as it is.

## Looked at and left

**Atlases.** Packing small pictures into shared textures saves what each texture costs beyond its texels, and that is
small: 1,000 textures of 32x32 cost 3.6 MB more than their texels on a second go (about 6 KB each above the texels at
most), so a game's 40 to 80 textures would save well under 1 MB, at the risk of an atlas's texel addressing moving a
sampled pixel. Not done.

**8-bit sounds.** There are none to keep (above).

**`MakeWindow`'s time** is the driver's (above).

## Open

**Pictures loaded but not yet drawn in the older games.** The upload at load (step 6) reaches the games on the
framework's resource manager. Atomica (43 to 45 MB of pixels at its first board), Big Money on the GPU path, Typer Shark,
Heavy Weapon, Venice and the other games that load each picture through `SexyAppBase::GetImage` still keep every loaded
picture's pixels until its first draw. Uploading there too would take most of it, but such a game may change a picture's
pixels or flags after `GetImage` returns (its own edits, `Palletize`), which would then make them again from the file, and
each game's own checks would have to show nothing moved. Not tried. A game's own composites (Big Money's `CopyImage`
pictures, about 5.7 MB on the GPU path) keep their pixels for good, since no file can make them again.

**The 2D flags are each port's to take.** The switch is in ("The 2D flags on the GPU path"); the ports whose originals
start in 2D or have no 3D mode set mDefault3D false in their own sessions, and SurfaceDisplay's modes without a renderer
still set mIs3D for the screen's read-back.

**A drawn cursor brings the renderer back.** On the software screen a game that sets a cursor image (Big Money's custom
cursors, off by default) has the window take a renderer for it, so the Direct3D device's 30 MB returns at 1x; the
original's 2D mode draws its cursor in software on the screen. Not done.

**A player's own window is 2x.** The framework opens 2x by default and full screen at the desktop's size, so a player is
on the GPU path with the device's memory unless the port takes software at 2x; the like-for-like figures here are 1x.

**Bejeweled Twist** holds 267 to 271 MB at its first board (121 MB of pixels, 55 MB of sounds) against the original's
about 158 MB at its menu; its pixels are mostly held for its software screen, and its own session would need to look at
what that keeps.

**libxmp seeds its random numbers with the time** (`libxmp_init_random`), so the music's random volume and pan swing and
random LFO differ from run to run, in every game with tracker music; a comparison of rendered music has to run within
one second.

**The Direct3D 11 device's 30 MB** is most of what is left between a port on the GPU path and its original, and it is
the NVIDIA driver's own ("Where the Direct3D 11 device's memory goes"). The software screen leaves it out at 1x, and
at 2x to 4x where a port asks; a port that keeps DDInterface keeps it, and so does a full screen at a size that is no
whole multiple. Each 2D port's opt-in is its own session's ("The other 2D ports on DDInterface").

**Windows' text-input modules in about one run in five.** In 7 of 35 runs of Big Money on rundesk's desktop the process
loaded Windows' text-input interface (`msutb.dll`, `InputSwitch.dll`, `TextInputFramework.dll`,
`Windows.UI.Core.TextInput.dll`, `CoreUIComponents.dll`, `dcomp.dll`, `DWrite.dll`, and `d3d11.dll` with `d3d10warp.dll`
for its own drawing, with `StaticCache.dat` mapped), 3.7 MB of private bytes more than the other runs; the original's
runs load none of them. SDL's `SDL_StartTextInput`, which turns on the text services framework, is the likely cause. Not
chased: it would need text input started only while a text field has the focus, and a check of every game's text fields.

## Bookworm Adventures' Lua heap (2026-11-15)

Asked to judge whether taking Lua out of Bookworm Adventures would pay, the port was run on the main build under
rundesk, windowed at 1x, in real time, and the game's `lua` test command asked the scripts' VM for its own count
(`gcinfo()`, the kilobytes Lua has allocated), then again after a full collection, with the process's private bytes beside
each. One run; the bot played from the title through the first chapter into the second (book 1, chapter 2, Polyphemus).

| Where | Private bytes | Lua allocated | Lua live (after `collectgarbage()`) |
|---|---|---|---|
| The title, its load over | 133.3 MB | 2.8 MB | 2.6 MB |
| A battle, book 1 chapter 2 | 139.0 MB | 13.9 MB | 8.7 MB |

Lua holds 2 to 6% of the port's private bytes when live, up to 10% counting garbage not yet collected (Lua 5.0 collects
when its count reaches twice what was live after the last collection). The original runs the same scripts on the
same fork of Lua 5.0.2, with the same object layout (`games/bookwormadv/docs/POPLUA.md`), so it pays about the same,
and the port's 39 MB over the original (the survey's "Thirteen games at a first board" and `build/memsurvey/table.md`)
lies outside Lua. Translating the 621 scripts into C++ would save at most this heap, less what their tables and texts
would take as C++ data; the scripts also carry the rules a large class of mods would change (`MOD_IDEAS.md`).
