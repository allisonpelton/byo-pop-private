# Translation jobs for the Sonnet agents (one at a time) and their status

Every job's prompt is: "Translate job <id> of games/typershark/docs/JOBS.md. Read docs/TRANSLATE_BRIEF.md first (and
docs/ENTITIES_BRIEF.md and docs/ENTITIES.md for the entity jobs), then WinWSApp.h and the headers it names. Write each
file as soon as its functions are done (an agent that writes nothing for an hour loses all of it), run the brief's
syntax check on every .cpp, and report: the files, what you assumed, what you left (function, reason)." The agent
stops after about 25 KB of original code; a job whose report lists leftovers gets a follow-up job.

Scope rules for all jobs (CLAUDE.md, news.md of 2026-11-07 / 08): registration code (trial, nag, unlock, register and
buy paths, RegisterDialog, MaxPlays...) is not translated; neither is any button or link that leads out of the game
(More Games, Buy, the web site buttons): leave the neighbours' layout as the original has it and list what was left
out and where in the file's header comment, so the later removal of the gap is easy. Update checks and anything that
talks to a server are left out. User-visible text goes through `GT(0xADDR)`.

| id | what | original code | status |
| --- | --- | --- | --- |
| J1 | WinWSApp core: ctor/dtor, init, loading thread, the app's helpers, lessons (0x41ad70-0x428500 and 0x42b..-0x437960) | ~54 KB | done 2026-11-08 (WinWSApp.h/.cpp, WinWSAppScreens.cpp; leftovers listed under J2) |
| J2 | ButtonDepress (0x431630, 8.5 KB) and the rest of WinWSApp that J1 left: 0x42a870 (reset, then ShowMenuScreen), 0x42ee10 and 0x42f3c0 (build the LessonManager via 0x4270e0 and 0x42bfb0, then Board, StatusBar, MapScreen), 0x429450, 0x426c60, 0x429110, 0x4292b0, 0x428d80, 0x42aad0, 0x430b20, 0x426a40, 0x4262b0 (GetDefaultUserNumber), 0x413ff0, 0x4137d0 (LessonManager::IsTutorialActive); also the declared loaders LoadWaves, LoadWordLists, LoadGemNames, LoadShipNames, LoadThemes; WinWSApp's destructor frees lesson manager and config tables | ~25 KB | done 2026-11-08 (WinWSAppButtons.cpp, WinWSAppGame.cpp) |
| E1 | entities: the sharks' and piranhas' variants and Projectile | ~13 KB | done 2026-11-08 (11 of 14 classes; Torpedo, CannonBall, SquidCannonBall went to E2) |
| E2 | entities: Torpedo, CannonBall, SquidCannonBall, MechaTorpedo, ProjectileBoss, TorpedoBoss, GhostShip, GhostShark, MechaShark, MechaSquid | ~18 KB | done 2026-11-08 |
| E3 | entities: BonusCreature, Clam, Player, Bubble, BonusText, BackgroundFish, Effect, and whatever of 0x401000-0x412330 `coverage.py --missing` still lists (GameObject / Enemy leftovers, unknown functions) | ~14 KB | done 2026-11-08 (the range 0x401000-0x412330 is covered) |
| P1 | UserProfileMgr / UserProfile (0x4172f0-0x428000) | ~25 KB | done 2026-11-08 (UserProfile.h/.cpp, UserProfileMgr.h/.cpp) |
| P2 | the config parser 0x42bfb0 (11.8 KB) and the lessons / gem names / words / themes loaders | ~25 KB | done 2026-11-08 (LessonManager.h/.cpp, LessonManagerConfig.cpp, WinWSAppLoad.cpp; the app's tables typed and renamed) |
| D1 | dialogs of the game's own: GameOverDialog (ctor 0x43adb0; Board.h assumes `mField11c`, `mRankTitle`, `mField144`), WinGameDialog, DifficultyDialog, AbyssModeDialog, KeyDialog, HintDialog, and the `(unknown)` functions among them (0x43bcc0, 0x43bf10, 0x43d050, 0x43a2d0, 0x43a3e0, 0x43ed20 ...) | ~25 KB | done 2026-11-08 (the six classes; 0x43bcc0 NewUserDialog, 0x43bf10 and 0x43d050 are D2's) |
| D2 | dialogs copy-adapted from BookWorm's (`games/bookworm/src`): SelectUserDialog, OptionsDialog, NewUserDialog, DeleteUserDialog, and the WinWSApp-attributed functions 0x43dbd0, 0x43df60, 0x43e7a0, 0x43edb0, 0x43f0f0 (the dialogs' button handlers); RegisterDialog, MoneyDialog and CreditsDialog are NOT translated (registration, shop) | ~15 KB done 2026-11-08 (the four classes; the options dialog's Register and partner-link buttons left out; dialog 0x30c is the credits dialog) |
| W1 | small widgets and panels: StatusBar (HideButtons 0x441f10 and ShowButton 0x441f80 are assumed by Board), TabbedWidget, TabPanel, TutorialPanel, AdventurePanel, AbyssPanel, StatsScreen, the XWidget and FadeButton leftovers | ~24 KB done 2026-11-08 (MapScreen's 0x4538e0, 0x4537b0, 0x4506a0 left for S1) |
| S1 | screens: TitleScreen (AddControls 0x448f40), MenuScreen, HighScoreScreen, MapScreen (ctor 0x458d10, 8 KB; S1 = Title, Menu, HighScore, S2 = MapScreen and its helpers) | ~22 KB | S1 and S2 done 2026-11-08 |
| T1 | TypingTutor (ctor 0x460b30) and the leftovers `coverage.py --missing` still lists | ~15 KB | done 2026-11-08 (TypingTutor.h/.cpp, TypingTutorDraw/Keys/Lessons.cpp; the first full build links) |
| B0 | Board.h and docs/BOARD.md: the class layout and the map of its methods | header only | done 2026-11-08 (Board.h: 109 fields, 52 declarations for 53 functions; the finished files compile against it) |
| B1-B3 | Board in three parts, as BOARD.md splits them: B1 BoardUpdate.cpp and BoardEvents.cpp (Update 0x45c140 and what runs from it, the five event loops; 20.4 KB), B2 Board.cpp and BoardDraw.cpp (constructor, Widget overrides, Draw 0x44d200 and its helpers, the HudBar class; 18.1 KB), B3 BoardWaves.cpp, BoardKeys.cpp and BoardGame.cpp (SpawnWave 0x455bf0, KeyChar 0x454e40, what WinWSApp calls; 17.4 KB) | ~56 KB | B1 done 2026-11-08 (BoardUpdate.cpp, BoardEvents.cpp); B2 done 2026-11-08 (Board.cpp, BoardDraw.cpp, HudBar.h/.cpp); B3 open |

`python games/typershark/tools/coverage.py [--range LO HI] --missing` shows what the sources do not mention yet (the
mentions are the `// 0x4xxxxx` comments before each function), by guessed class.

## How the coordinator checks a job's result

Syntax-check the files, read the headers, spot-check the three or four most complex functions against `decread`
(or the machine code where decread is wrong: x87 comparisons, virtual-base accesses), grep for `TODO` and for
invented names that the original does not have, and fix what is wrong before the next job. Commit after each job.
