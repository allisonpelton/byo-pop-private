// Board: the search for a move that is left (AnyMoveLeft, 0x459bc0, and its helpers 0x454170, 0x453a50, 0x4538d0, 0x433ce0,
// 0x43a960). The game ends, or a new row comes, when no move can make a group of the size the mode breaks up (m1a54) out of
// the bricks of one colour. A move is the taking of up to 6 same-coloured bricks from the foot of a column and the dropping
// of them in another; the bricks the player holds count as a stack of their own. The search works on the stacks of one
// colour at the foot of the columns (below the lowest stone, steel or virus brick of the column) and on the bricks above
// those obstacles, which can only be joined from the side.
//
// It is a counting argument that the original wrote as nested loops, and it is translated as written: its results are
// part of when a game ends, so nothing in it is simplified.

#include "Board.h"
#include "Brick.h"
#include <algorithm>

using namespace Sexy;

namespace
{

const int kNumColours = 7;      // the original's counts are vectors of seven ints
const int kCountSlots = 16;     // (a virus brick counts at 0xc, past the end of the original's vector; room is made here)

// A stack of bricks of one colour at the foot of a column (0x10 bytes in the original): its colour, how many bricks, the
// column, and the row of its lowest brick. The bricks in the hand are one stack of column and row -1.
struct MoveRun
{
	int		mColour;
	int		mCount;
	int		mCol;
	int		mRow;
};

// A brick above the lowest obstacle of its column (0x14 bytes in the original): where it is, its colour and from which sides
// (the neighbouring column is free down to that row) a stack can be put next to it.
struct MoveSpot
{
	int		mRow;
	int		mCol;
	int		mRow2;
	int		mColour;
	bool	mLeft;
	bool	mRight;
};

// the kinds that are used up in one piece
bool IsToolKind(int theKind)
{
	return theKind == 6 || theKind == 8 || theKind == 10 || theKind == 0xe || theKind == 0xb || theKind == 0xd;
}

// 0x43a960: the bricks in all the stacks
int RunTotal(const std::vector<MoveRun>& theRuns)
{
	int aTotal = 0;
	for (size_t i = 0; i < theRuns.size(); i++)
		aTotal += theRuns[i].mCount;
	return aTotal;
}

// 0x433ce0: whether theCount bricks can be made up of stacks of three, two and one (as many of each as there are), taking as
// many threes as there are room for, then twos, then ones, and once more with one three fewer. theCommit takes them off the
// counts (the callers never do).
bool Partition(int* theOnes, int* theTwos, int* theThrees, int theCount, bool theCommit)
{
	if (theCount < 1)
		return true;

	int aThrees = theCount / 3;
	if (*theThrees < aThrees)
		aThrees = *theThrees;
	int aTwosAvailable = *theTwos;
	int aTwos = (theCount - aThrees * 3) / 2;
	if (aTwosAvailable < aTwos)
		aTwos = aTwosAvailable;
	int aSum = aThrees * 3 + aTwos * 2;
	int aRest = theCount - aSum;
	int aOnes = 0;
	if (aRest > 0)
	{
		if (*theOnes < aRest)
			aRest = *theOnes;
		aSum += aRest;
		aOnes = aRest;
	}
	if (aSum != theCount)
	{
		aThrees = aThrees - 1;
		if (aThrees < 0)
			aThrees = 0;
		if (*theThrees < aThrees)
			aThrees = *theThrees;
		aTwos = (theCount - aThrees * 3) / 2;
		if (aTwosAvailable < aTwos)
			aTwos = aTwosAvailable;
		aSum = aThrees * 3 + aTwos * 2;
		aRest = theCount - aSum;
		aOnes = 0;
		if (aRest > 0)
		{
			aOnes = *theOnes;
			if (aOnes < aRest)
				aSum += aOnes;
			else
			{
				aSum += aRest;
				aOnes = aRest;
			}
		}
		if (aSum != theCount)
			return false;
	}
	if (theCommit)
	{
		*theOnes -= aOnes;
		*theTwos -= aTwos;
		*theThrees -= aThrees;
	}
	return true;
}

// The cells of the stacks, each from the stack's lowest brick up
void ExpandRuns(const std::vector<MoveRun>& theRuns, std::vector<CellRef>& theCells)
{
	for (size_t i = 0; i < theRuns.size(); i++)
	{
		for (int j = 0; j < theRuns[i].mCount; j++)
		{
			CellRef aCell;
			aCell.mCol = theRuns[i].mCol;
			aCell.mRow = theRuns[i].mRow - j;
			theCells.push_back(aCell);
		}
	}
}

// 0x4538d0: how many cells of the kind are joined to this one (up to theLimit), not counting the cells in theCells. The
// gathering is done in the Board's own list (mb8, which keeps what was in it) and the list is put back as it was.
int CountConnected(Board* theBoard, int theKind, int theCol, int theRow, unsigned int theLimit, const std::vector<CellRef>& theCells)
{
	std::vector<CellRef> aSaved = theBoard->mb8;
	theBoard->CollectConnected(theKind, theCol, theRow, theLimit, false, &theCells, -1, -1);
	int aCount = (int) theBoard->mb8.size();
	theBoard->mb8 = aSaved;
	return aCount;
}

// 0x453a50: a second brick of the colour above the lowest obstacle in a column two or three columns across (theDir says which
// way the stack comes from) joins the group when the stacks can fill the space between. The first brick is the spot at
// theIdx; the others are the spots after it. True when the group can reach the size the mode breaks up.
bool AnalyseSecond(Board* theBoard, int theColour, const std::vector<MoveRun>& theRuns, const std::vector<MoveSpot>& theSpots,
				   unsigned int theIdx, int theOnes, int theTwos, int theThrees, const std::vector<int>& theColTop, int theDir)
{
	std::vector<CellRef> aCells;
	ExpandRuns(theRuns, aCells);

	MoveSpot aSpot = theSpots[theIdx];
	int aBase = CountConnected(theBoard, theColour, aSpot.mCol, aSpot.mRow2, (unsigned int) theBoard->m1a54, aCells);
	std::vector<MoveRun> aRuns = theRuns;
	unsigned int aNext = theIdx;
	int aTarget = theBoard->m1a54;

	if (RunTotal(theRuns) + aBase < aTarget)
	{
		do
		{
			unsigned int aCount = (unsigned int) theSpots.size();
			if (aCount - 1U <= aNext || aRuns.empty())
				break;
			if (aCount - 1U <= theIdx)
				break;
			aNext++;
			MoveSpot aOther = theSpots[aNext];

			if (aOther.mColour == aSpot.mColour)
			{
				int aDx = aOther.mCol - aSpot.mCol;
				int aDy = aOther.mRow2 - aSpot.mRow2;
				int aAbsDx = aDx < 0 ? -aDx : aDx;
				int aAbsDy = aDy < 0 ? -aDy : aDy;
				if ((theDir < 1 || aDx >= 0) && (theDir > -1 || aDx < 1) && aDx != 0 && aDx != 1 &&
					(aAbsDx < 3 || aAbsDy < 1) && aAbsDx < 4)
				{
					int aSide = aDx > 0 ? 1 : -1;
					if (aAbsDx == 3 && aDy == 0)
					{
						if (theRuns.size() > 1)
						{
							int aFirst = theColTop[aSpot.mCol + aSide];
							if (aFirst < 0)
								aFirst = 0;
							int aSecond = theColTop[aSpot.mCol + aSide * 2];
							if (aSecond < 0)
								aSecond = 0;
							if (Partition(&theOnes, &theTwos, &theThrees, (aSpot.mRow2 - aFirst) - 1, false) &&
								Partition(&theOnes, &theTwos, &theThrees, (aSpot.mRow2 - aSecond) - 1, false))
							{
								aTarget = theBoard->m1a54;
								int aJoined = CountConnected(theBoard, theColour, aOther.mCol, aOther.mRow2, (unsigned int) aTarget, aCells);
								if (aTarget <= RunTotal(theRuns) + (aBase + aJoined))
									return true;
							}
						}
					}
					else if (aAbsDx == 2)
					{
						if (aDy == 0)
						{
							if (!theRuns.empty())
							{
								int aFirst = theColTop[aSpot.mCol + aSide];
								if (aFirst < 0)
									aFirst = 0;
								if (Partition(&theOnes, &theTwos, &theThrees, (aSpot.mRow2 - aFirst) - 1, false))
								{
									aTarget = theBoard->m1a54;
									int aJoined = CountConnected(theBoard, theColour, aOther.mCol, aOther.mRow2, (unsigned int) aTarget, aCells);
									if (aTarget <= RunTotal(theRuns) + aBase + aJoined)
										return true;
								}
							}
						}
						else if (aDy == 1)
						{
							if (!theRuns.empty() && RunTotal(theRuns) > 1)
							{
								int aFirst = theColTop[aSpot.mCol + aSide];
								if (aFirst < 0)
									aFirst = 0;
								if (Partition(&theOnes, &theTwos, &theThrees, (aSpot.mRow2 - aFirst) - 1, false))
								{
									aTarget = theBoard->m1a54;
									int aJoined = CountConnected(theBoard, theColour, aOther.mCol, aOther.mRow2, (unsigned int) aTarget, aCells);
									if (aTarget <= RunTotal(theRuns) + aBase + aJoined)
										return true;
								}
							}
						}
						else if (aDy == -1 && !theRuns.empty() && RunTotal(theRuns) > 1)
						{
							int aFirst = theColTop[aSpot.mCol + aSide];
							if (aFirst < 0)
								aFirst = 0;
							if (Partition(&theOnes, &theTwos, &theThrees, (aOther.mRow2 - aFirst) - 1, false))
							{
								aTarget = theBoard->m1a54;
								int aJoined = CountConnected(theBoard, theColour, aOther.mCol, aOther.mRow2, (unsigned int) aTarget, aCells);
								if (aTarget <= RunTotal(theRuns) + (aBase + aJoined))
									return true;
							}
						}
					}
				}
			}
		} while (RunTotal(theRuns) + aBase < aTarget);
	}
	return theBoard->m1a54 <= RunTotal(theRuns) + aBase;
}

// 0x454170: whether a stack put beside the spot makes the group big enough. The stacks that go there are made of the
// stacks of the other colours (as many threes, twos and ones as there are) and fill the column down to the row of the spot,
// from as little as the bricks of the colour that are in stacks to as many as the column holds. The neighbouring column's
// lowest obstacle (theLimitRow) is how far down the stack has to reach.
bool AnalyseSpot(Board* theBoard, int theRow, int theColour, int theLimitRow, int theTotal, int theOnes, int theTwos,
				 int theThrees, const std::vector<MoveRun>& theRuns, const std::vector<MoveSpot>& theSpots,
				 unsigned int theIdx, const std::vector<int>& theColTop, int theDir)
{
	std::vector<CellRef> aCells;
	ExpandRuns(theRuns, aCells);

	int aReach = theRow - theLimitRow;
	int aCount = aReach - theTotal;
	if (aCount < 0)
		aCount = 0;
	int aMost = aReach - 1;
	if (aMost < 0)
		aMost = 0;
	MoveSpot aSpot = theSpots[theIdx];
	bool aTriedSecond = false;
	while (aCount <= aMost)
	{
		if (Partition(&theOnes, &theTwos, &theThrees, aCount, false))
		{
			int aJoined = CountConnected(theBoard, theColour, aSpot.mCol, aSpot.mRow2, (unsigned int) theBoard->m1a54, aCells);
			if (theBoard->m1a54 <= aJoined + theTotal)
				return true;
			if (!aTriedSecond)
			{
				aTriedSecond = true;
				if (AnalyseSecond(theBoard, theColour, theRuns, theSpots, theIdx, theOnes, theTwos, theThrees, theColTop, theDir))
					return true;
			}
		}
		aCount++;
	}
	return false;
}

}

// 0x459bc0: whether the player can still make a group. theFlag (when given) says that the field is empty and nothing is held.
// Bricks that are still moving mean there may be a move; so does a hand with a whole group in it, or with one of the one-use
// kinds, or one of those kinds below the lowest obstacle of its column.
bool Board::AnyMoveLeft(bool* theFlag)
{
	if (theFlag != NULL)
		*theFlag = false;
	if (CountMovingBricks(false) > 0)
		return true;

	std::vector<int> aCount(kCountSlots, 0);        // the bricks of each colour on the field and in the hand
	std::vector<int> aReach(kCountSlots, 0);        // those of them that can be taken from the foot of their column
	std::vector<int> aColTop(10, -1);               // the row of the lowest stone, steel or virus brick of each column
	std::vector<MoveRun> aRuns;
	std::vector<MoveSpot> aSpots;

	if ((unsigned int) m70.size() >= (unsigned int) m1a54)
		return true;

	if (!m70.empty())
	{
		Brick* aHeld = m70.front();
		if (aHeld->mMutated != 1 && IsToolKind(aHeld->mType))
			return true;
	}
	if (theFlag != NULL)
	{
		*theFlag = true;
		if (!m70.empty())
			*theFlag = false;
	}

	// the lowest obstacle of each column (rows 1 and up are looked at, from the foot of the field)
	for (int aCol = 0; aCol < 10; aCol++)
	{
		int aRows = (int) m314.size();
		for (int aRow = aRows - 1; aRow > 0; aRow--)
		{
			Brick* aBrick = m314[aRow][aCol];
			if (aBrick != NULL && (aBrick->mMutated == 1 || aBrick->mType == 7 || aBrick->mType == 9))
			{
				aColTop[aCol] = aRow;
				break;
			}
		}
	}

	for (int aCol = 0; aCol < 10; aCol++)
	{
		int aRows = (int) m314.size();
		for (int aRow = aRows - 1; aRow > 0; aRow--)
		{
			Brick* aBrick = m314[aRow][aCol];
			if (aBrick == NULL)
				continue;
			if (theFlag != NULL)
				*theFlag = false;

			int aMut = aBrick->mMutated;
			int aType = aBrick->mType;
			if (aMut == 1)
				continue;
			if (IsToolKind(aType) && aColTop[aCol] < aRow)
				return true;
			// the bricks of a colour count, and so does a brick marked special
			if (!((aType >= 0 && aType <= 5) || aBrick->mSpecial))
				continue;

			if (aType >= 0 && aType < kCountSlots)
				aCount[aType]++;
			if (aColTop[aCol] < aRow)
			{
				int aKey = aType;
				if (aKey < 0 || aKey >= kCountSlots)
					continue;
				int aReached = ++aReach[aKey];
				if (m1a54 <= aReached)
					return true;

				// a brick on one of the same colour is counted with that stack
				int aRowTotal = (int) m314.size();
				if (aRow != aRowTotal - 1 && m314[aRow + 1][aCol] != NULL)
				{
					Brick* aBelow = m314[aRow + 1][aCol];
					int aBelowKey = (aBelow->mMutated == 1) ? 0xc : aBelow->mType;
					if (aBelowKey == aKey)
						continue;
				}

				// the lowest brick of a stack: how high the stack goes
				int aRunLen = 1;
				for (int r = aRow - 1; r > 0; r--)
				{
					Brick* aUp = m314[r][aCol];
					if (aUp == NULL)
						break;
					int aUpKey = (aUp->mMutated == 1) ? 0xc : aUp->mType;
					if (aUpKey != aKey)
						break;
					aRunLen++;
				}
				MoveRun aRun;
				aRun.mColour = aKey;
				aRun.mCount = aRunLen;
				aRun.mCol = aCol;
				aRun.mRow = aRow;
				aRuns.push_back(aRun);
			}
			else if (aRow < aColTop[aCol])
			{
				// above the obstacle: joined only from a side where the neighbouring column is free down to this row
				bool aLeft = aCol > 0 && aColTop[aCol - 1] < aRow;
				bool aRight = aCol < 9 && aColTop[aCol + 1] < aRow;
				if (aLeft || aRight)
				{
					MoveSpot aSpot;
					aSpot.mRow = aRow;
					aSpot.mCol = aCol;
					aSpot.mRow2 = aRow;
					aSpot.mColour = aType;
					aSpot.mLeft = aLeft;
					aSpot.mRight = aRight;
					aSpots.push_back(aSpot);
				}
			}
		}
	}

	// the bricks in the hand are one more stack
	if (!m70.empty())
	{
		Brick* aHeld = m70.front();
		int aKey = (aHeld->mMutated == 1) ? 0xc : aHeld->mType;
		aCount[aKey] += (int) m70.size();
		aReach[aKey] += (int) m70.size();
		MoveRun aRun;
		aRun.mColour = aKey;
		aRun.mCount = (int) m70.size();
		aRun.mCol = -1;
		aRun.mRow = -1;
		aRuns.push_back(aRun);
	}

	// a colour with a group's worth below the obstacles is a move; no colour with a group's worth at all is none
	bool anyEnough = false;
	for (int i = 0; i < kNumColours; i++)
	{
		if (aReach[i] >= m1a54)
			return true;
		if (aCount[i] >= m1a54)
			anyEnough = true;
	}
	if (!anyEnough)
		return false;

	// otherwise the bricks above the obstacles: can stacks beside one of them make a group
	for (int aColour = 0; aColour < kNumColours; aColour++)
	{
		std::vector<MoveRun> aColourRuns;
		int aTotal = 0;
		int aHistogram[8] = {1, 0, 0, 0, 0, 0, 0, 0};       // the stacks of the other colours by their size (1, 2, 3)

		if (m1a54 <= aCount[aColour] && 0 < aReach[aColour])
		{
			for (size_t i = 0; i < aRuns.size(); i++)
			{
				if (aRuns[i].mColour == aColour)
				{
					aTotal += aRuns[i].mCount;
					aColourRuns.push_back(aRuns[i]);
				}
				else if (aRuns[i].mCount >= 0 && aRuns[i].mCount < 8)
					aHistogram[aRuns[i].mCount]++;
			}

			for (unsigned int aSpotIndex = 0; aSpotIndex < aSpots.size(); aSpotIndex++)
			{
				MoveSpot aSpot = aSpots[aSpotIndex];
				if (aSpot.mColour == aColour)
				{
					if (aSpot.mLeft &&
						AnalyseSpot(this, aSpot.mRow, aColour, std::max(aColTop[aSpot.mCol - 1], 0), aTotal, aHistogram[1],
									aHistogram[2], aHistogram[3], aColourRuns, aSpots, aSpotIndex, aColTop, -1))
						return true;
					if (aSpot.mRight &&
						AnalyseSpot(this, aSpot.mRow, aColour, std::max(aColTop[aSpot.mCol + 1], 0), aTotal, aHistogram[1],
									aHistogram[2], aHistogram[3], aColourRuns, aSpots, aSpotIndex, aColTop, 1))
						return true;
				}
			}
		}
	}
	return false;
}
