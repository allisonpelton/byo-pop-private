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

## LessonManager (job P2; ctor 0x4270e0, sizeof 0xc0, dtor 0x421c50)
`LessonManager(WinWSApp*)`, `int LoadConfig(int mode)` (0x42bfb0; ECX is the manager, returns 0 on success),
`SetLevel(int)` (0x4309d0), `IsTutorialActive()` (0x4137d0), `bool mAbyssMode` (+0xa5), `int mLevel` (+0x8c),
`int mField0a8` (+0xa8, getter 0x412370), `GetBossRecord(int)` (0x4139d0, returns a pointer to a record) and
`GetBossRecordCount()` (0x413a10). The boss record's fields, named by offset: word lengths +0x1c, +0x20; fire interval
+0x40 (double); hit points +0x48; ints at +0x4c and +0x50; bob speed +0x58 (double); shot speeds +0x60 and +0x68
(doubles) (ProjectileBoss.cpp / TorpedoBoss.cpp use `mFireInterval`, `mHitPoints`, `mField4c`, `mField50`, `mBobSpeed`,
`mShotSpeedMin`...).
WinWSApp fields the config parsers fill (typed by the config job; WinWSApp.cpp lines ~104 memset them): `mVector754` and
`mVector764` (now `void*[4]`; `GetWord` and `GetThemeWord` read element 1 and 2 as begin and end of an array of
`std::vector<std::string>`; retype and fix the callers), `mMap774` (`std::map<int, std::vector<std::string>>`), `mMap780`
(used words: `std::map<std::string,bool>`), `mVector730`, `mVector704`, `mMap714`, `mString6b4`, `mString6d0`,
`mVector6f4`, `mMap720`, `mField740`, `mField680`/`mField690`/`mField6b0`. The destructor must free the lesson manager
(+0x65c) and the tables (TODO comments in WinWSApp.cpp).

## UserProfileMgr / UserProfile (job P1; mgr ctor takes WinWSApp*, profile ctor 0x421f50, sizeof 0x2c8)
UserProfileMgr: `std::list<UserProfile*> mUsers` (+8), `mUserCount` (+0xc), `GetCurrentUserName()` (0x4150a0),
`LoadUser(name)` (0x425f20), `LoadUserProfileFromRegistry(const std::string&)` (0x425390), `GetDefaultUserNumber()`
(0x4262b0, written by J2), `FirstFreeUserNumber()` (0x413ff0, written by J2), `WriteUserProfileToRegistry(UserProfile*, bool,
bool)` (0x41f1d0), `SaveUser(UserProfile*)` (0x420c60), `RegisterDefaultUserName(const std::string&)` (0x419da0).
UserProfile: `SetName(const std::string&)` (0x416380), `GetName()` (0x415060), `SetUserNumber(int)` (0x412380),
`SetField1e4`, `SetField1e8(int)` (0x412390), `SetField1f4` (0x412640), `SetField1f8` (0x412650), `SetSpacePauses`,
`RecordResult(bool,int,int,int,int,int)` (0x412580), `RecordResultWithDouble(bool,int,int,double,int,int,int)`
(0x412520), `mUserNumber` (+0x30), `mShowHints` (+0x1fd), `mGameVersion` (std::string, +0x288), `mFlag1dc`, `mField34`
(+0x34); fields at their offsets: mField10, mField60, mField1b0/1b4/1c0/1c4/1c8/1cc/1d0/1d4/1d8/1e0/1e8, `double
mDouble1b8`, mFlag1fc, mFlag230, `int mField234[6]`, `int mField24c[4]`, `char mField25c[0x24]`, mField280,
`std::vector<ProfilePair*> mVector2a4` (8-byte pairs `{int mFirst, mSecond}`, malloc'd in the original), `std::vector<int>
mVector2b4`. `MapNode {bool mFlag38 (+0x38); int mField24 (+0x24)}` (the map screen's nodes).

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
