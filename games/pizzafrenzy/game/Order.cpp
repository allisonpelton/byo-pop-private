#include "Order.h"
#include "Level.h"
#include "PizzaFrenzy.h"
#include "Random.h"

using namespace pizzafrenzy;

// 0x419690
void Order::clear()
{
	mWaitTime = 0;
	mTile = nullptr;
	mPizzaCount = 0;
	mTopping = nullptr;
	mCharacter = nullptr;
	mSecondsSinceDelivery = 0;
	mEvent = nullptr;
	mIndex = 0;
}

// 0x44b490: the waves it waits are drawn from the event's two numbers, and none when it happens in the first wave
EventTimer::EventTimer(LevelEvent* theEvent) : mEvent(theEvent)
{
	mWavesLeft = engine::randomRange(theEvent->mDelay);
	if (theEvent->mFirstWave)
		mWavesLeft = 0;
}

// 0x44b3c0
void EventTimer::reset()
{
	mWavesLeft = engine::randomRange(mEvent->mDelay);
}

// 0x44b530
Order* EventTimer::tick()
{
	mWavesLeft--;
	if (mWavesLeft < 1)
	{
		Order* anOrder = makeOrder();
		mWavesLeft = engine::randomRange(mEvent->mDelay);
		return anOrder;
	}
	return nullptr;
}

// 0x44b3e0
Order* EventTimer::makeOrder()
{
	TileManifest* aManifest = PizzaFrenzy::instance()->mTileManifest;
	SpecialEvent* anEvent = aManifest->getEvent(mEvent->mName);
	if (anEvent == nullptr)
		return nullptr;
	Order* anOrder = new Order();
	anOrder->clear();
	anOrder->mEvent = anEvent;
	Character* aCharacter = aManifest->getCharacterByNameTheme(anEvent->mCharacter);
	if (aCharacter != nullptr)
	{
		anOrder->mCharacter = aCharacter;
		anOrder->mPizzaCount = aCharacter->mNumPizzas;
	}
	return anOrder;
}
