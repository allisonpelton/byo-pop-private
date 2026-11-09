// port(variant): flame and lightning gems (Specials.h). Not part of the original game.

#include "Specials.h"

#include <algorithm>
#include <cstdlib>
#include <deque>

using namespace Sexy;
using namespace Sexy::Specials;

bool Specials::Matches(const MatchFinder::Group& theGroup, const MatchFinder::Spec& theSpec, const std::string& thePattern)
{
	if ((thePattern.size() > 3) && (thePattern.compare(0, 3, "run") == 0))
	{
		// GameRules accepts only "run2" .. "run16"
		int aLength = atoi(thePattern.c_str() + 3);
		return ((theGroup.mKind == MatchFinder::GROUP_VERTICAL) || (theGroup.mKind == MatchFinder::GROUP_HORIZONTAL)) &&
			(theGroup.mRunLength >= aLength);
	}
	switch (theGroup.mKind)
	{
	case MatchFinder::GROUP_DIAGONAL_DOWN:
	case MatchFinder::GROUP_DIAGONAL_UP:
		return thePattern == "diagonal";
	case MatchFinder::GROUP_L:
		return thePattern == "L";
	case MatchFinder::GROUP_T:
		return thePattern == "T";
	case MatchFinder::GROUP_PLUS:
		return thePattern == "plus";
	case MatchFinder::GROUP_SHAPE:
		return (theGroup.mShape >= 0) && (theGroup.mShape < (int) theSpec.mShapes.size()) && (theSpec.mShapes[theGroup.mShape].mId == thePattern);
	default:
		return false;
	}
}

int Specials::KindFor(const MatchFinder::Group& theGroup, const MatchFinder::Spec& theSpec, const GameRules::SpecialRules& theRules)
{
	const SpecialRule* aRules[2] = {&theRules.mLightning, &theRules.mFlame};
	const int aKinds[2] = {KIND_LIGHTNING, KIND_FLAME};
	for (int i = 0; i < 2; i++)
	{
		if (!aRules[i]->mEnabled)
			continue;
		for (const std::string& aPattern : aRules[i]->mCreatedBy)
		{
			if (Matches(theGroup, theSpec, aPattern))
				return aKinds[i];
		}
	}
	return KIND_NONE;
}

void Specials::Creations(const std::vector<MatchFinder::Group>& theGroups, const MatchFinder::Spec& theSpec, const GameRules::SpecialRules& theRules,
	const std::vector<Cell>& theMoved, const Grid& theGrid, std::vector<Creation>* theCreations)
{
	theCreations->clear();
	auto aFree = [&](const Cell& theCell) -> bool
	{
		if (theGrid.mKind[theCell.second][theCell.first] != KIND_NONE)
			return false;
		for (const Creation& c : *theCreations)
		{
			if (c.mCell == theCell)
				return false;
		}
		return true;
	};
	for (size_t g = 0; g < theGroups.size(); g++)
	{
		const MatchFinder::Group& aGroup = theGroups[g];
		int aKind = KindFor(aGroup, theSpec, theRules);
		if ((aKind == KIND_NONE) || aGroup.mCells.empty())
			continue;
		Creation aCreation;
		aCreation.mKind = aKind;
		aCreation.mGroup = (int) g;
		bool isFound = false;
		for (size_t i = 0; (i < theMoved.size()) && !isFound; i++)
		{
			if (aGroup.Contains(theMoved[i].first, theMoved[i].second) && aFree(theMoved[i]))
			{
				aCreation.mCell = theMoved[i];
				isFound = true;
			}
		}
		int aCount = (int) aGroup.mCells.size();
		for (int i = 0; (i < aCount) && !isFound; i++)
		{
			const Cell& aCell = aGroup.mCells[(aCount / 2 + i) % aCount];
			if (aFree(aCell))
			{
				aCreation.mCell = aCell;
				isFound = true;
			}
		}
		if (isFound)
			theCreations->push_back(aCreation);
	}
}

void Specials::Blasts(const std::vector<MatchFinder::Group>& theGroups, const BoardLayout& theLayout, const GameRules::SpecialRules& theRules,
	const std::vector<Creation>& theCreations, Grid* theGrid, std::vector<Blast>* theBlasts)
{
	theBlasts->clear();
	Grid& aGrid = *theGrid;
	bool isQueued[MatchFinder::MAX_ROWS][MatchFinder::MAX_COLS] = {};
	bool isCreated[MatchFinder::MAX_ROWS][MatchFinder::MAX_COLS] = {};
	for (const Creation& c : theCreations)
		isCreated[c.mCell.second][c.mCell.first] = true;
	std::deque<Cell> aQueue;
	auto aPush = [&](const Cell& theCell)
	{
		int c = theCell.first;
		int r = theCell.second;
		if (aGrid.mGem[r][c] && (aGrid.mKind[r][c] != KIND_NONE) && aGrid.mDying[r][c] && !isCreated[r][c] && !isQueued[r][c])
		{
			isQueued[r][c] = true;
			aQueue.push_back(theCell);
		}
	};
	for (const MatchFinder::Group& aGroup : theGroups)
	{
		for (const Cell& aCell : aGroup.mCells)
			aPush(aCell);
	}
	while (!aQueue.empty())
	{
		Blast aBlast;
		aBlast.mCell = aQueue.front();
		aQueue.pop_front();
		int aCol = aBlast.mCell.first;
		int aRow = aBlast.mCell.second;
		aBlast.mKind = aGrid.mKind[aRow][aCol];
		int aRadius = theRules.mFlame.mRadius;
		for (int aRowIndex = 0; aRowIndex < aGrid.mRows; aRowIndex++)
		{
			for (int aColIndex = 0; aColIndex < aGrid.mCols; aColIndex++)
			{
				bool isReached = (aBlast.mKind == KIND_FLAME) ? ((abs(aColIndex - aCol) <= aRadius) && (abs(aRowIndex - aRow) <= aRadius)) : ((aColIndex == aCol) || (aRowIndex == aRow));
				if (!isReached || !(theLayout.IsPlayable(aColIndex, aRowIndex) || theLayout.IsDirt(aColIndex, aRowIndex)))
					continue;
				aBlast.mArea.push_back(Cell(aColIndex, aRowIndex));
				if (!aGrid.mGem[aRowIndex][aColIndex] || aGrid.mDying[aRowIndex][aColIndex] || isCreated[aRowIndex][aColIndex])
					continue;
				aGrid.mDying[aRowIndex][aColIndex] = true;
				aBlast.mDestroyed.push_back(Cell(aColIndex, aRowIndex));
				if (theRules.mChain)
					aPush(Cell(aColIndex, aRowIndex));
			}
		}
		theBlasts->push_back(aBlast);
	}
}
