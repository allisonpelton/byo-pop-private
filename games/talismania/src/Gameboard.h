#ifndef __TALISMANIA_GAMEBOARD_H__
#define __TALISMANIA_GAMEBOARD_H__

// The game board (Gameboard, built by 0x4aae10 from its GameboardClass): a grid of pots laid out as hexagons, each pot
// holding a cell. A column's pots stand 72 pixels apart and the columns 62 apart (the board class's pot size), the odd
// columns half a pot higher; row 0 is the bottom and the board's origin the middle of the bottom pot of the first column.
// Translated so far: the pots laid out from the column templates and the layout's own pots, and each pot's first cell
// drawn from its class's layers shown in the "normal" view mode. Not yet: the cells' paths, falling, the side pots'
// paths, the spawning pots and any play.

#include "AtlantisData.h"
#include "Container.h"
#include "Engine.h"

#include <map>
#include <vector>

class AtlantisApp;

// One place of the grid after the layout is laid out
struct BoardPot
{
	engine::Vector2<int> position;  // column and row
	PotClass props;
	std::string cellClass;  // the class of the cell standing in it ("" for none)
	std::string pathId;
};

class Gameboard
{
public:
	// The pots laid out, and a cell drawn in each into theCells
	Gameboard(AtlantisApp* theApp, const BoardClass& theClass, engine::Container* theCells);

	const std::vector<BoardPot>& getPots() const { return mPots; }
	// Where a pot's middle is, from the board's origin
	engine::Vector2f getPotPosition(const engine::Vector2<int>& thePosition) const;

private:
	void layOut();
	void drawCell(const BoardPot& thePot);
	SpawnItem pickSpawn(const PotClass& theProps) const;
	std::string pickPathId(const std::string& thePoolName) const;
	void drawPath(const std::string& thePathClass, const std::string& thePathId, const engine::Vector2f& theMiddle);

	AtlantisApp* mApp;
	BoardClass mClass;
	engine::Container* mCells;
	std::vector<BoardPot> mPots;
};

#endif
