// Test hooks: the bot's choice of a cell (see Bot.h).

#include "Bot.h"
#include "AlchemyApp.h"
#include "Board.h"
#include "Piece.h"

#include <algorithm>
#include <vector>

using namespace Sexy;

int Sexy::BotRand(unsigned int* theSeed, int theRange)
{
	*theSeed = *theSeed * 1103515245u + 12345u;
	return (int) ((*theSeed >> 16) % (unsigned int) theRange);
}

void Sexy::BotCellToPointer(int theCol, int theRow, int* theX, int* theY)
{
	*theX = AlchemyApp::BOARD_X + Board::ColToX(theCol) + 0x30;
	*theY = Board::RowToY(theRow) + 0x30;
}

BotChoice Sexy::ChooseBotMove(Board* theBoard, unsigned int* theSeed, int theMiss)
{
	BotChoice aChoice;
	Piece* aPiece = theBoard->mCurPiece;
	if (aPiece == NULL)
		return aChoice;

	std::vector<int> aCells;
	std::vector<int> aScores;
	int aBest = -1;
	for (int aRow = 0; aRow < Board::ROWS; aRow++)
		for (int aCol = 0; aCol < Board::COLS; aCol++)
		{
			int aReason;
			if ((theBoard->mGrid[aRow][aCol] != NULL && !aPiece->mBomb) || !theBoard->CanPlace(aPiece, aCol, aRow, &aReason))
				continue;
			// a cell that completes a row or a column is best (the tutorial asks for it too); otherwise the more
			// neighbours a cell has, the sooner a row or column fills
			int aRowCount = 0;
			for (int i = 0; i < Board::COLS; i++)
				if (theBoard->mGrid[aRow][i] != NULL)
					aRowCount++;
			int aColCount = 0;
			for (int i = 0; i < Board::ROWS; i++)
				if (theBoard->mGrid[i][aCol] != NULL)
					aColCount++;
			int aNeighbors = (aRowCount == Board::COLS - 1 || aColCount == Board::ROWS - 1) ? 10 : 0;
			if (aRow > 0 && theBoard->mGrid[aRow - 1][aCol] != NULL) aNeighbors++;
			if (aRow < Board::ROWS - 1 && theBoard->mGrid[aRow + 1][aCol] != NULL) aNeighbors++;
			if (aCol > 0 && theBoard->mGrid[aRow][aCol - 1] != NULL) aNeighbors++;
			if (aCol < Board::COLS - 1 && theBoard->mGrid[aRow][aCol + 1] != NULL) aNeighbors++;
			aCells.push_back(aRow * Board::COLS + aCol);
			aScores.push_back(aNeighbors);
			if (aNeighbors > aBest)
				aBest = aNeighbors;
		}

	if (aCells.empty())
	{
		// nowhere to put it: the forge
		aChoice.mKind = BotChoice::KIND_FORGE;
		return aChoice;
	}

	int aPick = -1;
	if (theMiss > 0 && BotRand(theSeed, theMiss) == 0)
	{
		// a cell the piece cannot go to (the game answers with a message of the wizard's)
		std::vector<int> aBad;
		for (int i = 0; i < Board::ROWS * Board::COLS; i++)
			if (std::find(aCells.begin(), aCells.end(), i) == aCells.end())
				aBad.push_back(i);
		if (!aBad.empty())
		{
			aPick = aBad[BotRand(theSeed, (int) aBad.size())];
			aChoice.mDeliberateMiss = true;
		}
	}
	if (aPick >= 0)
		;
	else if (BotRand(theSeed, 10) == 0)
		aPick = aCells[BotRand(theSeed, (int) aCells.size())];
	else
	{
		int aCount = 0;
		for (size_t i = 0; i < aCells.size(); i++)
			if (aScores[i] == aBest && BotRand(theSeed, ++aCount) == 0)
				aPick = aCells[i];
	}

	aChoice.mKind = BotChoice::KIND_PLACE;
	aChoice.mCol = aPick % Board::COLS;
	aChoice.mRow = aPick / Board::COLS;
	return aChoice;
}
