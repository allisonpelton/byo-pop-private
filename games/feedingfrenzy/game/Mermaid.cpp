#include "Mermaid.h"
#include "BonusBubble2X.h"
#include "FishFood.h"
#include "Random.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The seconds the kind of fish's timer starts at, which is no time running
const float NO_TIMER_SECONDS = -1.0f;
// The picture sequence's folder
const char* const SWIM_FOLDER = "swim\\swim_cycle.";
// The bubbles' file, the number of fish its pool holds, and the particle file of the sparkles
const char* const BONUS_FILE = "config\\fish\\FallingBonus.xml";
const int BONUS_POOL_SIZE = 10;
const char* const SPARKLES_FILE = "config\\fx\\mermaidSparkles.xml";
// How far the sparkles are put from the mermaid's place, towards the side it swims to
const float SPARKLES_DISTANCE = 140.0f;
// The seconds the mermaid waits to let the first bubble fall, and to let the next
const float FIRST_DROP_SECONDS = 0.5f;
const float DROP_SECONDS = 0.5f;
// The seconds the decision and the state of a fish last: a time that is never over
const float NEVER_OVER_SECONDS = 10000.0f;
// A bubble falls when the mermaid's nose is this far from the side it swims away from, and inside the stage
const float SIDE_MARGIN = 100.0f;
// Where the bubble is made from the nose, against the way the mermaid swims, and its velocity: a share of the
// mermaid's, in the other direction, and upward
const float DROP_DISTANCE_X = 30.0f;
const Point DROP_VELOCITY_SHARE(0.3f, 0.6f);
const float DROP_VELOCITY_Y = -140.0f;
// The seconds a bubble that falls grows for, and the size it grows to
const float BUBBLE_GROW_SECONDS = 0.6f;
const float BUBBLE_GROW_SIZE = 1.0f;

// 0x4094f0
Mermaid::Mermaid() : mSparkles(nullptr), mBonusSpawner(nullptr)
{
	mTargetable = false;
	mPoisonProof = false;
	mStunnable = false;
	mReactable = false;
	mLeafSeconds = NO_TIMER_SECONDS;
	mKind = FISH_KIND_BONUS;
}

// 0x4091f0
Mermaid::~Mermaid()
{
	delete mBonusSpawner;
}

// 0x409430
void Mermaid::loadAnimationsOf(const std::string& theFolder)
{
	std::string aSwim = theFolder + SWIM_FOLDER;
	loadAnimations(aSwim.c_str(), nullptr, nullptr, nullptr);
}

// 0x409300
void Mermaid::loadEffects()
{
	AIFish::loadEffects();
	mBonusSpawner = new FishSpawner();
	mBonusSpawner->load(BONUS_FILE, BONUS_POOL_SIZE);
	mBonusSpawner->createFish();
	mBonusSpawner->setDisplayMenuImage(false);
	mBonusSpawner->setMode(SPAWN_STOPPED);
	mSparkles = new ParticleSystem();
	mSparkles->load(SPARKLES_FILE);
	addChild(mSparkles);
}

// 0x4091c0
void Mermaid::takeOutOfPlay()
{
	mSparkles->stopEmitting();
	AIFish::takeOutOfPlay();
}

// 0x408e90
void Mermaid::updateSpecial(UpdateInfo& theInfo)
{
	if (!isInStage())
	{
		mDesiredVelocity.x = 0.0f;
		if (mBonusSpawner->isDone())
			takeOutOfPlay();
		return;
	}

	bool aDropHere = false;
	mLeafSeconds = mLeafSeconds - theInfo.mSeconds;
	StageMgr* aStage = FishFood::instance()->getStageMgr();
	if (0.0f < mVelocity.x && SIDE_MARGIN < mNose.x && mNose.x < (float)aStage->getWorldWidth())
		aDropHere = true;
	else if (mVelocity.x < 0.0f && mNose.x < (float)aStage->getWorldWidth() - SIDE_MARGIN && 0.0f < mNose.x)
		aDropHere = true;

	if (mLeafSeconds < 0.0f && aDropHere)
	{
		mBonusSpawner->setLayer(LAYER_FOREGROUND);
		float aSide = 0.0f <= mVelocity.x ? 1.0f : -1.0f;
		float aY = mNose.y;
		float aX = mNose.x - aSide * DROP_DISTANCE_X;
		float aVelocityX = -(randomFloat(DROP_VELOCITY_SHARE) * mVelocity.x);
		FishBase* aBubble =
		    mBonusSpawner->spawnAt(FishFood::instance()->getStageMgr(), false, aX, aY, aVelocityX, DROP_VELOCITY_Y);
		if (aBubble != nullptr)
			static_cast<BonusBubble2X*>(aBubble)->startGrowing(BUBBLE_GROW_SECONDS, BUBBLE_GROW_SIZE);
		mLeafSeconds = DROP_SECONDS;
	}
	FishBase::update(theInfo);
}

// 0x4090e0
void Mermaid::beginSpecial()
{
	if (getX() < 0.0f)
	{
		mDesiredVelocity.x = mMaxSpeedX;
		mSparkles->setPosition((float)(((double)mNose.x - mPosition.x) - SPARKLES_DISTANCE), mNose.y - mPosition.y);
	}
	else
	{
		mDesiredVelocity.x = -mMaxSpeedX;
		mSparkles->setPosition((float)(((double)mNose.x - mPosition.x) + SPARKLES_DISTANCE), mNose.y - mPosition.y);
	}
	mState = AI_SPECIAL;
	mDesiredVelocity.y = 0.0f;
	mLeafSeconds = FIRST_DROP_SECONDS;
	mSparkles->startEmitting();
	mDecisionSeconds = randomFloat(Point(NEVER_OVER_SECONDS, NEVER_OVER_SECONDS));
	mStateSeconds = randomFloat(Point(NEVER_OVER_SECONDS, NEVER_OVER_SECONDS));
}

} // namespace feedingfrenzy
