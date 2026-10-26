// port(variant): the moves of the move rules (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.4, 4.5 and 4.7, stage 10);
// see MoveSet.h. Not part of the original game.

#include "MoveSet.h"

#include <algorithm>

using namespace Sexy;

namespace
{

// cos, sin x 1024 at 0, 5, ..., 90 degrees, as (int)(cos(d * pi / 180) * 1024) gives them in double precision
const int gTwistTable[MoveSet::TWIST_STEPS + 1][2] =
{
	{1024, 0}, {1020, 89}, {1008, 177}, {989, 265}, {962, 350}, {928, 432}, {886, 511}, {838, 587}, {784, 658},
	{724, 724}, {658, 784}, {587, 838}, {512, 886}, {432, 928}, {350, 962}, {265, 989}, {177, 1008}, {89, 1020},
	{0, 1024}
};

// The scan of MoveSet.h's order: theStopAt the index of the move to return (into theFound), or -1 for all of them
// (into theAll). True when the theStopAt-th move was found.
bool Scan(const MatchFinder::ColorGrid& theGrid, const BoardLayout& theLayout, const MatchFinder::Spec& theSpec,
	const MoveSet::Options& theOptions, int theStopAt, MoveSet::Found* theFound, std::vector<MoveSet::Found>* theAll)
{
	static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

	MatchFinder::ColorGrid aGrid = theGrid;
	int aCount = 0;
	// One move found: the gem from theFrom lands at theTo. True when the scan stops
	auto aTake = [&](const MoveSet::Move& theMove, const MoveSet::Cell& theTo, const MoveSet::Cell& theFrom) -> bool
	{
		MoveSet::Found aFound;
		aFound.mMove = theMove;
		aFound.mTo = theTo;
		aFound.mFrom = theFrom;
		if (theAll != NULL)
			theAll->push_back(aFound);
		if (aCount++ != theStopAt)
			return false;
		if (theFound != NULL)
			*theFound = aFound;
		return true;
	};
	// The move made on aGrid: its first moved gem that lands in a group (theTo, theFrom); aGrid is restored
	auto aLands = [&](const MoveSet::Move& theMove, MoveSet::Cell* theTo, MoveSet::Cell* theFrom) -> bool
	{
		MoveSet::Cell aFrom[MoveSet::MAX_MOVED];
		MoveSet::Cell aTo[MoveSet::MAX_MOVED];
		int aMoved = MoveSet::MovedCells(theMove, aFrom, aTo);
		int aColors[MoveSet::MAX_MOVED];
		for (int i = 0; i < aMoved; i++)
			aColors[i] = aGrid.mColor[aFrom[i].second][aFrom[i].first];
		for (int i = 0; i < aMoved; i++)
			aGrid.mColor[aTo[i].second][aTo[i].first] = aColors[i];
		int aHit = -1;
		for (int i = 0; (i < aMoved) && (aHit < 0); i++)
		{
			if (MatchFinder::InGroup(aGrid, theSpec, aTo[i].first, aTo[i].second, theOptions.mHorizontal, theOptions.mVertical))
				aHit = i;
		}
		for (int i = 0; i < aMoved; i++)
			aGrid.mColor[aFrom[i].second][aFrom[i].first] = aColors[i];
		if (aHit < 0)
			return false;
		*theTo = aTo[aHit];
		*theFrom = aFrom[aHit];
		return true;
	};
	auto aGem = [&](int theCol, int theRow) -> bool
	{
		return theLayout.IsPlayable(theCol, theRow) && (aGrid.At(theCol, theRow) != MatchFinder::NO_GEM);
	};

	// The swaps: stage 9's FindMoveVariant, the original's order and test
	if (theOptions.mTypes & GameRules::MOVE_SWAP)
	{
		for (int aRow = 0; aRow < aGrid.mRows; aRow++)
		{
			for (int aCol = 0; aCol < aGrid.mCols; aCol++)
			{
				if (aGrid.mColor[aRow][aCol] == MatchFinder::NO_GEM)
					continue;
				for (int aDir = 0; aDir < 4; aDir++)
				{
					int aCol2 = aCol + aDirs[aDir][0];
					int aRow2 = aRow + aDirs[aDir][1];
					if ((aCol2 < 0) || (aCol2 >= aGrid.mCols) || (aRow2 < 0) || (aRow2 >= aGrid.mRows) ||
						(aGrid.mColor[aRow2][aCol2] == MatchFinder::NO_GEM))
						continue;
					std::swap(aGrid.mColor[aRow][aCol], aGrid.mColor[aRow2][aCol2]);
					bool isMove = MatchFinder::InGroup(aGrid, theSpec, aCol, aRow, theOptions.mHorizontal, theOptions.mVertical);
					std::swap(aGrid.mColor[aRow][aCol], aGrid.mColor[aRow2][aCol2]);
					if (!isMove)
						continue;
					MoveSet::Move aMove;
					aMove.mType = GameRules::MOVE_SWAP;
					aMove.mA = aCol;
					aMove.mB = aRow;
					aMove.mC = aCol2;
					aMove.mD = aRow2;
					if (aTake(aMove, MoveSet::Cell(aCol, aRow), MoveSet::Cell(aCol2, aRow2)))
						return true;
				}
			}
		}
	}

	// The twists: 2x2 blocks of gems, row-major
	if (theOptions.mTypes & GameRules::MOVE_TWIST)
	{
		for (int aRow = 0; aRow + 1 < aGrid.mRows; aRow++)
		{
			for (int aCol = 0; aCol + 1 < aGrid.mCols; aCol++)
			{
				if (!aGem(aCol, aRow) || !aGem(aCol + 1, aRow) || !aGem(aCol, aRow + 1) || !aGem(aCol + 1, aRow + 1))
					continue;
				for (int aDir = 1; aDir >= (theOptions.mTwistBoth ? -1 : 1); aDir -= 2)
				{
					MoveSet::Move aMove;
					aMove.mType = GameRules::MOVE_TWIST;
					aMove.mA = aCol;
					aMove.mB = aRow;
					aMove.mC = aDir;
					MoveSet::Cell aTo;
					MoveSet::Cell aFrom;
					if (aLands(aMove, &aTo, &aFrom) && aTake(aMove, aTo, aFrom))
						return true;
				}
			}
		}
	}

	// The slides: the rows' segments, then the columns', each by 1 .. length - 1 cells
	if (theOptions.mTypes & GameRules::MOVE_SLIDE)
	{
		for (int anAxis = MoveSet::AXIS_ROW; anAxis <= MoveSet::AXIS_COL; anAxis++)
		{
			int aLines = (anAxis == MoveSet::AXIS_ROW) ? aGrid.mRows : aGrid.mCols;
			for (int aLine = 0; aLine < aLines; aLine++)
			{
				BoardLayout::Segment aSegments[MoveSet::MAX_MOVED];
				int aSegmentCount = (anAxis == MoveSet::AXIS_ROW) ? theLayout.RowSegments(aLine, aSegments) : theLayout.ColSegments(aLine, aSegments);
				for (int s = 0; s < aSegmentCount; s++)
				{
					int aStart = aSegments[s].mStart;
					int aLength = aSegments[s].mLength;
					bool isFull = true;
					for (int i = 0; i < aLength; i++)
						isFull = isFull && ((anAxis == MoveSet::AXIS_ROW) ? aGem(aStart + i, aLine) : aGem(aLine, aStart + i));
					if (!isFull)
						continue;
					for (int k = 1; k < aLength; k++)
					{
						MoveSet::Move aMove;
						aMove.mType = GameRules::MOVE_SLIDE;
						aMove.mA = anAxis;
						aMove.mB = aLine;
						aMove.mC = aStart;
						aMove.mD = aLength;
						aMove.mE = k;
						MoveSet::Cell aTo;
						MoveSet::Cell aFrom;
						if (aLands(aMove, &aTo, &aFrom) && aTake(aMove, aTo, aFrom))
							return true;
					}
				}
			}
		}
	}
	return false;
}

}

int MoveSet::MovedCells(const Move& theMove, Cell* theFrom, Cell* theTo)
{
	switch (theMove.mType)
	{
	case GameRules::MOVE_SWAP:
		theFrom[0] = Cell(theMove.mC, theMove.mD);
		theTo[0] = Cell(theMove.mA, theMove.mB);
		theFrom[1] = theTo[0];
		theTo[1] = theFrom[0];
		return 2;

	case GameRules::MOVE_TWIST:
		{
			// The corners clockwise from the top left; a gem goes one corner on in the move's direction
			int c = theMove.mA;
			int r = theMove.mB;
			const Cell aCorners[4] = {Cell(c, r), Cell(c + 1, r), Cell(c + 1, r + 1), Cell(c, r + 1)};
			for (int i = 0; i < 4; i++)
			{
				theFrom[i] = aCorners[i];
				theTo[i] = aCorners[(i + ((theMove.mC > 0) ? 1 : 3)) % 4];
			}
			return 4;
		}

	case GameRules::MOVE_SLIDE:
		for (int i = 0; i < theMove.mD; i++)
		{
			int aTo = theMove.mC + (i + theMove.mE) % theMove.mD;
			if (theMove.mA == AXIS_ROW)
			{
				theFrom[i] = Cell(theMove.mC + i, theMove.mB);
				theTo[i] = Cell(aTo, theMove.mB);
			}
			else
			{
				theFrom[i] = Cell(theMove.mB, theMove.mC + i);
				theTo[i] = Cell(theMove.mB, aTo);
			}
		}
		return theMove.mD;
	}
	return 0;
}

bool MoveSet::FindMove(const MatchFinder::ColorGrid& theGrid, const BoardLayout& theLayout, const MatchFinder::Spec& theSpec,
	const Options& theOptions, int theMoveNum, Found* theFound)
{
	return Scan(theGrid, theLayout, theSpec, theOptions, theMoveNum, theFound, NULL);
}

void MoveSet::FindMoves(const MatchFinder::ColorGrid& theGrid, const BoardLayout& theLayout, const MatchFinder::Spec& theSpec,
	const Options& theOptions, std::vector<Found>* theMoves)
{
	theMoves->clear();
	Scan(theGrid, theLayout, theSpec, theOptions, -1, NULL, theMoves);
}

bool MoveSet::SegmentAt(const BoardLayout& theLayout, int theAxis, int theCol, int theRow, int* theStart, int* theLength)
{
	BoardLayout::Segment aSegments[MAX_MOVED];
	int aCount = (theAxis == AXIS_ROW) ? theLayout.RowSegments(theRow, aSegments) : theLayout.ColSegments(theCol, aSegments);
	int aPos = (theAxis == AXIS_ROW) ? theCol : theRow;
	for (int i = 0; i < aCount; i++)
	{
		if ((aPos >= aSegments[i].mStart) && (aPos < aSegments[i].mStart + aSegments[i].mLength))
		{
			*theStart = aSegments[i].mStart;
			*theLength = aSegments[i].mLength;
			return true;
		}
	}
	return false;
}

void MoveSet::TwistCosSin(int theStep, int* theCos, int* theSin)
{
	theStep = std::max(0, std::min((int) TWIST_STEPS, theStep));
	*theCos = gTwistTable[theStep][0];
	*theSin = gTwistTable[theStep][1];
}
