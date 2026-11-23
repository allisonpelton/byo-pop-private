#include "Oyster.h"
#include "AnimImage.h"
#include "AnimImagePivot.h"
#include "App.h"
#include "CollisionRect.h"
#include "FishFood.h"
#include "GameSession.h"
#include "ImageNode.h"
#include "PlayerFish.h"
#include "Random.h"
#include "StageMgr.h"
#include <vector>

using namespace engine;

namespace feedingfrenzy
{

// The shell's pictures: the end of the folder that has them, and their extension
const char* const OPEN_PICTURES = "open.";
const char* const PICTURE_EXTENSION = ".jpg";
// The three pearls' pictures: the white one, the black one and the bubble of the extra fish, and how large that one shows
const char* const WHITE_PEARL_PICTURE = "resources\\Oyster\\pearl.jpg";
const char* const BLACK_PEARL_PICTURE = "resources\\Oyster\\blackPearl.jpg";
const char* const LIFE_PEARL_PICTURE = "resources\\bonus\\fishBubble1.png";
const float LIFE_PEARL_SCALE = 1.25f;
// Where a pearl rests from the oyster
const float PEARL_REST_X = 0.0f;
const float PEARL_REST_Y = 15.0f;
// The chomp's particle file, the seconds the particles are run for at once to start with, and where they stand from the
// oyster
const char* const CHOMP_FX_FILE = "config\\fx\\oysterChompFx.xml";
const float CHOMP_FX_WARMUP_SECONDS = 0.05f;
const Point CHOMP_FX_OFFSET(0.0f, 8.0f);
// The pearl's glimmer: its particle file, where it stands from the oyster, and the group of the stage it joins (the middle
// ground's)
const char* const GLIMMER_FILE = "config\\fx\\oysterPearlFx.xml";
const Point GLIMMER_OFFSET(0.0f, 8.0f);
const bool GLIMMER_IN_MID_GROUND = true;
// The sound the player makes eating the pearl: its name, its file, how many voices it has, its volume, its pitch and its
// category
const char* const PEARL_SOUND_NAME = "oysterPearl";
const char* const PEARL_SOUND_FILE = "resources\\sounds\\oysterPearl.wav";
const int PEARL_SOUND_VOICES = 3;
const float PEARL_SOUND_VOLUME = 1.0f;
const float PEARL_SOUND_PITCH = 1.0f;
const int PEARL_SOUND_CATEGORY = 0;
// The sound of the shell closing, and the volume and pitch of its two plays (the second when it has taken a fish)
const char* const BITE_SOUND = "oysterBite";
const float BITE_VOLUME = 0.6f;
const float BITE_PITCH = 1.0f;
const float BITE_FISH_VOLUME = 1.0f;
const float BITE_FISH_PITCH = 0.6f;
// How fast the shell's pictures play, opening and closing (pictures a second)
const float OPEN_RATE = 10.0f;
const float CLOSE_RATE = -50.0f;
// How large the pearl is as a share of the pictures that have played: opening, closing, and the share closing it goes
// out below
const float OPEN_PEARL_SCALE = 0.9f;
const float CLOSE_PEARL_SCALE = 0.8f;
const float CLOSE_PEARL_MIN_SCALE = 0.3f;
// The seconds the shell stays open when a fish that it cannot eat is at its mouth
const float REOPEN_SECONDS = 0.5f;
// A marked oyster shows a pearl this often
const float PEARL_PRESENT_CHANCE = 0.6f;
// The chances of a pearl being the one of the extra fish (not in the timed game) and of it being black; the black
// pearl is worth this many white ones
const float LIFE_PEARL_CHANCE = 0.06f;
const float BLACK_PEARL_CHANCE = 0.15f;
const float BLACK_PEARL_FACTOR = 5.0f;
// The range the chances are drawn from
const Point CHANCE_RANGE(0.0f, 1.0f);
// The layer of the stage the oyster stands in (the middle one)
const int STAGE_LAYER = 1;
// The seconds the oyster takes to eat a fish, and the seconds the pearl takes to be eaten that the base is given
const float EAT_SECONDS = 0.1f;
const float EATEN_BASE_SECONDS = 0.2f;
// The volume of the sound of the pearl being eaten, by the player and by a fish
const float EATEN_BY_PLAYER_VOLUME = 1.0f;
const float EATEN_BY_FISH_VOLUME = 0.6f;

// 0x40af30
Oyster::Oyster() :
    mShellPhase(SHELL_CLOSED), mPearl(nullptr), mChompFx(nullptr), mPearlPresent(true), mPearlKind(PEARL_WHITE),
    mPearlPoints(0.0f), mLeaving(false)
{
	mEdibleKind = EDIBLE_OBJECT;
	mKind = FISH_KIND_BONUS;
	mPoisonProof = false;
	mStunnable = false;
	mTargetable = false;
	mReactable = false;
	mCountsAsFood = false;
}

// 0x40a480
void Oyster::loadAnimations(const char* theOpenPath, const char*, const char*, const char*)
{
	mAnimations = new Selector();
	addChild(mAnimations);

	Ref<AnimImage> aShell(new AnimImage());
	aShell->load(theOpenPath, PICTURE_EXTENSION, 1);
	setAnimationPivot(aShell, 1);
	mAnimations->addChild(aShell);
	mAnimations->select(0, 0.0f, true);
	setScale(mSize, mSize);
	updateRects();
	setFlags(NODE_DRAWABLE | NODE_LISTENERS);

	mPearl = new Selector();
	Image* aWhitePicture = App::instance()->getImage(WHITE_PEARL_PICTURE);
	aWhitePicture->setPivot(1);
	mPearl->addChild(new ImageNode(aWhitePicture));
	Image* aBlackPicture = App::instance()->getImage(BLACK_PEARL_PICTURE);
	aBlackPicture->setPivot(1);
	mPearl->addChild(new ImageNode(aBlackPicture));
	Image* aLifePicture = App::instance()->getImage(LIFE_PEARL_PICTURE);
	aLifePicture->setPivot(1);
	ImageNode* aLifePearl = new ImageNode(aLifePicture);
	mPearl->addChild(aLifePearl);
	aLifePearl->setScale(LIFE_PEARL_SCALE, LIFE_PEARL_SCALE);
	mPearl->setVisible(false);
	mPearlOffset.set(PEARL_REST_X, PEARL_REST_Y);
	mPearl->setPosition(mPearlOffset);
	mPearl->setOpacity(1.0f);
	addChild(mPearl);
}

// 0x40b900
void Oyster::loadAnimationsOf(const std::string& theFolder)
{
	std::string aPath = theFolder + OPEN_PICTURES;
	loadAnimations(aPath.c_str(), nullptr, nullptr, nullptr);
	FishFood::instance()->getSoundBank().add(PEARL_SOUND_NAME, PEARL_SOUND_FILE, PEARL_SOUND_VOICES,
	                                         PEARL_SOUND_VOLUME, PEARL_SOUND_PITCH, PEARL_SOUND_CATEGORY);
}

// 0x40b790
void Oyster::loadEffects()
{
	mChompFx = new ParticleSystem();
	mChompFx->warmup(CHOMP_FX_WARMUP_SECONDS);
	mChompFx->setPosition(CHOMP_FX_OFFSET.x, CHOMP_FX_OFFSET.y);
	addChild(mChompFx);
	mChompFx->load(CHOMP_FX_FILE);

	mGlimmer = new FishAttachFx();
	mGlimmer->init(GLIMMER_FILE, this, GLIMMER_OFFSET, GLIMMER_IN_MID_GROUND);
}

// 0x40af00
void Oyster::setPoints(float thePoints)
{
	mPoints = thePoints;
	mPearlPoints = thePoints;
}

// 0x40aa00
bool Oyster::canBeEatenByLevel(float)
{
	return mState == AI_SPECIAL && mShellPhase == SHELL_OPEN && mPearl->isVisible() && mPearlPresent;
}

// 0x40aa40
bool Oyster::canBeEatenBy(FishBase*)
{
	return canBeEatenByLevel(0.0f);
}

// 0x40b470
void Oyster::startBeingEaten(FishBase* theEater, float theSeconds)
{
	mBeingEatenSeconds = theSeconds;
	mState = AI_BEING_EATEN;
	mBeingEatenLeft = theSeconds;
	mEater = theEater;
	FishBase::startBeingEaten(theEater, EATEN_BASE_SECONDS);

	float aVolume = theEater == FishFood::instance()->getStageMgr()->getPlayer() ? EATEN_BY_PLAYER_VOLUME
	                                                                              : EATEN_BY_FISH_VOLUME;
	if (!mEatenSoundName.empty())
		FishFood::instance()->playSound(mEatenSoundName, aVolume, 1.0f);
}

// 0x40abd0
bool Oyster::finishBeingEaten(FishBase*)
{
	mEatCount++;
	mPearlPresent = false;
	mPearl->setVisible(false);
	mPearl->setPosition(PEARL_REST_X, PEARL_REST_Y);
	mGlimmer->stop();
	mState = AI_SPECIAL;
	return true;
}

// 0x40ba30
void Oyster::eat(FishBase* theVictim)
{
	theVictim->startBeingEaten(this, EAT_SECONDS);
	mVictims.push_back(theVictim);
}

// 0x40b150
void Oyster::putInPlay()
{
	AIFish::putInPlay();
	mAnimations->select(0, 0.0f, true);
	beginWander();
	mPearlPresent = true;
	pickPearl();
}

// 0x40adc0
void Oyster::updatePlacedHitRect()
{
	FishBase::updatePlacedHitRect();
	Point anOrigin = FishFood::instance()->getStageMgr()->layerToScreen(Point(0.0f, 0.0f), STAGE_LAYER);
	mPlacedHitRect.offset(anOrigin.x, anOrigin.y);
}

// 0x40ac30
void Oyster::updatePlacedRects()
{
	mNose = FishFood::instance()->getStageMgr()->layerToScreen(mPosition, STAGE_LAYER);
	mTail = mNose;
	mPlacedBodyRect.left = mBodyRect.left + mNose.x;
	mPlacedBodyRect.right = mBodyRect.right + mNose.x;
	mPlacedBodyRect.top = mBodyRect.top + mNose.y;
	mPlacedBodyRect.bottom = mBodyRect.bottom + mNose.y;
	const Rect& aMouth = mMouthRect.getRect();
	mPlacedMouthRect.set(aMouth.left + mNose.x, aMouth.top + mNose.y, aMouth.right + mNose.x,
	                     aMouth.bottom + mNose.y);
}

// 0x419550
void Oyster::updateAnimation()
{
	mAnimations->select(0, 0.0f, true);
}

// 0x40aa50
void Oyster::updateBeingEaten(UpdateInfo& theInfo)
{
	mBeingEatenLeft = mBeingEatenLeft - theInfo.mSeconds;
	if (mBeingEatenLeft < 0.0f)
		mBeingEatenLeft = 0.0f;

	// The pearl moves from the eater's nose, as the stage's layer shows the oyster, to where it rests, as the time runs out
	Point aMove = mEater->getNosePosition();
	aMove.subtract(FishFood::instance()->getStageMgr()->layerToScreen(mPosition, STAGE_LAYER));
	aMove.subtract(mPearlOffset);
	float aDistance = aMove.length();
	aMove.normalize();
	double aShareExact = (double)mBeingEatenLeft / (double)mBeingEatenSeconds;
	float aShare = (float)aShareExact;
	aMove.scale((float)(aShareExact * (double)aDistance));
	mPearl->setScale(aShare, aShare);
	mPearl->setPosition(aMove.x + mPearlOffset.x, aMove.y + mPearlOffset.y);
	updateSpecial(theInfo);
	Container::update(theInfo);
}

// 0x40a760
void Oyster::updateWander(UpdateInfo& theInfo)
{
	FishBase::update(theInfo);
	mStateSeconds = mStateSeconds - theInfo.mSeconds;
	if (mStateSeconds < 0.0f && (mMarked || mPearlPresent))
		beginSpecial();
}

// 0x40a7b0
void Oyster::updateSpecial(UpdateInfo& theInfo)
{
	AnimImage* aShell = static_cast<AnimImage*>(mAnimations->getSelected());
	switch (mShellPhase)
	{
	case SHELL_CLOSED:
		finishEating();
		mChompFx->startEmitting();
		mGlimmer->stop();
		decide();
		break;
	case SHELL_OPENING:
	{
		if (mPearlPresent)
		{
			float aPearlScale = (float)((double)aShell->getFrame() / (double)aShell->getCount() *
			                            (double)OPEN_PEARL_SCALE);
			mPearl->setVisible(true);
			mPearl->setScale(aPearlScale, aPearlScale);
		}
		if (!aShell->isPlaying())
		{
			mDecisionSeconds = randomFloat(mWanderDecisionRange);
			mShellPhase = SHELL_OPEN;
			FishBase::update(theInfo);
			return;
		}
		break;
	}
	case SHELL_OPEN:
		if (!mLeaving)
		{
			mDecisionSeconds = mDecisionSeconds - theInfo.mSeconds;
			if (mDecisionSeconds < 0.0f)
			{
				mShellPhase = SHELL_CLOSING;
				bite();
				FishBase::update(theInfo);
				return;
			}
		}
		break;
	case SHELL_CLOSING:
	{
		// The pearl is compared before it is rounded to a float, as the original's does
		double aPearlShare = (double)aShell->getFrame() / (double)aShell->getCount() * (double)CLOSE_PEARL_SCALE;
		float aPearlScale = (float)aPearlShare;
		if (aPearlShare < (double)CLOSE_PEARL_MIN_SCALE)
			aPearlScale = 0.0f;
		mPearl->setScale(aPearlScale, aPearlScale);
		if (!aShell->isPlaying())
		{
			mDecisionSeconds = randomFloat(mWanderDecisionRange);
			mShellPhase = SHELL_CLOSED;
			FishBase::update(theInfo);
			return;
		}
		break;
	}
	default:
		break;
	}
	FishBase::update(theInfo);
}

// 0x40a9c0
void Oyster::decide()
{
	if (mState == AI_WANDER)
	{
		if (mMarked || mPearlPresent)
			beginSpecial();
	}
	else if (mState == AI_SPECIAL)
		beginWander();
}

// 0x40a730
void Oyster::beginWander()
{
	mState = AI_WANDER;
	mStateSeconds = randomFloat(mWanderStateRange);
}

// 0x40b040
void Oyster::beginSpecial()
{
	if (mLeaving)
		return;

	mState = AI_SPECIAL;
	mShellPhase = SHELL_OPENING;
	AnimImage* aShell = static_cast<AnimImage*>(mAnimations->getSelected());
	aShell->setLoop(false);
	aShell->setRate(OPEN_RATE);
	aShell->play();
	if (mMarked)
	{
		mPearlPresent = randomFloat(CHANCE_RANGE) < PEARL_PRESENT_CHANCE;
		pickPearl();
	}
	if (mPearlPresent)
	{
		mPearl->setScale(0.0f, 0.0f);
		mPearl->setPosition(mPearlOffset);
		mPearl->select(mPearlKind, 0.0f, true);
		mPearl->setVisible(true);
		mGlimmer->start();
	}
}

// 0x40af20
void Oyster::beginLeave()
{
	mLeaving = true;
}

// 0x40ae60
void Oyster::pickPearl()
{
	float aChance = randomFloat(CHANCE_RANGE);
	if (aChance < LIFE_PEARL_CHANCE && FishFood::instance()->getSession()->getMode() != GAME_MODE_TIMED)
	{
		mPearlKind = PEARL_LIFE;
		mPoints = 0.0f;
		return;
	}
	if (aChance < BLACK_PEARL_CHANCE)
	{
		mPearlKind = PEARL_BLACK;
		mPoints = mPearlPoints * BLACK_PEARL_FACTOR;
	}
	else
	{
		mPearlKind = PEARL_WHITE;
		mPoints = mPearlPoints;
	}
}

// 0x40b530
void Oyster::bite()
{
	const std::vector<FishBase*>& aFish = FishFood::instance()->getStageMgr()->getFishList();
	for (size_t aFishIndex = 0; aFishIndex < aFish.size(); aFishIndex++)
	{
		FishBase* aCandidate = aFish[aFishIndex];
		CollisionRect aHit = *aCandidate->getPlacedHitRect();
		if (aCandidate != this && aHit.intersects(*getBodyRect()) && !aCandidate->canBeEatenBy(this))
		{
			mDecisionSeconds = REOPEN_SECONDS;
			mShellPhase = SHELL_OPEN;
			return;
		}
	}

	for (size_t aFishIndex = 0; aFishIndex < aFish.size(); aFishIndex++)
	{
		FishBase* aCandidate = aFish[aFishIndex];
		CollisionRect aHit = *aCandidate->getPlacedHitRect();
		if (aHit.intersects(*getBodyRect()) && aCandidate->canBeEatenBy(this))
			eat(aFish[aFishIndex]);
	}

	FishFood::instance()->playSound(BITE_SOUND, BITE_VOLUME, BITE_PITCH);
	AnimImage* aShell = static_cast<AnimImage*>(mAnimations->getSelected());
	aShell->setRate(CLOSE_RATE);
	aShell->play();
	if (!mVictims.empty())
		FishFood::instance()->playSound(BITE_SOUND, BITE_FISH_VOLUME, BITE_FISH_PITCH);
}

} // namespace feedingfrenzy
