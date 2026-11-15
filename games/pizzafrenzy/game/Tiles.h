#ifndef __PIZZAFRENZY_TILES_H__
#define __PIZZAFRENZY_TILES_H__

// The things a city file puts on the map: roads, bridges, customers' houses and kitchens, each a node (a Container) of
// its own that sits on the cells of the map (CityMap.h). A tile has a type name (its node's name: the tile manifest's
// entry it takes its pictures from) and a name of its own on the map, and keeps the cell it was placed on.

#include "DocHandler.h"
#include "ImageNode.h"
#include "Node.h"
#include "Selector.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class CityMap;

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

private:
	engine::Ref<engine::Selector> mRoadImages;    // +0x194: "RoadImages"
	engine::Ref<engine::Selector> mGroundImages;  // +0x198: "GroundImages"
	float mRaisedTime;                            // +0x19c: 0; nothing translated reads it
	bool mRaised;                                 // +0x1a0: false; nothing translated reads it
};

// A house of a customer (0x1dc bytes, constructor 0x41ba60, vtable 0x4fae00; its base 0x41c490, 0x1b8 bytes)
class CustomerTile : public Tile
{
public:
	explicit CustomerTile(const std::string& theTypeName);  // 0x41ba60
	bool accept(CityMap* theMap, const std::string& theInstanceName) override;  // 0x41bbe0

	engine::IntPair mParkingSpot;   // +0x198: the cell a van stops at
	std::string mCharacterName;     // +0x1b8: the character that lives there
};

// A kitchen (0x218 bytes, constructor 0x41cf90, vtable 0x4fafb8)
class KitchenTile : public Tile
{
public:
	explicit KitchenTile(const std::string& theTypeName);  // 0x41cf90
	bool accept(CityMap* theMap, const std::string& theInstanceName) override;  // 0x41d190
	void place(CityMap* theMap, const engine::IntPair& theCell) override;       // 0x41c720

	engine::IntPair mParkingSpot;  // +0x198: the cell the vans start from
	std::string mVanType;          // +0x1b8: "PizzaVan"

private:
	enum { SIGN_DIGITS = 5 };
	engine::Ref<engine::ImageNode> mSignBoard;     // +0x1d8: the board's place, with no picture yet
	engine::Ref<engine::Selector> mSignDigits;     // +0x1d4: the pictures of the numbers on the board
	engine::Ref<engine::ImageNode> mSignDigitNodes[SIGN_DIGITS];  // +0x1dc
	int mSignFrame;                                // +0x20c: -1, none
};

} // namespace pizzafrenzy

#endif
