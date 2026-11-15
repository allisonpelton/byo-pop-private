#ifndef __PIZZAFRENZY_CITYMAP_H__
#define __PIZZAFRENZY_CITYMAP_H__

// The map of a city (0x148 bytes, constructor 0x414260, vtable 0x4fa66c; the application keeps it at +0x11c): a grid of
// cells that the tiles of a city file sit on, the tiles by name and by kind, and the decorations. The city file's load
// (CityFile.cpp) fills it and the game screen draws from it.

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

	// Left for the game's play: 0x413b00 and 0x413f40 (the lists at +0x68 and +0x88), 0x413b80 (the vehicles, +0xbc),
	// 0x413fc0 (what a kitchen's registration and unregistering end with) and 0x429cf0 (the pathfinder's grid, +0x104,
	// which setGrid fills)

	// the tiles by their names (+0x2c), which the game screen takes its nodes from in this order
	std::map<std::string, engine::Ref<Tile>> mTiles;
	std::vector<engine::Ref<Tile>> mKitchens;           // +0x38
	std::vector<engine::Ref<Tile>> mCustomers;          // +0x48
	std::vector<engine::Ref<Tile>> mRoads;              // +0x58
	std::vector<engine::Ref<Tile>> mBridges;            // +0x78
	std::vector<engine::Ref<Decoration>> mDecorations;  // +0xac

private:
	// 0x414720: the tile is taken off the cells it covers and unregistered
	void removeTile(Tile* theTile);
	// 0x4144b0
	void unregisterTile(Tile* theTile);
	// 0x414830
	void registerTile(Tile* theTile);
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
