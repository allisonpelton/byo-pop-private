#include "JohnDoryPlayer.h"
#include "App.h"
#include "CollisionRect.h"
#include "FishFood.h"
#include "Sound.h"
#include "StageMgr.h"

#include <algorithm>
#include <math.h>

using namespace engine;

namespace feedingfrenzy
{

// The charge: how much a full one lasts, how much it must have for the power to begin, and the share of the frame's
// seconds that comes back to it while the power is not used
const float SUCK_FULL_SECONDS = 0.6f;
const float SUCK_MIN_SECONDS = 0.3f;
const float SUCK_RECHARGE_SHARE = 0.125f;
// The charge as a share of what shows on the meter (1 divided by 0.6)
const float SUCK_PROGRESS_FACTOR = 1.6666666f;
// The mouth's picture plays at this rate, and the opening is its pictures 0 to this one
const float SUCK_ANIMATION_RATE = 15.0f;
const int SUCK_OPENING_LAST_FRAME = 2;
// How far in front of the nose the pull reaches, and how far above and below it
const float PULL_REACH = 150.0f;
const float PULL_HALF_HEIGHT = 60.0f;
// The seconds a fish takes to be eaten, and how hard a held fish is pushed to the mouth
const float EATEN_SECONDS = 0.4f;
const float PULL_STRENGTH = -25.0f;
// The water that is drawn in sits this far from the nose along the fish
const float FX_OFFSET = 10.0f;
// The seconds a poisoned fish's shake takes off the poison
const float POISON_SHAKE_SECONDS = 0.6f;
// The files, and the sound's place in the sound system
const char* const SUCK_FX_LEFT_FILE = "config\\fx\\johnDorySuckFxRL.xml";
const char* const SUCK_FX_RIGHT_FILE = "config\\fx\\johnDorySuckFxLR.xml";
const char* const SUCK_SOUND_NAME = "jdSuck";
const char* const SUCK_SOUND_FILE = "resources/sounds/jdSuckSound.wav";
const int SUCK_SOUND_CATEGORY = 0;
// The name the stage's files and the fish's reactions know the player's fish by
const char* const PLAYER_KIND_TYPE_NAME = "PlayerFish";

// 0x40eeb0
JohnDoryPlayer::JohnDoryPlayer() :
    mSavedState(PLAYER_SWIM), mSuckQueued(false), mSuckPhase(SUCK_OPENING), mSuckAnimation(nullptr),
    mSuckSeconds(SUCK_FULL_SECONDS), mSuckingRight(false), mSuckSound(nullptr)
{
}

// 0x40ec10
JohnDoryPlayer::~JohnDoryPlayer()
{
	if (mSuckSound != nullptr)
		mSuckSound->close();
}

// 0x40eac0
void JohnDoryPlayer::update(UpdateInfo& theInfo)
{
	PlayerFish::update(theInfo);
	mSuckFxLeft->setPosition((mNose.x - mPosition.x) + FX_OFFSET, mNose.y - mPosition.y);
	mSuckFxLeft->setScale(fabsf(1.0f / mSize), fabsf(1.0f / mSize));
	mSuckFxRight->setPosition((mNose.x - mPosition.x) - FX_OFFSET, mNose.y - mPosition.y);
	mSuckFxRight->setScale(fabsf(1.0f / mSize), fabsf(1.0f / mSize));
	if (mState != PLAYER_SPECIAL)
	{
		mSuckSeconds = theInfo.mSeconds * SUCK_RECHARGE_SHARE + mSuckSeconds;
		if (SUCK_FULL_SECONDS < mSuckSeconds)
			mSuckSeconds = SUCK_FULL_SECONDS;
	}
	if (mSuckQueued && (mAnimState == ANIM_IDLE || mAnimState == ANIM_SWIM))
		useSpecial();
}

// 0x40ed20
void JohnDoryPlayer::loadAnimationsOf(const std::string& theFolder)
{
	mSuckFxLeft = new ParticleSystem();
	addChild(mSuckFxLeft);
	mSuckFxLeft->load(SUCK_FX_LEFT_FILE);
	mSuckFxRight = new ParticleSystem();
	addChild(mSuckFxRight);
	mSuckFxRight->load(SUCK_FX_RIGHT_FILE);

	PlayerFish::loadAnimationsOf(theFolder);
	mAnimations->select((mHasTurnAnimation ? 1 : 0) + 1, 0.0f, true);
	mSuckAnimation = static_cast<AnimImage*>(mAnimations->getSelected());
	mAnimations->select(0, 0.0f, true);
}

// 0x40e9e0
void JohnDoryPlayer::loadEffects()
{
	PlayerFish::loadEffects();
	SoundSystem* aSounds = App::getSoundSystem();
	if (aSounds != nullptr)
	{
		aSounds->load(SUCK_SOUND_NAME, SUCK_SOUND_FILE);
		mSuckSound = aSounds->createSound(SUCK_SOUND_NAME, false, SUCK_SOUND_CATEGORY);
	}
	else
		mSuckSound = nullptr;
}

// 0x40e970
void JohnDoryPlayer::startBeingEaten(FishBase* theEater, float theSeconds)
{
	if (mState == PLAYER_SPECIAL)
	{
		mSuckFxLeft->stopEmitting();
		mSuckFxRight->stopEmitting();
		mSuckPhase = SUCK_DONE;
		finishEating();
		releasePulled();
	}
	PlayerFish::startBeingEaten(theEater, theSeconds);
}

// 0x40f820
std::string JohnDoryPlayer::getTypeName()
{
	return PLAYER_KIND_TYPE_NAME;
}

// 0x40f3c0
void JohnDoryPlayer::updateAnimation()
{
	if (mState != PLAYER_SPECIAL)
		FishBase::updateAnimation();
}

// 0x40ea70
void JohnDoryPlayer::rightButtonDown()
{
	if (mPoisonLeft <= 0.0f)
	{
		if (mDashCooldown < 0.0f)
			useSpecial();
	}
	else
		mPoisonLeft = mPoisonLeft - POISON_SHAKE_SECONDS;
}

// 0x40e850
void JohnDoryPlayer::updateSpecial(UpdateInfo& theInfo)
{
	switch (mSuckPhase)
	{
	case SUCK_OPENING:
		if (!mSuckAnimation->isPlaying())
		{
			mSuckPhase = SUCK_PULLING;
			updateSwimming(theInfo);
			return;
		}
		break;
	case SUCK_PULLING:
		mSuckSeconds = mSuckSeconds - theInfo.mSeconds;
		if (0.0f <= mSuckSeconds)
		{
			pullFish();
			updateSwimming(theInfo);
			return;
		}
		endPull();
		updateSwimming(theInfo);
		return;
	case SUCK_CLOSING:
		if (!mSuckAnimation->isPlaying())
		{
			mState = mSavedState;
			mSuckPhase = SUCK_DONE;
			finishEating();
			releasePulled();
			mSuckQueued = false;
			updateSwimming(theInfo);
			return;
		}
		// the mouth is still closing: the button's release is repeated, which restarts the closing picture
		releaseSpecial();
		break;
	case SUCK_DONE:
		releaseSpecial();
		break;
	}
	updateSwimming(theInfo);
}

// 0x40e6c0
void JohnDoryPlayer::useSpecial()
{
	if ((mAnimState == ANIM_SWIM || mAnimState == ANIM_IDLE) && (mState == PLAYER_SWIM || mState == PLAYER_INVINCIBLE))
	{
		if (SUCK_MIN_SECONDS <= mSuckSeconds)
		{
			mSavedState = mState;
			mState = PLAYER_SPECIAL;
			if (mScale.x <= 0.0f)
			{
				mSuckFxRight->startEmitting();
				mSuckingRight = true;
			}
			else
			{
				mSuckFxLeft->startEmitting();
				mSuckingRight = false;
			}
			if (mSuckSound != nullptr)
				mSuckSound->play();
			mAnimations->select((mHasTurnAnimation ? 1 : 0) + 1, 0.0f, true);
			AnimImage* aMouth = static_cast<AnimImage*>(mAnimations->getSelected());
			mEatFinished = false;
			aMouth->setRate(SUCK_ANIMATION_RATE);
			aMouth->setRange(0, SUCK_OPENING_LAST_FRAME);
			aMouth->setFrame(0);
			aMouth->setLoop(false);
			aMouth->play();
			mSuckPhase = SUCK_OPENING;
			mSuckQueued = false;
		}
		return;
	}
	mSuckQueued = true;
}

// 0x40e7e0
void JohnDoryPlayer::releaseSpecial()
{
	mSuckQueued = false;
	endPull();
}

// 0x40ea40
float JohnDoryPlayer::getSpecialProgress()
{
	if (mSuckSeconds <= 0.0f)
		return 0.0f;
	return mSuckSeconds * SUCK_PROGRESS_FACTOR;
}

// 0x40ef80
void JohnDoryPlayer::pullFish()
{
	forgetRemovedFish();
	CollisionRect aMouth;
	float aBottom = mNose.y + PULL_HALF_HEIGHT;
	if (!mSuckingRight)
		aMouth.set(mNose.x - PULL_REACH, mNose.y - PULL_HALF_HEIGHT, mNose.x, aBottom);
	else
		aMouth.set(mNose.x, mNose.y - PULL_HALF_HEIGHT, mNose.x + PULL_REACH, aBottom);

	const std::vector<FishBase*>& aFishList = FishFood::instance()->getStageMgr()->getFishList();
	std::vector<FishBase*>::const_iterator aFish = aFishList.begin();
	while (aFish != aFishList.end())
	{
		FishBase* aCandidate = *aFish;
		if (aMouth.intersects(*aCandidate->getPlacedHitRect()) && !aCandidate->isEaten() && aCandidate->isPoisonProof())
		{
			if (aCandidate->getLevel() < mLevel && aCandidate->canBeEatenBy(this))
			{
				aCandidate->setEaten(true);
				addToChain(1);
				aCandidate->startBeingEaten(this, EATEN_SECONDS);
				mVictims.push_back(aCandidate);
			}
			else
			{
				aCandidate->setEaten(true);
				mPulledFish.push_back(aCandidate);
			}
		}
		++aFish;
	}

	std::vector<FishBase*>::iterator aPulled = mPulledFish.begin();
	while (aPulled != mPulledFish.end())
	{
		FishBase* aHeld = *aPulled;
		if (!aMouth.intersects(*aHeld->getPlacedHitRect()))
		{
			aHeld->setEaten(false);
			aHeld->clearPush();
		}
		else
		{
			Point aPull = aHeld->getNosePosition();
			aPull.subtract(mNose);
			aPull.normalize();
			aPull.x = aPull.x * PULL_STRENGTH;
			aPull.y = aPull.y * PULL_STRENGTH;
			aHeld->clearPush();
			aHeld->addPush(aPull.x, aPull.y);
		}
		++aPulled;
	}
}

// 0x40e7f0
void JohnDoryPlayer::endPull()
{
	mSuckFxLeft->stopEmitting();
	mSuckFxRight->stopEmitting();
	if (mSuckSound != nullptr)
		mSuckSound->stop();
	if (mSuckAnimation != nullptr)
	{
		mSuckAnimation->resetRange();
		mSuckAnimation->play();
	}
	mSuckPhase = SUCK_CLOSING;
}

// 0x40ebd0
void JohnDoryPlayer::releasePulled()
{
	forgetRemovedFish();
	std::vector<FishBase*>::iterator aPulled = mPulledFish.begin();
	while (aPulled != mPulledFish.end())
	{
		(*aPulled)->setEaten(false);
		(*aPulled)->clearPush();
		++aPulled;
	}
}

// port: the stage deletes a fish that leaves it, and the list of held fish would then point at nothing
void JohnDoryPlayer::forgetRemovedFish()
{
	const std::vector<FishBase*>& aFishList = FishFood::instance()->getStageMgr()->getFishList();
	std::vector<FishBase*>::iterator aPulled = mPulledFish.begin();
	while (aPulled != mPulledFish.end())
	{
		if (std::find(aFishList.begin(), aFishList.end(), *aPulled) == aFishList.end())
			aPulled = mPulledFish.erase(aPulled);
		else
			++aPulled;
	}
}

} // namespace feedingfrenzy
