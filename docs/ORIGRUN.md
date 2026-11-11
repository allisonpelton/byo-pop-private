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

The user allows origrun to hook, inside a copy: its time reads, to run it faster than real time ("Yes, hook them",
2026-11-02), and its timed waits ("Yes, waits too", 2026-11-03); its GetCursorPos, so that it returns origrun's scripted
mouse position ("Yes, hook it", 2026-11-03); its MessageBox and MessageBeep calls, so that a message box is answered
without appearing or sounding ("Yes, hook them", 2026-11-03); its GetKeyState, so that scripted Shift and Ctrl presses
register, and its GetLastInputInfo, so that it keeps drawing while the user is away (both "Yes, hook it", 2026-11-09);
and its registry calls, so that what a copy writes under HKLM\SOFTWARE\PopCap lands under HKCU and a restarted copy
finds it ("Yes hook registry calls", 2026-11-10; origrun has no such hook yet). Any other hook of a Windows function
inside a copy still needs the user's OK first.

How any copy's protection is handled is kept in the private notes under Per-game status, which a public
snapshot strips out (`docs/PUBLISHING.md`); this document otherwise names games only by title.

## Build

    tools/origrun/build.sh

Builds, 32-bit (the originals are 32-bit), with llvm-mingw's i686 target under `E:\retools`, into
`build/origrun-bin/` (git-ignored):

- `origrun.exe` — the host: copies the game, launches it under rundesk, drives it, captures, watches audio.
- `dsound.dll` — the DirectSound sound stand-in placed beside the copy (silence + a log, optionally WAVs). With it
  origrun writes `<exe>.manifest` beside the copy's exe, which makes the stand-in COM's DirectSound too (guarantee 3).

A run on the host needs `build/native/rundesk.exe` (`cmake --build build/native --target rundesk`). A `--sandbox`
run needs the Windows Sandbox feature and its `wsb` command (Windows 11 24H2 or later), and no rundesk. Start
`origrun.exe` itself directly, never under rundesk: it puts the game under rundesk on its own, and wrapped in rundesk it
fails with "cannot start the command" (Seven Seas, 2026-11-07).

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
- `--sound real` — no stand-in; the game uses the real audio device (turn the speakers off first). The audio watch
  still mutes each of its sessions as soon as it appears, so its sound reaches the speakers only for the moment before
  that (a `SOUND LEAK` line says so). It takes out the manifest an earlier run wrote beside the exe.
- `--timeout <sec>` — wall-clock cap (default 120, max 3600). A run past about 4 minutes goes in the background of the
  session that owns it, never in a subagent (CLAUDE.md, "Token economy"); it is still one game process.
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
- `--arg <text>` — added to the game's own command line after the exe, e.g. `--arg -autoplay` for a game's test
  switch. Repeatable (the values go in order, one space apart, at most 255 characters in all); letters, digits and
  `- = _ . ,` only, since the text goes in unquoted. Passed into the sandbox too.
- `--watch <expr>` — a value in the game's memory (read as 32 bits), logged at each change with the update counter:
  "watch 1: 48 -> 47 at update 2150", and "-" while it cannot be read (through a null pointer, say). Up to four, each
  polled every millisecond and logging at most 4000 changes (`--watch-max <n>` changes the cap, for a whole game's RNG
  seed). It measures how many updates something lasts in the
  original, such as a loading screen whose length follows the machine's speed (Bookworm Adventures' pilot watches its
  chapter load screen's timer and hands the count to the port). Measure with the update numbers, not by counting
  changes: after a held shot the game runs its catch-up updates back to back, faster than a poll, so a value can skip.
  The game's process is looked up again every 100 ms (and soon after a failed read), not at every read: on the
  sandbox's desktop a window enumeration costs about a millisecond.
- `--clock virtual` / `--clock real` — run the game on origrun's virtual clock, as fast as it can draw and update for
  update as in real time, or on the wall clock (the default). Needs the sound stand-in, so it is refused with
  `--sound real`. See "The virtual clock".
- `--cursor scripted` / `--cursor real` — the copy's `GetCursorPos` returns the last mouse position the script
  posted, or Windows' cursor (the default), for a game that reads the cursor after posted input (Bejeweled Twist's
  board). With or without `--clock virtual`; refused with `--sound real`. See "The scripted cursor".
- `--keys scripted` / `--keys real` — the copy's `GetKeyState` returns the key and mouse-button state the script's
  posted edges left, or Windows' (the default), for a game that asks whether Shift or Ctrl is down while it handles a
  key (Mahjong Escape). Refused with `--sound real`. See "The scripted keys".
- `--msgbox show` — show the copy's message boxes as before (each plays its sound, and the dialog watch answers it once
  it is on screen) instead of answering them in the stand-in with no window and no sound, which every run with the
  stand-in does by default (`--msgbox hook`). For a phase-two comparison that photographs a box; turn the speakers off
  for it. See "Message boxes and beeps (hooked)".
- `--coverage <list>` — write each function of the list (from `tools/coverage.py list`) that the copy reaches to
  `build/origrun/<run>/coverage.txt`, for the coverage check (`docs/COVERAGE.md`). On the host only, with the stand-in
  (refused with `--sound real` and `--sandbox`). See "Coverage (`--coverage`)".
- `--patches <file>` — a patch list to apply to the copy (see "Patching the copy"). Defaults to
  `tools/origrun/patches/<game>.txt` if present. `--no-patches` runs the copy unpatched.
- `--seed <N>` — a value substituted for the `{seed}` token in a patch, to pin the original's RNG so its boards
  and other random content match the port it is compared with. A seed that no applied patch carries stops the run
  (exit 2): with `--exe` and no `--patches`, or a list without `{seed}`, nothing would pin it, and the copy would
  draw its own random content (Alchemy's profile runs, 2026-11-09). With `--no-patches` it is only a warning.
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
so a dialog button is hit as well as a game that hit-tests in its own window. The client area is the game's own
size (640x480 for most), so take coordinates in it, never in a scaled screenshot's pixels. A title screen may take no
click until the game has loaded, and a click before that is lost without a sign: wait for the game's loaded state (an
update count by `at`, a `--watch` value or a screenshot) before the first click, never a guessed `wait`. Seven Seas'
unregistered copy first fills a delay bar and takes a click from about update 1000, the same on every run: 10 s in
real time, a fraction of a second on the virtual clock.

- `wait <ms>` — pause.
- `move <x> <y>` — move the mouse.
- `click <x> <y>` / `rclick <x> <y>` — left / right click.
- `clickready <x> <y> [timeout_ms]` — wait for the window to exist, then click (use it for the first click,
  since a game's window can be slow to appear while it tries to phone home).
- `key <vk>` — a virtual-key press (decimal or `0x..`).
- `text <chars...>` — type the rest of the line, all of it after the one space that follows `text`, spaces included
  (`text  ` types a space). Each character is a `WM_CHAR` alone; a game that reads a key as its key down takes `key`.
- `waitfor <expr> <op> <value> [<timeout_ms>]` — holds the script until the memory expression (the `--watch` grammar)
  reads so, `op` one of `==`, `!=`, `<`, `<=`, `>`, `>=`, the value decimal or `0x` hex; 60 s by default, logged either
  way. The game runs free meanwhile, on the virtual clock too: a gate on the game's own state for the first input,
  where an update count is a guess (an original's loading thread runs on the wall clock while its updates run on the
  virtual one; Typer Shark, 2026-11-09). A `mark` after it lets `at +N` count from there.
- `char <code>` — one `WM_CHAR` of that code (decimal or `0x..`), for a control character a script line cannot hold
  (Ctrl+Z, 0x1a, ends a script file, which is read in text mode).
- `down x y` / `up x y` (`rdown` / `rup` for the right button), `keydown <vk>` / `keyup <vk>`, `dclick x y` (`rdclick`
  for the right button) — one edge of a button or key, or a double click (down, up, `WM_LBUTTONDBLCLK`, up), posted now. They are the same as at an
  `at` line (below) and are for a game with no update counter (Mahjong Escape): a `wait` between the edges makes a held
  press, and moves in between make a drag. A held key changes no key state the game reads with `GetKeyState` or
  `GetAsyncKeyState` (Ctrl+C as `keydown 17`, `key 67`, `keyup 17` works only in a game that reads its messages),
  unless the run has `--keys scripted`, which answers `GetKeyState` ("The scripted keys").
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
  (`shot`, `click`/`rclick`/`key`/`text`/`move`/`focus`, `dclick`, `keydown`/`keyup` for one edge of a key (`kdown`/`kup` are the same; a game that polls its key state, or clears a key
  when it is let go, loses a `key`'s down and up in one update), `down`/`up` or `rdown`/`rup` for one edge of the left or
  right button, or `burst`), then resume it; `quit` is not among them (origrun logs that it cannot do it at an update), so a
  script that ends on its own puts a plain `quit` line after its last `at` line. A `move` while a `down` has had no `up`
  carries that button's flag (MK_LBUTTON, MK_RBUTTON), as Windows' moves do, so a drag is a `down`, one or more `move`s and
  an `up`, each at an update of its own. Lands on the exact update
  (the counter is polled every ~1 ms against ~10 ms updates), so the original and the port can be captured at
  the same game update rather than the same wall-clock time. A held game reads all the posted messages before its
  next update, so `at N click x y` gives it the down and the up in the same update, which some games lose; `at N
  click x y k` (or `rclick`) posts the move and the down at `N` and the up `k` updates later, at `N+k`. `at N key vk k` does
  the same for a key, with a `keydown` at `N` and a `keyup` at `N+k`. `at` does not
  take the `hw*` commands: in the sandbox the game runs in session 0, which desktop input does not reach anyway
  (`GetCursorPos` reads 0,0 whatever `SetCursorPos` did; for a game that reads the cursor, see "The scripted cursor").
- `at <N> peek <expr> [<expr>...]` — logs the ints that the expressions read (the `--watch` grammar: `[0xPTR]+0xOFF`, nested brackets) as one
  line, `peek u=<counter> <expr>=<value> ...` (signed; `-` where an expression cannot be read). On the virtual clock the game stands at
  the update the `at` waited for, so the values belong to exactly that update: a state log of the original, one line per update
  of a stretch with as many `at N peek` lines, to set beside a port's own state log (`--watch` polls and misses changes between
  two polls). It needs no window and changes nothing.
- `mark` — reads the update counter now; `at +<N> <command...>` then waits for the counter to reach N past that value.
  For a counter that does not start at 0, such as an event time on the virtual clock, which starts at the real time
  (Ning Po: a `wait` to the first input, the input, `mark`, then `at +N` lines at the port's updates from there).
- `msgbox <button> <text...>` — when a message box whose caption or text contains `<text>` appears, click the
  button labelled `<button>` (e.g. `msgbox No Trial expired`, `msgbox OK update`). Put these at the top of the
  script; they are registered before the game launches, so even a box that pops up early is answered. A box
  with no matching rule is answered by its own default button. The label match ignores the `&` accelerator and
  case. The same rules answer the boxes the stand-in hooks, which never appear ("Message boxes and beeps (hooked)").
  Never name a link/"Buy"/"More Games" button here (see the warning below).
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
   whose window origrun never opens (but with `--sandbox-connect`, below); a guard stops the sandbox at once if one appears
   (see "Windows Sandbox").
3. **Silent by default.** The `dsound.dll` stand-in beside the copy makes `IDirectSound`/`IDirectSoundBuffer`
   objects that accept every call and never open a device, so DirectSound and anything on top of it that loads it by
   name stays silent. So does a DirectSound made through COM (`CoCreateInstance` of `CLSID_DirectSound` or
   `CLSID_DirectSound8`, as the BASS of Alchemy, Pixelus and Rocket Mania makes it), since 2026-11-03: origrun writes a
   manifest beside the copy's exe (`<exe>.manifest`, marked as origrun's) that names the stand-in as the server of those
   two classes, and Windows' registration-free COM then makes them in the stand-in instead of loading the system's
   `dsound.dll` from the registry. Nothing is registered or hooked, and the manifest says nothing else. Windows keeps
   what it read of an exe's manifest until the exe changes (a copy that had run before went on loading the system's
   `dsound.dll` until its exe was touched), so origrun sets the exe's write time to now whenever it writes or takes out
   the manifest. The buffers of such a DirectSound keep their play cursor on the wall clock, as the real device did:
   on the virtual clock BASS has to mix its music as fast as that clock runs, and the game slowed with it (Alchemy's
   case ran at 18x instead of 960x). A manifest beside the exe that is not origrun's is left alone, and an
   exe with a manifest of its own ignores one beside it (none of the games so far has one); the log says so, and the
   session mute below still covers them. `--sound real` takes origrun's manifest out.

   Sound that goes past the stand-in is silenced by a session mute (since 2026-11-03): as the
   stand-in loads, a thread of its own mutes the process's default audio session (`GUID_NULL`, the one every stream
   without a session of its own joins) on every active render device, and the stand-in's entry points wait for that
   thread before they answer. A stream the game opens later, through the system's DirectSound, winmm's `waveOut` or
   `PlaySound`, or FMOD's own output, joins a session that is already muted, and its sound never reaches the speakers.
   Nothing in Windows is hooked for it. As a second check the host watches Windows Core Audio: it looks at each of the
   game's audio sessions on every render device as soon as Windows creates it (a notification wakes the watch) and every
   20 ms, mutes one that is not muted yet, and reads each session's peak meter, which Windows measures before the
   session's mute. Sound measured while a session was not muted reached the speakers; the run log then has a
   `WARNING: SOUND LEAK` line and its RESULT line says `SOUND LEAK`, with the times. Otherwise the RESULT line says that
   each of the game's sessions was muted from its start, and whether the game made sound in them at all (the first at
   which time, and the loudest peak). The watch also looks at the modules of each game process with a session every half
   second: a `WARNING` line says when one loaded the system's `dsound.dll`, and the run's audio lines end with whether
   any did. With the stand-in a run always shows its sessions, one per render device, muted
   and silent in a game whose sound all goes to the stand-in; "no audio session from the game was seen" now means the
   copy never loaded the stand-in and played nothing.

   A SexyApp game shows an audio session even when nothing goes past the stand-in: `DSoundManager::SetMasterVolume`
   sets the wave-out volume through the mixer API, which Windows maps to the process's own session volume, so the
   session appears (silent) when the game starts, sound or none. Not covered: a stream with a session GUID of its own, a
   render device that becomes active during the run, a `PlaySound` with `SND_SYSTEM` (the shared system-sounds session)
   and a game that never loads the stand-in (Mahjong Escape's packed `bass.dll`, say); the watch mutes the first two
   within milliseconds of their start and reports any sound they made as a leak. `--sound nodriver` stops BASS's music
   as well: the game's own `DirectSoundCreate` fails first, so BASS is never started and COM is never asked, and the
   game plays no sound at all.
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
   `--dgvoodoo` gives it a window instead (see "Hardware acceleration (3D mode)").
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
   nothing is injected into the game. The watch sees a box only once it is
   on screen, after its sound, so since 2026-11-03 the stand-in answers the copy's MessageBox calls itself, with no
   window and no sound, by the same rules, and silences MessageBeep ("Message boxes and beeps (hooked)"); the watch
   still answers every other dialog, and the boxes of a run with `--msgbox show` or `--sound real`.

Registry and save redirection is **not** implemented on the host (the user allowed the originals to use their real
registry and saves). A host run therefore writes the game's settings and trial state to the real registry and save
folders, which the user permitted. A `--sandbox` run keeps all of that inside the sandbox, where it vanishes with it.

## Windows Sandbox

### The host-capture ban

origrun takes no shot on the host, of any kind and of any game: three captures there returned the user's own screen
instead of the game, and Windows 11's own DirectDraw loads `dxgi.dll`, so no rule about GPU modules could keep the
DirectDraw originals safe either. A host run is therefore scripted input without shots, and every capture comes from
inside Windows Sandbox, where a capture can only ever see the sandbox's own display. A shot that shows anything but
the game's own window is deleted at once, unopened where possible, and reported to the coordinator.

### Setup

Windows Sandbox is an optional Windows feature ("Windows Sandbox" under Windows Features, or `Enable-WindowsOptionalFeature
-Online -FeatureName Containers-DisposableClientVM` from an elevated shell, then a restart). It needs Windows 11 Pro
or Enterprise with virtualization on. The feature itself brings only `WindowsSandbox.exe` and its client; the `wsb`
command comes with the Store package `MicrosoftWindows.WindowsSandbox`, which the built-in app downloads the first
time it is opened (Windows 11 24H2 and later); origrun itself
downloads nothing. origrun finds `wsb.exe` in System32 (through `Sysnative`, since origrun is 32-bit) or as the app
alias under the user's `WindowsApps` folder, where it is here, and fails cleanly when neither is there. The first `wsb`
call of a while starts the package's broker, `WindowsSandboxServer.exe`, in the user's session; it is no sandbox, it
ends by itself after a while, and origrun leaves it alone.

    origrun --game <name> --script <file.txt> --sandbox [--sandbox-vgpu] [--sandbox-connect] [options]

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

### Headless

origrun never uses the `.wsb` file's `LogonCommand` (it needs a user session), starts its run with `wsb exec -r
System`, which needs none, and runs `wsb connect`, which opens a window with a remote desktop session, only with
`--sandbox-connect` (see "Hardware acceleration"), an option for runs the user has allowed onto their desktop. `wsb
start` alone shows no window or dialog on the user's desktop, and the sandbox then has no user session and no console
session at all: only session 0 and a listening remote-desktop session with no user.

A guard thread looks, every 100 ms from before `wsb start` to after the stop, for a visible window on the desktop
the user sees of a Windows Sandbox process (the three names above), of the broker `WindowsSandboxServer.exe` or of
`vmconnect.exe`, matched by exact name, never by a part of one (another product's "sandbox", Sandboxie say, is not
Windows Sandbox). When it finds one, it stops this run's sandbox with `wsb stop --id`, which closes the sandbox's
windows; it never ends a process itself, so a window that is not this sandbox's costs only this run. The run logs
"STOP: a sandbox window was on the user's desktop" and exits with code 4. It has never fired. The one exception is
`--sandbox-connect`: once origrun has run `wsb connect`, the window of that process, or of a process it started, is
allowed (logged once); every other window, before the connect or of any other process, still stops the run. Every run ends with
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

### What the sandbox offers a game

The display is 1024x768 at 32 bpp, with 18 modes, of which none is 16 bpp (800x600 only at 32 bpp). Without
`--sandbox-vgpu` DirectDraw has no hardware layer (no video memory, no 3D), and it loads `dxgi.dll` there too. With
the input desktop as session 0 has it (`Winlogon`), every `SetDisplayMode` fails (`E_NOTIMPL`); after `SwitchDesktop`
to `Default`, exclusive full-screen 800x600x32 works with a flipping primary surface, while 16 bpp modes stay
unsupported. A game that asks for 16 bpp full screen may still get it through Windows' compatibility shims, which
apply inside the sandbox as on any Windows.

### The configuration, and why

`Networking` is disabled, so a game cannot reach the internet or the user's network (no "More Games" page, no
update check, no Steam). `ClipboardRedirection`, `PrinterRedirection`, `AudioInput` and `VideoInput` are disabled,
so nothing crosses through the clipboard, a printer, the microphone or a camera. `ProtectedClient` is enabled, which
runs the sandbox's remote-desktop client in an AppContainer, in case a client is ever started. `vGPU` is disabled by
default, so the sandbox renders in software (WARP) and shares nothing of the host's GPU driver; `--sandbox-vgpu`
enables it for a 3D run (Microsoft notes that a shared GPU widens what the sandbox can reach of the host). The user
allowed it for Bookworm Adventures' 3D comparison (2026-11-01) and for Bejeweled Twist's 3D mode (2026-11-02); another
game's 3D runs ask the user first.

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

### Load times

On a quiet machine `wsb start` returns in about 2 s and origrun runs inside about 3 s after the start began. The game's
window comes within a second of its launch, or 3 s on a copy's first run, when its files are read for the first time.
The queue on the lock is the other real cost: a run waits there for minutes while other sessions' sandboxes run, and
its log says so. `--timeout` counts from the game's launch, so neither the queue nor the boot shortens a run.

A sandbox that stays up between runs and stops itself when idle was allowed (the user, 2026-11-07: "Yes, if it
helps"), and it is not built.

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

On the host nothing vanishes: a save kept outside the game's folder is the user's own. Bejeweled Deluxe keeps its saved
game in `C:\ProgramData\Steam\Bejeweled\userdata`, so a host run that quits in the middle of a game leaves one there, and
the next host run meets it at Play (found 2026-11-03: the new board stayed paused and the script's moves did nothing).
origrun does not touch that file: a session that runs Bejeweled on the host deletes `savegame.dat` there before and after
its runs itself (the virtual clock's test scripts of 2026-11-03 did).

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

## Recording sound (`--sound wav`)

With `--sound wav` the stand-in advances a fake play cursor in real time (so a streaming source such as BASS or
FMOD keeps writing), and writes to `snd/`:

- a static sound as `play_<seq>_buf<id>.wav` each time it is played, in play order, at the buffer's declared
  format — so a session can measure each sound's RMS level and when it starts;
- a looping/streaming buffer (mixed music) as one growing `stream_buf<id>.wav` (capped at ~90 s).

The sound log (`logs/sound.log`) lists every buffer create, data write (size + hash + format), play
(once/looping, volume, pan, frequency), volume/pan/frequency change and stop, each with a timestamp — enough to
line up "which sound plays when" against the port without the WAVs. This catches sounds a game plays through
DirectSound; a game that mixes elsewhere needs its own path handled.

Since 2026-11-03 the stand-in's buffers answer as DirectSound's do in every run, `stub` and `wav` alike, not only on the
virtual clock. `GetStatus` says a buffer plays from `Play` until `Stop`, or until a buffer played once has played its
length (before, it said every buffer was stopped); a finished one-shot's cursor reads 0; `Play` after `Stop` goes on
from where it stopped, and `Play` on a buffer that plays does not restart it; `SetCurrentPosition` and `SetFrequency`
move the cursor on from the new place or rate. The cursor runs on the wall clock, or on the virtual clock when it runs.
At the game's exit the log says who asked ("GetStatus: 35254 calls from popcapgame1.exe, 34835 of them answered
playing").

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
`ReadProcessMemory` on the game it launched (a documented debugging API; nothing is injected; on the virtual clock
the sound stand-in reads them inside the game instead, see "The virtual clock"), at an address a game session gives
in the per-game config:

    counter=[0x4d125c]+0x368      # the int 0x368 bytes past the pointer stored at 0x4d125c
    flips=[0x4d125c]+0x364        # optional: a counter the game bumps once per presented frame
    update_ms=10                  # one update's length, for a clip's interval_ms

`[0xPTR]+0xOFF` reads the pointer at `PTR` then the int `OFF` bytes into it — for a SexyAppBase game, the app
pointer's global and `mUpdateCount` (each session supplies its own; the offset is in its `re/<game>/`). A plain
`0xADDR` reads the int at that address. Brackets nest, so `[[0x8259e4]+0x36c]+0x38` follows two pointers (the
3D flag below), and an offset may also be subtracted. Then `at <N> <command>` waits for the counter, suspends the window
thread, runs the command and resumes; `burst <name> <count> update <k>` captures every `k` updates the same way.

Each game's offset is its own, even between SexyAppBase games of one era: take it from the game's own update function,
never from another game's config: an offset that holds something else (a frame time that stays at 10, say) makes
every `at` wait for an update that never comes. A wait that times out on a counter that never moved says so in its
log line.

**Precision (measured on Noah's Ark):** `at` lands on the exact update — `at 1500/1700/1900` each fired with the
counter reading exactly that value — because the ~1 ms poll is far finer than the ~10 ms update. The hold is kept
as short as it can be, because the game's clock runs on while it is held, and on resume the game catches up on
updates it never draws. Input under `at` is posted with no gaps between its messages (the queue keeps their order,
and the game reads every pending message before its next update), so a held click holds the game for about
0.04 ms; a held shot resumes the game as soon as the pixels are grabbed, before the PNG is made and written (about
3-5 ms held). The log gives each hold ("at click: the game was held 0.04 ms"; per burst, the average and longest
hold and how many frames were held past their update).

A held capture, like every capture, is taken only inside Windows Sandbox (guarantee 4); on the host it is refused
before the game is held. Input under `at` is still held on the host.

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
bitsAt,pitchAt,w,h,bpp` names two globals holding the pixel pointer and the pitch (5-6-5 at 16 bpp, 5-5-5 at 15, 8-8-8 at 32: Cosmic Bugs gets 5-5-5 in Windows Sandbox);
`--surface <addr>` names an Image-like header instead (pixel pointer at +0, width at +4, height at +8, pitch at +0x1c,
bpp at +0x44, then the masks, bit counts and shifts). With `--surface-screen-base ptrAt`, bitsAt and pitchAt are offsets into the object whose pointer the global at ptrAt holds, read at each shot (Ning Po's display is on the heap and its static app holds the pointer). A surface shot needs a gate, and is refused without one:
`--surface-eip lo,hi[,lo,hi...]` lists every function of the game that writes the locked screen (all it runs between
its lock and unlock), and the shot waits (the game running) until the held thread is in none of them and has no return
address into one anywhere in its stack (to the end of the stack's region, at most 64 KB; an unreadable page closes the gate); `--surface-gate <addr>` is an int that must read 0
(a draw list's count). The pixels and `--sample a,b` values are read under the hold, the game is let go, and then the
PNG is written. The header is refused unless the pitch is at most 16384 and no more than 64 bytes over a row.

**Input in the sandbox.** The sandbox has no real cursor: `GetCursorPos` reads 0,0 whatever `SetCursorPos` or hardware
input did, so a game that polls the cursor sees nothing. `at <counter> move x y` posts the window messages
(`WM_MOUSEMOVE`), and `at <counter> down x y` / `at <counter> up x y` post `WM_LBUTTONDOWN` / `WM_LBUTTONUP`; a game
that reads the cursor with `GetCursorPos` needs `--cursor scripted` (see "The scripted cursor") or a private patch that
takes the position from `WM_MOUSEMOVE` (Water Bugs has one, `mouse` lines in its patch list). Send the down and the up
in separate updates (40 or more apart; `at N click x y k` does it in one line): a down and up read in one update are
lost. `--input-eip lo,hi[,lo,hi...]` (config key `inputeip`) holds an input command until the
game thread is outside the listed ranges (Water Bugs: 41f467,41f511, between the message pump and the end of the update
loop), and the log line `at <cmd>: input at counter N (k holds)` gives the update that read it, which is the number to
replay in the port. The game's own `SetCursorPos` calls have no effect there, so a port run replaying them needs its
cursor warps off (Water Bugs's runner: `WB_NOWARP=1`).

A burst still waits for each frame's update. When a capture (`PrintWindow` and writing the PNG, about 35 ms for Bookworm
Adventures) takes longer than the burst's spacing, its frames land after their updates, and `landed.tsv` gives each
frame's real update.

A hold begins only once the suspension has taken effect (`GetThreadContext` returns after it has), and
if origrun crashes while it holds the game, it resumes the game first. A
SexyAppBase game counts update `N` at its start and draws after it, so a shot held as soon as the counter reads `N`
races that draw and mostly gets update `N-1`'s frame. Without `flips`, a held shot therefore waits for update `N+1`
to begin, when update `N`'s frame is on screen; with `flips` set it instead waits for the next present (capped so a
static screen, which presents nothing new, does not stall). SexyAppBase's catch-up can still double or skip an
update under load; a per-game patch could force one update per frame if a game ever needs it (not needed so far).

## The virtual clock (`--clock virtual`)

With `--clock virtual` an original runs as fast as it can draw, and update for update as it would in real time: a
minute of Bejeweled's play takes four or five seconds. The user allowed the hook on 2026-11-02 ("Yes, hook them"; see
the top of this document), for the time functions only.

**What it does.** The clock lives in the sound stand-in (`tools/origrun/vclock.c`, built into the `dsound.dll` origrun
places beside the copy), so it is in the game's process from the moment the game loads DirectSound, and it installs
itself there when origrun's environment says `ORIGRUN_CLOCK=virtual`. It pins the stand-in first, so a game, BASS or
FMOD that unloads `dsound.dll` leaves it mapped. It patches, in memory only, the import tables of the game's exe and of
the other DLLs in the copy's folder (not origrun's own stand-ins and wrappers, nor `Steam.dll`), and looks at the list
of loaded DLLs again about every 16 ms of wall time while the game waits, to patch one loaded later. The game's reads
of the time (`timeGetTime`, `GetTickCount` and `GetTickCount64`, `QueryPerformanceCounter` and
`QueryPerformanceFrequency`, `GetSystemTimeAsFileTime`, `GetSystemTimePreciseAsFileTime`, `GetSystemTime`,
`GetLocalTime`, `timeGetSystemTime`) then answer from a clock that starts at the real time of the install and moves
only where the game waits.

The clock assumes that one thread runs the game, its main thread: the one that started at the exe's entry point (if
none did, the oldest thread, and the clock's log says so). When that thread calls `Sleep`, `SleepEx`, or
`MsgWaitForMultipleObjects(Ex)` on no handles, with a timeout, the stand-in first asks Windows with a zero timeout. If a
message (or an APC) is ready, that is the answer and the clock stays; otherwise the call returns at once and the clock
moves on by the timeout. A timed wait of the main thread on handles (`WaitForSingleObject(Ex)`,
`WaitForMultipleObjects(Ex)`, a `MsgWaitForMultipleObjects(Ex)` with handles) that is not ready at once goes to Windows
for real, since another thread (a loader, a library's, the C runtime's) may signal it any moment and a timeout handed
back early would change what the game does next; the clock moves on by the real time it took, or by its timeout when it
ran out. Waits without a timeout or with a zero timeout, and every wait of the game's other threads, go to Windows as
before, and those threads read the same virtual clock. Only a timed wait of the main thread that was not ready at once
resets a count of its clock reads: a main thread that reads the clock 200 times without one (a loop polling the clock,
or one whose waits have no timeout or a zero timeout) moves it on by 10 µs at each further read, so a loop polling the
clock still ends. The speed comes only from not waiting: no thread is added to the game, and the game runs on its own
threads.

**The guard.** A thread the game starts runs on the wall clock while the main thread races ahead: on the virtual clock
Bejeweled's title screen was still loading at the update its Play button was clicked. So while a thread the exe
started (its start address lies in the exe; threads of BASS, FMOD, DirectX and Windows are not counted) uses more than
a quarter of a core, the main thread's waits that would be skipped take their real time, and still move the clock by
their timeouts. The clock's log says when this starts and stops ("the exe's other threads work"). It is a heuristic: a
loader that waits on the disk without using the processor is not seen. `ORIGRUN_CLOCK_GUARD=0` in origrun's
environment turns it off. Where the guard misses a loader, gate the script's first input on the game's own loaded flag
instead: `--counter` may name any value that changes once loading is done, and `at N` waits for it. Bejeweled's coverage
runs (2026-11-09) clicked Play before its title had loaded until they gave `--counter 0x53ba60` (the app's loaded flag
as the third byte of that word, so it reads 16777216 once set) and began the script with `at 16777216`.

**In step with origrun.** Through a block both share, origrun holds the game's main thread at a wait: `at N` arms a
barrier at which the main thread stops as it enters its first wait once the counter has reached `N`, and while held it
still answers sent messages. `at` and shots then suspend the window's thread on top of that hold, as in a real-time run,
for the moment they post their input or read the picture, and the game stays held at its wait afterwards. The game
starts held: once its window is up origrun steps it wait by wait, so a script starts at the same point in every run.
Between two script commands the game stays held, so `wait <ms>` is game time from the hold, the gaps inside a click cost
nothing, and the update an `at` lands on does not depend on the machine. The update counter and the `--watch` values
are read inside the game, at each of its waits and at each read of the clock once the counter has moved, so no update
is missed however fast the game runs; origrun logs them in the usual lines. The stand-in's play cursor and `GetStatus`
follow the virtual clock (see "Recording sound").

`clickready`, `waitshot`, `record`, a `burst` timed in milliseconds, `quit`, the end of the script and a run with no
script let the game run free, and on the virtual clock running free means uncapped: the game's milliseconds pass at
whatever speed it can draw (ten to over a hundred times real time, the most on a still title
screen). So a `record` or a timed `burst` covers far more game time than its milliseconds say (a 20-second `record` can
see several minutes of play), and a `waitshot` may find the game well past where a real-time run would be. For shots
on exact updates use `at N shot` and bursts by update.

**When holds go off.** If a hold lasts 60 s with no release from origrun, origrun ends while the game is held, or a
hold's wait fails, the game holds no more for the rest of the run and says so in the block (`barrierOff`, with the
update and the reason). origrun logs it as soon as it sees it ("WARNING: pid ... holds no more from update N"), stops
waiting for holds (from then on `at` polls the update counter and `wait` sleeps on the wall clock, as on the real
clock), and ends its summary with either "HOLDS WENT OFF at update N" or "holds stayed on to the end": a
script's `at`, `wait` and update bursts after that point did not land on exact updates. The caps run on the wall clock
(`GetTickCount64`). A first hold that origrun has not taken up within 5 s lets the game run free until origrun arms a
barrier, without switching holds off.

**Use.** Add `--clock virtual` (inside the sandbox the run file carries `clock=virtual`). origrun's log ends with a
summary ("virtual clock (pid ...): 77.5 s of game time in 3.7 s of wall time to its last wait ..."), and the run's
`logs/clock.log` lists the imports it patched, those it does not cover ("also imports what the clock does not
cover"), the guard's switches, and at the game's exit, for each call site, the waits skipped and the waits on handles
that went to Windows. origrun follows the newest of the copy's processes that has a clock, and switches to a newer one
with a clock when it appears (a game that starts itself again); every other one runs free.

**One difference in the watch lines.** A value the game changes while it handles input between updates `N` and `N+1`
is logged at `N+1` on the virtual clock. A real-time run logs a quick handler's change at `N+1` too (Bejeweled's
swaps), but a slow one at `N`, as its 1 ms poll catches the value before the next update (Bejeweled's Play, which
builds the board: 800 in real time, 801 on the virtual clock). The real-time poll also sees values in the middle of an
update (Twist's level going up and back within one update), which the virtual clock does not; compare the last value
of each update.

**What it cannot cover:**
- `SetTimer` and its `WM_TIMER` stay on the wall clock. Every SexyAppBase original imports it but TipTop, Cosmic Bugs
  and Water Bugs (Bejeweled's mouse-in check and Twist's run from it). `timeSetEvent`, waitable timers, timer-queue
  timers, `SignalObjectAndWait` and `GetMessageTime` are not hooked either; a static scan of the 20 originals' imports
  finds none of these but `SetTimer`, and the clock's log names any it meets.
- A game that paces its frames by anything but a timed wait of its main thread gets no speed, and its clock crawls:
  by `WM_TIMER`, by `GetMessage` or another wait with no timeout, or by a vertical-sync or flip wait inside DirectDraw or
  Direct3D. Its main thread then reads the clock with no skipped wait in between, so after 200 reads each read moves
  the clock by 10 µs and the game's time runs far slower than the wall clock. The games tried draw windowed without
  vertical sync.
- A game that paces its frames by a timed wait on a handle (an event another thread signals, or a timeout on one
  nobody signals) runs at real speed there, since such waits take their real time.
- One main thread: only its waits move the clock and only it can be held. A game whose updates run on another thread
  gets no speed. If the main thread has not waited 5 s after the install while another thread calls the time functions,
  the clock's log says so.
- Imports bound late or by another name are not patched: delay-loaded imports, and imports through the `api-ms-win-*`
  sets.
- The threads of BASS and FMOD keep the wall clock (they find their functions at run time), and so do a game's
  music streams: the stand-in's play cursor moves with the virtual clock, so a stream is fed as fast as the game runs,
  but a game that times something by its music's position as the library reports it would drift. A DirectSound made
  through COM (BASS's, in Alchemy, Pixelus and Rocket Mania) keeps its buffers on the wall clock, as the real device
  it played on before 2026-11-03 did (guarantee 3).
- The clock starts when the game loads `dsound.dll`: most originals load it at run time (SexyAppBase's sound manager, or
  BASS), Seven Seas, Noah's Ark, Mummy Maze, Atomica, TipTop and NingPo import it. Time read before that is real, and a
  game that never loads DirectSound through the stand-in (Mahjong Escape's packed `bass.dll`, Bonnie's FMOD 3 in the
  sandbox) never gets the clock. With `--sound real` there is no stand-in, so the option is refused.
- Loading: a loader thread's time follows the guard (real time while it works), so its screen lasts about as long as in
  real time on this machine. A load done on the main thread takes no game time at all on the virtual clock, where in
  real time the game catches up on the updates the load took afterwards.
- At the end, a game's shutdown loops (Bejeweled's `Sleep(10)` while its other threads finish) run the clock ahead, so
  the game time at exit varies from run to run (59.7 to 96.4 s for the same minute of Bejeweled); play is not affected.
- Shots on the virtual clock, and a run with no script (both only inside the sandbox), are untested.

## The scripted cursor (`--cursor scripted`)

Posted input moves no cursor. A game that asks `GetCursorPos` where the mouse is finds Windows' cursor: on rundesk's
desktop the call fails and leaves the point as it was, and in the sandbox it reads 0,0, which the game turns into the
window's offset from the screen's corner. Bejeweled Twist's board reads it while it draws and puts its twist selector
there, so posted moves and clicks never reach the board, though the menus and dialogs take posted clicks; Mahjong
Escape: Ancient Japan reads it for its hover.

With `--cursor scripted` (`cursor=scripted` in the sandbox's run file) the sound stand-in points the copy's
`GetCursorPos` imports at its own function (`tools/origrun/cursor.c`; the user's OK of 2026-11-03 above, and nothing
else is hooked). Before it starts the game origrun makes a small shared block (`cursor.h`, `Local\origrun-cursor-<id>`),
and each command that posts a mouse position writes the point there first: `move`, `click`, `rclick`, `clickready`,
`at N` with `move`, `click`, `rclick`, `down`, `up`, `rdown` or `rup` (a bare `up` keeps the last point), and
`hwmove`/`hwclick`/`hwrclick`, which move the real cursor too. The point is kept in screen coordinates, the client
point through the game window's `ClientToScreen` when the command runs, so the game's `ScreenToClient` gives back the
script's point. Until the script's first mouse command the call goes to Windows as before. It works on the wall clock
and on the virtual clock, with the clock's import patching (the stand-in pinned, execute rights kept, the exe and the
DLLs of its folder but origrun's own and Steam.dll); DLLs the copy loads later are patched at the clock's waits or,
without the clock, at the next `GetCursorPos` call. The stand-in logs to `logs/cursor.log` (`logs/clock.log` with the
clock), and origrun's log ends with the number of reads the scripted point answered.

Use it for a game whose board, hover or drag reads the cursor; a run without it is unchanged. A game that reads the
cursor only while it has the focus (Twist's board) needs a `focus regain` first.

**Limits:**
- Only `GetCursorPos`. What a game asks besides gets Windows' answer: `WindowFromPoint` at the scripted point finds
  whatever window lies there on rundesk's shared desktop, and `GetCursorInfo`, `GetMessagePos`, `GetAsyncKeyState` and
  `GetPhysicalCursorPos` are not hooked (hooking any of them needs the user's OK first). Twist's exe imports none of the
  last four.
- The hook lives in the stand-in: a game that never loads `dsound.dll` from its folder never gets it (Mahjong Escape's
  packed `bass.dll`, as for the clock), and `--sound real` refuses it.
- Imports bound late (`GetProcAddress`, delay-loaded imports, `api-ms-win-*` sets) are not patched.
- The point is converted when it is posted: a window moved later keeps the old screen point.
- One point for the run: every process of the copy reads the last one posted, a process that starts later too, as a
  real mouse stays where it was.

## The scripted keys (`--keys scripted`)

A posted key or click changes no key state: Windows keeps that per thread from real input, so a game that asks
`GetKeyState` whether Shift or Ctrl is down while it handles a posted key finds them up, and Shift+Tab, a selection
made with Shift and the arrows, or Ctrl+Insert reach it as plain Tab, arrows and Insert. Mahjong Escape reads both that
way (0x42c660, as it reads the mouse buttons). `--keys scripted` points the copy's `GetKeyState` imports at the
stand-in's own function (`tools/origrun/cursor.c`; the user's OK of 2026-11-09 above, and nothing else is hooked). It
shares the scripted cursor's block (`cursor.h`): before each key or button edge it posts (`keydown`, `keyup`, `key`,
`down`, `up`, `click`, `rclick`, `dclick`, `rdclick`, at lines and plain ones alike) origrun writes the key's new state
there, and from the first one on `GetKeyState` answers from it: the high bit while the key is down, and the low bit
flipped by each press, as Windows has them. Shift, Ctrl and Alt read as down while either side of them is. Until the
first edge the call goes to Windows. The run's log ends with a line of how many edges were posted and how many calls
the block answered, and the stand-in's log (`logs/cursor.log`, or `logs/clock.log` with the clock) has each process's
count.

The state is what origrun last posted, not what the game has read so far: a game that handles the messages later reads
the state as of the last edge. So a modifier is pressed on an update (or after a `wait`) before the key it modifies and
let go on one after, as a real player's are: `at 500 keydown 16`, `at 501 key 9`, `at 502 keyup 16` is a Shift+Tab, and
`keydown 16`, `key 9`, `keyup 16` on three plain lines in one go is a plain Tab.

**Limits:**
- Only `GetKeyState`. `GetAsyncKeyState` and `GetKeyboardState` are not hooked (hooking either needs the user's OK
  first), and `hwkey` and `hwclick` go through Windows' own input, not the block.
- The hook lives in the stand-in, as the cursor's does: a copy that never loads it never gets it (a coverage run loads
  it, since the exe then imports it), and imports bound late are not patched.

## The last-input hook (always on)

A SexyAppFramework original asks Windows once a second whether a screensaver is set and how long its timeout is
(`SystemParametersInfo`), and when the machine's last keyboard or mouse input was (`GetLastInputInfo`, which counts input
anywhere on the machine). Once that input is older than the timeout it sets a flag, and its `DrawDirtyStuff` then draws
nothing until input comes again. rundesk's private desktop would make no difference. No run has yet been seen to set
the flag. The hook is a
precaution: without it, a run made while the user is away from the machine (overnight runs included), or one on the
virtual clock past ten virtual minutes (the copy's GetTickCount starts at Windows' and runs ahead of it), could draw a
frame now and then instead of on every update, with stale screenshots and low CPU and draw counts. Fifteen originals' exes import
`GetLastInputInfo` (Alchemy, AstroPop, Banana Bugs, Bejeweled 1, 2, 3 and Twist, Big Money, Bookworm Adventures, Hammer
Heads, Heavy Weapon, Typer Shark, Venice, Zuma's Revenge); any other game that reads it gets the same answer.

Since 2026-11-09 the stand-in answers every copy's `GetLastInputInfo` itself (`tools/origrun/vclock.c`,
`H_GetLastInputInfo`; the user's OK above), in every run that loads it, with no option: the last input is always now, on
the copy's own clock (the virtual one when it is on, read without moving it). The framework finds the function with
`GetProcAddress` (Windows 95's user32 has none), which no import patch reaches, so the stand-in puts a jump to its own
function at the start of user32's `GetLastInputInfo`, in the copy's process only; its import is patched as well. A call
with no structure or a wrong `cbSize` fails with `ERROR_INVALID_PARAMETER`. The stand-in's log has each process's count
of calls answered. A copy that
never loads the stand-in (`--sound real`, or a game that loads no DirectSound from its folder) is not hooked, so its
draws could stop while the machine is idle: run those while the user is at the machine.

## Message boxes and beeps (hooked)

A message box plays its sound as it appears (the error chime or question ding of its icon), and the dialog watch
(guarantee 6) sees a box only once it is on screen. So in every run that has the sound stand-in (`--sound stub`,
`nodriver` and `wav`), on the host and in the sandbox,
the stand-in points the copy's imports of `MessageBoxA`/`W`, `MessageBoxExA`/`W`, `MessageBoxIndirectA`/`W`,
`MessageBoxTimeoutA`/`W` and `MessageBeep` at its own functions (`tools/origrun/msgbox.c`; the user's OK of 2026-11-03
above, and nothing else is hooked). It uses the clock's import patching, as the scripted cursor does: the stand-in
pinned, the exe and the DLLs of its folder but origrun's own and Steam.dll, and DLLs the copy loads later patched at the
clock's waits or at the next hooked call.

A hooked box makes no window, so it plays no sound and takes no focus. The call returns the button the dialog watch
would have clicked: the button of the first `msgbox` rule whose text the caption or the text contains and whose button
the box has (labels compared without case; never Help, which MessageBox never returns), else the box's default
button, where `MB_DEFBUTTON1` to `4` pick the nth button of its type's set (the first when the set is shorter) and the
first is the default. It returns that button's ID: `IDOK`, `IDCANCEL`, `IDYES`, `IDNO` and the others. A style Windows
refuses (a type above `MB_CANCELTRYCONTINUE`) returns 0 with no box, as MessageBox does; a string resource in
`MessageBoxIndirect` is loaded from its `hInstance`. `MessageBeep` returns TRUE with no sound.

Before it starts the game origrun makes a small shared block (`msgbox.h`, `Local\origrun-msgbox-<id>`) with the
script's `msgbox` rules in it (they are now read before the launch), and the stand-in hands each box and beep back
through it. origrun's dialog thread writes them to its log in the dialog watch's own words, marked `[hooked]`, so a
script that greps those lines still finds them (a line break in the text is written `\n`):

    dialog appeared: 'Error' (pid 31164) [hooked MessageBoxA: no window]
    DIALOG caption='Error' text='DirectSoundCreate failed with error '120'.  Sound will be unavailable.' buttons=[OK] [hooked]
      answered with 'OK' (default button) [hooked: returned IDOK]
    MessageBeep(MB_ICONHAND) (pid 35952) [hooked: no sound]

The lines come within 250 ms of the call, or after the game's last line when it exits first, and the run ends with the
count of boxes and beeps. The stand-in's own log, `logs/msgbox.log` (`logs/cursor.log` with the scripted cursor,
`logs/clock.log` with the clock), lists the import entries it patched and each box with its style.

`--msgbox show` (`msgbox=show` in the sandbox's run file) leaves the boxes as they were: shown, with their sound, and
answered by the dialog watch once they are on screen. A phase-two comparison needs it to photograph a box and to see
the focus go to the box and come back; turn the speakers off for it. A `--sound real` run has no stand-in and so no
hooks, and says so in its log: its boxes show and ring as before. The dialog watch stays on for every other dialog: a
game's own dialog-template windows (class `#32770` as well) are not message boxes and play no sound.

**Limits:**
- Calls bound late are not patched: `GetProcAddress` (which the C runtime's own error boxes use), delay-loaded imports
  and `api-ms-win-*` sets. Such a box is shown, rings and goes to the dialog watch.
- The hooks live in the stand-in: a game that never loads `dsound.dll` from its folder gets none (as with the scripted
  cursor), and a game that loads it late (with `LoadLibrary`) shows the boxes that come before it.
- Nothing else is hooked: `PlaySound` and the other sound functions still play (hooking any of them needs the user's
  OK first), and there is still no guard against URL or program launches.
- A hooked box takes no time, where a shown one stayed up at least 300 ms before the watch clicked it, and the game's
  windows get no messages from a box's modal loop. A game that asks again until it gets another answer (a Retry box)
  asks at full speed; a `msgbox` rule that picks the way out ends it.

## Coverage (`--coverage`)

`--coverage <list>` records which of the original's functions a run reaches, for the coverage check
(`docs/COVERAGE.md`; `tools/coverage.py` writes the list and reads the hits). The recorder is in the stand-in
(`coverage.c`). As it loads, the first byte of every listed function in the exe's executable sections becomes INT3, and
a vectored exception handler takes each such breakpoint the first time it runs: it appends the address to
`build/origrun/<run>/coverage.txt` at once (so a run that is killed keeps what it had), puts the byte back and lets the
function run from its first instruction. A function costs one exception at most and runs unchanged after its first
call. Nothing in Windows is hooked: the breakpoints are in the game's own code, so this needs no OK beyond the hooks
above. The list holds addresses as the exe's file has them; an exe that Windows moves (ASLR, which none of the originals
uses) has them moved too, and the hits are written back at the file's addresses.

The recorder sees only what runs after the stand-in loads, and a game that takes DirectSound through COM, BASS or
`LoadLibrary` loads it only when it first wants sound, after its startup code. So for a coverage run origrun gives the
copy's exe one more import, the stand-in's `DirectSoundCreate`, in a section of its own (`.orcov`): Windows then loads
the stand-in with the exe's other DLLs, before the game's first instruction. The exe keeps its unpatched bytes beside
it (`.origrun-unpatched`) and the next run puts them back, as with the patch list (`--patches`); an exe that imports
`dsound.dll` already is left alone. The new section header needs 40 free bytes after the section table; where they hold
something else (Mahjong Escape keeps its copyright text there), origrun moves the PE headers and the section table up
over the DOS stub, which the loader never reads, and fixes `e_lfanew`. The run log says what was done, and its last line gives the number of functions
reached and whether the game ended normally (the hits file's closing line).

Each line of the hits file is `<address> <order> <thread id>`, in the order the functions were first reached; lines
that start with `#` are the recorder's own (how many functions it watched, how many listed addresses lay outside the
exe's code and were left out, and the closing line). Limits: the exe's own functions only (not the game's DLLs, such as
`bass.dll`); a function is reached or not, with no count of calls and no lines or branches; code that reads its own bytes
(a checksum of the exe) would see the breakpoints; not inside Windows Sandbox yet. `tools/origrun/covtest.sh` (in
check.sh's origrun suite) runs it on `covtest.exe`, a program with no window and no sound, built for ASLR, once as built
and once with text after its section table (`tools/origrun/fillslack.py`).

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

**3D mode cannot run on rundesk's desktop without a wrapper.** Windows gives legacy DirectDraw hardware only to the
input desktop, which a hidden desktop never is: with `Is3D=1`, DDInterface::Init fails with "CreateSurface Windowed
DrawSurface: DDERR_NODIRECTDRAWHW", and the game runs in software and saves `Is3D=0`. Direct3D 9's HAL device fails
there too (D3DERR_NOTAVAILABLE); Direct3D 11 does get a hardware device, which dgVoodoo2 uses (below).

**No original has run in 3D inside the sandbox.** `wsb exec -r System` puts origrun and the game in session 0, which
has no display at all: with `--sandbox --sandbox-vgpu --3d on` and no wrapper the game's Direct3D test fails at
DirectDraw's GetDeviceIdentifier (DDERR_NOTFOUND) and it runs in software, and with `--dgvoodoo` it crashes at start in
dgVoodoo2's `DDraw.dll`, on WARP and with `--sandbox-vgpu` alike. `--sandbox-connect` (only for runs the user has
allowed onto their desktop) runs `wsb connect` after the start, so the sandbox's user logs on and origrun moves the
game into that session (session 1). Its display is the "Microsoft Remote Display Adapter", where Direct3D 7's
CreateDevice fails (E_OUTOFMEMORY); with dgVoodoo2 the game also falls back to software, and GDI captures of its
window come out flat. `ORIGRUN_DGVOODOO_CONF` names a dgVoodoo.conf to use in place of origrun's own, for trying
settings.

On the user's visible desktop an original runs in 3D with no wrapper (Bookworm Adventures, from a copy under the main
checkout's `build/`). A run and its shots there need the user's leave, given game by game: for Bookworm Adventures' 3D
comparison ("Yes, capture it", 2026-11-01), and for Bejeweled Twist's ("Sandbox, then desktop", 2026-11-02: first
`--sandbox --sandbox-vgpu --sandbox-connect`, whose sandbox window shows on the user's desktop, and only if the
original still falls back to 2D there, a run on the user's desktop). Such a run goes as Bookworm Adventures'
`tools/pilot/visrun.py` does it (`games/bookwormadv/docs/IDENTICAL_PLAY.md`, "3D mode"): a copy under `build/`, the
window kept on top, a shot refused while another window overlaps it, the picture taken only with PrintWindow, from the
game's window itself, never from the screen, and input posted to that window, never through the user's cursor or
keyboard. The host-capture ban holds for every other run. Bookworm Adventures also needs a short path: it builds
absolute paths from the working directory in a 256-byte stack buffer, so a copy under a worktree's long path dies at
start with STATUS_STACK_BUFFER_OVERRUN (0xC0000409). origrun's sandbox copies run from `C:\run\game` and never meet it.

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

## Per-game status

What each game reached under origrun, as its session's runs found it. "Trial" means the copy shows PopCap's trial
screen first; its **Play** button is clicked through with `clickready` (it is not a hard lock while plays/time
remain). Captures are the main window's client size. Where the Capture column says no more than "yes", it records
what the game gave on the host before the host-capture ban; captures now come only from runs inside Windows Sandbox.

The 3D column says whether the game itself (its real exe, not the launcher that wraps it) has SexyApp's Direct3D 7
mode, and what `--3d` reaches (see "Hardware acceleration (3D mode)"). Each session adds its own game's registry key
and 3D-flag address to games.tsv.

| Game | Source | Reached under origrun | Capture | 3D | Audio seen | Notes |
|------|--------|-----------------------|---------|----|------------|-------|
| alchemy | local install | trial, then into the game past Play | yes | has one; on needs `--dgvoodoo` (untried) | its BASS music, through COM's DirectSound, which is the stand-in since 2026-11-03 (origrun's manifest): none past the stand-in; before, the system's DirectSound past the stand-in, whose start could reach the speakers | its music is the sound not via the stand-in (see Audio seen) |
| atomica | local install | FATAL ERROR at startup, then exits | — | none | none | FMOD fails to load `sounds\capclick` (Ogg); not the stand-in or display — the Atomica session's to fix |
| mummymaze | local install | title after a slow (~45 s) start | yes, 646×509 | none | none | window is late to appear |
| noahsark | local install | own title screen | yes, 646×509 | none | none | update counter proven here (`[0x4d125c]+0x368`); `at` lands on the exact update |
| sevenseas | local install | own title screen | yes, 646×509 | none | none | update counter `[0x4e21bc]+0x378` (2026-11-07; `+0x368` is its frame time, so an `at` past 10 never fires); on Windows 10.0.26300 it runs as before, its main thread asleep in the frame loop between frames |
| pixelus | local install | trial screen (not clicked through yet) | yes | has one; on needs `--dgvoodoo` (untried) | its BASS music, through COM's DirectSound, which is the stand-in since 2026-11-03 (origrun's manifest): none past the stand-in; before, the system's DirectSound past the stand-in, whose start could reach the speakers | games.tsv's Pixelus.exe (~256 KB) is a smaller build than the port's decompiled exe — its counter offsets do not apply to it |
| tiptop | local install | trial, plays | yes | none | none | DirectInput: posted clicks are ignored; the `hw*` commands are unconfirmed (see the gaps) |
| wordharmony | local install | trial (60 min), Play present | yes | none | none | |
| bookwormadv | local install | in-game Updates dialog; in the sandbox the pilot's menus and battle (2026-11-01) | **in the sandbox** (2026-11-01), 806×629, client at (3,26); update-timed shots and bursts `printwindow-bracketed (sandbox)`, exact with `--pending-draw [0x8259e4]+0x45a --update-state [0x8259e4]+0x488 --draw-empty [0x8259e4]+0x459` | **off and on proven** (on with `--dgvoodoo`) | muted a session | its sound is not via DirectSound (BWA audio work); stage under a short `--stage-root` (long worktree path makes its launcher exit) |
| ningpo | local install | main window | **yes** (after init); **in the sandbox too** (2026-11-01) | none | none past the stand-in (2026-11-03) | windowed DirectDraw; `waitshot`'s flat-retry now captures real content once it has drawn. **Stops at 88.8 s of game time with `--sound stub --clock virtual` once a game is dealt** (2026-11-07; three runs of its mouse23 case, guard off, each at 88800 waits whatever the script had reached; the title screen alone ran 21895 s): the exe's Ogg music streamer, called by its software mixer (0x424fbd), spins in the stream read's end-of-stream branch (0x4253f7: the read gives 0, it seeks back to the loop point at stream+0x10 and reads again) holding the stream's lock, and the main thread waits on that lock in the stream's rewind (0x4253b4), so it never waits again and the clock stops. A lock longer than the buffer (8820 bytes, 100 ms) was ruled out. Not yet known: whether the wall clock does the same past 88.8 s of a dealt game (no run has dealt and lasted that long), and why the read after the seek gives nothing (read stream+0x10 and the song's length in a stalled run). Its identical-play runs use `--sound nodriver`, which has no stream |
| waterbugs | local install | window, then "Screen Setup Failed"; **title menu with `--dgvoodoo`** | window only; with `--dgvoodoo` the title, 3840×2160 stretched | none | none | full-screen DirectDraw: the headless desktop cannot open its 800×600 mode; dgVoodoo2 gives it a desktop-sized window |
| cosmicbugs | local install | window, then "Screen Setup Failed"; **title menu with `--dgvoodoo`** | window only; with `--dgvoodoo` the title, 3840×2160 stretched | none | none | same as Water Bugs |
| dynomite | Steam copy | **title and on** | **yes** | has one; on needs `--dgvoodoo` (untried) | none | fixed: `Steam.dll` is now auto-copied from the Steam library root into the copy |
| bejeweled | wrapped launcher | **title, prepared copy runs** | **yes** | has one; on needs `--dgvoodoo` (untried) | none | prepared as the private notes below say, run via an absolute `--exe` |
| bigmoney | wrapped launcher | **whole games on the host, on the virtual clock** (2026-11-03): title, selector, help, every mode to its game over, name entry, play again, a saved game continued | none on the host | has one; on needs `--dgvoodoo` (untried) | none past the stand-in (2026-11-03): the session it opened before was the mixer's master volume | prepared as Bejeweled (`games/bigmoney/tools/origcmp/orig.sh --prepare`). About 11 times real time with `--clock virtual`. The carved exe runs without registry virtualization: it reads the real `HKLM\SOFTWARE\WOW6432Node\PopCap\BigMoney` and, origrun not being elevated, cannot write it, so every run starts from the same settings. Run one copy at a time: one started while another ran exited at once, before its window. Give it `focus regain` first. `games/bigmoney/docs/IDENTICAL_PLAY.md` |
| bookworm | wrapped Steam exe | prepares cleanly | — | has one; on needs `--dgvoodoo` (untried) | none past the stand-in (2026-11-03, a whole case of the retail 1.03 exe its session runs): the session it opened before was the mixer's master volume | prepare as Bejeweled (not launched yet) |
| rocketmania | Steam copy | not run | — | has one; on needs `--dgvoodoo` (untried) | its BASS music (the same `bass.dll` as Pixelus's), through COM's DirectSound, which is the stand-in since 2026-11-03 (origrun's manifest): none past the stand-in; before, the system's DirectSound past the stand-in, whose start could reach the speakers | Steam copy: `Steam.dll` is auto-copied now, as for Dynomite |
| mahjongjapan | local install | **whole stages to the ending, in the sandbox** (2026-11-01) | **yes**, 806×629 in the sandbox | has one; off (`HardwareAcceleration=0` by `--reg`) | none | JollyBear engine, windowed DirectDraw (`FullScreen=0` by `--reg`). Posted clicks need a `move` 150 ms before. Its players live in `%LOCALAPPDATA%\JollyBear\Mahjong Escape Ancient Japan`, which inside is SYSTEM's, through the 32-bit view: stage them under `sandbox-files/C/Windows/System32/config/systemprofile/AppData/Local/...`, re-signed for `C:\run\game\` (a player's checksum covers the exe's folder). The game reads the cursor with `GetCursorPos` for its hover, which posted input does not move, and `hwmove` did not light a button either in session 0. `--cursor scripted` would answer that call (the exe imports it), but only in a copy that loads the stand-in, which its packed `bass.dll` does not ("The scripted cursor"); untested |
| mahjongchina | local install | **whole stages and the ending, in the sandbox** (2026-11-01) | **yes**, with `games/mahjongchina/tools/ddrawcap` beside the copy: without it every sandbox shot is black (China's window class has a black background brush, which the sandbox paints over the software display's output) | has one; off (`HardwareAcceleration=0` by `--reg`) | none | As mahjongjapan, and `games/mahjongchina/tools/sandbox_run.sh` does it all (the copy, ddrawcap, the BASS proxy, the registry and the re-signed players). Its players live in `%LOCALAPPDATA%\JollyBear\Mahjong Escape` (no "Ancient China"), staged the same way. Never click the PopCap and SpinTop logos at the menu's bottom corners: they are links |
| bookwormadv2 | Steam copy | not run | — | has one; on needs `--dgvoodoo` (untried) | — | not wrapped; `Steam.dll` auto-copied now |
| bonnies | installer copy | **in the sandbox (2026-11-01): menus, the classic and action games, help, options** | **yes, in the sandbox**, 806×629 (`shot`, `waitshot`) | none (SDL 1.2) | none: FMOD 3 makes no DirectSound buffer in the sandbox, so `--sound wav` records nothing (the stand-in attached in stub mode) | SDL 1.2 windowed, GDI; `focus lose`/`regain` pause and resume the game as on the user's desktop; the RNG seed is pinned by `games/bonnies/tools/origrun/patches.txt` (`--patches`, `--seed 1`); no registry, the saves beside the exe |
| bejeweledtwist | Steam copy, prepared by its session | autoplay games on the virtual clock; menus by posted clicks; the 2D board's twists with `--cursor scripted` (on the host, 2026-11-03) | in the sandbox, 806×629 (an 800×600 client) | see its session's notes | an audio session opens and is muted | Not one of the 18; added for the scripted cursor. The board reads the cursor with `GetCursorPos` while it draws (while the window has the focus), so without `--cursor scripted` it never sees a scripted mouse: no selector under it and no twist. With it a `move` puts the selector there and a click twists ("The scripted cursor"). In its 800×600 window the main menu's Play is at 400,300 and Classic at 200,170 in the mode select |

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

Known limits of the message-box watch (guarantee 6):

- **A shown box's own sound still plays.** The watch sees a dialog only once it is already on screen, so the sound
  Windows plays as the box appears has already sounded before the box is logged and dismissed. Since 2026-11-03 the
  stand-in answers the copy's MessageBox calls with no window, so this is left only for a run with `--msgbox show` or
  `--sound real`, a game that never loads the stand-in, and boxes called through `GetProcAddress` ("Message boxes and
  beeps (hooked)").
- **`PlaySound` system sounds are not seen.** They make a sound without a window, so the dialog watch neither logs
  nor stops them, and hooking them needs the user's OK. `MessageBeep` is hooked with the boxes (same section, same
  limits). A `PlaySound` from the game's process (tested with a sound in memory) plays in its default audio session,
  which the stand-in has muted (guarantee 3); one with `SND_SYSTEM` plays in the shared system-sounds session, which is
  not the game's, and is not muted.
- **No guard against URL or program launches.** Nothing in origrun blocks an original that calls
  `ShellExecute`/`WinExec`/`CreateProcess` (for instance a "More Games" `steam://` link, which would bring up the
  user's Steam client). The only safeguard is the rule that **a script must never click a link, "Buy" or "More
  Games" button**; click only Play and the game's own controls.

Other gaps:

- **Registry presets only inside**: `--reg` imports a `.reg` file into a sandbox run (see "Saves and the
  registry"); a host run has no such option and writes only the `--3d` setting, which it puts back afterwards.

- **DirectInput games** (TipTop): the `hw*` commands (`SetCursorPos` + `SendInput`) are **unconfirmed**. They run
  only inside Windows Sandbox, where the input desktop is the sandbox's own (`Default`, after origrun's
  `SwitchDesktop`), and there they are untested. Patching an input function in the game's process would need the
  user's OK first.
