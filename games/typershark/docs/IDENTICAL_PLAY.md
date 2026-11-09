# Typer Shark: identical play

Phase 1 (the game's logic) is in `PHASE1.md`. This file holds the phase 2 pass, the screens. Status: started, not finished. The
menu screens are compared and identical; the game tours (board, creatures, bosses, map, typing tutor, game over, win dialog) are
written and recorded but their originals were not run, so nothing of them is judged yet. The next session takes up from "What is
left".

## Phase 2: screens

### Method

Both programs get the same input at the same updates. A tour is one list of steps (`games/typershark/tools/phase2/p2lib.py`) that is
written twice: as an origrun script for the original (`at <update> click|move|text|key|shot|peek`) and as a `-script` for the port
(the port's line N+1 matches origrun's `at N`; at one update a shot comes before the inputs of that update on both sides). The original
runs only inside Windows Sandbox (`origrun --sandbox`, the virtual clock, the scripted cursor, update-timed shots; run name
`typershark-p2`); the port runs under rundesk with `-fast -drawevery` from a users folder fixture whose registry holds `ScreenMode 0`
and `Is3D 0`, so that its Options dialog starts where a sandbox run's does (windowed, no 3D). The 640x480 client is cut from the
original's 646x509 capture, and the pairs are compared by `tools/shotdiff` with `games/typershark/shotdiff.json`. The state of the
original at a shot can be read with `at N peek <expr>` and set beside the port's state log.

<!-- private: The original's loading thread runs on the wall clock, so on the virtual clock the title screen was still loading when the
script began. A scratch patch list (git-ignored, under build/ts-p2) makes the loading in the copy run in one go. -->
The copy of the original is started with a patch list kept outside the repository: its loading runs in one go (the port loads in one
go by the user's rule), the other patches are those of phase 1's runs.
<!-- /private -->

### Tour M: the menus (98 pairs, 98 match)

The scenario: a first start (no profile), the NEW USER dialog at several updates, a name typed a letter an update (a shot after each of
the first ones, then later for the caret), the "assess WPM" question and its No, the menu, the cursor over each button and link (the
Adventure, Abyss and Typing Tutor buttons, the Hall of Fame, Options and Quit buttons, the "not you" and statistics links, away),
the Options dialog (open, hover, Enable Fx off and on, Custom Cursors on and off, Space bar pauses, both sliders dragged, Credits and
its dialog, Done), the Hall of Fame (empty), the three statistics tabs, the difficulty dialog with the cursor over each button and its
Cancel. Title shots at updates 5, 30, 100 and 300. All 98 pairs match with no ignore rectangle.

| Screen | Result |
| --- | --- |
| Title, loading | match |
| NEW USER dialog, empty and typed (caret) | match |
| Assess WPM question | match |
| Menu and every hover state | match |
| Options dialog (every checkbox state, sliders, Credits dialog, Done) | match |
| Hall of Fame (empty) | match |
| Statistics (three tabs) | match |
| Difficulty dialog (every button hovered, Cancel) | match |

Two things that first showed as differences and are not screen differences:

- The port's Options dialog showed Fullscreen and Hardware Acceleration checked. The port's shots came from a run that started with
  the framework's registry defaults, a sandbox run is windowed and without 3D. With the fixture registry (`ScreenMode 0`, `Is3D 0`)
  the dialogs match. An environment difference, no code change.
- The original's capture holds the cursor the game draws itself when "Custom Cursors" is on (the image cursors, drawn into the window by
  the DDInterface at `SetCursorImage`/`DrawCursorTo`; `EnforceCursor` 0x4aba70 draws them only when +0x52b, the option, or +0x541, the
  demo-playback flag, is set). The port's `screenshot` command renders the widgets to the screen image and never draws the cursor (the
  framework draws it in `DDInterface::Present`, after the capture). A tour that turns the option on shows a cursor box of 27x26 or
  19x27 pixels at the mouse position in every later shot. The tour turns the option off again right after the Options shots, so the
  remaining 98 pairs hold no cursor. With the option off, the original sets the system cursor, and its capture does not hold that.
  A probe (`at N peek`) of the original on the menu read +0x52b = 0, +0x541 = 0 and cursor number 1 (hand) over the Adventure button.

### Shared-code notes for the coordinator (specs, no change made)

1. A `screenshot` variant that draws `DDInterface::DrawCursor`'s image into the capture would let a tour compare the custom cursor too.
   Until then the original's custom cursor in a shot is not comparable.
2. With "Custom Cursors" off the original sets its own system cursors: the hand and the drag hand are made by `CreateCursor` in the
   SexyApp constructor (32x32 monochrome masks at 0x561788/0x561808 with hotspot (11,4), and 0x561888/0x561908 with hotspot
   (15,10)). The framework's `SetSystemCursor` (framework/SexyAppBase.cpp, near line 268) uses SDL's system hand for both, so the
   pointer over a button is Windows' hand, not the game's. It is the standard SexyAppFramework hand and drag cursor, so the fix is
   shared: build the two cursors with `SDL_CreateCursor` from the framework's own hand data.
3. Under rundesk the port's window reports a fullscreen flag at start (`SDL_WINDOWEVENT_SIZE_CHANGED` sets `mIsWindowed` from the
   window's flags, line 2177), so a run without a registry fixture saves `ScreenMode 1`. To check in a normal window whether the first
   start is windowed.

### Candidates (GAME_NOTES.md section 10)

- F-8, the Options dialog's Credits button: the port and the original match in every Options pair (open, hover, pressed), so the port
  keeps the original's look. The Credits label is drawn in the same white as the Done label in the original's shots. Not changed.
- F-4 (tutor lines from the fifth line of a task), F-5 (keyboard picture keys 8, `*`, `{ } _ | \`) and F-6 (the final map panel's
  ship): not yet judged, the original's tutor and map tours were not run. The tour `g_tut18` (lesson 18, a `tfirst` of 4 is reached) and
  the whole-adventure tour `g_rt2` hold the shots; the symbol keys need a longer tutor recording (`rec.py ... --next 30` or more).

### What is left (written, not run on the original)

Tours in `build/ts-p2/<name>/{orig,port}.txt` (git-ignored; regenerate with `tour_menu.py`, `tour_users.py`, `tour_saved.py`,
`tour_game.py <name> <tag>` from `rec.py` recordings): `u1` (the players: ten users, Select User with the Delete line and the Delete
dialog; the coordinates in `tour_users.py` are from the port's layout and not yet calibrated against pictures), `s1` (a saved
adventure: the CONTINUE? dialog, pause, options in a game, the quit confirmation; the port side was run, 33 shots), `g_idle` (game over;
the port side was run, 78 shots), `g_abyss`, `g_tutor17`, `g_tut18`, `g_exp` (penalty piranhas), `g_bon1` (clams), `g_rt2` (the whole
adventure: maps, bosses, the win dialog). `queue.sh ports|origs <name>...` runs a list of tours as one stream, and `chain.sh` runs the
port, the original and the diff of one tour. Not started: the abyss CONTINUE? dialog (needs a best depth of 400 feet: `abyss2` is
recorded, the game-over click and the Abyss click remain to be written), the Hall of Fame with entries, rank titles by level-end screens.
Run the port with `USERS=port-win` (the windowed, software fixture; make it from `port-clean` plus a `registry.ini` with `ScreenMode`
and `Is3D` at `dword:0`) so that its Options dialogs match.

### Oddities

None found so far.

### Awaiting the user's review

Nothing yet: every pair of tour M matches.
