// ObjBase: the scene graph every screen and sprite is built from (0x4509f0..0x453e30 in the original). The x87 maths
// keeps its intermediates in double, as the original's 53-bit precision does, and rounds to float where it stores.
#include "ObjBase.h"

#include "Anim.h"
#include "Color.h"
#include "GameApp.h"
#include "Graphics.h"
#include "MsvcSort.h"
#include "SoundQueue.h"

#include <cmath>

using namespace Sexy;

int ObjBase::sObjectCount = 0;
GameApp* ObjBase::sApp = nullptr;
ObjBase* ObjBase::sLevel = nullptr;
ObjBase* ObjBase::sRoot = nullptr;
int ObjBase::sFrame = 0;
int ObjBase::sDrawnCount = 0;
ObjBase* ObjBase::sDrawn[OBJ_MAX_DRAWN];
ObjBase* ObjBase::sDeleted[OBJ_MAX_DELETED];
int ObjBase::sDeletedCount = 0;
float ObjBase::sMouseX = 0.0f;
float ObjBase::sMouseY = 0.0f;
float ObjBase::sMouseDesignX = 0.0f;
float ObjBase::sMouseDesignY = 0.0f;
float ObjBase::sMouseAverageX = 0.0f;
float ObjBase::sMouseAverageY = 0.0f;
ObjBase* ObjBase::sHover = nullptr;
ObjBase* ObjBase::sPressed = nullptr;
int ObjBase::sKeyPressed = 0;
int ObjBase::sKeyHeld = 0;
float ObjBase::sHoverMinZ = 0.0f;
int ObjBase::sUpdateLevel = 0;
GameApp* ObjBase::sBounds = nullptr;
bool ObjBase::sMouseHeld = false;
bool ObjBase::sMouseReleased = false;

// The game's generator's seed; the exe starts it at this value (0x5fffe8)
static unsigned int gGameRandom = 0x058d2771;

// 0x4733b0
unsigned int Sexy::GameRandomNext()
{
	gGameRandom = (gGameRandom & 1) * 0x5eb0a82fu - (gGameRandom >> 1) * 0x429eaf9du;
	return gGameRandom;
}

// 0x4733e0
void Sexy::GameRandomSeed(unsigned int theSeed)
{
	gGameRandom = theSeed;
}

// port: the generator's state, for the test logs
unsigned int Sexy::GameRandomState()
{
	return gGameRandom;
}

// 0x404890
int Sexy::GameRandomRange(int theMax, int theMin)
{
	unsigned int aDraw = GameRandomNext();
	return (int)(aDraw % (unsigned int)(theMax - theMin + 1)) + theMin;
}

// 0x404850
float Sexy::FloatCos(float theAngle)
{
	return (float)cos((double)theAngle);
}

// 0x404870
float Sexy::FloatSin(float theAngle)
{
	return (float)sin((double)theAngle);
}

// 0x405f30
float Sexy::FloatAbs(float theValue)
{
	return (float)fabs((double)theValue);
}

// 0x405f70
float Sexy::FloatSqrt(float theValue)
{
	return (float)sqrt((double)theValue);
}

// 0x405f90 (through 0x405f50)
float Sexy::FloatAtan2(float theY, float theX)
{
	return (float)atan2((double)theY, (double)theX);
}

// 0x452900
float Sexy::FloatSign(float theValue)
{
	if (theValue == 0.0f)
		return 0.0f;
	if (theValue < 0.0f)
		return -1.0f;
	return 1.0f;
}

// ---------------------------------------------------------------------------------------------------------------
// KeyModifier

// 0x452940
KeyModifier::KeyModifier()
{
	mName = "this will be name for xml";
	mRate = 0.0f;
	mDirection = 1;
	mBase = 0.0f;
	mAccum = 0.0f;
	mAmount = 0.0f;
	mNext = nullptr;
	mKind = 0;
	mPulseStep = 0.0f;
	mStep = 0.0f;
	mAngle = 0.0f;
	mTable = nullptr;
	mLastAngle = 0.0f;
	mInitialAmount = 0.0f;
	mInitialRate = 0.0f;
	mInitialBase = 0.0f;
}

// 0x4529a0
void KeyModifier::Set(int theKind, const float* theTable, float theRate, float theAmount, float theBase)
{
	mKind = theKind;
	mInitialRate = theRate;
	mTable = theTable;
	mRate = theRate;
	mInitialAmount = theAmount;
	mAmount = theAmount;
	mInitialBase = theBase;
	mBase = theBase;
	mPulseStep = -1.0f;
	mStep = (float)(theRate / 100.0);
	mAngle = 0.0f;
}

// 0x452960
void KeyModifier::Reset()
{
	mDirection = 1;
	mRate = mInitialRate;
	mAmount = mInitialAmount;
	mBase = mInitialBase;
	mPulseStep = -1.0f;
	mStep = (float)(mInitialRate / 100.0);
}

// 0x4529e0: one modifier's change to one value of the object, by the key's progress
static void ApplyModifier(KeyModifier* theMod, ObjBase* theObj, int theChannel, float theProgress)
{
	AnimKey* aKey = theObj->mCurrentKey;
	float* aState = theObj->mState;
	int aFrame = ObjBase::sFrame;
	switch (theMod->mKind) {
	case MOD_TOWARD_NEXT:
		if (aKey->mNext != nullptr)
			aState[theChannel] = (float)(((1.0 - theProgress) * aKey->mValue[theChannel] + theObj->mBase[theChannel]) +
				(double)theProgress * aKey->mNext->mValue[theChannel]);
		break;
	case MOD_SINE: {
		float anArg = (float)((double)theProgress * PI_F * theMod->mRate + theMod->mBase);
		float aSin = FloatSin(anArg);
		aState[theChannel] = (float)((double)aSin * aKey->mValue[theChannel] * theMod->mAmount + aState[theChannel]);
		break;
	}
	case MOD_ORBIT_SIN: {
		double aValue = aState[theChannel];
		float aSin = FloatSin(aState[STATE_ORBIT_ANGLE]);
		aState[theChannel] = (float)(aValue - (double)aSin * aState[STATE_ORBIT_RADIUS]);
		break;
	}
	case MOD_ORBIT_COS: {
		float aCos = FloatCos(aState[STATE_ORBIT_ANGLE]);
		aState[theChannel] = (float)((double)aCos * aState[STATE_ORBIT_RADIUS] + aState[theChannel]);
		break;
	}
	case MOD_ADD_VALUE:
		aState[theChannel] = aKey->mValue[theChannel] + aState[theChannel];
		break;
	case MOD_FACE_MOTION: {
		float anAngle = FloatAtan2(theObj->mPrevY - aState[STATE_Y], aState[STATE_X] - theObj->mPrevX);
		theMod->mLastAngle = anAngle;
		float aSum = anAngle + theMod->mAccum;
		float aSmoothed = (float)(aSum * 0.5);
		theMod->mAccum = aSmoothed;
		aState[STATE_ROTATION] = aSmoothed;
		break;
	}
	case MOD_TURN_TOWARD: {
		if (aKey->mNext == nullptr)
			break;
		AnimKey* aNext = aKey->mNext;
		float anAngle = FloatAtan2(aState[STATE_Y] - aNext->mValue[STATE_Y], aState[STATE_X] - aNext->mValue[STATE_X]);
		double aTarget = anAngle;
		float aDiff = (float)(aTarget - aState[theChannel]);
		float anOther = (float)(aTarget - FloatSign(aDiff) * TWO_PI_F);
		float aDiffAbs = FloatAbs(aDiff);
		float aTurnTo = anAngle;
		if (!(aDiffAbs <= FloatAbs((float)(anOther - aState[theChannel]))))
			aTurnTo = anOther;
		if (FloatAbs((float)(aState[theChannel] - aTurnTo)) > 0.001) {
			float aRemaining = (float)(aTurnTo - aState[theChannel]);
			double aValue = aState[theChannel];
			double aTurn = PI_F - FloatAbs(aRemaining);
			aState[theChannel] = (float)(aValue - (FloatSign(aRemaining) * aTurn) /
				(aState[STATE_ORBIT_RADIUS] * 2.5999999046325684 + 1.0));
		} else if (TWO_PI_F < aState[theChannel]) {
			aState[theChannel] = (float)(aState[theChannel] - TWO_PI_F);
		} else if (aState[theChannel] < 0.0f) {
			aState[theChannel] = (float)(TWO_PI_F + aState[theChannel]);
		}
		break;
	}
	case MOD_COUNT_UP: {
		float aValue = theMod->mStep + aState[theChannel];
		aState[theChannel] = aValue;
		if (theMod->mAmount <= aValue) {
			aValue = aValue - theMod->mAmount;
			aState[theChannel] = aValue;
			if ((int)aValue == 0)
				aState[theChannel] = 0.0f;
		}
		break;
	}
	case MOD_COUNT_DOWN: {
		float aValue = aState[theChannel] - theMod->mStep;
		aState[theChannel] = aValue;
		if (aValue < 0.0f)
			aState[theChannel] = aValue + theMod->mAmount;
		break;
	}
	case MOD_COUNT_BY_PULSE: {
		float aPulse = theObj->mParent->mKeyPulse;
		float aStep = (float)((1.0 - aPulse) * theMod->mBase + (double)theMod->mRate * aPulse);
		theMod->mPulseStep = aStep;
		float aValue = (float)(aStep / 100.0 + aState[theChannel]);
		aState[theChannel] = aValue;
		if (theMod->mAmount <= aValue)
			aState[theChannel] = aValue - theMod->mAmount;
		break;
	}
	case MOD_PING_PONG:
		if (theMod->mDirection > 0) {
			float aValue = theMod->mStep + theMod->mAccum;
			theMod->mAccum = aValue;
			aState[theChannel] = aValue;
			if (theMod->mAmount <= aValue) {
				theMod->mDirection = -1;
				aState[theChannel] = (float)(theMod->mAmount - 1.0);
			}
		} else {
			float aValue = theMod->mAccum - theMod->mStep;
			theMod->mAccum = aValue;
			aState[theChannel] = aValue;
			if (theMod->mBase > aValue) {
				theMod->mDirection = 1;
				aState[theChannel] = theMod->mBase;
			}
		}
		break;
	case MOD_RANDOM: {
		int anEvery = (int)theMod->mRate;
		if (aFrame % anEvery == 0) {
			int aMax = (int)theMod->mBase;
			int aMin = (int)theMod->mAmount;
			aState[theChannel] = (float)GameRandomRange(aMax, aMin);
		}
		break;
	}
	case MOD_RANDOM_HUNDREDTHS: {
		int anEvery = (int)theMod->mRate;
		if (aFrame % anEvery == 0) {
			int aMax = (int)theMod->mBase;
			int aMin = (int)theMod->mAmount;
			aState[theChannel] = (float)((double)GameRandomRange(aMax, aMin) / 100.0);
		}
		break;
	}
	case MOD_WRAP_DEGREES: {
		int anEvery = (int)theMod->mRate;
		if (aFrame % anEvery == 0) {
			float aDegrees = (float)((double)aState[theChannel] * 180.0 / PI_F);
			float anAngle = aDegrees - theMod->mAmount;
			theMod->mAngle = anAngle;
			if (anAngle < 0.0f)
				theMod->mAngle = (float)(anAngle + 360.0);
			aState[theChannel] = (float)((double)theMod->mAngle * PI_F / 180.0);
		}
		break;
	}
	case MOD_SPIN: {
		int anEvery = (int)theMod->mRate;
		if (aFrame % anEvery == 0) {
			float anAngle = theMod->mAmount + theMod->mAngle;
			theMod->mAngle = anAngle;
			if (360.0f <= anAngle)
				theMod->mAngle = (float)(anAngle - 360.0);
			aState[theChannel] = (float)((double)theMod->mAngle * PI_F / 180.0);
		}
		break;
	}
	case MOD_SPIN_BY_SPEED: {
		float aDegrees = (float)((double)aState[theChannel] * 180.0 / PI_F);
		theMod->mAngle = aDegrees;
		float aSpeed = theObj->mVelX <= theObj->mVelY ? theObj->mVelY : theObj->mVelX;
		theMod->mAngle = (float)((double)aDegrees - (double)theMod->mAmount * aSpeed);
		if (theMod->mAngle < 0.0f)
			theMod->mAngle = (float)(theMod->mAngle + 360.0);
		aState[theChannel] = (float)((double)theMod->mAngle * PI_F / 180.0);
		break;
	}
	case MOD_SPIN_BY_START_SPEED: {
		float aDegrees = (float)((double)aState[theChannel] * 180.0 / PI_F);
		theMod->mAngle = aDegrees;
		float anAngle = (float)((double)aDegrees - (double)theObj->mKeyStartSpeed * theMod->mAmount);
		theMod->mAngle = anAngle;
		if (anAngle < 0.0f)
			theMod->mAngle = (float)(anAngle + 360.0);
		aState[theChannel] = (float)((double)theMod->mAngle * PI_F / 180.0);
		theMod->mAmount = (float)(theMod->mAmount * 0.99);
		break;
	}
	case MOD_FLICKER: {
		float aShade = GameRandomRange((int)theMod->mRate, 0) == 0 ? theMod->mAmount : theMod->mBase;
		aState[STATE_RED] = aShade;
		aState[STATE_GREEN] = aShade;
		aState[STATE_BLUE] = aShade;
		break;
	}
	case MOD_COPY_ORBIT_ANGLE:
		aState[theChannel] = aState[STATE_ORBIT_ANGLE];
		break;
	case MOD_DECAY:
		aState[theChannel] = aState[theChannel] - theMod->mBase;
		theMod->mBase = (theMod->mAmount + theMod->mBase) * theMod->mRate;
		break;
	case MOD_PATH:
		if (aKey->mNext != nullptr) {
			float aPosition = (float)((1.0 - theProgress) * aKey->mValue[STATE_PATH] +
				(double)aKey->mNext->mValue[STATE_PATH] * theProgress);
			aState[STATE_PATH] = aPosition;
			int anIndex = (int)((double)aPosition + 0.5);
			aState[theChannel] = theMod->mTable[anIndex] + aKey->mValue[theChannel];
		}
		break;
	default:
		aState[theChannel] = aKey->mValue[theChannel] + theObj->mBase[theChannel];
		break;
	}
}

// ---------------------------------------------------------------------------------------------------------------
// AnimKey

// 0x453290
AnimKey::AnimKey()
{
	mDuration = 1.0f;
	mUnusedOne = 1.0f;
	mEndCondition = KEY_END_TIME;
	mEventId = 0;
	mRefCount = 0;
	mEasing = EASE_LINEAR_CLAMPED;
	mDrawMode = 0;
	mAnim = nullptr;
	mNext = nullptr;
	mRemoveAtEnd = false;
	mEndChannel = 0;
	mEndValue = 0.0f;
	for (int i = 0; i < NUM_STATES; i++) {
		mValue[i] = 0.0f;
		mModifiers[i] = nullptr;
		mKeepCurrent[i] = false;
		mNextValue[i] = 0.0f;
	}
	mValue[STATE_SCALE_X] = 1.0f;
	mValue[STATE_SCALE_Y] = 1.0f;
	mValue[STATE_RED] = 1.0f;
	mValue[STATE_GREEN] = 1.0f;
	mValue[STATE_BLUE] = 1.0f;
	mValue[STATE_ALPHA] = 1.0f;
}

// 0x453300
AnimKey::~AnimKey()
{
	if (mAnim != nullptr)
		mAnim->Release();
	for (int i = 0; i < NUM_STATES; i++) {
		KeyModifier* aMod = mModifiers[i];
		while (aMod != nullptr) {
			KeyModifier* aNext = aMod->mNext;
			delete aMod;
			aMod = aNext;
		}
	}
}

// 0x4533a0
void AnimKey::AddRef()
{
	mRefCount++;
}

// 0x453380
void AnimKey::Release()
{
	mRefCount--;
	if (mRefCount < 1)
		delete this;
}

// 0x453350
void AnimKey::ResetModifiers()
{
	for (int i = 0; i < NUM_STATES; i++)
		for (KeyModifier* aMod = mModifiers[i]; aMod != nullptr; aMod = aMod->mNext)
			aMod->Reset();
}

// 0x4535d0
void AnimKey::SetNextValues(AnimKey* theNext)
{
	const float* aFrom = theNext != nullptr ? theNext->mValue : mValue;
	for (int i = 0; i < NUM_STATES; i++)
		mNextValue[i] = aFrom[i];
}

// 0x453590
void AnimKey::SetDuration(int theFrames)
{
	if (theFrames > -1)
		mDuration = (float)theFrames;
	else
		mDuration = 0.0f;
}

// Appends a modifier to a value's chain. The original walks a chain of two or more with the new modifier's own
// uninitialised link (0x453422), so it could only ever append a second; the port appends at the end.
static void AppendModifier(KeyModifier** theChain, KeyModifier* theMod)
{
	theMod->mNext = nullptr;
	KeyModifier** aLink = theChain;
	while (*aLink != nullptr)
		aLink = &(*aLink)->mNext;
	*aLink = theMod;
}

// 0x4533c0: the value a channel reaches and how; the first modifier of a channel sets its value and keep flag
void AnimKey::AddModifier(int theChannel, bool keepCurrent, float theValue, int theKind, float theRate, float theAmount,
	float theBase, const float* theTable)
{
	if (mModifiers[theChannel] == nullptr) {
		mValue[theChannel] = theValue;
		mKeepCurrent[theChannel] = keepCurrent;
	}
	KeyModifier* aMod = new KeyModifier();
	aMod->Set(theKind, theTable, theRate, theAmount, theBase);
	AppendModifier(&mModifiers[theChannel], aMod);
}

// 0x453460: a MOD_DECAY modifier starting at theStart * theScale
void AnimKey::AddDecay(int theChannel, float theValue, float theStart, float theScale, float theRate, float theAmount)
{
	if (mModifiers[theChannel] == nullptr)
		mValue[theChannel] = theValue;
	float aStart = theStart * theScale;
	KeyModifier* aMod = new KeyModifier();
	aMod->Set(MOD_DECAY, nullptr, theAmount, theRate, aStart);
	AppendModifier(&mModifiers[theChannel], aMod);
}

// 0x453500: restarts a channel's MOD_DECAY at theStart * theScale
void AnimKey::SetDecay(int theChannel, float theStart, float theScale)
{
	for (KeyModifier* aMod = mModifiers[theChannel]; aMod != nullptr; aMod = aMod->mNext) {
		if (aMod->mKind == MOD_DECAY) {
			aMod->mBase = theStart * theScale;
			aMod->mInitialBase = theStart * theScale;
			return;
		}
	}
}

// 0x4535f0: the key's progress, 0 to 1 (1 when an end condition holds); sets the object's key pulse
float AnimKey::Progress(ObjBase* theObj)
{
	float aProgress = 0.0f;
	switch (mEasing) {
	case EASE_LINEAR_CLAMPED:
		if (1.0f <= mDuration)
			aProgress = theObj->mKeyTime / mDuration;
		else
			aProgress = 0.0f;
		break;
	case EASE_IN_SQUARE: {
		float aT = theObj->mKeyTime / mDuration;
		aProgress = aT * aT;
		break;
	}
	case EASE_IN_CUBE: {
		float aT = theObj->mKeyTime / mDuration;
		aProgress = (float)((double)aT * aT * aT);
		break;
	}
	case EASE_LINEAR:
		aProgress = theObj->mKeyTime / mDuration;
		break;
	case EASE_COSINE:
	case EASE_COSINE_TOO: {
		float aT = theObj->mKeyTime / mDuration;
		float anAngle = (float)((double)aT * PI_F);
		aProgress = (float)((1.0 - FloatCos(anAngle)) * 0.5);
		break;
	}
	case EASE_BY_DISTANCE: {
		AnimKey* aKey = theObj->mCurrentKey;
		float aDY = theObj->mState[STATE_Y] - aKey->mNextValue[STATE_Y];
		float aDX = theObj->mState[STATE_X] - aKey->mNextValue[STATE_X];
		float aLeft = FloatSqrt((float)((double)aDX * aDX + (double)aDY * aDY));
		float aWholeDY = aKey->mValue[STATE_Y] - aKey->mNextValue[STATE_Y];
		float aWholeDX = aKey->mValue[STATE_X] - aKey->mNextValue[STATE_X];
		float aWhole = FloatSqrt((float)((double)aWholeDX * aWholeDX + (double)aWholeDY * aWholeDY));
		aProgress = (float)((double)aLeft / aWhole);
		if (aLeft <= mDuration)
			aProgress = 1.0f;
		break;
	}
	case EASE_UNSET:
		// the original goes on with whatever its stack held; nothing builds a key with this easing
		aProgress = 0.0f;
		break;
	default:
		aProgress = 0.5f;
		break;
	}

	bool anEnded = false;
	switch (mEndCondition) {
	case KEY_END_ABOVE:
		anEnded = mEndValue < theObj->mState[mEndChannel];
		break;
	case KEY_END_DRAWN_ABOVE:
		anEnded = mEndValue * gImageScaleY < theObj->mDrawn[mEndChannel];
		break;
	case KEY_END_DRAWN_BELOW:
		anEnded = mEndValue * gImageScaleY > theObj->mDrawn[mEndChannel];
		break;
	case KEY_END_ARRIVED: {
		float aDY = theObj->mState[STATE_Y] - mNextValue[STATE_Y];
		float aDX = theObj->mState[STATE_X] - mNextValue[STATE_X];
		float aDistance = FloatSqrt((float)((double)aDX * aDX + (double)aDY * aDY));
		anEnded = aDistance <= 8.0f;
		break;
	}
	case KEY_END_OFF_SCREEN: {
		float aX = theObj->mDrawn[STATE_X];
		float aY = theObj->mDrawn[STATE_Y];
		anEnded = aX < 0.0f || (float)ObjBase::sBounds->mWidth <= aX || aY < 0.0f ||
			(float)ObjBase::sBounds->mHeight <= aY;
		break;
	}
	case KEY_END_AT_ONCE:
		anEnded = true;
		break;
	default:
		break;
	}
	if (anEnded)
		aProgress = 1.0f;

	float aPulse = aProgress + aProgress;
	theObj->mKeyPulse = aPulse;
	if (1.0f < aPulse)
		theObj->mKeyPulse = (float)(1.0 - (aPulse - 1.0));
	return aProgress;
}

// ---------------------------------------------------------------------------------------------------------------
// AnimTrack

// 0x453930
AnimTrack::AnimTrack()
{
	mFirst = nullptr;
	mCurrent = nullptr;
	mLast = nullptr;
	mKeyCount = 0;
	mDelayLeft = 0;
	mDelay = 0;
	mKeyStarting = true;
	mNext = nullptr;
}

// port: the original frees a track with free() after Clear (0x453950)
AnimTrack::~AnimTrack()
{
	Clear();
}

// 0x4539a0: shares the keys
void AnimTrack::CopyFrom(const AnimTrack& theTrack)
{
	mKeyCount = theTrack.mKeyCount;
	mFirst = theTrack.mFirst;
	mCurrent = theTrack.mCurrent;
	mLast = theTrack.mLast;
	mDelayLeft = theTrack.mDelayLeft;
	mDelay = theTrack.mDelay;
	mKeyStarting = true;
	AnimKey* aKey = mFirst;
	for (int i = 0; i < mKeyCount; i++) {
		aKey->AddRef();
		aKey = aKey->mNext;
	}
}

// 0x453950
void AnimTrack::Clear()
{
	AnimKey* aKey = mFirst;
	for (int i = 0; i < mKeyCount; i++) {
		AnimKey* aNext = aKey->mNext;
		aKey->Release();
		aKey = aNext;
	}
	mFirst = nullptr;
	mCurrent = nullptr;
	mLast = nullptr;
	mKeyCount = 0;
	mDelayLeft = 0;
	mDelay = 0;
	mNext = nullptr;
	mKeyStarting = true;
}

// 0x4539f0: theNext is where the key leads (null for the last; the first key makes the track loop)
void AnimTrack::AddKey(AnimKey* theKey, AnimKey* theNext)
{
	theKey->AddRef();
	if (mFirst == nullptr) {
		mFirst = theKey;
		mCurrent = theKey;
		mLast = theKey;
	} else {
		mLast->mNext = theKey;
		mLast = theKey;
	}
	theKey->mNext = theNext;
	mKeyCount++;
}

// 0x453a40
void AnimTrack::Restart()
{
	AnimKey* aKey = mFirst;
	for (int i = 0; i < mKeyCount; i++) {
		aKey->ResetModifiers();
		aKey = aKey->mNext;
	}
	mCurrent = mFirst;
	mFirst->SetNextValues(mFirst->mNext);
	mKeyStarting = true;
	mDelayLeft = mDelay;
}

// 0x453a90
void AnimTrack::SetDelay(int theFrames, bool skipFirstFrame)
{
	if (theFrames > -1) {
		mDelayLeft = theFrames;
		mDelay = skipFirstFrame ? 0 : theFrames;
	}
}

// 0x453ac0
Anim* AnimTrack::GetCurrentAnim() const
{
	return mCurrent->mAnim;
}

// 0x453ab0
int AnimTrack::GetCurrentDrawMode() const
{
	return mCurrent->mDrawMode;
}

// 0x453ad0: one frame of the current key: its modifiers, then on to the next key when it is done
int AnimTrack::Update(ObjBase* theObj)
{
	if (mCurrent == nullptr)
		return TRACK_WAIT;
	if (mDelayLeft < 1) {
		if (mKeyStarting) {
			mKeyStarting = false;
			if (mCurrent->mEndCondition == KEY_SOUND)
				ObjBase::sApp->mSoundQueue->Request(mCurrent->mEndChannel, (int)mCurrent->mEndValue, false);
		}
	} else {
		if (mDelayLeft != mDelay) {
			mDelayLeft--;
			return TRACK_WAIT;
		}
		mDelayLeft--;
	}

	float aProgress = 1.0f;
	if (1.0f <= mCurrent->mDuration)
		aProgress = mCurrent->Progress(theObj);
	for (int aChannel = 0; aChannel < NUM_STATES; aChannel++)
		for (KeyModifier* aMod = mCurrent->mModifiers[aChannel]; aMod != nullptr; aMod = aMod->mNext)
			ApplyModifier(aMod, theObj, aChannel, aProgress);

	if (!(1.0f <= aProgress))
		return TRACK_RUNNING;

	AnimKey* anEnded = mCurrent;
	mKeyStarting = true;
	if (anEnded->mNext != nullptr) {
		AnimKey* aNext = anEnded->mNext;
		for (int i = 0; i < NUM_STATES; i++)
			if (aNext->mKeepCurrent[i])
				aNext->mValue[i] = theObj->mState[i];
	}
	mCurrent = anEnded->mNext;
	theObj->mCurrentKey = mCurrent;
	theObj->mKeyStartSpeed = theObj->mSmoothVelX <= theObj->mSmoothVelY ? theObj->mSmoothVelY : theObj->mSmoothVelX;
	if (mCurrent != nullptr && (1.0f <= mCurrent->mDuration || mCurrent->mNext != nullptr)) {
		mCurrent->SetNextValues(mCurrent->mNext);
		return mCurrent->mAnim != nullptr ? TRACK_NEXT_KEY_WITH_ANIM : TRACK_NEXT_KEY;
	}
	if (mNext != nullptr)
		return TRACK_NEXT_TRACK;
	return anEnded->mRemoveAtEnd ? TRACK_ENDED_REMOVE : TRACK_ENDED;
}

// ---------------------------------------------------------------------------------------------------------------
// ObjBase

// 0x4509f0
ObjBase::ObjBase()
{
	mUnusedC = 0;
	mUnused10 = 0;
	mChildColorOwn = false;
	mFollowMouse = false;
	mNotUpdatedYet = true;
	mUpdateCount = 0;
	for (int i = 0; i < OBJ_MAX_TRACKS; i++)
		mTracks[i] = nullptr;
	for (int i = 0; i < NUM_OBJ_LISTS; i++) {
		mFirstChild[i] = nullptr;
		mNextSibling[i] = nullptr;
		mLastChild[i] = nullptr;
		mPrevSibling[i] = nullptr;
	}
	mKeyTime = 0.0f;
	sObjectCount++;
	mKeySpeed = 1.0f;
	mEventTarget = this;
	mMouseListener = this;
	mType = OBJ_TYPE_PLAIN;
	mMouseX = 0.0f;
	mCurrentTrack = nullptr;
	mMouseY = 0.0f;
	mTrackCount = 0;
	mPrevX = 0.0f;
	mCurrentKey = nullptr;
	mPrevY = 0.0f;
	mParent = nullptr;
	mVelX = 0.0f;
	mOwner = nullptr;
	mVelY = 0.0f;
	mActive = false;
	mSmoothVelX = 0.0f;
	mAnim = nullptr;
	mSmoothVelY = 0.0f;
	mOwnsAnim = true;
	mKeyStartSpeed = 0.0f;
	mDrawMode = 0;
	mHidden = true;
	mHitRadius = 16.0f;
	mHitWidth = 16.0f;
	mHitHeight = 16.0f;
	mScreenX = 0.0f;
	mScreenY = 0.0f;
	mMouseable = false;
	mHoverToParent = false;
	mDeleteIndex = -1;
	mWholePixels = false;
	mUpdateLevel = 1;
	mKeyPulse = 0.0f;
	ResetStates();
	if (sRoot != nullptr) {
		mParent = sRoot;
		ListAdd(LIST_OWNED, mParent);
		ListAdd(LIST_INACTIVE, mParent);
	}
}

// 0x450bb0
ObjBase::~ObjBase()
{
	if (sPressed == this)
		sPressed = nullptr;
	if (sHover == this)
		sHover = nullptr;
	mEventTarget = nullptr;
	if (mDeleteIndex > -1)
		sDeleted[mDeleteIndex] = nullptr;
	sObjectCount--;
	sDrawnCount = 0;
	for (int i = 0; i < mTrackCount; i++) {
		if (mTracks[i] != nullptr)
			delete mTracks[i];
	}
	if (mParent != nullptr) {
		mParent->Deactivate(this);
		ListRemove(LIST_INACTIVE, mParent);
		ListRemove(LIST_OWNED, mParent);
	}
	for (ObjBase* aChild = mFirstChild[LIST_OWNED]; aChild != nullptr;) {
		ObjBase* aNext = aChild->mNextSibling[LIST_OWNED];
		if (aChild->mType == OBJ_TYPE_OWNED_ELSEWHERE && aChild->mOwner != this && aChild->mOwner != nullptr)
			aChild->mOwner->Deactivate(aChild);
		aChild = aNext;
	}
	for (ObjBase* aChild = mFirstChild[LIST_OWNED]; aChild != nullptr;) {
		ObjBase* aNext = aChild->mNextSibling[LIST_OWNED];
		delete aChild;
		aChild = aNext;
	}
}

// 0x450cc0: the other object's tracks (sharing their keys), picture and values
void ObjBase::CopyFrom(ObjBase* theObj)
{
	if (theObj == this)
		return;
	for (int i = 0; i < theObj->mTrackCount; i++) {
		AnimTrack* aTrack = new AnimTrack();
		aTrack->CopyFrom(*theObj->mTracks[i]);
		aTrack->Restart();
		AddTrack(aTrack, nullptr);
	}
	mCurrentTrack = mTracks[0];
	if (mTracks[0] != nullptr) {
		mCurrentKey = mTracks[0]->GetCurrent();
		if (mCurrentKey != nullptr && mCurrentKey->mAnim != nullptr) {
			mCurrentKey->mAnim->AddRef();
			mAnim = mCurrentKey->mAnim;
			mDrawMode = mCurrentKey->mDrawMode;
			mOwnsAnim = false;
			UpdateHitSize();
		}
	}
	if (theObj->mAnim != nullptr) {
		mOwnsAnim = true;
		theObj->mAnim->AddRef();
		mAnim = theObj->mAnim;
		mDrawMode = theObj->mDrawMode;
		UpdateHitSize();
	}
	for (int i = 0; i < NUM_STATES; i++)
		mDrawn[i] = theObj->mDrawn[i];
	for (int i = 0; i < NUM_STATES; i++)
		mState[i] = theObj->mState[i];
	for (int i = 0; i < NUM_STATES; i++)
		mBase[i] = theObj->mBase[i];
}

// 0x450dd0
void ObjBase::Init()
{
	AnimTrack* aTrack = mCurrentTrack;
	mKeyTime = 0.0f;
	mKeySpeed = 1.0f;
	mVelX = 0.0f;
	mVelY = 0.0f;
	mSmoothVelX = 0.0f;
	mSmoothVelY = 0.0f;
	mKeyStartSpeed = 0.0f;
	aTrack->Restart();
	mCurrentKey = mCurrentTrack->GetCurrent();
	if (mCurrentKey != nullptr && mCurrentKey->mAnim != nullptr) {
		mAnim = mCurrentKey->mAnim;
		mDrawMode = mCurrentKey->mDrawMode;
		UpdateHitSize();
	}
}

// 0x450e40: theObj goes into its parent's active list (called on the object itself, or on a parent for a child).
// The hit size it then updates is this object's, not theObj's.
void ObjBase::Activate(ObjBase* theObj, bool hidden)
{
	if (theObj->mActive) {
		theObj->mHidden = hidden;
		return;
	}
	theObj->ListRemove(LIST_INACTIVE, theObj->mParent);
	theObj->ListAdd(LIST_ACTIVE, theObj->mParent);
	theObj->mKeyTime = 0.0f;
	theObj->mActive = true;
	theObj->mKeySpeed = 1.0f;
	if (mAnim != nullptr)
		UpdateHitSize();
	theObj->mHidden = hidden;
}

// 0x450ec0
void ObjBase::Deactivate(ObjBase* theObj)
{
	if (!theObj->mActive)
		return;
	if (sHover == theObj)
		sHover = nullptr;
	theObj->ListRemove(LIST_ACTIVE, theObj->mParent);
	theObj->ListAdd(LIST_INACTIVE, theObj->mParent);
	theObj->mActive = false;
}

// 0x450f10: a key of theObj's ended; the base passes it on
void ObjBase::Notify(ObjBase* theObj)
{
	if (theObj != this)
		theObj->Notify(this);
}

// 0x450f30: one frame of the current track, then the drawn values and the motion
bool ObjBase::Update()
{
	mUpdateCount++;
	mPrevX = mState[STATE_X];
	mPrevY = mState[STATE_Y];
	mNotUpdatedYet = false;
	if (mCurrentTrack == nullptr || mCurrentKey == nullptr) {
		for (int i = 0; i < NUM_STATES; i++)
			mState[i] = mBase[i];
	} else {
		switch (mCurrentTrack->Update(this)) {
		case TRACK_RUNNING:
			mKeyTime = mKeySpeed + mKeyTime;
			if (mState[STATE_ALPHA] < 0.0f)
				mState[STATE_ALPHA] = 0.0f;
			if (1.0f < mState[STATE_ALPHA])
				mState[STATE_ALPHA] = 1.0f;
			break;
		case TRACK_NEXT_KEY:
			mKeyTime = 1.0f;
			Notify(mEventTarget);
			break;
		case TRACK_NEXT_KEY_WITH_ANIM:
			mKeyTime = 1.0f;
			Notify(mEventTarget);
			mAnim = mCurrentTrack->GetCurrentAnim();
			mDrawMode = mCurrentTrack->GetCurrentDrawMode();
			mOwnsAnim = false;
			UpdateHitSize();
			break;
		case TRACK_ENDED:
			mKeyTime = 0.0f;
			Notify(mEventTarget);
			if (mCurrentKey == nullptr) {
				ObjBase* aFrom = mOwner != nullptr ? mOwner : mParent;
				aFrom->Deactivate(this);
			}
			break;
		case TRACK_ENDED_REMOVE:
			mKeyTime = 0.0f;
			Notify(mEventTarget);
			Delete();
			return false;
		case TRACK_NEXT_TRACK: {
			mKeyTime = 1.0f;
			Notify(mEventTarget);
			mCurrentTrack = mCurrentTrack->mNext;
			mCurrentTrack->Restart();
			AnimKey* aKey = mCurrentTrack->GetCurrent();
			mCurrentKey = aKey;
			if (aKey->mAnim != nullptr) {
				mAnim = aKey->mAnim;
				mDrawMode = aKey->mDrawMode;
				mOwnsAnim = false;
				UpdateHitSize();
			}
			mCurrentTrack->Update(this);
			break;
		}
		default:
			break;
		}
	}
	ComputeDrawn();
	float aVelX = mState[STATE_X] - mPrevX;
	mVelX = aVelX;
	mVelY = mState[STATE_Y] - mPrevY;
	mSmoothVelX = (float)((mSmoothVelX + aVelX) * 0.5);
	mSmoothVelY = (float)(((mState[STATE_Y] - mPrevY) + mSmoothVelY) * 0.5);
	return true;
}

// 0x4511a0: deleted after this frame's update
void ObjBase::Delete()
{
	if (mDeleteIndex < 0) {
		mDeleteIndex = sDeletedCount;
		sDeleted[sDeletedCount] = this;
		sDeletedCount++;
	}
}

// 0x4511c0: the current cel, centred on the drawn position, scaled, offset, rotated and tinted. The original also
// draws a debug box for a field nothing sets (+0x268); the port leaves it out.
void ObjBase::Draw(Graphics* g)
{
	SexyTransform2D aMatrix;
	if (mHidden)
		return;
	float aX = mDrawn[STATE_X];
	float aY = mDrawn[STATE_Y];
	if (mAnim == nullptr)
		return;
	aMatrix.LoadIdentity();
	aMatrix.Scale(mDrawn[STATE_SCALE_X], mDrawn[STATE_SCALE_Y]);
	aMatrix.Translate(mDrawn[STATE_OFFSET_X], mDrawn[STATE_OFFSET_Y]);
	aMatrix.RotateRad(mDrawn[STATE_ROTATION]);
	g->SetColorizeImages(true);
	int anAlpha = (int)((double)mDrawn[STATE_ALPHA] * 255.0);
	int aBlue = (int)((double)mDrawn[STATE_BLUE] * 255.0);
	int aGreen = (int)((double)mDrawn[STATE_GREEN] * 255.0);
	int aRed = (int)((double)mDrawn[STATE_RED] * 255.0);
	g->SetColor(Color(aRed, aGreen, aBlue, anAlpha));
	g->SetDrawMode(mDrawMode);
	int aCel = (int)mState[STATE_FRAME];
	if (mWholePixels)
		mAnim->DrawCelAt(aCel, g, (int)aX, (int)aY, aMatrix, mDrawn[STATE_EXTRA_WIDTH], mDrawn[STATE_EXTRA_HEIGHT]);
	else
		mAnim->DrawCel(aCel, g, aX, aY, aMatrix, mDrawn[STATE_EXTRA_WIDTH], mDrawn[STATE_EXTRA_HEIGHT]);
	g->SetColorizeImages(false);
	g->SetDrawMode(0);
}

// 0x4e2910 (a shared stub)
bool ObjBase::MouseDown(ObjBase* theObj, bool isDown)
{
	return false;
}

// 0x5656c0 (a shared stub)
bool ObjBase::MouseUp(ObjBase* theObj)
{
	return false;
}

// 0x4d4050 (a shared stub)
void ObjBase::MouseEnter(ObjBase* theObj, bool isNew)
{
}

// 0x42c290 (a shared stub)
void ObjBase::MouseLeave(ObjBase* theObj)
{
}

// 0x42c290 (a shared stub)
void ObjBase::Slot12(int theValue)
{
}

// 0x5656b0 (a shared stub)
void ObjBase::Slot13(int theFirst, int theSecond, int theThird)
{
}

// 0x451500: every value 0 but scale and colour 1, at a depth just above the last object made
void ObjBase::ResetStates()
{
	for (int i = 0; i < NUM_STATES; i++) {
		mDrawn[i] = 0.0f;
		mBase[i] = 0.0f;
		mState[i] = 0.0f;
	}
	SetState(STATE_Z, (float)((float)sObjectCount / 1e+07));
	SetStateInt(STATE_RED, 1);
	SetStateInt(STATE_GREEN, 1);
	SetStateInt(STATE_BLUE, 1);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
}

// 0x451580: moves the object under another parent (the root for null), keeping it active or not
void ObjBase::SetParent(ObjBase* theParent, ObjBase* theOwner)
{
	if (sRoot == nullptr)
		return;
	if (theParent == nullptr)
		theParent = sRoot;
	if (theParent == this || theParent == mParent)
		return;
	if (mParent != nullptr)
		ListRemove(LIST_OWNED, mParent);
	ListAdd(LIST_OWNED, theParent);
	int aList;
	if (mActive) {
		if (mParent != nullptr)
			ListRemove(LIST_ACTIVE, mParent);
		aList = LIST_ACTIVE;
	} else {
		if (mParent != nullptr)
			ListRemove(LIST_INACTIVE, mParent);
		aList = LIST_INACTIVE;
	}
	ListAdd(aList, theParent);
	mParent = theParent;
	if (theOwner != nullptr)
		mOwner = theOwner;
}

// 0x451600: a box of 32 without a picture, else 0.35 of the cel each way, no lower than it is wide
void ObjBase::UpdateHitSize()
{
	if (mAnim == nullptr) {
		mHitWidth = 32.0f;
		mHitHeight = 32.0f;
		mHitRadius = mHitWidth;
		return;
	}
	const double HIT_SHARE = 0.3499999940395355;
	mHitWidth = (float)(mAnim->GetSourceCelWidth() * HIT_SHARE);
	float aHeight = (float)(mAnim->GetSourceCelHeight() * HIT_SHARE);
	mHitHeight = aHeight;
	if (aHeight < mHitWidth)
		mHitHeight = mHitWidth;
	mHitRadius = mHitWidth;
}

// 0x4516b0
void ObjBase::SetMouseable(float theWidth, float theHeight)
{
	mMouseable = true;
	mHoverToParent = false;
	if (mAnim == nullptr) {
		mHitWidth = theWidth;
		mHitHeight = theHeight;
	}
}

// 0x4516e0
void ObjBase::ListAdd(int theList, ObjBase* theParent)
{
	if (theParent->mFirstChild[theList] == nullptr) {
		theParent->mFirstChild[theList] = this;
		mPrevSibling[theList] = nullptr;
		mNextSibling[theList] = nullptr;
		theParent->mLastChild[theList] = this;
		return;
	}
	theParent->mLastChild[theList]->mNextSibling[theList] = this;
	mPrevSibling[theList] = theParent->mLastChild[theList];
	mNextSibling[theList] = nullptr;
	theParent->mLastChild[theList] = this;
}

// 0x451740
void ObjBase::ListRemove(int theList, ObjBase* theParent)
{
	if (this == theParent->mFirstChild[theList])
		theParent->mFirstChild[theList] = mNextSibling[theList];
	if (this == theParent->mLastChild[theList])
		theParent->mLastChild[theList] = mPrevSibling[theList];
	if (mPrevSibling[theList] != nullptr)
		mPrevSibling[theList]->mNextSibling[theList] = mNextSibling[theList];
	if (mNextSibling[theList] != nullptr)
		mNextSibling[theList]->mPrevSibling[theList] = mPrevSibling[theList];
}

// 0x4517b0: an x in the design space, made relative to the parent's picture (or to the space's centre)
void ObjBase::SetDesignX(float theX)
{
	float aX;
	if (mParent->mAnim == nullptr)
		aX = (float)((double)theX - 960.0);
	else
		aX = (float)((double)theX - mParent->mAnim->GetSourceCelWidth() * 0.5);
	mBase[STATE_X] = aX;
	mState[STATE_X] = aX;
}

// 0x4517f0
void ObjBase::SetDesignY(float theY)
{
	float aY;
	if (mParent->mAnim == nullptr)
		aY = (float)((double)theY - 600.0);
	else
		aY = (float)((double)theY - mParent->mAnim->GetSourceCelHeight() * 0.5);
	mBase[STATE_Y] = aY;
	mState[STATE_Y] = aY;
}

// 0x451830: the value now, as drawn and as the base
void ObjBase::SetState(int theChannel, float theValue)
{
	mDrawn[theChannel] = theValue;
	mBase[theChannel] = theValue;
	mState[theChannel] = theValue;
	if (theChannel == STATE_X)
		mPrevX = theValue;
	else if (theChannel == STATE_Y)
		mPrevY = theValue;
}

// 0x451880: the first track becomes the current one
int ObjBase::AddTrack(AnimTrack* theTrack, AnimTrack* theNext)
{
	if (mCurrentTrack == nullptr) {
		mCurrentTrack = theTrack;
		AnimKey* aKey = theTrack->GetCurrent();
		mCurrentKey = aKey;
		if (aKey->mAnim != nullptr) {
			mAnim = aKey->mAnim;
			mDrawMode = aKey->mDrawMode;
			mOwnsAnim = false;
			UpdateHitSize();
		}
	}
	mTracks[mTrackCount] = theTrack;
	theTrack->mNext = theNext;
	mTrackCount++;
	return mTrackCount - 1;
}

// 0x4518e0
void ObjBase::ClearTracks()
{
	for (int i = 0; i < mTrackCount; i++) {
		if (mTracks[i] != nullptr)
			delete mTracks[i];
		mTracks[i] = nullptr;
	}
	if (mCurrentKey->mAnim != nullptr)
		mAnim = nullptr;
	mTrackCount = 0;
	mCurrentKey = nullptr;
	mCurrentTrack = nullptr;
}

// 0x451940
void ObjBase::StartTrack(int theIndex)
{
	mCurrentTrack = mTracks[theIndex];
	mCurrentTrack->Restart();
	AnimKey* aKey = mCurrentTrack->GetCurrent();
	mCurrentKey = aKey;
	if (aKey->mAnim != nullptr) {
		mAnim = aKey->mAnim;
		mDrawMode = aKey->mDrawMode;
		mOwnsAnim = false;
		UpdateHitSize();
	}
	mKeyTime = 0.0f;
}

// 0x451990: the drawn values from the parent's, and the mouse in the object's own space
void ObjBase::ComputeDrawn()
{
	GameApp* anApp = sApp;
	ObjBase* aParent = mParent;
	float aMouseX;
	float aMouseY;
	if (aParent == nullptr) {
		for (int i = 0; i < NUM_STATES; i++)
			mDrawn[i] = mBase[i];
		aMouseX = sMouseX;
		mMouseX = (float)(((double)sMouseX * anApp->mDesignScaleX - 960.0 - mState[STATE_X]) * gImageScaleX);
		aMouseY = sMouseY;
		mMouseY = (float)(((double)sMouseY * anApp->mDesignScaleY - 600.0 - mState[STATE_Y]) * gImageScaleY);
	} else {
		float aLocalX = mState[STATE_X];
		float aLocalY = mState[STATE_Y];
		if (aParent->mDrawn[STATE_ROTATION] != 0.0f) {
			float anAngle = -aParent->mDrawn[STATE_ROTATION];
			float aCos = FloatCos(anAngle);
			float aSin = FloatSin(anAngle);
			float aX = aLocalX;
			aLocalX = (float)((double)aCos * aX - (double)aSin * aLocalY);
			aLocalY = (float)((double)aCos * aLocalY + (double)aSin * mState[STATE_X]);
		}
		float aY = (float)((double)FloatAbs(aParent->mDrawn[STATE_SCALE_Y]) * aLocalY * gImageScaleY +
			aParent->mDrawn[STATE_Y]);
		mDrawn[STATE_X] = (float)((double)aParent->mDrawn[STATE_X] +
			(double)FloatAbs(aParent->mDrawn[STATE_SCALE_X]) * aLocalX * gImageScaleX);
		mDrawn[STATE_Y] = aY;
		mDrawn[STATE_Z] = aParent->mDrawn[STATE_Z] + mState[STATE_Z];
		mDrawn[STATE_ROTATION] = mState[STATE_ROTATION] + aParent->mDrawn[STATE_ROTATION];
		mDrawn[STATE_ORBIT_ANGLE] = aParent->mDrawn[STATE_ORBIT_ANGLE] + mState[STATE_ORBIT_ANGLE];
		mDrawn[STATE_SCALE_X] = mState[STATE_SCALE_X] * aParent->mDrawn[STATE_SCALE_X];
		mDrawn[STATE_SCALE_Y] = mState[STATE_SCALE_Y] * aParent->mDrawn[STATE_SCALE_Y];
		mDrawn[STATE_OFFSET_X] = (float)((double)mState[STATE_OFFSET_X] * gImageScaleX * mDrawn[STATE_SCALE_X]);
		mDrawn[STATE_OFFSET_Y] = (float)((double)mState[STATE_OFFSET_Y] * gImageScaleY * mDrawn[STATE_SCALE_Y]);
		mDrawn[STATE_EXTRA_HEIGHT] = mState[STATE_EXTRA_HEIGHT];
		mDrawn[STATE_EXTRA_WIDTH] = mState[STATE_EXTRA_WIDTH];
		if (aParent->mChildColorOwn) {
			mDrawn[STATE_RED] = mState[STATE_RED];
			mDrawn[STATE_GREEN] = mState[STATE_GREEN];
			mDrawn[STATE_BLUE] = mState[STATE_BLUE];
			mDrawn[STATE_ALPHA] = mState[STATE_ALPHA];
		} else {
			mDrawn[STATE_RED] = aParent->mDrawn[STATE_RED] * mState[STATE_RED];
			mDrawn[STATE_GREEN] = aParent->mDrawn[STATE_GREEN] * mState[STATE_GREEN];
			mDrawn[STATE_BLUE] = aParent->mDrawn[STATE_BLUE] * mState[STATE_BLUE];
			mDrawn[STATE_ALPHA] = aParent->mDrawn[STATE_ALPHA] * mState[STATE_ALPHA];
		}
		aMouseX = sMouseX;
		mMouseX = (float)(((double)sMouseX - mDrawn[STATE_X]) * mDrawn[STATE_SCALE_X] * anApp->mDesignScaleX);
		aMouseY = sMouseY;
		mMouseY = (float)(((double)sMouseY - aY) * mDrawn[STATE_SCALE_Y] * anApp->mDesignScaleY);
	}
	if (mFollowMouse) {
		mDrawn[STATE_X] = aMouseX;
		mDrawn[STATE_Y] = aMouseY;
		mState[STATE_X] = mMouseX;
		mBase[STATE_X] = mMouseX;
		mState[STATE_Y] = mMouseY;
		mBase[STATE_Y] = mMouseY;
	}
	mScreenX = (float)((double)anApp->mWindowToDesignX * mDrawn[STATE_X] - 960.0);
	mScreenY = (float)((double)anApp->mWindowToDesignY * mDrawn[STATE_Y] - 600.0);
}

// 0x451cd0: this object, then its active children while it lives on
bool ObjBase::UpdateTree()
{
	bool isAlive = Update();
	if (isAlive) {
		for (ObjBase* aChild = mFirstChild[LIST_ACTIVE]; aChild != nullptr;) {
			ObjBase* aNext = aChild->mNextSibling[LIST_ACTIVE];
			aChild->UpdateTree();
			aChild = aNext;
		}
	}
	return isAlive;
}

// 0x451d10: the same, only for objects whose level is above the engine's (a pause lets the menus run)
bool ObjBase::UpdateTreeAtLevel()
{
	bool isAlive;
	if (mUpdateLevel < sUpdateLevel) {
		isAlive = Update();
		if (!isAlive)
			return false;
	} else {
		isAlive = true;
	}
	for (ObjBase* aChild = mFirstChild[LIST_ACTIVE]; aChild != nullptr;) {
		ObjBase* aNext = aChild->mNextSibling[LIST_ACTIVE];
		aChild->UpdateTreeAtLevel();
		aChild = aNext;
	}
	return isAlive;
}

// 0x451d60: every shown object with something to draw or to hit, in tree order
void ObjBase::CollectDrawn()
{
	if (!mHidden && !mNotUpdatedYet && (mAnim != nullptr || mMouseable)) {
		sDrawn[sDrawnCount] = this;
		sDrawnCount++;
	}
	for (ObjBase* aChild = mFirstChild[LIST_ACTIVE]; aChild != nullptr; aChild = aChild->mNextSibling[LIST_ACTIVE])
		aChild->CollectDrawn();
}

// 0x451de0: the whole tree, then the objects deleted during it
void ObjBase::UpdateAll(int theFrame)
{
	sFrame = theFrame;
	if (sUpdateLevel < 1) {
		sRoot->UpdateTree();
	} else {
		sRoot->UpdateTreeAtLevel();
		// the original sleeps a millisecond here (Sleep(1)); the port does not wait
	}
	for (int i = 0; i < sDeletedCount; i++) {
		if (sDeleted[i] != nullptr)
			delete sDeleted[i];
	}
	sDeletedCount = 0;
}

// 0x451e50: the draw order, by drawn depth
static bool DrawnBelow(ObjBase* theLeft, ObjBase* theRight)
{
	return theLeft->mDrawn[STATE_Z] < theRight->mDrawn[STATE_Z];
}

// 0x451e80: draws every object by depth and finds the one under the mouse, the first in drawing order whose box
// holds it
void ObjBase::DrawAll(Graphics* g)
{
	ObjBase* aLastHover = sHover;
	sHover = nullptr;
	sDrawnCount = 0;
	sRoot->CollectDrawn();
	MsvcSort::Sort(sDrawn, sDrawn + sDrawnCount, DrawnBelow);
	for (int i = 0; i < sDrawnCount; i++) {
		ObjBase* anObj = sDrawn[i];
		anObj->Draw(g);
		if (anObj->mMouseable && sHoverMinZ <= anObj->mDrawn[STATE_Z] && sHover == nullptr) {
			float aDistX = FloatAbs((anObj->mState[STATE_OFFSET_X] + anObj->mScreenX) - sMouseDesignX);
			float aDistY = FloatAbs((anObj->mState[STATE_OFFSET_Y] + anObj->mScreenY) - sMouseDesignY);
			double aHalfWidth;
			double aHalfHeight;
			if (anObj->mAnim == nullptr) {
				aHalfWidth = (double)anObj->mHitWidth * 0.5;
				aHalfHeight = 0.5 * anObj->mHitHeight;
			} else {
				aHalfWidth = anObj->mAnim->GetSourceCelWidth() * 0.5;
				aHalfHeight = anObj->mAnim->GetSourceCelHeight() * 0.5;
			}
			float aHalfW = (float)aHalfWidth;
			if (aDistX <= aHalfW && aDistY <= (float)aHalfHeight) {
				if (anObj->mHoverToParent)
					anObj = anObj->mParent;
				sHover = anObj;
				anObj->mMouseListener->MouseEnter(anObj, aLastHover != anObj);
			}
		}
	}
	if (aLastHover != sHover && aLastHover != nullptr)
		aLastHover->mMouseListener->MouseLeave(aLastHover);
}

// 0x452040: whether the object under the mouse took the press
bool ObjBase::MousePress()
{
	sMouseHeld = true;
	sMouseReleased = false;
	bool aTaken = false;
	if (sHover != nullptr)
		aTaken = sHover->mMouseListener->MouseDown(sHover, true);
	sPressed = sHover;
	return aTaken;
}

// 0x452080
void ObjBase::MouseDrag(int x, int y)
{
	sMouseHeld = true;
	SetMousePos(x, y);
	if (sHover != nullptr)
		sHover->mMouseListener->MouseDown(sHover, false);
}

// 0x4520b0
void ObjBase::MouseRelease()
{
	sMouseHeld = false;
	sMouseReleased = true;
	if (sPressed != nullptr)
		sPressed->mMouseListener->MouseUp(sPressed);
}

// 0x452100
int ObjBase::TakeKeyPressed()
{
	int aKey = sKeyPressed;
	sKeyPressed = 0;
	return aKey;
}

// 0x452120: the pictures are made at the window's scale
void ObjBase::FollowScreenScale()
{
	gImageScaleX = gScreenScaleX;
	gImageScaleY = gScreenScaleY;
}

// 0x452140: the root object, at the window's centre
void ObjBase::InitEngine(GameApp* theApp)
{
	sApp = theApp;
	if (theApp == nullptr)
		return;
	Anim::SetApp(theApp);
	sRoot = new ObjBase();
	sRoot->mType = OBJ_TYPE_ROOT;
	sRoot->SetState(STATE_X, (float)((double)sApp->mWidth * 0.5));
	sRoot->SetState(STATE_Y, (float)((double)sApp->mHeight * 0.5));
	sRoot->SetStateInt(STATE_Z, 0);
	sRoot->SetStateInt(STATE_SCALE_X, 1);
	sRoot->SetStateInt(STATE_SCALE_Y, 1);
	sRoot->SetStateInt(STATE_ROTATION, 0);
	sRoot->SetStateInt(STATE_ORBIT_ANGLE, 0);
	sRoot->SetStateInt(STATE_OFFSET_X, 0);
	sRoot->SetStateInt(STATE_OFFSET_Y, 0);
	sRoot->SetStateInt(STATE_RED, 1);
	sRoot->SetStateInt(STATE_GREEN, 1);
	sRoot->SetStateInt(STATE_BLUE, 1);
	sRoot->SetStateInt(STATE_ALPHA, 1);
	sRoot->mParent = nullptr;
}

// 0x452270
void ObjBase::SetMousePos(int x, int y)
{
	float aLastX = sMouseDesignX;
	float aLastY = sMouseDesignY;
	sMouseX = (float)x;
	sMouseY = (float)y;
	sMouseDesignX = (float)((double)sApp->mWindowToDesignX * (float)x - 960.0);
	sMouseDesignY = (float)((double)sApp->mWindowToDesignY * (float)y - 600.0);
	sMouseAverageX = (float)((sMouseDesignX + aLastX) * 0.5);
	sMouseAverageY = (float)((sMouseDesignY + aLastY) * 0.5);
}
