#include "PlayerSpawn.h"

#include "FeedingFrenzyTwo.h"
#include "FishFactory.h"
#include "GameData.h"
#include "PlayerFish.h"
#include "Stage.h"
#include "UserData.h"

using namespace engine;

namespace
{

// The player whose fish is the alien (0x558158 (feedingfrenzy2))
const char* const ALIEN_USER = "alienUser";
// The time the frenzy holds after a bite (0x566988 (feedingfrenzy2))
const float FRENZY_HOLD_SECONDS = 1.1f;

} // namespace

PlayerSpawn::PlayerSpawn(Stage* theStage)
    : Spawner(theStage), mLayer(0), mPhaseIndex(0), mDeaths(0), mLive(false)
{
}

SubDocHandler* PlayerSpawn::createDocHandler()
{
	return new PlayerSpawnDocHandler(this);
}

void PlayerSpawn::read(Params& theAttributes)
{
	Spawner::read(theAttributes);
	mVisualName = theAttributes.getString("visual", "");
	// 0x402750 and 0x490380 (feedingfrenzy2)
	if (getApp()->mUserData->getName() == ALIEN_USER)
		mVisualName = theAttributes.getString("alienVisual", mVisualName);
	mBehaviorName = theAttributes.getString("behavior", "");
	mLayer = theAttributes.getInt("layer", 0);
}

void PlayerSpawn::finish()
{
	// 0x4926b0 and 0x4a1800 (feedingfrenzy2)
	mStage->addSpawner(this);
	mStage->addPlayerSpawn(this);
}

void PlayerSpawn::setup(int& theProgress)
{
	// A fish visual for each phase from the fish factory, at the phase's size times the stage's size unit, with no
	// tint (0x5083da (feedingfrenzy2), (0, 0, 0, 0)) and set up as in the air
	FishFactory* aFactory = FishFactory::get();
	mVisuals.resize(mPhases.size());
	for (size_t i = 0; i < mPhases.size(); i++)
	{
		float aUnit = mStage->getSizeUnit();
		float aSize = (float)((double)aUnit * mPhases[i].mSize);
		// 0x40e250 (feedingfrenzy2)
		FishVisualInfo* anInfo = aFactory->findFishVisual(mVisualName);
		FishVisualData* aData = nullptr;
		if (anInfo)
		{
			aData = new FishVisualData();
			aData->setup(anInfo, aFactory->mAnimBase, aSize, Color(0.0f, 0.0f, 0.0f, 0.0f), true);
		}
		mVisuals[i] = aData;
		theProgress++;
	}
	// 0x488030 (feedingfrenzy2)
	mBehavior = aFactory->findFishBehavior(mBehaviorName);
	if (mBehavior == nullptr)
		getEngine()->log("There is no such fishBehavior as %s.\n", mBehaviorName.c_str());
	mFish = new PlayerFish();
	mFish->create(mStage);
	mFish->setFishBehavior(mBehavior.get());
	mFish->setBounds(mStage, false);
	mLive = false;
}

void PlayerSpawn::start()
{
	if (mLive)
	{
		// The cold path 0x4f27be (feedingfrenzy2): not yet translated
		return;
	}
	mFish->setFrenzyHoldTime(FRENZY_HOLD_SECONDS);
	mFish->setFoodValue(0);
	// The fish's slot 0x70 (0x496b40 (feedingfrenzy2)): the rank, and its list at +0x2b8 (not yet translated)
	mFish->setRank(mPhaseIndex);
	mFish->setEaterRank(mPhaseIndex + 1);
	mFish->setPlayerVisualData(mPhases[mPhaseIndex].mSize, mVisuals[mPhaseIndex].get());
	// 0x4984c0 (feedingfrenzy2): the spawn's layer at every detail level
	int* aDetailLayers = mFish->getDetailLayers();
	for (int i = 0; i < 3; i++)
		aDetailLayers[i] = mLayer;
	mFish->setLayer(mLayer);
	mFish->setSpawn(this);
	mFish->addToStage(mStage, SPAWN_LOCATION_POINT);
	mLive = true;
}

void PlayerSpawn::update(UpdateContext* theContext, bool canSpawn)
{
	if (!mFish || !mLive)
		return;
	if (mFish->isDead())
	{
		mFish->remove();
		mDeaths++;
		mLive = false;
		return;
	}
	int aFood = getApp()->mGameData->getFood();
	if (aFood < mPhases[mPhaseIndex].mFoodGoal)
		return;
	int aNext = mPhaseIndex + 1;
	if (aNext >= (int)mPhases.size())
		return;
	mPhaseIndex = aNext;
	mFish->setRank(mPhaseIndex);
	mFish->setEaterRank(mPhaseIndex + 1);
	mFish->setPlayerVisualData(mPhases[mPhaseIndex].mSize, mVisuals[mPhaseIndex].get());
	if ((double)mPhases[mPhaseIndex - 1].mSize != (double)mPhases[mPhaseIndex].mSize)
	{
		// The theme's sound playerGrow: not yet translated, as the sounds are not
	}
	// The HUD's rank (0x48e6e0 (feedingfrenzy2), the eater rank): not yet translated, as the HUD is not
}

void PlayerSpawn::takeOut()
{
	if (!mLive)
		return;
	mFish->remove();
	mLive = false;
}

void PlayerSpawnDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName != "phase")
		return;
	float aSize = theAttributes.getFloat("size", 0.0f);
	int aFoodGoal = theAttributes.getInt("foodGoal", 0);
	mSpawn->addPhase(PlayerPhase(aSize, aFoodGoal));
}
