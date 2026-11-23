# origrun: running the original games for comparison

`tools/origrun` runs an original game so that a port can be compared with it (CLAUDE.md, "Done means identical")
without disturbing the user, who shares the machine. It runs a **copy** of the original, never the install,
invisibly and by default silently; drives it with scripted input and focus changes; can patch the copy first (to
pin the RNG seed or skip a trial); can time input and captures to the game's own update counter; and can run it
faster than real time on a virtual clock.

The rules a session follows:

- **Captures come only from inside Windows Sandbox** (`--sandbox`). On the host origrun takes no shot of any kind
  ("The host-capture ban"), with one exception: for a game that passed the in-game frame reader's trial, a host
  run's `at <N> shot` lines are read from the game's own off-screen draw surface ("The in-game frame reader").
- **A script never clicks a link button**: "Buy", "Buy Now", "Register", "More Games", a store or web address.
  They open a browser or bring up the user's Steam client. The stand-in's launch guard fails the launches it
  knows ("The launch guard"), but a script does not lean on it: a link opened some other way (a COM call, a game
  that never loads the stand-in) is not caught. Click only Play and the game's own controls; a trial dialog
  puts the link buttons next to Play.
- Start `origrun.exe` directly, never under rundesk: it puts the game there itself, and wrapped it fails with
  "cannot start the command".
- A run is one game process for the machine's limit, a sandbox run once it holds the sandbox; a run waiting in the
  queue is not (the user, 2026-11-22, "Count only once running"; `docs/RULES.md`). A run
  past about 4 minutes goes in the background of the session that owns it, never in a subagent.
- Sound and saves: "You are welcome to run the originals with sound on and mess with my save data" (the user,
  2026-10-31; their saves are backed up). Silence and an untouched registry are therefore conveniences, not
  gates, and the user turns the speakers off for a run that needs real sound.
- Hooks of Windows functions inside a copy that the user has allowed: its time reads ("Yes, hook them",
  2026-11-02) and timed waits ("Yes, waits too", 2026-11-03), for the virtual clock; `GetCursorPos` ("Yes, hook
  it", 2026-11-03); `MessageBox` and `MessageBeep` ("Yes, hook them", 2026-11-03); `GetKeyState` and
  `GetLastInputInfo` (both "Yes, hook it", 2026-11-09); its registry calls, so that what a copy writes under
  HKLM\SOFTWARE\PopCap lands under HKCU and a restarted copy finds it ("Yes hook registry calls", 2026-11-10; not
  built yet); its network calls, so that a connection or download fails at once and nothing leaves the machine
  ("Yes, block them", 2026-11-11: "The network block"); and its program, file and link launches (`ShellExecute`,
  `WinExec`, `CreateProcess`), which fail at once ("allow blocking programs, files and links", 2026-11-11:
  "The launch guard"); and its window-activation calls (`SetForegroundWindow`, `SetActiveWindow`, `BringWindowToTop`,
  `SwitchToThisWindow`, and `ShowWindow`, `ShowWindowAsync`, `SetWindowPos`, `DeferWindowPos` and `SetWindowPlacement`
  made non-activating), so that a copy on the user's desktop never takes the focus ("Yes, hook window activation
  calls", 2026-11-22, after Bejeweled Twist's 3D original held the user's foreground for a whole visrun run: "The
  activation hook"); and its `ClipCursor` and `SetCursorPos`, which do nothing, so that a copy never confines or
  moves the user's cursor ("Yes, hook both", 2026-11-23, after Feeding Frenzy's original held the user's mouse in a
  small rectangle: "The cursor guard"). Any other hook needs the user's OK first.
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
    origrun --queue      who holds the machine's sandbox lock and who waits for it, in order

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
| `--sound real` | No stand-in: the real audio device (speakers off first). The audio watch still mutes each of the game's sessions as it appears and reports a `SOUND LEAK`. Without the stand-in there is no virtual clock, scripted cursor or keys, coverage, last-input hook, message-box hook, network block or launch guard (the RESULT lines say so: "The network block", "The launch guard"). |
| `--timeout <sec>` | Wall-clock cap from the game's launch (default 120, at most 3600). |
| `--fresh` | Only a note to re-copy: delete `build/origrun/<run>` first to force a clean copy. |
| `--copy-only` | Stage the copy and print its folder and exe without launching, to prepare it by hand before `--exe`. |
| `--stage-root <dir>` | Put the game copy under `<dir>\<run>\game`; shots and logs stay under `build/origrun/<run>`. For a game that fails under a worktree's long path (Bookworm Adventures), e.g. `--stage-root E:\byo-og`. Not with `--sandbox`, where the copy is at `C:\run\game`. |
| `--sandbox` | Run inside a fresh Windows Sandbox instead of under rundesk ("Windows Sandbox"). `--sandbox-vgpu` gives it the host's GPU; `--sandbox-connect` opens its window (only for runs the user has allowed onto their desktop); `--sandbox-wait <sec>` is how long to queue behind another run's sandbox (default 600; the queue is first come, first served, and `origrun --queue` shows it: "Safeguards", "One sandbox on the machine"); `--sandbox-probe` starts a sandbox with no game and writes what origrun finds there to `build/origrun/sandbox-probe/probe.txt`. |
| `--reg <file.reg>` | Registry presets for a `--sandbox` run ("Saves and the registry"). Refused on the host. |
| `--counter <expr>`, `--flips <expr>`, `--update-ms <ms>` | The game's update counter, an optional per-frame counter and one update's length ("Timing to the game's updates"); with `flips` a held shot waits for the next present instead of the next update. Config keys `counter=`, `flips=`, `update_ms=`. |
| `--pending-draw <expr>`, `--update-state <expr>`, `--draw-empty <expr>` | A SexyAppBase game's `mHasPendingDraw` (one byte), `mUpdateAppState` and optionally `mLastDrawWasEmpty` (one byte), so that a sandbox shot shows exactly the update it is labelled with. Config keys `pending_draw=`, `update_state=`, `draw_empty=`. |
| `--surface-screen`, `--surface`, `--surface-screen-base`, `--surface-eip`, `--surface-gate`, `--sample` | Read a shot from the game's memory instead of its window ("Timing to the game's updates"). |
| `--reader-trial` | Marks a host reader run whose shots are for a comparison with sandbox shots of the same screens ("The in-game frame reader"); its log says so. No longer needed for a game without a trial line, since none is required. Refused with `--sandbox` and for any other script. |
| `--reader-surface <expr>`, `--reader-waits <hex,...>` | The game's draw surface and the waits that follow a present, for the reader (config keys `reader_surface=` and `reader_waits=`). |
| `--input-eip lo,hi[,...]` | Hold an `at` input until the game thread is outside these address ranges (config key `inputeip`). |
| `--arg <text>` | Added to the game's command line, e.g. `--arg -autoplay`. Repeatable, 255 characters in all; letters, digits and `- = _ . ,` only. Passed into the sandbox too. |
| `--watch <expr>` | Log a 32-bit value of the game's memory at each change, with the update counter: "watch 1: 48 -> 47 at update 2150" ("-" while unreadable). Up to four, each polled every millisecond; at most 4000 changes each, or `--watch-max <n>` (a watch that reaches the cap logs "watch N: 4000 changes logged; no more": nothing after it was recorded, which is no sign that the value stopped changing; until 2026-11-10 the option did not reach a `--sandbox` run, which kept the 4000). Measure by the update numbers, not by counting changes: a real-time poll can miss one. |
| `--clock virtual\|real` | Run on origrun's virtual clock, or on the wall clock (the default). |
| `--cursor scripted\|real` | The copy's `GetCursorPos` returns the script's last mouse position, or Windows' cursor (the default). |
| `--keys scripted\|real` | The copy's `GetKeyState` returns the state the script's key and button edges left, or Windows' (the default). |
| `--reg-map popcap\|off` | The copy's creates and opens of `HKEY_LOCAL_MACHINE\SOFTWARE\PopCap` keys go to `HKEY_CURRENT_USER` with the same path, or to Windows as they are (the default): for a 2004 game that keeps its players and settings under HKLM, which an unelevated host run cannot write. Config key `reg_map=popcap`. Refused with `--sound real`. "The registry map". |
| `--msgbox show` | Show the copy's message boxes, with their sound, instead of answering them in the stand-in (`--msgbox hook`, the default). |
| `--coverage <list>` | Record which functions of the list the copy reaches ("Coverage"). Host only. |
| `--count <list>` | Count the cycles the copy's threads spend inside the functions of the list, each one's own time, for the cycle split ("Counting cycles inside the original's functions"). Host only. |
| `--patches <file>`, `--no-patches` | The patch list for the copy (default `tools/origrun/patches/<game>.txt` if present), or none ("Patching the copy"). |
| `--seed <N>` | The value of the `{seed}` token in a patch. A seed that no applied patch carries stops the run (exit 2), since the copy would draw its own random content; with `--no-patches` it is a warning. |
| `--3d on\|off` | Set the game's Hardware Acceleration for the run and check from its memory that it ran so; the other mode is exit code 3. `--3d-key <HKCU key>` and `--3d-flag <expr>` give the key and flag when games.tsv has none. With `--exe` alone they come from the games.tsv row with the same exe name. |
| `--dgvoodoo <dir\|env>` | Put dgVoodoo2 into the staged copy, from `<dir>` or from the folder in `ORIGRUN_DGVOODOO` ("Hardware acceleration"). On the host `--3d on` uses `ORIGRUN_DGVOODOO` by itself when it is set. |
| `--in-sandbox <dir>` | origrun's own mode inside the sandbox; refused anywhere else. |

A host run that would take a shot is refused before anything is copied: a script with `shot`, `waitshot`,
`record`, `burst` or `at <N> shot|burst`, a script origrun cannot read, or no script at all (such a run takes a
title shot). `--copy-only` is not refused. The exception is a script whose only shots are `at <N> shot` lines, for a
game that passed the in-game frame reader's trial: those run on the host through the reader, and are refused with
the reason when the game's config or the run does not meet the reader's terms ("The in-game frame reader").

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
| `waitfor <expr> <op> <value> [timeout_ms]` | Hold the script until the memory expression (the `--watch` grammar) reads so; `op` is `==`, `!=`, `<`, `<=`, `>` or `>=`; 60 s by default; logged either way. The game runs free meanwhile, on the virtual clock too, and there the stand-in holds it at the first wait at which the expression reads so (the script's waitfor expressions, at most 8, reach it at launch), so the next command starts from that wait however late origrun sees it. For the first input after loading, where an update count is a guess (a loading thread runs on the wall clock while the updates run on the virtual one). |
| `mark` | Read the update counter now; `at +<N> <command>` then counts from that value. For a counter that does not start at 0, and after a `waitfor`. |
| `at <N> <command>` | When the update counter reaches `N`, hold the game's window thread, run the command and resume: `shot`, `burst`, `peek`, `count`, `click`, `rclick`, `dclick`, `move`, `key`, `text`, `focus`, or one edge (`down`, `up`, `rdown`, `rup`, `keydown`, `keyup`). Not `quit` (put a plain `quit` after the last `at` line) and not the `hw*` commands. A held game reads every posted message before its next update, and some games lose a down and an up in one update: `at N click x y k` posts the move and the down at `N` and the up at `N+k`, and `at N key vk k` does the same for a key. |
| `at <N> peek <expr> [<expr>...]` | Log `peek u=<counter> <expr>=<value> ...` (signed ints; `-` where unreadable). On the virtual clock the values belong to exactly that update, so a line per update is a state log of the original. Needs no window. |
| `count <label>` | With `--count`: write a snapshot of the counters to `counts.jsonl` now. `at <N> count <label>` writes it at update `N` (nothing is held); `quit` writes one labelled `exit` first. `count pause` and `count resume` stop and start the counting (`at <N>` too), to compare a run with and without it. Needs no window. |
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
   a game process loaded the system's `dsound.dll`. Before the script starts, origrun waits up to 120 s for the
   game's window (a game pinned to one core can need over 20 s: BookWorm took 24 s); if none appears the script runs
   anyway, but the RESULT line says `NO WINDOW 120s` and the run is not a valid measurement.

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
   before the pixels are saved; the one host shot is the in-game frame reader's, which reads no window, DC or
   screen. A game that takes the screen full-screen through DirectDraw cannot open its mode
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
8. **Nothing the copy launches starts.** The same stand-in makes the copy's `ShellExecute`, `ShellExecuteEx`,
   `WinExec` and `CreateProcess` calls fail at once with `ERROR_ACCESS_DENIED`, in every run that loads it
   ("The launch guard"): no browser, no Steam client, no second copy of the game, no file opened by its
   association. Every blocked launch is a `LAUNCH BLOCKED` line in the run's log, and the RESULT line has a
   `LAUNCH` entry. Checked by the same `nettest.sh`: the test exe's canary command (`cmd.exe /c echo ...`), called
   through each of the seven functions, never runs. Not covered: the gaps listed under "The launch guard".

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

One exception is approved (the user, 2026-11-22, `docs/RULES.md`): a reader inside the original that copies only
the game's own off-screen frame, never the primary surface, with each shot checked. It is built ("The in-game frame
reader", next). The trial against sandbox shots that each game first had to pass was dropped the same day (the
user: "Drop it"), so any game whose draw surface the reader accepts uses it.

### The in-game frame reader

On the host, a script's `at <N> shot` lines are taken by a reader inside the original, for any game whose config
says where its draw surface is. It lives in the sound stand-in beside the virtual clock (`tools/origrun/reader.c`) and hooks
nothing. When the clock holds the game's main thread at the update an `at` waited for, origrun asks for a read
through the clock's shared block, and the held thread itself calls the methods of the game's own draw surface:
`QueryInterface` for `IDirectDrawSurface7`, `GetSurfaceDesc`, `Lock` read-only, `Unlock` and `Release`. The rows go
into a pixel block (`Local\origrun-vclock-<id>-<pid>-pixels`) that origrun opens read-only, converts with the
surface's own masks and writes as a PNG of the client area (640x480 for a 640x480 game, where a sandbox shot is the
646x509 window; shotdiff cuts the frame away itself). Nothing reads the window, its DC, the screen or the primary
surface.

Two config keys say where to read. `reader_surface=` is a pointer expression in the counter's grammar for the
off-screen surface the game draws its frames into, a SexyAppFramework game's DDInterface `mDrawSurface`
(`[[<app global>]+<mDDInterface>]+0x3c` in the 2003-2004 framework). `reader_waits=` lists the return addresses (hex)
of the main thread's waits at which that surface holds the frame the game last presented: in SexyAppFramework's
`Process`, the `Sleep` of the branch that drew, never the one after an update nor a pacing sleep inside
`DrawDirtyStuff`, which comes before the present. At an `at <N> shot` origrun checks that the game is held at update
N; when the hold's wait is not one of those, it steps the game to its next wait, up to 64 waits or 3 s, and the
counter must still read N. With `pending_draw=` and `update_state=` configured, the wait must also leave no update
waiting to be drawn.

**A game that catches up (`reader_waits=hold`).** Hammer Heads' `Process` runs several updates in one call before its single draw and
Sleep, and reads the clock about 300 times between waits, so no wait ever has the counter at N: the counter has moved on (7403 for
an `at 7401`), and the step to the next hold lands on a clock poll, not a wait. For such a game `reader_waits=hold` takes the
surface in the hold at update N itself, where it stands: the surface then holds the frame the game last presented, the same one a
sandbox shot (PrintWindow of the held window) shows at N, and the counter still must read N before and after the copy. A hold at a
clock poll records the return address of the clock call (as a wait does of its Sleep), and the reader's log line names it; the
hold's site must be one outside the drawing code, so a run's first shots are checked by reading that address (Hammer Heads: all
holds at 0x4D5FCA, the `timeGetTime` after `Process`'s update section and before its draw). `ORIGRUN_READER_TRACE=1` logs every
hold the reader looks at (shot, number, address, counter). A wait list and `hold` can be given together. Measured on Hammer Heads
(`reader_surface=[[0x5cea54]+0x330]+0x68`, counter `[0x5cea54]+0x44c`): the menu screens (15 shots), 129 zoom-wipe frames and a
Classic game's 33 shots, each at its update, nothing ignored in `tools/shotdiff`: identical to the sandbox's wherever the sandbox
landed on the update it was asked for (its `landed.tsv`), apart from the how-to-play movie and the loading screen, which run on
the wall clock. In a long scenario the sandbox's shots landed up to 4700 updates before their label, and those pairs differ.

Before a PNG is written every one of these holds, or the shot is refused. In the game: the expression reads a
pointer to readable memory; that interface's vtable and every method called lie in the system's `ddraw.dll` (a
wrapper's is refused, so a `--dgvoodoo` copy gets no reader shots); the surface's caps include
`DDSCAPS_OFFSCREENPLAIN` and none of `PRIMARYSURFACE`, `FRONTBUFFER`, `BACKBUFFER`, `FLIP`, `COMPLEX`, `OVERLAY` or
`VISIBLE`; the pixel format is RGB at 16, 24 or 32 bits with three disjoint masks; and the `Lock` gives the same
size and format. In origrun, again: the caps; the surface's size equals the game window's client size; the update
counter, read in the game right after the copy, is N; and the byte count is the size's. A refused shot writes
nothing (a file of its name from an earlier run is deleted unopened) and is logged as
`shot <name> refused [reader (host)]: <reason>`. Each taken shot's log line names its method, `reader (host)`
(`reader (host, trial)` in a trial run), with the update, the wait it was read at, the surface's address and caps,
its size, depth and masks, and the counter read. The run ends with `RESULT: reader (host): <n> shots taken, <m>
refused`.

A run uses the reader only when all of this holds, and is refused before anything is copied, with the reason, when
any is missing: a host run whose only shots are `at <N> shot` lines (any other shot, `burst`, `record`,
`waitshot` or hw* input still needs `--sandbox`); `--clock virtual` with the update counter configured; the
stand-in loaded (not `--sound real`); `reader_surface=` and `reader_waits=` in the game's config; and
no trial line is needed (a config's `reader_trial=passed <date> <commit>` is logged, and `--reader-trial` marks a run
whose shots are for a comparison with the sandbox's). `--reader-surface` and `--reader-waits` override the config keys.

**A comparison, when wanted.** No trial is required before a game uses the reader (dropped 2026-11-22: the user, "Drop it"). A session that doubts a reader shot can still run the same scripted screens through the reader on the host (`--reader-trial`, whose log marks its shots as for the comparison) and through the sandbox, and compare every pair with `tools/shotdiff` (`docs/SHOTDIFF.md`) with nothing ignored. Two games ran such a trial before it was dropped, both in windowed 2D: Bookworm Deluxe and Mummy Maze (its tutorial's 16 screens, all matching). The reader accepts only a DirectDraw off-screen surface: a game that draws with GDI or SDL needs a reader of its own, and a full-screen game's flipping chain is refused. The draw surface's place differs by framework: in Mummy Maze's older one it is `[[<app global>]+0x254]+0x14`, in Bejeweled 2's of 2005 `[[<app global>]+0x36c]+0x68` (its `+0x3c` is a critical section, which the reader refuses as no DirectDraw interface), and the 2003-2004 framework has `+0x3c`; Pixelus has `[[<app global>]+0x33c]+0x64` (its `+0x5c` is the primary surface, which the reader refuses); take it from the game's own export. A comparison with the sandbox shows one more thing to know: a title's loading thread runs on the wall clock, so at a fixed update the menu's animations stand wherever that machine's load left them (Bejeweled 2's host and sandbox shots were an animation phase apart). Gate a script on the game's loaded flag (`waitfor`, `mark`, then `at +N`) to take the load's length out of a comparison. And a host run starts from the real registry where the sandbox starts from a fresh one plus its `--reg` preset, so a copy that asks questions at start only on the host is reading the user's values: Pixelus sat under its update-check boxes until it ran with `--reg-map popcap` and an empty HKCU key of its own, deleted before each run. Export a key before changing or deleting it, put it back afterwards, and delete nothing under HKLM (restoring it needs an elevated prompt).

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
machine's sandbox lock (after its turn in the queue), starts a sandbox with `wsb start --id <guid> --config ...` and runs
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
  **The queue is first come, first served** (since 2026-11-22). A run first takes a ticket, a file in
  `byo-pop\sandbox-queue\` named by its arrival time and pid (`<16 hex digits of the time>-<8 of the pid>.ticket`,
  so the names sort in arrival order), and only the run whose ticket is the oldest live one tries the lock. The
  ticket's text gives `pid`, `run`, `since`, `dir`, the process's creation time (`created`, so a reused pid is
  not taken for the run) and `wait`, and `held=<time>` once its run has the lock: the holder keeps its ticket
  until it releases the lock, so it heads the queue. A ticket is deleted when its handle closes, so Windows
  removes it however origrun ends; one left all the same (Windows itself stopped) names a process that no longer
  runs, and the next run that lists the queue skips it, deletes it and logs "removed the ticket of run ...". A
  waiting run's log gives its place when it queues and whenever it changes ("sandbox queue: position 3 of 4;
  ahead: ..." with each run ahead, its pid and its minutes), and once a minute the seconds waited.
  `origrun --queue` prints who holds the lock (run, pid, since when) and who waits, in order, with run, pid and
  minutes waited. To see whether the lock is held it opens it for reading, which keeps a run that tries it at
  that instant out for one try (a quarter of a second). An origrun built before the queue takes no ticket and
  still retries the lock, so it can still jump the queue, and `--queue` then says the lock is "held by a run
  with no ticket": rebuild origrun after merging the trunk. `tools/origrun/queuetest.sh` tests the order with
  six fake waiters (`--queue-waiter`, in a folder of its own given by `--queue-dir`, never the machine's queue;
  no sandbox, no game, about 8 s): one holds the lock, five arrive 300 ms apart, one of them is ended with
  `TerminateProcess` while it waits and a leftover ticket of a dead pid sits at the head; the lock must go to the
  live ones in arrival order, the logs must show their places changing, and `--queue` must list them in order.
  **A session keeps at most one sandbox run queued** and starts the next when it ends, so that every session's
  runs take turns: a session that queued five would put the others' behind all five. Put several scenarios in one
  run where the script allows. A waiting run takes none of the machine's 10 game processes (the user, 2026-11-22,
  "Count only once running"); the sandbox it runs in counts once it starts.
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
  (2026-11-02), and Typer Shark's and Bejeweled 2's 3D comparisons (2026-11-11), and since 2026-11-15 any game may
  use it at any hour (CLAUDE.md, "Desktop and Sandbox at any hour"). Exactly two folders are mapped, the run folder read-write
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

**The user's cursor.** An original's `ClipCursor` and `SetCursorPos` reach the user's own cursor from rundesk's
private desktop; the stand-in's cursor guard makes both do nothing ("The cursor guard"). A copy that runs without the
stand-in (`--sound real`, or a game that loads no DirectSound from its folder: its RESULT: CURSOR line says so) needs
the calls patched out instead (kind `safety`: pop the arguments and leave TRUE in `eax`; `ClipCursor(NULL)`, which
frees the cursor, may stay), and a session that runs one watches `GetClipCursor` on the host: anything but the whole
screen means stop.

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
outside the listed ranges (Water Bugs: 41f489,41f511, the update loop past its frame wait; the older 41f467,41f511
took in the wait itself, which was harmless only with 1 ms frames: under the 10-step-frame patch it held every input
for thousands of updates),
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
sent messages. The game starts held once its window is up: the stand-in holds it at its first wait with a window of its own up
(visible, top-level and not owned, at least 64x64, the test origrun uses to find the window), and origrun turns that
into the next wait when it sees a window first. A window that goes again before the script starts, as a splash another
thread closes, does not count: the game runs on to a wait with a window up again. Stepping the game wait by wait instead
cost a round trip a wait, about 100 ms with the machine busy (80 s before Bookworm Adventures Volume 2's window). The
game stays held between script commands, so a script starts at the same point in every run, `wait <ms>` is game time, and the update an `at` lands on does not depend
on the machine. The counter and the `--watch` values are read inside the game at each of its waits, so no update
is missed. The stand-in's play cursor and `GetStatus` follow the virtual clock.

`clickready`, `waitshot`, `record`, a `burst` timed in milliseconds, `waitfor` (until it is met), `quit`, the end of the script and
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

## The activation hook (on the user's desktop)

`tools/visrun.py` runs an original on the user's visible desktop, and the run must never take their focus. visrun
launches the game with `SW_SHOWNOACTIVATE`, but that covers only the game's first `ShowWindow`; later calls
activate the window, and Windows refuses visrun's `SetForegroundWindow` to hand the foreground back, because visrun
is not the foreground process (a Bejeweled Twist 3D run held the user's foreground for its whole 26 s: 1268 takes,
0 hand-backs, 1268 refusals). The user allowed the hook on 2026-11-22 ("Yes, hook window activation calls").

The stand-in (`tools/origrun/activation.c`) therefore changes nine user32 functions in the copy's process, in every
run that loads it. `SetForegroundWindow` and `BringWindowToTop` return TRUE and do nothing; `SetActiveWindow` returns
`GetActiveWindow()` and changes nothing; `SwitchToThisWindow` does nothing. `ShowWindow` turns `SW_SHOWNORMAL`,
`SW_SHOWDEFAULT` and `SW_RESTORE` into `SW_SHOWNOACTIVATE`, `SW_SHOW` into `SW_SHOWNA` and `SW_SHOWMINIMIZED` into
`SW_SHOWMINNOACTIVE`, and passes the rest on; `SW_SHOWMAXIMIZED` has no form that does not activate, so it passes on
and is counted on its own. `ShowWindowAsync` maps its command the same way, and `SetWindowPlacement` maps the `showCmd`
of a copy of the `WINDOWPLACEMENT` and passes the copy on, so the caller's structure is not changed. `SetWindowPos` and
`DeferWindowPos` get `SWP_NOACTIVATE` added. The last five still do their work: each hook calls the real function
through a trampoline.

The way in is the last-input hook's: five bytes at the start of user32's own function become a jump, so every caller
in the process reaches the hook, ddraw.dll, the d3d DLLs and a function found with `GetProcAddress` included, which an
import patch would miss. The trampoline copies the bytes the jump overwrote and knows the three starts the 32-bit
user32 has (`mov edi,edi; push ebp; mov ebp,esp`, `jmp [address]`, `jmp`); another start leaves that function
unhooked, and the install line says so. What it can miss: a caller that makes the system call itself (win32u's
`NtUser*` functions or a stub of its own) and user32's own internal calls (a dialog or menu that activates its window).
Left unhooked on purpose, because none of them makes a background window the foreground window here: `SetFocus` (it
moves the keyboard focus within the calling thread's own queue; the foreground window of another process stays),
`CreateWindowEx` (a window is shown through `ShowWindow` or `SetWindowPos`, which are hooked), `AnimateWindow`, the dialog
functions (a dialog is shown through `ShowWindow`, and `MessageBox` is answered by the message-box hook) and input sent
to a window (it activates a window only when something clicks it, and visrun's posted input is messages, not clicks).
Hooking any other function needs the user's OK first.

`logs/net.log` of the run folder holds the install line ("activation hook installed: reached ..."), the CBT line
("activation CBT block installed on N threads ...") and, at exit, how many calls each function answered, with the
`SW_SHOWMAXIMIZED` passes and the CBT refusals. The hook goes in only where a copy's window could take the user's
focus: on the user's own desktop (the interactive `Default` desktop), or anywhere with `ORIGRUN_ACTIVATION=block`,
which visrun sets for every run it starts and actest uses under rundesk. On rundesk's private desktop and in the
sandbox a copy's window is activated as in a plain run, so a game that waits for the focus runs there as it always has
(the log says "activation hook off (not the user's desktop ...)" or, in the sandbox, "activation hook off
(ORIGRUN_ACTIVATION=allow)"; the first version, f0a447c0, had it on everywhere). The sandbox's desktop is `Default` too,
and the stand-in's own test of the user name did not tell it apart (Bookworm Adventures' full-screen runs, 2026-11-22:
the game never activated and a click on Options' Credits did not take), so origrun, which knows it is in the sandbox
from what the machine reports, sets `ORIGRUN_ACTIVATION=allow` there itself. Nothing from the host's environment
reaches the sandbox side, so a run cannot ask for the hook there. `ORIGRUN_ACTIVATION=allow` turns the hook and the CBT block off everywhere
(the log says "activation hook off"), for a run that needs the real behaviour. A copy that never loads the
stand-in is not hooked.

**The CBT block.** The first real run of Bejeweled Twist's 3D original took the foreground once and visrun stopped it,
before visrun had found the game's window, although the install line named all nine functions as reached and the
stand-in had attached at process start. The log of that run (net.log, sound.log, cursor.log of its kept folder) holds
only the install line: the game was killed before the exit counts were written, so it did not say which call activated
the window. Whatever the path was (a window created with `WS_VISIBLE`, `SetFocus`, a Direct3D 9 `CreateDevice` or
`Reset`, win32u's `NtUser*` entry points, user32's internal calls, or the process's first `ShowWindow`, whose command
Windows takes from the startup information and not from the call, so the hook's mapped command never arrives), the
function hooks cannot be the whole answer. The stand-in therefore also puts a thread-local `WH_CBT` hook on every thread
of the copy that can own windows; its procedure answers 1 to `HCBT_ACTIVATE`, which makes the system not activate the
window, and passes every other code to `CallNextHookEx`. It does not depend on which call asked for the activation. The
hook has no DLL (`hMod` NULL, a thread id of this process), so `SetWindowsHookEx` loads nothing and is safe under the
loader lock; it runs from DllMain on process attach for each thread a toolhelp snapshot lists, and from
`DLL_THREAD_ATTACH` on each new thread (the stand-in no longer calls `DisableThreadLibraryCalls`). SetWindowsHookEx
fails (error 87) for another thread that has no user32 state yet, which is every thread of a process that loads the stand-in
late and has not made a window; a short-lived helper thread of the stand-in tries those again every 20 ms for 5 s and logs
how many it reached. A game that imports dsound.dll has only its main thread when the stand-in loads, so nothing waits on
it.

**The live record.** Each activation event (a hooked function that changed or refused something, or an `HCBT_ACTIVATE`
refused) writes one line to net.log at once, so a killed run still shows what happened: the function or "CBT activate",
the window handle and class, the command or flags, the caller as module+offset (for the CBT block the first frames
outside user32, win32u, ntdll and the stand-in, from a frame-pointer walk that can be short in code compiled without
frame pointers), and the process that owned the foreground window just before. At most 200 lines per process, then one
"further events not logged" line; the exit counts stay.

What the two together still cannot catch: a thread that has no user32 state when the stand-in loads and creates its first
window before the helper's next try; and the foreground right a new process inherits. Windows lets a process take the
foreground when the process that started it holds it, and visrun's ancestors include the user's foreground app. The
CBT block refuses the activation itself, so that right should not matter, but only a run on the user's desktop shows
it: the private desktop actest runs on has no foreground process and no ancestor holding the foreground right, so a pass
there cannot say whether the inherited right gives a copy's window the foreground by a path that sends no
`HCBT_ACTIVATE` (nothing found so far does). A stop with status 5 on the user's desktop now names what happened in
visrun.log. The block also refuses the user's own click on a copy's window, so a copy under visrun cannot be clicked into
the foreground.

`tools/origrun/actest.sh` (in `tools/check.sh origrun`) tests it on `actest.exe`: a window shown inactive, then every
function and `ShowWindow` command called on it through the import table, through `GetProcAddress` and from a second
DLL, with neither the foreground nor the active window ever being it and the window in the state the mapped command
gives; then what only the CBT block stops: a window created with `WS_VISIBLE`, `SetFocus` on a top-level window,
win32u's `NtUserSetForegroundWindow` through `GetProcAddress` (the case is skipped, with a line saying so, when win32u
does not export it), a window created on a thread that existed before the stand-in loaded and on one started after it,
and the process's first `ShowWindow(SW_SHOWNORMAL)` in two child runs, one with the default startup information and
one with `STARTF_USESHOWWINDOW` and `SW_SHOWNORMAL`. A second run with `ORIGRUN_ACTIVATION=allow` checks that each of
them reaches Windows and activates the window, and that net.log holds no CBT line. The live record is checked for a
CBT event with class and caller, a hooked-function event with its foreground owner, and its cap (200 lines and one
"further events" line after 300 calls).

visrun logs at the start of a run whether the hook was installed (read from `net.log`), and its `keep_focus_away`
thread still tries one hand-back, but stops the run at once if the game's process owns the foreground window all the
same: it ends the game process by its pid, the `finally` block restores the window, registry and saves, the log says
"the game took the user's foreground: run stopped (handed back H, refused R)" and visrun exits with status 5. visrun.log
then also holds the foreground window's class, title, style, extended style, thread and process, the milliseconds since
the launch, and the last 30 lines of net.log that start with the game's pid, so the cause is in one place.

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
`InternetCheckConnection` / `InternetGetConnectedState`, which only read state; named pipes, RPC and every other
transport. A program, file or link the copy launches is the launch guard's ("The launch guard"), not this block's. A game that never loads the stand-in, and `--sound real` runs, get no block.
A blocked call fails at once, so a game that asks again until it gets an answer asks at full speed: a script should
not wait for a network answer.

**Seven Seas' "Submit to Internet High Score List" is not a network call of the game, so this block does not stop it; the launch guard does.**
The button (on the high-score screen at y 356 to 386, above the New Game button and presumably as wide: x 135 to 505) opens a dialog (the dialog's SUBMIT button is at
x 387 to 557, y 355 to 386); the dialog's SUBMIT runs the upload (0x41f340), which fills the template `scoreul.tpl` into a file
`score<N>.html` in the game's folder and ends by calling `SexyApp::vf19` (0x42e370, `OpenURL`), whose only action is
`ShellExecuteA(open, <that file>)`: the user's default browser starts, and the form posts to the publisher's site from
there. The game imports no connection function (its one `wsock32` import is `shutdown`). Under origrun the stand-in's
network block therefore logs nothing for it: replays of the Seven Seas coverage run `vlose` (seed 7, the one-life saved game
from its coverage tools) reached the button and the dialog (`NET: no network call`; 9 of the 12 submission functions) but not
the upload. A script keeps its clicks off that row and off the dialog (the rule at the top); the launch itself is stopped
by the launch guard that follows.

## The launch guard (always on)

The user allowed it on 2026-11-11 ("allow blocking programs, files and links"), after the network block's test showed
that Seven Seas' submit button starts the user's browser and no network call of the game is involved. Every run that
loads the stand-in blocks the copy's program, file and link launches, with no option to turn it off: the call returns
failure at once and nothing starts. It is the network block's machinery (`tools/origrun/net.c`: the same jump written
over the first five bytes of each function, the same patching of libraries mapped at load or loaded later, the same
log file).

| Library | Functions | Returns |
|---|---|---|
| shell32 | `ShellExecuteA/W` | `(HINSTANCE)5` (`SE_ERR_ACCESSDENIED`, a value of 32 or below is a failure), last error `ERROR_ACCESS_DENIED` (5) |
| | `ShellExecuteExA/W` | `FALSE`, last error 5, `hInstApp` 5, `hProcess` `NULL` |
| kernel32 and kernelbase | `WinExec` | 5 (below 32 is a failure), last error 5 |
| | `CreateProcessA/W` | `FALSE`, last error 5, `PROCESS_INFORMATION` zeroed |

kernel32's `CreateProcess` and `WinExec` are in front of kernelbase's on current Windows, and on older versions the one
is a stub or a forwarder of the other: the jump goes into both, and a function that forwards is left alone. `system`,
`_spawn*`, `_exec*` and `rundll32 url.dll,FileProtocolHandler` end in `CreateProcess`, so they are caught there; a link
(`http:`, `steam:`, `mailto:`), a file opened by its association and a folder opened in Explorer end in `ShellExecute`.

**What a run shows.** One line in `logs/net.log` for each blocked launch, with the verb, file and parameters, or the
command line:

    pid 31164: LAUNCH BLOCKED ShellExecuteA open C:\...\game\score3.html

origrun copies the lines of this run into its own log, ahead of the RESULT lines, and adds an entry of its own:

    RESULT: LAUNCH: 1 launch of the game blocked and logged (above, and logs/net.log); each failed at once and nothing was started through it
    RESULT: LAUNCH: no program, file or link launch from the game; the guard was in place in 1 process
    RESULT: LAUNCH: the stand-in's launch guard was not in the game (it loads no DirectSound from its folder, or --sound real): its program, file and link launches were not blocked

The line with `launch guard installed` in `net.log` tells which processes had the hooks in.

**Tested** by `tools/origrun/nettest.sh` (part of `tools/check.sh origrun`, with the network block): the test exe asks
`ShellExecuteA`, `ShellExecuteW`, `ShellExecuteExA`, `ShellExecuteExW`, `WinExec`, `CreateProcessA` and `CreateProcessW`
(the wide ones and `ShellExecuteExW` found with `GetProcAddress`) to start `cmd.exe /c echo ... > nettest-launched.txt`;
each must fail with the code above, return no process handle and leave a `LAUNCH BLOCKED` line, and the file must not exist
a second and a half later. The second run of the script must count only its own launches.

**Gaps.** Not blocked: `CreateProcessAsUser`, `CreateProcessWithLogonW`, `CreateProcessWithTokenW` and
`CreateProcessInternalW`, which nothing in the games calls; COM launches (`IShellDispatch::ShellExecute`, `IWebBrowser2`,
`HlinkNavigate`, `CoCreateInstance` of a browser) and DDE; a direct system call; and the same cases as the network
block's (a game that never loads the stand-in, `--sound real` runs). A copy that starts a second process of itself or a
helper program (a restart after a setting changes, a wrapper that starts the real exe) now fails at that call and runs
on without it, so a game that needs the helper to go on may stop at that point; the RESULT line shows it.

**Seven Seas.** Its submit button's upload (0x41f340) ends in `ShellExecuteA(open, score<N>.html)`: under the guard it
fails and logs `LAUNCH BLOCKED ShellExecuteA` with that file name, and the game goes on as if the browser had not started.
That launch itself has not been seen from the real button. Three scripted replays on the host (the coverage run's one-life saved
game, then clicks along the Submit row and at the dialog's SUBMIT, the right-hand button, x 387 to 557, y 355 to 386) opened and
closed the dialog (9 of the 12 submission functions) but never pressed SUBMIT, and the host takes no shot to show why. The last
one ran under the guard: the stand-in patched all seven functions in the game's process, the game ran as it did before, and its
log says `RESULT: LAUNCH: no program, file or link launch from the game; the guard was in place in 1 process`. The call that
Seven Seas makes (`ShellExecuteA` with an `open` verb and a file path) is the test exe's first launch. A later try takes shots
inside Windows Sandbox to see the dialog before it clicks.

## The cursor guard (always on)

The cursor's clip rectangle and its position belong to the whole window station, so rundesk's private desktop does not
contain them: a copy that calls `ClipCursor` with a rectangle confines the user's own mouse to its hidden window, and one
that calls `SetCursorPos` moves it. On 2026-11-23 repeated runs of Feeding Frenzy's original (`FeedingFrenzy.exe` on a
rundesk desktop) held the user's mouse in a small rectangle near the top left, each clip its window's client area, about
once every ten seconds while the runs went on. The user allowed the hook that day ("Yes, hook both").

The stand-in (`tools/origrun/net.c`, beside the network block and the launch guard, by the same five-byte jumps) makes
`user32!ClipCursor`, `user32!SetCursorPos` and the `win32u` system calls behind them (`NtUserClipCursor`,
`NtUserSetCursorPos`) do nothing and say they succeeded, in the copy's process only, whoever there calls them (DirectDraw's
exclusive mode as well as the game). Windows' are never called. The first 20 calls of a process are logged to
`logs/net.log` as `CURSOR BLOCKED <function> <rectangle or point>`, the rest are counted, and the run's
`RESULT: CURSOR` line gives the total. A copy that reads the cursor back gets what it got before: the scripted cursor
(`--cursor scripted`), or on rundesk's desktop a failed `GetCursorPos`. visrun's runs carry the guard too.

It is off inside Windows Sandbox, whose cursor is the sandbox's own (origrun sets `ORIGRUN_CURSORGUARD=off` there), so a
game's own cursor warps there still happen as in the game. A copy that never loads the stand-in (`--sound real`, or a game
that loads no DirectSound from its folder) has no guard, which the RESULT line says: patch its calls out of the copy
("Patching the copy"). check.sh's origrun suite tests the guard on `nettest.exe` (`tools/origrun/nettest.sh`): the four
jumps are in place, the calls succeed, the cursor's clip is unchanged and the three calls are logged and counted.

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

## Counting cycles inside the original's functions (`--count`)

`--count <list>` counts the processor cycles the copy's threads spend inside chosen functions of its exe, the original's
side of the cycle split (`docs/TEST_COMMANDS.md`, "The cycle split"; the port counts the same boundaries with its
`-cyclesplit` option). Host only, with the stand-in, and the exe must sit at its preferred base (the originals do). The list
has one function a line, `<label> <address> len=<N>`: the exe's virtual address of the function, and the length of its first
whole instructions, five bytes or more with no relative branch or call among them; `tools/origrun/prologue.py <exe>
<label>=<address>...` finds the length and `--check <list>` checks a list. origrun reads those bytes out of the exe, so a
list in the repository holds addresses and lengths and nothing of the game (BookWorm's:
`games/bookworm/tools/profile/count.txt`). The labels `update`, `draw` and `present` are the parts `tools/cyclesplit.py`
reports; others are written to the file and ignored by it.

As the stand-in loads (`count.c`; origrun gives the exe the stand-in as an import, as for `--coverage`), the first
instructions of each listed function are copied into a stub of the stand-in's and replaced by a jump to it. The stub
counts a call at its entry and its exit: it swaps the return address for a thunk of the stand-in's, which takes the
thread's cycle count (`QueryThreadCycleTime`) on the way in, last, and again on the way out, first. A function's cycles
are its own: its time less the time of the counted functions it called, so the draw's present is not in the draw. The
registers and flags the function takes or gives (`eax`, `ecx`, `edx`, the x87 stack) are left as they were, and the
counters touch no vector register and no last error. A function left by an exception or a longjmp is dropped at the
next entry or exit and counted in `misses`. Nothing in Windows is hooked.

The counters live in a block of shared memory (`count.h`) that origrun reads from outside, so nothing in the game
waits for it. A script's `count <label>`, or `at <N> count <label>` at an update, writes one snapshot: a JSON line in
`build/origrun/<run>/counts.jsonl` with the update counter, the process's cycles (`QueryProcessCycleTime`, which count
the threads that have ended), the loop thread's cycles (the thread that ran the most counted time), each part's calls
and cycles, and every thread's cycles with its name and the module its start address is in. A snapshot labelled `exit`
is written by `quit` before it posts the close. Nothing is held, so a function that is running when a snapshot is taken
counts when it returns: a stretch between two snapshots is off by a part of one call at each end, a few parts in ten
thousand over a stretch of two thousand updates. `tools/cyclesplit.py` reports a file and sets a port's beside an
original's.

The cost: a thread cycle reading is a system call, made at each entry and each exit (two a counted call), and the stub's own work
is a few dozen instructions. A reading takes 500 to 860 cycles in a tight loop (the file's `counter.cycles_in_a_row`, measured when
the stand-in loads) but 4 to 8 thousand in the running game (`counter.cycles_each`: the time-stamp ticks the readings took, which
include whatever interrupted one), a system call after game code finding its kernel path cold. Measured as a whole, with counting
alternated against `count pause` in blocks of 200 updates of the same run (BookWorm's idle title screen, six runs, the loop
thread's cycles an update compared; `games/bookworm/tools/profile/toggleruns.sh`, `cycles.py toggle`), counting adds 0 to 1.6 %
there (the neighbouring blocks' median ratio 1.004; runs from -4 to +23 thousand cycles an update, mean 9.5 thousand), about 0.2 %
of the 3.9 to 4.8 million cycles an update of the like-for-like scenario. `count pause` and `count resume` (script commands, host
side) stop and start the counting: a function entered while paused is not counted, and costs only the stub's jump. Limits: the functions' first bytes must not be a jump target
from elsewhere; a game that reads its own code would see the jumps; not inside Windows Sandbox. `tools/origrun/counttest.sh`
(check.sh's origrun suite) tests it on `counttest.exe`, whose functions use `ret N`, return a 64-bit value, take `this` in
`ecx` and recurse.

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
| the user's visible desktop | Works with no wrapper. Since 2026-11-15 any game, port or original, may run there at any hour, with sound (CLAUDE.md, "Desktop and Sandbox at any hour"). Before that it needed the user's leave game by game: Bookworm Adventures' 3D comparison ("Yes, capture it", 2026-11-01) and Bejeweled Twist's ("Sandbox, then desktop", 2026-11-02: first `--sandbox --sandbox-vgpu --sandbox-connect`, and the desktop only if the original still falls back to 2D there), and Typer Shark's and Bejeweled 2's on the same terms ("Yes, both routes", 2026-11-11); from 2026-11-11 to 2026-11-15 any game could also run there between 22:00 and 08:00. |

The shared driver for it is `tools/visrun.py` (`docs/VISRUN.md`: a config and a step file per game; the game-local
drivers named below are what it replaces).

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
| bookworm | wrapped Steam exe | host (the retail 1.03 exe), virtual clock, short `--stage-root`; shots on the host through the in-game frame reader (trial passed, below the table) | has one | prepared as Bejeweled |
| bookwormadv | local install | sandbox: the pilot's menus and battle. Its 806×629 window has the client at (3,26); update-timed shots are exact with `--pending-draw [0x8259e4]+0x45a --update-state [0x8259e4]+0x488 --draw-empty [0x8259e4]+0x459` | off and on proven (on with `--dgvoodoo`) | its sound is not through DirectSound (its session is muted); it needs a short `--stage-root` |
| bookwormadv2 | Steam copy | virtual clock, scripted cursor, seed patch; its saves and registry values in the sandbox | has one; the sandbox has none (`Is3D=0`) | not wrapped; `Steam.dll` is copied in |
| cosmicbugs | local install | host, windowed, virtual clock, scripted cursor, two test patches (the seed, and a frame wait of exactly 10 ms). Sandbox shots by `--surface-screen` (5-5-5) | none | full screen it stops at "Screen Setup Failed" on the headless desktop, which cannot open its 800×600 mode; `--dgvoodoo` gives it a desktop-sized window and the title menu |
| dynomite | Steam copy | host, virtual clock, short `--stage-root` | has one | `Steam.dll` is copied from the Steam library root into the copy |
| hammerheads | Steam copy (`HammerHeadsGame.exe`) | host, virtual clock, the seed pinned (`games/hammerheads/tools/origrun/patches.txt`); each scenario adds its jump, unlock and cheat patches through `PATCHES` (`games/hammerheads/tools/idplay`, recipe in `games/hammerheads/docs/WORKING_NOTES.md`, "Phase one's runs") | | |
| mahjongchina | local install | sandbox: whole stages and the ending | has one; off (`HardwareAcceleration=0` by `--reg`) | as mahjongjapan; `games/mahjongchina/tools/sandbox_run.sh` does it all. Shots need `games/mahjongchina/tools/ddrawcap` beside the copy, or every one is black (its window class has a black background brush). Its players live in `%LOCALAPPDATA%\JollyBear\Mahjong Escape`. Never click the PopCap and SpinTop logos at the menu's bottom corners: they are links |
| mahjongjapan | local install | sandbox: whole stages to the ending | has one; off (`HardwareAcceleration=0` by `--reg`) | JollyBear engine, windowed DirectDraw (`FullScreen=0` by `--reg`). Posted clicks need a `move` 150 ms before. Its players live in `%LOCALAPPDATA%\JollyBear\Mahjong Escape Ancient Japan`, which inside is SYSTEM's: stage them under `sandbox-files/C/Windows/System32/config/systemprofile/AppData/Local/...`, re-signed for `C:\run\game\` (a player's checksum covers the exe's folder). Its hover reads `GetCursorPos`; `--cursor scripted` would answer it only in a copy that loads the stand-in, which its packed `bass.dll` does not (a coverage run's exe does); untested |
| mummymaze | local install | virtual clock, scripted cursor; shots on the host by the in-game frame reader (trial passed 2026-11-22: the tutorial's 16 screens, 16 matching the sandbox's, nothing ignored; host runs at about 19 times game speed) | none | in real time its window takes about 45 s to appear. Reader: `reader_surface=[[0x4ad0cc]+0x254]+0x14`, `reader_waits=4383da` (the Sleep at 0x4383d4 in the Process loop, after its draw branch) |
| ningpo | local install | `--sound nodriver`; shots in the sandbox, after its init | none | windowed DirectDraw. **With `--sound stub --clock virtual` it stops at 88.8 s of game time once a game is dealt**: the exe's Ogg music streamer, called by its software mixer (0x424fbd), spins in the stream read's end-of-stream branch (0x4253f7: the read gives 0, it seeks back to the loop point at stream+0x10 and reads again) holding the stream's lock, and the main thread waits on that lock in the stream's rewind (0x4253b4), so the clock stops. A lock longer than the buffer was ruled out. Not known: whether the wall clock does the same past 88.8 s of a dealt game, and why the read after the seek gives nothing |
| noahsark | local install | virtual clock | none | update counter `[0x4d125c]+0x368` |
| pixelus | local install | virtual clock, scripted cursor, seed patch | has one | BASS through COM. games.tsv's Pixelus.exe (about 256 KB) is a smaller build than the port's decompiled exe, and its counter offsets do not apply to it |
| rocketmania | Steam copy | host, virtual clock, scripted cursor, short `--stage-root` | has one | BASS through COM (the same `bass.dll` as Pixelus); `Steam.dll` is copied in |
| sevenseas | local install | sandbox | none | update counter `[0x4e21bc]+0x378` (`+0x368` is its frame time, so an `at` past 10 never fires); its unregistered copy fills a delay bar and takes a click from about update 1000; its "Submit to Internet High Score List" starts the browser (`ShellExecuteA`), which the launch guard fails and logs ("The launch guard") |
| tiptop | local install | host, virtual clock, `--cursor scripted` (it reads the pointer with `GetCursorPos`), short `--stage-root` | none | trial |
| typershark | local install | from a copy under `build/origrun/typershark/game` (`games/typershark/docs/GAME_NOTES.md`, section 8) | | its loading thread runs on the wall clock: gate the first input with `waitfor` |
| waterbugs | local install | sandbox, a fresh profile each run; shots by `--surface-screen` with `--surface-eip`; update counter 0x460a1c | none | full-screen DirectDraw, as Cosmic Bugs; `--input-eip` and a `mouse` patch for its input |
| wordharmony | local install | host, virtual clock, scripted cursor, short `--stage-root`; its registry key is cleared before each run | none | trial (60 min) |
| zumasrevenge | wrapped Steam exe | host, virtual clock (`--counter '[0x9fc740]+0x4c4' --update-ms 10`), from a copy staged under `--stage-root 'E:\byo-og'` with the game exe (`re/zumasrevenge/bin/ZumasRevengeGame.exe`) beside the wrapper and run with `--exe ZumasRevengeGame.exe`; no Steam, nothing patched, no network or launch calls, sound muted | Direct3D only | the window is 1600x1200 (its Hi-Res art), so clicks are in those pixels; sandbox shots come out flat (Direct3D is not reachable by GDI), so peeks only until a visible-desktop run; it writes its saves to `C:\ProgramData\Steam\ZumasRevenge`, which a run must move aside and leave absent as it was; a fresh profile into Jungle1: click the loading screen, `waitfor [0x9fc740]+0x934 != 0` (the name dialog), click (805,245), type a name, key 13, wait for that value to be 0 and for `0x9fc3a0 < 1`, click once (the map's pan), then about 900 updates later click (1300,600) twice (the jungle temple) |

**The in-game frame reader's trials.** Bookworm Deluxe (the carved 1.03 `BookwormGame.exe`, windowed, 2D) passed
with the reader of 6b5f6492 on 2026-11-22 (its config line: `reader_trial=passed 2026-11-22 6b5f6492`), with
`reader_surface=[[0x5347d4]+0x334]+0x3c` (the app at 0x5347d4, its DDInterface at +0x334, `mDrawSurface` at +0x3c: caps 0x840, an off-screen plain surface in system memory, 640x480 at 32 bits) and
`reader_waits=480927` (the return from the `Sleep` at 0x480921 in `Process`'s branch that drew). The sandbox side
was the Bookworm session's run `bwsb-menus` of 2026-11-08, with its script, seed 1 and 23-patch list; its shots were
copied, not taken again. Its 37 screens are the title, a new user, the mode selector and its hovers, the hall of fame,
the users dialog, the quit question, the help pages, the board, the options, the credits, the leave question
and a new game. The host run through the reader took all 37 at the same updates (3008 to 5158), refused none, and
every pair is identical pixel for pixel at the client offset (3,26); shotdiff gave 37 matching, 0 differing, 0 faint.
A first try on the session's earlier run `bwsb` (the loading screen, updates 10 to 260) is not a trial of the
reader: its first two screens matched, and the other six differed only in the loading bar and the "click here to
play" line, because the loader thread runs on the wall clock (the virtual clock lets the main thread's waits take
real time only while it works), so how far it has loaded at a given update differs from run to run. Every other
pixel of those six was identical. In the same game the primary surface given as `reader_surface` was refused
(caps 0x8A00), and a `reader_waits` that is never reached refused both shots (the counter had moved past their
update); neither wrote a PNG. Not tried: 3D mode, whose draw surface is a Direct3D device surface in video memory,
and full screen. A session that needs either runs a trial of that mode first.

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

- **The launch guard has gaps.** It fails `ShellExecute`, `ShellExecuteEx`, `WinExec` and `CreateProcess` (a "More Games"
  `steam://` link, a browser), not a COM launch, a launch through `CreateProcessAsUser` and its kin or a direct system call
  ("The launch guard"), and a game that never loads the stand-in has none. The rule at the top, that a script never
  clicks a link button, still stands.
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
