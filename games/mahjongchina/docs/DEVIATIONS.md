# Mahjong Escape: Ancient China: where the port differs from the original

The port is the original game, unmodified. This list holds every place where its code does something other than
what `MahjongEscapeAC.exe` does, and why. None of them has an effect a player can see, except the shape of full screen
on a graphics driver set to stretch ("The device") and players that keep their progress when they move ("The
players' checksum"). A comment at each place in the code names the original's address,
so a grep for the address finds both sides. The code is shared with Ancient Japan's, so the comments on it give both
exes' addresses, each with the game's name after it, and this list gives both where the two exes' functions pair
(`re/mahjongchina/compare/addrmap.tsv` pairs them). The engine's members sit at other offsets in China (the game's own fields too, a
word or two lower), so the offsets below are Japan's.

## The device

The original draws through DirectDraw or Direct3D 7, plays through BASS and reads its cursor and its window from
Win32. The port replaces all of that with the platform (`docs/ENGINE.md`, "The device boundary"). Only the software
display is translated: the screen is an 800x600 buffer of 32-bit pixels that the host shows. Four things follow from
that. First, the original's 3D test (0x421370) always passes, and its warning text (Japan 0x5d8e20) is always empty, so
checking "Use 3-D Hardware Acceleration" in the options (0x412880 (China 0x4122f0)) never shows the note about the card. The setting is
still kept and saved as the original keeps it. Second, the vertical-blank wait that the map's curtain asks for
(0x4215e0, reached through 0x42a5b0) does nothing, and the windowed present's wait for the blit (0x432a30 (China 0x42c9e0)) does
nothing either: the host paces the frames. In the browser the game starts in the page even when its setting
(`FullScreen`, on for a new install) says full screen, because a browser enters full screen only from a click: the
host writes the setting off before the game reads it, as the framework starts every game's web build in the page.
The page's full screen button and the options switch it afterwards. Third, the accelerated display's software cursor is the host's cursor
(0x42a820 (China 0x420680)). Fourth, `GetTimeFormatA` and `GetNumberFormatA` (0x423450 (China 0x4256e0), 0x4232d0 (China 0x425560)) become fixed formats: "H:mm:ss" with
':' and thousands grouped by three with ','. Those are the formats of an English Windows, where the game was made.

Full screen keeps the desktop's resolution and shows the 800x600 screen at its own 4:3 shape, scaled and centred,
because the framework presents every game that way. The original does this only with "Wide Screen" checked on a
desktop at least 15% wider than 4:3 (or with `-Widescreen`, or with `-ScreenWidth` and `-ScreenHeight`, read at 0x426060). It sets
the screen's rect at 0x432000 (China 0x42bfc0) and maps the mouse back at 0x42cb90 (China 0x422ca0). Without that, the original switches the display to
800x600 (0x432f50 (China 0x42cec0)), and then the graphics driver's scaling decides the picture. Most drivers keep the shape, which
looks the same as the port; a driver set to stretch fills a wide screen. The port keeps the setting, shows its
checkbox when the original would (Japan 0x5d8e32, from the desktop's size) and saves it, but never changes the display mode.

## The window's background

China's window class is registered with a black background brush (`GetStockObject(BLACK_BRUSH)`, China 0x4295a0;
Japan's has none). The port has no window class of its own and draws every pixel of the screen, so nothing of it
shows. It matters only to the comparison: under Windows Sandbox, where the original's software display cannot draw
into its window, the brush paints over what a capture sees, so origrun's shots of the original came back black.
`tools/ddrawcap/` (a DDRAW.dll beside a copy of the exe that forwards to the real one and clears the brush when the
game first creates DirectDraw) makes them match; it changes nothing the game draws.

## The players' checksum

A player's file (`<id>.usr`) ends in a checksum that also covers the folder of the exe (0x416100 (China 0x415b60)), so the original
loads a player only from the install that saved it, and starts a moved one over. The port's folder is wherever its
game files are (another folder natively, a fixed one in the browser), so signing with it would start over every
player imported from the original and every player whose port moved. The port signs with the original's usual install
folder (`C:\Program Files (x86)\Popcap Game Collection\Mahjong Escape Ancient China\`) and loads a player signed with
that folder, with the same folder under `C:\Program Files\` (32-bit Windows), or with the port's own folder (players
the port saved before). A player from an original installed anywhere else still starts over, as it would in the
original after a move.

## Memory the original leaks

Every leak the translation found is fixed. Each object is freed after its last possible use:

- The board's clear (0x40a710 (China 0x40a590)) frees the list of floating score texts but not the texts on it; the port deletes them.
- The layout list (0x4022a0 (China 0x4022c0), `LayoutSelect.cpp`) deletes its two preview images only after a layout that loaded;
  the port deletes them always.
- The display's teardown (0x432610 (China 0x42c5c0)) walked its list of tracked images while each image's `Release` (0x433810) took
  itself out of that list, so every second image was skipped and leaked. The port lets go of all of them first.

## Bounds the original does not check

- The deal (`Deal.cpp`, the counts of the seasons and the flowers) reads past its eight fixed counts into its own
  stack when a layout's totals are larger. No layout of the game is that large. The port takes those counts as 0.
- The map reads the stage table (TXT 0x17f, fifty-two lines) with no bound on the number of lines (0x410d60 (China 0x410900)). The port
  stops at fifty-two.
- The keyed copy (0x42e960) loops once more than it should for an empty rectangle that needs no clipping. The port
  draws nothing for one.
- The list box's clear (0x427630) disables the scroll buttons even for a list that has none, through pointers it never
  set. The port skips them.
- `CString::Delete` (0x423700 (China 0x425990)) copies past the end for a count beyond the string. None of the game's calls passes such
  a count, and the port's copy stops at the end.

## Fields the original leaves unset

The tile's fly-in origin (+0x30, set by 0x40b780 (China 0x40b740) before its first use) and its free flag (+0x38, set by the first
test, 0x414c20 (China 0x4146a0)), the play layer's step (+0x12c, drawn only once the steps are set) and the tip box's position (all
nine ids set it, so the default is never used) start at 0 in the port. The board's tile-set pointer is set to null
after the clear (0x40a710 (China 0x40a590) leaves it, and nothing draws after the clear).

## Randomness

The game has one generator, the C runtime's `rand` (MSVC's linear congruential generator, 0x417d9f (China 0x4176d6)), and the port
keeps every `srand` and every draw in the original's order. The engine reseeds it from the clock every time it makes
an object (0x41ec10 (China 0x41e710)) or a stream (0x424d30 (China 0x42a500)). The board reseeds it right before each deal: with the stage's seed from
the stage table (TXT 0x17f) for an escape stage, and with the clock for a classic game and for every reshuffle
(0x40b0a0 (China 0x40af70)). The clock is the host's: `timeGetTime` natively and in the browser, or the framework's virtual clock under
`-fast`. That is why a scripted run repeats exactly.

## Buttons that led out of the game, a soft lock, the start and the hooks for the fixes mod (2026-11-08)

These four are Ancient Japan's, with China's addresses where the two exes' functions pair (`re/mahjongchina/compare/addrmap.tsv`);
`games/mahjongjapan/docs/DEVIATIONS.md` describes each, and each stands at the same function here.

- **The links.** The PopCap and SpinTop logos at the main menu's corners were link buttons that opened web sites. They are pictures
  that take no click now, and nothing in the game opens a URL. This is visible, and on purpose.
- **A soft lock.** A click on Shuffle while the last pair of a game flies cleared that flight in the redeal, so the round never
  closed: an empty board, the clock running, no result box. `Board::OnCommand` ignores the shuffle once no tile is left.
- **The start.** The two logo screens (2 s each) and the 0.77 s of black before them are out; the first layer is the main menu
  and the first `SwitchTo` has no fade-out (`jb::gFastStart`). `-logos` keeps the original's start. A player sees the menu
  come up in about a second where it took eight.
- **The hooks.** `Host::RealTickCount` (the deal search's time limit on the machine's clock) and `Host::SeedTick` (the three
  deal sites' seeds) answer as the original's `TickCount` does unless the fixes mod (`mods/fixes`) asks otherwise. The engine's reseeding at
  every object and stream (China 0x41e710 and 0x42a500) stays on the clock.

The JPEGs are decoded by the shared libjpeg 6b loader (`Platform::LoadImageFromMemory`), as the original's own libjpeg
does, where stb_image decoded them before; the frozen screens of the suite were taken again (at most 3 levels of a channel
of difference, in the JPEG backgrounds only).
