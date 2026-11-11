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

// port: one mummy's step in the search (a piece of 0x405580): the same rules as Board::SetMummiesInMotion, on a copy
// of the state
void Board::StepMummy(ChaseState& theState, int theIndex)
{
	int& x = theState.mMummyX[theIndex];
	int& y = theState.mMummyY[theIndex];
	if (!mRed)
	{
		if (x < 0 || mSize <= x || y < 0 || mSize <= y)
			theState.mMummyAlive[theIndex] = false;
		else if (x < theState.mExplorerX && (mCells[x][y] & WALL_EAST) == 0)
			x = x + 1;
		else if (theState.mExplorerX < x && (mCells[x][y] & WALL_WEST) == 0)
			x = x - 1;
		else if (y < theState.mExplorerY && (mCells[x][y] & WALL_SOUTH) == 0 && (!theState.mGateClosed || x != mGateX || y != mGateY))
			y = y + 1;
		else if (theState.mExplorerY < y && (mCells[x][y] & WALL_NORTH) == 0 && (!theState.mGateClosed || x != mGateX || y - 1 != mGateY))
			y = y - 1;
	}
	else
	{
		if (x < 0 || mSize <= x || y < 0 || mSize <= y)
			theState.mMummyAlive[theIndex] = false;
		else if (y < theState.mExplorerY && ((mCells[x][y] & WALL_SOUTH) == 0 || (!theState.mGateClosed && x == mGateX && y == mGateY)))
			y = y + 1;
		else if (theState.mExplorerY < y && ((mCells[x][y] & WALL_NORTH) == 0 || (!theState.mGateClosed && x == mGateX && y - 1 == mGateY)))
			y = y - 1;
		else if (x < theState.mExplorerX && (mCells[x][y] & WALL_EAST) == 0)
			x = x + 1;
		else if (theState.mExplorerX < x && (mCells[x][y] & WALL_WEST) == 0)
			x = x - 1;
	}
}

// port: the scorpion's step in the search (the same rules; it dies when it leaves the maze)
void Board::StepScorpion(ChaseState& theState)
{
	int& x = theState.mScorpionX;
	int& y = theState.mScorpionY;
	if (!mRed)
	{
		if (x < 0 || mSize <= x || y < 0 || mSize <= y)
			theState.mScorpionAlive = false;
		else if (x < theState.mExplorerX && (mCells[x][y] & WALL_EAST) == 0)
			x = x + 1;
		else if (theState.mExplorerX < x && (mCells[x][y] & WALL_WEST) == 0)
			x = x - 1;
		else if (y < theState.mExplorerY && (mCells[x][y] & WALL_SOUTH) == 0 && (!theState.mGateClosed || x != mGateX || y != mGateY))
			y = y + 1;
		else if (theState.mExplorerY < y && (mCells[x][y] & WALL_NORTH) == 0 && (!theState.mGateClosed || x != mGateX || y - 1 != mGateY))
			y = y - 1;
	}
	else
	{
		if (x < 0 || mSize <= x || y < 0 || mSize <= y)
			theState.mScorpionAlive = false;
		else if (y < theState.mExplorerY && ((mCells[x][y] & WALL_SOUTH) == 0 || (!theState.mGateClosed && x == mGateX && y == mGateY)))
			y = y + 1;
		else if (theState.mExplorerY < y && ((mCells[x][y] & WALL_NORTH) == 0 || (!theState.mGateClosed && x == mGateX && y - 1 == mGateY)))
			y = y - 1;
		else if (x < theState.mExplorerX && (mCells[x][y] & WALL_EAST) == 0)
			x = x + 1;
		else if (theState.mExplorerX < x && (mCells[x][y] & WALL_WEST) == 0)
			x = x - 1;
	}
}

// 0x405580 (theRecord: record the shortest solution in mSolution) and 0x411a90 (otherwise: only find out whether there is one)
// 0x404f60 (the visited list), 0x405010 (the best solution's list: below, where the index is looked up)
void Board::DoSolutionHelper(const ChaseState& theFrom, std::vector<ChaseState>& thePath, int theDX, int theDY, bool theRecord)
{
	if (theRecord)
	{
		if (mShowingSolution && mSolution.size() < thePath.size())
			return;
	}
	else if (mSolvable)
		return;

	ChaseState aState = theFrom;
	bool aUsedKey = false;
	aState.mExplorerX = theFrom.mExplorerX + theDX;
	aState.mExplorerY = theFrom.mExplorerY + theDY;
	if (aState.mExplorerX == mKeyX && aState.mExplorerY == mKeyY && (theDX != 0 || theDY != 0))
	{
		aUsedKey = true;
		aState.mGateClosed = !aState.mGateClosed;
	}
	if (aState.mExplorerX == mTrapX[0] && aState.mExplorerY == mTrapY[0])
		return;
	if (aState.mExplorerX == mTrapX[1] && aState.mExplorerY == mTrapY[1])
		return;

	// the scorpion moves first
	if (aState.mScorpionAlive)
		StepScorpion(aState);
	if (aState.mExplorerX == aState.mScorpionX && aState.mExplorerY == aState.mScorpionY && aState.mScorpionAlive)
		return;

	if (aState.mMummyAlive[0] && aState.mMummyX[0] == aState.mScorpionX && aState.mMummyY[0] == aState.mScorpionY)
		aState.mScorpionAlive = false;
	if (aState.mMummyAlive[1] && aState.mMummyX[1] == aState.mScorpionX && aState.mMummyY[1] == aState.mScorpionY)
		aState.mScorpionAlive = false;
	if (!aUsedKey && aState.mScorpionX == mKeyX && aState.mScorpionY == mKeyY && aState.mScorpionAlive &&
		(aState.mScorpionX != theFrom.mScorpionX || aState.mScorpionY != theFrom.mScorpionY))
	{
		aUsedKey = true;
		aState.mGateClosed = !theFrom.mGateClosed;
	}

	// the mummies move twice
	for (int aStep = 0; aStep < 2; aStep++)
	{
		if (aState.mMummyAlive[0])
			StepMummy(aState, 0);
		if (aState.mMummyAlive[1])
			StepMummy(aState, 1);
		bool aMummyFight = false;
		if (aState.mMummyX[0] == aState.mMummyX[1] && aState.mMummyY[0] == aState.mMummyY[1] && aState.mMummyAlive[0] && aState.mMummyAlive[1])
		{
			aState.mMummyAlive[1] = false;
			aMummyFight = true;
		}
		if (aState.mMummyX[0] == aState.mScorpionX && aState.mMummyY[0] == aState.mScorpionY && aState.mMummyAlive[0])
		{
			if (aState.mScorpionX == mKeyX && aState.mScorpionY == mKeyY)
				aState.mGateClosed = !aState.mGateClosed;
			aState.mScorpionAlive = false;
		}
		if (aState.mMummyX[1] == aState.mScorpionX && aState.mMummyY[1] == aState.mScorpionY && aState.mMummyAlive[1])
		{
			if (aState.mScorpionX == mKeyX && aState.mScorpionY == mKeyY)
				aState.mGateClosed = !aState.mGateClosed;
			aState.mScorpionAlive = false;
		}
		if (aState.mExplorerX == aState.mMummyX[0] && aState.mExplorerY == aState.mMummyY[0] && aState.mMummyAlive[0])
			return;
		if (aState.mExplorerX == aState.mMummyX[1] && aState.mExplorerY == aState.mMummyY[1] && aState.mMummyAlive[1])
			return;
		// port(mods, the fixes mod's solverFight): the board flips the gate when a fight on the key stone ends (Mummy::CheckGate), whether
		// or not the winner moved this turn and whether or not the key was used earlier in it; the original's search flips it only
		// for a mummy that moved and the key not used yet. The first kind it has right (the code below), the rest are added here.
		if (mApp->mFixes.mSolverFight && aMummyFight && aState.mMummyAlive[0] && aState.mMummyX[0] == mKeyX && aState.mMummyY[0] == mKeyY &&
			!(!aUsedKey && (aState.mMummyX[0] != theFrom.mMummyX[0] || aState.mMummyY[0] != theFrom.mMummyY[0])))
		{
			aState.mGateClosed = !aState.mGateClosed;
			aUsedKey = true;
			continue;
		}
		if (!aUsedKey)
		{
			if (aState.mMummyX[0] == mKeyX && aState.mMummyY[0] == mKeyY && aState.mMummyAlive[0] &&
				(aState.mMummyX[0] != theFrom.mMummyX[0] || aState.mMummyY[0] != theFrom.mMummyY[0]))
				aState.mGateClosed = !theFrom.mGateClosed;
			else
			{
				if (aState.mMummyX[1] != mKeyX || aState.mMummyY[1] != mKeyY || !aState.mMummyAlive[1] ||
					(aState.mMummyX[1] == theFrom.mMummyX[1] && aState.mMummyY[1] == theFrom.mMummyY[1]))
					continue;
				aState.mGateClosed = !theFrom.mGateClosed;
			}
			aUsedKey = true;
		}
	}

	thePath.push_back(aState);
	if (mVisited.find(aState) == mVisited.end())
	{
		mVisited.insert(aState);
		if ((mCells[aState.mExplorerX][aState.mExplorerY] & EXIT_ANY) == 0)
		{
			for (int dx = -1; dx < 2; dx++)
			{
				for (int dy = -1; dy < 2; dy++)
				{
					if (dx == 0 || dy == 0)
					{
						std::vector<ChaseState> aPath = thePath;
						if (CanMove(aState.mExplorerX, aState.mExplorerY, aState.mExplorerX + dx, aState.mExplorerY + dy, false, aState.mGateClosed))
							DoSolutionHelper(aState, aPath, dx, dy, theRecord);
					}
				}
			}
		}
		else if (!theRecord)
			mSolvable = true;
		else if (!mShowingSolution || thePath.size() < mSolution.size())
		{
			mShowingSolution = true;
			ClearSolutionList();
			mSolution = thePath;
		}
	}
	else if (theRecord && mShowingSolution)
	{
		// a shorter way to a state that the best solution passes through: it takes over the solution's rest
		int aIndex = -1;
		for (size_t i = 0; i < mSolution.size(); i++)
		{
			if (mSolution[i] == aState)
			{
				aIndex = (int) i;
				break;
			}
		}
		if (aIndex >= 0 && thePath.size() < (size_t) (aIndex + 1))	// 0x405010 returns the 1-based index
		{
			std::vector<ChaseState> aSolution(thePath.begin(), thePath.end());
			aSolution.insert(aSolution.end(), mSolution.begin() + aIndex + 1, mSolution.end());
			mSolution = aSolution;
		}
	}
}

// 0x4050b0: the Abandon Hope button: find the shortest way out and play it
void Board::DoSolution()
{
	ResetMaze();
	mShowingSolution = false;
	ClearMoves();
	ClearCheckedList();
	ClearSolutionList();
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
					DoSolutionHelper(aStart, aPath, dx, dy, true);
			}
		}
	}
	mState = STATE_READY;
	ResetMaze();
	// left out on purpose: when the search finds no way the original appends the maze to badmaps.dat (a debug file that the
	// player should not get)
	if (mShowingSolution && !mSolution.empty())
		mSolution.erase(mSolution.begin());
	ClearCheckedList();
}

// port: the shortest way out from the state the board is in now (DoSolution does the same from the start, after taking every
// move back, and plays it); the way is left in mSolution as well. The test bot uses it to go on after a turn that the
// search did not foresee (the original's search leaves out that a mummy that wins a fight on the key stone flips the gate)
bool Board::FindWay(std::vector<ChaseState>& thePath)
{
	mShowingSolution = false;
	ClearCheckedList();
	ClearSolutionList();
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
					DoSolutionHelper(aStart, aPath, dx, dy, true);
			}
		}
	}
	bool aFound = mShowingSolution && !mSolution.empty();
	thePath = mSolution;
	if (aFound)
		thePath.erase(thePath.begin());
	mShowingSolution = false;
	ClearCheckedList();
	ClearSolutionList();
	return aFound;
}

// 0x411640: the Ankh: is there still a way out
void Board::CheckSolvable(bool thePlaySound)
{
	if (mShowingSolution || (mCells[mExplorer->mX][mExplorer->mY] & EXIT_ANY) != 0 || mSize >= 7)
		return;
	mSolvable = false;
	ClearCheckedList();
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
					DoSolutionHelper(aStart, aPath, dx, dy, false);
			}
		}
	}
	mState = STATE_READY;
	if (!mSolvable && thePlaySound && mApp->mShowAnkh)
		mApp->PlaySample(SOUND_BADANKH);
	if (!mSolvable)
		mUnsolvableTick = mUpdateCnt;
	ClearCheckedList();
}
