# Memory and processor time

The ports used two to three times the memory of the original games: Big Money 124 MB of private bytes in its notes'
scripted minute against the original's 42 MB, Bejeweled 2 88 MB at the title against 40, Atomica 166 to 183 MB against
64 to 67. This is the framework work of 2026-11-14 that set out to bring that down without changing a pixel or a
sound: what the memory was, how it was measured, what each change saves in memory and processor time, and what was
looked at and left. The rule for every change was that every frozen baseline and every shot stays identical, that
nothing runs on a thread of ours, and that speed comes from not waiting.

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

## Looked at and left

**Atlases.** Packing small pictures into shared textures saves what each texture costs beyond its texels, and that is
small: 1,000 textures of 32x32 cost 3.6 MB more than their texels on a second go (about 6 KB each above the texels at
most), so a game's 40 to 80 textures would save well under 1 MB, at the risk of an atlas's texel addressing moving a
sampled pixel. Not done.

**8-bit sounds.** There are none to keep (above).

**`MakeWindow`'s time** is the driver's (above).

## Open

**Pictures loaded but not yet drawn** are the largest holder of memory left that a port can give back: the pixels column
above, 35 MB in Bejeweled 2 at its first board, 18 MB in Big Money, 31 MB in Dynomite, 43 to 45 MB in Atomica and
AstroPop, 40 MB in Heavy Weapon, 121 MB in Bejeweled Twist. Since a texture costs no private bytes, uploading every
picture as it is loaded and dropping its pixels then would take most of that, for the uploads' time at the start and a
second decoding of any picture whose first draw wants the other filter. It was not tried here.

**Bejeweled Twist** holds 267 to 271 MB at its first board (121 MB of pixels, 55 MB of sounds) against the original's
about 158 MB at its menu; its pixels are mostly held for its software screen, and its own session would need to look at
what that keeps.

**libxmp seeds its random numbers with the time** (`libxmp_init_random`), so the music's random volume and pan swing and
random LFO differ from run to run, in every game with tracker music; a comparison of rendered music has to run within
one second.

**The Direct3D 11 device's 30 MB** is most of what is left between a port and its original.
