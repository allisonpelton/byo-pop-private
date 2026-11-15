#include "GameStats.h"
#include "PizzaFrenzy.h"
#include "Random.h"

#include <stdexcept>

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
	mLevelOrders = 0;
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
	mLevelGoal = 0;
	mPizzasDelivered = 0;
	mHappiness = STARTING_HAPPINESS;
	mCustomersServed = 0;
	mPizzasOrdered = 0;
	mTipMoney = 0;
	mTipsEarned = 0;
	mWavesStarted = 0;
	mComboLength = 0;
	mTallies.clear();
	mCountI = 0;
	mCountJ = 0;
	mCountK = 0;
	mCountL = 0;
	mCountM = 0;
	mStarted = true;
}

// 0x41a740: the level's orders and goal, then the counters, and the kitchens' toppings when a kitchen has none
void GameStats::startLevel(Level* theLevel)
{
	mOrderCount = theLevel->mNumOrders;
	mLevelOrders = theLevel->mNumOrders;
	mLevelGoal = theLevel->mGoal;
	resetLevelCounters();
	bool anAllHaveToppings = true;
	for (size_t i = 0; i < theLevel->mKitchens.size(); i++)
	{
		if (theLevel->mKitchens[i].mTopping == nullptr)
			anAllHaveToppings = false;
	}
	if (!anAllHaveToppings)
		pickKitchenToppings(theLevel);
}

// 0x419c90
void GameStats::resetLevelCounters()
{
	mPizzasDelivered = 0;
	mHappiness = STARTING_HAPPINESS;
	mCustomersServed = 0;
	mPizzasOrdered = 0;
	mTipMoney = 0;
	mTipsEarned = 0;
	mWavesStarted = 0;
	mComboLength = 0;
	mTallies.clear();
	mCountI = 0;
	mCountJ = 0;
	mCountK = 0;
	mCountL = 0;
	mCountM = 0;
	mStarted = true;
}

// 0x41a220: the toppings the player has, in the order of the map, are the candidates; each kitchen that has no topping
// takes the one a random draw picks, and the candidate is crossed off. The kitchens' toppings are cleared first, so
// every kitchen takes one. The order of the draws follows the map of the toppings, which is by the toppings' addresses
void GameStats::pickKitchenToppings(Level* theLevel)
{
	for (size_t i = 0; i < theLevel->mKitchens.size(); i++)
		theLevel->mKitchens[i].mTopping = nullptr;
	std::vector<engine::Ref<Topping>> aCandidates;
	for (std::map<Topping*, int>::iterator anIt = mToppingLevels.begin(); anIt != mToppingLevels.end(); ++anIt)
		aCandidates.push_back(anIt->first);
	UserGameData* aUserData = PizzaFrenzy::instance()->mUserGameData;
	aUserData->mPickToppings.clear();
	for (size_t i = 0; i < theLevel->mKitchens.size(); i++)
	{
		LevelKitchen& aKitchen = theLevel->mKitchens[i];
		if (aKitchen.mTopping == nullptr && !aCandidates.empty())
		{
			int aPick = engine::randomRange(0, (int)aCandidates.size() - 1);
			if ((size_t)aPick >= aCandidates.size())
				throw std::out_of_range("invalid vector<T> subscript");
			aKitchen.mTopping = aCandidates[aPick];
			aUserData->mPickToppings.push_back(aKitchen.mTopping);
			for (size_t j = 0; j < aCandidates.size(); j++)
			{
				if (aCandidates[j] == aKitchen.mTopping)
				{
					aCandidates.erase(aCandidates.begin() + j);
					break;
				}
			}
		}
	}
	aUserData->save();
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

// 0x419b90
void GameStats::addTips(int theTips)
{
	mTipMoney += theTips;
	if (mTipMoney < 0)
		mTipMoney = 0;
}
