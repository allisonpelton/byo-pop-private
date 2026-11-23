#include "FishBase.h"
#include "FishFood.h"
#include "FishSchool.h"
#include "PlayerFish.h"
#include "StageMgr.h"
#include <cmath>
#include <cstdio>

using namespace engine;

namespace feedingfrenzy
{

// How far the horizontal speed must be before the fish turns, and the speeds a fish counts as moving at
const float TURN_SPEED = 15.0f;
const float FACE_SPEED = 10.0f;
const float MOVING_SPEED_SQUARED = 10.0f;
// The smallest speed along an axis that a fish keeps; slower is a stop
const float STOP_SPEED = 3.0f;
// The animation rates (pictures a second)
const float PLAYER_TURN_RATE = 20.0f;
const float FISH_TURN_RATE = 10.0f;
const float EAT_RATE = 15.0f;
const float EXTRA_RATE = 8.0f;
const float IDLE_RATE = 7.0f;
const float LEAST_SWIM_RATE = 7.5f;
const float SWIM_RATE_PER_SPEED = 15.0f;
// The player's turning animation starts on this frame
const int PLAYER_TURN_FIRST_FRAME = 3;
// A fish that is eaten shows no more than this fraction of its size
const float EATEN_SHRINK_LIMIT = 0.2f;

// 0x408520
FishBase::FishBase()
    : mAnimations(nullptr), mHasTurnAnimation(false), mHasEatAnimation(false), mHasExtraAnimation(false),
      mExtraAnimationIndex(0), mHuge(false), mAnimState(ANIM_IDLE), mEatFinished(false), mEatCount(0),
      mPreviousVelocityX(0.0f), mMaxSpeedX(100.0f), mMaxSpeedY(80.0f), mMinSpeedX(0.0f), mMinSpeedY(0.0f),
      mSpeedUp(500.0f), mSlowDown(80.0f), mSize(1.0f), mPoints(0.0f), mLevel(0.0f), mInPlay(false),
      mVisionRange(0.0f), mVisionRangeSquared(0.0f), mVisionCosine(0.0f), mTypeName("FishBase"),
      mEatenSoundPitch(1.0f), mEater(nullptr), mBeingEaten(false), mBeingEatenSeconds(0.0f), mBeingEatenLeft(0.0f),
      mStings(false), mPickup(false), mEaten(false), mSchool(nullptr), mPoisonProof(true), mStunnable(true),
      mTargetable(true), mReactable(false), mSwallowed(false), mMarked(false), mColorShift(nullptr),
      mEatFrame(5), mPngFrames(false), mCountsAsFood(true), mEdibleKind(EDIBLE_FISH), mUnsafeEatFirstFrame(0),
      mUnsafeEatLastFrame(100), mHidden(false), mKind(FISH_KIND_FISH)
{
	mThought.mScale = 1.0f;
}

// 0x407b10
FishBase::~FishBase() {}

// 0x407020
void FishBase::setPosition(const Point& thePosition)
{
	Container::setPosition(thePosition);
	updatePlacedHitRect();
	updatePlacedRects();
}

// 0x407050
void FishBase::setPosition(float theX, float theY)
{
	Container::setPosition(theX, theY);
	updatePlacedHitRect();
	updatePlacedRects();
}

// 0x406420
void FishBase::update(UpdateInfo& theInfo)
{
	if (mInPlay)
	{
		move(theInfo.mSeconds);
		updateAnimation();
		updatePlacedRects();
		updatePlacedHitRect();
		Container::update(theInfo);
	}
}

// 0x4076b0
void FishBase::loadAnimations(const char* theSwimPath, const char* theTurnPath, const char* theEatPath,
                              const char* theExtraPath)
{
	const char* anExtension = mPngFrames ? ".png" : ".jpg";

	mAnimations = new Selector();
	addChild(mAnimations);

	Ref<FishAnimImage> aSwim(new FishAnimImage());
	aSwim->loadShifted(theSwimPath, anExtension, 1, mColorShift);
	mAnimations->addChild(aSwim);
	mAnimations->select(0, 0.0f, true);

	// An animation with no frame is dropped, which leaves the fish without it
	mHasTurnAnimation = false;
	if (theTurnPath != nullptr)
	{
		Ref<FishAnimImage> aTurn(new FishAnimImage());
		aTurn->loadShifted(theTurnPath, anExtension, 1, mColorShift);
		if (aTurn->getCount() > 0)
		{
			mHasTurnAnimation = true;
			mAnimations->addChild(aTurn);
		}
	}

	mHasEatAnimation = false;
	if (theEatPath != nullptr)
	{
		Ref<FishAnimImage> anEat(new FishAnimImage());
		anEat->loadShifted(theEatPath, anExtension, 1, mColorShift);
		if (anEat->getCount() > 0)
		{
			mHasEatAnimation = true;
			mAnimations->addChild(anEat);
		}
	}

	mHasExtraAnimation = false;
	if (theExtraPath != nullptr)
	{
		Ref<FishAnimImage> anExtra(new FishAnimImage());
		anExtra->loadShifted(theExtraPath, anExtension, 1, mColorShift);
		if (anExtra->getCount() > 0)
		{
			mHasExtraAnimation = true;
			mAnimations->addChild(anExtra);
			mExtraAnimationIndex = mAnimations->getCount() - 1;
		}
	}

	setScale(mSize, mSize);
	updateRects();
	mAnimState = ANIM_IDLE;
	mPreviousVelocityX = 0.0f;
	setFlags(NODE_DRAWABLE | NODE_LISTENERS);
}

// 0x4083c0
void FishBase::loadAnimationsOf(const std::string& theFolder)
{
	std::string aSwim = theFolder + "swim\\swim_cycle.";
	std::string aTurn = theFolder + "turn\\turn_cycle.";
	std::string anEat = theFolder + "eat\\eat_cycle.";
	loadAnimations(aSwim.c_str(), aTurn.c_str(), anEat.c_str(), nullptr);
}

// 0x406180
void FishBase::setSpeeds(float theMaxSpeedX, float theMaxSpeedY, float theMinSpeedX, float theMinSpeedY,
                         float theSpeedUp, float theSlowDown)
{
	mMaxSpeedX = theMaxSpeedX;
	mMaxSpeedY = theMaxSpeedY;
	mMinSpeedX = theMinSpeedX;
	mMinSpeedY = theMinSpeedY;
	mSpeedUp = theSpeedUp;
	mSlowDown = theSlowDown;
}

// 0x4061f0
void FishBase::setVision(float theRange, float theAngle)
{
	mVisionRange = theRange;
	mVisionRangeSquared = theRange * theRange;
	mVisionCosine = (float)cos((double)theAngle * 3.141592653589793 * 0.005555555555555556);
}

// 0x408220
void FishBase::setEatenSound(const std::string& theName, float thePitch)
{
	mEatenSoundName = theName;
	mEatenSoundPitch = thePitch;
}

// 0x406e20
void FishBase::setBodyRect(float theLeft, float theTop, float theRight, float theBottom)
{
	mBodyRect.set(theLeft, theTop, theRight, theBottom);
}

// 0x406e50
void FishBase::setMouthRect(const CollisionRect& theRect)
{
	mMouthRect.set(theRect.getRect());
}

// 0x406290
void FishBase::setHitRect(const CollisionRect& theRect)
{
	mHitRect.set(theRect.getRect());
	updatePlacedHitRect();
}

// 0x4067b0
bool FishBase::canEat(FishBase*)
{
	return mAnimState != ANIM_TURN_RIGHT && mAnimState != ANIM_TURN_LEFT && isVisible() && !mBeingEaten;
}

// 0x406db0 (it ignores both arguments)
void FishBase::startBeingEaten(FishBase*, float)
{
	mBeingEaten = true;
	mEatenFrom = mPosition;
	mEatenCentreOffset = getCentre();
	mEatenCentreOffset.subtract(mEatenFrom);
}

// 0x407e60
void FishBase::playEatAnimation()
{
	if (!mHasEatAnimation)
		return;

	if (mAnimState == ANIM_TURN_LEFT || mAnimState == ANIM_TURN_RIGHT)
	{
		int aFrame = static_cast<AnimImage*>(mAnimations->getSelected())->getFrame();
		bool aKeepTurning = false;
		if (aFrame < mUnsafeEatFirstFrame)
			aKeepTurning = aFrame < mUnsafeEatLastFrame;
		else if (aFrame <= mUnsafeEatLastFrame)
		{
			// the turn is in the frames that may not be eaten in: the eating is given up
			mEatFinished = false;
			finishEating();
			return;
		}

		if (!aKeepTurning)
			setScale((mAnimState == ANIM_TURN_RIGHT ? -1.0f : 1.0f) * mSize, mSize);
	}

	mAnimState = ANIM_EAT;
	mAnimations->select(mHasTurnAnimation ? 2 : 1, 0.0f, true);
	AnimImage* anEat = static_cast<AnimImage*>(mAnimations->getSelected());
	mEatFinished = false;
	anEat->setFrame(0);
	anEat->setRate(EAT_RATE);
	anEat->play();
	anEat->setLoop(false);
}

// 0x407d70
void FishBase::takeOutOfPlay()
{
	mInPlay = false;
	if (mSchool != nullptr)
		mSchool->removeFish(this);
	mAnimations->select(-1, 0.0f, true);
}

// 0x407100
Point FishBase::getCentre()
{
	Point aCentre = getNosePosition();
	Point aTail = getTailPosition();
	aCentre.add(aTail);
	aCentre.scale(0.5f);
	return aCentre;
}

// 0x406230
void FishBase::putInPlay()
{
	mSchool = nullptr;
	mBeingEaten = false;
	mInPlay = true;
	mAnimState = ANIM_SWIM;
	mAnimations->select(0, 0.0f, true);
	setScale(mSize, mSize);
}

// 0x406d60
bool FishBase::isOnScreen()
{
	return FishFood::instance()->getStageMgr()->isFishOnScreen(this);
}

// 0x406d80
bool FishBase::isInStage()
{
	return FishFood::instance()->getStageMgr()->isFishInStage(this);
}

// 0x406ef0
void FishBase::addPush(float theX, float theY)
{
	mPush.x = theX + mPush.x;
	mPush.y = theY + mPush.y;
}

// 0x406f50
void FishBase::addPushAgainst(const Point& thePush)
{
	mPushAgainst.add(thePush);
}

// 0x408290
void FishBase::setThoughtBubble(const ThoughtBubbleInfo& theInfo)
{
	mThought = theInfo;
}

// 0x4071b0
void FishBase::setUnsafeEatFrames(int theFirst, int theLast)
{
	mUnsafeEatFirstFrame = theFirst;
	mUnsafeEatLastFrame = theLast;
}

// 0x407260
void FishBase::updateRects()
{
	Node* aPicture = mAnimations->getSelected();
	aPicture->layout();
	mBounds = *aPicture->getBounds();
	float aWidth = mBounds.width();
	float aHeight = mBounds.height();
	mHitRect.set(0.0f, 0.0f, aWidth, aHeight);
	mBounds.left = mSize * mBounds.left;
	mBounds.right = mSize * mBounds.right;
	mBounds.top = mSize * mBounds.top;
	mBounds.bottom = mSize * mBounds.bottom;
	mScaledHitRect.set(mHitRect.getRect());
	mScaledHitRect.scale(mSize, mSize);
	mPlacedHitRect.set(mScaledHitRect.getRect());
	mPlacedHitRect.offset(mBounds.left, mBounds.top);
}

// 0x4062c0
void FishBase::updatePlacedHitRect()
{
	mScaledHitRect.set(mHitRect.getRect());
	mScaledHitRect.scale(mSize, mSize);
	layout();
	mBounds.normalize();
	const Rect& aHit = mScaledHitRect.getRect();
	if (mScale.x < 0.0f)
		mPlacedHitRect.set(mBounds.right - aHit.right, aHit.top + mBounds.top, mBounds.right - aHit.left,
		                   aHit.bottom + mBounds.top);
	else
		mPlacedHitRect.set(aHit.left + mBounds.left, aHit.top + mBounds.top, aHit.right + mBounds.left,
		                   aHit.bottom + mBounds.top);
}

// 0x4067f0
void FishBase::updatePlacedRects()
{
	float aLeft = mBounds.left;
	float aWidth = mBounds.right - mBounds.left;
	float aMiddle = mBounds.top + (mBounds.bottom - mBounds.top) * 0.5f;
	mNose.y = aMiddle;
	mTail.y = aMiddle;

	// The nose and the tail: at the two ends of the bounds, or, while the fish turns, the share of the turn that has
	// gone
	if (mAnimState == ANIM_TURN_LEFT || mAnimState == ANIM_TURN_RIGHT)
	{
		AnimImage* aTurn = static_cast<AnimImage*>(mAnimations->getSelected());
		float aShare = ((float)aTurn->getFrame() / (float)aTurn->getCount()) * aWidth;
		if (mAnimState == ANIM_TURN_LEFT)
		{
			mNose.x = (aWidth + aLeft) - aShare;
			mTail.x = aShare + aLeft;
		}
		else
		{
			mNose.x = aLeft + aShare;
			mTail.x = (aWidth + aLeft) - aShare;
		}
	}
	else if (mScale.x < 0.0f)
	{
		mNose.x = aLeft + aWidth;
		mTail.x = aLeft;
	}
	else
	{
		mNose.x = aLeft;
		mTail.x = aLeft + aWidth;
	}

	Rect aBody = mBodyRect;
	aBody.scale(mSize, mSize);
	const Rect& aMouth = mMouthRect.getRect();
	if (mScale.x < 0.0f)
	{
		mPlacedBodyRect.right = mNose.x - aBody.left;
		mPlacedBodyRect.left = mNose.x - aBody.right;
		mPlacedBodyRect.bottom = aBody.bottom + mNose.y;
		mPlacedBodyRect.top = aBody.top + mNose.y;
		mPlacedMouthRect.set(mTail.x - aMouth.right, aMouth.top + mTail.y, mTail.x - aMouth.left,
		                     aMouth.bottom + mTail.y);
	}
	else
	{
		mPlacedBodyRect.left = aBody.left + mNose.x;
		mPlacedBodyRect.right = aBody.right + mNose.x;
		mPlacedBodyRect.top = aBody.top + mNose.y;
		mPlacedBodyRect.bottom = aBody.bottom + mNose.y;
		mPlacedMouthRect.set(aMouth.left + mTail.x, aMouth.top + mTail.y, aMouth.right + mTail.x,
		                     aMouth.bottom + mTail.y);
	}
}

// 0x4074d0
void FishBase::move(float theSeconds)
{
	if (mDesiredVelocity.x == 0.0f)
	{
		float aChange = (mDesiredVelocity.x <= mVelocity.x ? -1.0f : 1.0f) * mSlowDown * theSeconds;
		if (fabsf(aChange) <= fabsf(mVelocity.x))
			mVelocity.x = aChange + mVelocity.x;
		else
			mVelocity.x = 0.0f;
	}
	else
	{
		float aChange = (mDesiredVelocity.x <= mVelocity.x ? -1.0f : 1.0f) * mSpeedUp * theSeconds;
		mVelocity.x = aChange + mVelocity.x;
	}

	if (mDesiredVelocity.y == 0.0f)
	{
		float aChange = (mDesiredVelocity.y <= mVelocity.y ? -1.0f : 1.0f) * mSlowDown * theSeconds;
		if (fabsf(mVelocity.y) < fabsf(aChange))
			mVelocity.y = 0.0f;
		else
			mVelocity.y = aChange + mVelocity.y;
	}
	else
	{
		float aChange = (mDesiredVelocity.y <= mVelocity.y ? -1.0f : 1.0f) * mSpeedUp * theSeconds;
		mVelocity.y = aChange + mVelocity.y;
	}

	if (fabsf(mVelocity.x) < STOP_SPEED)
		mVelocity.x = 0.0f;
	if (fabsf(mVelocity.y) < STOP_SPEED)
		mVelocity.y = 0.0f;

	mVelocity.x = mPushAgainst.x + mPush.x + mVelocity.x;
	mVelocity.y = mPushAgainst.y + mPush.y + mVelocity.y;
	float aX = getX();
	float aY = getY();
	setPosition(theSeconds * mVelocity.x + aX, theSeconds * mVelocity.y + aY);
}

// 0x406470
void FishBase::updateAnimation()
{
	if (mHasTurnAnimation)
	{
		if (mAnimState == ANIM_IDLE || mAnimState == ANIM_SWIM)
		{
			// the fish turns when it moves faster than TURN_SPEED against the way it faces
			mAnimState = ANIM_IDLE;
			if (mVelocity.x > TURN_SPEED && mScale.x > 0.0f)
				mAnimState = ANIM_TURN_RIGHT;
			else if (mVelocity.x < -TURN_SPEED && mScale.x < 0.0f)
				mAnimState = ANIM_TURN_LEFT;
			else
				mAnimState = mVelocity.lengthSquared() <= MOVING_SPEED_SQUARED ? ANIM_IDLE : ANIM_SWIM;

			if (mAnimState == ANIM_TURN_RIGHT || mAnimState == ANIM_TURN_LEFT)
			{
				mAnimations->select(1, 0.0f, true);
				AnimImage* aTurn = static_cast<AnimImage*>(mAnimations->getSelected());
				if (this == FishFood::instance()->getStageMgr()->getPlayer())
				{
					aTurn->setFrame(PLAYER_TURN_FIRST_FRAME);
					aTurn->setRate(PLAYER_TURN_RATE);
				}
				else
				{
					aTurn->setFrame(0);
					aTurn->setRate(FISH_TURN_RATE);
				}
				aTurn->play();
				aTurn->setLoop(false);
			}
		}
	}

	if (!mHasTurnAnimation || mAnimState == ANIM_EAT)
	{
		// without a turning animation the picture is turned over to face the way the fish moves
		float aScaleX = mSize;
		if (-FACE_SPEED <= mVelocity.x)
			aScaleX = -mSize;
		setScale(aScaleX, mSize);
	}

	switch (mAnimState)
	{
	case ANIM_IDLE:
		if (mHasExtraAnimation)
			showExtraAnimation();
		else
			updateSwim();
		break;
	case ANIM_TURN_LEFT:
	case ANIM_TURN_RIGHT:
		updateTurn();
		break;
	case ANIM_EAT:
		updateEat();
		break;
	default:
		updateSwim();
		break;
	}
	mPreviousVelocityX = mVelocity.x;
}

// 0x4066a0
void FishBase::updateTurn()
{
	AnimImage* aTurn = static_cast<AnimImage*>(mAnimations->getSelected());
	if (!aTurn->isPlaying())
	{
		// the turn is done: the fish faces the other way
		float aDirection = mAnimState == ANIM_TURN_RIGHT ? -1.0f : 1.0f;
		mAnimations->select(0, 0.0f, true);
		setScale(aDirection * mSize, mSize);
		mAnimState = ANIM_SWIM;
	}
}

// 0x407380
void FishBase::updateSwim()
{
	mAnimations->select(0, 0.0f, true);
	AnimImage* aSwim = static_cast<AnimImage*>(mAnimations->getSelected());
	if (aSwim == nullptr)
		return;

	float aDesiredSpeed = mDesiredVelocity.length();
	float aSpeed = mVelocity.length();
	float aBlend = fabsf(aDesiredSpeed * 0.25f + aSpeed * 0.75f);
	if ((aDesiredSpeed != 0.0f || FACE_SPEED <= aBlend) && aBlend != 0.0f)
	{
		float aRate = (aBlend / mMaxSpeedX) * SWIM_RATE_PER_SPEED;
		if (aRate < LEAST_SWIM_RATE)
			aRate = LEAST_SWIM_RATE;
		aSwim->play();
		aSwim->setRate(aRate);
		return;
	}

	if (mHasExtraAnimation)
	{
		showExtraAnimation();
		return;
	}

	// at rest: the animation runs on to its first frame and stops there
	if (aSwim->mCurrent == 0)
		aSwim->stop();
	else
		aSwim->setRate(IDLE_RATE);
}

// 0x406ea0
void FishBase::showExtraAnimation()
{
	mAnimations->select(mExtraAnimationIndex, 0.0f, true);
	AnimImage* anExtra = static_cast<AnimImage*>(mAnimations->getSelected());
	if (anExtra != nullptr)
	{
		anExtra->play();
		anExtra->setRate(EXTRA_RATE);
	}
}

// 0x406720
void FishBase::updateEat()
{
	AnimImage* anEat = static_cast<AnimImage*>(mAnimations->getSelected());
	if (!mEatFinished)
	{
		if (mEatFrame <= anEat->getFrame())
		{
			mEatFinished = true;
			finishEating();
		}
	}

	if (!anEat->isPlaying())
	{
		mAnimations->select(0, 0.0f, true);
		mAnimState = ANIM_SWIM;
		if (!mEatFinished)
		{
			mEatFinished = true;
			finishEating();
		}
	}
}

// 0x406b00
void FishBase::updateBeingEaten(UpdateInfo& theInfo)
{
	mBeingEatenLeft = mBeingEatenLeft - theInfo.mSeconds;
	if (mBeingEatenLeft < 0.0f)
		mBeingEatenLeft = 0.0f;

	if (mEater == nullptr)
	{
		setVisible(false);
		return;
	}

	float aShare = mBeingEatenLeft / mBeingEatenSeconds;
	const Rect* aMouth = mEater->getBodyRect();
	Point aTarget((aMouth->left + aMouth->right) * 0.5f, (aMouth->top + aMouth->bottom) * 0.5f);
	Point anOffset = mEatenCentreOffset;
	anOffset.scale(aShare);
	aTarget.add(anOffset);
	Point aPosition = aTarget;
	aPosition.subtract(mEatenFrom);
	aPosition.scale(1.0f - aShare);
	aPosition.add(mEatenFrom);
	setPosition(aPosition);

	if (aShare < EATEN_SHRINK_LIMIT)
		aShare = 0.0f;
	aShare = aShare * mSize;
	setScale(mScale.x < 0.0f ? -aShare : aShare, aShare);
	updatePlacedRects();
	updatePlacedHitRect();
	Container::update(theInfo);
}

} // namespace feedingfrenzy
