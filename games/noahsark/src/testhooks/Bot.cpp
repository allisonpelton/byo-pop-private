// Test hooks: the bot's choice of a move (Bot.h). The three branches are the ones -autoplay always had, moved here so
// that the test command "move" gives a driver exactly the move -autoplay would click, from the same generator.

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

bool BotQuiet(Piece* p)
{
	return (p->mType < 0xd) && !p->mRemoved && !p->mFading && !p->mMoving && (p->mOffsetX == 0) && (p->mOffsetY == 0);
}

bool BotBoardSettled(Board* b)
{
	if (!b->mLink.empty())
		return false;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if (b->mGrid[x][y]->mMoving || ((b->mGrid[x][y]->mOffsetY != 0) && (b->mGrid[x][y]->mType < 0xd)) || b->mGrid[x][y]->mRemoved)
				return false;
	return true;
}

bool BotChooseMove(Board* b, int theGameMode, unsigned int& theSeed, BotMove& theMove)
{
	theMove = BotMove();
	if (theGameMode == GAMEMODE_STAMPEDE)
	{
		// every group of three or more, one of them at random
		std::vector<std::pair<int, int> > aGroups;
		for (int x = 0; x < 9; x++)
			for (int y = 0; y < 9; y++)
			{
				Piece* p = b->mGrid[x][y];
				if (!BotQuiet(p))
					continue;
				b->HighlightGroup(x, y);
				if (b->mLink.size() >= 3)
					aGroups.push_back(std::make_pair(x, y));
				b->ClearLinkList();
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
		for (int x = 0; x < 9; x++)
			for (int y = 0; y < 9; y++)
				for (int d = 0; d < 2; d++)
				{
					int nx = x + aDirs[d][0], ny = y + aDirs[d][1];
					if ((nx > 8) || (ny > 8) || !BotQuiet(b->mGrid[x][y]) || !BotQuiet(b->mGrid[nx][ny]))
						continue;
					std::swap(b->mGrid[x][y]->mType, b->mGrid[nx][ny]->mType);
					bool aMatch = false;
					for (int c = 0; c < 2; c++)
					{
						int cx = c ? nx : x, cy = c ? ny : y;
						int aType = b->mGrid[cx][cy]->mType;
						int aRun = 1;
						for (int i = cx - 1; i >= 0 && b->mGrid[i][cy]->mType == aType; i--) aRun++;
						for (int i = cx + 1; i < 9 && b->mGrid[i][cy]->mType == aType; i++) aRun++;
						if (aRun >= 3) aMatch = true;
						aRun = 1;
						for (int i = cy - 1; i >= 0 && b->mGrid[cx][i]->mType == aType; i--) aRun++;
						for (int i = cy + 1; i < 9 && b->mGrid[cx][i]->mType == aType; i++) aRun++;
						if (aRun >= 3) aMatch = true;
					}
					std::swap(b->mGrid[x][y]->mType, b->mGrid[nx][ny]->mType);
					if (aMatch)
						aSwaps.push_back(((x * 9 + y) << 8) | (nx * 9 + ny));
				}
		if (aSwaps.empty())
			return false;
		int aPick = aSwaps[BotRand(theSeed, (int) aSwaps.size())];
		int a = aPick >> 8, c = aPick & 0xff;
		theMove.mCount = 2;
		theMove.mCellX[0] = a / 9;
		theMove.mCellY[0] = a % 9;
		theMove.mCellX[1] = c / 9;
		theMove.mCellY[1] = c % 9;
		return true;
	}

	// action and strategy: a random pair that can be joined
	std::vector<std::pair<Piece*, Piece*> > aPairs;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
		{
			Piece* p = b->mGrid[x][y];
			if (!BotQuiet(p))
				continue;
			for (int x2 = x; x2 < 9; x2++)
				for (int y2 = (x2 == x) ? y + 1 : 0; y2 < 9; y2++)
				{
					Piece* q = b->mGrid[x2][y2];
					if (BotQuiet(q) && (q->mType == p->mType) && b->Reachable(p, q))
						aPairs.push_back(std::make_pair(p, q));
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
