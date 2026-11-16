# Peggle Deluxe: game notes

What the port does differently from the exe, what it leaves out and why, how to test it, and what is left. The analysis state
(tools, the original's structure, data) is in `WORKING_NOTES.md`; how a function is translated is in `TRANSLATION_GUIDE.md`.

## Status (2026-11-16)

Builds natively (`TARGET=peggle bash tools/build.sh native`). Runs to the main menu: the loading screen (logo, sun, light bar,
ball and sparks, the bricks, pegs and fever stars made while it is up), "Click to play", the scroll to the main menu, the
mascot's speech bubble that follows the mouse, the trophy, the dialogs that slide in on a chain, and the Demo (locked) and Quit
questions, and the first run's certificate (the name is typed, Enter or the OK button takes it, the stamp comes down, the
dialog slides out and the loading screen starts the adventure) and the list of players ("WHO ARE YOU?": choose with OK or a
double click, rename, delete with its question, create a new one, the name errors). Checked under rundesk with screenshots (`-script` with
`profile <name>`, `char`, `key RETURN`, `click`, `move`, `screenshot`). Nothing past the menu and the first run runs yet: every
other screen stops with a message that names the function (`grep NOT_TRANSLATED games/peggle/src`); the first run ends at
`StartAdventureGame`.

Translated: the framework glue (ThunderballApp's start-up, loading, music, screens' scroll, dialogs' frame), the data classes and
managers (StringParser, ConfigParser, DataSync, GameStats, PlayerInfo, ProfileMgr, HighScoreMgr, StatsMgr, ImageMgr, CharacterMgr's
file loader, TrophyMgr, StageMgr), BlendedImage, WidgetMover, ThunderButton / ThunderCheckbox / ThunderSlider / ThunderFrame,
ThunderCommon (the button and edit factories, PositionDialog), ThunderDialog, LoadingScreen, MainMenu, NewUserDialog and UserDialog (with the app's
DoCreateUserDialog, FinishCreateUserDialog, FinishNameErrorDialog, DoUserDialog, FinishUserDialog, the rename and delete
dialogs, CheckMaxStage; ProfileMgr's rename and delete).

## Left out of the port, by address

Registration, trial and outbound buttons are removed, not disabled (the registered path is the only one).

- `IsRegistered`, `IsExpired`, `IsTrialOver`, `IsLevelDemoLocked`, `IsLevelLockedTrial`, `TryExpire`, `CheckCanExpire`, the
  `UpsellScreen` class and its calls (`ShowUpsellScreen`, `EndUpsellScreen`, `TryPlayUpsell`, `UpsellScreen::Pause`); the Upsell
  resource group is not loaded; the app's upsell slot (+0x7bc) is not in the port.
- MainMenu: the "Buy" button (+0xb4, id 9, lines 480 to 482), the registration continuation at the end of `Update` (line 605, the
  flags at app +0x83a, +0x83c), the trial branch of the Demo button (+0x839), `IsTrialOver` in the Adventure button (it shows
  tip 0xb) and in the bubble of the Adventure button ("Start a new Adventure!" is shown only for a profile that has no stage
  yet), `IsLevelLockedTrial` in `StartGame` for Challenge.
- The partner logo of the loading screen (`properties/partner.xml`) and the marketing mode (`InitHook` 0x41bff0).
- The loading screen's `LoadTimer` and the debug prints of `PlayMusic`.

## Dead code of the original that is not translated

Numbers the original reads through `ModValInt` and that are 0, which switch a block off: ThunderDialog's swinging chain (line 851)
and the bounce at the end of a slide (lines 846, 845); MainMenu's blink of the Adventure button for a new profile (line 516).
UserDialog's first line "(Create a New User)" (line 935, a flag at +0x198 that is 0); NewUserDialog's frame of the edit box on the
certificate (line 674). Each is a constant folded away.

## Deviations (each without a visible effect)

- The level music (`music/pegglebeats.mo3`) is loaded by `LoadLevelMusic` at once; the original starts a thread for it and the
  loading screen waits for it.
- `MainMenu::PlaceButtons` holds the button positions that the original writes both in the constructor and in every `Update`
  (equal ModVal numbers); it is called from both, so the behaviour is the same.
- `MainMenu::ButtonDepress` of the original begins with `LogicMgr::KeyDown(id)`, which the Mac build's names make look
  like a test (0x437350); it is a stub that answers 0, so it is not translated.
- The hidden sequence of the main menu (the notes of "Ode to Joy" by hover) stops counting at its end; the original goes on
  reading the table past its last entry (nothing can match it there).
- `ThunderButton::mHoverSoundPitch`: the original does not set it for a button that has no hover sound (uninitialised); 0 here.
- `FinishRenameUserDialog` saves the profile in use before renaming; the original calls the save without asking whether there is
  one (a null `this`); the port skips it then. The dialog cannot be reached with none.
- The debug keys of the original (Ctrl, H, D, A in the loading screen's debug mode) are kept.

## Fix candidates (for the fixes mod, not the port)

None yet.

## Oddities

- The certificate's stamp: the OK button stays drawn under it (the original disables it; whether a disabled embedded button
  draws differently is to be seen in the shot-diff pass).
- The Quit button of the main menu (the round purple button at the bottom left) shows "UIT": the Q is under the left frame. It is
  drawn as a child above the frame, so the cause is in the button's artwork or the text's offset; compare with the original in
  the shot-diff pass.

## Test commands (`docs/TEST_COMMANDS.md`)

`profile <name>`: makes the profile if there is none and takes it as the one in use (the way round the new-player dialog, which is
not translated yet). The rest of the standard set comes with the board.

## Not translated yet

`grep NOT_TRANSLATED games/peggle/src` is the list. The order the next work takes:

1. The level files and the pieces of a level (PegInfo, Brick, Poly, Line, Hole, Gun, Mover, `LoadLevel`), then the physics
   (PhysObj, Ball, CollisionMgr, LogicMgr's 99 functions, 57 KB, the largest part), then the Board's rules (83 functions, 34 KB),
   the effects (Effect 47, EffectMgr, FloatingText), InterfaceMgr's drawing and CharacterMgr's animation. About 220 KB of the
   original's code in all.
2. The dialogs and screens that the menu leads to: OptionsDialog, LevelScreen,
   AdventureScreen, StoryScreen, TrophyScreen, HelpScreen, CharacterDialog, EndLevelDialog, SlotMachineDialog, the replay
   dialogs; `MainMenu::DrawTrophyEffect` (an x87 particle effect: 0x494b50).
3. Replays and the bot (AIMgr, AIBall) for the Demo and the Masters, then the test commands and `move`.
