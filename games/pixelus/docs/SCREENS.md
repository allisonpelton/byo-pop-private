# Screens and dialogs of Pixelus

Every screen and dialog the port can show, taken from the export and the source, with the shots that compare each one
against the original (see `docs/SHOTDIFF.md`, "Every screen, listed"). The original runs in 2D only (no original has
run in 3D in Windows Sandbox), so the 2D port is compared with the original; the game has no 3D-only screens. Scenario
scripts are `games/pixelus/tools/screens/scripts/<scenario>.txt` and `games/pixelus/tools/screens/final.sh` pools their
shots into the run `screens`; a shot is named `<scenario>-u<update>`. Scenarios: A (menu, user dialogs, options, credits,
help), B (story, island, locked temple, tutorial temple), CD (tutorial board, options and dialogs over it), C3 and RQ
(back to the menu and the island with a puzzle begun, the resume question), E (a temple of an unlocked game: thumbnails,
help, board, reset, hint, pause, island), H (a tutorial puzzle played to its tablet), I0 and I1 (the island of a new and
of a finished game), L (the loading screen). The differences each note calls explained are written up in
`IDENTICAL_PLAY.md`, "The screen coverage pass".

| Screen | Reached by | Shots | Note |
|---|---|---|---|
| LoadingScreen (bar, "click here to play" link) | launch (scenario L) | L-u40 L-u100 L-u140 | explained: the ribbon under the logo had "Deluxe" painted out on the user's wish of 2026-11-18, and the original shows no bar behind the link where the port keeps it faded (IDENTICAL_PLAY.md, "The screen coverage pass") |
| MainMenu ("Welcome", player name, Play/Options/Quit, "if this is not you") | after loading, after a dialog closes | A-u520 A-u1290 C3-u1950 |  |
| New user dialog (0x13) with the expert checkbox | menu Play on a fresh profile (A) | A-u200 A-u400 | explained: the text caret's blink phase (a 2 pixel line in A-u400) |
| Empty-name error (0x16) | New user OK with no name (A) | A-u260 |  |
| Name-exists error (0x16) | New user OK with a taken name |  | none: not shot; it is the empty-name error's dialog (A-u260) with the text ids 0x512588 and 0x512598 |
| Expert warning (0x1e) | ticking the expert box (A) | A-u345 |  |
| Select user dialog (0x15) | menu "if this is not you" (A) | A-u580 |  |
| Rename user dialog (0x17) | select user, Rename (A) | A-u640 | explained: the text caret's blink phase (a 2 pixel line) |
| Rename error (0x17) | Rename with a taken name |  | none: not shot; the same dialog as the empty-name error with the text ids 0x512528 and 0x5126d8 |
| Delete user question (0x14) | select user, Delete (A) | A-u690 A-u730 |  |
| Quit question (4) | menu Quit (A) | A-u850 |  |
| Options (2002) from the menu | menu Options (A) | A-u940 A-u1060 A-u1230 | explained: the original also has the Check for updates button and the Hardware Acceleration box, both left out of the port on purpose, so the port's dialog is shorter |
| Options over a puzzle (with Main menu) | Menu on a board (CD) | CD-u1800 | explained: the same two rows left out as in the menu's Options |
| Credits | Options, Credits (A) | A-u1000 | explained: the original's frame is 32 pixels taller with the text in the same place; cause not found (IDENTICAL_PLAY.md) |
| Help screen with two demo boards | Options, How to play (A); before a puzzle (E) | A-u1160 E-u1050 | explained: the demo boards' animation phase (A-u1160) |
| Reset question (24) | Reset on a board (E) | E-u1450 | explained: the god's idle motion over the board's top edge differs (1.4%) |
| Resume question (0x20) | a temple's thumbnail of a puzzle with saved moves (RQ) | RQ-u1950 | explained: the thumbnail under the pointer is lit in the original and not in the port (hover, see the oddity in IDENTICAL_PLAY.md) |
| Pause screen and focus lost | losing the focus on a board (E) | E-u1950 E-u2150 | explained: the god's idle motion (0.2% and 0.9%) |
| Not-yet dialog (0x19) | a tutorial puzzle out of order (CD) | CD-u900 |  |
| Skip-tutorial question (0x1b) | Skip the tutorial (CD) | CD-u1150 |  |
| Locked-temple dialog (0x1c7) | clicking a locked temple (B) | B-u1060 | explained: the gulls are random draws |
| StoryScreen mode 0 (the first story) | Play on a fresh profile after the loading screen (B) | B-u300 | explained: the Tile Master's place in the first frames comes from a draw of the generator reached in another order (IDENTICAL_PLAY.md) |
| StoryScreen modes 1 to 3 (temple openings, ends) | finishing the tutorial or a temple |  | none: needs a finished temple or the whole tutorial played on the original; not shot |
| IslandMap (flock, glow, arrows, panel) | after the story; back from a temple (B, E, I0, I1, C3) | B-u500 B-u700 B-u900 B-u1160 E-u500 E-u2550 I0-u500 I0-u800 I0-u1100 I1-u500 I1-u800 C3-u2150 | explained: gulls, surf and lightning are random draws; the hover under a stationary pointer differs (IDENTICAL_PLAY.md); the new player's first visit and a finished game match |
| IslandPanel (temple page, "Map of" text) | hovering a temple | I1-u500 I1-u800 C3-u2150 | explained: see the hover oddity of the island |
| Island bursts (Rumble, PauseBloom) of a newly opened temple | a temple opened by solving |  | none: needs a temple opened by play; E-u500 shows the port's bloom where the original shows none (see the island oddity) |
| TempleScreen (thumbnails, scroll bar, god's bubble, mystery chains) | temple click (B, E, H) | B-u1300 B-u1500 E-u700 E-u900 E-u2350 E-u2750 H-u2060 H-u2120 | explained: the god's greeting, the chains' sway and the lock's place are random draws, and the hover under a stationary pointer differs; medals and ribbons match since the medals are drawn over all thumbnails |
| Board: tutorial lesson and its bubble | first tutorial puzzle (CD, H) | CD-u1500 CD-u1650 H-u1500 H-u1570 H-u1640 | explained: a tile in flight is an update or two apart (origrun delivers the click); the god's idle motion |
| Board: normal, hint | a puzzle of a temple (E) | E-u1250 E-u1600 E-u1750 | explained: the god's idle motion over the top edge |
| Hint dialog (0x104, sticky spots) | the first puzzle with sticky spots |  | none: not shot; the bubble is the board's own (E-u1750) |
| Board: mystery chains, expert | a mystery puzzle, an expert profile |  | none: not shot; the mystery thumbnails' chains are on the temple screen (E-u2750) |
| PuzzleCompleted (tablet, characters, effects) | solving a puzzle (H) | H-u1800 H-u1815 H-u1830 H-u1850 H-u1870 H-u1900 H-u1930 H-u1960 H-u2000 H-u2030 | explained: the tablet comes about 8 updates earlier in the original (measured with dense shots), and its picture smoothing and sheen differ (IDENTICAL_PLAY.md) |
| CompletionScreen modes 1 to 5 | finishing a temple or the game |  | none: needs a finished temple; not shot |
| Editor | nothing in the export opens it |  | unreachable: nothing in the export opens it, and it is not ported |
