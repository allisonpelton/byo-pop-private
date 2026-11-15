#include "GameStats.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
// A game with more toppings than this counts the pizzas the player designed as toppings too
const int BASE_TOPPING_COUNT = 4;

const int STARTING_COUNT_C = 50;
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
	mCountC = STARTING_COUNT_C;
	mCountD = 0;
	mCountB = 0;
	mCountE = 0;
	mCountF = 0;
	mCountG = 0;
	mCountH = 0;
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
