#include "Mine.h"
#include "AnimImagePivot.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "Random.h"
#include "Selector.h"
#include "StageMgr.h"
#include <cmath>
#include <string.h>

using namespace engine;

namespace feedingfrenzy
{

// The ranges a mine's bobbing rate is picked from, the second rate that nothing reads, and the seconds into the
// bobbing the mine starts at (twice: the second is read by nothing either)
const Point BOB_RATE_RANGE(1.2f, 2.7f);
const Point SWAY_RATE_RANGE(2.0f, 2.7f);
const Point BOB_START_RANGE(0.0f, 100.0f);
// The pictures of the mine: the end of the folder that has them, and their extension
const char* const MINE_FOLDER = "mine";
const char* const PICTURE_EXTENSION = ".jpg";
// The particle files of the fall and of the explosion
const char* const FALL_FX_FILE = "config\\FX\\mineFallFx.xml";
const char* const EXPLOSION_FX_FILE = "config\\FX\\mineExplosion.xml";
// The explosion's pictures: where they are, their rate, their size and where they stand from the mine, as a factor of
// the mine's width and height
const char* const EXPLOSION_PATH = "resources\\mine\\explode.";
const float EXPLOSION_RATE = 15.0f;
const float EXPLOSION_SCALE = 1.5f;
const float EXPLOSION_PLACE_FACTOR = -2.0f;
// The level a mine has in play: no fish's is as high
const float MINE_LEVEL = 9999.0f;
// The mine sinks to a depth that is at least this far from the top and from the bottom of the stage
const float DEPTH_MARGIN = 180.0f;
// How far the bobbing moves the mine's vertical wish, in pixels a second
const float BOB_REACH = 20.0f;
// The seconds more that each mine of the stage that leaves waits before it goes off
const float LEAVE_STEP_SECONDS = 0.35f;
// The shake of the camera the explosion makes: its seconds and its amount
const float SHAKE_SECONDS = 0.5f;
const float SHAKE_AMOUNT = 4.0f;
// The sound of the explosion, its volume and its pitch
const char* const EXPLODE_SOUND = "mineExplode";
const float EXPLODE_SOUND_VOLUME = 1.0f;
const float EXPLODE_SOUND_PITCH = 1.0f;
// A fish whose picture is drawn smaller than this does not set the mine off
const float MIN_TOUCH_SCALE = 0.4f;
// The fish a mine does not go off for: the bonus bubbles (by the start of their names) and the jellyfish
const char* const BONUS_BUBBLE_NAME = "BonusBubble";
const char* const JELLYFISH_NAME = "BasicJellyFish";

int Mine::sLeaveCount = 0;

// 0x409c30
Mine::Mine() :
    mExploding(false), mExplosionFx(nullptr), mFallFx(nullptr), mLeaveSeconds(0.0f), mDepth(0.0f)
{
	mBobRate = randomFloat(BOB_RATE_RANGE);
	mSwayRate = randomFloat(SWAY_RATE_RANGE);
	mSwayTime = randomFloat(BOB_START_RANGE);
	mBobTime = randomFloat(BOB_START_RANGE);
	mPoisonProof = true;
	mStunnable = true;
	mStings = true;
	mTargetable = false;
	mKind = FISH_KIND_HAZARD;
}

// 0x409760
void Mine::update(UpdateInfo& theInfo)
{
	if (mExploding)
	{
		updateExploding(theInfo);
		return;
	}
	mBobTime = theInfo.mSeconds + mBobTime;
	mSwayTime = theInfo.mSeconds + mSwayTime;
	AIFish::update(theInfo);
}

// 0x40a020
void Mine::loadAnimations(const char* theMinePath, const char*, const char*, const char*)
{
	mAnimations = new Selector();
	addChild(mAnimations);

	Ref<AnimImage> aMine(new AnimImage());
	aMine->load(theMinePath, PICTURE_EXTENSION, 0);
	mAnimations->addChild(aMine);
	mAnimations->select(0, 0.0f, true);
	for (int aPicture = 0; aPicture < mAnimations->getCount(); aPicture++)
	{
		mAnimations->select(aPicture, 0.0f, true);
		setAnimationPivot(static_cast<AnimImage*>(mAnimations->getSelected()), 1);
	}
	mAnimations->select(0, 0.0f, true);
	setScale(mSize, mSize);
	updateRects();
	setFlags(NODE_DRAWABLE | NODE_LISTENERS);

	mFallFx = new ParticleSystem();
	addChild(mFallFx);
	mFallFx->load(FALL_FX_FILE);

	mExplosionFx = new ParticleSystem();
	addChild(mExplosionFx);
	mExplosionFx->load(EXPLOSION_FX_FILE);
}

// 0x40a320
void Mine::loadAnimationsOf(const std::string& theFolder)
{
	std::string aPath = theFolder + MINE_FOLDER;
	loadAnimations(aPath.c_str(), nullptr, nullptr, nullptr);
}

// 0x409630
void Mine::loadEffects()
{
	mExplosion = new AnimImage();
	mExplosion->setVisible(false);
	mExplosion->setScale(EXPLOSION_SCALE, EXPLOSION_SCALE);
	mExplosion->setLoop(false);
	mExplosion->setRate(EXPLOSION_RATE);
	mExplosion->load(EXPLOSION_PATH, PICTURE_EXTENSION, 1);
	addChild(mExplosion);
	float aPlaceY = mBounds.height() * EXPLOSION_PLACE_FACTOR;
	float aPlaceX = mBounds.width() * EXPLOSION_PLACE_FACTOR;
	mExplosion->setPosition(aPlaceX, aPlaceY);
}

// 0x409870
void Mine::putInPlay()
{
	mAnimations->select(0, 0.0f, true);
	AIFish::putInPlay();
	float aWorldHeight = (float)FishFood::instance()->getStageMgr()->getWorldHeight();
	mDepth = randomFloat(Point(DEPTH_MARGIN, aWorldHeight - DEPTH_MARGIN));
	mStings = true;
	mFallFx->startEmitting();
	mLevel = MINE_LEVEL;
}

// 0x409b10
void Mine::stun(float, FishBase* theSource, bool)
{
	if (isInPlay() && isInStage())
	{
		if (theSource == FishFood::instance()->getStageMgr()->getPlayer())
			explode();
	}
}

// 0x409e80
void Mine::onContact(FishBase* theFish)
{
	if (!isInPlay() || !isInStage())
		return;
	if (fabsf(theFish->mScale.x) < MIN_TOUCH_SCALE)
		return;
	if (theFish->getTypeName().compare(0, strlen(BONUS_BUBBLE_NAME), BONUS_BUBBLE_NAME) == 0)
		return;
	if (theFish->getTypeName() == getTypeName())
		return;
	if (theFish->getTypeName() == JELLYFISH_NAME)
		return;
	if (theFish->canBeEatenBy(this))
		theFish->finishBeingEaten(this);
	explode();
}

// 0x409d90
void Mine::updatePlacedHitRect()
{
	mScaledHitRect.set(mHitRect.getRect());
	mScaledHitRect.scale(mSize, mSize);
	const Rect& aHit = mScaledHitRect.getRect();
	mPlacedHitRect.set(mPosition.x - aHit.right, aHit.top + mPosition.y, mPosition.x - aHit.left,
	                   aHit.bottom + mPosition.y);
}

// 0x409960
void Mine::updatePlacedRects()
{
	const Rect& aHit = mPlacedHitRect.getRect();
	float aHalfWidth = (aHit.right - aHit.left) * 0.5f;
	mNose.x = getX() + aHalfWidth;
	float aHalfHeight = (aHit.bottom - aHit.top) * 0.5f;
	mNose.y = getY() + aHalfHeight;
	mTail.x = mNose.x;
	mTail.y = mNose.y;
	mPlacedBodyRect.left = mBodyRect.left + mNose.x;
	mPlacedBodyRect.right = mBodyRect.right + mNose.x;
	mPlacedBodyRect.top = mBodyRect.top + mNose.y;
	mPlacedBodyRect.bottom = mBodyRect.bottom + mNose.y;
}

// 0x419550
void Mine::updateAnimation()
{
	mAnimations->select(0, 0.0f, true);
}

// 0x4097a0
void Mine::updateEnter(UpdateInfo& theInfo)
{
	if (isSwallowed() || mDepth <= getY())
		beginSpecial();
	else
		mDesiredVelocity.y = mMaxSpeedY;
	FishBase::update(theInfo);
}

// 0x409800
void Mine::updateSpecial(UpdateInfo& theInfo)
{
	if (!isInStage())
	{
		mInPlay = false;
		mExploding = false;
		return;
	}
	bob();
	FishBase::update(theInfo);
}

// 0x409b90
void Mine::updateLeaving(UpdateInfo& theInfo)
{
	if (0.0f < mLeaveSeconds)
		mLeaveSeconds = mLeaveSeconds - theInfo.mSeconds;
	if (mLeaveSeconds <= 0.0f)
		explode();
}

// 0x409850
void Mine::beginSpecial()
{
	mDesiredVelocity.y = 0.0f;
	mState = AI_SPECIAL;
	mFallFx->stopEmitting();
}

// 0x409b50
void Mine::beginLeave()
{
	if (isInPlay())
	{
		mState = AI_LEAVE;
		mStings = false;
		mLeaveSeconds = (float)sLeaveCount * LEAVE_STEP_SECONDS;
		sLeaveCount++;
	}
}

// 0x4098f0
void Mine::updateExploding(UpdateInfo& theInfo)
{
	if (mExplosionFx->isDone())
	{
		if (!mExplosion->isPlaying())
		{
			mInPlay = false;
			mExploding = false;
			mExplosionFx->stopEmitting();
		}
	}
	if (!mExplosion->isPlaying())
		mExplosion->setVisible(false);
	Container::update(theInfo);
}

// 0x40a260
void Mine::explode()
{
	mStings = false;
	FishFood::instance()->getStageMgr()->shakeCamera(SHAKE_SECONDS, SHAKE_AMOUNT);
	mFallFx->stopEmitting();
	mExplosionFx->startEmitting();
	mExplosion->setFrame(0);
	mExplosion->setVisible(true);
	mExplosion->play();
	mAnimations->select(-1, 0.0f, true);
	FishFood::instance()->playSound(EXPLODE_SOUND, EXPLODE_SOUND_VOLUME, EXPLODE_SOUND_PITCH);
	mLevel = 0.0f;
	mExploding = true;
}

// 0x409d60 (the original takes a first argument, always 1, that it does not read, and the update of the entering mine
// calls it with its flag off, which does nothing)
void Mine::bob()
{
	mDesiredVelocity.y = (float)(sin((double)mBobTime * (double)mBobRate) * BOB_REACH);
}

} // namespace feedingfrenzy
