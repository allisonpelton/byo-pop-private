#include "BonusCoin.h"

#include "Anim.h"
#include "BananaBonus.h"
#include "Bonus.h"
#include "BonusAttack.h"
#include "BonusBomb.h"
#include "BonusCannon.h"
#include "BonusDisco.h"
#include "BonusEarthquake.h"
#include "BonusEffect.h"
#include "BonusFlyPaper.h"
#include "BonusHurricane.h"
#include "BonusMagicArrows.h"
#include "BonusMine.h"
#include "BonusMissile.h"
#include "BugObj.h"
#include "Emitter.h"
#include "FillObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Pending.h"
#include "Player.h"
#include "Res.h"
#include "ScoreBoard.h"
#include "SoundQueue.h"

using namespace Sexy;

BonusCoin* BonusCoin::sCurrent = nullptr;
Anim* BonusCoin::sBonusesAnim = nullptr;
int BonusCoin::sFlight[COIN_BONUS_KINDS] = {0};

// The key events the coin's and its sparks' tracks end with (the base's 1 and 2 are not used here). Event 1 of the
// picture's appear track is raised and not listened for
enum CoinEvent
{
	COIN_EVENT_ARRIVED = 3,       // the coin's fly track is over: the bonus starts
	COIN_EVENT_FADED = 4,         // the picture's fade track is over: the coin goes inactive
	COIN_EVENT_RAY_PULSED = 5,    // a spark's pulse is half over: it takes its resting colour
	COIN_EVENT_RAY_GONE = 6       // a spark has closed in: it takes its resting colour
};

// The cell's size and where the coin sits in it (design pixels): the centre of the place, half a place in
const int COIN_CELL_SIZE = 31;
const double COIN_CELL_CENTRE = 15.5;
const int COIN_Z = 3;                // the depth it lies at, and the depth it flies at
const int COIN_FLY_Z = 0x18;
const int COIN_UNDER_CELL_SCORE_FRAMES = 700; // Place stores it; nothing reads it

// The sway of the picture: as Bonus's (0x406870), whose constants these are
const float SWAY_REST_DEGREES = 0.6f;
const double SWAY_STEP = (double)0.02f;     // the exe's 0.02 is the float's value widened (0.01999999955)
const float SWAY_NEAR_REACH = 124.0f;
const double SWAY_SPEED_DIVISOR = 7.0;

// How the sparks light up: every 12 frames the next one turns red, so the ring is lit in 864 frames
const int RAY_LIGHT_FRAMES = 12;
const float RAY_LIT_RED = 1.0f;
const float RAY_LIT_GREEN = 0.1f;
const float RAY_LIT_BLUE = 0.1f;

// The sparks' resting colour as they are made and when the coin is placed (a pale green), and as an event 5 or 6 puts
// it back. The original gives the green and blue the other way round in the two places, as these values are
const float RAY_REST_RED = 0.3f;
const float RAY_MADE_GREEN = 0.8f;       // made: red 0.3, green 0.8, blue 1 (and as events 5 and 6 reset it)
const float RAY_MADE_BLUE = 1.0f;
const float RAY_PLACED_GREEN = 1.0f;     // placed: red 0.3, green 1, blue 0.8 (the original's order)
const float RAY_PLACED_BLUE = 0.8f;

// Where the coin flies to when it is not flying to an object (design pixels, from the centre of the screen's design
// space), by the kind of flight, and how long it takes to arrive
const int COIN_FLY_FRAMES = 50;
const float COIN_FLY_UP_X = 0.0f;
const float COIN_FLY_OFF_Y = -600.0f;
const float COIN_FLY_DOWN_X = 256.0f;
const float COIN_FLY_DOWN_Y = 600.0f;
const float COIN_FLY_UP_RIGHT_X = 960.0f;
const float COIN_FLY_UP_LEFT_X = -832.0f;
const float COIN_NO_TARGET = 9999.0f;

// How near the coin must come to what it flies to (the object: 39 pixels; a place: 8) before it is taken as arrived
const float COIN_ARRIVE_NEAR_OBJECT = 39.0f;
const float COIN_ARRIVE_NEAR_PLACE = 8.0f;

// The pause the monkey is made to take after an uncover, and the sound of a ring completed
const int COIN_UNCOVER_SHOT_COOLDOWN = 60;

// The picture's tracks and the coin's own
const int COIN_TRACK_IDLE = 0;           // the coin's first track, a key that stays: Place starts it

// The coin as the player sees it: 0x498 bytes, so each field is where the original has it
// 0x408ed0
BonusCoin::BonusCoin()
{
	// (the original stores the name "BonusCoin" at +0x1f0, which this port's ObjBase has no room for)
	mTargetX = COIN_NO_TARGET;
	mTargetY = COIN_NO_TARGET;
	mButtonId = 0;
	sCurrent = nullptr;
	mChase = nullptr;
	mFlyToPlayer = false;
	mIdle = false;
	mPhase = COIN_UNDER_CELL;
	mUpdateLevel = 2;
	mColumn = 0;
	mRow = 0;
	mCell = nullptr;
	mAngle = 0.0f;
	mUnused2d0 = 0;
	mUnused2d4 = 0;
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, COIN_Z);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_OFFSET_X, 0);
	SetStateInt(STATE_OFFSET_Y, 0);
	mEffect = new BonusEffect();
	mEffect->SetParent(this, nullptr);
	mEmitters = new EmitterSet(this);
	mEmitters->MakeGreyRing(25.0f);
	mSwayDegrees = SWAY_REST_DEGREES;
	mRayIndex = 0;
	mTick = 0;
	BuildTracks();
	BuildSway();
	BuildRays();

	// every kind the player may have, in a random order (the C library's), is made, told its kind and made a child
	int anOrder[COIN_BONUS_KINDS];
	ShuffleNumbers(anOrder, COIN_BONUS_KINDS);
	mCount = 0;
	for (int i = 0; i < COIN_BONUS_KINDS; i++) {
		int aKind = anOrder[i];
		if (Player::sCurrent->mBonusBlocked[aKind] != 0)
			continue;
		Bonus* aBonus;
		switch (aKind) {
		case BONUS_SHOT_SPEED: aBonus = new BonusShotSpeed(); break;
		case BONUS_BOMB: aBonus = new BonusBomb(); break;
		case BONUS_FLY_PAPER: aBonus = new BonusFlyPaper(); break;
		case BONUS_ZAPPER: aBonus = new BonusZapper(); break;
		case BONUS_SWAT: aBonus = new BonusSwat(); break;
		case BONUS_SPEAR_CRAFT: aBonus = new BonusSpearCraft(); break;
		case BONUS_MAGIC_ARROWS: aBonus = new BonusMagicArrows(); break;
		case BONUS_MISSILE: aBonus = new BonusMissile(); break;
		case BONUS_CANNON: aBonus = new BonusCannon(); break;
		case BONUS_RAIN: aBonus = new BonusRain(); break;
		case BONUS_HURRICANE: aBonus = new BonusHurricane(); break;
		case BONUS_EARTHQUAKE: aBonus = new BonusEarthquake(); break;
		case BONUS_PARROT: aBonus = new BonusParrot(); break;
		case BONUS_TARZAN: aBonus = new BonusTarzan(); break;
		case BONUS_DISCO: aBonus = new BonusDisco(); break;
		case BONUS_PEA_SHOOTERS: aBonus = new BonusAttack(); break;
		case BONUS_ZUMA: aBonus = new BonusZuma(); break;
		case BONUS_MINE: aBonus = new BonusMine(); break;
		default:
			continue; // kind 13 has no bonus (the original's switch skips it too)
		}
		mBonuses[mCount] = aBonus;
		// (the original stores the name "BonusObj" at +0x1f0 of each)
		aBonus->Setup(aKind);
		aBonus->SetParent(this, nullptr);
		mKinds[mCount] = aKind;
		mCount++;
	}
	// port (crash): with no kind left the original divides by nothing here
	mCurrentIndex = mCount > 0 ? (int)(GameRandomNext() % (unsigned int)mCount) : 0;
	SetHidden(true);
	Deactivate(this);
}

// 0x409470 deletes; the body 0x409490: nothing points at it any more
BonusCoin::~BonusCoin()
{
	sCurrent = nullptr;
}

// 0x409dd0: the coin's own tracks. The first holds still. The second flies (the orbit round the angle to the target
// grows from 1.5 pixels a frame, the coin shown at 2) for as long as the key lasts (set at the uncover: 50 frames, or
// until it is off the screen or near enough), and then its event 3 says it has arrived; the third waits 400 frames
void BonusCoin::BuildTracks()
{
	AnimKey* anIdleKey = new AnimKey();
	anIdleKey->SetDuration(150);
	AnimTrack* anIdleTrack = new AnimTrack();
	anIdleTrack->AddKey(anIdleKey, anIdleKey);
	AddTrack(anIdleTrack, nullptr);

	mFlyKey = new AnimKey();
	mFlyKey->SetDuration(100);
	mFlyKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	mFlyKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	mFlyKey->AddValue(STATE_ORBIT_ANGLE, 0.0f, MOD_HOLD);
	mFlyKey->AddValue(STATE_ORBIT_RADIUS, 1.5f, MOD_TOWARD_NEXT);
	mFlyKey->AddValue(STATE_ALPHA, 2.0f, MOD_HOLD);
	mFlyKey->mEndCondition = KEY_END_OFF_SCREEN;
	mFlyKey->mEndChannel = STATE_X;
	mFlyKey->mEndValue = 0.0f;

	mArriveKey = new AnimKey();
	mArriveKey->mEventId = COIN_EVENT_ARRIVED;
	mArriveKey->SetDuration(1);
	mArriveKey->AddValue(STATE_ALPHA, 0.8f, MOD_HOLD);
	mArriveKey->AddValue(STATE_ORBIT_RADIUS, 38.0f, MOD_HOLD);

	AnimTrack* aFlyTrack = new AnimTrack();
	aFlyTrack->AddKey(mFlyKey, nullptr);
	aFlyTrack->AddKey(mArriveKey, nullptr);
	mFlyTrack = AddTrack(aFlyTrack, nullptr);

	AnimKey* aWaitKey = new AnimKey();
	aWaitKey->SetDuration(400);
	AnimKey* aWaitEndKey = new AnimKey();
	aWaitEndKey->SetDuration(1);
	AnimTrack* aWaitTrack = new AnimTrack();
	aWaitTrack->AddKey(aWaitKey, nullptr);
	aWaitTrack->AddKey(aWaitEndKey, nullptr);
	mWaitTrack = AddTrack(aWaitTrack, nullptr);
}

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// 0x40a080: the picture of the bonus, a child of the coin that tells it its keys' events. It has two tracks. The first
// grows in from nothing (a turn of -2 degrees), settles to its size, grows to 0.85 and stays so (its event 1 is not
// listened to). The second, which the uncover starts when no bonus is to start, shrinks and fades away, and its event 4
// puts the coin out
void BonusCoin::BuildSway()
{
	mSwayObj = new ObjBase();
	mSwayObj->SetStateInt(STATE_X, 0);
	mSwayObj->SetStateInt(STATE_Y, 0);
	mSwayObj->SetState(STATE_Z, 5.5f);
	mSwayObj->SetStateInt(STATE_ALPHA, 0);
	mSwayObj->SetStateInt(STATE_SCALE_X, 0);
	mSwayObj->SetStateInt(STATE_SCALE_Y, 0);
	mSwayObj->SetStateInt(STATE_OFFSET_X, 0);
	mSwayObj->SetStateInt(STATE_OFFSET_Y, 0);
	mSwayObj->SetParent(this, nullptr);
	mSwayObj->SetEventTarget(this);
	Activate(mSwayObj, false);

	AnimKey* aGrowKey = new AnimKey();
	GiveAnim(aGrowKey, sBonusesAnim);
	aGrowKey->SetDuration(10);
	aGrowKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGrowKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aGrowKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aGrowKey->AddValue(STATE_ROTATION, -0.0349065848f, MOD_TOWARD_NEXT);
	AnimKey* aSettleKey = new AnimKey();
	GiveAnim(aSettleKey, sBonusesAnim);
	aSettleKey->SetDuration(10);
	aSettleKey->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aSettleKey->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aSettleKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aSettleKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aSeenKey = new AnimKey();
	aSeenKey->mEventId = 1;
	GiveAnim(aSeenKey, sBonusesAnim);
	aSeenKey->SetDuration(20);
	aSeenKey->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aSeenKey->AddValue(STATE_SCALE_X, 0.85f, MOD_HOLD);
	aSeenKey->AddValue(STATE_SCALE_Y, 0.85f, MOD_HOLD);
	AnimKey* aStayKey = new AnimKey();
	GiveAnim(aStayKey, sBonusesAnim);
	aStayKey->SetDuration(20);
	aStayKey->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aStayKey->AddValue(STATE_SCALE_X, 0.85f, MOD_HOLD);
	aStayKey->AddValue(STATE_SCALE_Y, 0.85f, MOD_HOLD);
	AnimTrack* anAppearTrack = new AnimTrack();
	anAppearTrack->AddKey(aGrowKey, nullptr);
	anAppearTrack->AddKey(aSettleKey, nullptr);
	anAppearTrack->AddKey(aSeenKey, nullptr);
	anAppearTrack->AddKey(aStayKey, aStayKey);
	mAppearTrack = mSwayObj->AddTrack(anAppearTrack, nullptr);

	AnimKey* aShrinkKey = new AnimKey();
	aShrinkKey->SetDuration(12);
	aShrinkKey->AddValue(STATE_SCALE_X, 0.85f, MOD_TOWARD_NEXT);
	aShrinkKey->AddValue(STATE_SCALE_Y, 0.85f, MOD_TOWARD_NEXT);
	aShrinkKey->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aGoneKey = new AnimKey();
	aGoneKey->mEventId = COIN_EVENT_FADED;
	aGoneKey->SetDuration(1);
	aGoneKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_ALPHA, 0.5f, MOD_HOLD);
	AnimTrack* aFadeTrack = new AnimTrack();
	aFadeTrack->AddKey(aShrinkKey, nullptr);
	aFadeTrack->AddKey(aGoneKey, nullptr);
	mFadeTrack = mSwayObj->AddTrack(aFadeTrack, nullptr);
}

// 0x40a650: the ring of 72 sparks, five degrees apart, each a child of the coin that tells the coin its keys' events.
// A spark has three tracks (their numbers are kept in the fields of ObjBase that buttons use: +0xe8 mButtonId, +0xec
// mHoverState, +0xf0 mUnusedF0): the first (Place starts it) closes in from a ring of 50 to 57 pixels to one of 30 and
// stays there, dim; the second (when the ring has been lit, ResetRays starts it) goes out to the outer ring and back,
// and its event 5 puts its colour back; the third (ResetCurrent starts it) closes in and away, and its event 6 puts its
// colour back
void BonusCoin::BuildRays()
{
	for (int i = 0; i < COIN_RAYS; i++) {
		int aDegrees = i * 5;
		ObjBase* aRay = new ObjBase();
		mRays[i] = aRay;
		Emitter::sParticlesAnim->AddRef();
		aRay->mAnim = Emitter::sParticlesAnim;
		aRay->SetStateInt(STATE_FRAME, 0);
		aRay->mUpdateLevel = 2;
		aRay->SetParent(this, nullptr);
		float anAngle = (float)((double)aDegrees * PI_F / 180.0 - 1.3613568544387817);
		float aCos = FloatCos(anAngle);
		float anInnerX = (float)((double)aCos * 30.0);
		float aSin = FloatSin(anAngle);
		float anInnerY = (float)((double)aSin * 30.0);
		float aRadius = (float)((double)(i & 7) + 50.0);
		float anOuterX = (float)((double)aCos * (double)aRadius);
		float anOuterY = (float)((double)aSin * (double)aRadius);
		aRay->SetStateInt(STATE_X, 0);
		aRay->SetStateInt(STATE_Y, 0);
		aRay->SetState(STATE_Z, (float)(-0.009999999776482582 - (double)i / 300.0));
		aRay->SetStateInt(STATE_SCALE_X, 0);
		aRay->SetStateInt(STATE_SCALE_Y, 0);
		aRay->SetState(STATE_RED, RAY_REST_RED);
		aRay->SetStateInt(STATE_BLUE, (int)RAY_MADE_BLUE);
		aRay->SetState(STATE_GREEN, RAY_MADE_GREEN);
		aRay->SetStateInt(STATE_ALPHA, 0);
		aRay->SetAdditive();
		aRay->SetEventTarget(this);

		AnimKey* aCloseInKey = new AnimKey();
		aCloseInKey->SetDuration((i & 7) + 0x32);
		aCloseInKey->AddValue(STATE_X, anOuterX, MOD_TOWARD_NEXT);
		aCloseInKey->AddValue(STATE_Y, anOuterY, MOD_TOWARD_NEXT);
		aCloseInKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aCloseInKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aCloseInKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aRestKey = new AnimKey();
		aRestKey->SetDuration(0x32);
		aRestKey->AddValue(STATE_X, anInnerX, MOD_HOLD);
		aRestKey->AddValue(STATE_Y, anInnerY, MOD_HOLD);
		aRestKey->AddValue(STATE_ALPHA, 0.2f, MOD_HOLD);
		aRestKey->AddValue(STATE_SCALE_X, 0.35f, MOD_HOLD);
		aRestKey->AddValue(STATE_SCALE_Y, 0.35f, MOD_HOLD);
		AnimTrack* anAppearTrack = new AnimTrack();
		anAppearTrack->AddKey(aCloseInKey, nullptr);
		anAppearTrack->AddKey(aRestKey, aRestKey);
		aRay->mButtonId = aRay->AddTrack(anAppearTrack, nullptr);

		AnimKey* aGoOutKey = new AnimKey();
		aGoOutKey->SetDuration(0x14);
		aGoOutKey->AddValue(STATE_X, anInnerX, MOD_TOWARD_NEXT);
		aGoOutKey->AddValue(STATE_Y, anInnerY, MOD_TOWARD_NEXT);
		aGoOutKey->AddValue(STATE_ALPHA, 0.2f, MOD_TOWARD_NEXT);
		aGoOutKey->AddValue(STATE_SCALE_X, 0.35f, MOD_TOWARD_NEXT);
		aGoOutKey->AddValue(STATE_SCALE_Y, 0.35f, MOD_TOWARD_NEXT);
		AnimKey* aBackKey = new AnimKey();
		aBackKey->mEventId = COIN_EVENT_RAY_PULSED;
		aBackKey->SetDuration(10);
		aBackKey->AddValue(STATE_X, anOuterX, MOD_TOWARD_NEXT);
		aBackKey->AddValue(STATE_Y, anOuterY, MOD_TOWARD_NEXT);
		aBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aBackKey->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
		aBackKey->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
		AnimKey* aPulseRestKey = new AnimKey();
		aPulseRestKey->SetDuration(0x32);
		aPulseRestKey->AddValue(STATE_X, anInnerX, MOD_HOLD);
		aPulseRestKey->AddValue(STATE_Y, anInnerY, MOD_HOLD);
		aPulseRestKey->AddValue(STATE_ALPHA, 0.2f, MOD_HOLD);
		aPulseRestKey->AddValue(STATE_SCALE_X, 0.35f, MOD_HOLD);
		aPulseRestKey->AddValue(STATE_SCALE_Y, 0.35f, MOD_HOLD);
		AnimTrack* aPulseTrack = new AnimTrack();
		aPulseTrack->AddKey(aGoOutKey, nullptr);
		aPulseTrack->AddKey(aBackKey, nullptr);
		aPulseTrack->AddKey(aPulseRestKey, aPulseRestKey);
		aRay->mHoverState = aRay->AddTrack(aPulseTrack, nullptr);

		AnimKey* aCloseKey = new AnimKey();
		aCloseKey->SetDuration(8);
		aCloseKey->AddValue(STATE_X, anInnerX, MOD_TOWARD_NEXT);
		aCloseKey->AddValue(STATE_Y, anInnerY, MOD_TOWARD_NEXT);
		aCloseKey->AddValue(STATE_SCALE_X, 0.35f, MOD_TOWARD_NEXT);
		aCloseKey->AddValue(STATE_SCALE_Y, 0.35f, MOD_TOWARD_NEXT);
		AnimKey* aVanishKey = new AnimKey();
		aVanishKey->mEventId = COIN_EVENT_RAY_GONE;
		aVanishKey->SetDuration(8);
		aVanishKey->AddValue(STATE_X, anOuterX, MOD_HOLD);
		aVanishKey->AddValue(STATE_Y, anOuterY, MOD_HOLD);
		aVanishKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
		aVanishKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
		AnimTrack* aGoTrack = new AnimTrack();
		aGoTrack->AddKey(aCloseKey, nullptr);
		aGoTrack->AddKey(aVanishKey, nullptr);
		aRay->mUnusedF0 = aRay->AddTrack(aGoTrack, nullptr);

		Deactivate(aRay);
	}
}

// 0x4094b0: the events of the coin's own keys and of its sparks' and its picture's
void BonusCoin::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case COIN_EVENT_ARRIVED:
		// it has flown to its target: the bonus starts, with the effect of its arrival where the coin ended (at the
		// monkey when it flew to it); or, when the level is as good as over, bananas are paid out in its place
		if (Player::sCurrent->mPercent < Player::GetTargetPercent() && BugObj::sAlive > 0) {
			BonusEffect* anEffect = mFlyToPlayer ? Player::sCurrent->mBonusEffect : mEffect;
			anEffect->Play();
			mSwayObj->SetHidden(true);
			mBonuses[mCurrentIndex]->Start();
			StartTrack(mWaitTrack);
		} else {
			static_cast<LevelObj*>(sLevel)->mBananaBonus->Spawn(mScreenX, mScreenY);
			mSwayObj->StartTrack(mFadeTrack);
			mFlyToPlayer = false;
			mTick = 0;
			mPhase = COIN_UNCOVERED;
		}
		return;
	case COIN_EVENT_FADED:
		Deactivate(this);
		return;
	case COIN_EVENT_RAY_PULSED:
	case COIN_EVENT_RAY_GONE:
		theObj->SetState(STATE_RED, RAY_REST_RED);
		theObj->SetStateInt(STATE_BLUE, (int)RAY_MADE_BLUE);
		theObj->SetState(STATE_GREEN, RAY_MADE_GREEN);
		return;
	default:
		return;
	}
}

// 0x4095f0: while the coin lies under its cell with two bonuses or more in it, a spark is lit every 12 frames, and
// when the ring is lit, it pulses and the next bonus is shown. Once the cell is cut the coin turns toward its target
// (the angle goes to its orbit) and is ended as it comes near. The picture sways, as Bonus's does
bool BonusCoin::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mIdle)
		return true;
	if (mPhase == COIN_UNDER_CELL && mCount > 1 && !mHidden) {
		mTick++;
		if (mTick == RAY_LIGHT_FRAMES) {
			mTick = 0;
			ObjBase* aRay = mRays[mRayIndex];
			aRay->SetStateInt(STATE_RED, (int)RAY_LIT_RED);
			aRay->SetState(STATE_BLUE, RAY_LIT_BLUE);
			aRay->SetState(STATE_GREEN, RAY_LIT_GREEN);
			mRayIndex++;
			if (mRayIndex >= COIN_RAYS) {
				mRayIndex = 0;
				ResetRays();
				sApp->mSoundQueue->Request(SOUND_BONUSCOIN, 0, false);
				mCurrentIndex = (mCurrentIndex + 1) % mCount;
				mSwayObj->SetStateInt(STATE_FRAME, mKinds[mCurrentIndex]);
			}
		}
	}
	if (mChase != nullptr) {
		float aDeltaX = (float)((double)mChase->mDrawn[STATE_X] - (double)mDrawn[STATE_X]);
		float aDeltaY = (float)((double)mDrawn[STATE_Y] - (double)mChase->mDrawn[STATE_Y]);
		mAngle = FloatAtan2(aDeltaY, aDeltaX);
		SetState(STATE_ORBIT_ANGLE, mAngle);
		float aNearY = (float)((double)mScreenY - (double)mChase->mScreenY);
		float aNearX = (float)((double)mScreenX - (double)mChase->mScreenX);
		float aDistance = FloatSqrt((float)((double)aNearX * (double)aNearX + (double)aNearY * (double)aNearY));
		if (aDistance < COIN_ARRIVE_NEAR_OBJECT) {
			mFlyKey->mEndCondition = KEY_END_AT_ONCE;
			mFlyKey->mEndChannel = STATE_X;
			mFlyKey->mEndValue = 0.0f;
			ComputeDrawn();
		}
		mSwayDegrees = 0.0f;
		mSwayObj->SetState(STATE_ROTATION, 0.0f);
	} else if (mTargetX < COIN_NO_TARGET) {
		float aDeltaX = (float)((double)mTargetX - (double)mScreenX);
		float aDeltaY = (float)((double)mScreenY - (double)mTargetY);
		mAngle = FloatAtan2(aDeltaY, aDeltaX);
		SetState(STATE_ORBIT_ANGLE, mAngle);
		float aNearY = (float)((double)mScreenY - (double)mTargetY);
		float aNearX = (float)((double)mScreenX - (double)mTargetX);
		float aDistance = FloatSqrt((float)((double)aNearX * (double)aNearX + (double)aNearY * (double)aNearY));
		if (aDistance < COIN_ARRIVE_NEAR_PLACE) {
			mFlyKey->mEndCondition = KEY_END_AT_ONCE;
			mFlyKey->mEndChannel = STATE_X;
			mFlyKey->mEndValue = 0.0f;
		}
		mSwayDegrees = 0.0f;
		mSwayObj->SetState(STATE_ROTATION, 0.0f);
	}
	// the sway, which settles on 0.6 and is shaken when the monkey is near (Bonus::Update's)
	float aSway = SWAY_REST_DEGREES;
	if (mSwayDegrees <= SWAY_REST_DEGREES) {
		if (mSwayDegrees < SWAY_REST_DEGREES)
			aSway = (float)((double)mSwayDegrees + SWAY_STEP);
	} else {
		aSway = (float)((double)mSwayDegrees - SWAY_STEP);
	}
	mSwayDegrees = aSway;
	if (Player::IsNear(this, SWAY_NEAR_REACH))
		mSwayDegrees = (float)(Player::GetSpeedAverage() / SWAY_SPEED_DIVISOR + (double)mSwayDegrees);
	mSwayObj->SetState(STATE_ROTATION, (float)((double)mSwayDegrees * PI_F / 180.0 + (double)mSwayObj->mState[STATE_ROTATION]));
	return true;
}

// 0x409970 (EAX theRow, theColumn pushed): the coin is put under a cell, showing a bonus of the kind the player has just
// been allowed to have if there is one (Player::sLastUnblocked), otherwise one chosen at random. With two bonuses or
// more the ring of sparks closes in and is shown; with one there is no ring
void BonusCoin::Place(int theColumn, int theRow)
{
	mIdle = false;
	mTargetX = COIN_NO_TARGET;
	mChase = nullptr;
	mTargetY = COIN_NO_TARGET;
	mButtonId = 0;
	sCurrent = this;
	SetState(STATE_X, (float)((double)(theColumn * COIN_CELL_SIZE) + COIN_CELL_CENTRE));
	SetState(STATE_Y, (float)((double)(theRow * COIN_CELL_SIZE) + COIN_CELL_CENTRE));
	SetStateInt(STATE_Z, COIN_Z);
	StartTrack(COIN_TRACK_IDLE);
	ComputeDrawn();
	mColumn = theColumn;
	mRow = theRow;
	mPhase = COIN_UNDER_CELL;
	mUnused2d4 = COIN_UNDER_CELL_SCORE_FRAMES;
	// port (crash): with no kind left the original divides by nothing here
	mCurrentIndex = mCount > 0 ? (int)(GameRandomNext() % (unsigned int)mCount) : 0;
	if (Player::sLastUnblocked >= 0) {
		for (int i = 0; i < mCount; i++) {
			if (mKinds[i] == Player::sLastUnblocked) {
				Player::sLastUnblocked = -1;
				mCurrentIndex = i;
				break;
			}
		}
	}
	mSwayObj->SetStateInt(STATE_FRAME, mKinds[mCurrentIndex]);
	mSwayObj->StartTrack(mAppearTrack);
	if (mCount > 1) {
		for (int i = 0; i < COIN_RAYS; i++) {
			ObjBase* aRay = mRays[i];
			aRay->StartTrack(aRay->mButtonId);
			aRay->SetState(STATE_RED, RAY_REST_RED);
			aRay->SetStateInt(STATE_GREEN, (int)RAY_PLACED_GREEN);
			aRay->SetState(STATE_BLUE, RAY_PLACED_BLUE);
			Activate(aRay, false);
		}
	} else {
		for (int i = 0; i < COIN_RAYS; i++)
			Deactivate(mRays[i]);
	}
	mSwayObj->SetHidden(false);
	Activate(mSwayObj, false);
	Activate(this, false);
}

// 0x409b90 (a guessed name): with two bonuses or more, every spark pulses (its second track) and a ring of grey bursts
void BonusCoin::ResetRays()
{
	if (mCount < 2)
		return;
	for (int i = 0; i < COIN_RAYS; i++)
		mRays[i]->StartTrack(mRays[i]->mHoverState);
	mEmitters->BurstGreyRing(0.0f, 0.0f);
}

// 0x409bf0: the cell is cut. When the level still has to be played (the percent is short and bugs are alive) the coin
// flies to where its kind sends it, 50 frames at most, ending when it comes near; its depth is raised above the ground's.
// Otherwise a bunch of bananas is paid out at once and the picture shrinks. The sparks go either way, and the monkey is
// kept from shooting for a second
void BonusCoin::Uncover()
{
	mFlyToPlayer = false;
	mPhase = COIN_UNCOVERED;
	mTick = 0;
	Player* aPlayer = Player::sCurrent;
	if (aPlayer->mPercent < Player::GetTargetPercent() && BugObj::sAlive > 0) {
		if (mBonuses[mCurrentIndex] != nullptr) {
			mFlyKey->mEndCondition = KEY_END_OFF_SCREEN;
			mFlyKey->mEndChannel = STATE_X;
			mFlyKey->mEndValue = 0.0f;
			switch (sFlight[mKinds[mCurrentIndex]]) {
			case COIN_FLIES_TO_SCORE_BOARD:
				mFlyKey->SetDuration(COIN_FLY_FRAMES);
				mChase = aPlayer->mScoreBoard;
				break;
			case COIN_FLIES_UP:
				mFlyKey->SetDuration(COIN_FLY_FRAMES);
				mTargetX = COIN_FLY_UP_X;
				mTargetY = COIN_FLY_OFF_Y;
				break;
			case COIN_FLIES_DOWN:
				mFlyKey->SetDuration(COIN_FLY_FRAMES);
				mTargetX = COIN_FLY_DOWN_X;
				mTargetY = COIN_FLY_DOWN_Y;
				break;
			case COIN_FLIES_UP_RIGHT:
				mFlyKey->SetDuration(COIN_FLY_FRAMES);
				mTargetX = COIN_FLY_UP_RIGHT_X;
				mTargetY = COIN_FLY_OFF_Y;
				break;
			case COIN_FLIES_UP_LEFT:
				mFlyKey->SetDuration(COIN_FLY_FRAMES);
				mTargetX = COIN_FLY_UP_LEFT_X;
				mTargetY = COIN_FLY_OFF_Y;
				break;
			default:
				mFlyKey->SetDuration(COIN_FLY_FRAMES);
				mChase = aPlayer;
				mFlyToPlayer = true;
				break;
			}
		}
		SetStateInt(STATE_Z, COIN_FLY_Z);
		StartTrack(mFlyTrack);
	} else {
		static_cast<LevelObj*>(sLevel)->mBananaBonus->Spawn(mScreenX, mScreenY);
		mSwayObj->StartTrack(mFadeTrack);
	}
	for (int i = 0; i < COIN_RAYS; i++)
		Deactivate(mRays[i]);
	Player::SetShotCooldown(COIN_UNCOVER_SHOT_COOLDOWN);
}

// 0x40af00 (a guessed name): the coin that has been placed is put away: it flies to nothing, its sparks close in and
// vanish (their third track), and the cell that covered it forgets it. The picture fades (its second track)
void BonusCoin::ResetCurrent()
{
	BonusCoin* aCoin = sCurrent;
	if (aCoin == nullptr)
		return;
	aCoin->mChase = nullptr;
	aCoin->mIdle = true;
	for (int i = 0; i < COIN_RAYS; i++)
		aCoin->mRays[i]->StartTrack(aCoin->mRays[i]->mUnusedF0);
	aCoin->mSwayDegrees = SWAY_REST_DEGREES;
	aCoin->mRayIndex = 0;
	aCoin->mTick = 0;
	if (aCoin->mCell != nullptr)
		aCoin->mCell->mBonus = nullptr;
	aCoin->mSwayObj->StartTrack(aCoin->mFadeTrack);
}

// 0x40af90: "images/bonuses" (cels of 72 by 72, 20 of them, the kinds' numbers) and the table of how a bonus's coin
// flies off. By default it flies to the monkey (0). The pea shooters (16) fly to the score board's monkeys (1); the rain (9)
// and the hurricane (10) rise off the top (2); the earthquake (11) falls toward the bottom (3); Tarzan (14) goes off
// the top to the right (4); and the disco (15) off the top to the left (5). The original's table has an entry for each
// of the 19 kinds (13 never set). Which bonuses a level may hold is not here: it is the player's blocked list (Player)
void Sexy::InitBonusTable()
{
	BonusCoin::sBonusesAnim = Anim::Get("images/bonuses", ANIM_SCALE_SMOOTH, false, 0x48, 0x48, 0x14, true);
	for (int aKind = 0; aKind < COIN_BONUS_KINDS; aKind++) {
		switch (aKind) {
		case BONUS_RAIN:
			BonusCoin::sFlight[aKind] = COIN_FLIES_UP;
			break;
		case BONUS_HURRICANE:
			BonusCoin::sFlight[aKind] = COIN_FLIES_UP;
			break;
		case BONUS_EARTHQUAKE:
			BonusCoin::sFlight[aKind] = COIN_FLIES_DOWN;
			break;
		case BONUS_TARZAN:
			BonusCoin::sFlight[aKind] = COIN_FLIES_UP_RIGHT;
			break;
		case BONUS_DISCO:
			BonusCoin::sFlight[aKind] = COIN_FLIES_UP_LEFT;
			break;
		case BONUS_PEA_SHOOTERS:
			BonusCoin::sFlight[aKind] = COIN_FLIES_TO_SCORE_BOARD;
			break;
		case 13:
			break; // no bonus
		default:
			BonusCoin::sFlight[aKind] = COIN_FLIES_TO_MONKEY;
			break;
		}
	}
}
