#include "Bot.h"

#include "Board.h"
#include "Deal.h"

#include <algorithm>
#include <cstdlib>
#include <random>

namespace mahjongchina
{

// the positions all the searches of one call may try before the bot goes on without a plan
static const long SEARCH_BUDGET = 100000;

// A copy of the live board's tiles: all of them in the board's order, the ones taken marked taken, each with the
// kind it has now and where it is drawn
void Bot::Copy(Layout& theLayout)
{
	mCopy.Clear(true);
	mCopy.mHasCounts = false;
	mTiles.clear();
	mLive.clear();
	for (jb::ListNode* aNode = theLayout.mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aLive = (Tile*)aNode->mObj;
		mCopy.AddTile(aLive->mX, aLive->mY, aLive->mZ, aLive->mKind);
		Tile* aTile = (Tile*)mCopy.mTail->mObj;
		aTile->mRect = aLive->mRect;
		aTile->mTaken = aLive->mTaken != 0 ? 1 : 0;
		mTiles.push_back(aTile);
		mLive.push_back(aLive);
	}
	mCopy.mHasCounts = theLayout.mHasCounts;
	mCopy.UpdateFree();
	mKeyGoal = false;
	for (Tile* aTile : mTiles)
		if (aTile->mTaken == 0 && aTile->mKind == 0x2b)
			mKeyGoal = true;
}

// The state of the copy as a string: each tile's kind, or a mark for a tile taken
std::string Bot::Key() const
{
	std::string aKey;
	for (Tile* aTile : mTiles)
		aKey += aTile->mTaken ? '\x7f' : (char)aTile->mKind;
	return aKey;
}

// The goal: the key tiles taken in an escape level, else no tile left
bool Bot::Done() const
{
	if (mKeyGoal)
		return !mSteps.empty() && mTiles[mSteps.back().mFirst]->mKind == 0x2b;
	for (Tile* aTile : mTiles)
		if (aTile->mTaken == 0)
			return false;
	return true;
}

// What UpdateFree (Layout 0x414780) would do after the tiles in theChanged were taken or put back, for the tiles it
// can change: a tile is free by the tiles on its own level two places to either side and on the level above it, so only
// the neighbours of a changed tile (and the changed tile itself when it came back) can differ
void Bot::Refresh(const std::vector<int>& theChanged, bool theBack)
{
	for (int aChanged : theChanged)
	{
		Tile* aThat = mTiles[aChanged];
		if (theBack)
			mCopy.UpdateFree(aThat);
		for (Tile* aTile : mTiles)
		{
			if (aTile->mTaken != 0 || aTile == aThat)
				continue;
			int aDeltaX = abs(aTile->mX - aThat->mX);
			int aDeltaY = abs(aTile->mY - aThat->mY);
			if ((aTile->mZ == aThat->mZ && aDeltaX == 2 && aDeltaY <= 1) || (aTile->mZ == aThat->mZ - 1 && aDeltaX <= 1 && aDeltaY <= 1))
				mCopy.UpdateFree(aTile);
		}
	}
}

void Bot::Apply(Step& theStep)
{
	theStep.mTaken.clear();
	theStep.mChanged = -1;
	Tile* aFirst = mTiles[theStep.mFirst];
	Tile* aSecond = mTiles[theStep.mSecond];
	int aKind = aFirst->mKind;
	if (aKind == 0x2e)
	{
		theStep.mTaken.push_back(theStep.mFirst);
		for (size_t i = 0; i < mTiles.size(); i++)
			if (mTiles[i]->mTaken == 0 && KindsMatch(mTiles[i]->mKind, aSecond->mKind))
				theStep.mTaken.push_back((int)i);
	}
	else if (aKind == 0x2f)
	{
		theStep.mTaken.push_back(theStep.mSecond);
		theStep.mChanged = theStep.mFirst;
		aFirst->mKind = aSecond->mKind;
	}
	else
	{
		theStep.mTaken.push_back(theStep.mFirst);
		theStep.mTaken.push_back(theStep.mSecond);
	}
	for (int i : theStep.mTaken)
		mTiles[i]->mTaken = 1;
	Refresh(theStep.mTaken, false);
}

void Bot::Undo(Step& theStep)
{
	for (int i : theStep.mTaken)
		mTiles[i]->mTaken = 0;
	if (theStep.mChanged >= 0)
		mTiles[theStep.mChanged]->mKind = 0x2f;
	Refresh(theStep.mTaken, true);
}

// A point of theTile that the hit test (Layout::HitTest, 0x414750) gives to it, in board coordinates; a tile can be
// visible (IsVisible) by a sliver the hit test gives to a tile drawn later
bool Bot::ClickPoint(Tile* theTile, int& theX, int& theY)
{
	for (int y = theTile->mRect.top + 26; y < theTile->mRect.bottom; y += 3)
		for (int x = theTile->mRect.left + 24; x < theTile->mRect.right; x += 3)
		{
			int z = -1, aTileX, aTileY;
			if (mCopy.HitTest(x, y, &z, &aTileX, &aTileY) == theTile)
			{
				theX = x;
				theY = y;
				return true;
			}
		}
	for (int y = theTile->mRect.top; y < theTile->mRect.bottom; y++)
		for (int x = theTile->mRect.left; x < theTile->mRect.right; x++)
		{
			int z = -1, aTileX, aTileY;
			if (mCopy.HitTest(x, y, &z, &aTileX, &aTileY) == theTile)
			{
				theX = x;
				theY = y;
				return true;
			}
		}
	return false;
}

// A free tile can lie under a tile two levels up with nothing between them (Layout::IsFree, 0x414c20, looks one
// level up), and then no click reaches it
bool Bot::CanClick(Tile* theTile)
{
	bool aHigher = false;
	for (Tile* aTile : mTiles)
		if (aTile->mTaken == 0 && theTile->mZ + 1 < aTile->mZ && aTile->mRect.left < theTile->mRect.right &&
			theTile->mRect.left < aTile->mRect.right && aTile->mRect.top < theTile->mRect.bottom &&
			theTile->mRect.top < aTile->mRect.bottom)
			aHigher = true;
	int x, y;
	return !aHigher || ClickPoint(theTile, x, y);
}

// The moves of the copy as it is, the ones nearest the key tiles first (an escape level), and for any other board in
// the board's order (or shuffled by the generator of a restart)
void Bot::Moves(std::vector<std::pair<int, Step>>& theMoves)
{
	std::vector<int> aKeys;
	std::vector<int> aFree;
	for (size_t i = 0; i < mTiles.size(); i++)
	{
		Tile* aTile = mTiles[i];
		if (aTile->mTaken != 0)
			continue;
		if (aTile->mKind == 0x2b)
			aKeys.push_back((int)i);
		if (aTile->mFree && CanClick(aTile))
			aFree.push_back((int)i);
	}
	auto aNear = [&](Tile* theTile) {
		int aBest = 1000;
		for (int aKeyTile : aKeys)
		{
			Tile* aK = mTiles[aKeyTile];
			aBest = std::min(aBest, abs(aK->mX - theTile->mX) + abs(aK->mY - theTile->mY) + abs(aK->mZ - theTile->mZ));
		}
		return aBest;
	};
	auto anAdd = [&](int theFirst, int theSecond) {
		Step aStep;
		aStep.mFirst = theFirst;
		aStep.mSecond = theSecond;
		int aRank = std::min(aNear(mTiles[theFirst]), aNear(mTiles[theSecond]));
		if (mTiles[theFirst]->mKind == 0x2b)
			aRank = -1;
		// every other start then takes the move that leaves the most tiles free first (the freest board keeps the most
		// ways open), the others the board's order
		int aFreeAfter = 0;
		if (mFreest)
		{
			Apply(aStep);
			for (Tile* aTile : mTiles)
				if (aTile->mTaken == 0 && aTile->mFree)
					aFreeAfter++;
			Undo(aStep);
		}
		theMoves.push_back({aRank * 1000 - aFreeAfter, aStep});
	};
	for (size_t i = 0; i < aFree.size(); i++)
		for (size_t j = i + 1; j < aFree.size(); j++)
		{
			Tile* aFreeTile = mTiles[aFree[i]];
			Tile* anOtherFree = mTiles[aFree[j]];
			if (0x2b < aFreeTile->mKind || 0x2b < anOtherFree->mKind || !TilesMatch(aFreeTile->mKind, anOtherFree->mKind))
				continue;
			anAdd(aFree[i], aFree[j]);
		}
	// a lock matters when it holds a goal tile: beside it, or on it. A start may take the keys only to those (a key spent
	// on another lock may be the one a lock that matters then lacks)
	auto aHoldsGoal = [&](Tile* theLock) {
		for (int aKeyTile : aKeys)
		{
			Tile* aK = mTiles[aKeyTile];
			int aDeltaX = abs(aK->mX - theLock->mX);
			int aDeltaY = abs(aK->mY - theLock->mY);
			if ((aK->mZ == theLock->mZ && aDeltaX == 2 && aDeltaY <= 1) || (aK->mZ + 1 == theLock->mZ && aDeltaX <= 1 && aDeltaY <= 1))
				return true;
		}
		return false;
	};
	for (int i : aFree)
	{
		Tile* aSpecial = mTiles[i];
		if (aSpecial->mKind < 0x2c || aSpecial->mKind == 0x2d)
			continue;
		for (size_t j = 0; j < mTiles.size(); j++)
		{
			Tile* aTarget = mTiles[j];
			int aTargetX, aTargetY;
			if (aTarget->mTaken != 0 || (int)j == i || !(aSpecial->mKind == 0x2c ? aTarget->mKind == 0x2d : aTarget->mKind < 0x2b))
				continue;
			if (aSpecial->mKind == 0x2c && mOnlyGoalLocks && !aKeys.empty() && !aHoldsGoal(aTarget))
				continue;
			if (mCopy.IsVisible(aTarget) && ClickPoint(aTarget, aTargetX, aTargetY))
				anAdd(i, (int)j);
		}
	}
	if (mShuffle)
	{
		std::mt19937 anOrder(mOrder);
		std::shuffle(theMoves.begin(), theMoves.end(), anOrder);
	}
	std::stable_sort(theMoves.begin(), theMoves.end(),
		[](const std::pair<int, Step>& x, const std::pair<int, Step>& y) { return x.first < y.first; });
}

bool Bot::SolveFrom()
{
	if (Done())
		return true;
	std::string aKey = Key();
	if (mDead.count(aKey))
		return false;
	if (mDeepest < (int)mSteps.size())
		mDeepest = (int)mSteps.size();
	if (mLimit < ++mNodes)
	{
		mGaveUp = true;
		return false;
	}
	std::vector<std::pair<int, Step>> aMoves;
	Moves(aMoves);
	for (auto& aRanked : aMoves)
	{
		mSteps.push_back(aRanked.second);
		Apply(mSteps.back());
		if (SolveFrom())
			return true;
		Undo(mSteps.back());
		mSteps.pop_back();
		if (mGaveUp)
			return false;
	}
	mDead.insert(aKey);
	return false;
}

// The search from the copy as it is. A search that goes wrong stays wrong (one in which the first pairs taken bury the
// rest), so it gives up early and starts again with the equally ranked moves in another order, with a limit that
// grows with each start
bool Bot::Search()
{
	bool aSolved = false;
	long aSpent = 0;
	for (int aTry = 0; !aSolved && aTry < 60 && aSpent < SEARCH_BUDGET; aTry++)
	{
		mSteps.clear();
		mDead.clear();
		mNodes = 0;
		mGaveUp = false;
		mShuffle = 1 < aTry;
		mFreest = aTry % 2 == 0;
		mOnlyGoalLocks = (aTry / 2) % 2 == 0;
		mOrder = (unsigned)aTry;
		mDeepest = 0;
		mLimit = std::min(400000L, 1500L << (aTry / 8));
		aSolved = SolveFrom();
		aSpent += mNodes;
		if (!mGaveUp)
			break;
	}
	if (getenv("BOTDEBUG") != nullptr)
		fprintf(stderr, "search: %ld positions, solved %d, gave up %d\n", aSpent, (int)aSolved, (int)mGaveUp);
	return aSolved;
}

// The step as a move of the live board: its tiles, kinds and the points that click them in game coordinates
bool Bot::Describe(Board& theBoard, const Step& theStep, BotMove& theMove)
{
	Tile* aFirst = mTiles[theStep.mFirst];
	Tile* aSecond = mTiles[theStep.mSecond];
	int aPos[2];
	theBoard.GetScreenPos(aPos);
	int aFirstX, aFirstY, aSecondX, aSecondY;
	if (!ClickPoint(aFirst, aFirstX, aFirstY) || !ClickPoint(aSecond, aSecondX, aSecondY))
		return false;
	theMove.mFirst = mLive[theStep.mFirst];
	theMove.mSecond = mLive[theStep.mSecond];
	theMove.mFirstKind = aFirst->mKind;
	theMove.mSecondKind = aSecond->mKind;
	theMove.mX1 = aPos[0] + theBoard.mOffX + aFirstX;
	theMove.mY1 = aPos[1] + theBoard.mOffY + aFirstY;
	theMove.mX2 = aPos[0] + theBoard.mOffX + aSecondX;
	theMove.mY2 = aPos[1] + theBoard.mOffY + aSecondY;
	theMove.mSpecial = 0x2b < aFirst->mKind;
	theMove.mEnds = aFirst->mKind == 0x2b;
	return true;
}

// The search alone on a layout (tools/dealtest.cpp times it): the number of moves to the end, or -1 when none was found
int Bot::SolveLayout(Layout& theLayout, long* theNodes)
{
	Copy(theLayout);
	bool aSolved = Search();
	if (theNodes != nullptr)
		*theNodes = mNodes;
	return aSolved ? (int)mSteps.size() : -1;
}

int Bot::CountUnclickable(Layout& theLayout)
{
	Copy(theLayout);
	int aCount = 0;
	for (Tile* aTile : mTiles)
		if (aTile->mTaken == 0 && aTile->mFree && !CanClick(aTile))
			aCount++;
	return aCount;
}

bool Bot::Solve(Board& theBoard, std::vector<BotMove>& theMoves)
{
	theMoves.clear();
	Copy(theBoard.mLayout);
	if (!Search())
		return false;
	std::vector<Step> aPlan = mSteps;
	// back to the board as it was, then each step described as it comes
	for (auto aStep = aPlan.rbegin(); aStep != aPlan.rend(); ++aStep)
		Undo(*aStep);
	for (Step& aStep : aPlan)
	{
		BotMove aMove;
		if (!Describe(theBoard, aStep, aMove))
			return false;
		theMoves.push_back(aMove);
		Apply(aStep);
	}
	return true;
}

bool Bot::NextMove(Board& theBoard, BotMove& theMove)
{
	if (theBoard.mOver || theBoard.mEscaping || !theBoard.mStarted)
		return false;
	Copy(theBoard.mLayout);
	std::string aKey = Key();
	std::string aLayout = theBoard.mLayout.mName + "/" + std::to_string(mTiles.size());
	// follow the plan found before while the board goes as it said
	size_t aStep = 0;
	bool aFound = false;
	if (mPlanLayout == aLayout)
		for (; aStep < mPlan.size(); aStep++)
			if (mPlanKeys[aStep] == aKey)
			{
				aFound = true;
				break;
			}
	// after a search that found no way to the end the bot plays on without one while the board goes as its moves say
	// (a shuffle or a new game changes it, and the next call searches again)
	if (!aFound && !(mNoPlan && aKey == mExpected))
	{
		mNoPlan = false;
		mPlan.clear();
		mPlanKeys.clear();
		mPlanLayout = aLayout;
		if (Search())
		{
			std::vector<Step> aSolution = mSteps;
			for (auto aBack = aSolution.rbegin(); aBack != aSolution.rend(); ++aBack)
				Undo(*aBack);
			for (Step& aNext : aSolution)
			{
				mPlanKeys.push_back(Key());
				mPlan.push_back(aNext);
				Apply(aNext);
			}
			for (auto aBack = aSolution.rbegin(); aBack != aSolution.rend(); ++aBack)
				Undo(*aBack);
			aStep = 0;
			aFound = !mPlan.empty();
		}
		else
			mNoPlan = true;
	}
	if (aFound)
	{
		Step aNext = mPlan[aStep];
		if (mTiles[aNext.mFirst]->mTaken == 0 && mTiles[aNext.mSecond]->mTaken == 0 &&
			Describe(theBoard, aNext, theMove))
			return true;
	}
	// no way to the end was found: the best ranked move that can be clicked
	mPlan.clear();
	mPlanKeys.clear();
	mShuffle = false;
	std::vector<std::pair<int, Step>> aMoves;
	Moves(aMoves);
	for (auto& aRanked : aMoves)
		if (Describe(theBoard, aRanked.second, theMove))
		{
			Apply(aRanked.second);
			mExpected = Key();
			Undo(aRanked.second);
			return true;
		}
	return false;
}

} // namespace mahjongchina
