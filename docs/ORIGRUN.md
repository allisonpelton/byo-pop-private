# origrun: running the original games for comparison

`tools/origrun` runs an original game so that a port can be compared with it (CLAUDE.md, "Done means identical")
without disturbing the user, who shares the machine. It runs a **copy** of the original, never the install,
invisibly and by default silently; drives it with scripted input and focus changes; can patch the copy first (to
pin the RNG seed or skip a trial); can time input and captures to the game's own update counter; and can run it
faster than real time on a virtual clock.

The rules a session follows:

- **Captures come only from inside Windows Sandbox** (`--sandbox`). On the host origrun takes no shot of any kind
  ("The host-capture ban").
- **A script never clicks a link button**: "Buy", "Buy Now", "Register", "More Games", a store or web address.
  They open a browser or bring up the user's Steam client, and origrun has no guard against URL or program
  launches. Click only Play and the game's own controls; a trial dialog puts the link buttons next to Play.
- Start `origrun.exe` directly, never under rundesk: it puts the game there itself, and wrapped it fails with
  "cannot start the command".
- A run is one game process for the machine's limit, a sandbox run too, also while it waits in the queue. A run
  past about 4 minutes goes in the background of the session that owns it, never in a subagent.
- Sound and saves: "You are welcome to run the originals with sound on and mess with my save data" (the user,
  2026-10-31; their saves are backed up). Silence and an untouched registry are therefore conveniences, not
  gates, and the user turns the speakers off for a run that needs real sound.
- Hooks of Windows functions inside a copy that the user has allowed: its time reads ("Yes, hook them",
  2026-11-02) and timed waits ("Yes, waits too", 2026-11-03), for the virtual clock; `GetCursorPos` ("Yes, hook
  it", 2026-11-03); `MessageBox` and `MessageBeep` ("Yes, hook them", 2026-11-03); `GetKeyState` and
  `GetLastInputInfo` (both "Yes, hook it", 2026-11-09); its registry calls, so that what a copy writes under
  HKLM\SOFTWARE\PopCap lands under HKCU and a restarted copy finds it ("Yes hook registry calls", 2026-11-10; not
  built yet); and its network calls, so that a connection or download fails at once and nothing leaves the machine
  ("Yes, block them", 2026-11-11: "The network block"). Any other hook needs the user's OK first.
- **No keygens.** Patch the check in the copy, or set the registry values the game itself writes when registered;
  never generate a registration code. How a copy's protection is handled is written only in the private notes
  under "Per-game status", which a published snapshot strips (`docs/PUBLISHING.md`).

## Build

`tools/origrun/build.sh` builds 32-bit (as the originals are) with llvm-mingw's i686 target under `E:\retools`
into the git-ignored `build/origrun-bin/`: `origrun.exe`, the host program, and `dsound.dll`, the stand-in placed
beside the copy, which holds the silent DirectSound, the virtual clock and every hook. A host run needs
`build/native/rundesk.exe`; a `--sandbox` run needs the Windows Sandbox feature and its `wsb` command (Windows 11
24H2 or later) and no rundesk.

## Use

    origrun --game <name> --script <file.txt> [options]
    origrun --exe "<absolute path to a copy already placed>" --script <file.txt> [options]

`--game` looks the install up in `tools/origrun/games.tsv` (name, install folder, exe leaf, and optionally the
game's registry key and 3D-flag address for `--3d`; the repository's table is read first, one beside `origrun.exe`
is a fallback; it is in `.publishignore`) and copies it into `build/origrun/<run>/game`. `--exe` runs a copy you
placed yourself, such as a real exe carved out of a wrapper. Shots land in
`build/origrun/<run>/shots`, logs in `logs`, recorded WAVs in `snd`. The patch lists and per-game configs
(`tools/origrun/patches/`, `tools/origrun/*.cfg`) are git-ignored.

| Option | Meaning |
|---|---|
| `--run <id>` | The run folder under `build/origrun/` (default: the game name): one plain folder name of letters, digits and `- _ . +`, at most 64 characters, not starting or ending with a dot. A run holds `<run>/run.lock`, so a second origrun with the same `--run` is refused. |
| `--sound stub` | The default: the silent stand-in, which accepts and logs every call. |
| `--sound nodriver` | The stand-in reports no sound device (`DSERR_NODRIVER`): for a game's very first run. BASS is then never started either, so the game plays nothing. |
| `--sound wav` | The silent stand-in also records what the game plays ("Recording sound"). |
| `--sound real` | No stand-in: the real audio device (speakers off first). The audio watch still mutes each of the game's sessions as it appears and reports a `SOUND LEAK`. Without the stand-in there is no virtual clock, scripted cursor or keys, coverage, last-input hook, message-box hook or network block (the RESULT line says so: "The network block"). |
| `--timeout <sec>` | Wall-clock cap from the game's launch (default 120, at most 3600). |
| `--fresh` | Only a note to re-copy: delete `build/origrun/<run>` first to force a clean copy. |
| `--copy-only` | Stage the copy and print its folder and exe without launching, to prepare it by hand before `--exe`. |
| `--stage-root <dir>` | Put the game copy under `<dir>\<run>\game`; shots and logs stay under `build/origrun/<run>`. For a game that fails under a worktree's long path (Bookworm Adventures), e.g. `--stage-root C:\byo-og`. Not with `--sandbox`, where the copy is at `C:\run\game`. |
| `--sandbox` | Run inside a fresh Windows Sandbox instead of under rundesk ("Windows Sandbox"). `--sandbox-vgpu` gives it the host's GPU; `--sandbox-connect` opens its window (only for runs the user has allowed onto their desktop); `--sandbox-wait <sec>` is how long to queue behind another run's sandbox (default 600); `--sandbox-probe` starts a sandbox with no game and writes what origrun finds there to `build/origrun/sandbox-probe/probe.txt`. |
| `--reg <file.reg>` | Registry presets for a `--sandbox` run ("Saves and the registry"). Refused on the host. |
| `--counter <expr>`, `--flips <expr>`, `--update-ms <ms>` | The game's update counter, an optional per-frame counter and one update's length ("Timing to the game's updates"); with `flips` a held shot waits for the next present instead of the next update. Config keys `counter=`, `flips=`, `update_ms=`. |
| `--pending-draw <expr>`, `--update-state <expr>`, `--draw-empty <expr>` | A SexyAppBase game's `mHasPendingDraw` (one byte), `mUpdateAppState` and optionally `mLastDrawWasEmpty` (one byte), so that a sandbox shot shows exactly the update it is labelled with. Config keys `pending_draw=`, `update_state=`, `draw_empty=`. |
| `--surface-screen`, `--surface`, `--surface-screen-base`, `--surface-eip`, `--surface-gate`, `--sample` | Read a shot from the game's memory instead of its window ("Timing to the game's updates"). |
| `--input-eip lo,hi[,...]` | Hold an `at` input until the game thread is outside these address ranges (config key `inputeip`). |
| `--arg <text>` | Added to the game's command line, e.g. `--arg -autoplay`. Repeatable, 255 characters in all; letters, digits and `- = _ . ,` only. Passed into the sandbox too. |
| `--watch <expr>` | Log a 32-bit value of the game's memory at each change, with the update counter: "watch 1: 48 -> 47 at update 2150" ("-" while unreadable). Up to four, each polled every millisecond; at most 4000 changes each, or `--watch-max <n>` (a watch that reaches the cap logs "watch N: 4000 changes logged; no more": nothing after it was recorded, which is no sign that the value stopped changing; until 2026-11-10 the option did not reach a `--sandbox` run, which kept the 4000). Measure by the update numbers, not by counting changes: a real-time poll can miss one. |
| `--clock virtual\|real` | Run on origrun's virtual clock, or on the wall clock (the default). |
| `--cursor scripted\|real` | The copy's `GetCursorPos` returns the script's last mouse position, or Windows' cursor (the default). |
| `--keys scripted\|real` | The copy's `GetKeyState` returns the state the script's key and button edges left, or Windows' (the default). |
| `--reg-map popcap\|off` | The copy's creates and opens of `HKEY_LOCAL_MACHINE\SOFTWARE\PopCap` keys go to `HKEY_CURRENT_USER` with the same path, or to Windows as they are (the default): for a 2004 game that keeps its players and settings under HKLM, which an unelevated host run cannot write. Config key `reg_map=popcap`. Refused with `--sound real`. "The registry map". |
| `--msgbox show` | Show the copy's message boxes, with their sound, instead of answering them in the stand-in (`--msgbox hook`, the default). |
| `--coverage <list>` | Record which functions of the list the copy reaches ("Coverage"). Host only. |
| `--patches <file>`, `--no-patches` | The patch list for the copy (default `tools/origrun/patches/<game>.txt` if present), or none ("Patching the copy"). |
| `--seed <N>` | The value of the `{seed}` token in a patch. A seed that no applied patch carries stops the run (exit 2), since the copy would draw its own random content; with `--no-patches` it is a warning. |
| `--3d on\|off` | Set the game's Hardware Acceleration for the run and check from its memory that it ran so; the other mode is exit code 3. `--3d-key <HKCU key>` and `--3d-flag <expr>` give the key and flag when games.tsv has none. With `--exe` alone they come from the games.tsv row with the same exe name. |
| `--dgvoodoo <dir\|env>` | Put dgVoodoo2 into the staged copy, from `<dir>` or from the folder in `ORIGRUN_DGVOODOO` ("Hardware acceleration"). On the host `--3d on` uses `ORIGRUN_DGVOODOO` by itself when it is set. |
| `--in-sandbox <dir>` | origrun's own mode inside the sandbox; refused anywhere else. |

A host run that would take a shot is refused before anything is copied: a script with `shot`, `waitshot`,
`record`, `burst` or `at <N> shot|burst`, a script origrun cannot read, or no script at all (such a run takes a
title shot). `--copy-only` is not refused.

## Script commands

One command per line; `#` starts a comment. Input is posted to the game's window, which works across the hidden
desktop, and a click goes only to a window of the game's own process, routed to the child control under the
point. Coordinates are in the window's client area at the game's own size (640x480 for most), never in a scaled
screenshot's pixels. A title screen may take no click until the game has loaded, and a click before that is lost
without a sign: gate the first click on the game's own state (`at`, `waitfor`), never on a guessed `wait`.

| Command | Meaning |
|---|---|
| `wait <ms>` | Pause (game time on the virtual clock). |
| `move <x> <y>` | Move the mouse. |
| `click <x> <y>`, `rclick <x> <y>` | A left or right click. |
| `clickready <x> <y> [timeout_ms]` | Wait for the window to exist, then click. |
| `key <vk>` | A virtual-key press (decimal or `0x..`). |
| `text <chars...>` | Type the rest of the line after the one space that follows `text`, each character a `WM_CHAR` alone. A game that reads a key as its key down takes `key`. |
| `char <code>` | One `WM_CHAR` of that code, for a control character a script line cannot hold (Ctrl+Z, 0x1a, ends a script file). |
| `down <x> <y>`, `up <x> <y>`, `rdown`, `rup`, `keydown <vk>`, `keyup <vk>` (also `kdown`, `kup`) | One edge of a button or key: a `wait` or an update between the edges makes a held press, and a `move` between a `down` and its `up` carries the button's flag, so it is a drag. A held key changes no state the game reads with `GetKeyState` unless the run has `--keys scripted`. |
| `dclick <x> <y>`, `rdclick <x> <y>` | A double click: down, up, `WM_LBUTTONDBLCLK`, up. |
| `focus lose`, `focus regain` | Post the messages a window gets when it loses or regains the focus (`WM_ACTIVATEAPP`, `WM_ACTIVATE`, `WM_KILLFOCUS`/`WM_SETFOCUS`). |
| `waitfor <expr> <op> <value> [timeout_ms]` | Hold the script until the memory expression (the `--watch` grammar) reads so; `op` is `==`, `!=`, `<`, `<=`, `>` or `>=`; 60 s by default; logged either way. The game runs free meanwhile, on the virtual clock too. For the first input after loading, where an update count is a guess (a loading thread runs on the wall clock while the updates run on the virtual one). |
| `mark` | Read the update counter now; `at +<N> <command>` then counts from that value. For a counter that does not start at 0, and after a `waitfor`. |
| `at <N> <command>` | When the update counter reaches `N`, hold the game's window thread, run the command and resume: `shot`, `burst`, `peek`, `click`, `rclick`, `dclick`, `move`, `key`, `text`, `focus`, or one edge (`down`, `up`, `rdown`, `rup`, `keydown`, `keyup`). Not `quit` (put a plain `quit` after the last `at` line) and not the `hw*` commands. A held game reads every posted message before its next update, and some games lose a down and an up in one update: `at N click x y k` posts the move and the down at `N` and the up at `N+k`, and `at N key vk k` does the same for a key. |
| `at <N> peek <expr> [<expr>...]` | Log `peek u=<counter> <expr>=<value> ...` (signed ints; `-` where unreadable). On the virtual clock the values belong to exactly that update, so a line per update is a state log of the original. Needs no window. |
| `shot <name>` | Capture the window to `shots/<name>.png`. |
| `waitshot <name> [timeout_ms]` | Wait for the window, then capture; it re-grabs while the client area is still flat. |
| `record <prefix> <duration_ms> <interval_ms>` | A numbered frame every interval for the duration, whenever a window is present. |
| `burst <name> <count> <interval_ms>` | A clip: `count` frames in `shots/<name>/0000.png ...` with the `clip.json` shotdiff expects (`{"interval_ms": N}`). A capture takes about 15 to 25 ms. |
| `burst <name> <count> update <k>` | A clip timed to the game's updates: a frame now and every `k` updates; `clip.json` records `k * update_ms`. Needs an update counter. |
| `hwmove`, `hwclick`, `hwrclick <x> <y>`, `hwkey <vk>` | Input through the desktop (`SetCursorPos` + `SendInput`) instead of a posted message. **Only inside Windows Sandbox**: a host run whose script has one is refused. Unconfirmed: in session 0 desktop input does not reach the game, and `hwmove` lights no hover on a SAF game. |
| `msgbox <button> <text...>` | Answer a message box whose caption or text contains `<text>` with the button labelled `<button>` (the match ignores `&` and case). The rules are read before the launch; a box with no rule gets its default button. Never name a link button. |
| `quit` | Ask the window to close, and end the run. |

## What it guarantees, and how each is checked

1. **A copy, never the install.** The install folder is only read; the launched path is under `build/` or
   `--stage-root` and is printed in the log.
2. **Invisible.** On the host the game runs under rundesk, on its private desktop at below-normal priority, and
   origrun drives it from a thread attached to that desktop. With `--sandbox` it runs inside the sandbox, whose
   window never opens (except with `--sandbox-connect`); a guard stops the sandbox if one appears.
3. **Silent by default**, in three layers. The stand-in's DirectSound accepts every call and opens no device,
   and a manifest that origrun writes beside the exe (`<exe>.manifest`, marked as origrun's; `--sound real`
   removes it) makes it COM's DirectSound too, as the BASS of Alchemy, Pixelus and Rocket Mania asks for it. As
   the stand-in loads it also mutes the process's default audio session on every render device, so a stream the
   game opens past it (the system's DirectSound, `waveOut`, `PlaySound`, FMOD's own output) is silent. And the
   host's audio watch mutes each of the game's sessions as Windows creates it and reads its peak meter, which is
   measured before the mute: sound while a session was not muted reached the speakers, and the log then has a
   `WARNING: SOUND LEAK` line and `SOUND LEAK` in its RESULT line, with the times. Otherwise the RESULT line says
   that each session was muted from its start and whether the game made sound at all; a `WARNING` line says when
   a game process loaded the system's `dsound.dll`.

   Details: Windows keeps what it read of a manifest until the exe changes, so origrun sets the exe's write time
   whenever it writes or removes one; a manifest that is not origrun's is left alone, and an exe with a manifest
   of its own ignores one beside it (no game so far). A COM-made DirectSound's buffers keep their play cursor on
   the wall clock, or BASS's mixing would slow the virtual clock. A SexyApp game always shows a silent session,
   because its `SetMasterVolume` goes through the mixer; "no audio session from the game was seen" means the copy
   never loaded the stand-in and played nothing. Not covered: a `PlaySound` with `SND_SYSTEM` (the shared
   system-sounds session) and a game that never loads the stand-in (Mahjong Escape's packed `bass.dll`); a stream
   with a session of its own, or on a render device that becomes active during the run, is muted by the watch
   within milliseconds and reported as a leak.
4. **Screens, only inside Windows Sandbox** ("The host-capture ban"). A capture is a `PrintWindow` of the game's
   window, written as a PNG; nothing ever reads the screen. Inside the sandbox only `PrintWindow` sees a game: a
   blit of the window or screen DC copies no pixels there, and DXGI desktop duplication finds no output in
   session 0. On the host every capture is refused, when it starts, before any window-DC read and once more
   before the pixels are saved. A game that takes the screen full-screen through DirectDraw cannot open its mode
   on rundesk's headless desktop and never draws; `--dgvoodoo` gives it a window.
5. **Scripted input and focus** register in the original as real input does (clicks, typed text, focus changes),
   apart from the cursor position and key state a game polls, for which there are `--cursor scripted` and
   `--keys scripted`.
6. **Message boxes are logged and answered.** The stand-in answers the copy's `MessageBox` calls itself
   ("Message boxes and beeps (hooked)"). A watch thread on the game's desktop answers every other standard dialog
   (class `#32770`) of the game's process, and the boxes of a run with `--msgbox show` or `--sound real`: it logs
   the caption, text, buttons and time, then posts `BM_CLICK` to the button a `msgbox` rule names, else to the
   default button.
7. **Nothing leaves the machine.** The stand-in makes the copy's network calls (WinSock, WinINet, URLMon, WinHTTP)
   fail at once, in every run that loads it, on the host and in the sandbox, with no switch to turn it off
   ("The network block"). Every blocked call is a `NET BLOCKED` line in the run's log, and the RESULT line has a
   `NET` entry: the count of calls blocked, "no network call from the game", or that the block was not in the game.
   Checked by `tools/origrun/nettest.sh` (part of `tools/check.sh origrun`): a test exe calls every hooked function
   and each one fails and is logged. Not covered: the gaps listed under "The network block".

The registry and saves are **not** redirected on the host: a host run writes the game's settings and trial state
to the real registry and save folders, which the user permitted. A `--sandbox` run keeps all of it inside the
sandbox, where it vanishes.

## Windows Sandbox

    origrun --game <name> --script <file.txt> --sandbox [--sandbox-vgpu] [--sandbox-connect] [options]

### The host-capture ban

origrun takes no shot on the host, of any kind and of any game: three captures there returned the user's own
screen, and Windows 11's own DirectDraw loads `dxgi.dll`, so no rule about GPU modules could keep the DirectDraw
originals safe either. Every capture comes from inside Windows Sandbox, where a capture can only see the
sandbox's own display. A shot that shows anything but the game's own window is deleted at once, unopened where
possible, and reported to the coordinator.

### Setup

Windows Sandbox is an optional Windows feature (`Enable-WindowsOptionalFeature -Online -FeatureName
Containers-DisposableClientVM` from an elevated shell, then a restart) of Windows 11 Pro or Enterprise with
virtualization on. Its `wsb` command comes with the Store package `MicrosoftWindows.WindowsSandbox`, which the
built-in app downloads the first time it is opened; origrun downloads nothing. origrun finds `wsb.exe` in System32
or as the app alias under the user's `WindowsApps` folder. The package's broker, `WindowsSandboxServer.exe`, is no
sandbox, ends by itself, and is left alone.

### What a run does

On the host origrun stages the run under `build/origrun/<run>/` (the copy in `game/`, patched, with the stand-in
and any wrapper; `script.txt`; the settings in `sandbox-run.txt`; the configuration in `sandbox.wsb`), takes the
machine's sandbox lock, starts a sandbox with `wsb start --id <guid> --config ...` and runs
`C:\origrun\origrun.exe --in-sandbox C:\run` inside with `wsb exec -r System`. It waits for `sandbox-done.txt`
(the inside run's exit code) for the timeout plus 60 s, stops the sandbox, checks with `wsb list` that it is gone,
releases the lock and copies the inside run's RESULT lines into its own log. Ctrl+C and a crash stop the sandbox
too. `wsb`'s output is in `logs/wsb.txt`, the inside run's log in `logs/origrun-sandbox.log`. `wsb start` returns
in about 2 s; the queue on the lock is the real cost.

Inside, origrun copies the files staged under `<run>/sandbox-files/C/` onto `C:`, imports the registry presets,
sets the 3D setting and runs the game in a job of its own at below-normal priority on the sandbox's desktop
(`WinSta0\Default`), in session 0 as SYSTEM: a sandbox started without `wsb connect` has no user session.
DirectDraw sets a display mode only on the input desktop, which in session 0 is `Winlogon`, so origrun first
makes `Default` the input desktop (`SwitchDesktop`). With `--sandbox-connect` the sandbox's user logs on, and
origrun runs itself and the game again in that session.

### Safeguards

- **Only its own sandbox.** origrun refuses to start while `wsb list` shows a sandbox or a Windows Sandbox
  process runs (`WindowsSandbox.exe`, `WindowsSandboxClient.exe`, `WindowsSandboxRemoteSession.exe`, by exact
  name), which may be the user's own, and it stops only the sandbox it started, with `wsb stop --id`.
- **One sandbox on the machine.** Every origrun opens `byo-pop\sandbox.lock` under `%LOCALAPPDATA%` with no
  sharing and queues behind its holder (logged, up to `--sandbox-wait`). Windows releases it however origrun
  ends. Its text names the holder (`pid`, `run`, `since`, `dir`) and, while its sandbox may be up,
  `sandbox=<id>`.
- **No window on the user's desktop.** origrun never uses the `.wsb` file's `LogonCommand`, and runs
  `wsb connect`, which opens a window, only with `--sandbox-connect`. A guard thread looks every 100 ms for a
  visible window of a Windows Sandbox process, of the broker or of `vmconnect.exe` (exact names) and, on one,
  stops this run's sandbox, logs "STOP: a sandbox window was on the user's desktop" and exits with code 4; with
  `--sandbox-connect` the connect process's window is allowed. Every run ends with "RESULT: no sandbox window
  showed on the user's desktop" or the STOP line.
- **A killed origrun** cannot stop its sandbox. The next run that takes the lock finds `sandbox=<id>`, waits for
  the run that wrote it to end and stops that sandbox; and the inside run arms a power-off (`InitiateShutdown`)
  at the run's timeout plus 90 s, and again 30 s after it writes `sandbox-done.txt`.
- **Captures only where the check passes.** `--in-sandbox` and every capture depend on a check that no option or
  environment variable sets. All three must hold: SMBIOS reports the manufacturer "Microsoft Corporation" and the
  product "Virtual Machine"; the local account `WDAGUtilityAccount` is enabled (the host has it disabled); and
  origrun's own token is SYSTEM or `WDAGUtilityAccount`. The log's first line gives the three values.
- **The configuration.** `Networking`, `ClipboardRedirection`, `PrinterRedirection`, `AudioInput` and
  `VideoInput` are disabled and `ProtectedClient` is enabled, so a game reaches no network (no "More Games" page,
  no update check, no Steam). `vGPU` is disabled, so the sandbox renders in software, unless `--sandbox-vgpu`:
  the user allowed that for Bookworm Adventures' 3D comparison (2026-11-01), Bejeweled Twist's 3D mode
  (2026-11-02), and Typer Shark's and Bejeweled 2's 3D comparisons (2026-11-11); between 22:00 and 08:00 local time
  any game may use it (CLAUDE.md, "Overnight, desktop and sound allowed"), and at other hours another game's 3D runs
  ask first. Exactly two folders are mapped, the run folder read-write
  as `C:\run` and `build/origrun-bin` read-only as `C:\origrun`; origrun resolves both to their final paths (a
  junction shows where it leads) and refuses the run unless the run folder sits directly under the repository's
  `build\origrun` and the binaries' folder ends in `\build\origrun-bin`, both on a local drive.

### What the sandbox offers a game

A fresh virtual machine that vanishes when stopped, with no network adapter and no audio path to the host (a run
still watches the host's audio sessions for one of a sandbox process: "RESULT: no audio session of the sandbox
appeared on the host"). The stand-in and the audio watch work inside as on the host. The display is 1024x768 at
32 bpp with 18 modes, none of them 16 bpp. Without `--sandbox-vgpu` DirectDraw has no hardware layer (no video
memory, no 3D). After the `SwitchDesktop`, exclusive full-screen 800x600x32 works with a flipping primary
surface; a game that asks for 16 bpp full screen may still get it through Windows' compatibility shims. There is
no real cursor: `GetCursorPos` reads 0,0 whatever `SetCursorPos` or hardware input did.

### Saves and the registry

Saves, settings and trial state vanish with the sandbox, so every run starts as a first run. A run that needs a
save stages it: files inside the game's folder go into the copy (`game/`), files elsewhere under
`<run>/sandbox-files/C/` at their path on `C:`.

On the host nothing vanishes, and a save kept outside the game's folder is the user's own. Bejeweled Deluxe keeps
its saved game in `C:\ProgramData\Steam\Bejeweled\userdata`: a host run that quits in the middle of a game leaves
one, and the next run meets it at Play (the new board stays paused and the script's moves do nothing). origrun
does not touch that file; a session that runs Bejeweled on the host deletes `savegame.dat` there before and after
its runs.

Inside, origrun and the game run as SYSTEM, so the game's HKCU is SYSTEM's hive, and UAC never virtualizes SYSTEM:
a game that writes under HKLM writes HKLM itself, not the per-user `Software\Classes\VirtualStore\MACHINE\...`
copy where the host keeps its values. origrun's own reads and writes of a games.tsv key (`--3d`, the cached
`Test3D` results) follow the game, through the 32-bit view.

**Presets** (`--reg <file.reg>`, for instance a regedit export from the host): the host copies the file to
`<run>/sandbox-files/registry.reg`, where a session may also put one itself. Before the launch the inside run
moves every key under `HKEY_CURRENT_USER\Software\Classes\VirtualStore\MACHINE\` to `HKEY_LOCAL_MACHINE\`
(dropping a `SOFTWARE\WOW6432Node` level, which the 32-bit view adds back), keeps HKCU keys as they are, writes the
result to `logs/registry-imported.reg`, imports it with `reg import /reg:32` and logs reg's exit code and how
many of the keys exist afterwards. **After the run** it exports `HKCU\Software\PopCap`, `HKLM\SOFTWARE\PopCap`
(32-bit view) and every other key the presets named, as the game left them, each to
`logs/registry-after-<n>.reg`. reg's own output is in `logs/reg.txt`.

## Recording sound (`--sound wav`)

With `--sound wav` the stand-in writes to `snd/` a static sound as `play_<seq>_buf<id>.wav` each time it is
played, in play order and in the buffer's declared format, and a looping or streaming buffer (mixed music) as one
growing `stream_buf<id>.wav`, capped at about 90 s. The sound log, `logs/sound.log`, is written in every mode: each
buffer create, data write (size, hash, format), play (once or looping, volume, pan, frequency), change and stop,
with a timestamp, which is enough to line up which sound plays when against the port without the WAVs. It covers
what a game plays through DirectSound.

The stand-in's buffers answer as DirectSound's do in every mode: `GetStatus` says a buffer plays from `Play` until
`Stop`, or until a buffer played once has played its length; a finished one-shot's cursor reads 0; `Play` after
`Stop` goes on from where it stopped; `SetCurrentPosition` and `SetFrequency` move the cursor on from the new
place or rate. The cursor runs on the wall clock, or on the virtual clock when that runs.

## Patching the copy

origrun applies a per-game list of static byte patches to the staged copy's files before it launches: a change
to a file on disk, never an injection into a running process. A list pins the RNG seed (the `{seed}` token stands
for `--seed`'s value as four little-endian bytes), skips a trial prompt, or makes a test copy usable (the user
approved the hover patch for test copies on 2026-11-01: rundesk's desktop has no cursor to read, so the patch
makes a game take the mouse position from the posted message instead of `GetCursorPos`). origrun knows and
generates none of the bytes; the game session that knows the code supplies them.

<!-- private -->
The hover patch, as found on Bookworm Adventures: SexyAppBase's handler for a deferred message (FUN_00547ea0 in its
export) takes `WM_MOUSEMOVE` at 0x548635 by asking `GetCursorPos` for the point and `WindowFromPoint` for the window
under it, then tests the point against the client rectangle. On rundesk's desktop `GetCursorPos` fails with
`ERROR_ACCESS_DENIED` and leaves the point unset, so the test never passes and no button lights. The patch makes
that path take the point from the message's own `lParam` (already copied to a local, local_44). The game session
that knows the code supplies the exact bytes in its `patches/<game>.txt` (kind `test`); origrun knows none of them.
<!-- /private -->

The list is `tools/origrun/patches/<game>.txt` (or `--patches <file>`), one patch per line:

    <kind> <file> <where> <expected> <replacement> <reason...>

`kind` is a free word for the log (`seed`, `trial`, `test`); `file` is a path under the copy; `where` is
`off:0xN` (a file offset), `rva:0xN` or `va:0xN` (converted through the PE section table); `expected` and
`replacement` are equal-length hex (`.` and `_` may group the digits), and `replacement` may contain `{seed}`.
Every entry is checked against the file's bytes first, and **one mismatch or malformed line refuses the whole
list and stops the run**, so a patch is never half-applied. Each patched file keeps its original bytes in
`<file>.origrun-unpatched`, which every run restores before it applies the list again.

## Timing to the game's updates

Input and captures can be timed to the game's update counter instead of the wall clock. origrun reads the counter
from outside with `ReadProcessMemory` (on the virtual clock the stand-in reads it inside the game), at an address
the game's session gives in the per-game config:

    counter=[0x4d125c]+0x368      # the int 0x368 bytes past the pointer stored at 0x4d125c
    flips=[0x4d125c]+0x364        # optional: a counter the game bumps once per presented frame
    update_ms=10                  # one update's length, for a clip's interval_ms

`[0xPTR]+0xOFF` reads the pointer at `PTR`, then the int `OFF` bytes into it (for a SexyAppBase game, the app
pointer's global and `mUpdateCount`); a plain `0xADDR` reads the int there; brackets nest
(`[[0x8259e4]+0x36c]+0x38`) and an offset may be subtracted. Each game's offset is its own, even between
SexyAppBase games of one era: take it from the game's own update function. An offset that holds something else
makes every `at` wait for an update that never comes, and a wait that times out on a counter that never moved
says so in its log line.

`at` lands on the exact update, since the counter is polled about every 1 ms against updates of about 10 ms. The
hold is kept short, because in real time the game's clock runs on while it is held: input is posted with no gaps
(about 0.04 ms held), and a held shot resumes the game once the pixels are grabbed, before the PNG is written.
The log gives each hold. A hold begins only once the suspension has taken effect, and origrun resumes the game
first if it crashes while holding it. On the host a held capture is refused before the game is held.

**An update-timed shot in the sandbox** (`at <N> shot`, `burst ... update`; logged as `printwindow-bracketed
(sandbox)`). Once the counter reads `N` the game is held, a helper thread enters `PrintWindow`, and the game is
released to answer it at its next message check; the hold lasts 1 to 2 ms. A shot is refused, never read another
way, if the helper does not take it up within 100 ms or `PrintWindow` does not return within 3 s; one that
returns FALSE is tried up to three times.

**Which frame the shot shows.** SexyAppBase checks messages after an update and its catch-up step but draws that
update in a later `Process` call, so a plain shot at `N` shows `N-1` or an older frame. With `--pending-draw` and
`--update-state`, origrun waits without holding until no update waits to be drawn and none runs (the flag clear
and the step "messages" (0) or "done" (3)), holds, and checks again; the shot then shows update `N` and the log
says "update N (drawn)"; if no such moment comes within 500 ms the shot is refused. A clear flag does not prove a
present, because DrawDirtyStuff also clears it when it skips a draw (nothing dirty, or the loading throttle): with
`--draw-empty` such a shot says "or-earlier", without it every shot says "drawn-unless-skipped". Without the two
options the shot shows `N` or an earlier update and says so. A failed read of the counter, the flag or the step
refuses the shot. The release is not checked: a helper pre-empted for longer than the margin plus an update makes
the shot show `N+1` while it says `N`, which is rare.

Every update-timed shot is listed in `shots/landed.tsv`, started afresh by each run, with its name, its update and
how sure that update is ("drawn", "drawn-unless-skipped", "or-earlier"), so the port can take its shot at the
update the original's landed on. When a capture takes longer than a burst's spacing (about 35 ms for Bookworm
Adventures), its frames land after their updates, and the file gives each frame's real update.

**A shot from the game's memory** ("surface-read (sandbox)"; landed.tsv says "or-earlier (surface)") is for a game
that draws straight to a locked screen, or whose window a wrapper presents through Direct3D, which leaves nothing
for `PrintWindow`. All addresses are hex.

| Option | Meaning |
|---|---|
| `--surface-screen bitsAt,pitchAt,w,h,bpp` | Two globals holding the pixel pointer and the pitch (5-6-5 at 16 bpp, 5-5-5 at 15, 8-8-8 at 32; Cosmic Bugs gets 5-5-5 in the sandbox). |
| `--surface-screen-base ptrAt` | bitsAt and pitchAt are then offsets into the object whose pointer the global at ptrAt holds, read at each shot (Ning Po's display is on the heap). |
| `--surface <addr>` | An Image-like header instead: pixel pointer at +0, width +4, height +8, pitch +0x1c, bpp +0x44, then the masks, bit counts and shifts. Refused unless the pitch is at most 16384 and no more than 64 bytes over a row. |
| `--surface-eip lo,hi[,lo,hi...]` | A gate, and a surface shot is refused without one: every function of the game that writes the locked screen. The shot waits, the game running, until the held thread is in none of them and has no return address into one anywhere in its stack (at most 64 KB; an unreadable page closes the gate). |
| `--surface-gate <addr>` | The other gate: an int that must read 0 (a draw list's count). |
| `--sample a,b` | Values read under the same hold as the pixels. |

**Input in the sandbox.** `at <N> move x y` posts `WM_MOUSEMOVE`, and `down`/`up` post the button messages; a game
that polls the cursor needs `--cursor scripted`, or a private patch that takes the position from the message
(Water Bugs has one, the `mouse` lines of its patch list). `--input-eip` holds an input until the game thread is
outside the listed ranges (Water Bugs: 41f467,41f511, between the message pump and the end of the update loop),
and the log line `at <cmd>: input at counter N (k holds)` gives the update that read it, which is the number to
replay in the port. The game's own `SetCursorPos` calls do nothing there, so a port run that replays such a game
needs its cursor warps off (Water Bugs' runner: `WB_NOWARP=1`).

## The virtual clock (`--clock virtual`)

With `--clock virtual` an original runs as fast as it can draw, and update for update as it would in real time:
a minute of Bejeweled's play takes four or five seconds. Nothing but the time functions is hooked, and no thread
is added to the game: the speed comes only from not waiting.

**What it does.** The clock is in the stand-in (`tools/origrun/vclock.c`) and installs itself when origrun's
environment says `ORIGRUN_CLOCK=virtual`. It pins the stand-in, so a game that unloads `dsound.dll` leaves it
mapped, and patches, in memory only, the import tables of the game's exe and of the other DLLs in the copy's
folder (not origrun's own stand-ins and wrappers, nor `Steam.dll`), looking again about every 16 ms of wall time
for a DLL loaded later. The game's reads of the time (`timeGetTime`, `GetTickCount`, `GetTickCount64`,
`QueryPerformanceCounter`, `QueryPerformanceFrequency`, `GetSystemTimeAsFileTime`,
`GetSystemTimePreciseAsFileTime`, `GetSystemTime`, `GetLocalTime`, `timeGetSystemTime`) then answer from a clock
that starts at the real time of the install and moves only where the game waits.

One thread runs the game, its main thread: the one that started at the exe's entry point, else the oldest. When
it calls `Sleep`, `SleepEx` or `MsgWaitForMultipleObjects(Ex)` on no handles with a timeout, the stand-in asks
Windows with a zero timeout: a ready message or APC is the answer and the clock stays; otherwise the call returns
at once and the clock moves on by the timeout. A timed wait of the main thread on handles that is not ready at
once goes to Windows for real, since another thread may signal it any moment, and the clock moves on by the real
time it took, or by its timeout when that ran out. Waits with no timeout or a zero one, and every wait of the
other threads, go to Windows as before; those threads read the same clock. A main thread that reads the clock 200
times without such a wait (a loop polling the clock) moves it on by 10 µs at each further read, so the loop still
ends.

**The guard.** A thread the game starts runs on the wall clock while the main thread races ahead (Bejeweled's
title was still loading at the update its Play button was clicked). So while a thread the exe started (its start
address lies in the exe) uses more than a quarter of a core, the main thread's waits take their real time and
still move the clock by their timeouts; the clock's log says when this starts and stops. It is a heuristic: a
loader that waits on the disk is not seen. `ORIGRUN_CLOCK_GUARD=0` in origrun's environment turns it off. Where
the guard misses a loader, gate the first input on the game's own loaded flag with `waitfor`, or name the flag as
`--counter` and begin with an `at` (Bejeweled's coverage runs: `--counter 0x53ba60`, whose third byte is the
app's loaded flag, and `at 16777216`).

**In step with origrun.** Through a shared block origrun holds the game's main thread at a wait: `at N` arms a
barrier at which the thread stops as it enters its first wait once the counter has reached `N`, still answering
sent messages. The game starts held once its window is up and stays held between script commands, so a script
starts at the same point in every run, `wait <ms>` is game time, and the update an `at` lands on does not depend
on the machine. The counter and the `--watch` values are read inside the game at each of its waits, so no update
is missed. The stand-in's play cursor and `GetStatus` follow the virtual clock.

`clickready`, `waitshot`, `record`, a `burst` timed in milliseconds, `waitfor`, `quit`, the end of the script and
a run with no script let the game run free, which on this clock means uncapped: ten to over a hundred times real
time. A `record` or timed `burst` then covers far more game time than its milliseconds say; for shots on exact
updates use `at N shot` and bursts by update.

**When holds go off.** If a hold lasts 60 s with no release, origrun ends while the game is held, or a hold's wait
fails, the game holds no more for the rest of the run. origrun logs "WARNING: pid ... holds no more from update
N", falls back to polling the counter, and ends its summary with "HOLDS WENT OFF at update N" instead of "holds
stayed on to the end": what the script did after that point did not land on exact updates. A first hold that
origrun has not taken up within 5 s lets the game run free until origrun arms a barrier.

**Logs.** origrun's log ends with a summary ("virtual clock (pid ...): 77.5 s of game time in 3.7 s of wall time
to its last wait ..."). `logs/clock.log` lists the imports patched, time imports the clock does not cover, the
guard's switches and, for each call site, the waits skipped and the waits that went to Windows. origrun follows
the newest of the copy's processes that has a clock (a game that starts itself again).

**The watch lines differ in one way.** A value the game changes while it handles input between updates `N` and
`N+1` is logged at `N+1` on the virtual clock; a real-time run logs a slow handler's change at `N`, and also sees
values in the middle of an update. Compare the last value of each update.

**What it cannot cover:**
- `SetTimer` and its `WM_TIMER` stay on the wall clock; every SexyAppBase original imports it. `timeSetEvent`,
  waitable timers, timer-queue timers, `SignalObjectAndWait` and `GetMessageTime` are not hooked; the clock's log
  names any it meets.
- A game that paces its frames by anything but a timed wait of its main thread gets no speed, and its clock
  crawls at 10 µs a read: by `WM_TIMER`, by `GetMessage` or another wait with no timeout, by a vertical-sync or
  flip wait, or with its updates on another thread. A game that paces by a timed wait on a handle runs at real
  speed.
- Imports bound late or by another name are not patched: `GetProcAddress`, delay-loaded imports, the
  `api-ms-win-*` sets.
- The threads of BASS and FMOD keep the wall clock. A stream is fed as fast as the game runs, but a game that
  times something by its music's position as the library reports it would drift.
- The clock starts when the game loads `dsound.dll`: most originals load it at run time, and Seven Seas, Noah's
  Ark, Mummy Maze, Atomica, TipTop and Ning Po import it. Time read before that is real, and a game that never
  loads DirectSound through the stand-in (Mahjong Escape's packed `bass.dll`, Bonnie's FMOD 3 in the sandbox)
  never gets the clock.
- Loading: a loader thread's screen lasts about as long as in real time. A load on the main thread takes no game
  time at all, where in real time the game catches up on the updates the load took.
- A game's shutdown loops run the clock ahead, so the game time at exit varies from run to run.
- Shots on the virtual clock, and a run with no script, are untested.

## The scripted cursor (`--cursor scripted`)

Posted input moves no cursor. A game that asks `GetCursorPos` finds Windows' cursor: on rundesk's desktop the
call fails and leaves the point as it was, and in the sandbox it reads 0,0. Bejeweled Twist's board, TipTop and
Mahjong Escape's hover read it that way.

With `--cursor scripted` the stand-in points the copy's `GetCursorPos` imports at its own function
(`tools/origrun/cursor.c`), with the clock's import patching, on either clock. origrun makes a shared block
(`cursor.h`, `Local\origrun-cursor-<id>`), and each command that posts a mouse position writes the point there
first: `move`, `click`, `rclick`, `clickready`, the mouse commands of `at` (a bare `up` keeps the last point), and
the `hw*` mouse commands. The point is kept in screen coordinates, through the game window's `ClientToScreen`
when the command runs, so the game's `ScreenToClient` gives back the script's point. Until the script's first
mouse command the call goes to Windows. The stand-in logs to `logs/cursor.log` (`logs/clock.log` with the clock),
and origrun's log ends with the number of reads the scripted point answered. A game that reads the cursor only
while it has the focus (Twist's board) needs a `focus regain` first.

Limits: only `GetCursorPos` is hooked (`WindowFromPoint`, `GetCursorInfo`, `GetMessagePos`, `GetAsyncKeyState` and
`GetPhysicalCursorPos` get Windows' answer); a game that never loads `dsound.dll` from its folder never gets the
hook; imports bound late are not patched; a window moved after a point was posted keeps the old screen point; and
there is one point for the run, which every process of the copy reads.

## The scripted keys (`--keys scripted`)

A posted key or click changes no key state, so a game that asks `GetKeyState` whether Shift or Ctrl is down while
it handles a posted key finds them up (Mahjong Escape reads its modifiers and the mouse buttons that way).
`--keys scripted` points the copy's `GetKeyState` imports at the stand-in (`tools/origrun/cursor.c`, the cursor's
block). Before each key or button edge it posts, origrun writes the new state there, and from the first edge on
`GetKeyState` answers from it: the high bit while the key is down, the low bit flipped by each press; Shift, Ctrl
and Alt read as down while either side is. The logs end with how many edges were posted and how many calls were
answered.

The state is what origrun last posted, not what the game has read so far. So a modifier goes down on an update
(or after a `wait`) before the key it modifies and up on one after: `at 500 keydown 16`, `at 501 key 9`,
`at 502 keyup 16` is a Shift+Tab, while the same three as plain lines in one go are a plain Tab.

Limits: only `GetKeyState` (`GetAsyncKeyState` and `GetKeyboardState` are not hooked; `hwkey` and `hwclick` go
through Windows); a copy that never loads the stand-in never gets it (a coverage run loads it, since the exe then
imports it); imports bound late are not patched.

## The last-input hook (always on)

A SexyAppFramework original asks Windows once a second how long the screensaver timeout is and when the machine's
last input was (`GetLastInputInfo`); once the input is older than the timeout its `DrawDirtyStuff` draws nothing
until input comes again. A run made while the user is away, or one on the virtual clock past ten virtual minutes,
could then draw only now and then, with stale shots and low draw counts. No run has been seen to do so; the hook
is a precaution. Fifteen originals' exes import the function (Alchemy, AstroPop, Banana Bugs, Bejeweled 1, 2, 3
and Twist, Big Money, Bookworm Adventures, Hammer Heads, Heavy Weapon, Typer Shark, Venice, Zuma's Revenge); any
other game that reads it gets the same answer.

The stand-in therefore answers every copy's `GetLastInputInfo` itself (`tools/origrun/vclock.c`,
`H_GetLastInputInfo`), in every run that loads it, with no option: the last input is always now, on the copy's
own clock. The framework finds the function with `GetProcAddress`, which no import patch reaches, so the stand-in
puts a jump to its own function at the start of user32's `GetLastInputInfo`, in the copy's process only. A copy
that never loads the stand-in (`--sound real`, or a game that loads no DirectSound from its folder) is not
hooked: run those while the user is at the machine.

## The network block (always on)

The user allowed it on 2026-11-11 ("Yes, block them"). Two of Seven Seas' coverage runs clicked its original's
"Submit to Internet High Score List" row by random input, on the host, which has a network (a sandbox has none).
Every run that loads the stand-in therefore blocks the copy's network calls, with no option to turn it off:
a connection or download fails at once, never reaches the network and takes no time.

**What is blocked** (`tools/origrun/net.c`; the functions a game of 2001 to 2010 uses to reach a server or fetch a URL,
and what each returns, with the matching last error):

| Library | Functions | Returns |
|---|---|---|
| WinSock (`ws2_32.dll`, `wsock32.dll`) | `connect`, `WSAConnect`, `WSAConnectByNameA/W`, `sendto`, `WSASendTo` | `SOCKET_ERROR` or `FALSE`, `WSAENETUNREACH` (10051) |
| | `gethostbyname`, `getaddrinfo`, `GetAddrInfoW` (`wship6.dll`'s too) | `NULL` or `WSAHOST_NOT_FOUND` (11001) |
| | `WSAAsyncGetHostByName` | `NULL`, `WSAENETDOWN` (10050) |
| WinINet (`wininet.dll`) | `InternetOpenA/W`, `InternetOpenUrlA/W`, `InternetConnectA/W`, `HttpSendRequestA/W`, `HttpSendRequestExA/W` | `NULL` or `FALSE`, `ERROR_INTERNET_CANNOT_CONNECT` (12029) |
| URLMon (`urlmon.dll`) | `URLDownloadToFileA/W`, `URLDownloadToCacheFileA/W` | `INET_E_DOWNLOAD_FAILURE` (0x800C0008) |
| | `URLOpenStreamA/W`, `URLOpenPullStreamA/W`, `URLOpenBlockingStreamA/W` | `INET_E_CANNOT_CONNECT` (0x800C0004), `*stream` set to `NULL` |
| WinHTTP (`winhttp.dll`) | `WinHttpOpen`, `WinHttpConnect`, `WinHttpSendRequest` | `NULL` or `FALSE`, 12029 |

The hooks are plain stdcall functions that never call Windows' own: no packet is made, no DNS name is asked for and no
socket is touched. `socket` and `closesocket` still work, so a game that opens a socket before it connects sees the
connection fail and not the open. Seven Seas' original imports `wsock32` by ordinal (its `shutdown`); no original of
ours has been seen to import `urlmon` or `winhttp`, so those two are tested only by the test exe.

**How the calls are caught.** Not by the import table: the games import `wsock32` by ordinal, and a library loaded later
or a `GetProcAddress` lookup would be missed. The first five bytes of each function inside the DLL itself become a jump to
the hook (the way "The last-input hook" does it), so every caller in the process reaches it, whatever it imported, in
the copy's process only. A forwarded export (`wsock32`'s `connect` forwards to `ws2_32`'s) is left alone, since the
function it forwards to is patched on its own. The libraries mapped when the stand-in loads are patched at once, without
loading anything (the loader lock is held); a library that loads later is patched when it loads (the loader tells the
stand-in: `LdrRegisterDllNotification`), and the module scan that the clock's and the message boxes' hooks make
(`hooks_rescan`) looks again. The stand-in pins itself in the process, since the jumps point into it.

**What a run shows.** The stand-in writes one line to `logs/net.log` for each blocked call, in one `WriteFile`:

    pid 31164: NET BLOCKED InternetOpenUrlA http://example.invalid/scores?name=x

a host, an address and port, or a URL, whichever the call has, and `network block installed` once a process has the
hooks in. origrun copies the lines of this run (the log appends across runs of one run folder) into its own log and
puts one entry in the RESULT line:

    RESULT: NET: 3 network calls of the game blocked and logged (above, and logs/net.log); each failed at once and nothing left the machine through it
    RESULT: NET: no network call from the game; the block was in place in 1 process
    RESULT: NET: the stand-in's network block was not in the game (it loads no DirectSound from its folder, or --sound real): its network calls were not blocked

The `NET BLOCKED` lines are in the run's log too, ahead of the RESULT lines. A game that loads no stand-in has no
block: a script for it clicks nothing that could connect.

**Gaps.** Not blocked: `ConnectEx`, `WSAConnectByList`, `GetAddrInfoEx` and `getnameinfo`; `bind`, `listen` and `accept`
(an inbound socket opens a port on the machine, though nothing in the games does); WinINet's FTP and Gopher calls and
`InternetCheckConnection` / `InternetGetConnectedState`, which only read state; `ShellExecute`, `WinExec`,
`CreateProcess` and URL protocol launches, which start a browser or the Steam client ("Gaps / next steps"); named
pipes, RPC and every other transport. A game that never loads the stand-in, and `--sound real` runs, get no block.
A blocked call fails at once, so a game that asks again until it gets an answer asks at full speed: a script should
not wait for a network answer.

**Seven Seas' "Submit to Internet High Score List" is not a network call of the game, so the block does not stop it.**
The button (on the high-score screen at y 356 to 386, above the New Game button and presumably as wide: x 135 to 505) opens a dialog (the dialog's SUBMIT button is at
x 387 to 557, y 355 to 386); the dialog's SUBMIT runs the upload (0x41f340), which fills the template `scoreul.tpl` into a file
`score<N>.html` in the game's folder and ends by calling `SexyApp::vf19` (0x42e370, `OpenURL`), whose only action is
`ShellExecuteA(open, <that file>)`: the user's default browser starts, and the form posts to the publisher's site from
there. The game imports no connection function (its one `wsock32` import is `shutdown`). Under origrun the stand-in's
network block therefore logs nothing for it: replays of the Seven Seas coverage run `vlose` (seed 7, the one-life saved game
from its coverage tools) reached the button and the dialog (`NET: no network call`; 9 of the 12 submission functions) but not
the upload. A script keeps its clicks off that row and off the dialog (the rule at the top), and stopping the launch needs a
hook of `ShellExecute` and the process-creating functions, which the user has not allowed ("Gaps / next steps").

## The registry map (`--reg-map popcap`)

A PopCap game of 2004 keeps its settings and players under `HKEY_LOCAL_MACHINE\SOFTWARE\PopCap\<Game>` (Typer Shark: the
options, a `Users` key with one key a player). A copy started on the host is not elevated, so it can neither make nor write
that key and a run's values are gone when it ends: "quit, start again, carry on" cannot be compared there. (In Windows
Sandbox the copy runs as SYSTEM and writes HKLM itself; `--reg` presets carry values from one sandbox run to the next.)

With `--reg-map popcap` the stand-in answers four of the copy's registry imports (`tools/origrun/regmap.c`; the user's OK:
"Yes hook registry calls", 2026-11-10, in CLAUDE.md; nothing else is hooked):

- `RegCreateKeyExA` and `RegOpenKeyExA`: a call on `HKEY_LOCAL_MACHINE` whose path is `SOFTWARE\PopCap` or lies under it
  (any letter case) is made on `HKEY_CURRENT_USER` with the same path. The handle the game gets is the HKCU key's, so
  set, query, enumerate, delete and close work on HKCU with no hook of their own. Every other root or path goes to Windows.
- `RegSetValueExA` and `RegQueryValueExA` only count the values written and read through mapped keys, for the log.

Off unless the command line or the game's `.cfg` asks for it. origrun's log says so when it is on, and the stand-in's log
(`logs/clock.log`, `cursor.log` or `msgbox.log`) has one line when the map first takes effect and each process's totals at
exit. The values are PopCap games' own, which the user lets a run change without restoring; a session that wants a clean
start deletes `HKCU\Software\PopCap\<Game>` before its first run and after its last.

Not done: the HKLM key is not read while the map is on (a copy starts from what HKCU holds, not from an installer's or an
elevated run's values); the wide-character forms, `RegOpenKeyA`, `RegCreateKeyA`, a `RegDeleteKeyA` that names HKLM with a
whole path, a key reached from a handle the game opened itself, and a function found with `GetProcAddress` are not mapped
(each needs the user's OK first); a copy that never loads the stand-in never gets the hook.

Tested with Typer Shark on the host (2026-11-10): a first run that made a player logged 4 creates and 105 opens sent to HKCU
and about 108 values written; `HKCU\Software\PopCap\TyperShark` then held 95 values, and a second run of the same copy read
103 values, 95 of which it found. In Windows Sandbox (2026-11-11) the map works the same way on SYSTEM's hive: a first run
logged 4 creates and 493 opens sent to HKCU, the export held the game's 89 values under `HKEY_CURRENT_USER\Software\PopCap`
(the same 89 a run without the map leaves under HKLM), and a second run given that export as its `--reg` preset found 89 of
the 97 values it read. The map is not needed there. One first run with it on in the sandbox ended one key press behind the
same script without it (the registry values at the end were the same; one run is not enough to blame the map, and it was
not chased), so a tour that must line up with a port update for update runs without the map in the sandbox.

## Message boxes and beeps (hooked)

A message box plays its sound as it appears, before the dialog watch can see it. So in every run with the
stand-in, on the host and in the sandbox, the stand-in points the copy's imports of `MessageBoxA`/`W`,
`MessageBoxExA`/`W`, `MessageBoxIndirectA`/`W`, `MessageBoxTimeoutA`/`W` and `MessageBeep` at its own functions
(`tools/origrun/msgbox.c`), with the clock's import patching.

A hooked box makes no window, plays no sound and takes no focus. The call returns the ID of the button the watch
would have clicked: that of the first `msgbox` rule whose text the caption or the text contains and whose button
the box has (never Help), else the box's default button (`MB_DEFBUTTON1` to `4` pick the nth button of its type's
set). A style Windows refuses returns 0, as MessageBox does. `MessageBeep` returns TRUE with no sound. The rules
reach the stand-in through a shared block (`msgbox.h`, `Local\origrun-msgbox-<id>`), and origrun writes each box
and beep to its log in the dialog watch's own words, marked `[hooked]`, so a script that greps those lines still
finds them (a line break in the text is written `\n`):

    dialog appeared: 'Error' (pid 31164) [hooked MessageBoxA: no window]
    DIALOG caption='Error' text='DirectSoundCreate failed with error '120'.  Sound will be unavailable.' buttons=[OK] [hooked]
      answered with 'OK' (default button) [hooked: returned IDOK]
    MessageBeep(MB_ICONHAND) (pid 35952) [hooked: no sound]

The lines come within 250 ms of the call, and the run ends with the count of boxes and beeps. The stand-in's own
log is `logs/msgbox.log` (`cursor.log` or `clock.log` when those are on).

`--msgbox show` leaves the boxes as they were, shown and with their sound, answered by the watch once on screen:
a phase-two comparison needs it to photograph a box and to see the focus go to it and come back; turn the
speakers off for it.

Limits: a box called through `GetProcAddress` (the C runtime's own error boxes), a delay-loaded import or an
`api-ms-win-*` set is shown and rings; a game that loads `dsound.dll` late shows the boxes that come before it;
`PlaySound` and the other sound functions are not hooked; a hooked box takes no time, so a game that asks again
until it gets another answer (a Retry box) asks at full speed until a `msgbox` rule picks the way out.

## Coverage (`--coverage`)

`--coverage <list>` records which of the original's functions a run reaches, for the coverage check
(`docs/COVERAGE.md`; `tools/coverage.py` writes the list and reads the hits). Host only, with the stand-in. As the
stand-in loads (`coverage.c`), the first byte of every listed function in the exe's executable sections becomes
INT3, and a vectored exception handler takes each breakpoint the first time it runs: it appends the address to
`build/origrun/<run>/coverage.txt` at once (a killed run keeps what it had), puts the byte back and lets the
function run. Nothing in Windows is hooked. The list holds addresses as the exe's file has them.

The recorder sees only what runs after the stand-in loads, so for a coverage run origrun gives the copy's exe one
more import, the stand-in's `DirectSoundCreate`, in a section of its own (`.orcov`): Windows then loads the
stand-in before the game's first instruction. The exe keeps its unpatched bytes beside it and the next run puts
them back; an exe that imports `dsound.dll` already is left alone. Where the 40 bytes after the section table are
not free (Mahjong Escape keeps text there), origrun moves the PE headers up over the DOS stub.

Each line of the hits file is `<address> <order> <thread id>`; lines that start with `#` are the recorder's own
(how many functions it watched, how many addresses lay outside the exe's code, and a closing line, which says
whether the game ended normally). Limits: the exe's own functions only, reached or not, with no counts; code that
reads its own bytes would see the breakpoints; not inside Windows Sandbox. `tools/origrun/covtest.sh` (check.sh's
origrun suite) tests it on `covtest.exe`.

## Hardware acceleration (3D mode)

A SexyAppBase game with a 3D mode keeps the player's choice in the registry value `Is3D` under its own key. At
start it runs the Direct3D test (D3DTester, cached in the key's `Test3D` subkey); when 3D fails it carries on in
software and saves `Is3D=0` at exit. The mode it is really in is `mDDInterface->mIs3D`.

games.tsv's columns 4 and 5 give the game's registry key (under HKCU) and the address of that flag, read as one
byte (Bookworm Adventures: `Software\PopCap\WinBAD` and `[[0x8259e4]+0x36c]+0x38`). The older games keep their
settings under `HKLM\SOFTWARE\PopCap\<Game>`, which an unelevated game reads and writes as
`Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\PopCap\<Game>` under HKCU: that is the key to name.
With both known, origrun polls the flag every 100 ms, logs the first reading and every change, and ends with a
"RESULT: 3D on/off" line. Before every run it snapshots `Is3D` and the `Test3D` values and puts them back after
it, with or without `--3d`, so that a software fallback on rundesk's desktop never switches 3D off in the user's
own play. `--3d on|off` writes `Is3D` for the run; a run whose mode differs from the one asked for, or whose flag
was never read, logs "RESULT: 3D MISMATCH" or "3D CHECK FAILED" and exits with code 3.

Where 3D runs:

| Where | Result |
|---|---|
| rundesk's desktop, no wrapper | Never: Windows gives legacy DirectDraw hardware only to the input desktop ("CreateSurface Windowed DrawSurface: DDERR_NODIRECTDRAWHW"); the game runs in software. |
| rundesk's desktop, `--dgvoodoo` | Works (Bookworm Adventures' on and off are proven): Direct3D 11 does get a hardware device there. No shot, as on every host run. It also gets the full-screen DirectDraw games past "Screen Setup Failed". |
| the sandbox | No original has run in 3D there. In session 0 the Direct3D test fails (DDERR_NOTFOUND), with `--sandbox-vgpu` too, and with `--dgvoodoo` the game crashes at start. With `--sandbox-connect` the game moves to the user session, whose "Microsoft Remote Display Adapter" fails Direct3D 7's CreateDevice. `ORIGRUN_DGVOODOO_CONF` names a dgVoodoo.conf to try in place of origrun's own. |
| the user's visible desktop | Works with no wrapper, and needs the user's leave game by game: Bookworm Adventures' 3D comparison ("Yes, capture it", 2026-11-01) and Bejeweled Twist's ("Sandbox, then desktop", 2026-11-02: first `--sandbox --sandbox-vgpu --sandbox-connect`, and the desktop only if the original still falls back to 2D there), and Typer Shark's and Bejeweled 2's on the same terms ("Yes, both routes", 2026-11-11). Between 22:00 and 08:00 local time any game may run there, with sound (CLAUDE.md, "Overnight, desktop and sound allowed"). |

A run on the user's desktop goes as Bookworm Adventures' `tools/pilot/visrun.py` does it
(`games/bookwormadv/docs/IDENTICAL_PLAY.md`, "3D mode"): a copy under `build/`, the window kept on top, a shot
refused while another window overlaps it, the picture taken only with `PrintWindow` from the game's own window,
and input posted to that window, never through the user's cursor or keyboard. Bookworm Adventures also needs a
short path: it builds absolute paths in a 256-byte stack buffer, so a copy under a worktree's long path dies at
start with STATUS_STACK_BUFFER_OVERRUN (0xC0000409).

Typer Shark's 3D comparison (2026-11-11, `games/typershark/tools/phase2/visrun3d.py`) ran the same way with no host
wrapper: the stand-in's stub sound (`ORIGRUN_SOUND=stub`), its registry map (`ORIGRUN_REGMAP=popcap`) and the scripted
cursor (`ORIGRUN_CURSOR=scripted` with `ORIGRUN_CURSOR_ID`), the window shown without activation, and the process suspended
while an input is posted and a shot read, so each step lands on a whole update. All four sandbox variants had run it in 2D.

**dgVoodoo2** implements DirectDraw and Direct3D 7 over Direct3D 11. Its source is its author's GitHub releases
page, `github.com/dege-diosg/dgVoodoo2/releases`; version 2.87.5 (`dgVoodoo2_87_5.zip`, 9,268,078 bytes, SHA-256
`5ffde6927f7355ca3fdd5d785b581256a8e6539fa13e395a891ade6ba1040850`) was downloaded with the user's OK and is
unpacked in `E:/retools/dgVoodoo2_87_5`, the folder to give `--dgvoodoo` or `ORIGRUN_DGVOODOO`. **It is freeware
that may not be redistributed**: its files are never committed or copied into the repository, only into a staged
copy.

`--dgvoodoo` copies `DDraw.dll` and `D3DImm.dll` (from the folder's `MS\x86`, or the folder itself) into the copy
and writes a `dgVoodoo.conf` of origrun's own: Direct3D 11 at feature level 11_0, a window even when the game
asks for the screen, flip-model presentation, no watermark or splash, the mouse not captured, Alt+Enter off. It
refuses to write into an install folder (anything under `Program Files` or `steamapps`) and never overwrites a
DLL the copy already has. A marker file (`origrun-wrapper.txt`) lists each file placed with its size and SHA-256.
The next run deletes those files first, each only while its size and hash still match and its final path lies
under `build\` or `--stage-root`; a file that changed or cannot be deleted stops the run, since a wrapper must not
carry over into a run that did not ask for it (delete it by hand or stage a fresh copy).

## Per-game status

How each original runs under origrun, from its session's notes (`games/<game>/docs/IDENTICAL_PLAY.md` unless
another file is named). "Host" is rundesk's desktop. Under 3D, "has one" is a game whose real exe has SexyApp's
Direct3D 7 mode: switching it on needs `--dgvoodoo` on the host and is untried unless the cell says more. "BASS
through COM" is music that takes DirectSound through COM, which is the stand-in (guarantee 3). Unless a note
says otherwise, a row with a 3D entry saw no sound past the stand-in; a blank cell is not recorded here. "Trial"
is a copy that shows PopCap's trial screen first, whose Play button `clickready` clicks. Each session adds its
game's registry key and 3D-flag address to games.tsv.

| Game | Source | How it runs | 3D | Notes |
|---|---|---|---|---|
| alchemy | local install | host, virtual clock, short `--stage-root` | has one | BASS through COM |
| astropop | | sandbox, virtual clock, scripted cursor (`games/astropop/tools/origcmp`) | | |
| atomica | local install | host, virtual clock, scripted cursor; its coverage runs too | none | |
| bejeweled | wrapped launcher | the sandbox for its identical-play pass; the host, on the virtual clock, for coverage | has one | prepared as the private notes below say, run with an absolute `--exe`; its host save is in "Saves and the registry" |
| bejeweled2 | | virtual clock, seed patch, scripted cursor, four `--watch` values (`games/bejeweled2/tools/origcmp`) | | |
| bejeweledtwist | Steam copy, prepared by its session | host: autoplay games on the virtual clock, menus by posted clicks, the 2D board's twists with `--cursor scripted`. Shots in the sandbox (an 800×600 client) | see its session's notes | an audio session opens and is muted. The board reads the cursor only while the window has the focus. The main menu's Play is at 400,300 and Classic at 200,170 in the mode select |
| bigmoney | wrapped launcher | host, virtual clock (about 11 times real time): whole games, every screen and mode, a saved game continued | has one | prepared as Bejeweled (`games/bigmoney/tools/origcmp/orig.sh --prepare`). The carved exe runs without registry virtualization: it reads the real `HKLM\SOFTWARE\WOW6432Node\PopCap\BigMoney` and cannot write it, so every run starts from the same settings. Run one copy at a time (a second exits at once). Give it `focus regain` first |
| bonnies | installer copy | sandbox: menus, the classic and action games, help, options | none (SDL 1.2) | FMOD 3 makes no DirectSound buffer in the sandbox, so `--sound wav` records nothing. SDL 1.2 windowed, GDI; `focus lose`/`regain` pause and resume it; the seed is pinned by `games/bonnies/tools/origrun/patches.txt` (`--patches`, `--seed 1`); no registry, the saves beside the exe |
| bookworm | wrapped Steam exe | host (the retail 1.03 exe), virtual clock, short `--stage-root` | has one | prepared as Bejeweled |
| bookwormadv | local install | sandbox: the pilot's menus and battle. Its 806×629 window has the client at (3,26); update-timed shots are exact with `--pending-draw [0x8259e4]+0x45a --update-state [0x8259e4]+0x488 --draw-empty [0x8259e4]+0x459` | off and on proven (on with `--dgvoodoo`) | its sound is not through DirectSound (its session is muted); it needs a short `--stage-root` |
| bookwormadv2 | Steam copy | virtual clock, scripted cursor, seed patch; its saves and registry values in the sandbox | has one; the sandbox has none (`Is3D=0`) | not wrapped; `Steam.dll` is copied in |
| cosmicbugs | local install | host, windowed, virtual clock, scripted cursor, two test patches (the seed, and a frame wait of exactly 10 ms). Sandbox shots by `--surface-screen` (5-5-5) | none | full screen it stops at "Screen Setup Failed" on the headless desktop, which cannot open its 800×600 mode; `--dgvoodoo` gives it a desktop-sized window and the title menu |
| dynomite | Steam copy | host, virtual clock, short `--stage-root` | has one | `Steam.dll` is copied from the Steam library root into the copy |
| mahjongchina | local install | sandbox: whole stages and the ending | has one; off (`HardwareAcceleration=0` by `--reg`) | as mahjongjapan; `games/mahjongchina/tools/sandbox_run.sh` does it all. Shots need `games/mahjongchina/tools/ddrawcap` beside the copy, or every one is black (its window class has a black background brush). Its players live in `%LOCALAPPDATA%\JollyBear\Mahjong Escape`. Never click the PopCap and SpinTop logos at the menu's bottom corners: they are links |
| mahjongjapan | local install | sandbox: whole stages to the ending | has one; off (`HardwareAcceleration=0` by `--reg`) | JollyBear engine, windowed DirectDraw (`FullScreen=0` by `--reg`). Posted clicks need a `move` 150 ms before. Its players live in `%LOCALAPPDATA%\JollyBear\Mahjong Escape Ancient Japan`, which inside is SYSTEM's: stage them under `sandbox-files/C/Windows/System32/config/systemprofile/AppData/Local/...`, re-signed for `C:\run\game\` (a player's checksum covers the exe's folder). Its hover reads `GetCursorPos`; `--cursor scripted` would answer it only in a copy that loads the stand-in, which its packed `bass.dll` does not (a coverage run's exe does); untested |
| mummymaze | local install | virtual clock, scripted cursor | none | in real time its window takes about 45 s to appear |
| ningpo | local install | `--sound nodriver`; shots in the sandbox, after its init | none | windowed DirectDraw. **With `--sound stub --clock virtual` it stops at 88.8 s of game time once a game is dealt**: the exe's Ogg music streamer, called by its software mixer (0x424fbd), spins in the stream read's end-of-stream branch (0x4253f7: the read gives 0, it seeks back to the loop point at stream+0x10 and reads again) holding the stream's lock, and the main thread waits on that lock in the stream's rewind (0x4253b4), so the clock stops. A lock longer than the buffer was ruled out. Not known: whether the wall clock does the same past 88.8 s of a dealt game, and why the read after the seek gives nothing |
| noahsark | local install | virtual clock | none | update counter `[0x4d125c]+0x368` |
| pixelus | local install | virtual clock, scripted cursor, seed patch | has one | BASS through COM. games.tsv's Pixelus.exe (about 256 KB) is a smaller build than the port's decompiled exe, and its counter offsets do not apply to it |
| rocketmania | Steam copy | host, virtual clock, scripted cursor, short `--stage-root` | has one | BASS through COM (the same `bass.dll` as Pixelus); `Steam.dll` is copied in |
| sevenseas | local install | sandbox | none | update counter `[0x4e21bc]+0x378` (`+0x368` is its frame time, so an `at` past 10 never fires); its unregistered copy fills a delay bar and takes a click from about update 1000 |
| tiptop | local install | host, virtual clock, `--cursor scripted` (it reads the pointer with `GetCursorPos`), short `--stage-root` | none | trial |
| typershark | local install | from a copy under `build/origrun/typershark/game` (`games/typershark/docs/GAME_NOTES.md`, section 8) | | its loading thread runs on the wall clock: gate the first input with `waitfor` |
| waterbugs | local install | sandbox, a fresh profile each run; shots by `--surface-screen` with `--surface-eip`; update counter 0x460a1c | none | full-screen DirectDraw, as Cosmic Bugs; `--input-eip` and a `mouse` patch for its input |
| wordharmony | local install | host, virtual clock, scripted cursor, short `--stage-root`; its registry key is cleared before each run | none | trial (60 min) |

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

- **No guard against URL or program launches.** Nothing blocks an original that calls `ShellExecute`, `WinExec`
  or `CreateProcess` (a "More Games" `steam://` link, say). The only safeguard is the rule at the top; "The network
  block" stops a copy's own connections and downloads, not a launched browser or program.
- **A shown box's own sound still plays**, in a run with `--msgbox show` or `--sound real`, in a game that never
  loads the stand-in, and for boxes called through `GetProcAddress`.
- **`PlaySound`** is not hooked. From the game's process it plays in the default audio session, which is muted;
  with `SND_SYSTEM` it plays in the shared system-sounds session, which is not.
- **The registry-call hook** the user allowed on 2026-11-10 is not built.
- **Registry presets only inside**: a host run has no `--reg` and writes only the `--3d` setting, which it puts
  back.
- **The `hw*` commands are unconfirmed**; no game needs them now. Patching an input function in a game's process
  would need the user's OK first.
- **Not built or not tested**: coverage inside Windows Sandbox; a sandbox that stays up between runs and stops
  itself when idle (allowed by the user, 2026-11-07: "Yes, if it helps"); shots on the virtual clock.
