#include "CityMap.h"

#include <cstdio>

using namespace pizzafrenzy;

namespace
{
const char* const DEFAULT_BACKGROUND = "res\\Themes\\Suburbs\\bgPizzaSuburbs.jpg";
const char* const DEFAULT_HSV_SHIFT = "0,0,0";
const char* const KIND_KITCHEN = "kitchen";
const char* const KIND_ROAD = "roads";
const char* const KIND_CUSTOMER = "customer";
const char* const KIND_BRIDGE = "bridge";
}

// 0x414260
CityMap::CityMap() :
	mRows(0), mColumns(0), mCellSize(0), mNameCounter(0), mBackground(DEFAULT_BACKGROUND),
	mHsvShift(DEFAULT_HSV_SHIFT)
{
	mOffset.mFirst = 0;
	mOffset.mSecond = 0;
}

// 0x413670
CityMap::~CityMap()
{
}

// 0x413270: the grid has no cells, and the names and the lists are empty
void CityMap::clear()
{
	mGrid.clear();
	mTiles.clear();
	mKitchens.clear();
	mCustomers.clear();
	mRoads.clear();
	mDecorations.clear();
	mBridges.clear();
}

// 0x4139d0
void CityMap::setGrid(int theColumns, int theRows, int theCellSize, const engine::IntPair& theOffset, int theUniqueId)
{
	clear();
	mCellSize = theCellSize;
	mRows = theRows;
	mColumns = theColumns;
	mOffset = theOffset;
	mNameCounter = theUniqueId;
	mGrid.assign(mColumns, std::vector<engine::Ref<Tile>>(mRows));
	// the original also fills its pathfinder's grid (0x429cf0) from the map here: translated with the game
}

// 0x412710
void CityMap::setBackground(const std::string& theBackground, const std::string& theHsvShift)
{
	mBackground = theBackground;
	mHsvShift = theHsvShift;
}

// 0x4124c0
std::string CityMap::nextName()
{
	char aName[32];
	snprintf(aName, sizeof(aName), "Tile%d", mNameCounter++);
	return aName;
}

// 0x411a80
engine::IntPair CityMap::cellToPoint(const engine::IntPair& theCell) const
{
	engine::IntPair aPoint = {0, 0};
	if (inGrid(theCell))
	{
		aPoint.mFirst = (theCell.mFirst + 1) * mCellSize - 1 + mOffset.mFirst;
		aPoint.mSecond = (theCell.mSecond + 1) * mCellSize - 1 + mOffset.mSecond;
	}
	return aPoint;
}

// 0x411c30
bool CityMap::isRoadAt(const engine::IntPair& theCell) const
{
	if (!inGrid(theCell))
		return false;
	Tile* aTile = mGrid[theCell.mFirst][theCell.mSecond];
	return aTile != nullptr && aTile->isRoadAt(theCell);
}

// 0x414a30
void CityMap::place(const engine::IntPair& theCell, Tile* theTile)
{
	for (int aRow = 0; aRow < theTile->mDimensions.mSecond; aRow++)
	{
		for (int aColumn = 0; aColumn < theTile->mDimensions.mFirst; aColumn++)
		{
			engine::IntPair aCovered = {theCell.mFirst + aColumn, theCell.mSecond + aRow};
			if (inGrid(aCovered))
			{
				engine::Ref<Tile> aPlaced = mGrid[aCovered.mFirst][aCovered.mSecond];
				if (aPlaced != nullptr)
					removeTile(aPlaced);
				mGrid[aCovered.mFirst][aCovered.mSecond] = theTile;
			}
		}
	}
	registerTile(theTile);
}

// 0x414720
void CityMap::removeTile(Tile* theTile)
{
	engine::IntPair aCell = theTile->getCell();
	for (int aRow = 0; aRow < theTile->mDimensions.mSecond; aRow++)
	{
		for (int aColumn = 0; aColumn < theTile->mDimensions.mFirst; aColumn++)
		{
			engine::IntPair aCovered = {aCell.mFirst + aColumn, aCell.mSecond + aRow};
			if (inGrid(aCovered))
				mGrid[aCovered.mFirst][aCovered.mSecond] = nullptr;
		}
	}
	unregisterTile(theTile);
}

// 0x4144b0: the erase of unregisterTile, once for each list
void CityMap::removeFrom(std::vector<engine::Ref<Tile>>& theTiles, Tile* theTile)
{
	for (size_t i = 0; i < theTiles.size(); i++)
	{
		if (theTiles[i] == theTile)
		{
			theTiles.erase(theTiles.begin() + i);
			return;
		}
	}
}

// 0x4144b0
void CityMap::unregisterTile(Tile* theTile)
{
	mTiles.erase(theTile->getInstanceName());
	if (theTile->getKind() == KIND_KITCHEN)
	{
		removeFrom(mKitchens, theTile);
		// the original goes on to 0x413fc0 here: translated with the game
	}
	else if (theTile->getKind() == KIND_ROAD)
		removeFrom(mRoads, theTile);
	else if (theTile->getKind() == KIND_CUSTOMER)
		removeFrom(mCustomers, theTile);
	else if (theTile->getKind() == KIND_BRIDGE)
		removeFrom(mBridges, theTile);
}

// 0x414830
void CityMap::registerTile(Tile* theTile)
{
	mTiles[theTile->getInstanceName()] = theTile;
	if (theTile->getKind() == KIND_KITCHEN)
		mKitchens.push_back(theTile);
	else if (theTile->getKind() == KIND_ROAD)
		mRoads.push_back(theTile);
	else if (theTile->getKind() == KIND_CUSTOMER)
		mCustomers.push_back(theTile);
	else if (theTile->getKind() == KIND_BRIDGE)
		mBridges.push_back(theTile);
}

// 0x413a80
void CityMap::addDecoration(Decoration* theDecoration)
{
	mDecorations.push_back(theDecoration);
}
