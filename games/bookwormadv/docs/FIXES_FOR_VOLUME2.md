# Bookworm Adventures fixes that Volume 2 should carry over

These are faults the user found by playing Bookworm Adventures, fixed on branch `claude/bookwormadv-78ebd4` in
October 2026. Each lies in engine code that `games/bookwormadv2` shares or copied. The Volume 1 session left that
folder alone, so Volume 2's session should check each one against its own exe's addresses and fix what applies.

## Colours given as tables were drawn transparent

common.lua's `Color(r, g, b[, a])` returns a table `{r, g, b, a}`, and the scripts pass it to `g:SetColor(c)`. The
original's `ToColor` (0x66fe40 in Volume 1) reads a table's `r`, `g`, `b` and `a` fields as ints, a missing one 0.
The port read only a packed number, so every such colour came out as 0, transparent black. This hid the enemies'
lore and attack texts in the right-hand panel (the likely cause of Volume 2's missing enemy attack text), the
smashed tile's countdown and other texts. Volume 2's `ToColor` is unchanged at `luaapp/PopLua.cpp:48`.

## BreakupString keyed its table by doubles

`gCApp:BreakupString` (0x44f020 in Volume 1) pushes each part's index as a PopLua int (tag 4). The port pushed a
number. A number key goes into the table's hash part, where the scripts' int indices and the size operator miss it,
so `common.DrawHilitedTextWrapped` drew nothing: the item descriptions on hover and in the new-treasure box were
missing. Volume 2 still uses `lua_pushnumber` at `src/GameAppLua.cpp:412`; it should be `lua_pushint`.

## PopAnim labels are upper case

The original upper-cases each label as it loads it (0x56cda0, called after RemapName at 0x52bc4c) and upper-cases
the label it looks up (0x529c80). Some files have lower-case labels (`bustedcirce.pam` has `anim1` and `anim1loop`)
and the scripts play `ANIM1`. With an exact match the scene after a book's last boss never started, and the game
waited at the treasure altar for good. Volume 2 already keeps its labels in a case-insensitive map
(`StringLessNoCase`), which finds the same labels, so nothing needs to change there unless something lists them.

## Black and White swapped on the title screen and the curtain

In Volume 1's exe the static initialisers make 0x825f00 `Color(0, 0, 0)` and 0x825ef0 `Color(255, 255, 255)`
(0x795a30 and 0x795a50; Graphics draws untinted images with 0x825ef0). The port read 0x825f00 as White in five
places, all Black in the original: the title screen's fade colour at start and after the logo, the fill behind the
PopCap logo, and the curtain's full-screen fill and loading text. The logo's JPEG has a black square background,
which is why the user saw a black box on a white screen. Volume 2 has the same five Whites (`TitleScreen.cpp`
lines 16, 151 and 160, `CurtainTransition.cpp` lines 136 and 148), around its fader at 0x489400.

## The chapter cheer was far too loud (shared)

CROWD_CHEER has `volume="10"` in resources.xml. DirectSound refuses the positive value that gives, so the original
plays the cheer at full volume, while the port amplified it about 13 times. The fix is in the shared
`framework/SDLSoundManager.cpp` and covers Volume 2 once merged; Volume 2's resources.xml has the same attribute.

## The music was far too quiet (shared, plus one game-side override)

Two causes. First, the shared `SDLMusicInterface::SetMusicAmplify` did nothing. Bookworm Adventures loads its music
with an amplify of 12.0, which BASS 2.3 takes as 1200 and holds at its maximum of 100 (its default is 50). The
shared interface now applies a gain measured from the game's own bass.dll, about +10 dB over libopenmpt's level at
amplify 100; the commit message has the measurements. Second, the original's `GameApp::CreateMusicInterface`
(0x44f470) makes a SexyBassMusicInterface whose SetVolume, on BASS 2.2 and later, sets the global music volume to
volume × 100 and ignores the 40 in mMaxMusicVolume. The port used the framework's default interface with that cap
of 40. Volume 1's GameApp now overrides CreateMusicInterface with mMaxMusicVolume at 100. Volume 2 needs the same
override if its bass.dll is also 2.2 or later (check `BASS_GetVersion`, and its CreateMusicInterface), and its
music may need its own amplify measurement if its module differs.

## The battle went dark over the menu

`Board::Draw` (0x41b9a8 in Volume 1) defers its pause shade at priority 0, under the dialogs, while the app's
`mDialogMap` holds any dialog, and at 10000 otherwise. The app's +0x328 is that map's head and +0x32c its size; the
port read +0x32c as a flag nothing sets, so the shade always covered the menu. The same member makes
`MyFlashWidget::Update` skip the movie's update while a dialog is open. Volume 2 has both (`src/Board.cpp:985`,
`src/FlashWidget.cpp:66`, MyFlashWidget at 0x472a90), and its `MainMenu.cpp:568` already reads +0x32c as the
number of dialogs.
