# Mahjong Escape: Ancient Japan: where the port differs from the original

The port is the original game, unmodified. This list holds every place where its code does something other than
what `MahjongEscapeAJ.exe` does, and why. None of them has an effect a player can see. A comment at each place in
the code names the original's address, so a grep for the address finds both sides.

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
