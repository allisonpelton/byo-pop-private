#ifndef __PIZZAFRENZY_VAN_H__
#define __PIZZAFRENZY_VAN_H__

// The vans (0x18c bytes, constructor 0x430860, vtable 0x4fc6b8) and the police's car (0x198 bytes, constructor
// 0x432d60, vtable 0x4fcf68): a vehicle waits at the tile it starts from, drives along the roads to a house, waits
// there for a second and finishes the house's popup. The original keeps three states for it (parked 0x4fc820, driving 0x4fc834,
// arrived 0x4fc848); the port keeps them as a phase.
//
// Left out: the effect that follows a vehicle (the original's +0x168), the police car's flashing lights (the blinker
// 0x481e50 with the two light pictures, drawn beside the car), and the sort of the vehicle in its group when it has
// moved a row (0x435ba0). The canoe (0x431ad0) and the copter (0x431dc0) are not translated: a vehicle of their
// class is a van. The pathfinder is the function findPath, with the original's search (0x42a600).

#include "Node.h"
#include "Selector.h"
#include "TileEntries.h"
#include "Tiles.h"

#include <string>
#include <vector>

namespace pizzafrenzy
{

class BuildingTile;
class CityMap;
class CustomerTile;

// 0x42a600: the cells of the way from the start to the goal along the roads, the start the first, each a corner or a
// crossing or the goal (the vehicle drives straight from one to the next); none when there is no way
std::vector<engine::IntPair> findPath(CityMap* theMap, const engine::IntPair& theStart, const engine::IntPair& theGoal);

class Van : public engine::Container
{
public:
	explicit Van(const std::string& theTypeName);  // 0x430860
	~Van() override;                               // 0x42f920

	// slot 0x134: 0x42f690: the vehicle stands on the map at the cell, starting from the tile
	virtual void setup(CityMap* theMap, const engine::IntPair& theCell, BuildingTile* theTile);
	// slot 0x138: 0x42fb80: the vehicle is put in the cell
	virtual void placeAt(const engine::IntPair& theCell);
	// slot 0x13c: 0x42f770: the vehicle is taken off its group
	virtual void leave();
	// slot 0x140: 0x430070: the speed and the pictures of the type, and the vehicle waits parked
	virtual void loadType();
	// slot 0x144: 0x42f780: a frame of the vehicle: where it drives, or how long it has arrived, and its node's
	void updateVehicle(engine::UpdateInfo& theInfo);
	// slot 0x148: 0x430480
	virtual void arriveSound();
	// slot 0x14c: 0x42fd20: the house is finished, and the vehicle goes back to the tile
	virtual void finish();
	// slot 0x150: 0x430130: the way to the cell is found, and the vehicle faces along it
	virtual void startPath(const engine::IntPair& theCell);
	// slot 0x154: 0x4302f0: the vehicle drives to the house (not when it is on its way, or at the house's road already)
	virtual void goTo(CustomerTile* theTile);
	// slot 0x158: 0x42f900: the way from the vehicle's cell to the cell
	virtual std::vector<engine::IntPair> findWay(const engine::IntPair& theCell);
	// slot 0x15c: 0x42f700: the vehicle faces the first road next to its cell
	virtual void faceRoad();
	// slot 0x160: 0x42f940: the pictures of the vehicle's type, east and west, north and south
	virtual void makeImages(Vehicle* theType);
	// slot 0x164: 0x42fc30: the vehicle faces the cell next to it
	virtual void face(const engine::IntPair& theCell);

	// 0x42f8f0: the vehicle is not on its way
	bool isIdle() const { return mPath.empty(); }
	int getLayer() const { return mLayer; }  // +0x158: 3, the ground's
	// 0x42f650: the map lets go of the vehicle
	void clearParent(engine::Node* theParent) override;

protected:
	enum Phase
	{
		PHASE_PARKED,
		PHASE_DRIVING,
		PHASE_ARRIVED
	};
	// 0x42fda0 (parked's entry)
	void park();
	// 0x430510: the vehicle drives on for the seconds
	void move(engine::UpdateInfo& theInfo);
	// 0x42fe60 (arrived's entry)
	void arrive();

	CityMap* mMap;                        // +0x128
	engine::IntPair mCell;                // +0x12c: the cell the vehicle is in
	engine::IntPair mHomeCell;            // +0x134: where it starts from
	std::vector<engine::IntPair> mPath;   // +0x13c: the way it drives, none when it is not on its way
	size_t mNext;                         // +0x148: the cell of the way it drives to
	float mProgress;                      // +0x140: how far from the last cell to the next, from 0 to 1
	float mSpeed;                         // +0x144: the pixels in a second
	bool mDirty;                          // +0x14c: the vehicle moved a row of the screen
	engine::IntPair mSegmentStart;        // +0x150: the cell the vehicle drove on from
	int mLayer;                           // +0x158
	engine::Ref<CustomerTile> mTarget;    // +0x15c: the house it drives to
	BuildingTile* mHome;                  // +0x160: the tile it starts from
	engine::Ref<engine::Selector> mImages;  // +0x164: east and west, north, south, and the parked one
	Phase mPhase;                         // +0x16c
	float mArrivedTime;                   // the seconds the vehicle has left at the house
};

// The police's car: finishes the house with an arrest, or with a false one
class PoliceCar : public Van
{
public:
	PoliceCar();  // 0x432d60
	// slot 0x14c: 0x432e30: the arrest is true for a customer whose order is not for pizza, and false for any other:
	// the customer hangs up (the splat "falseArrest" is left out)
	void finish() override;
	// slot 0x154: 0x433010: with the siren's sound
	void goTo(CustomerTile* theTile) override;
};

} // namespace pizzafrenzy

#endif
