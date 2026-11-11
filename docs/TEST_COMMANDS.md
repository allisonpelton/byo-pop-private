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
