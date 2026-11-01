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

**Captures are taken only inside Windows Sandbox** (`--sandbox`; see "Windows Sandbox"). On the host, origrun takes
no shot of any kind: three captures there showed the user's own screen ("The host-capture ban").

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

A run on the host needs `build/native/rundesk.exe` (`cmake --build build/native --target rundesk`). A `--sandbox`
run needs the Windows Sandbox feature and its `wsb` command (Windows 11 24H2 or later), and no rundesk.

## Use

    origrun --game <name> --script <file.txt> [options]
    origrun --exe "<path to an already-placed copy>" --script <file.txt> [options]

`--game <name>` looks the install up in `tools/origrun/games.tsv` (name, install folder, exe leaf, and optionally
the game's registry key and 3D-flag address, for `--3d`) and copies it into `build/origrun/<run>/game`. The
repository's table is read first; a copy beside `origrun.exe` is only a fallback. `--exe` runs a copy you placed yourself (used for a game whose real exe
you first carved out of a wrapper; see below). Captures land in `build/origrun/<run>/shots`, logs in
`.../logs`, recorded WAVs in `.../snd`.

Options:

- `--run <id>` — the run folder name under `build/origrun/` (default: the game name). One plain folder name:
  letters, digits and `- _ . +` only, no `..`, not starting or ending with a dot, at most 64 characters; anything else
  is refused, since a sandbox maps the folder read-write. Each run takes `<run>/run.lock`, opened with no sharing,
  before it writes anything in the folder, so a second origrun with the same `--run` is refused ("another origrun is
  using the run folder") instead of staging over the first; Windows lets the lock go when origrun ends.
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
  e.g. `--stage-root C:\byo-og` or main's `build/origrun`. Not with `--sandbox`, where the copy's path inside is
  always the short `C:\run\game`.
- `--sandbox` — run the game inside a fresh Windows Sandbox instead of under rundesk, the only way to take shots
  (see "Windows Sandbox"). `--sandbox-vgpu` gives the sandbox the host's GPU, for 3D; without it the sandbox renders
  in software. `--sandbox-wait <sec>` is how long a run waits in the queue while another run's sandbox is up
  (default 600). `--sandbox-probe` starts a sandbox with no game, in which origrun writes what it finds (the sandbox
  check, sessions, display, DirectDraw, folders, network) to `build/origrun/sandbox-probe/probe.txt`.
- `--reg <file.reg>` — registry presets for a game run with `--sandbox`, imported inside before the game starts,
  where the game looks for them (see "Saves and the registry"). Refused on the host and with `--sandbox-probe`.
- `--counter <expr>` / `--flips <expr>` / `--update-ms <ms>` — the game's update counter, for update-timed
  capture and input (see "Timing to the game's updates"). Usually set once in the per-game config instead.
- `--pending-draw <expr>` / `--update-state <expr>` / `--draw-empty <expr>` — a SexyAppBase game's
  `mHasPendingDraw` (one byte) and `mUpdateAppState`, given together, so that a shot inside Windows Sandbox shows
  exactly the update it is labelled with. `mLastDrawWasEmpty` (one byte) is optional and tells a skipped draw apart.
  In the per-game config these are `pending_draw=`, `update_state=` and `draw_empty=`. Giving only one of the first
  two logs a warning. See "Timing to the game's updates".
- `--watch <expr>` — a value in the game's memory (read as 32 bits), logged at each change with the update counter:
  "watch 1: 48 -> 47 at update 2150", and "-" while it cannot be read (through a null pointer, say). Up to four, each
  polled every millisecond and logging at most 4000 changes. It measures how many updates something lasts in the
  original, such as a loading screen whose length follows the machine's speed (Bookworm Adventures' pilot watches its
  chapter load screen's timer and hands the count to the port). Measure with the update numbers, not by counting
  changes: after a held shot the game runs its catch-up updates back to back, faster than a poll, so a value can skip.
  The game's process is looked up again every 100 ms (and soon after a failed read), not at every read: on the
  sandbox's desktop a window enumeration costs about a millisecond.
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
  word is `env`. On the host, `--3d on` uses `ORIGRUN_DGVOODOO` by itself when it is set. Needed for any 3D run on
  rundesk's desktop, and it also gets the full-screen games past "Screen Setup Failed" there (see "Hardware
  acceleration (3D mode)"). Like every host run, a host run with it takes no shot.

A run on the host that would take a shot (a script with `shot`, `waitshot`, `record`, `burst` or `at <N>
shot|burst`, a script origrun cannot read, or no script at all, since such a run takes a title shot) is refused
before anything is copied; `--copy-only` is not refused. `--in-sandbox <dir>` is origrun's own mode inside the
sandbox and refuses to run anywhere else.

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
  every `k` updates after, each taken at its update as `at ... shot` takes one (inside the sandbox, the game held at
  the update and released to answer `PrintWindow`; see "Timing to the game's updates").
  `clip.json` records `k * update_ms`. Needs an update counter (below). Frame-rate variation drops out of this.
- `hwmove <x> <y>` / `hwclick <x> <y>` / `hwrclick <x> <y>` / `hwkey <vk>` — input through the desktop
  (`SetCursorPos` + `SendInput`) instead of a posted window message, for a game that reads the mouse with
  DirectInput or `GetCursorPos` and so ignores posted clicks (TipTop). Plain `click`/`key` still post messages,
  which drive the SAF/LOWOS message path. **Only inside Windows Sandbox** (`--sandbox`), as shots are: on the host
  these calls reach the user's own input desktop, whatever desktop the game is on. A host run whose script has any
  of them is refused at the start, and each one is refused (logged) should it run on the host anyway. In the
  sandbox `hwmove` lights no hover on a SAF game (Seven Seas, 2026-11-01): use a posted `move` for hover there.
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
2. **Invisible.** On the host the game runs under `build/native/rundesk.exe`, on the private `bejeweled-test`
   desktop at below-normal priority; nothing is shown and focus is never taken from the user. Checked: the host
   never shows a window; it drives the game by attaching a worker thread to that desktop (`SetThreadDesktop`), which
   is also required for the window handles there to be usable. With `--sandbox` the game runs inside the sandbox,
   whose window origrun never opens; a guard stops the sandbox at once if one appears (see "Windows Sandbox").
3. **Silent by default.** The `dsound.dll` stand-in beside the copy makes `IDirectSound`/`IDirectSoundBuffer`
   objects that accept every call and never open a device, so DirectSound and anything on top of it (BASS uses
   DirectSound) stays silent. As a second check the host watches Windows Core Audio: if a session appears for
   the game it mutes it and says so in the run result. A game that plays through another path (winmm `waveOut`
   / `PlaySound`, or FMOD on its own output) is not silenced by the dsound stand-in yet (see Gaps); the audio
   watch still mutes and reports it, and for such a run the user turns the speakers off.
4. **Screens, only inside Windows Sandbox.** `shot`/`waitshot`/`record`/`burst` capture the window with
   `PrintWindow`, or a blit of the window's own DC where `PrintWindow` fails or comes out flat, and write a PNG.
   Inside the sandbox only `PrintWindow` sees a game: there a blit of the window DC or of the screen DC (with or
   without `CAPTUREBLT`) returns success but copies no pixels, held or not, and DXGI desktop duplication finds no
   output to duplicate in session 0 (`EnumOutputs` gives `DXGI_ERROR_NOT_FOUND`), measured on Bookworm Adventures
   (2026-11-01). Update-timed shots therefore use `PrintWindow` too ("Timing to the game's updates").
   Nothing ever reads the screen. origrun captures only when it has found for itself that it runs inside Windows
   Sandbox (see "Windows Sandbox", "How origrun knows it is in the sandbox"), where the only display is the
   sandbox's own, and there it captures whatever the game has loaded, GPU APIs included. On the host every capture
   is refused, asked when each capture starts, before a window-DC read that follows a `PrintWindow`, and once more
   before the pixels are saved, and a run that would take one is refused before it starts ("The host-capture
   ban"). The log still names the GPU modules the game has loaded (`dxgi`, `d3d8`, `d3d9`, `d3d10*`, `d3d11`,
   `d3d12`, `opengl32` or `vulkan-1`, read from outside with `EnumProcessModulesEx`) when its window comes up. A
   window not drawn yet comes out flat, and `waitshot` re-grabs while the client area is flat. A game that takes
   the screen full-screen through DirectDraw cannot open its mode on rundesk's headless desktop and never draws;
   whether it can on the sandbox's desktop is untested (see Gaps).
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

Registry and save redirection is **not** implemented on the host (the user allowed the originals to use their real
registry and saves). A host run therefore writes the game's settings and trial state to the real registry and save
folders, which the user permitted. A `--sandbox` run keeps all of that inside the sandbox, where it vanishes with it.

## Windows Sandbox

### The host-capture ban

origrun takes no shot on the host, of any kind and of any game (2026-11-01, after the third case). Three captures on
the host returned the user's own screen instead of the game. On 2026-10-31 a window-DC blit of Water Bugs, drawn
through dgVoodoo2's Direct3D 11, showed it; on 2026-11-01 at 01:13 a flat `waitshot` of Bonnie's Bookstore, from a
build that still fell back to the screen DC, showed the user's browser bookmarks bar; and at 01:19 every
`PrintWindow` shot of Mahjong Escape: Ancient Japan, run with 3D on through dgVoodoo2, showed another program on the
user's screen. Those shots were deleted. Windows 11's own DirectDraw loads `dxgi.dll` (Noah's Ark and Ning Po in
software mode, found the same day), so no rule about GPU modules could keep the DirectDraw originals safe either.
A host run is therefore scripted input without shots, and every capture comes from inside Windows Sandbox, where a
capture can only ever see the sandbox's own display. A shot that shows anything but the game's own window is deleted
at once, unopened where possible, and reported to the coordinator.

### Setup

Windows Sandbox is an optional Windows feature ("Windows Sandbox" under Windows Features, or `Enable-WindowsOptionalFeature
-Online -FeatureName Containers-DisposableClientVM` from an elevated shell, then a restart). It needs Windows 11 Pro
or Enterprise with virtualization on. The feature itself brings only `WindowsSandbox.exe` and its client; the `wsb`
command comes with the Store package `MicrosoftWindows.WindowsSandbox`, which the built-in app downloads the first
time it is opened (Windows 11 24H2 and later). The user did that on 2026-11-01 (version 0.8.107.0); origrun itself
downloads nothing. origrun finds `wsb.exe` in System32 (through `Sysnative`, since origrun is 32-bit) or as the app
alias under the user's `WindowsApps` folder, where it is here, and fails cleanly when neither is there. The first `wsb`
call of a while starts the package's broker, `WindowsSandboxServer.exe`, in the user's session; it is no sandbox, it
ends by itself after a while, and origrun leaves it alone.

    origrun --game <name> --script <file.txt> --sandbox [--sandbox-vgpu] [options]

### What a run does

On the host, origrun stages the run under `build/origrun/<run>/` as before: the copy in `game/`, patched, with the
sound stand-in and any `--dgvoodoo` wrapper; the script as `script.txt`; the run's settings in `sandbox-run.txt`;
and the configuration it hands `wsb` in `sandbox.wsb`. It then takes the machine's sandbox lock, starts a sandbox
with `wsb start --id <guid> --config <the XML>`, and runs `C:\origrun\origrun.exe --in-sandbox C:\run` inside it with
`wsb exec -r System`, retrying while the sandbox boots until origrun inside writes `sandbox-started.txt` (or
`sandbox-done.txt`, which a run refused inside writes alone). It refuses to start while `wsb list` shows a sandbox
or a Windows Sandbox process runs (`WindowsSandbox.exe`, `WindowsSandboxClient.exe` or
`WindowsSandboxRemoteSession.exe`, by exact name; the broker is none), the user's own say, so the only sandbox it
ever stops is the one it started, and only through `wsb stop --id`. It waits for `sandbox-done.txt` (the inside
run's exit code) for the run's timeout plus 60 s, then stops the sandbox with `wsb stop`, checks with `wsb list`
that it is gone (stopping it once more if it is still listed after 10 s), releases the lock, and copies the inside
run's RESULT lines into its own log. Ctrl+C and a crash of origrun stop the sandbox too; a killed origrun cannot,
which "When origrun is killed" below covers. `wsb`'s own output goes to `logs/wsb.txt`, the inside run's log to
`logs/origrun-sandbox.log`, and the shots to `shots/`, all in the run folder on the host.

Inside, origrun puts the files staged under `<run>/sandbox-files/C/` onto the sandbox's `C:` (saves, below),
imports the registry presets (`--reg`, below), sets the game's 3D setting for `--3d`, and runs the game in a job of its own at below-normal priority, on the sandbox's
own desktop (`WinSta0\Default`), without rundesk's hidden one. `wsb exec -r System` starts origrun in session 0, and a
sandbox started without `connect` has no other session a game could use (below), so the game runs in session 0 as
SYSTEM. There the input desktop is `Winlogon`, and DirectDraw sets a display mode only on the input desktop, so origrun
first makes `Default` the input desktop (`SwitchDesktop`, inside the sandbox only). Should a sandbox ever have a user
session (WDAGUtilityAccount logged on), origrun runs itself again in that session as that user, with the user's
environment, and the game with it. The log lists the sessions it found and which it used.

### Headless: what Microsoft documents

The `wsb` reference on Microsoft Learn (`windows-sandbox-cli`) says `wsb start` "creates and launches a new sandbox"
and returns its id, and `wsb connect` opens "a new window with a remote desktop session". It does not say whether
`start` alone shows a window, but `wsb stop` "also closes the window that shows the sandbox desktop", which would
be the one `connect` opened. For `wsb exec`, a command run as the logged-on user needs an active user session, made
with `connect` beforehand, and fails without one; run as System, it runs in the system context with no such
condition. `exec` has no process input or output and returns the command's exit code. The `.wsb` file's
`LogonCommand` runs after the sandbox logs on, which again needs a user session. So origrun never runs `connect` or
uses `LogonCommand`, and starts its run with `exec -r System`.

**Tested on 2026-11-01** (two `--sandbox-probe` runs). `wsb start` accepted the whole configuration below and
returned in 1.3-1.5 s (a sandbox is cloned from a VM Windows keeps ready, `vmmemCmZygote`), with no window or
dialog on the user's desktop. Its new processes on the host were a VM worker (`vmwp.exe`) and
`vmmemWindowsSandbox`. The first `wsb exec -r System` ran at once: the marker its command writes appeared 1.7-1.9 s
after the start began, and origrun was running inside 0.3 s later. The sandbox then has no user session and no
console session at all: only session 0 and a listening remote-desktop session with no user. `wsb stop` took about
0.5 s, and 0.4 s later `wsb list` showed nothing and both processes were gone.

A guard thread looks, every 100 ms from before `wsb start` to after the stop, for a visible window on the desktop
the user sees of a Windows Sandbox process (the three names above), of the broker `WindowsSandboxServer.exe` or of
`vmconnect.exe`, matched by exact name, never by a part of one (another product's "sandbox", Sandboxie say, is not
Windows Sandbox). When it finds one, it stops this run's sandbox with `wsb stop --id`, which closes the sandbox's
windows; it never ends a process itself, so a window that is not this sandbox's costs only this run. The run logs
"STOP: a sandbox window was on the user's desktop" and exits with code 4. It has never fired. Every run ends with
"RESULT: no sandbox window showed on the user's desktop" or the STOP line.

### When origrun is killed

A killed origrun (`TerminateProcess`, as a task stop does) runs no handler, so it cannot stop its sandbox. Two
things stop it instead. The lock names the sandbox: just before `wsb start`, the run writes `sandbox=<id>` into
the lock's text, and it takes the line out only once `wsb list` confirms the stop. The next run that takes the lock
and finds such a line waits for the run that wrote it to end, if it still runs, stops that sandbox with
`wsb stop --id`, and goes on only once `wsb list` shows none. And the sandbox shuts itself down: the origrun that
`wsb exec` starts inside arms a power-off (`InitiateShutdown`) at the run's timeout plus 90 s, and again 30 s after
it writes `sandbox-done.txt`; a host that stops the sandbox first makes it moot. It arms it only inside, after the
sandbox check passed, as SYSTEM, run from `C:\origrun`. A probe arms the 30 s one.

**Tested on 2026-11-01** with probes. Killed 0.6 s into `wsb start`, origrun left no sandbox: the start never
finished, and `wsb list` was empty. Killed as soon as `wsb start` returned, before anything ran inside, it left a
sandbox that only the lock knew of; the next run logged "the lock names the sandbox ...", stopped it and ran its
own probe. Killed as soon as origrun inside had started, it left a sandbox that powered itself off: `wsb list` was
empty 32 s after the kill. The lock's line from that run was cleared by the next run (its `wsb stop` found the id
gone). A game run with both timers armed launched and ran as before (Ning Po, below).

### What the sandbox offers a game (found 2026-11-01)

The display is 1024x768 at 32 bpp, with 18 modes, of which none is 16 bpp (800x600 only at 32 bpp). Without
`--sandbox-vgpu` DirectDraw has no hardware layer (no video memory, no 3D), and it loads `dxgi.dll` there too. With
the input desktop as session 0 has it (`Winlogon`), every `SetDisplayMode` fails (`E_NOTIMPL`); after `SwitchDesktop`
to `Default`, exclusive full-screen 800x600x32 works with a flipping primary surface, while 16 bpp modes stay
unsupported. A game that asks for 16 bpp full screen may still get it through Windows' compatibility shims, which
apply inside the sandbox as on any Windows; Water Bugs and Cosmic Bugs will tell.

**The first game in the sandbox: Ning Po (2026-11-01)**, unpatched, no wrapper, one `waitshot`. `wsb start` took
8.2 s this time (a fresh template VM, it seems), origrun ran inside 2.5 s later, and the whole run, from the start
to nothing left on the host, took 18 s. The game ran in session 0 as SYSTEM on `WinSta0\Default`, made the input
desktop, windowed as it starts by default; its window came up a second after the launch with `dxgi.dll` loaded,
and the log said so ("GPU modules: dxgi.dll; inside Windows Sandbox, so shots are taken"). `waitshot` re-grabbed
three times while the window was still flat, then `PrintWindow` gave the game's own window (its title bar and the
first frame of its loading screen). The stand-in's "DirectSoundCreate failed" box came up and was answered, as on
the host. No audio session of the game appeared inside, and none of the sandbox on the host. The guard saw no
window on the user's desktop, and `wsb stop` (2 s) left nothing. Whether Ning Po could go full screen there was
not tried (that is its Options dialog); the probe says 32 bpp full-screen modes work and 16 bpp ones do not. A
second run the same day, with the shutdown timers armed and `--reg` presets, gave the same shot: `wsb start` took
1.1 s, the game's window was up 0.5 s after origrun started inside, and nothing was left 5 s after the start began.

### The configuration, and why

`Networking` is disabled, so a game cannot reach the internet or the user's network (no "More Games" page, no
update check, no Steam). `ClipboardRedirection`, `PrinterRedirection`, `AudioInput` and `VideoInput` are disabled,
so nothing crosses through the clipboard, a printer, the microphone or a camera. `ProtectedClient` is enabled, which
runs the sandbox's remote-desktop client in an AppContainer, in case a client is ever started. `vGPU` is disabled by
default, so the sandbox renders in software (WARP) and shares nothing of the host's GPU driver; `--sandbox-vgpu`
enables it for a 3D run (Microsoft notes that a shared GPU widens what the sandbox can reach of the host).

There are exactly two mapped folders: the run folder, read-write, as `C:\run`, and origrun's binaries
(`build/origrun-bin`), read-only, as `C:\origrun`. Never the install folder, the user's profile or the repository:
the copy is already in the run folder. Before it writes `sandbox.wsb`, origrun resolves both to their final paths
(`GetFinalPathNameByHandle`, so a junction or link shows where it leads) and refuses the run unless the run folder
sits directly under the repository's `build\origrun` and the binaries' folder ends in `\build\origrun-bin`, both on
a local drive; the configuration names those final paths, so what is mapped is what was checked. A path that does
not fit the configuration, escaped, refuses the run too, rather than being cut short.

### The lock: one sandbox on the machine

Every session's origrun opens the same lock file, `byo-pop\sandbox.lock` in the user's local application data
folder (as Windows reports it, `%LOCALAPPDATA%`), with no sharing, before it starts a sandbox. While another run holds
it, a run waits in the queue (logged, up to `--sandbox-wait`, default 600 s) and gives up after that. Windows closes
the handle when origrun ends, however it ends, so the lock cannot be left behind. Its text is `key=value` lines: the
holder's `pid`, `run`, `since` and `dir`, and while its sandbox may be up, `sandbox=<id>` (see "When origrun is
killed"). A sandbox run counts as one game process for the machine's limit of 3, while it waits in the queue as
well. The run folder has its own lock besides, `<run>/run.lock` (see `--run`).

### How origrun knows it is in the sandbox

`--in-sandbox` and every capture depend on a check origrun makes for itself, which no option or environment variable
sets. All three must hold. The firmware reports a Hyper-V virtual machine: SMBIOS's system structure, read with
`GetSystemFirmwareTable`, gives the manufacturer "Microsoft Corporation" and the product "Virtual Machine" (this
host's board reports MSI and MS-7D25). The local account `WDAGUtilityAccount`, the sandbox's own user, is enabled
(the host has the account, disabled). And origrun's own token is SYSTEM or `WDAGUtilityAccount`. On the host the
check fails and logs its three values; inside, the log's first line gives them. Inside it reads "SMBIOS Microsoft
Corporation / Virtual Machine, WDAGUtilityAccount enabled, running as SYSTEM" (2026-11-01).

### What is and is not isolated

The sandbox is a fresh virtual machine with its own kernel, display, file system, registry and user, and it vanishes
when stopped. What crosses to the host: the run folder, which the game and origrun can write and whose contents
stay (it lies under `build/`, git-ignored); the binaries, read-only (a write to `C:\origrun` is denied); with
`--sandbox-vgpu`, the GPU; and the host's CPU and memory. The sandbox has no network adapter at all. Sound: the
sandbox has no client, and so no remote-desktop audio path to the host; a run watches the host's audio sessions for
any of a Windows Sandbox process and logs the loudest peak ("RESULT: no audio session of the sandbox appeared on the
host" in both probes). The sound stand-in and the audio watch work inside as on the host.

### Saves and the registry

Saves, settings and trial state live inside the sandbox and vanish with it, so every run starts as a first run.
A run that needs a save stages it first: files inside the game's folder go into the copy (`game/`), and files
elsewhere go under `<run>/sandbox-files/C/` at their path on `C:` (for example
`sandbox-files/C/ProgramData/<game>/userdata/...`), which origrun copies onto `C:` before the game starts.

The registry starts as a fresh Windows has it. Inside, origrun and the game both run as SYSTEM, so the game's
HKCU is SYSTEM's own hive (`HKEY_USERS\S-1-5-18`), not the user's, and UAC never virtualizes SYSTEM: a game that
writes under HKLM writes HKLM itself, not the per-user `Software\Classes\VirtualStore\MACHINE\...` copy where the
host keeps its values. origrun follows the game. Its own reads and writes of a games.tsv key (`--3d`, the cached
`Test3D` results) open HKCU as the game does, and a key under `Software\Classes\VirtualStore\MACHINE\` at its HKLM
path, all through the 32-bit view the 32-bit games use (`HKLM\SOFTWARE\X` is then kept in
`HKLM\SOFTWARE\WOW6432Node\X`). In a user session the game would be virtualized as on the host, and nothing is moved.

**Presets: `--reg <file.reg>`.** A run that needs values in place (a setting, a registration, a high-score table)
gives them as a `.reg` file, for instance one exported with regedit from the host. The host copies it to
`<run>/sandbox-files/registry.reg` (a session may also put a file there itself, and it is then imported the same
way). Inside, the origrun that launches the game, before the `--3d` setting and the launch: moves every key under
`HKEY_CURRENT_USER\Software\Classes\VirtualStore\MACHINE\` to `HKEY_LOCAL_MACHINE\` (dropping a
`SOFTWARE\WOW6432Node` level, which the 32-bit view adds back), keeps HKCU keys as they are, writes the result to
`logs/registry-imported.reg` in the file's own encoding (UTF-16 or UTF-8 with a BOM, or ANSI), imports it with
`reg import /reg:32`, and logs reg's exit code and how many of the keys exist afterwards ("2 of the 2 keys are
there now"). **Reads: after the run** origrun exports, as the game left them, `HKCU\Software\PopCap`,
`HKLM\SOFTWARE\PopCap` (32-bit view) and every other key the presets named, each to
`logs/registry-after-<n>.reg`, and logs which file holds which key. The exports name keys as the 32-bit view shows
them (`HKEY_LOCAL_MACHINE\SOFTWARE\PopCap`, without `WOW6432Node`). reg's own output is in `logs/reg.txt`.
**Tested on 2026-11-01** with a UTF-16 file holding one VirtualStore key and one HKCU key under `PopCap`: both were
imported (one moved to HKLM), both were there before the launch, and both came back in the two exports.

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

The held capture on the host used the window-DC blit, not `PrintWindow` (which never returns while the window's
thread is suspended). Like every capture it is now taken only inside Windows Sandbox (guarantee 4); on the host it is
refused before the game is held. Input under `at` is still held on the host.

**Inside the sandbox (`printwindow-bracketed (sandbox)`, 2026-11-01).** The window DC reads nothing there, so an
update-timed shot (`at <N> shot`, `burst ... update`) works as follows.

1. Once the counter reads `N`, the game is held and the counter is read.
2. `PrintWindow` is posted to a helper thread. The helper is started once per run on the desktop of the thread that
   captures (a new thread starts on the process's first desktop, where `PrintWindow` returns TRUE with a black
   window), at raised priority.
3. Once the helper signals that it is entering `PrintWindow`, a 0.5 ms margin is spun for it to send its message.
   `Sleep` is not used because it would round up to the 15.6 ms timer tick.
4. The game is released and answers the message at its next message check. The hold lasts about 1-2 ms.

If the helper does not start within 2 s of its first use, it is never used, and every update-timed shot of the run
is refused. If it does not take up a shot within 100 ms, or `PrintWindow` does not return within 3 s, that shot is
refused. The
helper frees that shot's bitmap whenever `PrintWindow` does return, and until then every further shot is refused and
a burst stops. A shot whose `PrintWindow` returns FALSE is tried again, up to three times. Nothing else reads the
window on this path, so a failure never turns into a window-DC read.

**Which frame the shot shows.** SexyAppBase checks messages after an update and its catch-up step, but draws that
update only in a later `Process` call. A hold that falls inside update `N`, its catch-up step or the message check
after them therefore gets the message answered before frame `N` is drawn. The shot then shows `N-1`, or an older frame
when the game skipped a draw to catch up.

With `--pending-draw` and `--update-state` (`mHasPendingDraw` and `mUpdateAppState`), origrun waits, without holding,
until no update waits to be drawn and none runs. That means the flag is clear and the step is "messages" (0) or
"done" (3); a draw and the catch-up updates run in "done", but with the flag still set. origrun then holds the game and
checks again, releasing and retrying if the game moved on in between. The last draw then followed update `N` (or found
nothing to redraw), and the next message check comes before the next update. So the shot shows update `N`, and the log
says "update N (drawn)". If no such moment comes within 500 ms, the shot is refused. The wait reads the flag and
the step in a loop that yields (`SwitchToThread`) after each read; `Sleep` would round up to the 15.6 ms timer tick.

A cleared flag still does not prove a present. DrawDirtyStuff also clears it when it skips the draw, either because
nothing was dirty or because of the draw throttle that runs while the game loads. The frame on screen is then an
older one: unchanged when nothing was dirty, but stale under the throttle. With `--draw-empty` (`mLastDrawWasEmpty`,
which the skip sets and a real draw clears), such a shot says "or-earlier". Without it the shot says
"drawn-unless-skipped".

The release itself is not checked. If the helper is pre-empted, after it signals and before `PrintWindow` sends its
message, for longer than the margin plus an update, the game answers a frame late. That shot shows `N+1` while it
says `N`. At the helper's priority this is rare. An exact release would wait until the helper is blocked in the
send, and is not needed yet.

Without the two options the shot shows `N` or an earlier update, and the log and `landed.tsv` say so ("or an earlier
one", "or-earlier"). A failed read of the counter, the flag or the step refuses the shot.

**What is logged.** The counter is read once more when `PrintWindow` returns. That read is logged but not checked,
because by then the game has caught up on the time it was held: one update on, or more under load. Every update-timed
shot is listed in `shots/landed.tsv` with its name, its update and how sure that update is ("drawn",
"drawn-unless-skipped", "or-earlier", or "held" for the host's held shots from before the ban). Each run starts the file afresh, so the port can take its own
shot at the update the original's landed on.

A game that draws straight to a locked screen, or whose window a wrapper such as cnc-ddraw presents through Direct3D,
leaves nothing in the window for PrintWindow. For it a held shot can read the frame from the game's memory instead
("surface-read (sandbox)", landed.tsv says "or-earlier (surface)"). All addresses are hex. `--surface-screen
bitsAt,pitchAt,w,h,bpp` names two globals holding the pixel pointer and the pitch (5-6-5 at 16 bpp, 8-8-8 at 32);
`--surface <addr>` names an Image-like header instead (pixel pointer at +0, width at +4, height at +8, pitch at +0x1c,
bpp at +0x44, then the masks, bit counts and shifts). A surface shot needs a gate, and is refused without one:
`--surface-eip lo,hi[,lo,hi...]` lists every function of the game that writes the locked screen (all it runs between
its lock and unlock), and the shot waits (the game running) until the held thread is in none of them and has no return
address into one anywhere in its stack (to the end of the stack's region, at most 64 KB; an unreadable page closes the gate); `--surface-gate <addr>` is an int that must read 0
(a draw list's count). The pixels and `--sample a,b` values are read under the hold, the game is let go, and then the
PNG is written. The header is refused unless the pitch is at most 16384 and no more than 64 bytes over a row.

A burst still waits for each frame's update. When a capture (`PrintWindow` and writing the PNG, about 35 ms for Bookworm
Adventures) takes longer than the burst's spacing, its frames land after their updates, and `landed.tsv` gives each
frame's real update.

**Rejected: reading the counter before and after an unheld `PrintWindow`.** The shot was kept only when both reads
agreed. Inside the sandbox `PrintWindow` takes about an update's time, so the reads agreed once in 1 to 20 tries, and
shots landed up to 238 updates after their update.

A hold begins only once the suspension has taken effect (`GetThreadContext` returns after it has), and
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
`Test3D` values before every run (logging each one the game left, as hex) and puts them back after it, with or without `--3d`: on rundesk's desktop the game
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

**Captures of a GPU-drawn window were not safe on the host (2026-11-01).** Twice on 2026-11-01 a capture of a game
drawing through Direct3D 11 (dgVoodoo2 in the copy) returned what was on the user's own screen, once through the
window-DC blit and once through PrintWindow itself, and Windows 11's own DirectDraw loads DXGI as well: Noah's Ark
and Ning Po, run with no wrapper in software DirectDraw, had `dxgi.dll` loaded by the time their window came up, and
Noah's Ark later `d3d11.dll` and `D3D10Warp.dll`, where a plain GDI test window loads none of them. origrun therefore
takes no shot on the host at all ("The host-capture ban"). Inside Windows Sandbox the game may load what it likes and
is captured. On the sandbox's own desktop the hidden-desktop limit above may not apply, so a 3D run there needs no
wrapper: `--sandbox --sandbox-vgpu --3d on`, untested so far.

**dgVoodoo2 runs the 3D mode on rundesk's desktop.** It implements DirectDraw and
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

On the host a run with `--dgvoodoo` takes no shot, like every host run; it serves runs that need no picture, such
as the 3D-mode check below. Inside the sandbox a copy with the wrapper is captured like any other.

**3D mode proven on Bookworm Adventures (2026-10-31).** With `--3d on` and dgVoodoo2, the game reports 3D on from
about 2 s after launch to the end of a one-minute battle and ends "RESULT: 3D on, as asked", so the wrapper keeps
presenting while rundesk's window counts as occluded.

Before the ban, the pilot's battle (`games/bookwormadv/tools/pilot`: the map, the entry transition, the battle
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

Also before the ban, Water Bugs and Cosmic Bugs, run with `--dgvoodoo` (no `--3d`; they have no 3D mode),
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
updated for it. The Capture column records what was taken on the host before 2026-11-01. Since then origrun takes no
shot on the host at all, and captures come only from runs inside Windows Sandbox (`--sandbox`), of which none has
run yet; the column will be redone from those (see "Windows Sandbox").

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
| bookwormadv | local install | in-game Updates dialog; in the sandbox the pilot's menus and battle (2026-11-01) | **in the sandbox** (2026-11-01), 806×629, client at (3,26); update-timed shots and bursts `printwindow-bracketed (sandbox)`, exact with `--pending-draw [0x8259e4]+0x45a --update-state [0x8259e4]+0x488 --draw-empty [0x8259e4]+0x459` | **off and on proven** (on with `--dgvoodoo`) | muted a session | its sound is not via DirectSound (BWA audio work); stage under a short `--stage-root` (long worktree path makes its launcher exit) |
| ningpo | local install | main window | **yes** (after init); **in the sandbox too** (2026-11-01) | none | none | windowed DirectDraw; `waitshot`'s flat-retry now captures real content once it has drawn |
| waterbugs | local install | window, then "Screen Setup Failed"; **title menu with `--dgvoodoo`** | window only; with `--dgvoodoo` the title, 3840×2160 stretched | none | none | full-screen DirectDraw: the headless desktop cannot open its 800×600 mode; dgVoodoo2 gives it a desktop-sized window |
| cosmicbugs | local install | window, then "Screen Setup Failed"; **title menu with `--dgvoodoo`** | window only; with `--dgvoodoo` the title, 3840×2160 stretched | none | none | same as Water Bugs |
| dynomite | Steam copy | **title and on** | **yes** | has one; on needs `--dgvoodoo` (untried) | none | fixed: `Steam.dll` is now auto-copied from the Steam library root into the copy |
| bejeweled | wrapped launcher | **title, prepared copy runs** | **yes** | has one; on needs `--dgvoodoo` (untried) | none | prepared as the private notes below say, run via an absolute `--exe` |
| bigmoney | wrapped launcher | prepares cleanly | — | has one; on needs `--dgvoodoo` (untried) | — | prepare as Bejeweled (not launched yet) |
| bookworm | wrapped Steam exe | prepares cleanly | — | has one; on needs `--dgvoodoo` (untried) | — | prepare as Bejeweled (not launched yet) |
| rocketmania | Steam copy | not run | — | has one; on needs `--dgvoodoo` (untried) | — | Steam copy: `Steam.dll` is auto-copied now, as for Dynomite |
| mahjongjapan | local install | **whole stages to the ending, in the sandbox** (2026-11-01) | **yes**, 806×629 in the sandbox | has one; off (`HardwareAcceleration=0` by `--reg`) | none | JollyBear engine, windowed DirectDraw (`FullScreen=0` by `--reg`). Posted clicks need a `move` 150 ms before. Its players live in `%LOCALAPPDATA%\JollyBear\Mahjong Escape Ancient Japan`, which inside is SYSTEM's, through the 32-bit view: stage them under `sandbox-files/C/Windows/System32/config/systemprofile/AppData/Local/...`, re-signed for `C:\run\game\` (a player's checksum covers the exe's folder). The game reads the cursor with `GetCursorPos` for its hover, which posted input does not move, and `hwmove` did not light a button either in session 0 |
| mahjongchina | local install | **whole stages and the ending, in the sandbox** (2026-11-01) | **yes**, with `games/mahjongchina/tools/ddrawcap` beside the copy: without it every sandbox shot is black (China's window class has a black background brush, which the sandbox paints over the software display's output) | has one; off (`HardwareAcceleration=0` by `--reg`) | none | As mahjongjapan, and `games/mahjongchina/tools/sandbox_run.sh` does it all (the copy, ddrawcap, the BASS proxy, the registry and the re-signed players). Its players live in `%LOCALAPPDATA%\JollyBear\Mahjong Escape` (no "Ancient China"), staged the same way. Never click the PopCap and SpinTop logos at the menu's bottom corners: they are links |
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

- **Windows Sandbox: two games so far.** The empty sandbox, windowed DirectDraw games (Ning Po; Bookworm Adventures with update-timed shots), the
  stop of a killed run's sandbox and registry presets are proven (see "Windows Sandbox"). Still to test, by the game
  sessions once this is merged: whether Water Bugs and Cosmic Bugs open their full-screen mode there; whether
  Bookworm Adventures reaches 3D with `--sandbox-vgpu` and no wrapper; whether a game that plays sound past the
  stand-in reaches the host's speakers; and a game whose own settings live under HKLM (VirtualStore on the host),
  read and written there as SYSTEM.
- **3D on the host needs dgVoodoo2**: legacy DirectDraw has no hardware on the hidden desktop, so an original with
  a 3D mode runs in 3D there only with `--dgvoodoo` (see "Hardware acceleration (3D mode)"). Proven on Bookworm
  Adventures only; each session adds its game's registry key and 3D flag to games.tsv. Host runs take no shot.
- **Registry presets only inside**: `--reg` imports a `.reg` file into a sandbox run (see "Saves and the
  registry"); a host run has no such option and writes only the `--3d` setting, which it puts back afterwards.

- **Windowed DirectDraw now captures** (Ning Po): a window is often flat only because it has not drawn its first
  frame, so `waitshot`/`capture_ready` re-grab while flat and get real content once it appears. **Full-screen
  DirectDraw still does not** (Water Bugs, Cosmic Bugs): on the headless private desktop the game cannot open its
  800×600 exclusive mode ("Screen Setup Failed") and never reaches its content. The least-invasive paths
  (PrintWindow, a window-DC blit) can't help because the game refuses to run without the screen, and
  Windows.Graphics.Capture needs a displayed window and DWM, which the private desktop has not. Reaching them
  needs the game forced into a windowed mode (toggle not yet found) or a **ddraw stand-in** that reports a
  display and copies the primary surface (the documented last resort; built like the dsound stand-in, not yet
  written). **Update 2026-11-01:** `--dgvoodoo` forces the window and both reach their title menus, in a
  desktop-sized window with the picture stretched (see "Hardware acceleration (3D mode)"). The sandbox's own desktop
  may let them open their mode unaided; untested.
- **DirectInput games** (TipTop): the `hw*` commands (`SetCursorPos` + `SendInput`) are **unconfirmed**. Windows
  documents both calls as working only from the input desktop, which the private desktop is not, so they may fail
  silently there. TipTop's trial screen changed right after an `hwclick`, but no control run (the same script
  without the click) has ruled out chance, or the user's own clicks reaching the game through DirectInput. Before
  relying on them: log `SendInput`'s and `SetCursorPos`'s results, make a control run, and never switch the driver
  thread to the input desktop, which would move the user's cursor and click on their screen. Since 2026-11-01 they
  run only inside Windows Sandbox, where the input desktop is the sandbox's own (`Default`, after origrun's
  `SwitchDesktop`); there they are untested too. The in-process
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
