#ifndef __PIZZAFRENZY_BOATSPAWNER_H__
#define __PIZZAFRENZY_BOATSPAWNER_H__

// What sends the boats of a level down its river (0x20 bytes, constructor 0x42f5b0): it waits a time drawn from the
// boat's delays, makes the boat, and waits again. The city map keeps one for each boat of the level and updates them.

#include "Level.h"
#include "Node.h"
#include "Object.h"

namespace pizzafrenzy
{

class CityMap;

class BoatSpawner : public engine::Object
{
public:
	BoatSpawner(CityMap* theMap, LevelBoat* theBoat);  // 0x42f5b0

	// slot 0x4: 0x42f4b0: the wait counts down, and a boat is sent when it is up
	void update(engine::UpdateInfo& theInfo);

private:
	// slot 0x8: 0x42f4f0: the boat is made on the river, faces down it, stands on the map, and is given to the mode's
	// ground group
	void spawn();

	LevelBoat* mBoat;  // +0x0c
	float mTimeLeft;   // +0x10: the seconds before the next boat
	CityMap* mMap;     // +0x14
};

} // namespace pizzafrenzy

#endif
