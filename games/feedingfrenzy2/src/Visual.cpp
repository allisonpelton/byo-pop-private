#include "Visual.h"

#include "ActorInfo.h"
#include "Engine.h"
#include "ParticleSystem.h"
#include "Theme.h"
#include "VectorMath.h"

#include <cmath>

using namespace engine;

namespace
{

// The run names the states play (std::strings made at start: 0x5ac71c, 0x5ac8cc, 0x5ac878, 0x5ac8b0, 0x5ac8e8 and
// 0x5ac920 (feedingfrenzy2))
const char* const RUN_IDLE = "idle";
const char* const RUN_SWIM = "swim";
const char* const RUN_TURN = "turn";
const char* const RUN_EAT = "eat";
const char* const RUN_SUCK_INTRO = "SuckIntro";
const char* const RUN_SUCK_OUTRO = "SuckOutro";

// The points of the data the four effects sit at (the effects' table at 0x5ac738, 0x50 bytes each, its names at +0x14:
// 0x5ac6c8 "halo" twice, 0x5ac6e4 "body" and 0x5ac700 "glow" (feedingfrenzy2))
const char* const EFFECT_POINTS[4] = {"halo", "halo", "body", "glow"};

// 0x566b44, 0x566924 and 0x566858 (feedingfrenzy2)
const float IDLE_THRESHOLD = 20.0f;
const float SWIM_THRESHOLD = 60.0f;
const float TURN_THRESHOLD = 15.0f;
// 0x54a334 (feedingfrenzy2): a fast turn's rate
const float FAST_TURN_RATE = 2.0f;
// The size's pulse (0x4251c0 (feedingfrenzy2)): its middle key's time and ease-in (0x566b90, 0.85), the overshoot
// past the new size (0x566b88, the float 1.2 as a double), the middle key's ease-out (0x566880, 0.05) and the end's
// ease-in and its speed (0x54ddd4, 0.1)
const float PULSE_MIDDLE_TIME = 0.85f;
const double PULSE_OVERSHOOT = (double)1.2f;
const float PULSE_MIDDLE_OUT_TIME = 0.05f;
const float PULSE_END_IN = 0.1f;

// 0x409520 (feedingfrenzy2): the frames a second of the data's run, 0 for none
double getRunRate(VisualData* theData, int theRun)
{
	if (theRun < 0 || theRun >= theData->getAnimationCount())
		return 0.0;
	return 1.0 / (double)theData->getAnimation(theRun).mFrameTime;
}

// The x87 loads an unsigned count as a signed one and adds 2^32 below 0 (0x56684c (feedingfrenzy2))
double toUnsignedDouble(int theValue)
{
	return (double)(unsigned int)theValue;
}

} // namespace

Visual::Visual()
{
}

void Visual::setup()
{
	mBehindGroup = new Container();
	mBehindGroup->setName("VisualBehindGroup");
	addChild(mBehindGroup);
	mMirroredGroup = new Container();
	mMirroredGroup->setName("VisualMirroredGroup");
	addChild(mMirroredGroup);
	mImage = new AnimatedImage();
	mImage->mFinished.connect(this, &Visual::onImageFinished);
	mMirroredGroup->addChild(mImage);
	mFrontGroup = new Container();
	mFrontGroup->setName("VisualFrontGroup");
	addChild(mFrontGroup);
}

void Visual::setData(VisualData* theData)
{
	mData = theData;
	mImage->setAnimationData(theData);
	mImage->addState(STATE_BOUNDS_DIRTY);
	mImage->addOwnState(STATE_VISIBLE);
	setName(theData->getInfo()->mModel->mName);
}

void Visual::play(const std::string& theRun, float theRate, int theMode, bool restart)
{
	int aRun = mData->findAnimation(theRun);
	int aDirection = theRate < 0.0f ? -1 : 1;
	if (mImage->getRun() == aRun && mImage->getDirection() == aDirection && aDirection != 0 && !restart)
		return;
	// 0x494ca0 (feedingfrenzy2): the run's frames, its last the start going back
	int anOffset = 0;
	if (aDirection != 1)
	{
		int aCount = 0;
		if (aRun >= 0 && aRun < mData->getAnimationCount())
		{
			const AnimationRun& aRunInfo = mData->getAnimation(aRun);
			aCount = (int)(aRunInfo.mEnd - aRunInfo.mStart);
		}
		anOffset = aCount - 1;
	}
	mImage->setRun(aRun, anOffset);
	mImage->setMode(theMode);
	mImage->setRate((float)fabs((double)theRate));
	mImage->addOwnState(STATE_ANIMATED);
	mImage->setDirection(aDirection);
}

void Visual::onImageFinished(unsigned int theRun)
{
	mFinished.emit();
}

float FishVisualState::getTimeToChomp()
{
	VisualData* aData = mVisual->mData;
	int anEat = aData->findAnimation(RUN_EAT);
	double anEatFrame = toUnsignedDouble(mVisual->mFishData->getFishInfo()->mFishModel->mEatFrame);
	return (float)(anEatFrame / getRunRate(aData, anEat));
}

void FishVisualState::eat()
{
	mVisual->changeState(mVisual->mEatState);
}

void FishVisualIdleState::enter()
{
	int aShuttle = mVisual->mFishData->getFishInfo()->mFishModel->mIdleShuttle;
	mVisual->play(RUN_IDLE, 1.0f, aShuttle < 1 ? ANIMATION_MODE_LOOP : ANIMATION_MODE_SHUTTLE, false);
}

void FishVisualIdleState::update(UpdateContext* theContext)
{
	if (mVisual->turnIfNeeded())
		return;
	if (mVisual->mSwimThreshold < lengthSquared(mVisual->mVelocity))
		mVisual->changeState(mVisual->mSwimState);
}

void FishVisualSwimState::enter()
{
	int aShuttle = mVisual->mFishData->getFishInfo()->mFishModel->mSwimShuttle;
	mVisual->play(RUN_SWIM, 1.0f, aShuttle < 1 ? ANIMATION_MODE_LOOP : ANIMATION_MODE_SHUTTLE, false);
}

void FishVisualSwimState::update(UpdateContext* theContext)
{
	if (mVisual->turnIfNeeded())
		return;
	if (!mVisual->mHasIdle || mVisual->mIdleThreshold <= lengthSquared(mVisual->mVelocity))
	{
		double aRate = (double)length(mVisual->mVelocity) * mVisual->mSwimRateScale + mVisual->mSwimRateBase;
		mVisual->mImage->setRate((float)aRate);
	}
	else
		mVisual->changeState(mVisual->mIdleState);
}

void FishVisualTurnState::enter()
{
	mVisual->play(RUN_TURN, mVisual->mFastTurn ? FAST_TURN_RATE : 1.0f, ANIMATION_MODE_ONCE, false);
	mEatSuppressed = false;
}

void FishVisualTurnState::exit()
{
	FishModelInfo* aModel = mVisual->mFishData->getFishInfo()->mFishModel;
	if (aModel->mTurnSafe.mLow <= mVisual->getRunFrame())
	{
		mVisual->mFacing = (float)((double)mVisual->mFacing * -1.0);
		mVisual->setRotation((float)(-1.0 * (double)mVisual->getRotation()));
	}
	mVisual->mMirror = (float)((double)mVisual->mOrientation * mVisual->mFacing);
	mVisual->setMirrorScale();
	if (mEatSuppressed)
		mVisual->mChomp.emit();
}

void FishVisualTurnState::update(UpdateContext* theContext)
{
	float aProgress = (float)mVisual->getRunProgress();
	double aMirror = (double)mVisual->mOrientation * mVisual->mFacing;
	mVisual->mMirror = (float)(aMirror * (1.0 - ((double)aProgress + aProgress)));
	mVisual->setMirrorScale();
	// 0x5471cc (feedingfrenzy2): 0
	float aSide = (float)((double)mVisual->mMirror * mVisual->mFacing);
	if (0.0f < aSide)
		mVisual->setRotation((float)((double)mVisual->getRotation() * -1.0));
}

void FishVisualTurnState::onRunFinished()
{
	mVisual->resume();
}

bool FishVisualTurnState::canEat()
{
	// Inlined: the frame from the run's start, 0 without frames or a run (as 0x471550 (feedingfrenzy2))
	int aFrame = mVisual->getRunFrame();
	const IntRange& aSafe = mVisual->mFishData->getFishInfo()->mFishModel->mTurnSafe;
	if (aFrame < aSafe.mLow || aSafe.mHigh < aFrame)
		return true;
	return mVisual->mFastTurn;
}

void FishVisualTurnState::eat()
{
	int aFrame = mVisual->getRunFrame();
	const IntRange& aSafe = mVisual->mFishData->getFishInfo()->mFishModel->mTurnSafe;
	if (aFrame < aSafe.mLow || aSafe.mHigh < aFrame)
	{
		mVisual->changeState(mVisual->mEatState);
		return;
	}
	getEngine()->log("EAT SUPPRESSED.\n");
	mEatSuppressed = true;
}

void FishVisualEatState::enter()
{
	mVisual->play(RUN_EAT, 1.0f, ANIMATION_MODE_ONCE, true);
	mChomped = false;
}

void FishVisualEatState::update(UpdateContext* theContext)
{
	unsigned int aFrame = (unsigned int)mVisual->getRunFrame();
	if (!mChomped)
	{
		if ((unsigned int)mVisual->mFishData->getFishInfo()->mFishModel->mEatFrame <= aFrame)
		{
			mChomped = true;
			mVisual->mChomp.emit();
		}
	}
	else
		mVisual->turnIfNeeded();
}

void FishVisualEatState::onRunFinished()
{
	if (!mChomped)
	{
		mChomped = true;
		mVisual->mChomp.emit();
	}
	mVisual->resume();
}

float FishVisualEatState::getTimeToChomp()
{
	if (mChomped)
		return FishVisualState::getTimeToChomp();
	VisualData* aData = mVisual->mData;
	int anEat = aData->findAnimation(RUN_EAT);
	unsigned int anEatFrame = (unsigned int)mVisual->mFishData->getFishInfo()->mFishModel->mEatFrame;
	unsigned int aFrame = (unsigned int)mVisual->getRunFrame();
	float aRate = (float)getRunRate(aData, anEat);
	if (anEatFrame < aFrame)
		return 0.0f;
	float aTime = (float)(toUnsignedDouble((int)(anEatFrame - aFrame)) / aRate);
	return (float)((double)aTime - mVisual->getFrameTimeShown());
}

void FishVisualPukeState::enter()
{
	mVisual->play(RUN_EAT, -1.0f, ANIMATION_MODE_ONCE, false);
	mChomped = false;
}

void FishVisualPukeState::update(UpdateContext* theContext)
{
	unsigned int aFrame = (unsigned int)mVisual->getRunFrame();
	if (!mChomped && aFrame <= (unsigned int)mVisual->mFishData->getFishInfo()->mFishModel->mEatFrame)
	{
		mChomped = true;
		mVisual->mChomp.emit();
	}
}

void FishVisualPukeState::onRunFinished()
{
	if (!mChomped)
	{
		mChomped = true;
		mVisual->mChomp.emit();
	}
	mVisual->resume();
}

float FishVisualPukeState::getTimeToChomp()
{
	VisualData* aData = mVisual->mData;
	int anEat = aData->findAnimation(RUN_EAT);
	unsigned int anEatFrame = (unsigned int)mVisual->mFishData->getFishInfo()->mFishModel->mEatFrame;
	unsigned int aFrame = (unsigned int)mVisual->getRunFrame();
	float aRate = (float)getRunRate(aData, anEat);
	if (anEatFrame < aFrame)
		return 0.0f;
	float aTime = (float)(toUnsignedDouble((int)(anEatFrame - aFrame)) / aRate);
	return (float)((double)aTime - mVisual->getFrameTimeShown());
}

void FishVisualStunnedState::enter()
{
	mVisual->play(RUN_SWIM, 1.0f, ANIMATION_MODE_ONCE, true);
	mVisual->mImage->stop();
}

void FishVisualSuckIntroState::enter()
{
	mVisual->play(RUN_SUCK_INTRO, 1.0f, ANIMATION_MODE_ONCE, false);
	// With the suck's effect (+0x114, the player's): the effect at the data's "mouth" point (0x5ac904), its slot 0x14
	// and its start (0x4306a0 (feedingfrenzy2)): not yet translated, as the player's is not
}

void FishVisualSuckIntroState::exit()
{
	// The suck's effect stopped (0x4322f0 (feedingfrenzy2)): not yet translated, as the player's is not
}

float FishVisualSuckIntroState::getTimeToChomp()
{
	VisualData* aData = mVisual->mData;
	int anEat = aData->findAnimation(RUN_EAT);
	FishModelInfo* aModel = mVisual->mFishData->getFishInfo()->mFishModel;
	double aFrames = toUnsignedDouble(aModel->mEatFrame - aModel->mSuckFrame);
	return (float)(aFrames / getRunRate(aData, anEat));
}

void FishVisualSuckOutroState::enter()
{
	mVisual->play(RUN_SUCK_OUTRO, 1.0f, ANIMATION_MODE_ONCE, false);
	mChomped = false;
}

void FishVisualSuckOutroState::update(UpdateContext* theContext)
{
	unsigned int aFrame = (unsigned int)mVisual->getRunFrame();
	FishModelInfo* aModel = mVisual->mFishData->getFishInfo()->mFishModel;
	if (!mChomped && (unsigned int)(aModel->mEatFrame - aModel->mSuckFrame) <= aFrame)
	{
		mChomped = true;
		mVisual->mChomp.emit();
	}
}

void FishVisualSuckOutroState::onRunFinished()
{
	// The flag is not set here, as the original leaves it
	if (!mChomped)
		mVisual->mChomp.emit();
	mVisual->resume();
}

float FishVisualSuckOutroState::getTimeToChomp()
{
	VisualData* aData = mVisual->mData;
	int anEat = aData->findAnimation(RUN_EAT);
	FishModelInfo* aModel = mVisual->mFishData->getFishInfo()->mFishModel;
	unsigned int aChompFrame = (unsigned int)(aModel->mEatFrame - aModel->mSuckFrame);
	unsigned int aFrame = (unsigned int)mVisual->getRunFrame();
	float aRate = (float)getRunRate(aData, anEat);
	if (aChompFrame < aFrame)
		return 0.0f;
	float aTime = (float)(toUnsignedDouble((int)(aChompFrame - aFrame)) / aRate);
	return (float)((double)aTime - mVisual->getFrameTimeShown());
}

FishVisual::FishVisual(Theme* theTheme, Fish* theFish)
    : mLastUpdateFrame(0), mTheme(theTheme), mFish(theFish), mIdleThreshold(IDLE_THRESHOLD),
      mSwimThreshold(SWIM_THRESHOLD), mTurnThreshold(TURN_THRESHOLD), mFastTurn(false), mHasIdle(false),
      mHasTurn(false), mVelocity(0.0f, 0.0f), mForward(0.0f, 0.0f), mSwimRateScale(0.0f), mSwimRateBase(0.0f),
      mTurnBias(0.0f), mScale(1.0f), mMirrorRotation(0.0f), mFacing(-1.0f), mOrientation(0.0f), mMirror(1.0f),
      mMirrorScale(0.0f, 0.0f)
{
	// The swim rate's scale and base (+0x148 and +0x14c) and the turn's bias (+0x150) are left as the allocation
	// had them; the fish sets the first two as it takes its behaviour (0x42aa30 (feedingfrenzy2)), and the port
	// starts all three at 0
	for (int i = 0; i < 4; i++)
		mEffectPoints[i] = Vector2f(0.0f, 0.0f);
	mIdleState = new FishVisualIdleState(this);
	mSwimState = new FishVisualSwimState(this);
	mTurnState = new FishVisualTurnState(this);
	mEatState = new FishVisualEatState(this);
	mPukeState = new FishVisualPukeState(this);
	mStunnedState = new FishVisualStunnedState(this);
	mSuckIntroState = new FishVisualSuckIntroState(this);
	mSuckOutroState = new FishVisualSuckOutroState(this);
	addOwnState(STATE_ANIMATED);
}

void FishVisual::setupFish()
{
	setup();
	mImage->mFinished.connect(&mStateMgr, &FishVisualStateMgr::onImageFinished);
	// 0x550828 (feedingfrenzy2): the theme's inked effect, in front
	mInkedEffect = mTheme->getParticleSystem("inkedFx");
	// The original adds what it found, null or not; the port adds an effect the theme has
	if (mInkedEffect)
		mFrontGroup->addChild(mInkedEffect);
}

void FishVisual::setFacing(float theFacing)
{
	mFacing = theFacing;
	mMirror = (float)((double)mOrientation * theFacing);
	double aBend = (double)mMirror - 1.0;
	float aForwardX = (float)fabs(mForward.x);
	mMirrorScale.x = (float)(aForwardX * aBend + 1.0);
	float aForwardY = (float)fabs(mForward.y);
	mMirrorScale.y = (float)(aForwardY * aBend + 1.0);
}

void FishVisual::resetEffects()
{
	mImage->setColorMode(0);
	mImage->setColor(1.0f, 1.0f, 1.0f);
}

void FishVisual::setFishData(FishVisualData* theData)
{
	setData(theData);
	mFishData = theData;
	FishModelInfo* aModel = theData->getFishInfo()->mFishModel;
	mForward = aModel->mForward;
	mOrientation = aModel->mOrientation;
	// The mirror's scale from the start's mirror value (+0x164), as a turn's end sets it
	double aMirror = (double)mMirror - 1.0;
	mMirrorScale.x = (float)((double)(float)fabs((double)mForward.x) * aMirror + 1.0);
	mMirrorScale.y = (float)((double)(float)fabs((double)mForward.y) * aMirror + 1.0);
	mHasIdle = 0 <= theData->findAnimation(RUN_IDLE);
	mHasTurn = 0 <= theData->findAnimation(RUN_TURN);
	for (int i = 0; i < 4; i++)
		mEffectPoints[i] = theData->getAnchor(EFFECT_POINTS[i]);
	setName(theData->getFishInfo()->mName);
}

void FishVisual::startSizePulse(float theFrom, float theTo, float theSeconds)
{
	mPulseCurve.clear();
	FloatKey aStart;
	aStart.mTime = 0.0f;
	aStart.mValue = theFrom;
	aStart.mInTime = 0.0f;
	aStart.mInSpeed = 1.0f;
	aStart.mOutTime = 0.0f;
	aStart.mOutSpeed = 1.0f;
	mPulseCurve.addKey(aStart);
	FloatKey aMiddle;
	aMiddle.mTime = PULSE_MIDDLE_TIME;
	aMiddle.mValue = (float)(((double)theTo - theFrom) * PULSE_OVERSHOOT + theFrom);
	aMiddle.mInTime = PULSE_MIDDLE_TIME;
	aMiddle.mInSpeed = 0.0f;
	aMiddle.mOutTime = PULSE_MIDDLE_OUT_TIME;
	aMiddle.mOutSpeed = 0.0f;
	mPulseCurve.addKey(aMiddle);
	FloatKey anEnd;
	anEnd.mTime = 1.0f;
	anEnd.mValue = theTo;
	anEnd.mInTime = PULSE_END_IN;
	anEnd.mInSpeed = PULSE_END_IN;
	anEnd.mOutTime = 0.0f;
	anEnd.mOutSpeed = 1.0f;
	mPulseCurve.addKey(anEnd);
	mPulseTime.start(theSeconds, 1.0f);
}

bool FishVisual::turnIfNeeded()
{
	float aDot = (float)dot(mVelocity, mForward);
	double anAlong = (double)mTurnBias + aDot;
	if (((double)mTurnThreshold < anAlong && mFacing < 0.0f) || (anAlong < -(double)mTurnThreshold && 0.0f < mFacing))
	{
		turn();
		return true;
	}
	return false;
}

void FishVisual::turn()
{
	if (mHasTurn)
	{
		changeState(mTurnState);
		return;
	}
	// 0x4f3fd2 (feedingfrenzy2)
	mFacing = (float)((double)mFacing * -1.0);
	setRotation((float)(-1.0 * (double)getRotation()));
	mMirror = (float)((double)mOrientation * mFacing);
	setMirrorScale();
}

void FishVisual::resume()
{
	if (mHasIdle && lengthSquared(mVelocity) < mIdleThreshold)
		changeState(mIdleState);
	else
		changeState(mSwimState);
}

void FishVisual::update(UpdateContext& theContext)
{
	if (mLastUpdateFrame < theContext.mFrame)
		mLastUpdateFrame = theContext.mFrame;
	else
		getEngine()->log("DOUBLE UPDATE\n");
	mStateMgr.update(&theContext);
	Container::update(theContext);
	double aMirror = (double)mOrientation * mFacing - 1.0;
	float aScaleX = (float)((double)(float)fabs((double)mForward.x) * aMirror + 1.0);
	float aScaleY = (float)((double)(float)fabs((double)mForward.y) * aMirror + 1.0);
	mMirroredGroup->setScale(aScaleX, aScaleY);
	mMirroredGroup->setRotation(mMirrorRotation);
	if (mPulseTime.getDirection() != TIME_STOPPED)
	{
		mPulseTime.advance(theContext.mElapsed);
		mScale = (float)mPulseCurve.getValue((float)mPulseTime.getValue());
	}
	setScale(mScale, mScale);
	// The fading component (+0x210, with its time at +0x1e8), the four effects' events and the attached components
	// (+0x448): not yet translated; nothing sets them on a menu
}

void FishVisual::clearParent()
{
	Visual::clearParent();
	mStateMgr.changeState(nullptr);
	// The fading component (+0x210) let go: not yet translated
}

int FishVisual::getRunFrame() const
{
	AnimationData* aData = mImage->getAnimationData();
	if (aData == nullptr || aData->getFrameCount() == 0)
		return 0;
	if ((unsigned int)mImage->getRun() >= (unsigned int)aData->getAnimationCount())
		return 0;
	return mImage->getFrameIndex() - (int)aData->getAnimation(mImage->getRun()).mStart;
}

double FishVisual::getRunProgress() const
{
	AnimationData* aData = mImage->getAnimationData();
	if (aData == nullptr || aData->getFrameCount() == 0)
		return 0.0;
	if ((unsigned int)mImage->getRun() >= (unsigned int)aData->getAnimationCount())
		return 0.0;
	const AnimationRun& aRun = aData->getAnimation(mImage->getRun());
	double aFrameDone = ((double)aRun.mFrameTime - mImage->getTimeLeft()) / aRun.mFrameTime;
	double aFrames = toUnsignedDouble(mImage->getFrameIndex() - (int)aRun.mStart) + aFrameDone;
	return aFrames / toUnsignedDouble((int)(aRun.mEnd - aRun.mStart));
}

double FishVisual::getFrameTimeShown() const
{
	// The original reads the run without checking it
	const AnimationRun& aRun = mImage->getAnimationData()->getAnimation(mImage->getRun());
	return (double)aRun.mFrameTime - mImage->getTimeLeft();
}

void FishVisual::setMirrorScale()
{
	double aMirror = (double)mMirror - 1.0;
	mMirrorScale.x = (float)((double)(float)fabs((double)mForward.x) * aMirror + 1.0);
	mMirrorScale.y = (float)((double)(float)fabs((double)mForward.y) * aMirror + 1.0);
}
