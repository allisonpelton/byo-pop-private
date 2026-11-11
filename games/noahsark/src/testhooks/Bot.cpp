// port: test hooks: the bot's choice of a move (Bot.h). Its three branches serve -autoplay and the
// test command "move" alike, so a driver gets exactly the move -autoplay would click, from the same generator.

#include "Bot.h"
#include "NoahApp.h"
#include "Board.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace Sexy
{

int BotRand(unsigned int& theSeed, int theRange)
{
	theSeed = theSeed * 1103515245u + 12345u;
	return (int) ((theSeed >> 16) % (unsigned int) theRange);
}

bool BotQuiet(Piece* thePiece)
{
	return (thePiece->mType < NUM_ANIMALS) && !thePiece->mRemoved && !thePiece->mFading && !thePiece->mMoving && (thePiece->mOffsetX == 0) && (thePiece->mOffsetY == 0);
}

bool BotBoardSettled(Board* theBoard)
{
	if (!theBoard->mLink.empty())
		return false;
	for (int x = 0; x < Board::GRID_SIZE; x++)
		for (int y = 0; y < Board::GRID_SIZE; y++)
			if (theBoard->mGrid[x][y]->mMoving || ((theBoard->mGrid[x][y]->mOffsetY != 0) && (theBoard->mGrid[x][y]->mType < NUM_ANIMALS)) || theBoard->mGrid[x][y]->mRemoved)
				return false;
	return true;
}

bool BotChooseMove(Board* theBoard, int theGameMode, unsigned int& theSeed, BotMove& theMove)
{
	theMove = BotMove();
	if (theGameMode == GAMEMODE_STAMPEDE)
	{
		// every group of three or more, one of them at random
		std::vector<std::pair<int, int> > aGroups;
		for (int x = 0; x < Board::GRID_SIZE; x++)
			for (int y = 0; y < Board::GRID_SIZE; y++)
			{
				Piece* aPiece = theBoard->mGrid[x][y];
				if (!BotQuiet(aPiece))
					continue;
				theBoard->HighlightGroup(x, y);
				if (theBoard->mLink.size() >= 3)
					aGroups.push_back(std::make_pair(x, y));
				theBoard->ClearLinkList();
			}
		if (aGroups.empty())
			return false;
		std::pair<int, int> aPick = aGroups[BotRand(theSeed, (int) aGroups.size())];
		theMove.mCount = 1;
		theMove.mCellX[0] = aPick.first;
		theMove.mCellY[0] = aPick.second;
		return true;
	}
	if (theGameMode == GAMEMODE_PUZZLE)
	{
		static const int aDirs[2][2] = {{1, 0}, {0, 1}};
		std::vector<int> aSwaps;
		for (int x = 0; x < Board::GRID_SIZE; x++)
			for (int y = 0; y < Board::GRID_SIZE; y++)
				for (int aDir = 0; aDir < 2; aDir++)
				{
					int aNextX = x + aDirs[aDir][0], aNextY = y + aDirs[aDir][1];
					if ((aNextX > Board::GRID_LAST) || (aNextY > Board::GRID_LAST) || !BotQuiet(theBoard->mGrid[x][y]) || !BotQuiet(theBoard->mGrid[aNextX][aNextY]))
						continue;
					std::swap(theBoard->mGrid[x][y]->mType, theBoard->mGrid[aNextX][aNextY]->mType);
					bool aMatch = false;
					for (int aEnd = 0; aEnd < 2; aEnd++)
					{
						int aEndX = aEnd ? aNextX : x, aEndY = aEnd ? aNextY : y;
						int aType = theBoard->mGrid[aEndX][aEndY]->mType;
						int aRun = 1;
						for (int i = aEndX - 1; i >= 0 && theBoard->mGrid[i][aEndY]->mType == aType; i--) aRun++;
						for (int i = aEndX + 1; i < Board::GRID_SIZE && theBoard->mGrid[i][aEndY]->mType == aType; i++) aRun++;
						if (aRun >= 3) aMatch = true;
						aRun = 1;
						for (int i = aEndY - 1; i >= 0 && theBoard->mGrid[aEndX][i]->mType == aType; i--) aRun++;
						for (int i = aEndY + 1; i < Board::GRID_SIZE && theBoard->mGrid[aEndX][i]->mType == aType; i++) aRun++;
						if (aRun >= 3) aMatch = true;
					}
					std::swap(theBoard->mGrid[x][y]->mType, theBoard->mGrid[aNextX][aNextY]->mType);
					if (aMatch)
						aSwaps.push_back(((x * Board::GRID_SIZE + y) << 8) | (aNextX * Board::GRID_SIZE + aNextY));
				}
		if (aSwaps.empty())
			return false;
		int aPick = aSwaps[BotRand(theSeed, (int) aSwaps.size())];
		int aFrom = aPick >> 8, aTo = aPick & 0xff;
		theMove.mCount = 2;
		theMove.mCellX[0] = aFrom / Board::GRID_SIZE;
		theMove.mCellY[0] = aFrom % Board::GRID_SIZE;
		theMove.mCellX[1] = aTo / Board::GRID_SIZE;
		theMove.mCellY[1] = aTo % Board::GRID_SIZE;
		return true;
	}

	// action and strategy: a random pair that can be joined
	std::vector<std::pair<Piece*, Piece*> > aPairs;
	for (int x = 0; x < Board::GRID_SIZE; x++)
		for (int y = 0; y < Board::GRID_SIZE; y++)
		{
			Piece* aPiece = theBoard->mGrid[x][y];
			if (!BotQuiet(aPiece))
				continue;
			for (int aOtherX = x; aOtherX < Board::GRID_SIZE; aOtherX++)
				for (int aOtherY = (aOtherX == x) ? y + 1 : 0; aOtherY < Board::GRID_SIZE; aOtherY++)
				{
					Piece* aOther = theBoard->mGrid[aOtherX][aOtherY];
					if (BotQuiet(aOther) && (aOther->mType == aPiece->mType) && theBoard->Reachable(aPiece, aOther))
						aPairs.push_back(std::make_pair(aPiece, aOther));
				}
		}
	if (aPairs.empty())
		return false;
	std::pair<Piece*, Piece*> aPick = aPairs[BotRand(theSeed, (int) aPairs.size())];
	theMove.mCount = 2;
	theMove.mCellX[0] = aPick.first->mX;
	theMove.mCellY[0] = aPick.first->mY;
	theMove.mCellX[1] = aPick.second->mX;
	theMove.mCellY[1] = aPick.second->mY;
	return true;
}

}
