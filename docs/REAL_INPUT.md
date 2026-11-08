# -realinput: a hand's input for a native port

A `-script` run hands its input straight to the widget manager (`SexyAppBase::RunScript`), and an `-autoplay` bot does
the same through `AddScriptCommand`. That input never passes SDL's queue, `SexyAppBase::HandleEvent`, the window's mapping
to game pixels (`DisplayGeometry`), F2's mod menu or Alt+Enter, and both kinds of run drop the real mouse, keyboard and
focus. A port can pass every scripted comparison and still fail a player: Bejeweled Twist could not twist a gem in the
browser (the user, 2026-11-05).

Posting Windows messages to the window under rundesk covers only part of it. SDL 2.32 asks Windows to report when the
mouse leaves the window (`TrackMouseEvent`), and on rundesk's desktop the real cursor is never over it, so right after any
posted `WM_MOUSEMOVE` SDL takes the real cursor's position (`GetCursorPos`) and sends the window's LEAVE. A move, press and
release posted back to back are all handled before that, so a click posted in one burst lands where it should (Seven Seas'
`realinput.py` and Bonnie's `realplay.py` click this way), and posted keys with their scan codes type as they should. But a
move on its own (a hover) is followed in the same millisecond by a motion to the real cursor and the LEAVE; a button held
across frames is dragged to the real cursor at once; every move of a drag jumps there and back; and the window can never
gain the focus there (`SetForegroundWindow` fails, and posted activation messages make no focus events), so a focus loss
cannot be posted either. SDL's event log of each case on fwtest (2026-11-07) showed exactly this. Moving the real cursor
instead would move the user's own mouse.

`-realinput=<file>` is the framework's answer, in every port built on `SexyAppBase`. It reads timed events and, when
their update comes, pushes them into SDL's own queue with `SDL_PushEvent`. From there they take exactly the path a hand's
events take: `SDL_PollEvent`, `HandleEvent` (the click codes, the key's modifiers, Alt+Enter, F2, the text a key types,
focus and minimize), the window-to-game mapping and the widget manager. Window changes are real: `minimize`, `restore`,
`maximize` and `resize` call SDL's own functions on the window, and the window sends SDL its own events back. The command
list is in `framework/SexyAppBase.h`, next to `-script`'s.

## Running it

Start the port under rundesk like any test run, with absolute paths:

    build/native/rundesk.exe build/native/dynomite.exe -gamedir=<abs> -userdir=<abs> -realinput=<abs file> -fast

It works with `-fast` and in real time. The file is a `-script` file, one `<update count> <command> [args]` a line. The
events of a line are pushed when that update starts and handled once it is done, before the next update, as a hand's
events between two updates are; so the effect of a click at update 300 shows at update 301. A line's points are window
pixels, or game pixels after the word `game` (`click game 320 450`), mapped as the window shows the game at that moment,
so the same file works in a window of any size and in full screen. `screenshot`, `screenshotfull` and `quit` work as in
`-script` and run at once; any other command goes to the game's `HandleScriptCommand`, so a game's own test hooks (a save,
a state dump) can check what the input did. In real time a game may still be loading at an update count that is past
loading with `-fast` (in real time TipTop was still on its loading bar at update 300 and at its menu by 900), so take a
screenshot before the first click.

The run is a test run (`IsTestRun()`): message boxes only log, `WarpMouse` does nothing, and no URL is opened. From the
moment the window exists the system's own mouse, keyboard, text and focus events are dropped (an `SDL_SetEventFilter`
lets through only what the file pushes), so the run is the same whatever happens to the real mouse or the window's focus.
It does not go with `-script` (the run stops with an error), nor with a game's `-autoplay`, which sets `mScriptedInput`
and so drops real input (the run logs it). `HandleEvent` drops real input only in those two kinds of run: a run with a
script file, and one whose bot sets `mScriptedInput`. A game's test command that waits for its moment by queuing itself
again through `AddScriptCommand` leaves real input alone; until 2026-11-08 any line in the script dropped it, so one
waiting `goto` on the live channel kept every real click out of Bookworm Adventures.

A driver that decides as it goes (a solver that reads the board before each move) gives `-commands=<file>` as well,
with an empty `-realinput` file: the lines it appends to the commands file, with no update count, are played the same
way at the next update and answered on the log (`OK click game 320 450`), and the game's test commands (`state`,
`goto`, docs/TEST_COMMANDS.md) answer through the same file. `tools/fwtest/commands.sh` drives fwtest that way.

## How it differs from a hand

The file says everything a hand's input would make Windows send, and nothing more is made up for it. A hand that clicks
the minimize button also takes the focus away; the file writes `focus lost` before `minimize`. Two quick clicks of a hand
make SDL count a double click; the file writes `dblclick`, or `down <point> left 2`. A key that types sends its text after
the key, as Windows sends `WM_CHAR` after `WM_KEYDOWN`, worked out for a US layout with Shift and Caps Lock; other text,
accented letters among them, comes from `text`. When a game warps the cursor, a real warp would also send a motion to that
point; every test run skips the warp, so the file's mouse stays where the file left it.

## The check

`tools/check.sh unit` runs `fwtest -realinput=tools/fwtest/realinput.txt` with `-fast` and in real time, with no game
files. The file moves, presses, drags, double-clicks and scrolls in a window resized to twice the game's size and in a
letterboxed one, clicks fwtest's button in game pixels, types with Shift, repeats a held key, sends text alone, types
control characters, opens and closes the mod menu with F2, loses the focus with a key held and regains it, leaves and
re-enters the window, minimizes and restores it, and goes full screen and back with Alt+Enter. fwtest's own `expect`
lines check what reached its board and the app (41 checks). A run that posted 443 real key, character, mouse and focus
messages to the window during a `-realinput` run found none of them at the board; with the filter taken out, the same
posts reached it.

Dynomite, Big Money and TipTop were driven to their next screens with it natively under rundesk, with `-fast` and in
real time, through a focus loss, Alt+Enter both ways and a real minimize and restore, watched from outside the process.

## Games it cannot fully reach

Every port's input comes through `HandleEvent`; no game polls SDL's events itself or overrides `HandleEvent`. A few read
state elsewhere or treat any test run as focused, and their sessions decide whether to change that:

| Game | What `-realinput` cannot reach | What would fix it |
| --- | --- | --- |
| Cosmic Bugs, Water Bugs | Focus: `IsActive()` is `mActive \|\| IsTestRun()`, so a file's `focus lost` mutes the framework but never pauses the game | `mActive` alone in a `-realinput` run (only the file's focus events reach it there) |
| Bonnie's Bookstore | Focus: `getAppState()` gives input and mouse focus to any test run unless its own script took it; and in full screen the cursor's position comes from `SDL_GetGlobalMouseState`, the real cursor | `mHasFocus` and `mMouseInside` in a `-realinput` run; in full screen the position the events gave. Its host joins each character to the key press just before it, so type there with `key` or `type`, not `text` alone |
| TipTop | Nothing in its input, but its `AppMessage` shows an SDL message box unless the run is `-fast`, `-script` or scripted, so a real-time `-realinput` run that meets an error message would sit on it | `IsTestRun()`, as the Mahjongs and Ning Po check |
| Alchemy, Atomica, Bejeweled, Big Money, BookWorm | Their `-autoplay` sets `mScriptedInput`, which drops real input | Not to be combined; drive these with the file alone |

On the web, the page's own input in the Browser pane is the real path. The web build compiles `-realinput` too, but it has
not been tried there; it could only run where `-script` does, in a web build under Node. The page's test kit treats the focus as
`-realinput` does (since 2026-11-08): the browser's own blur and focus events never reach the game on the player-only page, so a
hidden Browser pane, which sends a real blur, no longer pauses a run until its driver calls `byopop.test.focus()`. Only the
driver's `byopop.test.blur()` and `focus()` move the game's focus there, and `?realfocus=1` lets the browser's through again
(`docs/WEB_PLAYER.md`, "The player-only page").
