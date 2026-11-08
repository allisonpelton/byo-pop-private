# Mahjong Escape: Ancient Japan: where the port differs from the original

The port is the original game, unmodified. This list holds every place where its code does something other than
what `MahjongEscapeAJ.exe` does, and why. None of them has an effect a player can see, except the shape of full screen
on a graphics driver set to stretch ("The device") and players that keep their progress when they move ("The
players' checksum"). A comment at each place in the code names the original's address,
so a grep for the address finds both sides.

## The device

The original draws through DirectDraw or Direct3D 7, plays through BASS and reads its cursor and its window from
Win32. The port replaces all of that with the platform (`docs/ENGINE.md`, "The device boundary"). Only the software
display is translated: the screen is an 800x600 buffer of 32-bit pixels that the host shows. Four things follow from
that. First, the original's 3D test (0x421370) always passes, and its warning text (0x5d8e20) is always empty, so
checking "Use 3-D Hardware Acceleration" in the options (0x412880) never shows the note about the card. The setting is
still kept and saved as the original keeps it. Second, the vertical-blank wait that the map's curtain asks for
(0x4215e0, reached through 0x42a5b0) does nothing, and the windowed present's wait for the blit (0x432a30) does
nothing either: the host paces the frames. In the browser the game starts in the page even when its setting
(`FullScreen`, on for a new install) says full screen, because a browser enters full screen only from a click: the
host writes the setting off before the game reads it, as the framework starts every game's web build in the page.
The page's full screen button and the options switch it afterwards. Third, the accelerated display's software cursor is the host's cursor
(0x42a820). Fourth, `GetTimeFormatA` and `GetNumberFormatA` (0x423450, 0x4232d0) become fixed formats: "H:mm:ss" with
':' and thousands grouped by three with ','. Those are the formats of an English Windows, where the game was made.

Full screen keeps the desktop's resolution and shows the 800x600 screen at its own 4:3 shape, scaled and centred,
because the framework presents every game that way. The original does this only with "Wide Screen" checked on a
desktop at least 15% wider than 4:3 (or with `-Widescreen`, or with `-ScreenWidth` and `-ScreenHeight`, read at 0x426060). It sets
the screen's rect at 0x432000 and maps the mouse back at 0x42cb90. Without that, the original switches the display to
800x600 (0x432f50), and then the graphics driver's scaling decides the picture. Most drivers keep the shape, which
looks the same as the port; a driver set to stretch fills a wide screen. The port keeps the setting, shows its
checkbox when the original would (0x5d8e32, from the desktop's size) and saves it, but never changes the display mode.

## The players' checksum

A player's file (`<id>.usr`) ends in a checksum that also covers the folder of the exe (0x416100), so the original
loads a player only from the install that saved it, and starts a moved one over. The port's folder is wherever its
game files are (another folder natively, a fixed one in the browser), so signing with it would start over every
player imported from the original and every player whose port moved. The port signs with the original's usual install
folder (`C:\Program Files (x86)\Popcap Game Collection\Mahjong Escape Ancient Japan\`) and loads a player signed with
that folder, with the same folder under `C:\Program Files\` (32-bit Windows), or with the port's own folder (players
the port saved before). A player from an original installed anywhere else still starts over, as it would in the
original after a move.

## Memory the original leaks

Every leak the translation found is fixed. Each object is freed after its last possible use:

- The board's clear (0x40a710) frees the list of floating score texts but not the texts on it; the port deletes them.
- The layout list (0x4022a0, `LayoutSelect.cpp`) deletes its two preview images only after a layout that loaded;
  the port deletes them always.
- The display's teardown (0x432610) walked its list of tracked images while each image's `Release` (0x433810) took
  itself out of that list, so every second image was skipped and leaked. The port lets go of all of them first.

## Bounds the original does not check

- The deal (`Deal.cpp`, the counts of the seasons and the flowers) reads past its eight fixed counts into its own
  stack when a layout's totals are larger. No layout of the game is that large. The port takes those counts as 0.
- The map reads the stage table (TXT 0x17f, sixty lines) with no bound on the number of lines (0x410d60). The port
  stops at sixty.
- The keyed copy (0x42e960) loops once more than it should for an empty rectangle that needs no clipping. The port
  draws nothing for one.
- The list box's clear (0x427630) disables the scroll buttons even for a list that has none, through pointers it never
  set. The port skips them.
- `CString::Delete` (0x423700) copies past the end for a count beyond the string. None of the game's calls passes such
  a count, and the port's copy stops at the end.

## Fields the original leaves unset

The tile's fly-in origin (+0x30, set by 0x40b780 before its first use) and its free flag (+0x38, set by the first
test, 0x414c20), the play layer's step (+0x12c, drawn only once the steps are set) and the tip box's position (all
nine ids set it, so the default is never used) start at 0 in the port. The board's tile-set pointer is set to null
after the clear (0x40a710 leaves it, and nothing draws after the clear).

## Randomness

The game has one generator, the C runtime's `rand` (MSVC's linear congruential generator, 0x417d9f), and the port
keeps every `srand` and every draw in the original's order. The engine reseeds it from the clock every time it makes
an object (0x41ec10) or a stream (0x424d30). The board reseeds it right before each deal: with the stage's seed from
the stage table (TXT 0x17f) for an escape stage, and with the clock for a classic game and for every reshuffle
(0x40b0a0). The clock is the host's: `timeGetTime` natively and in the browser, or the framework's virtual clock under
`-fast`. That is why a scripted run repeats exactly.

## Buttons that led out of the game (2026-11-08)

The PopCap and SpinTop logos at the main menu's bottom corners were link buttons (`LinkButton`, 0x425ba0, whose click is a `ShellExecuteA` "open" at 0x425cb0) that opened the companies' web sites. The port keeps the logos as pictures and the buttons
disabled, so a click on them does nothing, and nothing in the game opens a URL (`Host::OpenURL` is gone). This is a
change a player can see, made on purpose (the user, 2026-11-07, "Yes, now too": every button that leads out of a game
comes out in this phase). The game has no registration, trial or buy screen to remove.

## A soft lock the original has (2026-11-08)

The Shuffle button is taken while a board is not over, not shuffling and not in a pause (`Board::OnCommand`, 0x40b500), and
the last pair of a game flies for about a second before `mLeft == 0` closes the round (`Board::Update`, 0x40bd70). A
shuffle started in that second redeals the layout through `Board::Reshuffle` (0x40b0a0), which clears the flights, so the
flight whose end closes the round is gone: the board stays empty, the clock keeps running and the result box never opens. The
port ignores the shuffle once no tile is left (`mLeft == 0`), the one line `OnCommand` needs, so the round closes as it
does when the shuffle is not clicked. A player cannot see the difference except by clicking Shuffle in that second.

## The game opens at its main menu (2026-11-08)

The original shows two logo screens first, 2 s each (`Logo::Update`, 0x42f8c0, counts down and reads no click or key; the
publisher's SplashLogo picture, then Jolly Bear's), after 0.77 s of black (`Manager::SwitchTo`, 0x42ae80, fades out of a
screen when there is none), and loads nothing meanwhile. The port takes that wait out (the loading rule): the first layer is
the main menu (`MahjongGame::CreateFirstLayer`, 0x40f540, which the original gives the splash) and `SwitchTo` has no fade-out
when no layer is shown (`jb::gFastStart`); the menu's own fade-in stays. `-logos` keeps the original's start. A player sees it:
the menu comes up in about a second where it took eight. `GAME_NOTES.md`, "The start", has the numbers.

## Hooks for the fixes mod (2026-11-08)

The fixes mod (`mods/fixes`, `GAME_NOTES.md`) needs three places where the clock is read to be asked of the host. Without
the mod each answers as the original does, so the game plays the same: `Host::TickCount` stays the framework's clock
(`timeGetTime`, or the virtual clock under `-fast`), `Host::RealTickCount` is the machine's clock in milliseconds (the deal
search's time limit, checked at 0x413fa0 and started at 0x413500; under `-fast` it is `SDL_GetTicks`, because the virtual clock does not
move while a deal searches), and `Host::SeedTick` is the clock again at the three deal sites (`Board::SetLayout` 0x40aa40,
`Board::Start` 0x40ae60, `Board::Reshuffle` 0x40b0a0). `steadyClock` replaces the first with a clock of the updates,
and `seededDeals` the third with a seed the page gives.
