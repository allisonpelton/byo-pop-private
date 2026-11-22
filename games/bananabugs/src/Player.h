// Player: a player's profile and, while a level is played, the monkey and the machine it drives (0x459380..0x45e900 in
// the original). There are eight profiles, made once and kept in a table; the current one is the object the level and
// the map read and change: its lives, bananas, score, upgrades and how far the game has got. In a level it is also the
// hand that aims: it follows the mouse, finds the stretch of ground a rope could cut, shoots the rope and shows the
// monkey's moods (idle, looking about, drumming while the rope runs, tangled, falling, dead).
// The profiles are saved to profiles.dat in the user's folder: 8 records of 0x90 bytes (see SaveProfiles).
#pragma once

#include "Pending.h"
#include "Bonus.h"

namespace Sexy
{

class Anim;
class BonusEffect;
struct AnimKey;
class Emitter;
class ObjString;
class ScoreBoard;

const int PLAYER_PROFILES = 8;
const int PLAYER_NAME_SIZE = 32;      // the name's room; at most 31 characters and the end mark
const int PLAYER_UPGRADES = 10;
const int PLAYER_BONUSES = 19;        // the bonus kinds a player may be blocked from
const int PLAYER_DRUM_SOUNDS = 9;
const int PLAYER_SPEED_EMITTERS = 4;
const int PLAYER_WARN_SOUNDS = 4;
const int PLAYER_LOST_LIVES_START = 6;

// profiles.dat: the records and their fields (little-endian 32-bit numbers after the name)
const int PROFILE_RECORD_SIZE = 0x90;
const int PROFILE_FILE_SIZE = PLAYER_PROFILES * PROFILE_RECORD_SIZE;

// What the monkey is doing (Player::mMonkeyState)
enum MonkeyState
{
	MONKEY_INIT = 0,
	MONKEY_PARKED = 1,        // not in a level: slides to a stop
	MONKEY_PLAYING = 2,       // follows the mouse
	MONKEY_DRUMMING = 3,      // the rope is running
	MONKEY_TANGLED = 4,       // a bug touched the rope
	MONKEY_FALLING = 5,
	MONKEY_HIT = 6,
	MONKEY_DEAD = 7,
};

// How the shot goes (Player::mShotPhase)
enum ShotPhase
{
	SHOT_NONE = 0,
	SHOT_FIRED = 1,           // waits for the rope to be ready to aim again
	SHOT_WAITING = 2,         // then a few frames more
	SHOT_DONE = 3,
};

// What the map is to do after a level (Player::mMapEvent)
enum MapEventKind
{
	MAP_EVENT_NONE = 0,
	MAP_EVENT_HALFWAY = 1,
	MAP_EVENT_NEW_WORLD = 2,
};

// The upgrades, in the order the player keeps them; the ones with no name of their own are named by their place until
// the shop shows what they are
enum UpgradeKind
{
	UPGRADE_FIRST = 0,        // level 1 sets mFirstUpgradeFlag, level 2 also unblocks bonus 0xf
	UPGRADE_SECOND = 1,       // the value in mSecondUpgradeValue, and bonus 0x10
	UPGRADE_THIRD = 2,        // bonuses 7, 1 and 8
	UPGRADE_SHOT_TIME = 3,    // the shot's time (mShotTime)
	UPGRADE_FIFTH = 4,        // bonuses 5 and 6
	UPGRADE_SIXTH = 5,        // bonuses 0xc and 0x11
	UPGRADE_SEVENTH = 6,      // the value in mSeventhUpgradeValue
	UPGRADE_BANANA_VALUE = 7, // what a banana is worth (mBananaValue)
	UPGRADE_NINTH = 8,        // bonuses 9, 0xb and 0xa
	UPGRADE_TENTH = 9,        // the value in mTenthUpgradeValue
};

class Player : public ObjBase
{
public:
	// The fields the original's ObjBase leaves spare, which the Player uses (port: the ObjBase here has no spare words)
	float mLookSpeed;         // +0x4c: how fast the monkey looks about, smoothed, 0 to 9

	// The profile (offsets are the original's, from the object)
	int mLives;               // +0x27c
	int mInUse;               // +0x280: the profile holds a player
	ScoreBoard* mScoreBoard;  // +0x284: the level's board of figures, given by SetScoreBoard
	int mLevelBonus;          // +0x288: what the last level's end gave
	int mTenthUpgradeValue;    // +0x28c: UPGRADE_9's value, 0 to 3
	int mBananasEarned;       // +0x290: in this level
	int mBananas;             // +0x294
	int mUpgradeBought;       // +0x298: set by an upgrade bought, taken by TakeUpgradeBought
	int mBugCount;            // +0x29c: bugs shown in this level
	int mBugsKilled;          // +0x2a0
	int mShotsFired;          // +0x2a4
	int mExtraPercent;        // +0x2a8: the percent filled above the level's target
	int mScore;               // +0x2ac
	int mScoreBeforeLevel;    // +0x2b0
	int mSavedProgress;       // +0x2b4: levels finished; above 0 once a game is in progress
	int mMapLevel;            // +0x2bc: the world the map moves to
	int mMapEvent;            // +0x2c0: MapEventKind, set by MapPosition
	int mPrevMapLevel;        // +0x2c4
	int mBugKinds;            // +0x2c8: 2 at the start, one more per new world up to 8, one fewer for two deaths, down to 2
	float mWorldProgress;     // +0x2cc: how far through the world's levels
	int mPercent;             // +0x2d0: filled so far
	int mUnused2d4;        // +0x2d4: set to 1 by Reset and by a shot; nothing reads it
	int mMonkeyState;         // +0x2d8: MonkeyState
	int mRopeMode;            // +0x2dc: 0 the usual rope; the bonus ropes are 1 and 2 (RopeObj reads it)
	int mShotX;               // +0x2e0: where the last shot was fired from (whole pixels)
	int mShotY;               // +0x2e4
	int mHeldCount;           // +0x2e8: how many of the held thing may be put down
	int mUnused2ec;           // +0x2ec: set to 0 by Reset; nothing reads it
	bool mLostLife;           // +0x2f0
	bool mGameOver;           // +0x2f1
	BonusEffect* mBonusEffect; // +0x2f4
	ObjBase* mFlare;          // +0x2f8: the flash the monkey appears and goes in
	int mFlareStage;          // +0x2fc: 0 appearing, 1 going (2 never set here)
	int mBonusBlocked[PLAYER_BONUSES]; // +0x300: 1 while the bonus may not show up
	int mTutorialDone;        // +0x34c
	int mWalkThruDone;        // +0x350
	int mShotCooldown;        // +0x354: frames before the next shot
	bool mInLevel;            // +0x358
	Bonus* mHeld;           // +0x35c: what the monkey holds, if any
	ObjBase* mHeldOldParent;  // +0x360: its parent before it was picked up
	ObjBase* mHolder;         // +0x364: what holds it (turns it)
	int mUpgradeLevel[PLAYER_UPGRADES]; // +0x368 + 8 * kind
	int mUpgradeMax[PLAYER_UPGRADES];   // +0x36c + 8 * kind
	int mMetaGameStarted;     // +0x3b8
	int mMetaProgress[3];     // +0x3bc
	int mWarnCooldown;        // +0x3c8: frames before the monkey may call out again
	int mWarnVoice;           // +0x3cc: which of mWarnSounds
	int mWarnSounds[PLAYER_WARN_SOUNDS]; // +0x3d0
	char mName[PLAYER_NAME_SIZE]; // +0x3e0
	int mBananasAtStart;      // +0x400: kept so a level played again starts from them
	int mScoreAtStart;        // +0x404
	float mVelX;              // +0x408: the monkey's smoothed speed
	float mVelY;              // +0x40c
	int mHolding;             // +0x410: 1 while a thing is held
	float mUnused414;         // +0x414: set to -128 by Reset; nothing reads it
	float mUnused418;         // +0x418: set to -32 by Reset; nothing reads it
	int mHeldRotation;        // +0x41c: 0 to 3
	ObjString* mPercentText;  // +0x420
	ObjBase* mMachine;        // +0x428
	ObjBase* mMonkey;         // +0x42c
	ObjBase* mTooClose;       // +0x430: the picture that flashes when a held thing is put down too near another
	int mMachineTrack;        // +0x434
	int mTangledTrack;        // +0x438
	int mFallTrack;           // +0x440
	int mDeaths;              // +0x444: since the last level was finished; two make the bugs fewer
	int mUnused458;           // +0x458: set to 0 by Reset; nothing reads it
	int mIdleTrack;           // +0x45c: the monkey's tracks
	int mDrumTrack;           // +0x460
	int mLookUpTrack;         // +0x464
	int mLookDownTrack;       // +0x468
	int mLookLeftTrack;       // +0x46c
	int mLookRightTrack;      // +0x470
	int mSpazzTrack;          // +0x474
	bool mUnused478;          // +0x478: set to 0 by Reset; nothing reads it
	int mDrumSounds[PLAYER_DRUM_SOUNDS]; // +0x47c
	int mDrumIndex;           // +0x4a4
	AnimKey* mFallKey;        // +0x4a8: the fall's first key, its decay started again at each fall
	AnimKey* mFlipKeyA;       // +0x4ac: the held thing's turn, the rotation it starts at and the one it reaches
	AnimKey* mFlipKeyB;       // +0x4b0
	int mCellX;               // +0x4b4: the ground cell the monkey points at
	int mCellY;               // +0x4b8
	int mPrevCellX;           // +0x4bc
	int mPrevCellY;           // +0x4c0
	int mSegFrom;             // +0x4c4: the stretch of cells the rope would cut, from and to
	int mSegTo;               // +0x4c8
	int mSegVertical;         // +0x4cc: 1 when the stretch runs down the column
	int mSegLocked;           // +0x4d0: set by turning the stretch; kept until the next shot
	bool mLooking;            // +0x4d4: the monkey is startled by a bug
	int mTargetPercent;       // +0x4d8: the percent the level asks for
	int mSecondUpgradeValue;     // +0x4dc: 0, 1, 2 or 4
	int mBananaValue;         // +0x4e0: 2 to 5
	int mFirstUpgradeFlag;     // +0x4e4
	float mSeventhUpgradeValue;   // +0x4e8: 0, 0.03, 0.05 or 0.08
	float mShotTime;          // +0x4ec: 0.4 to 0.25
	float mSpeedScale;        // +0x4f0: 1 unless a bonus speeds the rope
	int mShotPhase;           // +0x4f4: ShotPhase
	int mShotWait;            // +0x4f8
	bool mAimShown;           // +0x4fc: the rope's preview is on the ground
	bool mOverBug;            // +0x4fd: a bug is near the monkey this frame
	ObjBase* mTargetTile;     // +0x500: the tile the monkey points at, or nullptr
	Emitter* mSpeedEmitters[PLAYER_SPEED_EMITTERS]; // +0x504: the sparks at the monkey's four sides, shown while sped up

	// The eight profiles, the current one and its place in the table
	static Player* sProfiles[PLAYER_PROFILES]; // 0x67019c
	static Player* sCurrent;   // 0x670198
	static int sCurrentIndex;  // 0x600508: -1 for none
	static int sLastUnblocked; // 0x600674: the last bonus a player was let to have (not 0)
	static int sFreeSlot;      // 0x6701f0: the first profile not in use, or -1 (set by FindFreeSlot)
	static int sUpdateCount;   // 0x6a5590: Update calls
	static float sPrevDrawnX;  // 0x6a6670
	static float sPrevDrawnY;  // 0x6a666c
	static char sPercentBuffer[16]; // 0x6a6660: the percent text, built before it is shown

	// The pictures, made as a player is made
	static Anim* sMachineAnim;   // 0x6701bc
	static Anim* sIdleAnim;      // 0x6701c0
	static Anim* sDrumAnim;      // 0x6701c4
	static Anim* sLookUpAnim;    // 0x6701c8
	static Anim* sLookDownAnim;  // 0x6701cc
	static Anim* sLookLeftAnim;  // 0x6701d0
	static Anim* sLookRightAnim; // 0x6701d4
	static Anim* sSpazzAnim;     // 0x6701d8
	static Anim* sFallingAnim;   // 0x6701dc
	static Anim* sDeathAnim;     // 0x6701e0
	static Anim* sTangledAnim;   // 0x6701e4
	static Anim* sSpeedAnim;     // 0x6701e8
	static Anim* sTooCloseAnim;  // 0x6701ec

	Player();                  // 0x459380
	~Player() override;        // 0x459880
	void Notify(ObjBase* theObj) override;     // 0x459aa0
	bool Update() override;                    // 0x459eb0
	void Draw(Graphics* g) override;           // 0x45a3e0

	void Reset(bool theFull);                  // 0x4598b0
	void UpdateLook();                         // 0x45a500
	void HideAll();                            // 0x45a710
	void ShowAll();                            // 0x45a790
	void PlayWarning();                        // 0x45a7c0
	void UpdateAim();                          // 0x45a860
	void ApplyUpgrade(int theKind);            // 0x45ade0
	void Block(int theBonus) { mBonusBlocked[theBonus] = 1; }  // 0x45add0
	void Unblock(int theBonus);                // 0x45adb0
	int MapPosition(int theProgress, int* theStart, int* theSize, int* theEnd, int* theMid, float* theWorldProgress); // 0x45b540

	// The builders of the constructor
	void BuildMonkey();                        // 0x45b8d0
	void BuildMachine();                       // 0x45b750
	void BuildTangledKeys();                   // 0x45c080
	void BuildFallKeys();                      // 0x45c1f0
	void BuildSpeedEmitters();                 // 0x45c600
	void BuildPercentText();                   // 0x45ca30
	void BuildHolder();                        // 0x45b610
	void BuildFlare();                         // 0x45ce10
	void BuildTooClose();                      // 0x45d130

	// What the level and the map do with the current player (they act on sCurrent)
	static void AppearAtMouse();               // 0x45a400
	static void PickUp(Bonus* theObj, int theCount);   // 0x45ab70
	static Bonus* Drop();                    // 0x45abd0
	static int RotateHeld();                   // 0x45ac20
	static int FlipHeld();                     // 0x45ad10
	static void AddLives(int theCount);        // 0x45b170
	static int BuyUpgrade(int theKind, int theCost); // 0x45b180: 0 when bought, else the bananas short
	static void MarkUpgradeBought();           // 0x45b1d0
	static int TakeUpgradeBought();            // 0x45b1e0
	static int SpendBananas();                 // 0x45b200: 100 of them; 0 when spent, else the bananas short
	static int GetUpgradeLevel(int theKind);   // 0x45b230
	static double GetSpeedAverage();           // 0x45b240
	static void StartLevelPlay();              // 0x45b260
	static void EndLevelPlay();                // 0x45b2c0
	static void MoveTo(int theWindowX, int theWindowY); // 0x45b2e0
	static void LoseLife();                    // 0x45b340
	static void AddBananas(int theCount);      // 0x45d460
	static int GetBananaValue();               // 0x45d450
	static int GetTargetPercent();             // 0x45d440
	static int GetRopeMode();                  // 0x45d470
	static void SetRopeMode(int theMode);      // 0x45d480
	static double GetShotInterval();           // 0x45d490
	static void SetSpeedScale(float theScale); // 0x45d4b0
	static bool IsNear(ObjBase* theObj, float theReach); // 0x45d520
	static void AddScore(int theScore);        // 0x45d590
	static void SetPercent(int thePercent);    // 0x45d5b0
	static bool MarkLastBananaIcon();          // 0x45d690
	static void CountBug();                    // 0x45d6b0
	static void CountKill();                   // 0x45d6c0
	static int TakeMapEvent(bool theClear);    // 0x45d6d0
	static void RestoreLevelStart();           // 0x45d6f0
	static void StartLevel();                  // 0x45d720
	static void FinishLevel();                 // 0x45d760
	static void BoardClick(int theButton);     // 0x45d8e0
	static void Shoot();                       // 0x45da10
	static int HitBugBox();                   // 0x45db70
	static void TurnRope();                    // 0x45dbb0
	static void SetScoreBoard(ScoreBoard* theBoard); // 0x45dc30
	static void Reactivate();                  // 0x45d8c0: leaves and joins the scene again

	// The profiles
	static int AddProfile(int theSlot, const char* theName);   // 0x45dca0: the slot, or -1 no name, -2 no room, -3 taken
	static bool NameIsFree(const char* theName, int theSkip);  // 0x45dd80
	static const char* GetNameAt(int theIndex);                // 0x45de10
	static int ChooseFirstInUse();                             // 0x45de30: the current profile, or -1
	static void StartMetaGame(int theSlot, int theValue);      // 0x45de70
	static void EndMetaGame();                                 // 0x45de90
	static void SortProfiles();                                // 0x45dea0
	static bool DeleteProfile(int theIndex);                   // 0x45e050
	static void SetCurrent(int theIndex);                      // 0x45e0b0
	static void SaveAll();                                     // 0x45e110
	static void LoadFile();                                    // 0x45e4c0
	static void AddHighScoreOfCurrent();                       // 0x45e770
	static void FindFreeSlot();                                // 0x45e790
	static void ResetCurrent();                                // 0x45e7d0
	static void MakeProfiles();                                // 0x45e7f0
	static void SetShotCooldown(int theFrames);                // 0x45e8c0
};

// The profiles the main menu reads and changes
const char* GetProfileName(); // 0x45dde0: the current profile's name, or nullptr when no profile is in use
void ResetProfile();          // 0x45e7d0: starts the current profile's game afresh

// GameApp's globals (0x42d330)
void LoadProfiles();          // 0x45e7f0
void SaveProfiles();          // 0x45e110

} // namespace Sexy
