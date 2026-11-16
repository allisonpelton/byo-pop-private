#include "BoatSpawner.h"
#include "Boat.h"
#include "CityMap.h"
#include "GameMode.h"
#include "Node.h"
#include "PizzaFrenzy.h"
#include "Random.h"

using namespace pizzafrenzy;

namespace
{
engine::Point toPoint(const engine::FloatPair& thePair)
{
	return engine::Point(thePair.mFirst, thePair.mSecond);
}
}

// 0x42f5b0
BoatSpawner::BoatSpawner(CityMap* theMap, LevelBoat* theBoat) : mBoat(theBoat), mMap(theMap)
{
	mTimeLeft = engine::randomFloat(toPoint(theBoat->mDelay));
}

// 0x42f4b0
void BoatSpawner::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft -= theInfo.mSeconds;
	if (mTimeLeft <= 0.0f)
	{
		spawn();
		mTimeLeft = engine::randomFloat(toPoint(mBoat->mDelay));
	}
}

// 0x42f4f0
void BoatSpawner::spawn()
{
	Boat* aBoat = new Boat(mBoat->mName);
	aBoat->loadType();
	aBoat->setDirection(engine::Point((float)mBoat->mDirection.mFirst, (float)mBoat->mDirection.mSecond));
	aBoat->start(mMap, mBoat->mPosition);
	PizzaFrenzy::instance()->mMode->addVehicleNode(aBoat);
}
