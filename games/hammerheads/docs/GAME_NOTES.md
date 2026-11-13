# Hammer Heads Deluxe: game notes

What the port does differently from the original, what it leaves out, and what is odd. The analysis state is in
`WORKING_NOTES.md`; the code is the translation of `HammerHeadsGame.exe` (PopCap, 2010, SexyAppFramework 1.2 layout, the same
as Big Money's), written to `docs/CODE_STANDARDS.md` from its first draft.

## Test commands

Reachable only from a test (`-script`, `-commands`, the web page's test call; `docs/TEST_COMMANDS.md`). Replies go to stdout:
`STATE {json}`, `OK <command>`, `ERROR <command>: <reason>`. A script line is `<update> <command>`.

| command | what it does |
| --- | --- |
| `state` | `{screen, zoom, mode, level, score, health, coins, timer, phase, paused, dialogs, enemy, gnomes}`; `gnomes` lists each cubby with a gnome: `{cubby, kind, state, cubbyState, rise, health, x, y}` |
| `goto <screen>` | `menu`, `highscores`, `trophies`, `shop`, `challenge`, or a mode: `classic`, `tough`, `marathon` |
| `level <n>` | starts level n of the current mode over the board there |
| `unlock all` | Tough Cookie is unlocked |
| `set score\|health\|coins\|level <v>` | the score, health, coins or level of the game in play |
| `win`, `lose` | the level in play goes to its end at once (the pause after the last cubby), or the health runs out |
| `seed <n>` | seeds the game's random numbers the way a new game does |
| `move` | `MOVE {"kind":"smash","cubby":i,"input":[...]}` for the bot's next swing (the first cubby with a gnome up that a hammer can hit and that is not a bomb), or `MOVE null` |
| `memreport` | the framework's, `MEMREPORT {...}` |

The bot reads the game's state and draws no random number. Its swing is a `move` to the gnome's strike point, two updates' wait,
a `click` and ten updates' wait, which is what a player's click does.

## Deviations (each without a visible effect)

- **The refresh rate is fixed at 60.** The original reads the display's rate (60 under its test runs) and runs its logic once a
  refresh in `UpdateF(theFrac)`, with `theFrac = 1000 / (10 * rate)`; the port fixes `mSyncRefreshRate` at 60 so that every machine
  takes the original's 60 Hz steps (the framework's own default is 100).
- **XML.** The player roster, the saved games and the settings are read and written with `XmlLite`, a small reader and writer; the
  original uses TinyXML, which is not available. The files keep the original's element names and order.
- **How to play.** The original shows its how-to-play pages through a Flash control (`Flash.ocx`, `drm.swf`'s sibling); the port
  has no SWF player yet, and the widget does nothing until the shared one exists.

## Left out (nothing in play can reach it)

- The level **editor**: the original's GameScreen is a tool widget with the game on top; the port keeps the widget and none of the
  editor.
- The **DRM client** (a window broadcast to the Steam wrapper, a three-second wait at start), the **beta tools** object at
  Application +0x77c (a login and update check, 0x41a000-0x41bac0) and its menu items.
- **Registration**: the demo limit, the level-allowed check (0x41e520, always true), the upsell dialog, the buy and register
  buttons and every string that only they used. **More Games**, the Steam link and every other button that leaves the game.

## Fix candidates

(none listed yet)

## Oddities (not chased)

(none listed yet)
