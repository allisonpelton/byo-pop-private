#include "Level.h"

using namespace pizzafrenzy;

// 0x417620
Level::Level(const std::string& theCityFile, const std::string& theName, const std::string& theMusic, int theGoal,
             int theNumOrders, float theWaveGap) :
	mCityFile(theCityFile), mMusic(theMusic), mName(theName), mBonusRound(false), mGoal(theGoal),
	mWaveGap(theWaveGap), mNumOrders(theNumOrders)
{
}

// 0x417830: the list counts the topping (a topping the manifest has none of is a null entry)
void Level::addTopping(Topping* theTopping)
{
	mToppings.push_back(theTopping);
}

// 0x4178b0
void Level::addPizza(Pizza* thePizza)
{
	mPizzas.push_back(thePizza);
}

// 0x417930
void Level::addKitchen(const std::string& theVanType)
{
	LevelKitchen aKitchen;
	aKitchen.mKitchen = nullptr;
	aKitchen.mVanType = theVanType;
	mKitchens.push_back(aKitchen);
}

// 0x4179d0
void Level::addBoat(const std::string& theName, engine::IntPair thePosition, const engine::FloatPair& theDelay,
                    engine::IntPair theDirection)
{
	LevelBoat aBoat;
	aBoat.mName = theName;
	aBoat.mPosition = thePosition;
	aBoat.mDelay = theDelay;
	aBoat.mDirection = theDirection;
	mBoats.push_back(aBoat);
}

// 0x417ad0
void Level::addEvent(const std::string& theName, engine::IntPair theDelay, bool theFirstWave)
{
	LevelEvent anEvent;
	anEvent.mName = theName;
	anEvent.mDelay = theDelay;
	anEvent.mFirstWave = theFirstWave;
	mEvents.push_back(anEvent);
}

// 0x417bb0
void Level::addWave(const engine::IntPair& theSize, const engine::FloatPair& theSpawnDelay,
                    const engine::FloatPair& theWaitTime)
{
	LevelWave aWave;
	aWave.mSize = theSize;
	aWave.mSpawnDelay = theSpawnDelay;
	aWave.mWaitTime = theWaitTime;
	mWaves.push_back(aWave);
}
