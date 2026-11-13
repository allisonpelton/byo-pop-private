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
- **How to play.** The original shows its how-to-play movie (`images\howtoplay.swf`, 800 by 500, 24 frames a second, looping)
  through a Flash control in "Transparent" window mode; the port plays it through the shared SWF player (`engines/swf`), which
  draws the movie's own background (a green), not what is behind it. The shot comparison must judge whether the page differs.
- **Game text kept out of the repository.** The default high score names and the trophy names and descriptions are tables built by
  the exe's static initialisers; `tools/highscorenames.py` and `tools/trophytext.py` read them from the exe and write the
  git-ignored `src/HighScoreNames.inc` and `src/TrophyText.inc`, which the build includes.
- **The loading screen** no longer moves on by itself, as the stand-in did; its co-branding Flash widget is not made.
- **The desktop's size** (Application +0x70c and +0x710), which only the options box reads (a full screen that is turned off must
  leave a window that fits), is asked of Windows in the original before the display is made (`GetSystemMetrics`); the port reads
  SDL's usable bounds right after the framework has made the display.
- **The options box's tests of the card.** The original asks its 3D tester twice (the card can run 3D; the card is one it
  recommends); the framework's two answers are one in the port (the renderer's `mCan3D`), so the "Warning" notice (3D works but is
  not recommended) never comes. `mForceFullscreen` (a desktop that is not in 16 or 32 bit colour) is never set, so the notice about
  it, and the full screen box that cannot be cleared then, cannot come either.
- **Debugger lines.** The original writes lines to the debugger (`OutputDebugStringA`) where the options box pauses and un-pauses
  the game, where the leave box does, and in the credits' Flash listener; the port writes none.
- **The trophy page buttons** (the announcement of a new trophy and the close-up) are one class here, `TrophyPageButton`; the
  original has two classes with the same code and fields. A trophy's shadow picture (the unearned ones) is deleted with its
  button; the original leaks it (a memory leak, fixed here as the rules say).

## Left out (nothing in play can reach it)

- The level **editor**: the original's GameScreen is a tool widget with the game on top; the port keeps the widget and none of the
  editor.
- The **DRM client** (a window broadcast to the Steam wrapper, a three-second wait at start), the **beta tools** object at
  Application +0x77c (a login and update check, 0x41a000-0x41bac0) and its menu items.
- **Registration**: the demo limit, the level-allowed check (0x41e520, always true), the upsell dialog, the buy and register
  buttons and every string that only they used. **More Games**, the Steam link and every other button that leaves the game.
- **State that is written and never read.** The trophy room's fade (+0xb0, set to 3.0 and counted down), its hover alpha (+0xa8)
  and trophy count, the trophy button's +0x150 flag, an unused "Click to continue" string; the game-won page's counter
  (DAT_00598084, one more while a page exists); none of it is in the port.
- **The resurrect page's flag.** `ResurrectScreenWidget` takes a flag whose only call (0x422a72) passes false; with it set the
  page has a "Back to Game" button and two lines about a new savepoint. That page, its button, the pointer at
  +0x84 (a button that nothing creates) and `Application::ResurrectBackToGame` are not in the port, and `ResurrectOrPlay` takes no
  argument.
- **The credits' Flash widget.** `CreditsDialog` holds a pointer to a Flash widget (+0x170) that nothing ever makes; it is also a
  `FlashListener` whose two functions (0x44f300, 0x44f370) only write to the debugger. The code that places and adds the widget,
  and the listener, are not in the port. The options box and the quit box have no registration or outbound button, so nothing was
  taken out of them.

## Fix candidates

Behaviour of the original that looks like a mistake and is kept in the port until the fixes mod.

- **The coin count on a restart point's card** is measured in the font of the line above it (Eurofurence 14) and drawn in the
  other (Arial 10 bold), so the pair of the coin and the number sits a little off the middle.
- **The announcement of a trophy** plays its sting when the trophy room opens from the main menu as well as when a game awards one
  (the constructor, 0x45f5e0).

- **The escape key does nothing in the quit box**: it sets no escape answer, as the other boxes do (the options box answers it
  as "Done").

## Oddities (not chased)

- **The pictures of the restart point buttons** are taken from the exe's table of pictures from `IMAGE_PHOTO01` on, and only five
  of its entries are photos; a sixth point (if a game can have one) is drawn on the high score background. The port keeps the
  table.
- **The game menu's buttons** are in the order Main Menu, then the options, then Continue at the bottom.
- **The button sound of a moving dialog** (`NcMovingDialog::ButtonPress`) compares the id with the id plus 2000 and 3000, so it
  never plays.
- **The ready box** (`ReadyGoDialog::Closed`) un-pauses the game twice.
- **High scores** that were changed in the file load as their value modulo 256.
- **The game-over box's gnome** is not set when its file cannot be read (the original uses it uninitialised).
- **The main menu's locked-mode box** has the id 0x14.
- **The swollen trophy on the announcement page** is placed by half its width where its height was meant (0x45f6a0).
- **The level introduction** takes the gnome's pointer before it tests it for null in its destructor, and its update and draw
  assume it is set (a level file that names a gnome the factory does not know would crash).
- **The quit box** draws its picture box a second time over the one the dialog draws, and its 3D hammer is put by its middle at a
  different place from the 2D one's.
- **The options box's notices** hold runs of tab characters in the middle of their lines (the exe's strings); they show as
  nothing in the port.
- **A check box that cannot be ticked** (hardware acceleration, or full screen when the window would not fit) is put back by a
  second answer to the listener, so the check sound plays twice.
- **"Reset Help"** sets the flag of the current player without testing that there is one.
- **The quit and game-over boxes' button code** (0x44fe50) and the options box's press sound (0x44fe90) are functions the Ghidra
  export lacks (code the linker shared); they are found in the boxes' tables of the button listener (`tools/vtable.py` does not
  print them: read the table at the address in the class's constructor).
