#include "GameStats.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
// A game with more toppings than this counts the pizzas the player designed as toppings too
const int BASE_TOPPING_COUNT = 4;

const int STARTING_HAPPINESS = 50;
}

// 0x41a930: the two maps are empty, and the reset does the rest
GameStats::GameStats()
{
	reset();
}

// 0x41a7b0: every topping of the manifest the player has goes into the levels, and out of the toppings that are new
// to the player
void GameStats::reset()
{
	mScore = 0;
	mElapsedTime = 0;
	mCountN = 0;
	mToppingLevels.clear();
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	std::map<std::string, engine::Ref<Topping>>& aToppings = anApp->mTileManifest->getToppings();
	for (std::map<std::string, engine::Ref<Topping>>::iterator anIt = aToppings.begin(); anIt != aToppings.end();
	     ++anIt)
	{
		Topping* aTopping = anIt->second;
		int aLevel = anApp->mUserGameData->getToppingLevel(aTopping);
		if (-1 < aLevel)
		{
			mToppingLevels[aTopping] = aLevel;
			anApp->mUserGameData->removeNewTopping(aTopping);
		}
	}
	if (BASE_TOPPING_COUNT < (int)mToppingLevels.size())
		addDesignedPizzas();
	mCountO = 0;
	mCountA = 0;
	mHappiness = STARTING_HAPPINESS;
	mCountD = 0;
	mCountB = 0;
	mTipMoney = 0;
	mCountF = 0;
	mCountG = 0;
	mComboLength = 0;
	mTallies.clear();
	mCountI = 0;
	mCountJ = 0;
	mCountK = 0;
	mCountL = 0;
	mCountM = 0;
	mStarted = true;
}

// 0x41a900
void GameStats::resume(UserGameData* theUserData, int theMode)
{
	reset();
	mScore = theUserData->getLastScore(theMode);
	mElapsedTime = theUserData->getElapsedTime(theMode);
}

// 0x419d30: the score and time are the mode's saved game's, and the player's toppings are the game's (a null topping
// is left out)
void GameStats::saveTo(UserGameData* theUserData, int theMode)
{
	ModeRecord& aMode = theUserData->mModes[theMode];
	aMode.mLastScore = mScore;
	aMode.mElapsedTime = mElapsedTime;
	theUserData->mToppingLevels.clear();
	for (std::map<Topping*, int>::iterator i = mToppingLevels.begin(); i != mToppingLevels.end(); ++i)
	{
		if (i->first != nullptr)
			theUserData->mToppingLevels[i->first] = i->second;
	}
}

// 0x41a6a0
void GameStats::addDesignedPizzas()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	for (size_t i = 0; i < anApp->mDesignedPizzas.size(); i++)
	{
		Topping* aTopping = anApp->mPizzaToppings[anApp->mDesignedPizzas[i]->mId];
		int aLevel = anApp->mUserGameData->getToppingLevel(aTopping);
		if (aLevel < 0)
			mToppingLevels[aTopping] = 0;
		else
			mToppingLevels[aTopping] = aLevel;
	}
}
