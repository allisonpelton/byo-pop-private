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
moves a driver makes are real input, not script clicks: DOM events on the web, burst-posted messages natively
(CLAUDE.md, "Done means identical").

## Sending them

A `-script` file runs a command at a fixed update, so a run that must repeat exactly (a comparison, a frozen
baseline) uses one. A driver that decides as it goes sends them live: natively with `-commands <file>`, whose new
lines (`<command> [args]`, no update count) the framework reads between updates and runs at the next one, and on the
web through the exported function the player-only test page calls (`byopop.test.command(line)`, which resolves with
the reply lines). Both go through the same dispatch as `-script`: the framework's commands first, then the game's
`HandleScriptCommand`. The live channel is the framework's (the shared session builds it); the page's call is the
site's (`docs/WEB_PLAYER.md`). The shared loop that plays a game by `move` is also theirs: natively a tool that runs
the game under rundesk with `-commands` and sends each move as posted input, and on the web a driver the test page
runs (`?driver=`).
