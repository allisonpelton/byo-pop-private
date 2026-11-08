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
site's (`docs/WEB_PLAYER.md`).

Natively the option is `-commands=<file>`: the file may come after the game starts, a line still being written waits
for its end of line, and the run is a test run. The framework answers its own commands on the live channel (`OK click
320 450`, `ERROR click 10: click needs <x> <y>`, `ERROR <command>: unknown command` for one nobody knows) and flushes
the log line by line, so a driver can wait for each answer before it sends the next line; given with `-realinput` (an
empty file will do) the lines take its dispatch, so the driver's moves are real SDL events through the same file
(`docs/REAL_INPUT.md`). `tools/fwtest/commands.sh` is a driver of that kind. On the web the exported function is
`byopop_test_command(line)`: it returns 1 when the line is queued and 0 when it is refused (no game running, no command
in the line, too many waiting), and the line runs inside the next update, so a page that holds the game's updates
steps one to run it. The answers are the ordinary log lines, which SDL's log starts with `INFO: `.
The full description is with `-script`'s in `framework/SexyAppBase.h`.
