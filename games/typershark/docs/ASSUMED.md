# What the translated files assume of classes that are not translated yet

Each translation job (J1, J2, E1, E2...) wrote against throwaway stub headers, because the classes it calls come from
later jobs. This file collects what they call, from their reports (names are the jobs' own; the address is the
original's). A job that writes one of these classes reads its section first: it supplies these members with these
signatures, or changes the callers (grep the callers in `src/`) when the original shows the name or type is wrong, and
says so in its report. Fields named after their offset (`mField1c4`) are placeholders: a job that learns the role
renames the field everywhere (`grep -rn mField1c4 games/typershark/src`).

## Board (jobs B1-B3; header done by job B0: Board.h and BOARD.md)
Now `src/Board.h`, the contract for the three jobs that write the methods (BOARD.md has the map of the methods, the split
into jobs, the unknown fields, the registration field and the event codes). The finished files compile against it. Renames
made when the stub members became the real ones: `mLevelConfig` is `mPlayer` (a `Player*`, not a "LevelConfig"; its
`mGameMode` is `GameObject::mState`, `mField0f4` is `Player::mScore`, `mDouble0f8` is `Player::mEmpCharge`), `mField1c4` is
`mScore`, `mField0e1c` is `mTypeTime`, `mField0f94` is `mContinueLink` (a `HyperlinkWidget*`), `mField0eb8` is `mBoss` (an
`Enemy*`, 0 when there is no boss). `mPaused` (bool, +0xe60) is read by every creature's Draw. `AddBubble(int x, int y)`
(0x452f30) is confirmed x then y. The Board casts a `Shark*` or `Piranha*` obtained through a virtual base with
`dynamic_cast` (ENTITIES.md). `WinWSApp::mBoard` is +0x634. `BlackShark::mWord2` and `RedShark::mWord3` are filled by the
Board (`GetWord2` / `GetWord3` in its code); `WhitePiranha::SetWord2(std::string)`. The event codes the creatures push on
`Enemy::mEvents` and what the Board does with each are in BOARD.md, part 5.

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
Placeholders left in the profile: mField0c, mField64, `StatGroup::mField0` / `mField8`, and `mField1c8` (the original
never sets it; zeroed). The default high-score names are read through GT from the static initializer's texts (0x52d87c...).
Quirks kept (for the game notes): a loaded profile's stat groups hold 70 records, of which only the first 35 are written
back; `WriteDataToDisk` stores `mField1ec` where `LoadDataFromDisk` reads `mField1b4`; `RegisterDefaultUserName` always
returns false; `SaveUser` does not free the profile it replaces.

## Screens, widgets, dialogs (jobs S1, W1, D1, D2, T1)
- MenuScreen(WinWSApp*) (0x4436e0); TitleScreen(WinWSApp*), `AddControls()` (0x448f40) (its `mPlayButton` was removed with the registration code, job R1);
  HighScoreScreen(WinWSApp*) (its +0x58 is set to the app's mProfileMgr).
- W1 done (StatusBar, TabbedWidget, TabPanel, TutorialPanel, AdventurePanel, AbyssPanel, StatsScreen; headers are the contract). Profile
  renames: mField68 `mBestTreasureWords`, mField6c `mGemsFound`, mField84 `mSecretsFound`, mField22c `mGamesCompleted`.
  XWidget::MouseDown (0x458cb0) assumes of MapScreen: `HyperlinkWidget* mContinueLink` (+0x178, the "press space bar to continue"
  link), `char mChosenLetter` (+0x235) and `void OnLevelChosen()` (0x4538e0, with its helper 0x4537b0; attribute.py files both
  under XWidget but they are MapScreen's: job S1 translates them). 0x4506a0 is MapScreen's too.
- S2 done: MapScreen is translated (MapScreen.h/.cpp, MapScreenDraw.cpp, MapScreenUpdate.cpp; the header is the contract; `MapNode::mFlag38`
  is `mVisited`, `mField24` is `mId`). The older list of what the callers assumed of it:
- MapScreen(WinWSApp*) (0x458d10, sizeof 0x238; +0x228 is the chosen level `mLevel`), `OnEnter()` (0x4533b0), `ResetScroll()`
  (0x441030), `ClearNodeWidgets()` (0x443e20), `SetFinalLevel(bool)` (0x441080), `std::map<int,MapNode*> mMap1a8`,
  `vector<ProfilePair*> mVector1c4`, `vector<MapNode*> mVector1d4`, mField1f8, mFlag224, mFlag234.
- StatsScreen(WinWSApp*, int) (0x45baa0), `SetUser(UserProfile*)` (0x451050); StatusBar(WinWSApp*) (0x448a40, sizeof 0x88, a
  Widget; its height comes from `mImages[IMAGE_STATUSBARX]`, id 8).
  The Board also calls `StatusBar::HideButtons()` (0x441f10: hides its buttons and sends the widget at +0x68 to the back),
  `ShowButton(bool)` (0x441f80) and the Widget virtual `MouseLeave()`.
- D1 done: GameOverDialog, WinGameDialog, DifficultyDialog, AbyssModeDialog, KeyDialog (the plain message box) and HintDialog are
  translated and their headers are the contract (AbyssModeDialog folds in the original's MoneyDialog base). WinGameDialog's
  `mTime`, `mCharsTyped`, `mMistakes` are set by MapScreen's update (0x45ad20) from the app's +0x900, +0x90c, +0x908
  (named `mField900`... by use only). `Player::mField114` is the 0-based level index. UserProfile +0x22c counts finished games
  (1-2 bronze, 3-4 silver, 5+ gold medal); +0x194 is the abyss "Highest Score".
- GameOverDialog(WinWSApp*, Player*, bool theEndless, int, int, int, int theScore) (ctor 0x43adb0) with `int mField11c` (+0x11c),
  `std::string mRankTitle` (+0x128) and `int mField144` (+0x144, set only in an endless game); a Dialog2004, added as DIALOG_GAME_OVER.
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
- Board (now Board.h): `Enemy* mBoss` (+0xeb8, was `mField0eb8`; `Player::SetState(2)` compares it with NULL through
  `mApp->mBoard` without a null check; it is the boss of the level), `AddBubble(x, y)` (0x452f30, confirmed x then y; the Board
  sets the position with SetX / SetY after constructing), `AddBonusText(std::string, x, y, Font* = NULL)` (0x452fd0; a NULL font
  becomes `app->mFontUnsteadyOversteer22`, +0x8b8; pushes onto the vector at Board +0xdb4). The Board constructs `BackgroundFish(app)`,
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
