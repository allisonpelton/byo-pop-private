// The maze solver: the "Abandon Hope" button solves the maze (0x4050b0, 0x405580) and the Ankh turns red when it can no
// longer be solved (0x411640, 0x411a90). Both search the same graph of chase states depth first, trying the explorer's
// moves in the order west, north, wait, south, east, with the mummies' and the scorpion's replies as the board plays
// them. See Board.h and docs/GAME_NOTES.md.

#include "Board.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Screens.h"
#include "MummyApp.h"
#include "Compat2004.h"

#include <cstdio>
#include <cstdlib>

using namespace Sexy;

// one mummy's step in the search: the same rules as Board::PlanMummies, on a copy of the state
void Board::StepMummy(ChaseState& s, int theIndex)
{
	int& x = s.mMummyX[theIndex];
	int& y = s.mMummyY[theIndex];
	if (!mRed)
	{
		if (x < 0 || mSize <= x || y < 0 || mSize <= y)
			s.mMummyAlive[theIndex] = false;
		else if (x < s.mExplorerX && (mCells[x][y] & WALL_EAST) == 0)
			x = x + 1;
		else if (s.mExplorerX < x && (mCells[x][y] & WALL_WEST) == 0)
			x = x - 1;
		else if (y < s.mExplorerY && (mCells[x][y] & WALL_SOUTH) == 0 && (!s.mGateClosed || x != mGateX || y != mGateY))
			y = y + 1;
		else if (s.mExplorerY < y && (mCells[x][y] & WALL_NORTH) == 0 && (!s.mGateClosed || x != mGateX || y - 1 != mGateY))
			y = y - 1;
	}
	else
	{
		if (x < 0 || mSize <= x || y < 0 || mSize <= y)
			s.mMummyAlive[theIndex] = false;
		else if (y < s.mExplorerY && ((mCells[x][y] & WALL_SOUTH) == 0 || (!s.mGateClosed && x == mGateX && y == mGateY)))
			y = y + 1;
		else if (s.mExplorerY < y && ((mCells[x][y] & WALL_NORTH) == 0 || (!s.mGateClosed && x == mGateX && y - 1 == mGateY)))
			y = y - 1;
		else if (x < s.mExplorerX && (mCells[x][y] & WALL_EAST) == 0)
			x = x + 1;
		else if (s.mExplorerX < x && (mCells[x][y] & WALL_WEST) == 0)
			x = x - 1;
	}
}

// the scorpion's step (the same rules; it dies when it leaves the maze)
void Board::StepScorpion(ChaseState& s)
{
	int& x = s.mScorpionX;
	int& y = s.mScorpionY;
	if (!mRed)
	{
		if (x < 0 || mSize <= x || y < 0 || mSize <= y)
			s.mScorpionAlive = false;
		else if (x < s.mExplorerX && (mCells[x][y] & WALL_EAST) == 0)
			x = x + 1;
		else if (s.mExplorerX < x && (mCells[x][y] & WALL_WEST) == 0)
			x = x - 1;
		else if (y < s.mExplorerY && (mCells[x][y] & WALL_SOUTH) == 0 && (!s.mGateClosed || x != mGateX || y != mGateY))
			y = y + 1;
		else if (s.mExplorerY < y && (mCells[x][y] & WALL_NORTH) == 0 && (!s.mGateClosed || x != mGateX || y - 1 != mGateY))
			y = y - 1;
	}
	else
	{
		if (x < 0 || mSize <= x || y < 0 || mSize <= y)
			s.mScorpionAlive = false;
		else if (y < s.mExplorerY && ((mCells[x][y] & WALL_SOUTH) == 0 || (!s.mGateClosed && x == mGateX && y == mGateY)))
			y = y + 1;
		else if (s.mExplorerY < y && ((mCells[x][y] & WALL_NORTH) == 0 || (!s.mGateClosed && x == mGateX && y - 1 == mGateY)))
			y = y - 1;
		else if (x < s.mExplorerX && (mCells[x][y] & WALL_EAST) == 0)
			x = x + 1;
		else if (s.mExplorerX < x && (mCells[x][y] & WALL_WEST) == 0)
			x = x - 1;
	}
}

// 0x405580 (theRecord: record the shortest solution in mSolution) and 0x411a90 (otherwise: only find out whether there is one)
void Board::SolveRec(const ChaseState& theFrom, std::vector<ChaseState>& thePath, int theDX, int theDY, bool theRecord)
{
	if (theRecord)
	{
		if (mShowingSolution && mSolution.size() < thePath.size())
			return;
	}
	else if (mSolvable)
		return;

	ChaseState s = theFrom;
	bool aUsedKey = false;
	s.mExplorerX = theFrom.mExplorerX + theDX;
	s.mExplorerY = theFrom.mExplorerY + theDY;
	if (s.mExplorerX == mKeyX && s.mExplorerY == mKeyY && (theDX != 0 || theDY != 0))
	{
		aUsedKey = true;
		s.mGateClosed = !s.mGateClosed;
	}
	if (s.mExplorerX == mTrapX[0] && s.mExplorerY == mTrapY[0])
		return;
	if (s.mExplorerX == mTrapX[1] && s.mExplorerY == mTrapY[1])
		return;

	// the scorpion moves first
	if (s.mScorpionAlive)
		StepScorpion(s);
	if (s.mExplorerX == s.mScorpionX && s.mExplorerY == s.mScorpionY && s.mScorpionAlive)
		return;

	if (s.mMummyAlive[0] && s.mMummyX[0] == s.mScorpionX && s.mMummyY[0] == s.mScorpionY)
		s.mScorpionAlive = false;
	if (s.mMummyAlive[1] && s.mMummyX[1] == s.mScorpionX && s.mMummyY[1] == s.mScorpionY)
		s.mScorpionAlive = false;
	if (!aUsedKey && s.mScorpionX == mKeyX && s.mScorpionY == mKeyY && s.mScorpionAlive &&
		(s.mScorpionX != theFrom.mScorpionX || s.mScorpionY != theFrom.mScorpionY))
	{
		aUsedKey = true;
		s.mGateClosed = !theFrom.mGateClosed;
	}

	// the mummies move twice
	for (int aStep = 0; aStep < 2; aStep++)
	{
		if (s.mMummyAlive[0])
			StepMummy(s, 0);
		if (s.mMummyAlive[1])
			StepMummy(s, 1);
		if (s.mMummyX[0] == s.mMummyX[1] && s.mMummyY[0] == s.mMummyY[1] && s.mMummyAlive[0] && s.mMummyAlive[1])
			s.mMummyAlive[1] = false;
		if (s.mMummyX[0] == s.mScorpionX && s.mMummyY[0] == s.mScorpionY && s.mMummyAlive[0])
		{
			if (s.mScorpionX == mKeyX && s.mScorpionY == mKeyY)
				s.mGateClosed = !s.mGateClosed;
			s.mScorpionAlive = false;
		}
		if (s.mMummyX[1] == s.mScorpionX && s.mMummyY[1] == s.mScorpionY && s.mMummyAlive[1])
		{
			if (s.mScorpionX == mKeyX && s.mScorpionY == mKeyY)
				s.mGateClosed = !s.mGateClosed;
			s.mScorpionAlive = false;
		}
		if (s.mExplorerX == s.mMummyX[0] && s.mExplorerY == s.mMummyY[0] && s.mMummyAlive[0])
			return;
		if (s.mExplorerX == s.mMummyX[1] && s.mExplorerY == s.mMummyY[1] && s.mMummyAlive[1])
			return;
		if (!aUsedKey)
		{
			if (s.mMummyX[0] == mKeyX && s.mMummyY[0] == mKeyY && s.mMummyAlive[0] &&
				(s.mMummyX[0] != theFrom.mMummyX[0] || s.mMummyY[0] != theFrom.mMummyY[0]))
				s.mGateClosed = !theFrom.mGateClosed;
			else
			{
				if (s.mMummyX[1] != mKeyX || s.mMummyY[1] != mKeyY || !s.mMummyAlive[1] ||
					(s.mMummyX[1] == theFrom.mMummyX[1] && s.mMummyY[1] == theFrom.mMummyY[1]))
					continue;
				s.mGateClosed = !theFrom.mGateClosed;
			}
			aUsedKey = true;
		}
	}

	thePath.push_back(s);
	if (mVisited.find(s) == mVisited.end())
	{
		mVisited.insert(s);
		if ((mCells[s.mExplorerX][s.mExplorerY] & EXIT_ANY) == 0)
		{
			for (int dx = -1; dx < 2; dx++)
			{
				for (int dy = -1; dy < 2; dy++)
				{
					if (dx == 0 || dy == 0)
					{
						std::vector<ChaseState> aPath = thePath;
						if (CanMove(s.mExplorerX, s.mExplorerY, s.mExplorerX + dx, s.mExplorerY + dy, false, s.mGateClosed))
							SolveRec(s, aPath, dx, dy, theRecord);
					}
				}
			}
		}
		else if (!theRecord)
			mSolvable = true;
		else if (!mShowingSolution || thePath.size() < mSolution.size())
		{
			mShowingSolution = true;
			ClearSolution();
			mSolution = thePath;
		}
	}
	else if (theRecord && mShowingSolution)
	{
		// a shorter way to a state that the best solution passes through: it takes over the solution's rest
		int aIndex = -1;
		for (size_t i = 0; i < mSolution.size(); i++)
		{
			if (mSolution[i] == s)
			{
				aIndex = (int) i;
				break;
			}
		}
		if (aIndex >= 0 && thePath.size() < (size_t) aIndex)
		{
			std::vector<ChaseState> aSolution(thePath.begin(), thePath.end());
			aSolution.insert(aSolution.end(), mSolution.begin() + aIndex + 1, mSolution.end());
			mSolution = aSolution;
		}
	}
}

// 0x4050b0: the Abandon Hope button: find the shortest way out and play it
void Board::Solve()
{
	Retry();
	mShowingSolution = false;
	ClearUndo();
	ClearVisited();
	ClearSolution();
	ChaseState aStart = GetChaseState();
	for (int dx = -1; dx < 2; dx++)
	{
		for (int dy = -1; dy < 2; dy++)
		{
			if (dx == 0 || dy == 0)
			{
				std::vector<ChaseState> aPath;
				aPath.push_back(aStart);
				mVisited.insert(aStart);
				if (CanMove(aStart.mExplorerX, aStart.mExplorerY, aStart.mExplorerX + dx, aStart.mExplorerY + dy, false,
					aStart.mGateClosed))
					SolveRec(aStart, aPath, dx, dy, true);
			}
		}
	}
	mState = STATE_READY;
	Retry();
	if (mShowingSolution && !mSolution.empty())
		mSolution.erase(mSolution.begin());
	ClearVisited();
}

// port: the shortest way out from the state the board is in now (Solve does the same from the start, after taking every
// move back, and plays it); the way is left in mSolution as well. The test bot uses it to go on after a turn that the
// search did not foresee (the original's search leaves out that a mummy that wins a fight on the key stone flips the gate)
bool Board::FindWay(std::vector<ChaseState>& thePath)
{
	mShowingSolution = false;
	ClearVisited();
	ClearSolution();
	ChaseState aStart = GetChaseState();
	for (int dx = -1; dx < 2; dx++)
	{
		for (int dy = -1; dy < 2; dy++)
		{
			if (dx == 0 || dy == 0)
			{
				std::vector<ChaseState> aPath;
				aPath.push_back(aStart);
				mVisited.insert(aStart);
				if (CanMove(aStart.mExplorerX, aStart.mExplorerY, aStart.mExplorerX + dx, aStart.mExplorerY + dy, false,
					aStart.mGateClosed))
					SolveRec(aStart, aPath, dx, dy, true);
			}
		}
	}
	bool aFound = mShowingSolution && !mSolution.empty();
	thePath = mSolution;
	if (aFound)
		thePath.erase(thePath.begin());
	mShowingSolution = false;
	ClearVisited();
	ClearSolution();
	return aFound;
}

// 0x411640: the Ankh: is there still a way out
void Board::CheckSolvable(bool thePlaySound)
{
	if (mShowingSolution || (mCells[mExplorer->mX][mExplorer->mY] & EXIT_ANY) != 0 || mSize >= 7)
		return;
	mSolvable = false;
	ClearVisited();
	ChaseState aStart = GetChaseState();
	for (int dx = -1; dx < 2; dx++)
	{
		for (int dy = -1; dy < 2; dy++)
		{
			if (dx == 0 || dy == 0)
			{
				std::vector<ChaseState> aPath;
				aPath.push_back(aStart);
				mVisited.insert(aStart);
				if (CanMove(aStart.mExplorerX, aStart.mExplorerY, aStart.mExplorerX + dx, aStart.mExplorerY + dy, false,
					aStart.mGateClosed))
					SolveRec(aStart, aPath, dx, dy, false);
			}
		}
	}
	mState = STATE_READY;
	if (!mSolvable && thePlaySound && mApp->mShowAnkh)
		mApp->PlaySample(SOUND_BADANKH);
	if (!mSolvable)
		m274 = mUpdateCnt;
	ClearVisited();
}
