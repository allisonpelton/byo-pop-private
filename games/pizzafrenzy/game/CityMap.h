#ifndef __PIZZAFRENZY_CITYMAP_H__
#define __PIZZAFRENZY_CITYMAP_H__

// The map of a city (0x148 bytes, constructor 0x414260, vtable 0x4fa66c; the application keeps it at +0x11c): a grid of
// cells that the tiles of a city file sit on, the tiles by name and by kind, and the decorations. The city file's load
// (CityFile.cpp) fills it and the game screen draws from it.

#include "BoatSpawner.h"
#include "Decoration.h"
#include "DocHandler.h"
#include "Object.h"
#include "Tiles.h"

#include <map>
#include <string>
#include <vector>

namespace pizzafrenzy
{

class CityMap : public engine::Object
{
public:
	CityMap();            // 0x414260
	~CityMap() override;  // 0x413670

	// 0x413270: every tile and decoration is let go
	void clear();
	// 0x4139d0: a new grid of the size, which the map starts empty on; the offset moves the cells on the screen
	void setGrid(int theColumns, int theRows, int theCellSize, const engine::IntPair& theOffset, int theUniqueId);
	// 0x412710: the picture the ground is drawn on, and the colour shift of it (the text "hue,saturation,value")
	void setBackground(const std::string& theBackground, const std::string& theHsvShift);

	int getRows() const { return mRows; }                             // 0x411960
	int getColumns() const { return mColumns; }                       // 0x411970
	int getCellSize() const { return mCellSize; }                     // 0x411980
	const std::string& getBackground() const { return mBackground; }  // 0x40ddc0
	const std::string& getHsvShift() const { return mHsvShift; }      // 0x435c20

	// 0x4124c0: the next name for a tile that has none
	std::string nextName();
	// 0x411a80: the point on the screen at the bottom right of the cell ((0, 0) for a cell that is not on the grid)
	engine::IntPair cellToPoint(const engine::IntPair& theCell) const;
	// 0x411c30: a road is at the cell
	bool isRoadAt(const engine::IntPair& theCell) const;
	// 0x414a30: the tile covers the cells from the cell on (it takes the place of any tile on them) and is registered
	void place(const engine::IntPair& theCell, Tile* theTile);
	// 0x413a80
	void addDecoration(Decoration* theDecoration);

	// 0x4122e0 (slot 0x4): a frame of the map: its tiles (unless they are stopped), and the boat spawners; the boats
	// and the vehicles are translated next
	void update(engine::UpdateInfo& theInfo);
	// 0x413fc0: the toppings the kitchens make are listed, each once
	void collectKitchenToppings();
	// 0x412170: the kitchens that make the topping show the length of the combo on their signs, the others none
	void showCombo(Topping* theTopping, int theLength);
	// 0x412940: the boat spawners of the level are let go
	void clearBoatSpawners();
	// 0x413f40: the spawner is kept and updated
	void addBoatSpawner(BoatSpawner* theSpawner) { mBoatSpawners.push_back(theSpawner); }

	// 0x413c00: a house to order from, drawn from the customers' that have no popup, have no house with one in the
	// cells beside them and are not the police's; none when there is no such house
	CustomerTile* findFreeCustomerTile();
	// 0x413d80: a house, drawn from the customers' whose character is of the kind (Character::SPECIAL_...); none when
	// there is none
	CustomerTile* findCustomerTileBySpecial(int theSpecial);
	// 0x4120a0: one of the kitchens' toppings, drawn; none when there are none
	Topping* getRandomKitchenTopping();
	// 0x4120f0: the houses have nothing left to wait for (no popup that is not about to go) and no boat is raised
	bool isIdle();

	// Left for the game's play: 0x413b00 (the boats, +0x68), 0x413b80 (the vehicles, +0xbc) and 0x429cf0 (the
	// pathfinder's grid, +0x104, which setGrid fills)

	// the tiles by their names (+0x2c), which the game screen takes its nodes from in this order
	std::map<std::string, engine::Ref<Tile>> mTiles;
	std::vector<engine::Ref<Tile>> mKitchens;           // +0x38
	std::vector<engine::Ref<Tile>> mCustomers;          // +0x48
	std::vector<engine::Ref<Tile>> mRoads;              // +0x58
	std::vector<engine::Ref<Tile>> mBridges;            // +0x78
	std::vector<engine::Ref<BoatSpawner>> mBoatSpawners;  // +0x88
	std::vector<engine::Ref<Topping>> mKitchenToppings;  // +0x9c: the toppings the kitchens make, each once
	std::vector<engine::Ref<Decoration>> mDecorations;  // +0xac
	// +0x13c: the tiles' frames stop while it is set; nothing translated sets it
	bool mTilesStopped;

private:
	// 0x414720: the tile is taken off the cells it covers and unregistered
	void removeTile(Tile* theTile);
	// 0x4144b0
	void unregisterTile(Tile* theTile);
	// 0x414830
	void registerTile(Tile* theTile);
	// 0x411cd0: a house stands in one of the four cells beside the tile that has a popup (one to the right, two to
	// the left, two up and one down)
	bool isCrowded(Tile* theTile) const;
	static void removeFrom(std::vector<engine::Ref<Tile>>& theTiles, Tile* theTile);
	bool inGrid(const engine::IntPair& theCell) const
	{
		return -1 < theCell.mFirst && theCell.mFirst < mColumns && -1 < theCell.mSecond && theCell.mSecond < mRows;
	}

	int mRows;                                          // +0x0c
	int mColumns;                                       // +0x10
	int mCellSize;                                      // +0x14
	engine::IntPair mOffset;                            // +0x18
	std::vector<std::vector<engine::Ref<Tile>>> mGrid;  // +0x20: a column of rows
	int mNameCounter;                                   // +0x98
	std::string mBackground;                            // +0xcc
	std::string mHsvShift;                              // +0xe8
};

} // namespace pizzafrenzy

#endif
