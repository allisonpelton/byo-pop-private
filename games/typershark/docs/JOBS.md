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
| P2 | the config parser 0x42bfb0 (11.8 KB) and the lessons / gem names / words / themes loaders | ~25 KB | |
| D1 | dialogs: copy-adapt BookWorm's (Delete/Select/New user, Options, Credits, Hint, Money), the game's own (WinGame, GameOver, Difficulty, AbyssMode) | ~30 KB | |
| W1 | small widgets and panels: StatsScreen, StatusBar, TabbedWidget, TabPanel, TutorialPanel, AbyssPanel, AdventurePanel | ~20 KB | |
| S1 | screens: TitleScreen, MenuScreen, MapScreen, HighScoreScreen | ~25 KB | |
| T1 | TypingTutor | ~12 KB | |
| B1-B3 | Board in three parts (Update 0x45c140 9.7 KB, Draw 0x44d200 11.2 KB, KeyChar 0x454e40 and the rest) | ~61 KB | |

`python games/typershark/tools/coverage.py [--range LO HI] --missing` shows what the sources do not mention yet (the
mentions are the `// 0x4xxxxx` comments before each function), by guessed class.

## How the coordinator checks a job's result

Syntax-check the files, read the headers, spot-check the three or four most complex functions against `decread`
(or the machine code where decread is wrong: x87 comparisons, virtual-base accesses), grep for `TODO` and for
invented names that the original does not have, and fix what is wrong before the next job. Commit after each job.
