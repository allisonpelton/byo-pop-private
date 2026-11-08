# What the translated files assume of classes that are not translated yet

Each translation job (J1, J2, E1, E2...) wrote against throwaway stub headers, because the classes it calls come from
later jobs. This file collects what they call, from their reports (names are the jobs' own; the address is the
original's). A job that writes one of these classes reads its section first: it supplies these members with these
signatures, or changes the callers (grep the callers in `src/`) when the original shows the name or type is wrong, and
says so in its report. Fields named after their offset (`mField1c4`) are placeholders: a job that learns the role
renames the field everywhere (`grep -rn mField1c4 games/typershark/src`).

## Board (job B1-B3; ctor 0x4544b0)
`Board(WinWSApp*)`, `mPaused` (bool, +0xe60, read by every creature's Draw), `LevelConfig* mLevelConfig` (+0xd68, with
`mGameMode` +0xb8, `mField0f4`, `double mDouble0f8`, `mField110`, `mField114`), `mField1c4` (the score, +0x1c4),
`mField0e1c`, `mField0f94` (a widget), `AppStats GetStats()` (0x440080, the 0x30-byte block by value),
`SaveProgress(bool,bool)` (0x440580), `StartGame(const AppStats&)` (0x440130), `SetFastCPU(bool)` (0x440570),
`SetLessonManager(LessonManager*)` (0x451cb0), `SetDepth(int)` (0x452cf0), `SetPaused(bool)` (0x442100),
`AddBubble(int x, int y)` (0x452f30; the argument order is from the push order, unverified). The Board casts a `Shark*`
or `Piranha*` obtained through a virtual base with `dynamic_cast` (ENTITIES.md). `WinWSApp::mBoard` is +0x634.
`BlackShark::mWord2` and `RedShark::mWord3` are filled by the Board (`GetWord2` / `GetWord3` in its code);
`WhitePiranha::SetWord2(std::string)`. Event codes the creatures push on `Enemy::mEvents` (the Board drains them): 1
word finished, 2 came in, 4, 5 left the screen, 6, 7, 9, 0x13 hit the player, 0x14 hit a target, 0x15 then 5 left the screen
(Projectile).

## LessonManager (job P2, done: LessonManager.h/.cpp, LessonManagerConfig.cpp, WinWSAppLoad.cpp)
Translated; the header is the contract now. It is the original's unnamed 0xc0-byte class (ctor 0x4270e0, dtor 0x421c50) and not
the RTTI class Sexy::LessonManager (vftable 0x52c728, dtor body 0x427300), which is the typing tutor's lesson list and belongs to
the TypingTutor job. Renames the job made in the callers: `IsTutorialActive` is `IsBonusLevel`, `GetBossRecord` and
`GetBossRecordCount` are `GetEnemyRecord` and `GetEnemyRecordCount` (the records are the wave's enemies, boss waves included),
`mField0a8` is `mEndlessCount`, and the boss record's `mField4c` / `mField50` are `mMinMissiles` / `mMaxMissiles`. The code reads
the Board's `mField1c4` (the score) in `NextLevel` of the abyss. WinWSApp fields renamed or typed: `mField680` is `mAbyssDepth`,
`mField690` is `mDifficulty` (-1 in an abyss game), `mField6b0` is `mLongWordLength`, `mString6b4` / `mVector6f4` / `mMap720`
are `mShipName` / `mShipNames` / `mUsedShipNames`, `mString6d0` / `mVector704` / `mMap714` are `mGemName` / `mGemNames` /
`mUsedGemNames`, `mVector754` / `mVector764` are `mWordLists` / `mThemes` (now `std::vector<std::vector<std::string> >`),
`mMap774` is `mLongWords`, `mMap780` / `mMap78c` are `mUsedWords` / `mUsedLongWords`, `mVector730` / `mField740` are
`mUsedThemes` / `mTheme`; `GetWordA` / `GetWordB` are `GetShipName` / `GetGemName`.

## UserProfileMgr / UserProfile (job P1, done: UserProfile.h/.cpp, UserProfileMgr.h/.cpp)
Translated; the headers are the contract now. Renames the job made in the callers: `mUserCount` is `mUsers.size()`, `mField34` is
`mBestDepth` (registry AnnoDomini), `mField10` is `mLastDepth`, `mField60` is `mLastLesson`, `GetUserField34` is
`GetUserBestDepth`. The manager declares `GetDefaultUserNumber` and `FirstFreeUserNumber`, which stay in WinWSAppGame.cpp.
Placeholders left in the profile: mField0c, mField64, mField68, `StatGroup::mField0` / `mField8`, and `mField1c8` (the original
never sets it; zeroed). The default high-score names are read through GT from the static initializer's texts (0x52d87c...).
Quirks kept (for the game notes): a loaded profile's stat groups hold 70 records, of which only the first 35 are written
back; `WriteDataToDisk` stores `mField1ec` where `LoadDataFromDisk` reads `mField1b4`; `RegisterDefaultUserName` always
returns false; `SaveUser` does not free the profile it replaces.

## Screens, widgets, dialogs (jobs S1, W1, D1, T1)
- MenuScreen(WinWSApp*) (0x4436e0); TitleScreen(WinWSApp*), `AddControls()` (0x448f40), `mPlayButton` (+0x78);
  HighScoreScreen(WinWSApp*) (its +0x58 is set to the app's mProfileMgr).
- MapScreen(WinWSApp*) (0x458d10, sizeof 0x238; +0x228 is the chosen level `mLevel`), `OnEnter()` (0x4533b0), `ResetScroll()`
  (0x441030), `ClearNodeWidgets()` (0x443e20), `SetFinalLevel(bool)` (0x441080), `std::map<int,MapNode*> mMap1a8`,
  `vector<ProfilePair*> mVector1c4`, `vector<MapNode*> mVector1d4`, mField1f8, mFlag224, mFlag234.
- StatsScreen(WinWSApp*, int) (0x45baa0), `SetUser(UserProfile*)` (0x451050); StatusBar(WinWSApp*) (0x448a40, sizeof 0x88, a
  Widget; its height comes from `mImages[IMAGE_STATUSBARX]`, id 8).
- TypingTutor (Widget and ButtonListener; ctor 0x460b30): `ButtonDepress(int)` (the ButtonListener object is at +0x58),
  `mFlag261`, `mFlag262`.
- Dialogs: SelectUserDialog(WinWSApp*), OptionsDialog(WinWSApp*) (its check boxes), AbyssModeDialog(WinWSApp*, UserProfile*),
  NewUserDialog(WinWSApp*, const std::string&) (`mNameEdit`), DeleteUserDialog(WinWSApp*, vector<string>) with
  `DeleteSelectedUser`, `DifficultyDialog` (`mSelectedName`, `mSelectedDepth`), KeyDialog(app, dialogImg, buttonImg, id, modal,
  header, lines, footer, mode) with `mCloseOnKey` (+0x134), `RemoveCancelKey(char)` (0x43a3e0), `AddOkKey(char)` (0x43ed20),
  HintDialog(app, id, header, lines, footer, IMAGE_DIALOG, IMAGE_DBUTTON, IMAGE_CHECKED) with `mField120` (+0x120, set to 5)
  and `mField124`. The dialogs are Dialog2004 (framework/compat2004). The button ids come back as id+2000 (yes/ok) and
  id+3000 (no/cancel).

## Known doubts to check when the class is translated
- ShowMenuScreen (J1) removes mStatusBar and mBoard together; after ResetGame mStatusBar is NULL, so it calls
  RemoveWidget(NULL) (WinWSApp.cpp).
- 0x428d80 and 0x42aad0 divide by a list size that is 0 for an empty list, as the original does.
- GetWord (0x428d80): the first letter of the chosen word is taken from the word; the decompilation is unclear on that
  point, so the phase-1 comparison of the words drawn tests it.

## From the last entity job (E3: BackgroundFish, BonusCreature, Clam, Bubble, BonusText, Player)
- Board: `int mField0eb8` (+0xeb8; `Player::SetState(2)` compares it with 0 through `mApp->mBoard` without a null check; the
  role is unknown), `AddBubble(x, y)` (0x452f30, confirmed x then y; the Board sets the position with SetX / SetY after
  constructing), `AddBonusText(std::string, x, y, Font* = NULL)` (0x452fd0; a NULL font becomes
  `app->mFontUnsteadyOversteer22`, +0x8b8; pushes onto the vector at Board +0xdb4). The Board constructs `BackgroundFish(app)`,
  `BackgroundFish(app, leader)`, `BonusCreature(app)` (then `SetState(0)`, `SetX`, `SetY`, `SetVelY(0.3)`), `Clam(app, type)`,
  `Bubble(app)`, `BonusText(text, font)`; it reads the player's `mFlag118`, `mEmpCharge`, `mEmpActive`, `mField110`, `mFlag13d`
  and calls `Player::AddScore(int)`, `SetScore(int)`, `SetFlag118(bool)`; it writes `Enemy::mField0e4` (+0xe4) in every
  creature it makes (a number from its spawn record) and calls `Enemy::IsDeathFinished()` (0x401230), `RemoveEvent(int)`
  (0x407100), `PopEvent`, `HasEvent`, `SetWord`.
- There is no `Effect` class: the export's name is GameObject's deleting destructor (0x401160).
- Kept from the original, to be listed in the game notes: `BackgroundFish`'s copy constructor puts the copy's x at about
  4.29e9 (unsigned arithmetic), so the Board deletes it at its first tick and only the leader of a school shows; `Player`'s
  constructor leaves `mHoseFrameUp`, `mHoseY` and `mAlpha` unset (the port zeroes them, the one place where behaviour could
  differ); `Player::Update` state 1 has a dead branch.
