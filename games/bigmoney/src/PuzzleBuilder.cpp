#include "PuzzleBuilder.h"
#include "MTRand.h"

#include <algorithm>
#include <cmath>

using namespace Sexy;

namespace
{

const int COLS = PuzzleModel::COLS;
const int ROWS = PuzzleModel::ROWS;
const int MIN_HEIGHT = 9;       // every column reaches row 6, as the original's
const int MAX_HEIGHT = 13;      // row 2 stays empty, as the original's
const int MAX_TRIES = 100000;   // groups tried (a board needs a few dozen; it never gets near this)

struct Cell
{
	int mCol;
	int mRow;
};

int Height(const PuzzleModel& theBoard, int theCol)
{
	int aHeight = 0;
	while ((aHeight < ROWS) && (theBoard.mCell[ROWS - 1 - aHeight][theCol] != PuzzleModel::EMPTY))
		aHeight++;
	return aHeight;
}

}

void PuzzleBuilder::Build(MTRand& theRand, int theColors, PuzzleModel* theBoard, std::vector<PuzzleSolver::Move>* thePlan)
{
	static const int aDirs[4][2] = { {0, 0}, {-1, 0}, {1, 0}, {0, 1} }; // the original's, which never goes up

	*theBoard = PuzzleModel();
	thePlan->clear();
	std::vector<PuzzleSolver::Move> aBuilt; // a coin of each group, in the order built
	int aLeft = COLS;                       // the columns started: aLeft..aRight
	int aRight = -1;

	for (int aTry = 0; aTry < MAX_TRIES; aTry++)
	{
		// A column short of row 6, among the started ones or next to them (the original's choice)
		std::vector<int> aCandidates;
		for (int c = 0; c < COLS; c++)
		{
			if ((Height(*theBoard, c) < MIN_HEIGHT) && ((aRight < 0) || ((c >= aLeft - 1) && (c <= aRight + 1))))
				aCandidates.push_back(c);
		}
		if (aCandidates.empty())
			break;
		int aCol = aCandidates[theRand.Next() % (unsigned long) aCandidates.size()];

		// The original's start row (low in the column, mostly) and size (mostly 2)
		unsigned long aRand = theRand.Next() % 100000;
		int aStartRow = (int) ceil(14.0 - (double) Height(*theBoard, aCol) * pow((double) aRand * 5e-06, 2.0) * 4.0);
		int aColor = (int) (theRand.Next() % (unsigned long) theColors);
		aRand = theRand.Next() % 100000;
		int aTargetSize = std::max(2, (int) (pow((double) aRand * 5e-06, 4.0) * 16.0 * 15.0));

		PuzzleModel aBefore = *theBoard;
		int aBeforeLeft = aLeft;
		int aBeforeRight = aRight;
		std::vector<Cell> aMembers;
		bool hasNewColumn = false;
		for (int anAttempt = 0; (anAttempt < aTargetSize + 12) && ((int) aMembers.size() < aTargetSize); anAttempt++)
		{
			int aX;
			int aY;
			if (aMembers.empty())
			{
				if ((anAttempt > 4) && (aStartRow < ROWS - 1))
					aStartRow++;
				aX = aCol;
				aY = aStartRow;
			}
			else
			{
				// Grow from a random coin of the group
				const Cell& aFrom = aMembers[theRand.Next() % (unsigned long) aMembers.size()];
				unsigned long aDir = theRand.Next() & 3;
				aX = aFrom.mCol + aDirs[aDir][0];
				aY = aFrom.mRow + aDirs[aDir][1];
			}

			// On the floor or a coin, in a column with room; a new column only next to the started ones, and
			// one at most per group. (A click that empties two columns at once can leave a bag landing in
			// the outer one, and the game closes the gap between it and the board before it leaves.)
			if ((aX < 0) || (aX >= COLS) || (aY < 0) || (aY >= ROWS))
				continue;
			if ((aY < ROWS - 1) && (theBoard->mCell[aY + 1][aX] == PuzzleModel::EMPTY))
				continue;
			if (Height(*theBoard, aX) >= MAX_HEIGHT)
				continue;
			bool isNewColumn = (aRight < 0) || (aX < aLeft) || (aX > aRight);
			if ((aRight >= 0) && ((aX < aLeft - 1) || (aX > aRight + 1)))
				continue;
			if ((isNewColumn) && (hasNewColumn))
				continue;

			// The coin goes in, the coins from there up move up one (the group's too)
			PuzzleModel aStep = *theBoard;
			std::vector<Cell> aStepMembers = aMembers;
			for (int r = 1; r <= aY; r++)
				theBoard->mCell[r - 1][aX] = theBoard->mCell[r][aX];
			theBoard->mCell[aY][aX] = (int8_t) aColor;
			for (Cell& aMember : aMembers)
			{
				if ((aMember.mCol == aX) && (aMember.mRow <= aY))
					aMember.mRow--;
			}
			aMembers.push_back({aX, aY});

			// After each coin the group is still exactly what a click takes: all of it connected through the
			// new coin, and nothing else of its colour touching. (A coin put under a group coin that has
			// another coin under it ends up below that one, apart: the original's split groups.)
			std::vector<int> aGroup;
			theBoard->GetGroup(aX, aY, &aGroup);
			bool isOk = (aGroup.size() == aMembers.size());
			for (const Cell& aMember : aMembers)
				isOk &= (std::find(aGroup.begin(), aGroup.end(), aMember.mRow * COLS + aMember.mCol) != aGroup.end());
			if (!isOk)
			{
				*theBoard = aStep;
				aMembers = aStepMembers;
				continue;
			}
			aLeft = std::min(aLeft, aX);
			aRight = std::max(aRight, aX);
			hasNewColumn |= isNewColumn;
		}

		// Kept if it has two coins or more. (Other coins the pushes moved may now touch others of their
		// colour: that is undone before any of them is clicked.)
		if (aMembers.size() < 2)
		{
			*theBoard = aBefore;
			aLeft = aBeforeLeft;
			aRight = aBeforeRight;
			continue;
		}
		PuzzleSolver::Move aMove = { (int8_t) aMembers[0].mCol, (int8_t) aMembers[0].mRow, 0 };
		aBuilt.push_back(aMove);
	}

	thePlan->assign(aBuilt.rbegin(), aBuilt.rend());
}
