#include "CityMap.h"
#include "PizzaFrenzy.h"
#include "Random.h"

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
	mTilesStopped(false), mRows(0), mColumns(0), mCellSize(0), mNameCounter(0), mBackground(DEFAULT_BACKGROUND),
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
	mTilesStopped = false;
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

// 0x4122e0
void CityMap::update(engine::UpdateInfo& theInfo)
{
	if (!mTilesStopped)
	{
		for (std::map<std::string, engine::Ref<Tile>>::iterator anIter = mTiles.begin(); anIter != mTiles.end();
		     ++anIter)
			anIter->second->tick(theInfo);
		// the boats' frames (0x42e560) are translated next
	}
	// the vehicles' frames (slot 0x144 of each) are translated next
	for (size_t i = 0; i < mBoatSpawners.size(); i++)
		mBoatSpawners[i]->update(theInfo);
}

// 0x413fc0
void CityMap::collectKitchenToppings()
{
	mKitchenToppings.clear();
	for (size_t i = 0; i < mKitchens.size(); i++)
	{
		Topping* aTopping = static_cast<KitchenTile*>(mKitchens[i].get())->getTopping();
		if (aTopping == nullptr)
			continue;
		size_t aFound = 0;
		while (aFound < mKitchenToppings.size() && mKitchenToppings[aFound].get() != aTopping)
			aFound++;
		if (aFound == mKitchenToppings.size())
			mKitchenToppings.push_back(aTopping);
	}
}

// 0x412170
void CityMap::showCombo(Topping* theTopping, int theLength)
{
	for (size_t i = 0; i < mKitchens.size(); i++)
	{
		KitchenTile* aKitchen = static_cast<KitchenTile*>(mKitchens[i].get());
		aKitchen->showCombo(aKitchen->getTopping() == theTopping ? theLength : 0);
	}
}

// 0x412940
void CityMap::clearBoatSpawners()
{
	mBoatSpawners.clear();
}

// 0x411cd0
bool CityMap::isCrowded(Tile* theTile) const
{
	static const int NEIGHBOURS[4][2] = {{1, 0}, {-2, 0}, {0, -2}, {0, 1}};
	engine::IntPair aCell = theTile->getCell();
	for (int i = 0; i < 4; i++)
	{
		int aColumn = aCell.mFirst + NEIGHBOURS[i][0];
		int aRow = aCell.mSecond + NEIGHBOURS[i][1];
		if (-1 < aColumn && aColumn < mColumns && -1 < aRow && aRow < mRows)
		{
			Tile* aNeighbour = mGrid[aColumn][aRow].get();
			if (aNeighbour != nullptr && !aNeighbour->isEmpty())
				return true;
		}
	}
	return false;
}

// 0x413c00
CustomerTile* CityMap::findFreeCustomerTile()
{
	std::vector<CustomerTile*> aCandidates;
	for (size_t i = 0; i < mCustomers.size(); i++)
	{
		CustomerTile* aTile = static_cast<CustomerTile*>(mCustomers[i].get());
		if (aTile->isEmpty() && !isCrowded(aTile) && aTile->getCharacter()->mSpecial != Character::SPECIAL_POLICE)
			aCandidates.push_back(aTile);
	}
	if (aCandidates.empty())
		return nullptr;
	return aCandidates[engine::randomRange(0, (int)aCandidates.size() - 1)];
}

// 0x413d80
CustomerTile* CityMap::findCustomerTileBySpecial(int theSpecial)
{
	std::vector<CustomerTile*> aCandidates;
	for (size_t i = 0; i < mCustomers.size(); i++)
	{
		CustomerTile* aTile = static_cast<CustomerTile*>(mCustomers[i].get());
		if (aTile->getCharacter()->mSpecial == theSpecial)
			aCandidates.push_back(aTile);
	}
	if (aCandidates.empty())
		return nullptr;
	return aCandidates[engine::randomRange(0, (int)aCandidates.size() - 1)];
}

// 0x4120a0
Topping* CityMap::getRandomKitchenTopping()
{
	if (mKitchenToppings.empty())
		return nullptr;
	return mKitchenToppings[engine::randomRange(0, (int)mKitchenToppings.size() - 1)];
}

// 0x4120f0: the boats (the ones of the list at +0x68 that are raised, 0x160) are translated next
bool CityMap::isIdle()
{
	bool anIdle = true;
	for (size_t i = 0; i < mCustomers.size(); i++)
	{
		CustomerTile* aTile = static_cast<CustomerTile*>(mCustomers[i].get());
		if (!aTile->isEmpty() && !aTile->isOccupantFlagged() && aTile->getOccupant()->getState() != 2)
			anIdle = false;
	}
	return anIdle;
}
