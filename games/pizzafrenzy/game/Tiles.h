#ifndef __PIZZAFRENZY_TILES_H__
#define __PIZZAFRENZY_TILES_H__

// The things a city file puts on the map: roads, bridges, customers' houses and kitchens, each a node (a Container) of
// its own that sits on the cells of the map (CityMap.h). A tile has a type name (its node's name: the tile manifest's
// entry it takes its pictures from) and a name of its own on the map, and keeps the cell it was placed on.

#include "DocHandler.h"
#include "ImageNode.h"
#include "Node.h"
#include "PizzaPopup.h"
#include "Selector.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class Van;

class CityMap;
class PizzaPopup;

// The layers a tile has a node for, each drawn by the game screen (GameScreen.h)
enum TileLayer
{
	LAYER_GROUND_BELOW = 0,  // drawn once into the picture of the ground, below the others
	LAYER_GROUND_ABOVE = 1,  // drawn once into the picture of the ground
	LAYER_ROAD = 2,          // the game screen's road group
	LAYER_GROUND = 3,        // the ground group, whose nodes are sorted
	LAYER_AIR = 4,           // the air group
	LAYER_SFX = 5            // the effects group
};

// A tile of the map (0x190 bytes and more, constructor 0x41b570, vtable 0x4faaa0)
class Tile : public engine::Container
{
public:
	Tile();  // 0x41b570

	const std::string& getKind() const { return mKind; }                 // 0x41af80: "roads", "customer", ...
	const std::string& getInstanceName() const { return mInstanceName; }  // 0x41af90
	// 0x41b320: what kind of tile it is, which the map keeps it under
	void setKind(const std::string& theKind) { mKind = theKind; }

	// slot 0x130: the tile takes its pictures and size from the manifest's entry of its type name, and its name on
	// the map; false when the manifest has none (each kind of tile has its own)
	virtual bool accept(CityMap* theMap, const std::string& theInstanceName) = 0;
	// slot 0x134: 0x41b050; the node it has in the layer, or none
	virtual engine::Node* getLayerNode(int theLayer);
	// slot 0x13c: 0x41b4c0; the tile gets a name on the map when it has none, is put at the cell and the map is told
	virtual void place(CityMap* theMap, const engine::IntPair& theCell);
	// slot 0x140: 0x41b070; the tile's position on the screen from the cell
	virtual void locate(CityMap* theMap, const engine::IntPair& theCell);
	// slot 0x144: what the tile draws depends on the tiles around it; empty here (no kind that has more is translated
	// but the road's)
	virtual void prepare() {}
	// slot 0x154: 0x454e60 (false) here; the road and the bridge: whether a road is at the cell
	virtual bool isRoadAt(const engine::IntPair& theCell) { return false; }
	// slot 0x150: 0x41b230 (-1) here; the road and the bridge: what it costs a vehicle to drive over the cell
	virtual int getWeight(const engine::IntPair& theCell) { return -1; }
	// slot 0x158: false here; the bridge: a raised one blocks the vehicles
	virtual bool isBlocked(const engine::IntPair& theCell) { return false; }
	// slot 0x164: 0x492310 (nothing) here: a frame of the tile, from the map's update
	virtual void tick(engine::UpdateInfo& theInfo) {}
	// slot 0x168: 0x451610 (true) here; the buildings: nothing stands on the tile
	virtual bool isEmpty() { return true; }
	// slot 0x15c
	virtual engine::IntPair getCell() { return mCell; }

	// 0x41b260: the manifest's entry gives the tile its size, cells, offsets and picture, and the name it is called
	bool setTileType(TileType* theEntry, const std::string& theInstanceName);

	CityMap* mMap;                    // +0x160
	engine::IntPair mCell;            // +0x164: (-1, -1) before it is placed
	engine::IntPair mBottomRight;     // +0x16c: the offset from the cell's point to the picture's bottom right
	engine::IntPair mBase;            // +0x174
	engine::IntPair mDimensions;      // +0x17c: how many cells it covers, across and down
	engine::Ref<engine::Image> mImage;  // +0x184
	engine::Ref<engine::Node> mSfxNode;  // +0x188: the node of the effects layer
	int mImageWidth;                  // +0x18c
	int mImageHeight;                 // +0x190

protected:
	std::string mKind;          // +0x128
	std::string mInstanceName;  // +0x144
};

// A road (0x1a4 bytes, constructor 0x426f30, vtable 0x4fb800): one picture of the tile type's sheet, by the roads
// next to it
class RoadTile : public Tile
{
public:
	explicit RoadTile(const std::string& theTypeName);  // 0x426f30
	bool accept(CityMap* theMap, const std::string& theInstanceName) override;  // 0x426c90
	engine::Node* getLayerNode(int theLayer) override;                          // 0x426c10: layer 1 only
	void prepare() override;                                                   // 0x426d30
	bool isRoadAt(const engine::IntPair& theCell) override { return true; }
	int getWeight(const engine::IntPair& theCell) override { return mWeight; }  // 0x426ea0

private:
	engine::Ref<engine::ImageNode> mRoadNode;  // +0x194
	int mWeight;                               // +0x198: 1 (the city file's weight is read and not kept)
};

// A bridge (0x1ac bytes, constructor 0x4266e0, vtable 0x4fb658): a road and the ground under it, each with the way
// blocked or not
class BridgeTile : public Tile
{
public:
	explicit BridgeTile(const std::string& theTypeName);  // 0x4266e0
	bool accept(CityMap* theMap, const std::string& theInstanceName) override;  // 0x4267a0
	engine::Node* getLayerNode(int theLayer) override;                          // 0x4265c0: layers 3 and 2
	void place(CityMap* theMap, const engine::IntPair& theCell) override;       // 0x426580
	bool isRoadAt(const engine::IntPair& theCell) override { return true; }
	int getWeight(const engine::IntPair& theCell) override { return 1; }                // 0x426550
	bool isBlocked(const engine::IntPair& theCell) override { return mRaised; }         // 0x4263e0
	// slot 0x164: 0x4263f0: the time the bridge is raised runs out, and it is let down
	void tick(engine::UpdateInfo& theInfo) override;
	// slot 0x16c: 0x426660: a vehicle of the map is on the bridge
	virtual bool hasVehicle();
	// slot 0x170: 0x4265e0: the bridge is raised (the way blocked, for two seconds) or let down
	virtual void setRaised(bool theRaised);
	// slot 0x174: 0x426440: a boat's passing: false lets the bridge down; true raises it unless a vehicle is on it, and
	// says whether the bridge was raised (letting it down always succeeds)
	virtual bool raise(bool theRaise);

private:
	engine::Ref<engine::Selector> mRoadImages;    // +0x194: "RoadImages"
	engine::Ref<engine::Selector> mGroundImages;  // +0x198: "GroundImages"
	float mRaisedTime;                            // +0x19c: 0, and the seconds a raised bridge has left
	bool mRaised;                                 // +0x1a0: false; the way is blocked while it is set
};

// What a customer's house and a kitchen have in common (0x1b8 bytes, constructor 0x41c490, vtable 0x4fae00): the cell a
// van parks at, the vans of the tile, and the popup that stands on it (a customer with the order, the kitchen's)
class BuildingTile : public Tile
{
public:
	BuildingTile();  // 0x41c490

	// slot 0x164: 0x41c180: the popup has its frame; a popup that waits takes the place when there is none
	void tick(engine::UpdateInfo& theInfo) override;
	// slot 0x168: 0x41be10
	bool isEmpty() override { return mOccupant == nullptr; }
	// slot 0x188: 0x41bfa0: the popup is put on the tile, scaled to nothing and at the bottom of the cell its position
	// is in; when the tile has a popup already, it waits (slot 0x1a0 of 0x41c560 is the vans')
	virtual void setOccupant(PizzaPopup* thePopup);
	// slot 0x18c: 0x41c0e0
	PizzaPopup* getOccupant() { return mOccupant; }
	// slot 0x190: 0x41c0f0
	virtual void clearOccupant();
	// slot 0x17c: 0x41bf10: the popup takes a click
	bool isClickable();
	// slot 0x194: 0x41c120: the popup is taken off the tile at the next update
	virtual void removeOccupant();
	// slot 0x198: 0x41c140: the popup hangs up
	void hangUp();
	// slot 0x170: 0x41bf50: the popup is picked
	void pick();
	// slot 0x174: 0x41bf70: the popup's pick, if it is picked, is let go
	void unpick();
	// slot 0x1a4: 0x41bd40: the cell on the road where the vehicles of the tile stop and start: the one its parking
	// spot gives when that has a road, else the nearest road within four cells across and down; (-1, -1) for none
	engine::IntPair getRoadCell();
	// slot 0x19c: 0x41c160: the flag of the popup (PizzaPopup.h), false for a tile without one
	bool isOccupantLeaving();

	// slot 0xa8: 0x41bd20: the click is the mode's
	void onMouseDownEvent(engine::MouseEvent& theEvent) override;

	// 0x41c3d0: the vans are let go
	void clearVehicles();
	// slot 0x1a0: 0x41c560: the van is kept, put on the map at the road cell and on the screen
	void addVehicle(const engine::IntPair& theCell, Van* theVan);
	// slot 0x1a8: 0x41c460: the van is let go of and told to leave
	void removeVehicle(Van* theVan);

	engine::IntPair mParkingSpot;  // +0x198: the cell a van stops at

protected:
	// +0x1a0: the vans that start from the tile
	std::vector<engine::Ref<engine::Node>> mVehicles;
	engine::Ref<PizzaPopup> mOccupant;  // +0x1b0
	engine::Ref<PizzaPopup> mWaiting;   // +0x1b4: takes the place of the popup when the tile has none
};

// A house of a customer (0x1dc bytes, constructor 0x41ba60, vtable 0x4fac18)
class CustomerTile : public BuildingTile
{
public:
	explicit CustomerTile(const std::string& theTypeName);  // 0x41ba60
	bool accept(CityMap* theMap, const std::string& theInstanceName) override;  // 0x41bbe0

	// 0x41b7b0: the customer who lives there
	Character* getCharacter();
	// slot 0x1bc: 0x41b940: the order of the popup, when the tile's popup is a customer's
	Order* getOrder();
	// slot 0x1b8: 0x41b8f0: the same when the popup takes a click
	Order* getActiveOrder();
	// slot 0x1ac: 0x41b6a0: a customer's popup is made for the order (dimmed: it takes no click) and put on the tile
	void setOrder(Order* theOrder, bool theDimmed);
	// slot 0x1b0: 0x41b730: the police's popup is put on the tile when the kind is the police's
	void callPolice(int theSpecial);
	// slot 0x1b4: 0x41b820: the popup is served: the pizza is on its way
	void serve();
	// slot 0x1c0: 0x41b840: a customer's popup finishes, and the police's own customer, when other orders of the
	// events are pending, brings the police's popup back
	void finish();
	// slot 0x1c4: 0x41b8b0: a customer's popup is made impatient, or hangs up
	void setImpatient(bool theImpatient);
	// slot 0x1c8: 0x41b980: a customer's popup flashes
	void flash();

	std::string mCharacterName;     // +0x1b8: the character that lives there
};

// A kitchen (0x218 bytes, constructor 0x41cf90, vtable 0x4fafb8)
class KitchenTile : public BuildingTile
{
public:
	explicit KitchenTile(const std::string& theTypeName);  // 0x41cf90
	bool accept(CityMap* theMap, const std::string& theInstanceName) override;  // 0x41d190
	void place(CityMap* theMap, const engine::IntPair& theCell) override;       // 0x41c720
	// slot 0x164: 0x41c7b0: the building's frame, and the sign's time that runs out
	void tick(engine::UpdateInfo& theInfo) override;

	// 0x41c6f0: the topping the kitchen's popup makes, null when it has none
	Topping* getTopping();
	// 0x41c640: the kitchen's popup is made for the topping and put on the tile
	void setUpPopup(Topping* theTopping);
	// 0x41d170
	void setVanType(const std::string& theVanType) { mVanType = theVanType; }
	// 0x41cb80: the sign shows the length of the combo (none for 0): the number in its row of five, blinking, in the
	// colour its row gives
	void showCombo(int theLength);
	// 0x41c9e0: a van of the kitchen that is not on its way, or a new one of the kitchen's type (a copter, a canoe or
	// a van), put on the map
	Van* getVehicle();
	// port (the test commands' state): a van of the kitchen is on its way
	bool isVanOut() const;
	// slot 0x1ac: 0x41cf50: a van goes from the kitchen to the house, and the house and the kitchen's popup are served
	void sendVan(CustomerTile* theTile);

	std::string mVanType;          // +0x1b8: "PizzaVan"

private:
	enum { SIGN_DIGITS = 5 };
	engine::Ref<engine::ImageNode> mSignBoard;     // +0x1d8: the board's place, with no picture yet
	engine::Ref<engine::Selector> mSignDigits;     // +0x1d4: the pictures of the numbers on the board
	engine::Ref<engine::ImageNode> mSignDigitNodes[SIGN_DIGITS];  // +0x1dc
	engine::Ref<engine::Image> mSignDigitImages[SIGN_DIGITS];     // +0x1f0: the digits' pictures, shifted in hue
	engine::Ref<engine::Image> mSignImage;                        // +0x204: the board's picture, shifted in hue
	int mSignFrame;                                // +0x20c: -1, none
	float mSignTime;                               // +0x208: the seconds the sign still shows its number
};

} // namespace pizzafrenzy

#endif
