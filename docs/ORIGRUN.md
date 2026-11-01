# origrun: running the original games for comparison

Every port under `games/<game>/` must play identically to the user's original executable. Before a port's
final merge its session compares the port with the original screen by screen, at the same points — every
screen, dialog and text, which music and sounds play, and what happens when the window loses and regains the
focus (CLAUDE.md, "Done means identical"). `tools/origrun` runs an original so that comparison can be made
without disturbing the user, who shares the machine.

It runs a **copy** of the original (never the install), invisibly, and by default silently, and it captures the
screen, drives scripted input, and changes the focus on command. It can patch the copy before the run (to pin
the RNG seed or skip a trial), and it can time a capture or an input to the game's own update counter so
frame-rate variation drops out of a comparison.

The user's position (2026-10-31): "You are welcome to run the originals with sound on and mess with my save
data" (their saves are backed up). So silence and leaving the registry untouched are conveniences, not gates:
origrun prefers them, and the user turns the speakers off for a run that needs real sound.

How any copy's protection is handled is kept in the private notes under Per-game status, which a public
snapshot strips out (`docs/PUBLISHING.md`); this document otherwise names games only by title.

## Build

    tools/origrun/build.sh

Builds, 32-bit (the originals are 32-bit), with llvm-mingw's i686 target under `E:\retools`, into
`build/origrun-bin/` (git-ignored):

- `origrun.exe` — the host: copies the game, launches it under rundesk, drives it, captures, watches audio.
- `dsound.dll` — the DirectSound sound stand-in placed beside the copy (silence + a log, optionally WAVs).

`build/native/rundesk.exe` must exist (`cmake --build build/native --target rundesk`).

## Use

    origrun --game <name> --script <file.txt> [options]
    origrun --exe "<path to an already-placed copy>" --script <file.txt> [options]

`--game <name>` looks the install up in `tools/origrun/games.tsv` (name, install folder, exe leaf, and optionally
the game's registry key and 3D-flag address, for `--3d`) and copies it into `build/origrun/<run>/game`. The
repository's table is read first; a copy beside `origrun.exe` is only a fallback. `--exe` runs a copy you placed yourself (used for a game whose real exe
you first carved out of a wrapper; see below). Captures land in `build/origrun/<run>/shots`, logs in
`.../logs`, recorded WAVs in `.../snd`.

Options:

- `--run <id>` — the run folder name under `build/origrun/` (default: the game name).
- `--sound stub` (default) — the silent stand-in that accepts every call and logs it.
- `--sound nodriver` — the stand-in reports no sound device (`DSERR_NODRIVER`); use for a game's very first run.
- `--sound wav` — silent stand-in that also records to `snd/` what the game writes to its DirectSound buffers.
- `--sound real` — no stand-in; the game uses the real audio device (turn the speakers off first).
- `--timeout <sec>` — wall-clock cap (default 120, max 240; a run is one game process and must stay short).
- `--fresh` — note to re-copy (delete `build/origrun/<run>` first to force a clean copy).
- `--copy-only` — stage the copy (and print its folder and exe) without launching. Use it to place a copy you
  then prepare by hand (e.g. extracting a wrapped exe into it) before running it with `--exe`.
- `--stage-root <dir>` — put the **game copy** under `<dir>\<run>\game` instead of under the run folder (shots and
  logs stay under `build/origrun/<run>`). A worktree's `build/origrun` path is long, and some launchers (Bookworm
  Adventures) exit at once when their folder sits that deep; a worktree session stages the copy under a short path,
  e.g. `--stage-root C:\byo-og` or main's `build/origrun`.
- `--counter <expr>` / `--flips <expr>` / `--update-ms <ms>` — the game's update counter, for update-timed
  capture and input (see "Timing to the game's updates"). Usually set once in the per-game config instead.
- `--patches <file>` — a patch list to apply to the copy (see "Patching the copy"). Defaults to
  `tools/origrun/patches/<game>.txt` if present. `--no-patches` runs the copy unpatched.
- `--seed <N>` — a value substituted for the `{seed}` token in a patch, to pin the original's RNG so its boards
  and other random content match the port it is compared with.
- `--3d on|off` — the game's own Hardware Acceleration setting for this run, and a check, read from the game's
  memory, that it really ran in that mode; a run that ends up in the other mode fails with exit code 3 (see
  "Hardware acceleration (3D mode)"). With `--exe` alone, origrun takes the key and flag from the games.tsv row
  whose exe name is the same.
- `--3d-key <HKCU key>` / `--3d-flag <expr>` — the registry key and 3D-flag address when games.tsv has none, or
  to override it.
- `--dgvoodoo <dir|env>` — put dgVoodoo2 (a DirectDraw and Direct3D 7 implementation over Direct3D 11) into the
  staged copy for this run, from `<dir>`, or from the folder in the environment variable `ORIGRUN_DGVOODOO` when the
  word is `env`. `--3d on` uses `ORIGRUN_DGVOODOO` by itself when it is set. Needed for any 3D run, and it also gets
  the full-screen games past "Screen Setup Failed" (see "Hardware acceleration (3D mode)"). **It refuses to start a
  run that takes shots**: no capture of a game drawing through a GPU wrapper is safe.

## Script commands

One command per line; `#` starts a comment. Input is posted to the game's window (it works across the hidden
desktop). Coordinates are in the window's client area; a click is routed to the child control under the point,
so a dialog button is hit as well as a game that hit-tests in its own window.

- `wait <ms>` — pause.
- `move <x> <y>` — move the mouse.
- `click <x> <y>` / `rclick <x> <y>` — left / right click.
- `clickready <x> <y> [timeout_ms]` — wait for the window to exist, then click (use it for the first click,
  since a game's window can be slow to appear while it tries to phone home).
- `key <vk>` — a virtual-key press (decimal or `0x..`).
- `text <chars...>` — type the rest of the line.
- `focus lose` / `focus regain` — post the deactivate/activate messages a window gets when it loses and
  regains the focus (`WM_ACTIVATEAPP`, `WM_ACTIVATE`, `WM_KILLFOCUS`/`WM_SETFOCUS`).
- `shot <name>` — capture the window to `shots/<name>.png` now.
- `waitshot <name> [timeout_ms]` — wait for the window, then capture. It keeps re-grabbing while the client area
  is still flat (a window that has not drawn its first frame, as several DirectDraw games are slow to), so it
  catches real content rather than the blank first frame.
- `record <prefix> <duration_ms> <interval_ms>` — capture a numbered frame every interval for the duration,
  whenever a window is present. Good for a screen that appears at an unpredictable time.
- `burst <name> <count> <interval_ms>` — a clip: `count` frames, one every `interval_ms`, written to
  `shots/<name>/0000.png ...` with the `clip.json` shotdiff expects (`{"interval_ms": N}`). On this machine each
  capture takes about 15-25 ms (`PrintWindow`), so a frame lands within that of its scheduled time.
- `burst <name> <count> update <k>` — a clip timed to the game's own updates: a frame at the current update and
  every `k` updates after, each taken with the window thread held so the frame cannot change under the grab.
  `clip.json` records `k * update_ms`. Needs an update counter (below). Frame-rate variation drops out of this.
- `hwmove <x> <y>` / `hwclick <x> <y>` / `hwrclick <x> <y>` / `hwkey <vk>` — input through the desktop
  (`SetCursorPos` + `SendInput`) instead of a posted window message, for a game that reads the mouse with
  DirectInput or `GetCursorPos` and so ignores posted clicks (TipTop). Plain `click`/`key` still post messages,
  which drive the SAF/LOWOS message path.
- `at <N> <command...>` — when the update counter reaches `N`, hold the game's window thread, run the command
  (`shot`, `click`/`rclick`/`key`/`text`/`move`/`focus`, or `burst`), then resume it. Lands on the exact update
  (the counter is polled every ~1 ms against ~10 ms updates), so the original and the port can be captured at
  the same game update rather than the same wall-clock time.
- `msgbox <button> <text...>` — when a message box whose caption or text contains `<text>` appears, click the
  button labelled `<button>` (e.g. `msgbox No Trial expired`, `msgbox OK update`). Put these at the top of the
  script; they are registered before the game launches, so even a box that pops up early is answered. A box
  with no matching rule is answered by its own default button. The label match ignores the `&` accelerator and
  case. Never name a link/"Buy"/"More Games" button here (see the warning below).
- `quit` — ask the window to close, and end the run.

**A script must never click a link button** — "Buy", "Buy Now", "Register", "More Games", a store or web
address. Those open a browser or bring up the user's Steam client. Click only "Play" / "Click here to play"
and the game's own controls. The trial dialogs put the link buttons next to the Play button, so choose
coordinates that land on Play.

## What it guarantees, and how each is checked

1. **A copy, never the install.** The host copies the game's folder into `build/origrun/<run>/game` and runs
   that; the install folder is only read. `--exe` runs a copy you placed. Checked: the launched path is always
   under `build/`, printed in the log; the install is opened read-only for the copy.
2. **Invisible.** The game runs under `build/native/rundesk.exe`, on the private `bejeweled-test` desktop at
   below-normal priority; nothing is shown and focus is never taken from the user. Checked: the host never
   shows a window; it drives and captures the game by attaching a worker thread to that desktop
   (`SetThreadDesktop`), which is also required for the window handles there to be usable.
3. **Silent by default.** The `dsound.dll` stand-in beside the copy makes `IDirectSound`/`IDirectSoundBuffer`
   objects that accept every call and never open a device, so DirectSound and anything on top of it (BASS uses
   DirectSound) stays silent. As a second check the host watches Windows Core Audio: if a session appears for
   the game it mutes it and says so in the run result. A game that plays through another path (winmm `waveOut`
   / `PlaySound`, or FMOD on its own output) is not silenced by the dsound stand-in yet (see Gaps); the audio
   watch still mutes and reports it, and for such a run the user turns the speakers off.
4. **Screens.** `shot`/`waitshot`/`record`/`burst` capture the window with `PrintWindow`, or a blit of the
   window's own DC where `PrintWindow` fails or comes out flat, and write a PNG. Nothing ever reads the screen, and
   **nothing at all is captured of a game that has loaded a GPU API**: `dxgi`, `d3d8`, `d3d9`, `d3d10*`, `d3d11`,
   `d3d12`, `opengl32` or `vulkan-1`. origrun reads the game's module list from outside (`EnumProcessModulesEx`)
   when its window comes up and logs what it finds, asks again when each capture starts, right before a window-DC
   read that follows a `PrintWindow`, and once more before the pixels are saved. The first refusal names the
   module, and every later shot of the run is refused too; a module list it cannot read refuses that shot. A
   graphics wrapper in the copy (any `ddraw.dll`, `d3dimm.dll`, `d3d8.dll`, `d3d9.dll`, `d3d11.dll`, `dxgi.dll` or
   `opengl32.dll` there, whether `--dgvoodoo` put it there or a session did, as with cnc-ddraw) refuses every shot
   of the run before the game starts. The reason: on rundesk's desktop a window drawn through a GPU API showed
   **the user's own screen**, through the window DC (Water Bugs under dgVoodoo2, 2026-10-31) and through
   `PrintWindow` (Mahjong Escape: Ancient Japan under dgVoodoo2, 2026-11-01); those shots were deleted. **Windows
   11's own DirectDraw loads `dxgi.dll`**, so this refuses the DirectDraw originals too (see "Hardware
   acceleration (3D mode)"). A window not drawn yet comes out flat, and `waitshot` re-grabs while the client area
   is flat. A game that takes the screen full-screen through DirectDraw cannot open its mode on the headless
   desktop and never draws — see Gaps.
5. **Scripted input and focus.** Proven end to end: clicks, typed text and `focus lose`/`regain` all register
   (a test window showed the click count rise, the text appear, and the active flag and colour change with the
   focus), and on a real game the trial dialog's Play button is clicked and the game starts. rundesk's desktop is
   shared by every session's test runs, so another game's window can lie over a click point; a click goes only to
   a window of the game's own process (before 2026-11-01 it went to whatever window lay there, so clicks were lost
   to other sessions' games; the log names the first few such cases).
6. **Message boxes are logged and answered.** A separate worker thread, attached to the game's desktop, watches
   for standard dialog windows (class `#32770`) belonging to the game's process. Each is logged with its
   caption, text, button labels and the run time, then answered: with the button a `msgbox` rule names, else
   the dialog's own default button, by posting `BM_CLICK` to that button. Only documented window APIs are used;
   nothing is injected into the game. Checked: a test program that pops two boxes was answered correctly — the
   first by a `msgbox No Trial expired` rule (it returned IDNO) and the second by its default (it returned IDOK)
   — both logged, and the program then exited, which proves the clicks landed. See the gaps below for what this
   cannot do (it sees a box only once it is already on screen).

Registry and save redirection is **not** implemented (the user allowed the originals to use their real registry
and saves). A run therefore writes the game's settings and trial state to the real registry and save folders,
which the user permitted.

## Recording sound (`--sound wav`)

With `--sound wav` the stand-in advances a fake play cursor in real time (so a streaming source such as BASS or
FMOD keeps writing), and writes to `snd/`:

- a static sound as `play_<seq>_buf<id>.wav` each time it is played, in play order, at the buffer's declared
  format — so a session can measure each sound's RMS level and when it starts;
- a looping/streaming buffer (mixed music) as one growing `stream_buf<id>.wav` (capped at ~90 s).

The sound log (`logs/sound.log`) lists every buffer create, data write (size + hash + format), play
(once/looping, volume, pan, frequency), volume/pan/frequency change and stop, each with a timestamp — enough to
line up "which sound plays when" against the port without the WAVs. This catches sounds a game plays through
DirectSound; a game that mixes elsewhere needs its own path handled (Gaps).

## Patching the copy

Some copies are not activated, and for a faithful comparison the original must sometimes be made to match the
port (the same RNG seed, no trial nag). origrun applies a **per-game list of static byte patches to the staged
copy's files** before it launches — the same kind of change as the activation patches the user already expects
on an unactivated copy, done to a file on disk, not injected into a running process. It is used for two things:

- **pinning the RNG seed**, so a run gives the same boards and other random content as the port it is compared
  with. The `{seed}` token in a patch stands for `--seed`'s value as four little-endian bytes;
- **any patch a session needs for a clean comparison**, such as one that lets the copy skip its trial prompt.
- **hover** (approved by the user on 2026-11-01, for test copies only): rundesk's desktop has no cursor to read, so
  a game that asks `GetCursorPos` where the mouse is never sees it over a button, and the patch makes it read the
  posted message's own coordinates instead.

<!-- private -->
The hover patch, as found on Bookworm Adventures: SexyAppBase's handler for a deferred message (FUN_00547ea0 in its
export) takes `WM_MOUSEMOVE` at 0x548635 by asking `GetCursorPos` for the point and `WindowFromPoint` for the window
under it, then tests the point against the client rectangle. On rundesk's desktop `GetCursorPos` fails with
`ERROR_ACCESS_DENIED` and leaves the point unset, so the test never passes and no button lights. The patch makes
that path take the point from the message's own `lParam` (already copied to a local, local_44). The game session
that knows the code supplies the exact bytes in its `patches/<game>.txt` (kind `test`); origrun knows none of them.
<!-- /private -->

origrun does not know or generate any of these bytes. A game session that knows its own code supplies the exact
expected and replacement bytes. **No keygens**: patch the check in the copy, or set the registry values the game
itself writes when registered — never generate a registration code.

The list is `tools/origrun/patches/<game>.txt` (or `--patches <file>`), one patch per line:

    <kind> <file> <where> <expected> <replacement> <reason...>

`kind` is a free word for the log (`seed`, `trial`, `test`); `file` is a path under the copy; `where` is
`off:0xN` (a file offset), `rva:0xN` or `va:0xN` (converted through the file's PE section table); `expected` and
`replacement` are equal-length hex (`.`/`_` may group the digits), and `replacement` may contain `{seed}`. Every
entry is checked against the file's current bytes first, and **one mismatch or malformed line refuses the whole
list and stops the run**, so a wrong or stale list never runs and a patch is never half-applied. Each patched
file keeps its original bytes in `<file>.origrun-unpatched`, which every run restores before re-applying, so a
list is always checked against the originals and `--no-patches` runs the copy unmodified.

The patch lists and the per-game config are git-ignored (`tools/origrun/patches/`, `tools/origrun/*.cfg`): a
trial-skip entry is a protection detail, so it stays on this machine like `games.tsv`'s paths.

## Timing to the game's updates

Captures and input can be timed to the game's own update counter instead of the wall clock, so frame-rate
variation drops out of a comparison. origrun reads the counter from **outside** the process with
`ReadProcessMemory` on the game it launched (a documented debugging API; nothing is injected), at an address a
game session gives in the per-game config:

    counter=[0x4d125c]+0x368      # the int 0x368 bytes past the pointer stored at 0x4d125c
    flips=[0x4d125c]+0x364        # optional: a counter the game bumps once per presented frame
    update_ms=10                  # one update's length, for a clip's interval_ms

`[0xPTR]+0xOFF` reads the pointer at `PTR` then the int `OFF` bytes into it — for a SexyAppBase game, the app
pointer's global and `mUpdateCount` (each session supplies its own; the offset is in its `re/<game>/`). A plain
`0xADDR` reads the int at that address. Brackets nest, so `[[0x8259e4]+0x36c]+0x38` follows two pointers (the
3D flag below), and an offset may also be subtracted. Then `at <N> <command>` waits for the counter, suspends the window
thread, runs the command and resumes; `burst <name> <count> update <k>` captures every `k` updates the same way.

**Precision (measured on Noah's Ark):** `at` lands on the exact update — `at 1500/1700/1900` each fired with the
counter reading exactly that value — because the ~1 ms poll is far finer than the ~10 ms update. The hold is kept
as short as it can be, because the game's clock runs on while it is held, and on resume the game catches up on
updates it never draws. Input under `at` is posted with no gaps between its messages (the queue keeps their order,
and the game reads every pending message before its next update), so a held click holds the game for about
0.04 ms; a held shot resumes the game as soon as the pixels are grabbed, before the PNG is made and written (about
3-5 ms held). The log gives each hold ("at click: the game was held 0.04 ms"; per burst, the average and longest
hold and how many frames were held past their update).

The held capture uses the window-DC blit, not `PrintWindow` (which never returns while the window's thread is
suspended), and like every capture it is refused for a game that has loaded a GPU API (guarantee 4); the module list
is read before the game is held and again while it is. Since Windows 11's DirectDraw loads `dxgi.dll`, the held shots
measured below on Bookworm Adventures were taken before this rule and cannot be repeated now. Input under `at` is
still held. A hold begins only once the suspension has taken effect (`GetThreadContext` returns after it has), and
if origrun crashes while it holds the game, it resumes the game first. A
SexyAppBase game counts update `N` at its start and draws after it, so a shot held as soon as the counter reads `N`
races that draw and mostly gets update `N-1`'s frame. Without `flips`, a held shot therefore waits for update `N+1`
to begin, when update `N`'s frame is on screen; with `flips` set it instead waits for the next present (capped so a
static screen, which presents nothing new, does not stall). SexyAppBase's catch-up can still double or skip an
update under load; a per-game patch could force one update per frame if a game ever needs it (not needed so far).

**Measured on Bookworm Adventures (2026-11-01)**, the pilot's battle against the port, update by update with the
pilot's `lag.py` measure: the attack's moving frames match the port at the same update (22 of 25 frames, the rest
one update off, in two runs), where before they ran one or two updates behind, jittering by one. After the battle's
load the original's frames match the port's of 4-5 updates earlier: a long load makes the game catch up on updates
without drawing, and its curtain opens only once drawn, so this depends on the load's time on the machine, not on
origrun's holds (3-5 ms per burst frame). Only freezing the game's clock across the load would remove it.

## Hardware acceleration (3D mode)

A SexyAppBase game with a 3D mode (the Options dialog's Hardware Acceleration checkbox) keeps the player's choice in
the registry value `Is3D`, a DWORD under the game's own key. At start, MakeWindow reads it and DDInterface::Init runs
the Direct3D test (D3DTester, whose results are cached in the key's `Test3D` subkey), then in 3D creates its draw
surface in video memory for Direct3D 7. When that fails, the game carries on in software for the session and saves
`Is3D=0` at exit. Which mode the game is really in is `mDDInterface->mIs3D`, which is what `Is3DAccelerated()` returns
and what the game's own code branches on.

games.tsv's optional columns 4 and 5 give the game's registry key (under HKCU) and the address of that flag, in the
counter's address syntax, read as one byte (Bookworm Adventures: `Software\PopCap\WinBAD` and
`[[0x8259e4]+0x36c]+0x38`, the app global, `mDDInterface`, `mIs3D`). When both are known, origrun polls the flag
every 100 ms while the game's window is up, logs the first reading and every change ("3D mode: the game reports ON
(Direct3D)" or "OFF (software)"), and ends with a "RESULT: 3D on/off" line. It also snapshots `Is3D` and the
`Test3D` values before every run and puts them back after it, with or without `--3d`: on rundesk's desktop the game
falls back to software and saves `Is3D=0`, and a test that re-runs there caches a failure, and either would keep 3D
off in the user's own play. `--3d on|off` writes `Is3D` for the run. The verdict uses the last reading taken before
`quit` (a value read while the game tears its DDInterface down does not count); a run whose mode differs from the
one asked for, or whose flag was never read, logs "RESULT: 3D MISMATCH" or "3D CHECK FAILED" and exits with code 3.
The older games keep their settings under `HKLM\SOFTWARE\PopCap\<Game>`; run unelevated, as here, the game reads and
writes Windows' per-user copy of that key instead, `Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\PopCap\<Game>`
under HKCU, so that is the key to name in games.tsv. If a run is ever elevated, the game reads HKLM itself and the
memory check reports the mismatch.

**3D mode cannot run on rundesk's desktop (found 2026-10-31).** With `Is3D=1`, Bookworm Adventures' cached test
results say the card passed (no failure, no warning), so its test is not the problem. But DDInterface::Init fails
with "CreateSurface Windowed DrawSurface: DDERR_NODIRECTDRAWHW" (read from the game's memory), and the game runs in
software and saves `Is3D=0`. A small probe making the same DirectDraw calls shows why. On the private desktop
DirectDraw has no hardware layer at all (its HAL caps are bare and report no video memory), and the 3D surface fails
the same way. On the session's own desktop the same calls get the full HAL, the 3D surface and a Direct3D 7 HAL
device. Windows gives legacy DirectDraw hardware only to the input desktop, which a hidden desktop never is. Direct3D 9
is no better there: its HAL device fails with D3DERR_NOTAVAILABLE. Direct3D 11 does get a hardware device (feature
level 11_0; Present reports the window occluded but still presents). A first test found that PrintWindow captured
the window of a flip-model swap chain correctly. A blit-model window came out blank, and so did the window-DC blit
that `at` and update-timed bursts use, for both models. So on the user's own desktop the originals most likely do run
in 3D. It is origrun's hidden desktop that keeps them in software.

**Captures of a GPU-drawn window are not safe, and origrun refuses them (2026-11-01).** Twice on 2026-11-01, a
capture of a game drawing through Direct3D 11 (dgVoodoo2 in the copy) returned what was on the user's own screen
instead of the game. Once it was the window-DC blit, and once PrintWindow itself, in Mahjong Escape: Ancient Japan.
Since then, origrun refuses every shot of a game that has loaded DXGI, Direct3D 8 to 12, OpenGL or Vulkan, and every
later shot of that run, and its log names the module. Until a capture method is proven safe and the coordinator says
so, never put dgVoodoo2, cnc-ddraw or any other GPU wrapper into a copy that origrun captures. A shot that shows
anything but the game's own window is deleted at once, unopened where possible, and reported to the coordinator.

**Windows 11's own DirectDraw loads DXGI (found 2026-11-01).** Noah's Ark and Ning Po, run with no wrapper in the
copy and so in software DirectDraw, as every original runs on rundesk's desktop, had `dxgi.dll` loaded by the time
their window came up; Noah's Ark later also had `d3d11.dll` and `D3D10Warp.dll`. A plain GDI test window run the
same way loads none of them, so it is DirectDraw that brings them in. origrun therefore refuses every shot of these
games, and very likely of every DirectDraw original, until the coordinator decides what is safe to capture.

**dgVoodoo2 runs the 3D mode there, but no run that takes shots may use it.** It implements DirectDraw and
Direct3D 7 over Direct3D 11 and presents flip-model. Its official source is its author's GitHub releases page,
`github.com/dege-diosg/dgVoodoo2/releases`; version 2.87.5 (`dgVoodoo2_87_5.zip`, 9,268,078 bytes, SHA-256
`5ffde6927f7355ca3fdd5d785b581256a8e6539fa13e395a891ade6ba1040850`) was downloaded with the user's OK and unpacked
on this machine in `E:/retools/dgVoodoo2_87_5`, the folder to give `--dgvoodoo` or `ORIGRUN_DGVOODOO`. **dgVoodoo2 is
freeware that may not be redistributed**: its files are never committed or copied into the repository, and origrun
copies them only into a staged copy under `build/` or `--stage-root`.

`--dgvoodoo` copies `DDraw.dll` and `D3DImm.dll` (from the folder's `MS\x86`, or the folder itself) into the copy
and writes a `dgVoodoo.conf` of origrun's own beside them: output through Direct3D 11 at feature level 11_0, not
full-screen (`FullScreenMode = false`, `AppControlledScreenMode = false`, so a game that asks for the screen gets a
window), flip-model presentation (`PresentationModel = flip_discard`), no watermark and no splash
(`dgVoodooWatermark`, `WatermarkDisplayDuration`, the Glide `3DfxWatermark` and `3DfxSplashScreen`; DirectX has no
splash of its own), the mouse not captured, Alt+Enter not toggling the screen mode. It refuses to write into the
install folder (or anything under `Program Files` or `steamapps`) and never overwrites a DLL the copy already
has. A marker file in the copy (`origrun-wrapper.txt`) lists each file it placed with its size and SHA-256, each
line written only once its file is in place; a placement that fails takes back what it wrote. The next run deletes
those files first, each only while its size and hash still match, and only when its full long path and its final
path (junctions resolved) lie under `build\` or `--stage-root`. origrun deletes nothing anywhere else, and places
the wrapper only where it may take it out again. A file that changed or cannot be deleted stays in the copy and in
the marker, and the run stops, since a wrapper must not carry over into a run that did not ask for it: delete it by
hand or stage a fresh copy. So a run without `--dgvoodoo` runs the game on Windows' own DirectDraw again.

**`--dgvoodoo` refuses to start a run that takes shots**: a script with `shot`, `waitshot`, `record`, `burst` or
`at <N> shot|burst`, a script it cannot read, or no script at all (such a run takes a title shot). It serves runs
that need no picture, such as the 3D-mode check below.

**3D mode proven on Bookworm Adventures (2026-10-31).** With `--3d on` and dgVoodoo2, the game reports 3D on from
about 2 s after launch to the end of a one-minute battle and ends "RESULT: 3D on, as asked", so the wrapper keeps
presenting while rundesk's window counts as occluded.

Before the refusal, the pilot's battle (`games/bookwormadv/tools/pilot`: the map, the entry transition, the battle
intro, the word RAIN and its attack) was compared through shotdiff with the port, 3D on against the port with
`Is3D=1`, 3D off against the port with `Is3D=0`. The midground island (the pale land on the right, behind the enemy)
is drawn by both sides in 3D and by neither in software, as the game's 3D-only path says. In 3D, five of the six
stills differ only faintly (0.1–1.3% of the pixels); the sixth, after the attacks, differs at the enemy spearman's
animation (1.1%). In software the stills differ by 0.2–1.4%: the same spearman, and the Attack button's sparkles at
other positions. Both clips line up in time (offset 0, no drift, no stalls); the attack clip flags the moving sprites
in 8 of 12 frames in 3D and 12 of 12 in software, and the entry transition's fourth frame differs by 48% in both modes
alike, so that is the transition's timing, not the renderer. ArrowHint's fade and the Board's zoom are not reached by
this battle (they come from the tutorial scripts) and are untested. How far dgVoodoo2's Direct3D 11 rendering differs
from a native Direct3D 7 driver cannot be measured here, since no native driver runs on the hidden desktop; on this
battle it is as close to the port as software mode is.

Also before the refusal, Water Bugs and Cosmic Bugs, run with `--dgvoodoo` (no `--3d`; they have no 3D mode),
reached their title menus: the game's full-screen request becomes a borderless window the size of the desktop
(3840×2160 here) with its 800×600 picture stretched over it.

Bookworm Adventures in software mode is proven: `--3d off` ends "RESULT: 3D off, as asked". Its main menu at update
1600 was compared through shotdiff with the port run on SDL's software renderer (`SDL_RENDER_DRIVER=software`). For
now the port ignores `Is3D`, and its `Is3DAccelerated()` follows SDL's renderer; the Bookworm Adventures session is
making it honour the setting. The menu has no 3D-only effect: the port's two renderers differ there in 0.2% of the
pixels, faintly. Its 3D-only code paths (ArrowHint's fade, the Board's zoom, TileEngine) are all in battle. Without
dgVoodoo2, `--3d on` reaches no 3D at all, as above.

## Per-game status

The 18 games of `BYOPOP_WEB_FINISHED`, as of the first runs on 2026-10-31. "Trial" means the copy shows
PopCap's trial screen first; its **Play** button is clicked through with `clickready` (it is not a hard lock
while plays/time remain). Captures are the main window's client size. These are first findings, not a finished
comparison; the per-game session does the screen-by-screen pass.

The 3D column says whether the game itself (its real exe, not the launcher that wraps it) has SexyApp's Direct3D 7
mode, and what `--3d` reaches (see "Hardware acceleration (3D mode)"). Only Bookworm Adventures has its registry
key and 3D-flag address in games.tsv so far; each session adds its own game's.

Round 2 (2026-10-31) added update-timed capture, bursts, the copy patch list, and fixes below; the table is
updated for it. The Capture column records what was taken before 2026-11-01: since then origrun takes no shot of a
game that has loaded a GPU API, and on Windows 11 that includes the DirectDraw originals (see "Hardware acceleration
(3D mode)").

| Game | Source | Reached under origrun | Capture | 3D | Audio seen | Notes |
|------|--------|-----------------------|---------|----|------------|-------|
| alchemy | local install | trial, then into the game past Play | yes | has one; on needs `--dgvoodoo` (untried) | muted a session | some sound is not via DirectSound |
| atomica | local install | FATAL ERROR at startup, then exits | — | none | none | FMOD fails to load `sounds\capclick` (Ogg); not the stand-in or display — the Atomica session's to fix |
| mummymaze | local install | title after a slow (~45 s) start | yes, 646×509 | none | none | window is late to appear |
| noahsark | local install | own title screen | yes, 646×509 | none | none | update counter proven here (`[0x4d125c]+0x368`); `at` lands on the exact update |
| sevenseas | local install | own title screen | yes, 646×509 | none | none | |
| pixelus | local install | trial screen (not clicked through yet) | yes | has one; on needs `--dgvoodoo` (untried) | none | games.tsv's Pixelus.exe (~256 KB) is a smaller build than the port's decompiled exe — its counter offsets do not apply to it |
| tiptop | local install | trial, plays | yes | none | none | DirectInput: posted clicks are ignored; the `hw*` commands are unconfirmed (see the gaps) |
| wordharmony | local install | trial (60 min), Play present | yes | none | none | |
| bookwormadv | local install | in-game Updates dialog | yes, 806×629 | **off and on proven** (on with `--dgvoodoo`) | muted a session | its sound is not via DirectSound (BWA audio work); stage under a short `--stage-root` (long worktree path makes its launcher exit) |
| ningpo | local install | main window | **yes** (after init) | none | none | windowed DirectDraw; `waitshot`'s flat-retry now captures real content once it has drawn |
| waterbugs | local install | window, then "Screen Setup Failed"; **title menu with `--dgvoodoo`** | window only; with `--dgvoodoo` the title, 3840×2160 stretched | none | none | full-screen DirectDraw: the headless desktop cannot open its 800×600 mode; dgVoodoo2 gives it a desktop-sized window |
| cosmicbugs | local install | window, then "Screen Setup Failed"; **title menu with `--dgvoodoo`** | window only; with `--dgvoodoo` the title, 3840×2160 stretched | none | none | same as Water Bugs |
| dynomite | Steam copy | **title and on** | **yes** | has one; on needs `--dgvoodoo` (untried) | none | fixed: `Steam.dll` is now auto-copied from the Steam library root into the copy |
| bejeweled | wrapped launcher | **title, prepared copy runs** | **yes** | has one; on needs `--dgvoodoo` (untried) | none | prepared as the private notes below say, run via an absolute `--exe` |
| bigmoney | wrapped launcher | prepares cleanly | — | has one; on needs `--dgvoodoo` (untried) | — | prepare as Bejeweled (not launched yet) |
| bookworm | wrapped Steam exe | prepares cleanly | — | has one; on needs `--dgvoodoo` (untried) | — | prepare as Bejeweled (not launched yet) |
| rocketmania | Steam copy | not run | — | has one; on needs `--dgvoodoo` (untried) | — | Steam copy: `Steam.dll` is auto-copied now, as for Dynomite |
| bookwormadv2 | Steam copy | not run | — | has one; on needs `--dgvoodoo` (untried) | — | not wrapped; `Steam.dll` auto-copied now |

The install paths are in `tools/origrun/games.tsv`, kept out of any publish (`.publishignore`).

<!-- private: Preparing a wrapped or Steam copy to run under origrun is recorded on this machine only. -->
Preparing a wrapped or Steam copy to run under origrun:
- PopCap's Steam retail launchers wrap the real game. The wrapper is detected by the marker `!popcapdrmprotect!`
  (or `!popcapcdprotect!` for the CDProtect launchers). `WinBej.exe` (Bejeweled), `WinBM.exe` (Big Money) and
  `Bookworm.exe` (BookWorm Deluxe) carry one; `BigMoney.exe` and `BookwormAdventuresVol2.exe` do not.
- Extract the real PE with `tools/extract_game_exe.py "<copy>\<wrapper>.exe" -o "<copy>\<real>.exe"` (the tool is
  in `.publishignore`). Stage a copy first with `origrun --game <name> --copy-only`, carve inside that copy, then
  run it with an **absolute** `origrun --exe "<copy>\<real>.exe"` (a relative `--exe` path breaks the
  process-image match, so no window is found). Proven on Bejeweled: `WinBej.exe` carved to a 1.4 MB
  `BejeweledGame.exe`, which runs and captures. `WinBM.exe` and `Bookworm.exe` carve the same way.
- A Steam copy also needs `Steam.dll` beside it, or it shows the "Unable to load Steam.dll" box. origrun now
  copies `Steam.dll` from the Steam **library root** (a parent of the game folder, e.g.
  `A:\SteamLibrary\Steam.dll`) into the copy automatically when the copy lacks one. This fixed Dynomite (it now
  reaches its title and captures) and applies to Rocket Mania and Bookworm Adventures Volume 2.
- Activation/trial: a game session supplies a `tools/origrun/patches/<game>.txt` entry (see "Patching the copy")
  to make its carved copy act registered; origrun does not generate codes.
<!-- /private -->

## Gaps / next steps

Known limits of the message-box watch (guarantee 6). These are open; the coordinator is taking them to the
user:

- **A box's own sound still plays.** The watch sees a dialog only once it is already on screen, so the sound
  Windows plays as the box appears (an error chime, a question ding) has already sounded before the box is
  logged and dismissed.
- **`MessageBeep` and `PlaySound` system sounds are not seen.** These make a sound without a window, so the
  watch — which looks for dialog windows — neither logs nor stops them. The Core Audio watch may still catch and
  mute a sound routed through the default audio session, but a system-sounds beep is not reliably caught.
- **No guard against URL or program launches.** Nothing in origrun blocks an original that calls
  `ShellExecute`/`WinExec`/`CreateProcess` (for instance a "More Games" `steam://` link, which would bring up the
  user's Steam client). The only safeguard is the rule that **a script must never click a link, "Buy" or "More
  Games" button**; click only Play and the game's own controls.

Other gaps:

- **3D needs dgVoodoo2**: legacy DirectDraw has no hardware on the hidden desktop, so an original with a 3D mode
  runs in 3D only with `--dgvoodoo` (see "Hardware acceleration (3D mode)"). Proven on Bookworm Adventures only;
  each session adds its game's registry key and 3D flag to games.tsv. With the wrapper no shot is taken, and
  `--dgvoodoo` refuses to start a run that takes one.
- **No shots of DirectDraw originals for now**: Windows 11's DirectDraw loads `dxgi.dll` (Noah's Ark, Ning Po), and
  origrun refuses every capture of a game with a GPU API loaded (guarantee 4). Which captures are safe is the
  coordinator's to decide.

- **Windowed DirectDraw now captures** (Ning Po): a window is often flat only because it has not drawn its first
  frame, so `waitshot`/`capture_ready` re-grab while flat and get real content once it appears. **Full-screen
  DirectDraw still does not** (Water Bugs, Cosmic Bugs): on the headless private desktop the game cannot open its
  800×600 exclusive mode ("Screen Setup Failed") and never reaches its content. The least-invasive paths
  (PrintWindow, a window-DC blit) can't help because the game refuses to run without the screen, and
  Windows.Graphics.Capture needs a displayed window and DWM, which the private desktop has not. Reaching them
  needs the game forced into a windowed mode (toggle not yet found) or a **ddraw stand-in** that reports a
  display and copies the primary surface (the documented last resort; built like the dsound stand-in, not yet
  written). **Update 2026-11-01:** `--dgvoodoo` forces the window and both reach their title menus, in a
  desktop-sized window with the picture stretched (see "Hardware acceleration (3D mode)").
- **DirectInput games** (TipTop): the `hw*` commands (`SetCursorPos` + `SendInput`) are **unconfirmed**. Windows
  documents both calls as working only from the input desktop, which the private desktop is not, so they may fail
  silently there. TipTop's trial screen changed right after an `hwclick`, but no control run (the same script
  without the click) has ruled out chance, or the user's own clicks reaching the game through DirectInput. Before
  relying on them: log `SendInput`'s and `SetCursorPos`'s results, make a control run, and never switch the driver
  thread to the input desktop, which would move the user's cursor and click on their screen. The in-process
  import-patch route stays **dropped** (it was stopped by a permission check in round 1; see the hard rule).
- **Atomica won't start**: it shows "FATAL ERROR — Failed to load sound: sounds\capclick" and exits before any
  window, with the dsound stub, with `nodriver`, and with real audio alike. The Ogg and `fmod.dll` are present,
  so it is FMOD's init/decode of this copy on Windows 11 (or a game-data path) — the Atomica session's to fix;
  origrun cannot capture a game that will not start.
- **Audio not carried by DirectSound** (some Alchemy and Bookworm Adventures sound, and FMOD games) is not
  silenced by the `dsound` stand-in. The Core Audio watch mutes the game's session and reports it; for a run that
  must be truly silent, use `--sound real` with the speakers off, or rely on the watch's mute.
- **Long staging paths**: some launchers (Bookworm Adventures) exit at once when the staged copy sits under a
  worktree's long `build/origrun` path. Use `--stage-root <short dir>` to stage the copy under a short path
  (shots and logs stay under the run folder).
