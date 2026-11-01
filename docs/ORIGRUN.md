# origrun: running the original games for comparison

Every port under `games/<game>/` must play identically to the user's original executable. Before a port's
final merge its session compares the port with the original screen by screen, at the same points — every
screen, dialog and text, which music and sounds play, and what happens when the window loses and regains the
focus (CLAUDE.md, "Done means identical"). `tools/origrun` runs an original so that comparison can be made
without disturbing the user, who shares the machine.

It runs a **copy** of the original (never the install), invisibly, and by default silently, and it captures the
screen, drives scripted input, and changes the focus on command.

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

`--game <name>` looks the install up in `tools/origrun/games.tsv` (name, install folder, exe leaf) and copies
it into `build/origrun/<run>/game`. `--exe` runs a copy you placed yourself (used for a game whose real exe
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
- `waitshot <name> [timeout_ms]` — wait for the window, then capture.
- `record <prefix> <duration_ms> <interval_ms>` — capture a numbered frame every interval for the duration,
  whenever a window is present. Good for a screen that appears at an unpredictable time.
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
4. **Screens.** `shot`/`waitshot`/`record` capture the window with `PrintWindow` (a GDI blit as a fallback) and
   write a PNG. This works for windowed games (the SAF games run windowed by default). A game that takes the
   screen full-screen through DirectDraw/Direct3D renders outside GDI, so the capture is black — see Gaps.
5. **Scripted input and focus.** Proven end to end: clicks, typed text and `focus lose`/`regain` all register
   (a test window showed the click count rise, the text appear, and the active flag and colour change with the
   focus), and on a real game the trial dialog's Play button is clicked and the game starts.
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

## Per-game status

The 18 games of `BYOPOP_WEB_FINISHED`, as of the first runs on 2026-10-31. "Trial" means the copy shows
PopCap's trial screen first; its **Play** button is clicked through with `clickready` (it is not a hard lock
while plays/time remain). Captures are the main window's client size. These are first findings, not a finished
comparison; the per-game session does the screen-by-screen pass.

| Game | Source | Reached under origrun | Capture | Audio seen | Notes |
|------|--------|-----------------------|---------|------------|-------|
| alchemy | local install | trial, then into the game past Play | yes | muted a session | some sound is not via DirectSound |
| atomica | local install | no window within 100 s | — | none | slow/odd start; needs a longer look |
| mummymaze | local install | title after a slow (~45 s) start | yes, 646×509 | none | window is late to appear |
| noahsark | local install | own title screen | yes, 646×509 | none | |
| sevenseas | local install | own title screen | yes, 646×509 | none | |
| pixelus | local install | trial screen (not clicked through yet) | yes | none | |
| tiptop | local install | trial, plays | yes | none | DirectInput: posted clicks do not drive it |
| wordharmony | local install | trial (60 min), Play present | yes | none | |
| bookwormadv | local install | in-game Updates dialog | yes, 806×629 | muted a session | its sound is not via DirectSound (BWA audio work) |
| ningpo | local install | main window | captured but blank (DirectDraw) | none | windowed DirectDraw, outside GDI |
| waterbugs | local install | window, then full screen | blank/black | none | DirectDraw then full-screen |
| cosmicbugs | local install | full screen | black | none | full-screen, outside GDI |
| dynomite | Steam copy | "Unable to load Steam.dll" box | — | n/a | needs its real exe prepared (private note) |
| bejeweled | wrapped launcher | not run | — | — | needs its real exe extracted first (private note) |
| bigmoney | wrapped launcher | not run | — | — | needs its real exe extracted first (private note) |
| bookworm | wrapped Steam exe | not run | — | — | needs its real exe prepared first (private note) |
| rocketmania | wrapped launcher | not run | — | — | needs its real exe extracted first (private note) |
| bookwormadv2 | Steam copy | not run | — | — | not wrapped; expect the Steam.dll box |

The runs above (except the dialog test) were made before the message-box watch landed, so their logs predate
it; the watch is proven separately (guarantee 6). The install paths are in `tools/origrun/games.tsv`, which is
kept out of any publish (`.publishignore`).

<!-- private: Preparing a wrapped or Steam copy to run under origrun is recorded on this machine only. -->
Preparing a wrapped or Steam copy to run under origrun:
- PopCap's Steam retail launchers wrap the real game. The wrapper is detected by the marker `!popcapdrmprotect!`.
  `WinBej.exe` (Bejeweled), `WinBM.exe` (Big Money), and the Rocket Mania, Dynomite and BookWorm Deluxe Steam
  exes carry it; `BigMoney.exe` and `BookwormAdventuresVol2.exe` do not and were not run.
- Extract the real PE with `tools/extract_game_exe.py -o <copy>\<wrapper>.exe` (which `.publishignore` keeps out
  of any publish). Stage a copy first with `origrun --game <name> --copy-only`, carve inside that copy, then run
  it with `origrun --exe "<copy>\<real>.exe"`.
- A Steam copy also wants `Steam.dll` (or a stand-in) beside it, or it shows the "Unable to load Steam.dll" box.
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

- **Full-screen DirectDraw/Direct3D capture is black, and windowed DirectDraw captures blank** (NingPo, Water
  Bugs, Cosmic Bugs). Those games draw outside GDI, so `PrintWindow`/GDI grabs nothing. Reaching their pixels
  needs a different capture path (a display stand-in, or forcing windowed GDI-friendly output where a game
  offers it); not attempted yet.
- **DirectInput games** (e.g. TipTop) read the mouse from the device, not from window messages, so posted
  clicks do not drive them. Capture and focus still work; driving them needs a different input path.
- **Wrapped and Steam copies** are not run yet. A wrapped launcher's real executable must be prepared in the
  copy first, and a Steam copy wants its `Steam.dll` beside it; the specifics are in the private note under
  Per-game status. Use `--copy-only` to stage the copy, prepare it, then run it with `--exe`.
- **Audio not carried by DirectSound** (some Alchemy and Bookworm Adventures sound) is not silenced by the
  `dsound` stand-in. The Core Audio watch mutes the game's session and reports it; for a run that must be truly
  silent, use `--sound real` with the speakers off, or rely on the watch's mute.
