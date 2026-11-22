# Bejeweled 2 Deluxe: every screen

One row per screen or dialog the game can show, from the export and `src/` (the audit of 2026-11-21 in
`build/logs/screen-coverage/bejeweled2.md`, checked against both). The shots are pairs of the run `screens2d`
(`games/bejeweled2/tools/origcmp/screens_run.sh`, `screens_reader.sh`): the port's recording of each tour against the
original's shots of the same inputs, 2D 640x480 windowed, the original in Windows Sandbox. The 3D credits roll was compared on the
desktop lane in run `v3d_finale` (40 pairs, credits all matching, `docs/IDENTICAL_PLAY.md`); the rest of 3D and full screen
are not in this run. Shots are named `<tour>__<shot>`.

"Explained" below means: the More Games button and the pill's text are gone from the main menu and from every screen
that shows it (the user asked for it; box at 384,436), the title's letters and progress bar are at another phase (the
original's load runs on the wall clock, 319 updates warm and 1259 cold), and falling gems and the warp's sparkles and the
shine of a Hyper or Endless board are other random draws (docs/IDENTICAL_PLAY.md, "Oddities").

| Screen | Reached by | Shots | Note |
|---|---|---|---|
| Title, loading | launch | tour__title classic__title | explained: the load's phase |
| New player (CreateUserDialog), Enter Name | first launch; OK with an empty name | tour__newuser dialogs__enter-name | explained: More Games; the OK button is lit in the original (cursor still over it after the click that opened the box), not in the port's recording (oddity) |
| Welcome message (DIALOG_WELCOME) | first visit to the menu | tour__dialog36 | explained |
| Main menu, secret menu | after the title; a secret mode unlocked | tour__menu hyper__menu-secret twilight__menu-secret cognito__menu-secret finity__menu-secret | explained: More Games |
| High scores, reset question | Scores button; Reset | tour__scores dialogs__reset | |
| Options (menu and over a game) | Options button | tour__options tour__options-menu tour__options-menu-after | explained: More Games |
| Option verify (yes/no after a display change) | Options, a display setting | tour__verify-accel tour__verify-backdrops | explained: More Games; the lit button as above. The High Resolution? and Full Screen? question depends on the desktop's width (the original asks Full Screen? on a desktop up to 1024 wide, the port follows the machine): the desktop lane only |
| Load Game? and the loaded board | launch with a saved game | tour__loadgame tour__board-loaded | explained: More Games |
| How To Play, 4 pages | Help button | tour__dialog40 tour__dialog40b tour__dialog40c tour__dialog40d | |
| Boards of the 8 modes with tutorials and big text | each mode's menu button | tour__board tour__board-played classic__board classic__board-played action__board action__board-played hyper__board hyper__board-played twilight__board twilight__board-played endless__board endless__board-played puzzle__board puzzle__board-played cognito__board cognito__board-played finity__board finity__board-played | |
| Mode help boxes | first game of a mode | classic__dialog40 action__dialog40 hyper__dialog40 twilight__dialog40 endless__dialog40 cognito__dialog40 finity__dialog40 | |
| One-time boxes: timer tip, power gem tip, hyper cube tip | a first game with every hint on | classic__hintbox38 action__hintbox38 hyper__hintbox26 hyper__hintbox38 hyper__hintbox39 twilight__hintbox38 twilight__hintbox39 endless__hintbox38 | |
| Level change, Hyperspace whirlpool | a level's end | classic__level2-0 classic__level3-0 action__level2-0 action__level3-0 action__level4-0 hyper__level2-0 hyper__level11-0 twilight__level2-0 twilight__level4-0 endless__level2-0 endless__level5-0 | |
| End screens, scores after them | a timed mode's end | action__over0 action__over3 action__scores-after hyper__over0 hyper__over3 hyper__scores-after | |
| Puzzle world WARP | a puzzle world's first board | puzzle__w0-warp00 puzzle__w0-warp04 puzzle__w0-solved cognito__w0-warp00 cognito__w0-warp30 | explained: the warp's sparkles |
| Quit | Quit button | dialogs__quit | explained: More Games |
| User list, Rename, Delete, Create | Users button | dialogs__users dialogs__users-sel dialogs__rename dialogs__delete dialogs__create | explained: More Games |
| Puzzle packs selector | Puzzle button with more than one pack |  | unreachable: the stock install has one pack ("Galaxy One"), so the game never shows the selector |
| Galaxy map and Planet is Locked | the dome's map button; a locked planet | puzzle__galaxy-map puzzle__galaxy-closed puzzle__galaxy-planet0 puzzle__galaxy-planet0-closed | |
| Puzzle dome (five pods), puzzle completed | a puzzle's board; solving it | puzzle__board puzzle__w0-solved | |
| Puzzle hint warning (secret puzzle, a first hint costs the level's points) | Hint in a secret puzzle |  | rare: only a secret-mode puzzle (Cognito's) with no hint used; no tour reaches a secret puzzle's first hint |
| More Hints? | after an undo to a solvable board |  | rare: needs "No Longer Solvable" first (below) |
| Illegal Move, Still Solvable | a swap that takes nothing; Hint after a hint | puzzle__illegal-move puzzle__illegal-move-closed puzzle__hint-shown puzzle__hint-still puzzle__hint-still-closed | |
| No Longer Solvable (undo), No Hints | Hint on a board with no valid move |  | rare: a puzzle board stays solvable under every move the original lets a player make; a board with no move needs a finished move that leaves the solution, which no recorded input reaches |
| MODE UNLOCKED message | a mode's unlock score; the galaxy's finale | finale__fin-dialog37 | explained: a 1 pixel row under "Back to Main Menu" at 505,467 in the port, before the fix of this pass (below); none now |
| Name conflict, profile error | create a name that exists | dialogs__name-conflict | explained: More Games, the lit button |
| Credits roll (only from the Puzzle finale) | all puzzles solved | finale__fin00 finale__fin02 finale__fin10 finale__fin23 finale__link-menu finale__link-board finale__link-played | explained: link-menu shows the main menu, More Games |
| Puzzle galaxy completion, GALAXY COMPLETED | the last planet solved | finale__fin01 finale__fin05 finale__fin17 | |
| Pause, focus lost | key; window deactivated |  | none: the game's own pause has no screen (the board's draw is frozen) and focus loss is a desktop lane check |
| Cursor and effect overlays (EffectOverlay, ReloadDisplay) | play | finale__fin00 finale__fin02 | |
