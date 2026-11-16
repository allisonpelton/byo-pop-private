#ifndef __PIZZAFRENZY_BOAT_H__
#define __PIZZAFRENZY_BOAT_H__

// The boat (0x1a0 bytes, constructor 0x431810, vtable 0x4fca90): a vehicle of the map that sails down a river of a
// level in a straight line, made by the boat spawner (BoatSpawner.h). It has no home and no way to drive: each frame
// it moves on along its direction, and waits while the point ahead of it is a bridge that cannot be raised (a vehicle
// is on it); a bridge it passes under is raised for two seconds. A click on it makes it sail three times as fast for
// ten frames. It is let go once it has sailed off the screen.

#include "Van.h"

namespace pizzafrenzy
{

class Boat : public Van
{
public:
	explicit Boat(const std::string& theTypeName);  // 0x431810

	// slot 0x140: 0x4318f0: the pictures and the speed of the vehicle type, and the wake over the pictures
	void loadType() override;
	// slot 0x144: 0x4318d0: a frame: the boat sails on
	void updateVehicle(engine::UpdateInfo& theInfo) override;
	// slot 0x164: 0x431980: the picture and the side it faces are the ones of the boat's direction (the cell is not
	// looked at)
	void face(const engine::IntPair& theCell) override;
	// slot 0x168: 0x431550: the boat stands on the map at the point, with no home tile
	void start(CityMap* theMap, const engine::IntPair& thePoint);
	// slot 0x16c: 0x431760: the boat sails along the direction at the speed of its type, and faces that way
	void setDirection(const engine::Point& theDirection);

protected:
	// slot 0xa4: 0x431750: a click on the boat
	void handleMouseDownEvent(engine::MouseEvent& theEvent) override;

private:
	// 0x4315b0: the boat sails on for the seconds, if it may, and is let go once it is off the screen
	void sail(engine::UpdateInfo& theInfo);
	// slot 0x170: 0x431a10: the point is not on a bridge that cannot be raised; one that can is raised
	bool canSail(const engine::Point& thePoint);

	engine::Point mVelocity;  // +0x184: the pixels in a second, along the direction
	int mBoostFrames;         // +0x18c: the frames left of the click's speed
	engine::Point mHeading;   // +0x190: 25 pixels long, along the direction: how far ahead the boat looks
};

} // namespace pizzafrenzy

#endif
