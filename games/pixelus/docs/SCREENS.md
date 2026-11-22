# Screens and dialogs of Pixelus

Every screen and dialog the port can show, taken from the export and the source, with the shots that compare each one
against the original (see `docs/SHOTDIFF.md`, "Every screen, listed"). The original runs in 2D only (no original has
run in 3D in Windows Sandbox), so the 2D port is compared with the original and the 3D port with the 2D port. Scenario
scripts are under `games/pixelus/tools/screens/`; a shot is named `<scenario>-<update>`.

| Screen | Reached by | Shots | Note |
|---|---|---|---|
| LoadingScreen (bar, "click here to play" link) | launch | menu_tour `loading` | draws the "Deluxe" banner of the image; see the deviation in IDENTICAL_PLAY.md |
| MainMenu ("Welcome", player name, Play/Options/Quit, "if this is not you") | after loading | scenario A | |
| New user dialog (0x13) with the expert checkbox | menu Play on a fresh profile | scenario A | |
| Empty-name error (0x16) | New user OK with no name | scenario A | |
| Name-exists error (0x16) | New user OK with a taken name | scenario B | |
| Expert warning (0x1e) | ticking the expert box | scenario A | |
| Select user dialog (0x15) | menu "if this is not you" | scenario A | |
| Rename user dialog and rename error (0x17) | select user, Rename | scenario A, B | |
| Delete user question (0x14) | select user, Delete | scenario A | |
| Quit question (4) | menu Quit | scenario A | |
| Options (2002) | menu Options | scenario A, option checks in B | |
| Credits | Options, Credits | scenario A | |
| Help screen with two demo boards | Options, How to play | scenario A | |
| Reset question (24) | Options on a puzzle, Reset | scenario C | |
| Resume question (0x20) | Play with a puzzle in progress | scenario C | |
| StoryScreen modes 0 to 3 (intro, temple conversations) | Play, temple first visits | scenario D | |
| IslandMap and IslandPanel (flock, Rumble, PauseBloom, bubbles) | after the story | scenario D | |
| Locked-temple dialog (0x1c7) | clicking a locked temple | scenario D | |
| TempleScreen (thumbnails, scroll bar, god's bubble) | temple click | scenario E | |
| Not-yet dialog (0x19) | a mystery puzzle or a tutorial out of order | scenario E | |
| Board: tutorial lessons, hint bubbles, glows, flash buttons | first temple | scenario F | |
| Skip-tutorial question (0x1b) | tutorial board | scenario F | |
| Hint dialog (0x1f, 0x104) | Hint button | scenario F | |
| Board: normal, mystery chains, expert | puzzles | scenario G | |
| Pause screen and focus lost | pause button, losing the focus | scenario G | |
| PuzzleCompleted (temple characters, effects) | solving a puzzle | scenario H | |
| CompletionScreen modes 1 to 5 | finishing a temple or the game | scenario I | |
| Editor | unreachable: nothing in the export opens it, and it is not ported | | |

Rows with a scenario letter are filled in as the scenarios are run; this file is the plan until each row names its shots.
