# Standard test commands

The user, 2026-11-07, asked whether debug mods would speed up testing and could later become cheat menus, and chose
this plan ("Yes, add them"): every port gets the same set of state commands for tests, now, and in the mod phase each
game's cheat menu (`docs/MOD_ROADMAP.md`, standard mod 23, "Developer leftovers") is built on the same commands.

## What they are for

A driver script (CLAUDE.md, "Done means identical": sessions never play a game themselves) reaches the state it needs
in one command instead of playing up to it: a level, a boss, a locked minigame, a nearly finished board. It reads the
game's state back the same way. The moves it then makes still go through real input.

## Where they live

They are script commands, handled by the game's `HandleScriptCommand` override (`framework/SexyAppBase.h`, the
`-script` list). They are reachable only from a test: `-script <file>` natively, and on the web the player-only test
page's command call (`docs/WEB_PLAYER.md`). A normal run never reaches them, so unmodded play stays identical: a
command that is never given changes nothing, draws no random number and moves no state. A command that is given may
change anything, and a comparison with the original never uses one inside the stretch it compares.

## The set

Each game implements the ones that fit it, under these names, and lists them in its notes with the state keys it
prints. Every command is answered on the log (stdout) with one line: `STATE {...}` for `state`, `OK <command>` when it
has done its work, and `ERROR <command>: <reason>` when the game cannot do it (never silence). Games that already have
test hooks of their own keep them and add these names beside them, sharing the code where it does the same thing.

| Command | Does |
|---|---|
| `state` | prints the game's state as one JSON line, `STATE {...}`, with the keys that apply: `screen`, `mode`, `level`, `score`, `lives`, `health`, `enemy`, `timer`, `moves`, `coins`, and a `board` summary the driver can read (letters, gems, tiles) |
| `goto <screen>` | opens a screen directly through the game's own code: `title`, `menu`, `options`, `help`, `highscores`, `credits`, or a mode's name to start it |
| `level <n>` | starts level, stage or chapter `n` of the current mode, as the game itself would start it |
| `unlock all` | unlocks every mode, level, minigame and item in the current profile, as the game's own unlocks would |
| `set <name> <value>` | sets one of the `state` keys (`score 90000`, `enemy 1`, `timer 5`) |
| `win`, `lose` | ends the current level or game now, as a win or a loss, through the game's own ending |
| `seed <n>` | seeds the game's random numbers, where the game draws them from one generator |
| `move` | prints the move the game's bot would make next, `MOVE {...}`: what it is (the swap, the word, the tile, the shot) and `input`, the real input that makes it, as an ordered list of steps in game coordinates (below); or `MOVE null` when there is none; it changes nothing |
| `memreport [file]` | prints what the game holds in memory as one JSON line, `MEMREPORT {...}` (below); with a file it also writes the detail there (a SexyAppFramework port: every picture with its copies, and the heaps). It changes nothing |
| `cyclesplit [label]` | with `-cyclesplit=<file>`: a snapshot of the cycle counters, kept in memory and written with the file at exit ("The cycle split", below); `cyclesplit pause` and `cyclesplit resume` stop and start the readings; an error without the option |

`memreport` answers with the same line in every game, so that a game's memory splits the same way everywhere:
`{"images":{"count","pixels","copies","textures"},"sounds":{"count","bytes"},"music":{"count","tracks","moduleFiles"},`
`"others":{...},"process":{"workingSet","private","privateAtStart"}}`, every size in bytes. `pixels` is the pictures' own
pixels and palettes, `copies` the second copies made from them (the blitters' alpha caches, high-resolution pixels), and
`textures` an estimate of what the graphics device holds for them, which no process can read. `tracks` is what the mixer
keeps for audio tracks and `moduleFiles` the size of the module files loaded (a module's player keeps its own copy, about
that size or more). `privateAtStart` is the process's private bytes when its libraries were loaded and its window,
graphics device and sound device were up, before it loaded a picture, a sound or a song. From these a reader gets four
parts: libraries and devices (`privateAtStart`), graphics (`pixels` + `copies`), sound (`sounds.bytes` + the music), and the
game's own state (`private` less the other three; `others` names what an engine can account for in it).

A SexyAppFramework port answers it in the framework (`SexyAppBase::DoMemReport`, here and in `-script`), with nothing to
write in the game; a game with large holders of its own adds them in `AddMemReportHolders`. An engine of its own (the
Mahjong Escapes, Ning Po, Cosmic Bugs and Water Bugs, Bonnie's, Word Harmony) uses `platform/MemReport.h`: it calls
`Platform::NoteStartMemory()` once its window and devices are up and before it reads a game file, fills a
`Platform::MemReport` in its own `memreport` command with what its pictures and sounds hold
(`Platform::AddSongMemory` adds the mixer's songs; `mOthers` takes any other holder by name), and answers with
`Platform::LogMemReport`, which adds the process's numbers and prints the line.

`tools/memmap.py <pid>` is the same question asked from outside, of any running game (a port, or an original under origrun):
it reads the process's address space and totals it by kind (the libraries by module, mapped files, stacks, private memory, the
part of it in the heaps' first allocations) beside the process's own counters. It reads only, injects nothing and needs no test
command in the game. It tells what the process holds in libraries and the driver against its own allocations, which `memreport`
cannot see (`games/alchemy/docs/GAME_NOTES.md`, "The software path", has one on the default path, the software renderer and the
original). `tools/scaletest` measures the cost and the picture of scaling a finished frame to the window.

A move's `input` steps, played in order (Twist's and Alchemy's drivers set the form, 2026-11-07): `{"move":[x,y]}` moves
the pointer there (a hover: a piece that hangs from the pointer needs one before its click), `{"click":[x,y]}` clicks
there, with `"button":"right"` for the right button, `{"press":[x,y]}` and `{"release":[x,y]}` hold and let go for a
drag (the `move` steps between them carry it), `{"key":"B"}` presses and releases a key, and `{"wait":n}` waits n
updates. A port that printed `clicks` or `drags` lists earlier moves over to `input`.

## The cycle split

Where a run's processor time goes, split the same way in a port and in its original, so that a game's code is compared part by
part and drawing, which is most of every game's time, does not hide a gain in the rest (the user, 2026-11-12: "separate out the
cost of frame drawing for performance comparisons ... so I can see the significant speedups from better code more plainly").
The target of `docs/COORDINATION.md` stays on the total; the split is reported beside it.

**The parts**, counted in cycles of the thread that ran the game loop (`QueryThreadCycleTime`, so the time a thread waited is not
in it), each exclusive of the counted parts it calls:

| Part | The port (SexyAppBase) | The original (SexyApp; addresses in each game's export) |
|---|---|---|
| update (game logic) | `DoUpdateFrames`: the script, the widgets' updates, the music update | the same function, vtable slot 8 (BookWorm 0x47e220) |
| draw | `DrawDirtyStuff` less the present inside it: the dirty scan, `BeginFrame`, `DrawScreen` | the same function, slot 65 (0x486510) less its `Redraw` |
| present | `Redraw`: `DDInterface::Present` (SDL_RenderPresent, the cursor) | the same function, slot 66 (0x4884f0), which ends in `DDInterface::Redraw`'s Blt or Flip |

Every other thread is listed with its cycles, its name (`SetThreadDescription`) and the module its start address is in, and the
process's total (`QueryProcessCycleTime`, which counts the threads that ended too) closes the account: what the loop thread did
outside the three parts is "loop" (events, waking from Sleep, the snapshots and the cycle readings themselves), and the other
threads are "driver" (a module of the graphics driver or runtime), "audio" (SDL's audio thread, BASS, FMOD, DirectSound) and
"other". `tools/cyclesplit.py` does the sums.

**The limit**, to be stated wherever a split is shown: a GPU renderer batches its draw calls until the present, and DirectDraw can
blit asynchronously, so a side that draws through one has its drawing partly in `present` and in the driver's threads. Such a side
is reported as "picture" (draw, present and driver threads together) and never split; the three are split only for a side that
draws in software (the 2D software renderer, the original's own blitters).

**The port.** `-cyclesplit=<file>` (native only; the web build has no thread cycle counters and compiles it out): a test of one
flag at the three sites when off, and when on a cycle reading (a system call) at each boundary: two for each part a call of it
makes, so about four an update for BookWorm (an update and a draw, a present now and then), and nothing else during the run. The
script (or live) command
`cyclesplit [label]` takes a snapshot in memory (a thread listing, a millisecond or two, which falls in the next stretch's "loop"), and
`cyclesplit pause` and `cyclesplit resume` stop and start the readings (the sites then cost the test of one flag, as without the option);
the file is written with one more snapshot, labelled `exit`, when the loop ends.

**What counting costs** (BookWorm, measured, 2026-11-12). One reading takes 500 to 860 cycles in a tight loop (the file's
`counter.cycles_in_a_row`, measured at the start) but 3 to 13 thousand in the running game (`counter.cycles_each`: the time-stamp
ticks the readings took, a mean that includes whatever interrupted one), since a system call made after game code finds its
kernel path cold. The two sums disagree by a factor of ten, so the cost was measured as a whole: the same run alternates counting
and `cyclesplit pause` in blocks of 200 updates on the idle title screen (the machine's noise, about a quarter run to run, falls
on both), and the loop thread's cycles an update are compared (`games/bookworm/tools/profile/toggleruns.sh`, then `cycles.py
toggle`). Twelve runs of the port: counting adds 7 to 23 thousand cycles an update on average (a run's figure from -10 to 37
thousand), about four readings at 2 to 5 thousand; against the like-for-like scenario's 2.0 million an update that is 0.4 to 1.1 %
of the port's total, under what moves a run's total from one run to the next. A split states it beside its numbers, and it is no
reason to run a measurement of the total with counting on: the target's figure comes from a run without it. The code is
`platform/CycleSplit.{h,cpp}`. The option is not a test-run flag by itself: a measurement is a `-script` run (no `-commands`,
whose poll costs 340 thousand cycles an update).

**The file**, the same from the port and from origrun's `--count` (`docs/ORIGRUN.md`): one JSON object a snapshot, a line each,

    {"label":"a","update":200,"wall_s":3.93,"process_cycles":...,"main_thread_id":...,"main_thread_cycles":...,
     "parts":{"update":{"calls":..,"cycles":..},"draw":{...},"present":{...}},
     "counter":{"calls":..,"cycles_each":..,"cycles_in_a_row":..},"misses":..,
     "threads":[{"tid":..,"name":"SDLAudioP2","start":"sdl2.dll+0x11f810","cycles":..}, ...]}

`threads` holds every thread but the loop's; `counter` is the readings made and what one cost, in the running game and in a tight
loop (see "What counting costs"), so that a reader can take them out;
`misses` counts a part that was left unclosed or nested deeper than eight (a port: never; the original: a longjmp or an exception
that skipped a return). `python -I tools/cyclesplit.py <file> [<the original's file>] --from <label> --to <label>` prints a stretch's
parts in million cycles an update and as a share of a core, and the two sides side by side.

**Picking the functions** (a game that is not a SexyApp game, or whose original differs from BookWorm's). The three parts are
roles, and SexyApp's functions above are the standard case: a game on another engine (Retro64, the Mahjong engine, Venice's)
maps each role to its own functions, the same ones on both sides.

- *update* is everything that moves the game one step: the function the loop calls once a tick, with the logic, the widgets'
  or scenes' updates and the music's tick below it, and nothing that draws. *draw* is the game's own code that builds a frame's
  picture, up to the hand-off to the display (the dirty scan, the begin-frame, the screen's draw). *present* is the hand-off
  itself: the call that uploads, flips or swaps (SDL_RenderPresent, a Blt or Flip, a swap buffers) and what the game does
  around it (the cursor). Audio is not a part: it is the other threads, found by name.
- Where a role has several functions, pick the outermost that holds the whole step, never a callee of it, since a part's
  cycles are its own less the counted parts it calls; a present inside the draw (BookWorm's `DrawDirtyStuff` calls `Redraw`) is
  fine and is taken out of the draw. Do not count the same function under two labels.
- *The port:* put `Platform::CycleScope aScope(Platform::CYCLE_UPDATE)` (or `CYCLE_DRAW`, `CYCLE_PRESENT`) at the top of the
  function that plays the role, in the engine's code, once each, on the loop's thread (`platform/CycleSplit.h`). A SexyApp game
  has them in the framework already and adds nothing.
- *The original:* find the three functions in the game's export (the loop's tick, the frame's draw, the present), and check
  them against the game before listing them: the update counter says how many updates ran (`--count` shows each function's
  calls, so update calls equal the counter, draw is about one a drawn frame, present no more than draw). `python tools/origrun/prologue.py
  <exe> update=0x<va> draw=0x<va> present=0x<va>` prints each function's `len=` (the first whole instructions, five bytes or
  more, with no relative jump or call among them), and the three lines go in `games/<game>/tools/profile/count.txt`: a label,
  an address and a length, nothing of the game's bytes, which `--count` reads out of the exe (`docs/ORIGRUN.md`, "Counting
  cycles inside the original's functions"; BookWorm's file and `cycles.py` with `PERF_SPLIT=1` are the worked example).
- *Checks before a split is reported:* the counted parts are most of the loop thread's cycles (the rest is "loop": events and
  waking from Sleep), `misses` is 0, and the file's `counter` line says what the readings cost, which a split reports beside its
  numbers. A game whose GPU renderer batches its calls reports "picture" (draw, present and the driver's threads) and not the
  three, as above.

**The window line.** A run says what it measured in: `display: client 1280x960 px, logical 640x480, render scale 2, windowed,
renderer direct3d11` is logged (SDL_Log, so in the run's log) when the display is made and again at every size or mode change
(a line that repeats the last is not logged again). The client size is the drawable's in pixels, the render scale the
framework's (a GPU display has 1), the renderer SDL's name for it or a GPU display's description. The port's window is 2x the
game's own size when the desktop has room, and `-windowscale=1` makes it the game's own, as the original's (`-windowscale=N`
for N times the logical size; `-renderscale=N` is the render target's size, a separate setting).

## The bot behind `move`

The user, 2026-11-07: "If it would save usage, the scripts that play the games should be developed in a way that
their code can be reused for a mod that adds a bot." So the move choice lives in the port's own code, not in the
driver script: most ports already have an autoplay bot or a solver (Bejeweled, Twist, Big Money, BookWorm, Pixelus,
Mummy Maze, Rocket Mania, TipTop, Alchemy, Atomica, Noah's Ark, Seven Seas), and `move` answers from that same code.
A driver is then a thin loop (ask `state` and `move`, send the move as real input, wait until the game has taken it),
one shared loop can serve every game, and the bot mod (`docs/MOD_ROADMAP.md`, standard mod 11) later reuses the bot
unchanged. The bot reads the game's state and never moves it: it draws from its own generator, never the game's, so
asking for a move changes nothing. A game without a bot gets one in the port where its driver needs one; a driver
that already has a solver of its own keeps it, since rewriting a working driver costs more than it saves.

The framework's own commands (`click`, `key`, `focus lose|regain`, `screenshot`, `modmenu ...`) stay as they are. The
moves a driver makes are real input, not script clicks: DOM events on the web, `-realinput` natively (its events
through SDL's own queue, `docs/REAL_INPUT.md`; posted messages reach only clicks and keys there; CLAUDE.md, "Done
means identical").

## Sending them

A `-script` file runs a command at a fixed update, so a run that must repeat exactly (a comparison, a frozen
baseline) uses one. A driver that decides as it goes sends them live: natively with `-commands <file>`, whose new
lines (`<command> [args]`, no update count) the framework reads between updates and runs at the next one, and on the
web through the exported function the player-only test page calls (`byopop.test.command(line)`, which resolves with
the reply lines). Both go through the same dispatch as `-script`: the framework's commands first, then the game's
`HandleScriptCommand`. The live channel is the framework's (the shared session builds it); the page's call is the
site's (`docs/WEB_PLAYER.md`). The shared loop that plays a game by `move` is also theirs: natively
`tools/playbymove` (below), which runs the game under rundesk with `-commands` and sends each move as real input
(`-realinput` lines through the same file), and on the web a driver the test page runs (`?driver=move`).

Natively the option is `-commands=<file>`: the file may come after the game starts, a line still being written waits
for its end of line, and the run is a test run. The framework answers its own commands on the live channel (`OK click
320 450`, `ERROR click 10: click needs <x> <y>`, `ERROR <command>: unknown command` for one nobody knows) and flushes
the log line by line, so a driver can wait for each answer before it sends the next line; given with `-realinput` (an
empty file will do) the lines take its dispatch, so the driver's moves are real SDL events through the same file
(`docs/REAL_INPUT.md`). `tools/fwtest/commands.sh` is a driver of that kind. A game's command may wait for its moment by
queuing itself again for a later update (`AddScriptCommand`, as Bookworm Adventures' `word` does while its board is
busy). Real input still reaches the game meanwhile: `HandleEvent` drops it only in a `-script` run or while a game's
test bot drives (`mScriptedInput`). fwtest's `waitdown` checks it in `commands.sh`. On the web the exported
function is `byopop_test_command(line)`: it returns 1 when the line is queued and 0 when it is refused (no game running,
no command in the line, too many waiting), and the line runs inside the next update, so a page that holds the game's
updates steps one to run it. The answers are the ordinary log lines, which SDL's log starts with `INFO: `. The channel has one
command of its own, `wait <n>`: the lines after it wait n updates, and it answers `OK wait <n>` when they are up. The
framework also answers `pointer`, here and in `-script`, with `POINTER {"x":..,"y":..,"in":true|false}`: the mouse as the
widgets last saw it, in game pixels (the page check's resize line, `docs/WEB_PLAYER.md`, "The page check"). The
full description is with `-script`'s in `framework/SexyAppBase.h`.

## The native loop

`tools/playbymove/playbymove.py [options] <game exe> [game arguments]` is the shared native loop, the twin of the page's
`?driver=move` (`web/testkit.js`, `playByMove`). It starts the game under rundesk with `-commands`, an empty `-realinput`
file, `-hold` and `-fast`, then asks `state` and `move`, plays the move's `input` as `-realinput` lines in game
coordinates, waits its `updates`, and asks again, until the state says `"over": true`, a step limit (`--steps`, 2000),
`MOVE null` twice running (30 updates apart), an `ERROR`, or the same move on the same state 50 times running.
`--first <line>` sends a command before the loop (`goto`, `level`, `seed`, `set`), and `--shot` takes a screenshot at
the end. Its last line is the summary (`playbymove <game>: <n> moves, stop over; STATE {...}`), its run folder
(`build/playbymove/<game>` by default) holds the game's log and `driver.txt`, every line sent and its answer. A session's
native driver is a call to it, with the game's own arguments.

`-hold` is what makes it repeatable: once the game has loaded, an update runs only when the driver's lines let it (each
new batch runs one, `wait <n>` runs n), so a game under `-fast` never runs on while the driver decides, the bot's move
always fits the state it was asked on, and the same game plays the same way every run (`tools/check.sh unit` plays
fwtest's small task twice and compares the transcripts). A move's steps go one update apart, as a hand's would, and a
whole move is one batch: the steps with `wait 1` between them, the move's wait, and the next `state` and `move`. A key
step `{"key":"B"}` is typed as that character (`type B`); a DOM key name (`Enter`, `Escape`, `ArrowLeft`, `" "`) becomes
its SDL key; any other name is taken as an SDL name. Under `-realinput` a bare `move` is the game's test command, as in
`-script`; `move <point>` moves the mouse.
