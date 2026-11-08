# The Board: class layout and the map of its methods

Job B0. `src/Board.h` is the contract; this file is the map for the three translation jobs that write the Board's methods
(B1 to B3 in JOBS.md). The class is `Sexy::Board : Widget` (vftable 0x5309a4, 47 slots, constructor 0x4544b0, `new(0xf98)`).
Its methods lie in one stretch, 0x43fbf0 to 0x45e755, and are interleaved with other classes' functions; the
table lists only the ones that touch the Board's fields through `this`.

## 1. The methods

There are 53 member functions, 55977 bytes of code in all (the destructor counts as two: the deleting destructor and its body).
Two more functions in the vtable, 0x440310 (slot 19) and 0x440320 (slot 18), are `jmp` thunks to Widget's own LostFocus /
GotFocus; they need no override. Slot 3 and slot 23 point at the empty stub 0x4e2ef0 and the rest at Widget. Names are the
port's: roles are guessed where the table says so in `Board.h` and the others come from the code.

| address | size | method | .cpp | job | note |
| --- | ---: | --- | --- | --- | --- |
| 0x43fbf0 | 163 | RemovedFromManager | Board.cpp | B2 | slot 2 |
| 0x43fca0 | 60 | GetLevelBonus | BoardDraw.cpp | B2 | Draw and Update call it |
| 0x43fce0 | 54 | GetBossBonus | BoardDraw.cpp | B2 | Draw and HandleBossEvents call it |
| 0x43fd30 | 86 | IsBelowSurface | BoardUpdate.cpp | B1 | only Update calls it |
| 0x43fd90 | 509 | ShowRating | BoardUpdate.cpp | B1 | Update and the event loops call it |
| 0x43ff90 | 127 | GetTypingSpeed | BoardDraw.cpp | B2 | Draw and SaveProgress call it |
| 0x440010 | 109 | GetNextGoal | BoardDraw.cpp | B2 | only Draw calls it |
| 0x440080 | 174 | GetStats | BoardGame.cpp | B3 | WinWSApp calls it |
| 0x440130 | 128 | StartGame | BoardGame.cpp | B3 | WinWSApp calls it |
| 0x4401b0 | 149 | SetVisible | Board.cpp | B2 | slot 4 |
| 0x440250 | 49 | SetDisabled | Board.cpp | B2 | slot 10 |
| 0x440290 | 89 | GetFinishBonus | BoardDraw.cpp | B2 | Draw and Update call it |
| 0x4402f0 | 25 | ResetPiranhaSpeed | BoardUpdate.cpp | B1 | PlayerEaten and two event loops call it |
| 0x440330 | 570 | BuildWaterImage | BoardDraw.cpp | B2 | only Draw calls it |
| 0x440570 | 16 | SetFastCPU | BoardGame.cpp | B3 | WinWSApp calls it |
| 0x440580 | 836 | SaveProgress | BoardGame.cpp | B3 | WinWSApp and PlayerEaten call it |
| 0x4408d0 | 636 | GetSpotOffset | BoardUpdate.cpp | B1 | only Update calls it |
| 0x440b70 | 30 | IsHudActive | Board.cpp | B2 | called by the StatusBar |
| 0x440b90 | 132 | UpdateDangerMusic | BoardUpdate.cpp | B1 | only Update calls it |
| 0x440c20 | 142 | MouseMove | Board.cpp | B2 | slot 25 |
| 0x440cb0 | 10 | IsEmpTimerPaused | Board.cpp | B2 | called by the StatusBar |
| 0x441ac0 | 233 | DrawWavyImage | BoardDraw.cpp | B2 | only Draw calls it |
| 0x441bb0 | 87 | CountActiveLists | BoardUpdate.cpp | B1 | Update and the event loops call it |
| 0x442100 | 481 | SetPaused | BoardGame.cpp | B3 | WinWSApp calls it |
| 0x4423f0 | 280 | GetStreakBonus | BoardDraw.cpp | B2 | Draw and KeyChar call it |
| 0x442520 | 32 | KeyDown | Board.cpp | B2 | slot 21, empty |
| 0x442540 | 531 | PlaceBonusCreatures | BoardWaves.cpp | B3 | only SpawnWave calls it |
| 0x442760 | 277 | ShowTreasure | BoardUpdate.cpp | B1 | HandleClamEvents and Update call it |
| 0x443ae0 | 302 | TryLetter | BoardKeys.cpp | B3 | only KeyChar calls it |
| 0x445eb0 | 602 | AddedToManager | Board.cpp | B2 | slot 1: makes the link widget (id 300), adds the nine timers |
| 0x446110 | 1541 | AvoidWordOverlap | BoardUpdate.cpp | B1 | only Update calls it |
| 0x446720 | 534 | PickClamWord | BoardUpdate.cpp | B1 | only Update calls it |
| 0x446950 | 327 | GetRankTitle | BoardDraw.cpp | B2 | Draw and PlayerEaten call it; the six rank texts |
| 0x446aa0 | 112 | SetBannerText | BoardKeys.cpp | B3 | only KeyChar calls it |
| 0x44d200 | 11215 | Draw | BoardDraw.cpp | B2 | slot 16, the biggest function of the game |
| 0x44fe30 | 735 | PlayerEaten | BoardUpdate.cpp | B1 | only Update calls it; makes the GameOverDialog |
| 0x451700 | 1456 | ~Board (body) | Board.cpp | B2 | frees the lists, the markers, the timers, the player, the popup, the map |
| 0x451cb0 | 926 | SetLessonManager | BoardGame.cpp | B3 | WinWSApp calls it |
| 0x452050 | 2562 | PlaceEnemies | BoardWaves.cpp | B3 | only SpawnWave calls it |
| 0x452a60 | 645 | AddPiranha | BoardWaves.cpp | B3 | KeyChar calls it |
| 0x452cf0 | 567 | SetDepth | BoardGame.cpp | B3 | WinWSApp calls it |
| 0x452f30 | 145 | AddBubble | BoardGame.cpp | B3 | KeyChar, the event loops and TorpedoBoss call it |
| 0x452fd0 | 223 | AddBonusText | BoardGame.cpp | B3 | KeyChar and the event loops call it |
| 0x4544b0 | 2398 | Board(WinWSApp*) | Board.cpp | B2 | constructor: the Player (0x461320), nine WSTimers, sin table, RatingPopup, HudBar, the lists |
| 0x454e10 | 33 | ~Board (deleting destructor) | Board.cpp | B2 | calls the body at 0x451700, then Widget's |
| 0x454e40 | 3490 | KeyChar | BoardKeys.cpp | B3 | slot 20 |
| 0x455bf0 | 6285 | SpawnWave | BoardWaves.cpp | B3 | builds the boss by type, the sharks, piranhas, bonus creatures, fills mRecordsByType |
| 0x457480 | 1604 | HandleSharkEvents | BoardEvents.cpp | B1 | the sharks' event loop |
| 0x457ad0 | 1356 | HandlePiranhaEvents | BoardEvents.cpp | B1 | the piranhas' event loop |
| 0x458030 | 648 | HandleClamEvents | BoardEvents.cpp | B1 | the clams' event loop |
| 0x4582c0 | 986 | HandleBonusCreatureEvents | BoardEvents.cpp | B1 | the bonus creatures' event loop |
| 0x4586b0 | 1521 | HandleBossEvents | BoardEvents.cpp | B1 | the boss's event loop |
| 0x45c140 | 9749 | Update | BoardUpdate.cpp | B1 | slot 17; holds the registration test (part 4) |

Functions inside the same address range that are not Board methods: class `HudBar`, the strip at the top of the screen
(constructor 0x441730, 44 bytes; Update 0x441760, 134; Draw 0x449910, 809; 0x1c bytes: the diver, the app, the app's font +0x8bc, a
flash alpha and a counter; it flashes while the diver's mFlag13d is set and draws "Lives left" and the level and points line),
which no B job covers (give it to B2, 987 bytes), and the StatusBar's, TitleScreen's, MapScreen's and the dialogs' functions
(S1, W1, D1).

## 2. The translation jobs

The split follows the callers: a function only Update calls goes with Update. Each job writes its `.cpp` files against
`Board.h` and adds nothing to the header without telling the coordinator (a renamed field is renamed in every file:
`grep -rn mFieldXXX games/typershark/src`).

**B1** (20.4 KB). Update and everything that runs from it each tick: the creature lists' event loops, the losing of a life, the treasure.

- `BoardEvents.cpp` (6115 bytes): HandleSharkEvents 0x457480, HandlePiranhaEvents 0x457ad0, HandleClamEvents 0x458030, HandleBonusCreatureEvents 0x4582c0, HandleBossEvents 0x4586b0.
- `BoardUpdate.cpp` (14311 bytes): IsBelowSurface 0x43fd30, ShowRating 0x43fd90, ResetPiranhaSpeed 0x4402f0, GetSpotOffset 0x4408d0, UpdateDangerMusic 0x440b90, CountActiveLists 0x441bb0, ShowTreasure 0x442760, AvoidWordOverlap 0x446110, PickClamWord 0x446720, PlayerEaten 0x44fe30, Update 0x45c140.

**B2** (18.1 KB). The constructor, the destructor and the Widget overrides, and Draw with the bonus and title helpers that only it calls.

- `Board.cpp` (5064 bytes): RemovedFromManager 0x43fbf0, SetVisible 0x4401b0, SetDisabled 0x440250, IsHudActive 0x440b70, MouseMove 0x440c20, IsEmpTimerPaused 0x440cb0, KeyDown 0x442520, AddedToManager 0x445eb0, ~Board (body) 0x451700, Board(WinWSApp*) 0x4544b0, ~Board (deleting destructor) 0x454e10.
- `BoardDraw.cpp` (13064 bytes): GetLevelBonus 0x43fca0, GetBossBonus 0x43fce0, GetTypingSpeed 0x43ff90, GetNextGoal 0x440010, GetFinishBonus 0x440290, BuildWaterImage 0x440330, DrawWavyImage 0x441ac0, GetStreakBonus 0x4423f0, GetRankTitle 0x446950, Draw 0x44d200.

**B3** (17.4 KB). The wave spawner, the keyboard, and the calls WinWSApp makes (stats, pause, lesson, depth, bubbles and texts).

- `BoardGame.cpp` (3496 bytes): GetStats 0x440080, StartGame 0x440130, SetFastCPU 0x440570, SaveProgress 0x440580, SetPaused 0x442100, SetLessonManager 0x451cb0, SetDepth 0x452cf0, AddBubble 0x452f30, AddBonusText 0x452fd0.
- `BoardKeys.cpp` (3904 bytes): TryLetter 0x443ae0, SetBannerText 0x446aa0, KeyChar 0x454e40.
- `BoardWaves.cpp` (10023 bytes): PlaceBonusCreatures 0x442540, PlaceEnemies 0x452050, AddPiranha 0x452a60, SpawnWave 0x455bf0.

B1 is the longest, but 6 KB of it are five similar event loops. B2's Draw is one 11 KB function that touches 79 of the
Board's own offsets; its helpers are small. B3's wave spawner (6.3 KB) builds every kind of creature, so it needs the finished entity
headers (all done). No job needs a method of another: where one calls a function of another job it uses the declaration in
`Board.h`, and the build links once all three are in.

## 3. Fields whose role is unknown

Named after their offset in `Board.h`; the comment there says what the code does with each. Offsets in hex.

51 fields are named by offset (51 of the 109 in the Board proper).

| offset | field | type | what is known |
| --- | --- | --- | --- |
| +0x60 | mField060 | int | constructor 0, never used |
| +0x154 | mField154 | int | ++ every other tick (constructor 0) |
| +0x158 | mField158 | int | ++ every fourth tick (constructor 0) |
| +0x1c0 | mField1c0 | int | constructor 0; SetDepth sets it |
| +0xd10 | mField0d10 | double | y of a scrolling picture of the sea (constructor: the Widget's +0x14); SetLessonManager: mField0e10 + 270; Update moves it with the dive |
| +0xd18 | mField0d18 | double | the same for the next picture; SetLessonManager: mField0e10 + 120 |
| +0xd20 | mField0d20 | double | the same for the last picture; SetLessonManager: mField0e10 - 30 |
| +0xd28 | mFlag0d28 | bool | constructor false; SetDepth sets it; SpawnWave clears it and in an abyss game builds a boss when it was set |
| +0xd2c | mField0d2c | int | constructor -1; the first Update sets it from the profile; SpawnWave takes it mod 4 to pick the abyss boss (0 TorpedoBoss, 1 MechaShark...) |
| +0xd38 | mFlag0d38 | bool | constructor false; KeyChar sets it |
| +0xd60 | mFlag0d60 | bool | constructor false, never used |
| +0xd64 | mField0d64 | int | constructor 0; SpawnWave zeroes it; HandleBonusCreatureEvents counts in it |
| +0xe10 | mField0e10 | double | constructor: the Widget's +0x14; SetLessonManager: the y of the ruler's mOceanDepth mark; Draw compares it with the height |
| +0xe50 | mTimer0e50 | WSTimer* | starts disabled; the fade of mField0e54 waits 2000 ms of it |
| +0xe54 | mField0e54 | int | PlayerEaten sets 255; Update takes 3 off a tick once mTimer0e50 has passed 2 s (diver in state 1 or 9) |
| +0xe8c | mField0e8c | int | constructor 0; GetStats gives it out (AppStats word 5) and GetFinishBonus reads it |
| +0xe90 | mField0e90 | int | the diver's score at the start of the game (constructor, StartGame) |
| +0xe94 | mField0e94 | int | KeyChar counts in it and compares it with a lesson value; ResetPiranhaSpeed zeroes it |
| +0xea4 | mField0ea4 | int | constructor 255; Draw uses it as the alpha of the opening hint text ("Type the words on the sharks...") |
| +0xea8 | mFlag0ea8 | bool | constructor false; KeyChar sets it |
| +0xeb4 | mFlag0eb4 | bool | the EMP charge is draining (Update takes 0.01 off a tick) |
| +0xeb5 | mFlag0eb5 | bool | constructor false; the event loops add mTypeTimer's time to mTypeTime only while it is false |
| +0xebc | mFlag0ebc | bool | constructor false; HandleBossEvents sets it when a boss is beaten outside an abyss game |
| +0xec0 | mField0ec0[9] | int | constructor: zeros, except [5] = random 4..7 and [8] = random 8..11; only Draw uses them (three groups of three) |
| +0xee4 | mField0ee4 | int | constructor 0; Update counts it; IsHudActive tests it against 0x10 |
| +0xee8 | mImage0ee8 | Image* | constructor: the app's mImages at byte offset 0x324 (index 0xc9) |
| +0xeec | mImage0eec | Image* | constructor: the app's mImages at byte offset 0x338 (index 0xce) |
| +0xef0 | mImage0ef0 | Image* | constructor: the first entry of the app's mSonarSweepImages (+0x7cc); Update takes entries 1 to 4 of it in the sonar animation of diver state 7 |
| +0xef4 | mImage0ef4 | Image* | constructor: the first entry of the app's mSonarSweep2Images (+0x7dc); Update takes entries 1 to 4 of it in the sonar animation of diver state 7 |
| +0xef8 | mField0ef8 | int | constructor 255 |
| +0xefc | mFlag0efc | bool | constructor false |
| +0xf00 | mTimer0f00 | WSTimer* | starts disabled; stopped and reset when the level ends |
| +0xf04 | mField0f04 | int | constructor 255; only Draw reads it |
| +0xf08 | mFlag0f08 | bool | constructor false; Draw reads it |
| +0xf0c | mField0f0c | int | KeyChar adds GetStreakBonus to it; SaveProgress / Update keep the best in the profile (+0x64) |
| +0xf10 | mString0f10 | std::string | (0x1c bytes) KeyChar clears it when mField0f2c reaches its length; Draw reads it |
| +0xf2c | mField0f2c | int | KeyChar counts it up to the length of mString0f10 |
| +0xf30 | mField0f30 | int | GetStreakBonus uses it; SaveProgress / Update keep the best in the profile (+0x68) |
| +0xf38 | mField0f38 | double | constructor 0; KeyChar takes 5.0 off, Draw reads it |
| +0xf40 | mFlag0f40 | bool | constructor false |
| +0xf44 | mField0f44 | int | constructor 255 |
| +0xf48 | mField0f48 | int | constructor 0 |
| +0xf60 | mField0f60 | int | KeyChar counts it; SpawnWave zeroes it |
| +0xf64 | mField0f64 | int | KeyChar counts it; SpawnWave zeroes it |
| +0xf68 | mField0f68 | int | KeyChar counts it; GetStats gives it out (AppStats word 8) |
| +0xf6c | mField0f6c | int | constructor 0; GetStats gives it out (AppStats word 9) |
| +0xf70 | mField0f70 | int | KeyChar counts it |
| +0xf74 | mField0f74 | int | KeyChar counts it |
| +0xf80 | mFlag0f80 | bool | constructor true; AddPiranha clears it; the event loops and Update set it again |
| +0xf84 | mField0f84 | int | constructor: the +8 field of the app's mImages at byte offset 0x7c (index 0x1f) |
| +0xf88 | mField0f88 | int | constructor: the +8 field of the app's mImages at byte offset 0x5c (index 0x17); IsBelowSurface compares mDivePos with it |

Also unnamed inside embedded objects: `Smoother::mField38` and `FrameSmoother::mField38` (constructor zero, never used),
`BgLayer::mField00` / `mField04` / `mUnused10`, `RatingPopup::mField34`. Gaps in the original's layout that have no field at all
(alignment): after each bool, and four bytes at +0x15c, +0x1cc, +0xe0c and +0xf34 before a double; the compiler makes them again.

## 4. Registration and trial fields

There is one. `mMaxUnregAbyssDepth` (+0x150) is read from the registry value `MaxUnregAbyssDepth` (default 600) by the
constructor and used in one place, `Update` (0x45c140), in the middle of the function: when the lesson is an
abyss game (`LessonManager` +0xa5), no dialog is open (app +0x688 below 1), the app's byte at +0x470 is clear and the score has
reached the limit, the Board calls `SaveProgress(true, false)`, builds the message ("in the unregistered version you can only
dive up to N feet", string at 0x531648), shows it in a dialog through app 0x41c1c0 and calls `SetPaused(true)`. B1 leaves the whole
block out, and B2 leaves out the constructor's registry read (the field stays declared so the offsets stay the original's;
the constructor sets it to 0). Nothing else in the 53 functions tests registration. The
"HINT" boxes (the Shark Zapper hint in Update and the boss warnings in `SpawnWave`) are tutorial text, not registration:
they stay.

## 5. The event codes of `Enemy::mEvents`

Each creature queues codes in `mEvents` (Enemy.h) and the Board drains them in the five handlers, through `PopEvent` (0x407070;
0 means the queue is empty), `HasEvent` (0x403790) and `RemoveEvent` (0x407100). Meanings are the handlers', as far as they show.
A handler returns true after it removed the creature from its list (the tail path shared by code 5).

| code | pushed by | meaning | handled in |
| ---: | --- | --- | --- |
| 1 | Enemy::TypeLetter (last letter), BlackShark, RedShark, WhitePiranha, Clam (opening ended), Torpedo, TorpedoBoss | the word is finished (for a clam: it has opened). The sharks', piranhas', bonus creatures' and boss handler score it: points x (score / 600, at least 2, in the abyss), add the type time, rating points, AddScore, a "+N PTS" text, ResetPiranhaSpeed, clear mTarget and mTyped | all five |
| 2 | Shark, Piranha (reached the diver); Clam (closing ended) | a shark or piranha came in and bit: the diver gets state 3 unless it is in state 2 or 3 (and Player +0x144 is set); for a clam: it has closed | sharks, piranhas, clams |
| 3 | Clam (SetState(1) of a clam with a pearl) | a clam with a pearl opened: pairs it with mOpenClam (+100000, then +200000 points) or becomes mOpenClam | clams |
| 4 | Shark, Piranha (after 7, at the left edge) | every other swimming shark in the list gets its slot 7 called | sharks, piranhas |
| 5 | Shark, Piranha, BonusCreature, Torpedo, ProjectileBoss, Projectile (after 0x15) | left the screen: the creature leaves its list and is deleted (a boss: mBoss = NULL); the typing target is cleared if it was this one | sharks, piranhas, bonus creatures, boss |
| 6 | Shark, Piranha, BonusCreature (before 5), Torpedo, TorpedoBoss | the creature died: once every creature of the list has finished dying and CountActiveLists says it was the last list, the group is over (ShowRating, or mRatingTimer restarts); the boss handler has a case for it too | sharks, piranhas, bonus creatures, boss |
| 7 | Shark, Piranha (before 4, at the left edge) | the creature's GameObject slot 6 is called (the turn at the left edge) | sharks, piranhas |
| 8 | StealthShark, StealthPiranha (first time x <= 600) | 30 bubbles are added around the creature and a sound plays | sharks, piranhas |
| 9 | nobody in the finished files | no handler found in the Board either; the E-jobs' list of codes named it | none |
| 0xa | ProjectileBoss constructor | the boss exists: the handler touches mTimer0e50 (SetDisabled); the "DANGER!" hint dialog is shown by SpawnWave and the level start, not here | boss |
| 0xb | ProjectileBoss::SetState(7) | no case in the Board's handlers | none |
| 0xc | ProjectileBoss::SetState(8) when its hit points are gone | the boss is beaten: the profile counts it (a field of the user outside an abyss game, the boss list in an abyss game), GetBossBonus points, and when the diver is in state 9 the level ends | boss |
| 0xd | TorpedoBoss (mine/bubble shot) | no case in the Board's handlers | none |
| 0x13 | Projectile (HitsPlayer), TorpedoBoss | a shot hit the diver: events 1 and 0x13 are removed, mTarget and mTyped are cleared, the type timer restarts and the diver gets a new state | boss |
| 0x14 | Projectile (HitsTarget) | a shot hit its target; no case in the Board's handlers (the boss handles its own shots) | none |
| 0x15 | Projectile, TorpedoBoss | a shot is gone (it went off the left edge): if there is a typing target it is released, the letters typed are cleared and the type timer restarts | boss |

## 6. What else the class does that a translator should know

The Board holds seven lists of objects; each holds a pointer to the object's first base (the Enemy part for the sharks, piranhas, clams and bonus creatures): sharks (+0xd74),
piranhas (+0xd84), clams (+0xd94), bubbles (+0xda4), bonus texts (+0xdb4), bonus creatures (+0xdc4, the creatures of enemy record
type 11), background fish (+0xdd4); the depth ruler's labels are an eighth list of `DepthMarker*` (+0xf4c). A boss is held in
one pointer (+0xeb8) and not in a list. The nine `WSTimer`s (+0x58, +0xd34, +0xe08, +0xe18, +0xe50, +0xe5c, +0xe68, +0xeac,
+0xf00) are Widgets that the Board adds to the widget manager in `AddedToManager`; `SetPaused` adds the pause time to six of
them so the pause does not count. Embedded objects (not pointers): `AppStats` (+0xe20), two `Smoother`s (+0x68), a
`FrameSmoother` (+0x108), two `BgLayer`s (+0x170), a `std::map` (+0x1b0), three `std::string`s (+0xde4, +0xe6c, +0xf10)
and a table of 360 sines (+0x1d0, 2880 bytes). `RatingPopup` (+0xf5c) and the `HudBar` (+0xe04) are pointers.

Signs of inlined helpers: the compiler's `std::vector<T*>::erase` loops appear as the same "copy the tail down by one" code
in every event loop and in Update; `Insert_n` and `push_back` appear as calls (0x44c900 and 0x450bf0 are `std::map`'s insert and
`operator[]`; 0x4422f0 is its tree erase). A `Shark` or `Piranha` pointer is got by adding a virtual-base offset (the original reads the
offset from the virtual-base table); the port stores the Enemy pointer already and uses `dynamic_cast` where the Board needs a Shark
(ENTITIES.md). `GetStats` returns the block by hidden pointer; `StartGame` takes it by value (12 dwords on the stack), the port
by const reference.

`Player` is not a `LevelConfig`: the earlier stub header called the +0xd68 object that and gave it `mGameMode`; it is the
diver (`Player.h`), and `mGameMode` is `GameObject::mState`.
