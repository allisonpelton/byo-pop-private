#include "Player.h"
#include "Board.h"
#include "ScoreBoard.h"

#include "Anim.h"
#include "BBWindow.h"
#include "Buffer.h"
#include "Cursor.h"
#include "Emitter.h"
#include "GameApp.h"
#include "HighScore.h"
#include "LevelObj.h"
#include "Graphics.h"
#include "ObjString.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstdio>
#include <cstring>

using namespace Sexy;

Player* Player::sProfiles[PLAYER_PROFILES];
Player* Player::sCurrent = nullptr;
int Player::sCurrentIndex = -1;
int Player::sLastUnblocked = -1;
int Player::sFreeSlot = 0;
int Player::sUpdateCount = 0;
float Player::sPrevDrawnX = 0.0f;
float Player::sPrevDrawnY = 0.0f;
char Player::sPercentBuffer[16];

Anim* Player::sMachineAnim = nullptr;
Anim* Player::sIdleAnim = nullptr;
Anim* Player::sDrumAnim = nullptr;
Anim* Player::sLookUpAnim = nullptr;
Anim* Player::sLookDownAnim = nullptr;
Anim* Player::sLookLeftAnim = nullptr;
Anim* Player::sLookRightAnim = nullptr;
Anim* Player::sSpazzAnim = nullptr;
Anim* Player::sFallingAnim = nullptr;
Anim* Player::sDeathAnim = nullptr;
Anim* Player::sTangledAnim = nullptr;
Anim* Player::sSpeedAnim = nullptr;
Anim* Player::sTooCloseAnim = nullptr;

// The key events of the monkey's tracks, as Notify gets them
enum PlayerEvent
{
	EVENT_TANGLED_DONE = 2,   // the tangled monkey has hung long enough: it falls
	EVENT_HIT_GROUND = 3,
	EVENT_DEATH_DONE = 4,
	EVENT_VANISHED = 5,
	EVENT_RESPAWN = 7,
	EVENT_PERCENT_SHOWN = 8,  // the percent text has gone: nothing to do
	EVENT_SPAZZ_DONE = 9,
	EVENT_FLARE_PEAK = 10,
};

// The grid the ground is in: the cells a look along a row or a column goes over
const int GRID_COLUMNS = 40;
const int GRID_ROWS = 32;

// The cell size the monkey's place is cut into, and where a cell's middle is
const double CELL_SIZE = 31.0;
const double CELL_HALF_SIZE = 15.0;
const double CELL_SHIFT_Y = 24.0;

// The monkey is drawn from the mouse, which is in window pixels; the design space is 1920 by 1200 and its corner is
// the first position
const double DESIGN_HALF_WIDTH = 960.0;
const double DESIGN_HALF_HEIGHT = 600.0;

// The speed under which the parked monkey stops, and the share it keeps each frame
const double PARKED_STOP_SPEED = 0.001;

// The monkey's look: the least speed difference it follows, the fastest speed it shows and the slowest before it idles
const float LOOK_MIN_DIFFERENCE = 0.01f;
const float LOOK_MAX_SPEED = 9.0f;
const float LOOK_STOP_SPEED = 0.5f;

// The hand's shot: frames between shots, frames the rope is waited for
const int SHOT_COOLDOWN_FRAMES = 50;
const int SHOT_WAIT_FRAMES = 10;
const int WARN_COOLDOWN_FRAMES = 200;

// The distance in design pixels within which a bug makes the monkey look startled
const float STARTLE_REACH = 64.0f;

// Where the text of the lives left is shown (its x; the game over text is at the same x)
const float MESSAGE_X = 620.0f;

static void PlaySound(int theSound)
{
	gApp->mSoundQueue->Request(theSound, 0, false);
}

// An object that may be missing, as a ground cell's may in the port's stand-in
static bool IsActiveObject(ObjBase* theObj)
{
	return theObj != nullptr && theObj->IsActive();
}

// The place an angle in degrees has in radians, as the original works it out
static float DegreesToRadians(float theDegrees)
{
	return (float)(((double)theDegrees * PI_F) / 180.0);
}

// 0x459380
Player::Player()
{
	// port: the original leaves most fields as the heap gave them until Reset sets them; the rest are made zero
	mLookSpeed = 0.0f;
	mLives = 0;
	mInUse = 0;
	mScoreBoard = nullptr;
	mLevelBonus = 0;
	mTenthUpgradeValue = 0;
	mBananasEarned = 0;
	mBananas = 0;
	mUpgradeBought = 0;
	mBugCount = 0;
	mBugsKilled = 0;
	mShotsFired = 0;
	mExtraPercent = 0;
	mScore = 0;
	mScoreBeforeLevel = 0;
	mSavedProgress = 0;
	mMapLevel = 0;
	mMapEvent = 0;
	mPrevMapLevel = 0;
	mBugKinds = 0;
	mWorldProgress = 0.0f;
	mPercent = 0;
	mUnused2d4 = 0;
	mMonkeyState = MONKEY_INIT;
	mRopeMode = 0;
	mShotX = 0;
	mShotY = 0;
	mHeldCount = 0;
	mUnused2ec = 0;
	mLostLife = false;
	mGameOver = false;
	mBonusEffect = nullptr;
	mFlare = nullptr;
	mFlareStage = 0;
	mHeldOldParent = nullptr;
	mHolder = nullptr;
	for (int i = 0; i < PLAYER_UPGRADES; i++) {
		mUpgradeLevel[i] = 0;
		mUpgradeMax[i] = 0;
	}
	mMetaGameStarted = 0;
	for (int i = 0; i < 3; i++)
		mMetaProgress[i] = 0;
	memset(mName, 0, sizeof(mName));
	mBananasAtStart = 0;
	mScoreAtStart = 0;
	mVelX = 0.0f;
	mVelY = 0.0f;
	mHolding = 0;
	mUnused414 = 0.0f;
	mUnused418 = 0.0f;
	mHeldRotation = 0;
	mPercentText = nullptr;
	mMachine = nullptr;
	mMonkey = nullptr;
	mTooClose = nullptr;
	mMachineTrack = 0;
	mTangledTrack = 0;
	mFallTrack = 0;
	mDeaths = 0;
	mUnused458 = 0;
	mIdleTrack = 0;
	mDrumTrack = 0;
	mLookUpTrack = 0;
	mLookDownTrack = 0;
	mLookLeftTrack = 0;
	mLookRightTrack = 0;
	mSpazzTrack = 0;
	mUnused478 = false;
	mDrumIndex = 0;
	mFallKey = nullptr;
	mFlipKeyA = nullptr;
	mFlipKeyB = nullptr;
	mCellX = 0;
	mCellY = 0;
	mPrevCellX = 0;
	mPrevCellY = 0;
	mSegFrom = 0;
	mSegTo = 0;
	mSegVertical = 0;
	mSegLocked = 0;
	mLooking = false;
	mTargetPercent = 0;
	mSecondUpgradeValue = 0;
	mBananaValue = 0;
	mFirstUpgradeFlag = 0;
	mSeventhUpgradeValue = 0.0f;
	mShotTime = 0.0f;
	mSpeedScale = 1.0f;
	mShotPhase = 0;
	mShotWait = 0;
	mAimShown = false;
	mOverBug = false;
	mTargetTile = nullptr;
	for (int i = 0; i < PLAYER_SPEED_EMITTERS; i++)
		mSpeedEmitters[i] = nullptr;

	mShotCooldown = 0;
	mInLevel = false;
	mHeld = nullptr;

	// the bonuses a player may be kept from
	for (int i = 0; i < PLAYER_BONUSES; i++)
		mBonusBlocked[i] = 0;
	SetParent(nullptr, nullptr);

	sMachineAnim = Anim::Get("images/monkey_machine", ANIM_SCALE_SMOOTH, false, 0x7f, 0x7f, 8, false);
	sIdleAnim = Anim::Get("images/monkey_idle", ANIM_SCALE_SMOOTH, false, 0x7f, 0x7f, 0x14, false);
	sDrumAnim = Anim::Get("images/monkey_drum", ANIM_SCALE_SMOOTH, false, 0x7f, 0x7f, 0x14, false);
	sLookUpAnim = Anim::Get("images/monkey_look_up", ANIM_SCALE_SMOOTH, false, 0x7f, 0x7f, 10, false);
	sLookDownAnim = Anim::Get("images/monkey_look_down", ANIM_SCALE_SMOOTH, false, 0x7f, 0x7f, 10, false);
	sLookLeftAnim = Anim::Get("images/monkey_look_left", ANIM_SCALE_SMOOTH, false, 0x7f, 0x7f, 10, false);
	sLookRightAnim = Anim::Get("images/monkey_look_right", ANIM_SCALE_SMOOTH, false, 0x7f, 0x7f, 10, false);
	sSpazzAnim = Anim::Get("images/monkey_look_spazz", ANIM_SCALE_SMOOTH, false, 0x7f, 0x7f, 10, false);
	sTangledAnim = Anim::Get("images/monkey_tangled", ANIM_SCALE_SMOOTH, false, 0x7f, 0xa0, 5, false);
	sFallingAnim = Anim::Get("images/monkey_falling", ANIM_SCALE_SMOOTH, false, 0x7f, 0xa0, 0x14, false);
	sDeathAnim = Anim::Get("images/monkey_death", ANIM_SCALE_SMOOTH, false, 0x83, 0x83, 0x19, false);
	sSpeedAnim = Anim::Get("images/monkey_speed", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	sTooCloseAnim = Anim::Get("images/tooclose", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);

	mDrumSounds[0] = SOUND_DRUM_C;
	mDrumSounds[1] = SOUND_DRUM_E;
	mDrumSounds[2] = SOUND_DRUM_C;
	mDrumSounds[3] = SOUND_DRUM_G;
	mDrumSounds[4] = SOUND_DRUM_C;
	mDrumSounds[5] = SOUND_DRUM_A;
	mDrumSounds[6] = SOUND_DRUM_C;
	mDrumSounds[7] = SOUND_DRUM_A;
	mDrumSounds[8] = SOUND_DRUM_A;

	SetStateInt(STATE_Z, 2000);
	SetStateInt(STATE_RED, 0);
	SetStateInt(STATE_GREEN, 0);
	SetStateInt(STATE_BLUE, 0);
	SetStateInt(STATE_ALPHA, 0);

	// a track of the player's own: the colour comes up from black over a short time
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->mAnim = nullptr;
	aFirstKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(0x32);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aSecondKey);
	AddTrack(aTrack, nullptr);

	BuildMonkey();
	BuildMachine();
	BuildTangledKeys();
	BuildFallKeys();
	BuildSpeedEmitters();
	BuildPercentText();
	BuildHolder();
	BuildFlare();
	BuildTooClose();

	mBonusEffect = new BonusEffect();
	mBonusEffect->SetParent(this, nullptr);
	Reset(true);
	Deactivate(mMachine);
	Deactivate(mMonkey);
	mTutorialDone = 0;
	mWalkThruDone = 0;
	mShotCooldown = 0;
	mWarnCooldown = 0;
	mWarnVoice = 0;
	mWarnSounds[0] = SOUND_OVERBUG0;
	mWarnSounds[1] = SOUND_OVERBUG1;
	mWarnSounds[2] = SOUND_OVERBUG2;
	mWarnSounds[3] = SOUND_OVERBUG3;
}

// 0x459880: nothing of its own to free; the objects it made are its children and go with it
Player::~Player()
{
}

// 0x45b8d0: the monkey, with a track for each of the moods it looks in
void Player::BuildMonkey()
{
	mMonkey = new ObjBase();
	mMonkey->mWholePixels = true;
	mMonkey->SetParent(this, nullptr);
	mMonkey->SetStateInt(STATE_Z, 0);
	mMonkey->SetEventTarget(this);

	AnimKey* anIdleKey = new AnimKey();
	sIdleAnim->AddRef();
	anIdleKey->mAnim = sIdleAnim;
	anIdleKey->SetDuration(0x3c);
	anIdleKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 33.0f, 20.0f, 0.0f, nullptr);
	anIdleKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	anIdleKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	anIdleKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimTrack* anIdleTrack = new AnimTrack();
	anIdleTrack->AddKey(anIdleKey, anIdleKey);
	mIdleTrack = mMonkey->AddTrack(anIdleTrack, nullptr);

	AnimKey* aDrumKey = new AnimKey();
	sDrumAnim->AddRef();
	aDrumKey->mAnim = sDrumAnim;
	aDrumKey->SetDuration(0x14);
	aDrumKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 100.0f, 20.0f, 0.0f, nullptr);
	aDrumKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aDrumKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimTrack* aDrumTrack = new AnimTrack();
	aDrumTrack->AddKey(aDrumKey, aDrumKey);
	mDrumTrack = mMonkey->AddTrack(aDrumTrack, nullptr);
	mMonkey->StartTrack(mIdleTrack);

	// the four looks about hold one cel, which UpdateLook picks by the speed of the mouse
	AnimKey* aLookUpKey = new AnimKey();
	sLookUpAnim->AddRef();
	aLookUpKey->mAnim = sLookUpAnim;
	aLookUpKey->SetDuration(0x3c);
	aLookUpKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aLookUpKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aLookUpKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimTrack* aLookUpTrack = new AnimTrack();
	aLookUpTrack->AddKey(aLookUpKey, aLookUpKey);
	mLookUpTrack = mMonkey->AddTrack(aLookUpTrack, nullptr);

	AnimKey* aLookDownKey = new AnimKey();
	sLookDownAnim->AddRef();
	aLookDownKey->mAnim = sLookDownAnim;
	aLookDownKey->SetDuration(0x3c);
	aLookDownKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aLookDownKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aLookDownKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimTrack* aLookDownTrack = new AnimTrack();
	aLookDownTrack->AddKey(aLookDownKey, aLookDownKey);
	mLookDownTrack = mMonkey->AddTrack(aLookDownTrack, nullptr);

	AnimKey* aLookLeftKey = new AnimKey();
	sLookLeftAnim->AddRef();
	aLookLeftKey->mAnim = sLookLeftAnim;
	aLookLeftKey->SetDuration(0x3c);
	aLookLeftKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aLookLeftKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aLookLeftKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimTrack* aLookLeftTrack = new AnimTrack();
	aLookLeftTrack->AddKey(aLookLeftKey, aLookLeftKey);
	mLookLeftTrack = mMonkey->AddTrack(aLookLeftTrack, nullptr);

	AnimKey* aLookRightKey = new AnimKey();
	sLookRightAnim->AddRef();
	aLookRightKey->mAnim = sLookRightAnim;
	aLookRightKey->SetDuration(0x3c);
	aLookRightKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aLookRightKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aLookRightKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimTrack* aLookRightTrack = new AnimTrack();
	aLookRightTrack->AddKey(aLookRightKey, aLookRightKey);
	mLookRightTrack = mMonkey->AddTrack(aLookRightTrack, nullptr);

	// the startled look: it counts up to the last cel, which tells Notify it is over
	AnimKey* aSpazzKey = new AnimKey();
	sSpazzAnim->AddRef();
	aSpazzKey->mAnim = sSpazzAnim;
	aSpazzKey->SetDuration(0x32);
	aSpazzKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSpazzKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	aSpazzKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aSpazzKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimKey* aSpazzEndKey = new AnimKey();
	sSpazzAnim->AddRef();
	aSpazzEndKey->mAnim = sSpazzAnim;
	aSpazzEndKey->mEventId = EVENT_SPAZZ_DONE;
	aSpazzEndKey->SetDuration(1);
	aSpazzEndKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSpazzEndKey->AddValue(STATE_FRAME, 9.0f, MOD_HOLD);
	aSpazzEndKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aSpazzEndKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimTrack* aSpazzTrack = new AnimTrack();
	aSpazzTrack->AddKey(aSpazzKey, nullptr);
	aSpazzTrack->AddKey(aSpazzEndKey, nullptr);
	mSpazzTrack = mMonkey->AddTrack(aSpazzTrack, nullptr);

	Activate(this, false);
	Activate(mMonkey, false);
}

// 0x45b750: the machine the monkey sits on
void Player::BuildMachine()
{
	mMachine = new ObjBase();
	mMachine->mWholePixels = true;
	mMachine->SetParent(this, nullptr);
	mMachine->SetState(STATE_Z, -1.0f);

	AnimKey* aKey = new AnimKey();
	sMachineAnim->AddRef();
	aKey->mAnim = sMachineAnim;
	aKey->SetDuration(0x10);
	aKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 50.0f, 8.0f, 0.0f, nullptr);
	aKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey, aKey);
	mMachineTrack = mMachine->AddTrack(aTrack, nullptr);
	Activate(mMachine, false);
}

// 0x45c080: the monkey caught in the rope: three keys, the last tells Notify it is time to fall
void Player::BuildTangledKeys()
{
	AnimKey* aFirstKey = new AnimKey();
	sTangledAnim->AddRef();
	aFirstKey->mAnim = sTangledAnim;
	aFirstKey->SetDuration(1);
	aFirstKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSecondKey = new AnimKey();
	sTangledAnim->AddRef();
	aSecondKey->mAnim = sTangledAnim;
	aSecondKey->SetDuration(0x32);
	aSecondKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aThirdKey = new AnimKey();
	aThirdKey->mEventId = EVENT_TANGLED_DONE;
	sTangledAnim->AddRef();
	aThirdKey->mAnim = sTangledAnim;
	aThirdKey->SetDuration(1);
	aThirdKey->AddValue(STATE_FRAME, 4.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, nullptr);
	aTrack->AddKey(aThirdKey, nullptr);
	mTangledTrack = AddTrack(aTrack, nullptr);
}

// 0x45c1f0: the fall, the hit on the ground, the death and the way back
void Player::BuildFallKeys()
{
	AnimKey* aFallKey = new AnimKey();
	sFallingAnim->AddRef();
	aFallKey->mAnim = sFallingAnim;
	aFallKey->SetDuration(0xc8);
	aFallKey->AddDecay(STATE_X, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	aFallKey->AddDecay(STATE_Y, 0.0f, 0.0f, 0.0f, -0.22f, 1.0f);
	// the fall ends when the monkey passes the bottom of the window
	aFallKey->mEndCondition = KEY_END_ABOVE;
	aFallKey->mEndChannel = STATE_Y;
	aFallKey->mEndValue = 525.0f;
	aFallKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 60.0f, 20.0f, 0.0f, nullptr);
	aFallKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	mFallKey = aFallKey;

	AnimKey* aHitKey = new AnimKey();
	aHitKey->mEventId = EVENT_HIT_GROUND;
	sDeathAnim->AddRef();
	aHitKey->mAnim = sDeathAnim;
	aHitKey->SetDuration(0x64);
	aHitKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 30.0f, 25.0f, 0.0f, nullptr);
	aHitKey->mEndCondition = KEY_SOUND;
	aHitKey->mEndChannel = SOUND_COOCOO;
	aHitKey->mEndValue = 0.0f;

	AnimKey* aFadeKey = new AnimKey();
	sDeathAnim->AddRef();
	aFadeKey->mAnim = sDeathAnim;
	aFadeKey->SetDuration(5);
	aFadeKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 30.0f, 25.0f, 0.0f, nullptr);

	AnimKey* aDeadKey = new AnimKey();
	aDeadKey->mEventId = EVENT_DEATH_DONE;
	sDeathAnim->AddRef();
	aDeadKey->mAnim = sDeathAnim;
	aDeadKey->SetDuration(0x1e);
	aDeadKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aDeadKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);

	AnimKey* aGoneKey = new AnimKey();
	aGoneKey->mEventId = EVENT_VANISHED;
	aGoneKey->SetDuration(0x64);
	aGoneKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);

	AnimKey* aBackKey = new AnimKey();
	aBackKey->mEventId = EVENT_RESPAWN;
	aBackKey->SetDuration(1);
	aBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aBackKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFallKey, nullptr);
	aTrack->AddKey(aHitKey, nullptr);
	aTrack->AddKey(aFadeKey, nullptr);
	aTrack->AddKey(aDeadKey, nullptr);
	aTrack->AddKey(aGoneKey, nullptr);
	aTrack->AddKey(aBackKey, nullptr);
	mFallTrack = AddTrack(aTrack, nullptr);
}

// 0x45c600: the four sparks that show while the rope is sped up, one at each side of the monkey
void Player::BuildSpeedEmitters()
{
	float anAngles[PLAYER_SPEED_EMITTERS];
	anAngles[0] = 1.57079637f;
	anAngles[1] = 0.0f;
	anAngles[2] = 4.71238899f;
	anAngles[3] = 3.14159274f;
	for (int i = 0; i < PLAYER_SPEED_EMITTERS; i++) {
		AnimKey* aFirstKey = new AnimKey();
		sSpeedAnim->AddRef();
		aFirstKey->mAnim = sSpeedAnim;
		aFirstKey->SetDuration(0x28);
		aFirstKey->mDrawMode = 1;
		aFirstKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
		aFirstKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
		aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aFirstKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		aFirstKey->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
		aFirstKey->AddValue(STATE_GREEN, 0.9f, MOD_TOWARD_NEXT);
		aFirstKey->AddValue(STATE_BLUE, 0.9f, MOD_TOWARD_NEXT);
		aFirstKey->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
		aFirstKey->AddValue(STATE_ORBIT_RADIUS, 4.1f, MOD_TOWARD_NEXT);
		AnimKey* aSecondKey = new AnimKey();
		sSpeedAnim->AddRef();
		aSecondKey->mAnim = sSpeedAnim;
		aSecondKey->SetDuration(1);
		aSecondKey->mDrawMode = 1;
		aSecondKey->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
		aSecondKey->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
		aSecondKey->AddValue(STATE_RED, 0.8f, MOD_TOWARD_NEXT);
		aSecondKey->AddValue(STATE_GREEN, 0.8f, MOD_TOWARD_NEXT);
		aSecondKey->AddValue(STATE_BLUE, 0.7f, MOD_TOWARD_NEXT);
		aSecondKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aSecondKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aFirstKey, nullptr);
		aTrack->AddKey(aSecondKey, nullptr);

		Emitter* anEmitter = new Emitter(this, false);
		mSpeedEmitters[i] = anEmitter;
		anEmitter->SetState(STATE_Z, -0.7f);
		anEmitter->AddTrack(aTrack, nullptr);
		anEmitter->SetAdditive();
		Emitter::SetDepth(anEmitter, 0.1f);
		anEmitter->Setup(16, 8, 1, 0, 24, 0, 0, 0);
		anEmitter->SetState(STATE_ROTATION, anAngles[i]);
		anEmitter->SetState(STATE_ORBIT_ANGLE, anAngles[i]);
		Deactivate(anEmitter);
	}
}

// 0x45ca30: the percent that shows above the monkey as ground is cut
void Player::BuildPercentText()
{
	mPercentText = new ObjString();
	mPercentText->SetParent(this, nullptr);
	mPercentText->SetFont(gApp->mFont);
	mPercentText->SetStateInt(STATE_Y, -40);
	mPercentText->SetStateInt(STATE_Z, 10);
	mPercentText->SetStateInt(STATE_RED, 1);
	mPercentText->SetStateInt(STATE_GREEN, 1);
	mPercentText->SetState(STATE_BLUE, 0.2f);
	mPercentText->SetStateInt(STATE_ALPHA, 1);
	mPercentText->Create("000", false, 0);
	mPercentText->SetEventTarget(this);

	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(5);
	aFirstKey->mDrawMode = 1;
	aFirstKey->AddValue(STATE_ALPHA, -0.5f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(0x28);
	aSecondKey->mDrawMode = 1;
	aSecondKey->AddValue(STATE_ALPHA, -0.1f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	AnimKey* aThirdKey = new AnimKey();
	aThirdKey->SetDuration(1);
	aThirdKey->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aThirdKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aThirdKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aFourthKey = new AnimKey();
	aFourthKey->mEventId = EVENT_PERCENT_SHOWN;
	aFourthKey->SetDuration(1);
	aFourthKey->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aFourthKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFourthKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, nullptr);
	aTrack->AddKey(aThirdKey, nullptr);
	aTrack->AddKey(aFourthKey, nullptr);
	mPercentText->AddTrack(aTrack, nullptr);
	Deactivate(mPercentText);
}

// 0x45b610: what holds a thing the monkey has picked up, and the two keys that turn it
void Player::BuildHolder()
{
	mHolder = new ObjBase();
	mHolder->SetParent(this, nullptr);

	mFlipKeyA = new AnimKey();
	mFlipKeyA->SetDuration(0xf);
	mFlipKeyA->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mFlipKeyA->mEasing = EASE_IN_SQUARE;
	mFlipKeyB = new AnimKey();
	mFlipKeyB->SetDuration(0xf);
	mFlipKeyB->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mFlipKeyA, nullptr);
	aTrack->AddKey(mFlipKeyB, mFlipKeyB);
	mHolder->AddTrack(aTrack, nullptr);
	Activate(mHolder, false);
}

// 0x45ce10: the flash the monkey appears and goes in
void Player::BuildFlare()
{
	mFlare = new ObjBase();
	mFlare->mAnim = Anim::Get("images/flare", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mFlare->SetParent(this, nullptr);
	mFlare->SetStateInt(STATE_X, 0);
	mFlare->SetStateInt(STATE_Y, 0);
	mFlare->SetStateInt(STATE_SCALE_X, 0);
	mFlare->SetStateInt(STATE_SCALE_Y, 0);
	mFlare->SetStateInt(STATE_ALPHA, 0);
	mFlare->SetStateInt(STATE_Z, 1);
	mFlare->SetEventTarget(this);

	AnimKey* aGrowKey = new AnimKey();
	aGrowKey->SetDuration(0x1e);
	aGrowKey->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
	aGrowKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrowKey->AddValue(STATE_SCALE_Y, 0.8f, MOD_TOWARD_NEXT);
	aGrowKey->mDrawMode = 1;
	aGrowKey->mEasing = EASE_IN_SQUARE;
	AnimKey* aPeakKey = new AnimKey();
	aPeakKey->mEventId = EVENT_FLARE_PEAK;
	aPeakKey->SetDuration(0x12);
	aPeakKey->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPeakKey->AddValue(STATE_SCALE_X, 2.5f, MOD_TOWARD_NEXT);
	aPeakKey->AddValue(STATE_SCALE_Y, 2.5f, MOD_TOWARD_NEXT);
	aPeakKey->mDrawMode = 1;
	AnimKey* aFadeKey = new AnimKey();
	aFadeKey->SetDuration(1);
	aFadeKey->AddValue(STATE_ALPHA, 0.1f, MOD_HOLD);
	aFadeKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aFadeKey->AddValue(STATE_SCALE_Y, 0.6f, MOD_HOLD);
	aFadeKey->mDrawMode = 1;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aGrowKey, nullptr);
	aTrack->AddKey(aPeakKey, nullptr);
	aTrack->AddKey(aFadeKey, nullptr);
	mFlare->AddTrack(aTrack, nullptr);
}

// 0x45d130: the picture that flashes when a held thing is put down too near another
void Player::BuildTooClose()
{
	mTooClose = new ObjBase();
	sTooCloseAnim->AddRef();
	mTooClose->mAnim = sTooCloseAnim;
	mTooClose->SetParent(this, nullptr);
	mTooClose->SetStateInt(STATE_X, 0);
	mTooClose->SetStateInt(STATE_Y, 0);
	mTooClose->SetStateInt(STATE_SCALE_X, 0);
	mTooClose->SetStateInt(STATE_SCALE_Y, 0);
	mTooClose->SetStateInt(STATE_ALPHA, 0);
	mTooClose->SetStateInt(STATE_Z, 1);

	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(0xa);
	aFirstKey->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->mDrawMode = 1;
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(0x50);
	aSecondKey->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aSecondKey->mDrawMode = 1;
	AnimKey* aThirdKey = new AnimKey();
	aThirdKey->SetDuration(1);
	aThirdKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aThirdKey->AddValue(STATE_SCALE_X, 1.8f, MOD_HOLD);
	aThirdKey->AddValue(STATE_SCALE_Y, 1.8f, MOD_HOLD);
	aThirdKey->mDrawMode = 1;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, nullptr);
	aTrack->AddKey(aThirdKey, nullptr);
	mTooClose->AddTrack(aTrack, nullptr);
	Deactivate(mTooClose);
}

// 0x4598b0: starts a game afresh (theFull) or only what a level needs of it
void Player::Reset(bool theFull)
{
	Init();
	SetStateInt(STATE_Z, 500);
	mLookSpeed = 0.0f;
	mUnused414 = -128.0f;
	mPrevCellX = -99;
	mUnused418 = -32.0f;
	mPrevCellY = -99;
	mInLevel = false;
	mUnused458 = 0;
	mOverBug = false;
	mLooking = false;
	mShotTime = 0.4f;
	mUpgradeBought = 0;
	mFlareStage = 0;
	mSpeedScale = 1.0f;
	mWholePixels = true;
	mGameOver = false;
	mLives = PLAYER_LOST_LIVES_START;
	mVelX = 0.0f;
	mScore = 0;
	mVelY = 0.0f;
	mDrumIndex = 0;
	mShotPhase = SHOT_NONE;
	mTargetPercent = 0x55;
	mBananasEarned = 0;
	mShotsFired = 0;
	mPercent = 0;
	mUnused478 = false;
	mUnused2d4 = 1;
	mMonkeyState = MONKEY_INIT;
	mCellX = -1;
	mCellY = -1;
	mSegLocked = 0;
	mAimShown = false;
	mHeld = nullptr;
	mHolding = 0;
	mRopeMode = 0;
	mDeaths = 0;
	mHeldCount = 0;
	mUnused2ec = 0;
	mLostLife = false;
	mInUse = 0;
	mHeldRotation = 0;
	mBananasAtStart = mBananas;
	mScoreAtStart = 0;
	if (theFull) {
		mWorldProgress = 0.0f;
		mTenthUpgradeValue = 0;
		mBananas = 0;
		mMapLevel = 0;
		mSavedProgress = 0;
		mPrevMapLevel = 0;
		mBugKinds = 2;
		mMetaGameStarted = 0;
		mMetaProgress[0] = -1;
		mMetaProgress[1] = -1;
		mMetaProgress[2] = -1;
		for (int i = 0; i < PLAYER_UPGRADES; i++) {
			mUpgradeLevel[i] = 0;
			if (i == UPGRADE_FIRST || i == UPGRADE_FIFTH)
				mUpgradeMax[i] = 2;
			else
				mUpgradeMax[i] = 3;
			ApplyUpgrade(i);
		}
		Unblock(0);
	}
}

// 0x45adb0: the bonus may show up again; the last one let through (never the first) is remembered
void Player::Unblock(int theBonus)
{
	mBonusBlocked[theBonus] = 0;
	if (theBonus != 0)
		sLastUnblocked = theBonus;
}

// 0x45ade0: sets what the upgrade's level gives. Each starts from the bonuses it covers blocked and lets through
// more of them as the level rises
void Player::ApplyUpgrade(int theKind)
{
	int aLevel = mUpgradeLevel[theKind];
	switch (theKind) {
	case UPGRADE_FIRST:
		mFirstUpgradeFlag = 0;
		Block(0xf);
		if (aLevel == 1) {
			mFirstUpgradeFlag = 1;
			return;
		}
		if (aLevel == 2) {
			mFirstUpgradeFlag = 1;
			Unblock(0xf);
		}
		break;
	case UPGRADE_SECOND:
		if (aLevel == 1) {
			mSecondUpgradeValue = 1;
			Unblock(0x10);
			return;
		}
		if (aLevel == 2) {
			mSecondUpgradeValue = 2;
			Unblock(0x10);
			return;
		}
		if (aLevel == 3) {
			mSecondUpgradeValue = 4;
			Unblock(0x10);
			return;
		}
		mSecondUpgradeValue = 0;
		Block(0x10);
		return;
	case UPGRADE_THIRD:
		Block(1);
		Block(7);
		Block(8);
		if (aLevel == 1) {
			Unblock(7);
			return;
		}
		if (aLevel == 2) {
			Unblock(7);
			Unblock(1);
			return;
		}
		if (aLevel == 3) {
			Unblock(7);
			Unblock(1);
			Unblock(8);
			return;
		}
		break;
	case UPGRADE_SHOT_TIME:
		if (aLevel == 1) {
			mShotTime = 0.35f;
			return;
		}
		if (aLevel == 2) {
			mShotTime = 0.3f;
			return;
		}
		if (aLevel == 3) {
			mShotTime = 0.25f;
			return;
		}
		mShotTime = 0.4f;
		return;
	case UPGRADE_FIFTH:
		Block(5);
		Block(6);
		if (aLevel == 1) {
			Unblock(5);
			return;
		}
		if (aLevel == 2) {
			Unblock(5);
			Unblock(6);
			return;
		}
		break;
	case UPGRADE_SIXTH:
		Block(0xc);
		Block(0x11);
		if (aLevel == 1) {
			Unblock(0xc);
			return;
		}
		if (aLevel == 2) {
			Unblock(0xc);
			Unblock(0x11);
			return;
		}
		break;
	case UPGRADE_SEVENTH:
		if (aLevel == 1) {
			mSeventhUpgradeValue = 0.03f;
			return;
		}
		if (aLevel == 2) {
			mSeventhUpgradeValue = 0.05f;
			return;
		}
		if (aLevel == 3) {
			mSeventhUpgradeValue = 0.08f;
			return;
		}
		mSeventhUpgradeValue = 0.0f;
		return;
	case UPGRADE_BANANA_VALUE:
		if (aLevel == 1) {
			mBananaValue = 3;
			return;
		}
		if (aLevel == 2) {
			mBananaValue = 4;
			return;
		}
		if (aLevel == 3) {
			mBananaValue = 5;
			return;
		}
		mBananaValue = 2;
		return;
	case UPGRADE_NINTH:
		Block(9);
		Block(0xb);
		Block(0xa);
		if (aLevel == 1) {
			Unblock(9);
			return;
		}
		if (aLevel == 2) {
			Unblock(9);
			Unblock(0xb);
			return;
		}
		if (aLevel == 3) {
			Unblock(9);
			Unblock(0xb);
			Unblock(0xa);
			return;
		}
		break;
	case UPGRADE_TENTH:
		if (aLevel == 1) {
			mTenthUpgradeValue = 1;
			return;
		}
		if (aLevel == 2) {
			mTenthUpgradeValue = 2;
			return;
		}
		if (aLevel == 3) {
			mTenthUpgradeValue = 3;
			return;
		}
		mTenthUpgradeValue = 0;
		return;
	default:
		break;
	}
}

// 0x45b540: where in the map the game has got after theProgress levels: the world it is in (the result) and, through
// the pointers that are not nullptr, the first level of that group of levels, its size, where it ends, where its
// middle is and how far through the group the game is. A group of levels is 8 long to begin with and 4 longer for each
// world; reaching the middle or the end sets mMapEvent
int Player::MapPosition(int theProgress, int* theStart, int* theSize, int* theEnd, int* theMid, float* theWorldProgress)
{
	int anIndex = -1;
	int aSize = 8;
	int aMid = 4;
	int aWorld = 0;
	int anEnd = 8;
	int aStart = 0;
	int aHalf = 4;
	int anEvent = 0;
	if (theProgress >= 0) {
		int i = 0;
		do {
			anEvent = MAP_EVENT_NONE;
			if (i == anEnd) {
				aSize = aSize + 4;
				aHalf = aSize / 2;
				anIndex = -1;
				aWorld = aWorld + 1;
				anEvent = MAP_EVENT_NEW_WORLD;
				anEnd = aSize + i;
				aMid = aHalf + i;
				aStart = i;
			} else if (i == aMid) {
				anEvent = MAP_EVENT_HALFWAY;
				anIndex = -1;
				aMid = anEnd;
			}
			i = i + 1;
			anIndex = anIndex + 1;
		} while (i <= theProgress);
		mMapEvent = anEvent;
	}
	if (theWorldProgress != nullptr)
		*theWorldProgress = (float)((double)anIndex / (double)aHalf);
	if (theStart != nullptr)
		*theStart = aStart;
	if (theSize != nullptr)
		*theSize = aSize;
	if (theEnd != nullptr)
		*theEnd = anEnd;
	if (theMid != nullptr)
		*theMid = aMid;
	return aWorld;
}

// 0x459aa0: a key of the monkey, the flash or the percent text has ended
void Player::Notify(ObjBase* theObj)
{
	AnimKey* aKey = theObj->mCurrentKey;
	if (aKey == nullptr)
		return;
	switch (aKey->mEventId) {
	case EVENT_TANGLED_DONE: {
		SetStateInt(STATE_FRAME, 0);
		SetStateInt(STATE_ROTATION, 0);
		unsigned int aRandom = GameRandomNext();
		float anAngle = DegreesToRadians((float)(aRandom % 0x15 + 0x50));
		float aCos = FloatCos(anAngle);
		// the original starts both the sideways and the downward throw from the cosine
		mFallKey->SetDecay(STATE_X, 2.5f, aCos);
		mFallKey->SetDecay(STATE_Y, 2.5f, aCos);
		StartTrack(mFallTrack);
		PlaySound(SOUND_MONKEYFALL);
		mMonkeyState = MONKEY_FALLING;
		return;
	}
	case EVENT_HIT_GROUND:
		PlaySound(SOUND_MONKEYHIT);
		mMonkeyState = MONKEY_HIT;
		return;
	case EVENT_DEATH_DONE:
		mMonkeyState = MONKEY_DEAD;
		mScoreBoard->DeactivateHud();
		PlaySound(SOUND_DISAPPEAR);
		Activate(mFlare, false);
		mFlare->StartTrack(0);
		if (mLives < 1) {
			AddHighScoreOfCurrent();
			Reset(false);
			mInUse = 1;
			mGameOver = true;
			LevelObj::ShowGameOver();
			StartTrack(0);
			mAnim = nullptr;
			SaveAll();
		}
		mFlareStage = 1;
		return;
	case EVENT_VANISHED:
		mAnim = nullptr;
		SetHidden(true);
		return;
	case 6: // nothing
	case EVENT_PERCENT_SHOWN:
		break;
	case EVENT_RESPAWN: {
		SetHidden(true);
		mMachine->SetHidden(true);
		mGameOver = false;
		int aHalfWidth = gApp->mWidth / 2;
		int aHalfHeight = gApp->mHeight / 2;
		gApp->WarpCursor(aHalfWidth, aHalfHeight);
		ObjBase::SetMousePos(aHalfWidth, aHalfHeight);
		int aMouseX = (int)((double)gApp->mDesignScaleX * (double)sMouseX);
		mState[STATE_X] = (float)(aMouseX - 960);
		int aMouseY = (int)((double)gApp->mDesignScaleY * (double)sMouseY);
		mState[STATE_Y] = (float)(aMouseY - 600);
		ComputeDrawn();
		mMonkeyState = MONKEY_PLAYING;
		mFlareStage = 0;
		StartTrack(0);
		Activate(this, false);
		Activate(mFlare, false);
		mFlare->StartTrack(0);
		SetStateInt(STATE_ALPHA, 1);
		PlaySound(SOUND_APPEAR);
		return;
	}
	case EVENT_SPAZZ_DONE:
		mLookSpeed = 0.0f;
		mLooking = false;
		mMonkey->SetStateInt(STATE_FRAME, 0);
		mMonkey->StartTrack(mIdleTrack);
		break;
	case EVENT_FLARE_PEAK:
		if (mFlareStage == 0) {
			// the monkey shows in the flash
			SetState(STATE_X, sMouseAverageX);
			SetState(STATE_Y, sMouseAverageY);
			ShowAll();
			Activate(mMachine, false);
			Activate(mMonkey, false);
			ComputeDrawn();
			mMachine->ComputeDrawn();
			mMonkey->ComputeDrawn();
			mMonkey->StartTrack(mIdleTrack);
		} else if (mFlareStage == 2) {
			Deactivate(mMachine);
			Deactivate(mMonkey);
			Deactivate(this);
		}
		break;
	default:
		break;
	}
}

// 0x459eb0
bool Player::Update()
{
	sUpdateCount = sUpdateCount + 1;
	if (sLevel == nullptr)
		return false;
	if (mShotCooldown > 0)
		mShotCooldown = mShotCooldown - 1;
	if (mWarnCooldown > 0)
		mWarnCooldown = mWarnCooldown - 1;
	mTargetTile = nullptr;
	mOverBug = false;
	if (!ObjBase::Update())
		return false;
	if (!(Cursor::sCursor != nullptr && Cursor::sCursor->IsActive())) {
		if (mHidden)
			SetShotCooldown(SHOT_COOLDOWN_FRAMES);
		ShowAll();
	} else {
		HideAll();
	}

	RopeObj* aRope = RopeObj::Current();
	int aPhase = mShotPhase;
	if (aPhase == SHOT_FIRED) {
		if (aRope != nullptr && aRope->IsAimReady()) {
			mShotPhase = SHOT_WAITING;
			mShotWait = SHOT_WAIT_FRAMES;
		}
	} else if (aPhase == SHOT_WAITING) {
		if (mShotWait < 1)
			mShotPhase = SHOT_DONE;
		else
			mShotWait = mShotWait - 1;
	} else if (aPhase == SHOT_DONE) {
		Bonus::CancelIfDone();
		mShotPhase = SHOT_NONE;
		SetRopeMode(0);
		SetSpeedScale(1.0f);
	}

	int aState = mMonkeyState;
	if (aState == MONKEY_PARKED) {
		if ((double)mState[STATE_X] <= PARKED_STOP_SPEED)
			mState[STATE_X] = 0.0f;
		else
			mState[STATE_X] = (float)((double)mState[STATE_X] * (double)0.98f);
		float aNewY = 0.0f;
		if (PARKED_STOP_SPEED < (double)mState[STATE_X])
			aNewY = (float)((double)mState[STATE_Y] * (double)0.98f);
		mState[STATE_Y] = aNewY;
		ComputeDrawn();
	} else if (aState == MONKEY_DRUMMING) {
		if (aRope != nullptr && aRope->GetState() == 1) {
			// a drum beat every tenth frame of the drumming cycle
			int aFrame = (int)mMonkey->mState[STATE_FRAME];
			if (aFrame % 10 == 0) {
				PlaySound(mDrumSounds[mDrumIndex]);
				mDrumIndex = (mDrumIndex + 1) % PLAYER_DRUM_SOUNDS;
			}
		} else {
			mMonkeyState = MONKEY_PLAYING;
			mMonkey->StartTrack(mIdleTrack);
		}
	} else if (aState == MONKEY_PLAYING) {
		sPrevDrawnX = mDrawn[STATE_X];
		sPrevDrawnY = mDrawn[STATE_Y];
		if (!(Cursor::sCursor != nullptr && Cursor::sCursor->IsActive())) {
			SetState(STATE_X, sMouseAverageX);
			SetState(STATE_Y, sMouseAverageY);
			ComputeDrawn();
			mMachine->ComputeDrawn();
			mMonkey->ComputeDrawn();
		}
		mVelX = (float)((((double)mDrawn[STATE_X] - (double)sPrevDrawnX) + (double)mVelX) * 0.5);
		mVelY = (float)((((double)mDrawn[STATE_Y] - (double)sPrevDrawnY) + (double)mVelY) * 0.5);
		float aLocalX = mState[STATE_X] - sLevel->mState[STATE_X];
		float aLocalY = (float)(((double)mState[STATE_Y] - (double)sLevel->mState[STATE_Y]) + CELL_SHIFT_Y);
		// mTargetTile was cleared above and UpdateAim has not run yet, so this never copies
		if (mTargetTile != nullptr) {
			mPrevCellX = mCellX;
			mPrevCellY = mCellY;
		}
		mCellX = (int)(((double)aLocalX + CELL_HALF_SIZE) / CELL_SIZE);
		mCellY = (int)(((double)aLocalY + CELL_HALF_SIZE) / CELL_SIZE);
		UpdateLook();
		UpdateAim();
		if (sCurrent->mHeld == nullptr) {
			if (mSpeedScale != 1.0f && !mHidden) {
				// the monkey shakes while the rope is sped up, and the sparks sit at the sides the rope runs along
				mMonkey->SetStateInt(STATE_X, (int)(GameRandomNext() % 3) - 1);
				mMonkey->SetStateInt(STATE_Y, (int)(GameRandomNext() % 3) - 1);
				if (mSegVertical == 0) {
					Deactivate(mSpeedEmitters[0]);
					Deactivate(mSpeedEmitters[2]);
					Activate(mSpeedEmitters[1], true);
					Activate(mSpeedEmitters[3], true);
				} else {
					Deactivate(mSpeedEmitters[1]);
					Deactivate(mSpeedEmitters[3]);
					Activate(mSpeedEmitters[0], true);
					Activate(mSpeedEmitters[2], true);
				}
			} else {
				mMonkey->SetStateInt(STATE_X, 0);
				mMonkey->SetStateInt(STATE_Y, 0);
				mMonkey->ComputeDrawn();
			}
		}
	}

	// the rope's preview follows the aim; the original reads the level's rope without looking for one
	if (mTargetTile != nullptr && sLevel != nullptr && aRope != nullptr) {
		LevelObj::ShowRopePreview(mCellX, mCellY, mSegVertical, mSegFrom, mSegTo);
		aRope->SetAim(mSegVertical, mCellX, mCellY, mSegFrom, mSegTo);
		mAimShown = true;
		return true;
	}
	if (mAimShown) {
		mAimShown = false;
		LevelObj::ClearRopePreview();
	}
	if (aRope != nullptr)
		aRope->ResetIfFinished();
	return true;
}

// 0x45a3e0
void Player::Draw(Graphics* g)
{
	if (sLevel != nullptr)
		ObjBase::Draw(g);
}

// 0x45a500: the monkey looks the way the mouse moves, faster as it moves faster, and goes back to its idle when it stops
void Player::UpdateLook()
{
	if (mLooking)
		return;
	float aSpeed;
	// the original compares the difference of the two speeds, not of their sizes
	if (LOOK_MIN_DIFFERENCE <= FloatAbs(mVelX - mVelY)) {
		float aSizeX = FloatAbs(mVelX);
		float aSizeY = FloatAbs(mVelY);
		if (aSizeX <= aSizeY) {
			aSpeed = mVelY;
			if (aSpeed <= 0.0f) {
				mMonkey->StartTrack(mLookUpTrack);
				aSpeed = (float)((double)aSpeed * -1.0);
			} else {
				mMonkey->StartTrack(mLookDownTrack);
			}
			if (LOOK_MAX_SPEED < aSpeed)
				aSpeed = LOOK_MAX_SPEED;
		} else {
			aSpeed = mVelX;
			if (aSpeed <= 0.0f) {
				mMonkey->StartTrack(mLookLeftTrack);
				aSpeed = (float)((double)aSpeed * -1.0);
			} else {
				mMonkey->StartTrack(mLookRightTrack);
			}
			if (LOOK_MAX_SPEED < aSpeed)
				aSpeed = LOOK_MAX_SPEED;
			// only the sideways looks set the cel here; the others wait for the smoothed speed below
			mMonkey->SetState(STATE_FRAME, aSpeed);
		}
	} else {
		aSpeed = 0.0f;
	}
	aSpeed = (float)(((double)mLookSpeed + (double)aSpeed) * 0.5);
	mLookSpeed = aSpeed;
	if (0.0f <= aSpeed) {
		if (LOOK_MAX_SPEED < aSpeed)
			aSpeed = LOOK_MAX_SPEED;
	} else {
		aSpeed = 0.0f;
	}
	mMonkey->SetState(STATE_FRAME, aSpeed);
	if (mLookSpeed < LOOK_STOP_SPEED) {
		mLookSpeed = 0.0f;
		mMonkey->SetStateInt(STATE_FRAME, 0);
		mMonkey->StartTrack(mIdleTrack);
	}
}

// 0x45a710: the monkey, its machine and what it holds are not shown (the game's own cursor is)
void Player::HideAll()
{
	for (int i = 0; i < PLAYER_SPEED_EMITTERS; i++)
		sCurrent->Deactivate(mSpeedEmitters[i]);
	mMachine->SetHidden(true);
	mMonkey->SetHidden(true);
	SetHidden(true);
	if (sCurrent->mHeld != nullptr)
		sCurrent->mHeld->SetHidden(true);
}

// 0x45a790
void Player::ShowAll()
{
	mMachine->SetHidden(false);
	mMonkey->SetHidden(false);
	SetHidden(false);
	if (sCurrent->mHeld != nullptr)
		sCurrent->mHeld->SetHidden(false);
}

// 0x45a7c0: the monkey calls out at a bug; a second call too soon is the first voice
void Player::PlayWarning()
{
	if (mWarnCooldown < 1) {
		unsigned int aRandom = GameRandomNext();
		switch (aRandom % 19) {
		case 0:
		case 1:
			mWarnVoice = 3;
			break;
		case 2:
		case 3:
		case 4:
			mWarnVoice = 2;
			break;
		default:
			mWarnVoice = 1;
		}
	} else {
		mWarnVoice = 0;
	}
	PlaySound(mWarnSounds[mWarnVoice]);
	mWarnCooldown = WARN_COOLDOWN_FRAMES;
}

// 0x45a860: finds the stretch of ground the rope would cut from the cell the monkey points at: the cells along the row
// and along the column up to the walls (the active objects in the wall layer of the grid), and takes the shorter
void Player::UpdateAim()
{
	if (!LevelObj::IsPlaying() || mCellX < 0 || mCellY < 0 || !FillObj::IsReady()) {
		mPrevCellX = -1;
		mPrevCellY = -2;
		return;
	}
	if (PauseWin::IsBlocking())
		return;
	Player* aCurrent = sCurrent;
	if (aCurrent->mRopeMode == 0 && BugObj::FindNear(aCurrent->mScreenX, aCurrent->mScreenY, STARTLE_REACH)) {
		if (!mLooking) {
			mLooking = true;
			mMonkey->SetStateInt(STATE_FRAME, 0);
			mMonkey->StartTrack(mSpazzTrack);
			PlayWarning();
		}
		mOverBug = true;
	}
	int aCellX = mCellX;
	int aCellY = mCellY;
	if (aCellX >= GRID_COLUMNS || aCellY >= GRID_ROWS)
		return;
	int aTargetPercent = GetTargetPercent();
	if (aCurrent->mPercent >= aTargetPercent)
		return;
	FillObj* aTile = FillObj::GetTile(aCellX, aCellY);
	if (!LevelObj::IsPlaying() || aTile == nullptr || aTile->mCut || !aTile->IsActive())
		return;
	if (IsActiveObject(FillObj::GetWall(aCellX, aCellY)))
		return;
	if (mHolding != 0 || !IsActive() || !mMachine->IsActive() || !mMonkey->IsActive())
		return;
	if (mShotCooldown > 0 || mPercent >= aTargetPercent)
		return;
	mTargetTile = aTile;

	int aRight = aCellX;
	int aLeft = aCellX;
	if (aCellX < GRID_COLUMNS - 1) {
		int i = aCellX;
		do {
			if (IsActiveObject(FillObj::GetWall(i, aCellY))) {
				aRight = i;
				break;
			}
			i = i + 1;
		} while (i < GRID_COLUMNS);
	}
	if (aCellX > 0) {
		int i = aCellX;
		do {
			if (IsActiveObject(FillObj::GetWall(i, aCellY))) {
				aLeft = i;
				break;
			}
			i = i - 1;
		} while (i >= 0);
	}
	int aBottom = aCellY;
	int aTop = aCellY;
	if (aCellY < GRID_ROWS - 1) {
		int j = aCellY;
		do {
			if (IsActiveObject(FillObj::GetWall(aCellX, j))) {
				aBottom = j;
				break;
			}
			j = j + 1;
		} while (j < GRID_ROWS);
	}
	if (aCellY > 0) {
		int j = aCellY;
		do {
			if (IsActiveObject(FillObj::GetWall(aCellX, j))) {
				aTop = j;
				break;
			}
			j = j - 1;
		} while (j >= 0);
	}

	if (mSegLocked == 0) {
		if (aRight - aLeft < aBottom - aTop) {
			mSegFrom = aLeft;
			mSegTo = aRight;
			mSegVertical = 0;
			return;
		}
		mSegVertical = 1;
	} else if (mSegVertical == 0) {
		mSegFrom = aLeft;
		mSegTo = aRight;
		return;
	}
	mSegTo = aBottom;
	mSegFrom = aTop;
}

// 0x45a400: the monkey comes back at the mouse, in a flash, with the cursor moved to the middle of the window
void Player::AppearAtMouse()
{
	Player* aPlayer = sCurrent;
	aPlayer->SetHidden(true);
	int aHalfHeight = gApp->mHeight / 2;
	gApp->WarpCursor(gApp->mWidth / 2, aHalfHeight);
	ObjBase::SetMousePos(gApp->mWidth / 2, gApp->mHeight / 2);
	aPlayer->mState[STATE_X] = (float)((double)gApp->mDesignScaleX * (double)sMouseX - DESIGN_HALF_WIDTH);
	aPlayer->mState[STATE_Y] = (float)((double)gApp->mDesignScaleY * (double)sMouseY - DESIGN_HALF_HEIGHT);
	aPlayer->ComputeDrawn();
	aPlayer->mMonkeyState = MONKEY_PLAYING;
	aPlayer->Activate(aPlayer, false);
	aPlayer->StartTrack(0);
	aPlayer->mFlareStage = 0;
	aPlayer->Activate(aPlayer->mFlare, false);
	aPlayer->mFlare->StartTrack(0);
	PlaySound(SOUND_APPEAR);
}

// 0x45ab70: the monkey takes theObj, which may be put down theCount times
void Player::PickUp(HeldObj* theObj, int theCount)
{
	Player* aPlayer = sCurrent;
	if (aPlayer->mInLevel && aPlayer->mHeld == nullptr) {
		aPlayer->mHeldOldParent = theObj->mParent;
		theObj->SetParent(aPlayer->mHolder, nullptr);
		aPlayer->mHeldRotation = 0;
		aPlayer->mHeld = theObj;
		aPlayer->mHolding = 1;
		aPlayer->mHeldCount = theCount;
	}
}

// 0x45abd0: lets go of what is held and gives it back to its old parent
HeldObj* Player::Drop()
{
	Player* aPlayer = sCurrent;
	if (aPlayer->mHeld != nullptr)
		aPlayer->mHeld->SetParent(aPlayer->mHeldOldParent, nullptr);
	HeldObj* aHeld = aPlayer->mHeld;
	aPlayer->mHeld = nullptr;
	aPlayer->mHolding = 0;
	aPlayer->mHeldCount = 0;
	aPlayer->mHeldRotation = 0;
	return aHeld;
}

// 0x45ac20: the held thing turns a quarter; the holder's turn goes from the angle it was at to the new one
int Player::RotateHeld()
{
	Player* aPlayer = sCurrent;
	aPlayer->mHeldRotation = (aPlayer->mHeldRotation + 1) % 4;
	float aFrom;
	float aTo;
	switch (aPlayer->mHeldRotation) {
	case 0:
		aFrom = 180.0f;
		aTo = 90.0f;
		break;
	case 1:
		aFrom = 90.0f;
		aTo = 0.0f;
		break;
	case 2:
		aFrom = 360.0f;
		aTo = 270.0f;
		break;
	case 3:
		aFrom = 270.0f;
		aTo = 180.0f;
		break;
	default:
		return -1;
	}
	aPlayer->mFlipKeyA->mValue[STATE_ROTATION] = DegreesToRadians(aFrom);
	aPlayer->mFlipKeyB->mValue[STATE_ROTATION] = DegreesToRadians(aTo);
	aPlayer->mHolder->StartTrack(0);
	return aPlayer->mHeldRotation;
}

// 0x45ad10: the held thing is turned over, between the two ways it may lie
int Player::FlipHeld()
{
	Player* aPlayer = sCurrent;
	aPlayer->mHeldRotation = (aPlayer->mHeldRotation == 1 ? 1 : 0) * 2 + 1;
	float aFrom;
	float aTo;
	if (aPlayer->mHeldRotation == 1) {
		aFrom = 180.0f;
		aTo = 0.0f;
	} else {
		aFrom = 0.0f;
		aTo = 180.0f;
	}
	aPlayer->mFlipKeyA->mValue[STATE_ROTATION] = DegreesToRadians(aFrom);
	aPlayer->mFlipKeyB->mValue[STATE_ROTATION] = DegreesToRadians(aTo);
	aPlayer->mHolder->StartTrack(0);
	return aPlayer->mHeldRotation;
}

// 0x45b170
void Player::AddLives(int theCount)
{
	sCurrent->mLives = sCurrent->mLives + theCount;
}

// 0x45b180: buys the upgrade a level higher for theCost bananas and saves; the result is 0 when it was bought, else the
// bananas that are short. A cost of 0 buys nothing
int Player::BuyUpgrade(int theKind, int theCost)
{
	Player* aPlayer = sCurrent;
	if (theCost != 0) {
		int aBananas = aPlayer->mBananas;
		if (aBananas < theCost)
			return theCost - aBananas;
		aPlayer->mBananas = aBananas - theCost;
		if (aPlayer->mUpgradeLevel[theKind] < aPlayer->mUpgradeMax[theKind])
			aPlayer->mUpgradeLevel[theKind] = aPlayer->mUpgradeLevel[theKind] + 1;
		aPlayer->ApplyUpgrade(theKind);
		SaveAll();
	}
	return 0;
}

// 0x45b1d0
void Player::MarkUpgradeBought()
{
	sCurrent->mUpgradeBought = 1;
}

// 0x45b1e0
int Player::TakeUpgradeBought()
{
	int aBought = sCurrent->mUpgradeBought;
	sCurrent->mUpgradeBought = 0;
	return aBought;
}

// 0x45b200
int Player::SpendBananas()
{
	int aBananas = sCurrent->mBananas;
	if (99 < aBananas) {
		sCurrent->mBananas = aBananas - 100;
		return 0;
	}
	return 100 - aBananas;
}

// 0x45b230
int Player::GetUpgradeLevel(int theKind)
{
	return sCurrent->mUpgradeLevel[theKind];
}

// 0x45b240
double Player::GetSpeedAverage()
{
	return ((double)sCurrent->mVelY + (double)sCurrent->mVelX) * 0.5;
}

// 0x45b260: a level is about to be played: the counts of the level begin again
void Player::StartLevelPlay()
{
	Player* aPlayer = sCurrent;
	if (aPlayer->mMonkeyState < MONKEY_PLAYING)
		aPlayer->mMonkeyState = MONKEY_PLAYING;
	aPlayer->mScoreBeforeLevel = aPlayer->mScore;
	aPlayer->mShotsFired = 0;
	aPlayer->mBananasEarned = 0;
	aPlayer->mPercent = 0;
	aPlayer->mExtraPercent = 0;
	aPlayer->mBugCount = 0;
	aPlayer->mBugsKilled = 0;
	if (aPlayer->mScoreBoard != nullptr)
		aPlayer->mScoreBoard->ShowPercent(0);
}

// 0x45b2c0
void Player::EndLevelPlay()
{
	if (sCurrent->mMonkeyState == MONKEY_PLAYING)
		sCurrent->mMonkeyState = MONKEY_PARKED;
}

// 0x45b2e0: puts the monkey at a place in the window and the cursor on it
void Player::MoveTo(int theWindowX, int theWindowY)
{
	Player* aPlayer = sCurrent;
	aPlayer->mState[STATE_X] = (float)((double)gApp->mWindowToDesignX * (double)(float)theWindowX - DESIGN_HALF_WIDTH);
	aPlayer->mState[STATE_Y] = (float)((double)gApp->mWindowToDesignY * (double)(float)theWindowY - DESIGN_HALF_HEIGHT);
	aPlayer->ComputeDrawn();
	gApp->WarpCursor((int)aPlayer->mDrawn[STATE_X], (int)aPlayer->mDrawn[STATE_Y]);
}

// 0x45b340: a bug has touched the rope: the monkey is tangled in it and loses a life
void Player::LoseLife()
{
	Player* aPlayer = sCurrent;
	if (aPlayer->mLooking) {
		aPlayer->mLookSpeed = 0.0f;
		aPlayer->mLooking = false;
		aPlayer->mMonkey->SetStateInt(STATE_FRAME, 0);
		aPlayer->mMonkey->StartTrack(aPlayer->mIdleTrack);
	}
	Bonus::Cancel();
	aPlayer->mLostLife = true;
	SetRopeMode(0);
	SetSpeedScale(1.0f);
	aPlayer->mLives = aPlayer->mLives - 1;
	aPlayer->mShotPhase = SHOT_NONE;
	if (sLevel != nullptr) {
		LevelObj::ShowLives(aPlayer->mLives);
		WalkThru::OnLoseLife();
	}
	aPlayer->mMonkeyState = MONKEY_TANGLED;
	aPlayer->mMonkey->SetStateInt(STATE_FRAME, 0);
	aPlayer->StartTrack(aPlayer->mTangledTrack);
	aPlayer->mScoreBoard->ActivateHud();
	aPlayer->Deactivate(aPlayer->mMachine);
	aPlayer->Deactivate(aPlayer->mMonkey);
	PlaySound(SOUND_BOLT);
	aPlayer->mDeaths = aPlayer->mDeaths + 1;
	// every second death makes the bugs fewer kinds, down to 2
	if ((aPlayer->mDeaths & 1) == 0) {
		aPlayer->mDeaths = 0;
		if (aPlayer->mBugKinds > 2)
			aPlayer->mBugKinds = aPlayer->mBugKinds - 1;
	}
	char aText[24];
	int aLives = aPlayer->mLives;
	if (aLives < 1) {
		snprintf(aText, sizeof(aText), "NO LIVES LEFT");
		LevelObj::HideMenuButton();
	} else if (aLives == 1) {
		snprintf(aText, sizeof(aText), "1 LIFE LEFT");
	} else {
		snprintf(aText, sizeof(aText), "%d LIVES LEFT", aLives);
	}
	LevelObj::ShowMessage(aText, MESSAGE_X, 496.0f, 60, 1.5f, 1.0f, 0.35f, 0.0f, -1, 100);
}

// 0x45d460
void Player::AddBananas(int theCount)
{
	sCurrent->mBananas = sCurrent->mBananas + theCount;
}

// 0x45d450
int Player::GetBananaValue()
{
	return sCurrent->mBananaValue;
}

// 0x45d440
int Player::GetTargetPercent()
{
	return sCurrent->mTargetPercent;
}

// 0x45d470
int Player::GetRopeMode()
{
	return sCurrent->mRopeMode;
}

// 0x45d480
void Player::SetRopeMode(int theMode)
{
	sCurrent->mRopeMode = theMode;
}

// 0x45d490
double Player::GetShotInterval()
{
	return (double)sCurrent->mSpeedScale * (double)sCurrent->mShotTime;
}

// 0x45d4b0: the rope's speed; at normal speed or above the sparks of the sped-up rope are put out
void Player::SetSpeedScale(float theScale)
{
	Player* aPlayer = sCurrent;
	aPlayer->mSpeedScale = theScale;
	if (theScale >= 1.0f) {
		aPlayer->Deactivate(aPlayer->mSpeedEmitters[0]);
		sCurrent->Deactivate(sCurrent->mSpeedEmitters[1]);
		sCurrent->Deactivate(sCurrent->mSpeedEmitters[2]);
		sCurrent->Deactivate(sCurrent->mSpeedEmitters[3]);
	}
}

// 0x45d520: whether theObj is within theReach (in design pixels, scaled to the window) of the monkey, which is true
// only while the monkey is neither tangled nor falling
bool Player::IsNear(ObjBase* theObj, float theReach)
{
	Player* aPlayer = sCurrent;
	if (aPlayer->mMonkeyState < MONKEY_TANGLED) {
		float aDeltaY = theObj->mDrawn[STATE_Y] - aPlayer->mDrawn[STATE_Y];
		float aDeltaX = theObj->mDrawn[STATE_X] - aPlayer->mDrawn[STATE_X];
		float aDistance = FloatSqrt((float)((double)aDeltaX * (double)aDeltaX + (double)aDeltaY * (double)aDeltaY));
		double aReach = (double)gImageScaleY * (double)theReach;
		if (aReach >= (double)aDistance)
			return true;
	}
	return false;
}

// 0x45d590
void Player::AddScore(int theScore)
{
	Player* aPlayer = sCurrent;
	aPlayer->mScore = aPlayer->mScore + theScore;
	aPlayer->mScoreBoard->ShowScore(aPlayer->mScore);
}

// 0x45d5b0: the percent of the ground cut changes: the text above the monkey, the board, the percent above the level's
// target and, with every fifth percent, a banana
void Player::SetPercent(int thePercent)
{
	if (thePercent != sCurrent->mPercent) {
		snprintf(sPercentBuffer, sizeof(sPercentBuffer), "%02d%c", thePercent, '%');
		sCurrent->mPercentText->SetText(sPercentBuffer);
		sCurrent->Activate(sCurrent->mPercentText, true);
		sCurrent->mPercentText->StartTrack(0);
		sCurrent->mPercent = thePercent;
		sCurrent->mScoreBoard->ShowPercent(thePercent);
		Player* aPlayer = sCurrent;
		if (aPlayer->mTargetPercent < thePercent)
			aPlayer->mExtraPercent = thePercent - aPlayer->mTargetPercent;
		else
			aPlayer->mExtraPercent = 0;
		if (thePercent % 5 == 4) {
			aPlayer->mBananasEarned = aPlayer->mBananasEarned + 1;
			aPlayer->mScoreBoard->ShowBananaIcon();
		}
	}
}

// 0x45d690: marks the last banana shown, and says whether any was earned
bool Player::MarkLastBananaIcon()
{
	Player* aPlayer = sCurrent;
	aPlayer->mScoreBoard->MarkLastIcon();
	return 0 < aPlayer->mBananasEarned;
}

// 0x45d6b0
void Player::CountBug()
{
	sCurrent->mBugCount = sCurrent->mBugCount + 1;
}

// 0x45d6c0
void Player::CountKill()
{
	sCurrent->mBugsKilled = sCurrent->mBugsKilled + 1;
}

// 0x45d6d0: what the map is to do after the last level, and with theClear it is taken, so it is done once
int Player::TakeMapEvent(bool theClear)
{
	int anEvent = sCurrent->mMapEvent;
	if (theClear)
		sCurrent->mMapEvent = 0;
	return anEvent;
}

// 0x45d6f0: the level is played again: what it started with comes back
void Player::RestoreLevelStart()
{
	Player* aPlayer = sCurrent;
	aPlayer->mBananas = aPlayer->mBananasAtStart;
	aPlayer->mScore = aPlayer->mScoreAtStart;
	SetSpeedScale(1.0f);
}

// 0x45d720
void Player::StartLevel()
{
	Player* aPlayer = sCurrent;
	aPlayer->mBananasAtStart = aPlayer->mBananas;
	aPlayer->mScoreAtStart = aPlayer->mScore;
	aPlayer->mInLevel = true;
	SetRopeMode(0);
	SetSpeedScale(1.0f);
}

// 0x45d760: the level is won: the game moves on, the bananas and the bonus for the level are added, the map moves
// and the profile is saved
void Player::FinishLevel()
{
	int aStart;
	int aSize;
	int anEnd;
	int aMid;
	Player* aPlayer = sCurrent;
	aPlayer->mSavedProgress = aPlayer->mSavedProgress + 1;
	aPlayer->mDeaths = 0;
	aPlayer->mShotPhase = SHOT_NONE;
	aPlayer->mInLevel = false;
	aPlayer->mBananas = aPlayer->mBananas + GetBananaValue() * aPlayer->mBananasEarned;
	aPlayer->mPrevMapLevel = aPlayer->mMapLevel;
	aPlayer->mMapLevel = aPlayer->MapPosition(aPlayer->mSavedProgress, &aStart, &aSize, &anEnd, &aMid,
		&aPlayer->mWorldProgress);
	aPlayer->Unblock(0);
	if (aPlayer->mMapEvent == MAP_EVENT_NEW_WORLD && aPlayer->mBugKinds < 8)
		aPlayer->mBugKinds = aPlayer->mBugKinds + 1;
	if (aPlayer->mMapEvent != MAP_EVENT_NONE) {
		aPlayer->mWorldProgress = 1.0f;
		aPlayer->Block(2);
		aPlayer->Block(3);
		aPlayer->Block(4);
		aPlayer->Block(14);
		aPlayer->Block(18);
	}
	aPlayer->mLevelBonus = 0;
	if (aPlayer->mShotsFired < 4)
		aPlayer->mLevelBonus = (4 - aPlayer->mShotsFired) * 100;
	aPlayer->mLevelBonus = aPlayer->mLevelBonus + aPlayer->mExtraPercent * 100;
	if (aPlayer->mBugCount <= aPlayer->mBugsKilled)
		aPlayer->mLevelBonus = aPlayer->mLevelBonus + 1000;
	aPlayer->mScore = aPlayer->mScore + aPlayer->mLevelBonus;
	aPlayer->mScoreBoard->ShowScore(aPlayer->mScore);
	sCurrent->mHolding = 0;
	SaveAll();
}

// 0x45d8c0
void Player::Reactivate()
{
	sCurrent->Deactivate(sCurrent);
	sCurrent->Activate(sCurrent, false);
}

// 0x45d8e0: a click in the level: with a thing held it turns it (a click of the second button) or puts it down;
// otherwise the first button shoots the rope and the second turns the stretch it would cut. A click that does neither
// sounds the buzzer unless a window is open
void Player::BoardClick(int theButton)
{
	Player* aPlayer = sCurrent;
	if (aPlayer == nullptr || PauseWin::IsBlocking() || !LevelObj::IsPlaying() || !aPlayer->IsActive() ||
		aPlayer->mHidden)
		return;
	if (aPlayer->mHolding == 1) {
		if (aPlayer->mHeld != nullptr) {
			if (theButton < 0) {
				aPlayer->mHeld->Rotate();
				return;
			}
			if (!aPlayer->mHeld->Place()) {
				sCurrent->Activate(sCurrent->mTooClose, false);
				sCurrent->mTooClose->StartTrack(0);
				return;
			}
			sCurrent->mHeldCount = sCurrent->mHeldCount - 1;
			sCurrent->mHolding = 0 < sCurrent->mHeldCount ? 1 : 0;
			return;
		}
		aPlayer->mHolding = 0;
	} else {
		if (theButton < 0) {
			if (aPlayer->mTargetTile != nullptr) {
				TurnRope();
				WalkThru::OnTurn();
				return;
			}
		} else if (aPlayer->mTargetTile != nullptr && !aPlayer->mOverBug) {
			Shoot();
			SetSpeedScale(1.0f);
			return;
		}
		if (BBWindow::sOpenCount < 1)
			PlaySound(SOUND_BUZZER);
	}
}

// 0x45da10: shoots the rope along the stretch the monkey points at, if the monkey may shoot
void Player::Shoot()
{
	Player* aPlayer = sCurrent;
	if (aPlayer->mShotCooldown < 1 && aPlayer->mShotPhase == SHOT_NONE) {
		WalkThru::OnShoot();
		aPlayer->mSegLocked = 0;
		if (aPlayer->mMonkeyState == MONKEY_PLAYING) {
			if (aPlayer->mLooking) {
				aPlayer->mLookSpeed = 0.0f;
				aPlayer->mLooking = false;
				aPlayer->mMonkey->SetStateInt(STATE_FRAME, 0);
				aPlayer->mMonkey->StartTrack(aPlayer->mIdleTrack);
			}
			aPlayer->mShotCooldown = SHOT_COOLDOWN_FRAMES;
			PlaySound(SOUND_ARROWSHOOT);
			aPlayer->mShotPhase = SHOT_FIRED;
			aPlayer->mShotX = (int)aPlayer->mDrawn[STATE_X];
			aPlayer->mShotY = (int)aPlayer->mDrawn[STATE_Y];
			if (aPlayer->mMonkeyState == MONKEY_PLAYING) {
				BugObj::ResetKillStreak();
				aPlayer->mShotsFired = aPlayer->mShotsFired + 1;
				aPlayer->mUnused2d4 = 1;
				// the original reads the level's rope with no check for one; both of its branches pass the same
				RopeObj* aRope = RopeObj::Current();
				if (aRope != nullptr)
					aRope->Start(aPlayer->mSegVertical, aPlayer->mCellX, aPlayer->mCellY, aPlayer->mSegFrom,
						aPlayer->mSegTo);
				sCurrent->mMonkeyState = MONKEY_DRUMMING;
				sCurrent->mMonkey->StartTrack(sCurrent->mDrumTrack);
			}
		}
	}
}

// 0x45db70: asks the rope whether it touches the box the bug that was last looked at takes
int Player::HitBugBox()
{
	RopeObj* aRope = RopeObj::Current();
	if (aRope != nullptr)
		return aRope->HitBox(gBugBoxX, gBugBoxY, gBugBoxWidth, gBugBoxHeight);
	return 0;
}

// 0x45dbb0: turns the stretch the rope would cut, from along the row to along the column or back, and keeps it so
void Player::TurnRope()
{
	Player* aPlayer = sCurrent;
	if (aPlayer->mTargetTile != nullptr && aPlayer->mShotPhase == SHOT_NONE) {
		aPlayer->mSegLocked = 1;
		aPlayer->mSegVertical = aPlayer->mSegVertical == 0 ? 1 : 0;
		aPlayer->mPrevCellX = -999;
		aPlayer->mPrevCellY = -999;
		aPlayer->UpdateAim();
		LevelObj::ShowRopePreview(aPlayer->mCellX, aPlayer->mCellY, aPlayer->mSegVertical, aPlayer->mSegFrom,
			aPlayer->mSegTo);
	}
}

// 0x45dc30
void Player::SetScoreBoard(ScoreBoard* theBoard)
{
	for (int i = 0; i < PLAYER_PROFILES; i++)
		sProfiles[i]->mScoreBoard = theBoard;
}

// 0x45dca0: gives a profile a name and makes it the current one. The slot is the profile to take, or any number outside
// 0 to 7 for the first one not in use. The result is the slot, -1 for a name that is empty or starts with a space or a
// control character, -2 when no profile is free and -3 when another has the name (without regard to case)
int Player::AddProfile(int theSlot, const char* theName)
{
	if (theName != nullptr && theName[0] != '\0' && ' ' < theName[0]) {
		if (theSlot < 0 || 7 < theSlot) {
			FindFreeSlot();
			theSlot = sFreeSlot;
		}
		if (theSlot < 0)
			return -2;
		int i = 0;
		do {
			const char* aName = sProfiles[i]->mName;
			if (aName[0] != '\0' && i != theSlot) {
				if (_stricmp(theName, aName) == 0)
					return -3;
			}
			i = i + 1;
		} while (i < PLAYER_PROFILES);
		strncpy(sProfiles[theSlot]->mName, theName, 0x1f);
		sCurrentIndex = theSlot;
		SetCurrent(theSlot);
		SaveAll();
		return sCurrentIndex;
	}
	return -1;
}

// 0x45dd80: whether no other profile than theSkip has the name
bool Player::NameIsFree(const char* theName, int theSkip)
{
	int i = 0;
	do {
		if (i != theSkip) {
			const char* aName = sProfiles[i]->mName;
			if (aName[0] != '\0') {
				if (_stricmp(theName, aName) == 0)
					return false;
			}
		}
		i = i + 1;
	} while (i < PLAYER_PROFILES);
	return true;
}

// 0x45dde0
const char* Sexy::GetProfileName()
{
	if (-1 < Player::sCurrentIndex && Player::sProfiles[Player::sCurrentIndex]->mInUse != 0)
		return Player::sProfiles[Player::sCurrentIndex]->mName;
	return nullptr;
}

// 0x45de10: the name of a profile in use, or nullptr
const char* Player::GetNameAt(int theIndex)
{
	if (sProfiles[theIndex]->mInUse != 0)
		return sProfiles[theIndex]->mName;
	return nullptr;
}

// 0x45de30: with no current profile, the first one in use becomes it
int Player::ChooseFirstInUse()
{
	int anIndex = sCurrentIndex;
	if (sCurrentIndex < 0) {
		anIndex = 0;
		while (sProfiles[anIndex]->mInUse == 0) {
			anIndex = anIndex + 1;
			if (7 < anIndex)
				return sCurrentIndex;
		}
	}
	sCurrentIndex = anIndex;
	return anIndex;
}

// 0x45de70: the meta game (the map) has started, and a progress number of it (theSlot 0 to 2, or below 0 for none) is
// set; the original does not check the slot
void Player::StartMetaGame(int theSlot, int theValue)
{
	sCurrent->mMetaGameStarted = 1;
	if (-1 < theSlot)
		sCurrent->mMetaProgress[theSlot] = theValue;
}

// 0x45de90
void Player::EndMetaGame()
{
	sCurrent->mMetaGameStarted = 0;
}

// 0x45dea0: the profiles in use come first, the others after, each in the order it was in. The original sets the
// current index to the profile's old place, not the new one, when the current profile is in use
void Player::SortProfiles()
{
	Player* aSorted[PLAYER_PROFILES];
	Player* aCurrent = sCurrent;
	int aCount = 0;
	for (int i = 0; i < PLAYER_PROFILES; i++) {
		Player* aProfile = sProfiles[i];
		if (aProfile->mInUse != 0) {
			aSorted[aCount] = aProfile;
			if (aCurrent == aProfile)
				sCurrentIndex = i;
			aCount = aCount + 1;
		}
	}
	for (int i = 0; i < PLAYER_PROFILES; i++) {
		Player* aProfile = sProfiles[i];
		if (aProfile->mInUse == 0) {
			aSorted[aCount] = aProfile;
			aCount = aCount + 1;
		}
	}
	for (int i = 0; i < PLAYER_PROFILES; i++)
		sProfiles[i] = aSorted[i];
}

// 0x45e050: empties a profile, leaving no profile current
bool Player::DeleteProfile(int theIndex)
{
	if ((unsigned int)theIndex < (unsigned int)PLAYER_PROFILES) {
		sProfiles[theIndex]->mInUse = 0;
		sProfiles[theIndex]->Reset(true);
		sProfiles[theIndex]->mTutorialDone = 0;
		sProfiles[theIndex]->mName[0] = '\0';
		sCurrentIndex = -1;
		SortProfiles();
		return true;
	}
	return false;
}

// 0x45e0b0: makes a profile the current one, in use, and puts all the profiles' objects away; an index outside 0 to 7
// leaves none
void Player::SetCurrent(int theIndex)
{
	for (int i = 0; i < PLAYER_PROFILES; i++)
		sProfiles[i]->Deactivate(sProfiles[i]);
	if ((unsigned int)theIndex < (unsigned int)PLAYER_PROFILES) {
		sCurrent = sProfiles[theIndex];
		sCurrentIndex = theIndex;
		sCurrent->mInUse = 1;
		return;
	}
	sCurrent = nullptr;
	sCurrentIndex = -1;
}

// profiles.dat: one record of PROFILE_RECORD_SIZE bytes for each of the 8 profiles; the fields are 32-bit numbers,
// little-endian, at these places (the first 0x20 bytes are the name, the rest of them zero)
enum ProfileField
{
	PROFILE_NAME = 0x00,
	PROFILE_IS_CURRENT = 0x20,
	PROFILE_IN_USE = 0x24,
	PROFILE_META_STARTED = 0x28,
	PROFILE_META_PROGRESS = 0x2c,     // three numbers
	PROFILE_SAVED_PROGRESS = 0x38,
	PROFILE_LIVES = 0x3c,
	PROFILE_BUG_KINDS = 0x40,
	PROFILE_BANANAS = 0x44,
	PROFILE_SCORE = 0x48,
	PROFILE_WALK_THRU_DONE = 0x4c,
	PROFILE_TUTORIAL_DONE = 0x50,
	PROFILE_BLOCKED_BASE = 0x54,      // the numbers of bonuses 2, 3 and 4
	PROFILE_BLOCKED_FOURTEEN = 0x60,
	PROFILE_BLOCKED_EIGHTEEN = 0x64,
	PROFILE_UPGRADES = 0x68,          // the ten upgrades' levels
};

static const char* PROFILES_FILE_NAME = "profiles.dat";

static void PutNumber(unsigned char* theRecord, int theOffset, int theValue)
{
	unsigned int aValue = (unsigned int)theValue;
	theRecord[theOffset] = (unsigned char)(aValue & 0xff);
	theRecord[theOffset + 1] = (unsigned char)((aValue >> 8) & 0xff);
	theRecord[theOffset + 2] = (unsigned char)((aValue >> 16) & 0xff);
	theRecord[theOffset + 3] = (unsigned char)((aValue >> 24) & 0xff);
}

static int GetNumber(const unsigned char* theRecord, int theOffset)
{
	unsigned int aValue = (unsigned int)theRecord[theOffset] | ((unsigned int)theRecord[theOffset + 1] << 8) |
		((unsigned int)theRecord[theOffset + 2] << 16) | ((unsigned int)theRecord[theOffset + 3] << 24);
	return (int)aValue;
}

// A profile's record (the name's characters, with no end mark added, then the numbers)
static void WriteRecord(unsigned char* theRecord, Player* thePlayer, int theIsCurrent)
{
	memset(theRecord, 0, PROFILE_RECORD_SIZE);
	memcpy(theRecord + PROFILE_NAME, thePlayer->mName, strlen(thePlayer->mName));
	PutNumber(theRecord, PROFILE_IS_CURRENT, theIsCurrent);
	PutNumber(theRecord, PROFILE_IN_USE, thePlayer->mInUse);
	PutNumber(theRecord, PROFILE_META_STARTED, thePlayer->mMetaGameStarted);
	PutNumber(theRecord, PROFILE_SAVED_PROGRESS, thePlayer->mSavedProgress);
	PutNumber(theRecord, PROFILE_LIVES, thePlayer->mLives);
	PutNumber(theRecord, PROFILE_BUG_KINDS, thePlayer->mBugKinds);
	PutNumber(theRecord, PROFILE_BANANAS, thePlayer->mBananas);
	PutNumber(theRecord, PROFILE_SCORE, thePlayer->mScore);
	PutNumber(theRecord, PROFILE_WALK_THRU_DONE, thePlayer->mWalkThruDone);
	PutNumber(theRecord, PROFILE_TUTORIAL_DONE, thePlayer->mTutorialDone);
	PutNumber(theRecord, PROFILE_BLOCKED_BASE, thePlayer->mBonusBlocked[2]);
	PutNumber(theRecord, PROFILE_BLOCKED_BASE + 4, thePlayer->mBonusBlocked[3]);
	PutNumber(theRecord, PROFILE_BLOCKED_BASE + 8, thePlayer->mBonusBlocked[4]);
	PutNumber(theRecord, PROFILE_BLOCKED_FOURTEEN, thePlayer->mBonusBlocked[14]);
	PutNumber(theRecord, PROFILE_BLOCKED_EIGHTEEN, thePlayer->mBonusBlocked[18]);
	for (int i = 0; i < 3; i++)
		PutNumber(theRecord, PROFILE_META_PROGRESS + 4 * i, thePlayer->mMetaProgress[i]);
	for (int i = 0; i < PLAYER_UPGRADES; i++)
		PutNumber(theRecord, PROFILE_UPGRADES + 4 * i, thePlayer->mUpgradeLevel[i]);
}

// 0x45e110: saves the eight profiles: first those in use, as they are, the one that is current marked, then the
// others, each after being reset
void Player::SaveAll()
{
	unsigned char aFile[PROFILE_FILE_SIZE];
	int aRecord = 0;
	for (int i = 0; i < PLAYER_PROFILES; i++) {
		Player* aPlayer = sProfiles[i];
		if (aPlayer->mInUse != 0) {
			WriteRecord(aFile + aRecord * PROFILE_RECORD_SIZE, aPlayer, sCurrentIndex == i ? 1 : 0);
			aRecord = aRecord + 1;
		}
	}
	for (int i = 0; i < PLAYER_PROFILES; i++) {
		Player* aPlayer = sProfiles[i];
		if (aPlayer->mInUse == 0) {
			aPlayer->Reset(true);
			WriteRecord(aFile + aRecord * PROFILE_RECORD_SIZE, aPlayer, 0);
			aRecord = aRecord + 1;
		}
	}
	gApp->WriteBytesToFile(PROFILES_FILE_NAME, aFile, PROFILE_FILE_SIZE);
}

// 0x45e4c0: loads the eight profiles from profiles.dat; a file of any other size is not read
void Player::LoadFile()
{
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(PROFILES_FILE_NAME, &aBuffer))
		return;
	if (aBuffer.GetDataLen() != PROFILE_FILE_SIZE)
		return;
	const unsigned char* aFile = aBuffer.GetDataPtr();
	int aCurrentIndex = -1;
	int aStart;
	int aSize;
	int anEnd;
	int aMid;
	for (int i = 0; i < PLAYER_PROFILES; i++) {
		Player* aPlayer = sProfiles[i];
		const unsigned char* aRecord = aFile + i * PROFILE_RECORD_SIZE;
		aPlayer->Reset(true);
		int aWasCurrent = GetNumber(aRecord, PROFILE_IS_CURRENT);
		char aName[PROFILE_RECORD_SIZE];
		memcpy(aName, aRecord + PROFILE_NAME, PLAYER_NAME_SIZE);
		aName[PLAYER_NAME_SIZE] = '\0';
		strncpy(aPlayer->mName, aName, 0x1f);
		aPlayer->mInUse = GetNumber(aRecord, PROFILE_IN_USE);
		aPlayer->mMetaGameStarted = GetNumber(aRecord, PROFILE_META_STARTED);
		aPlayer->mSavedProgress = GetNumber(aRecord, PROFILE_SAVED_PROGRESS);
		aPlayer->mLives = GetNumber(aRecord, PROFILE_LIVES);
		aPlayer->mBugKinds = GetNumber(aRecord, PROFILE_BUG_KINDS);
		aPlayer->mBananas = GetNumber(aRecord, PROFILE_BANANAS);
		aPlayer->mScore = GetNumber(aRecord, PROFILE_SCORE);
		aPlayer->mTutorialDone = GetNumber(aRecord, PROFILE_TUTORIAL_DONE);
		aPlayer->mWalkThruDone = GetNumber(aRecord, PROFILE_WALK_THRU_DONE);
		aPlayer->mBonusBlocked[2] = GetNumber(aRecord, PROFILE_BLOCKED_BASE);
		aPlayer->mBonusBlocked[3] = GetNumber(aRecord, PROFILE_BLOCKED_BASE + 4);
		aPlayer->mBonusBlocked[4] = GetNumber(aRecord, PROFILE_BLOCKED_BASE + 8);
		aPlayer->mBonusBlocked[14] = GetNumber(aRecord, PROFILE_BLOCKED_FOURTEEN);
		aPlayer->mBonusBlocked[18] = GetNumber(aRecord, PROFILE_BLOCKED_EIGHTEEN);
		for (int k = 0; k < 3; k++)
			aPlayer->mMetaProgress[k] = GetNumber(aRecord, PROFILE_META_PROGRESS + 4 * k);
		for (int k = 0; k < PLAYER_UPGRADES; k++) {
			aPlayer->mUpgradeLevel[k] = GetNumber(aRecord, PROFILE_UPGRADES + 4 * k);
			aPlayer->ApplyUpgrade(k);
		}
		int aMapLevel = aPlayer->MapPosition(aPlayer->mSavedProgress, &aStart, &aSize, &anEnd, &aMid,
			&aPlayer->mWorldProgress);
		aPlayer->mMapLevel = aMapLevel;
		if (aMapLevel < 1)
			aPlayer->mPrevMapLevel = 0;
		else
			aPlayer->mPrevMapLevel = aMapLevel - 1;
		aPlayer->Unblock(0);
		if (aWasCurrent != 0 && aPlayer->mInUse != 0)
			aCurrentIndex = i;
		if (aPlayer->mInUse == 0)
			aPlayer->Reset(true);
	}
	FindFreeSlot();
	if (aCurrentIndex < 0) {
		for (int i = 0; i < PLAYER_PROFILES; i++) {
			if (sProfiles[i]->mInUse != 0) {
				aCurrentIndex = i;
				break;
			}
		}
	}
	SetCurrent(aCurrentIndex);
}

// 0x45e770
void Player::AddHighScoreOfCurrent()
{
	if (sCurrent != nullptr)
		HighScore::Add(sCurrent->mScore, sCurrent->mName);
}

// 0x45e790: sFreeSlot is the first profile not in use, or -1
void Player::FindFreeSlot()
{
	sFreeSlot = -1;
	int i = 0;
	do {
		if (sProfiles[i]->mInUse == 0) {
			sFreeSlot = i;
			return;
		}
		i = i + 1;
	} while (i < PLAYER_PROFILES);
}

// 0x45e7d0
void Player::ResetCurrent()
{
	if (sCurrent != nullptr) {
		sCurrent->Reset(true);
		sCurrent->mInUse = 1;
	}
}

// 0x45e7d0
void Sexy::ResetProfile()
{
	Player::ResetCurrent();
}

// 0x45e7f0: makes the eight profiles, named Player1 to Player8 until they are given names, and loads the file
void Player::MakeProfiles()
{
	sCurrentIndex = 0;
	for (int i = 0; i < PLAYER_PROFILES; i++) {
		Player* aPlayer = new Player();
		sProfiles[i] = aPlayer;
		aPlayer->Deactivate(aPlayer);
		aPlayer->mWholePixels = true;
		snprintf(aPlayer->mName, sizeof(aPlayer->mName), "Player%d", i + 1);
	}
	sCurrent = sProfiles[sCurrentIndex];
	LoadFile();
}

// 0x45e7f0
void Sexy::LoadProfiles()
{
	Player::MakeProfiles();
}

// 0x45e110
void Sexy::SaveProfiles()
{
	Player::SaveAll();
}

// 0x45e8c0
void Player::SetShotCooldown(int theFrames)
{
	if (sCurrent != nullptr)
		sCurrent->mShotCooldown = theFrames;
}
