#include "PuzzleSolver.h"

#include <algorithm>
#include <climits>
#include <cstring>

using namespace Sexy;

// Zobrist keys from a fixed generator (splitmix64), so hashes are the same on every machine and the game's
// own random numbers are not touched. Per cell: colours 0..4 and the bag; then the released count; then the
// extra colours 5..7 per cell, added after the others so the first keys stay as they were.
enum
{
	ZOBRIST_CELL = 6,
	ZOBRIST_RELEASED = PuzzleModel::ROWS * PuzzleModel::COLS * ZOBRIST_CELL,
	ZOBRIST_EXTRA = ZOBRIST_RELEASED + 64,
	ZOBRIST_EXTRA_CELL = PuzzleModel::COLORS - 5
};

static const uint64_t* ZobristKeys()
{
	static uint64_t sKeys[ZOBRIST_EXTRA + PuzzleModel::ROWS * PuzzleModel::COLS * ZOBRIST_EXTRA_CELL];
	static bool sReady = false;
	if (!sReady)
	{
		uint64_t aState = 0x42696720'4d6f6e65ull; // "Big Mone"
		for (uint64_t& aKey : sKeys)
		{
			uint64_t z = (aState += 0x9e3779b97f4a7c15ull);
			z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
			z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
			aKey = z ^ (z >> 31);
		}
		sReady = true;
	}
	return sKeys;
}

static const int gDirs[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

PuzzleModel::PuzzleModel()
{
	memset(mCell, EMPTY, sizeof(mCell));
	mReleased = 0;
	mNeeded = 0;
	mDiagonal = false;
}

void PuzzleModel::GetGroup(int theCol, int theRow, std::vector<int>* theCells) const
{
	theCells->clear();
	if ((theCol < 0) || (theCol >= COLS) || (theRow < 0) || (theRow >= ROWS))
		return;
	int aColor = mCell[theRow][theCol];
	if ((aColor == EMPTY) || (aColor == BAG))
		return;

	bool aSeen[ROWS * COLS] = {};
	theCells->push_back(theRow * COLS + theCol);
	aSeen[theRow * COLS + theCol] = true;
	int aNumDirs = mDiagonal ? 8 : 4;
	for (size_t i = 0; i < theCells->size(); i++)
	{
		int aRow = (*theCells)[i] / COLS;
		int aCol = (*theCells)[i] % COLS;
		for (int d = 0; d < aNumDirs; d++)
		{
			int c = aCol + gGroupDirs[d][0];
			int r = aRow + gGroupDirs[d][1];
			if ((c < 0) || (c >= COLS) || (r < 0) || (r >= ROWS) || (aSeen[r * COLS + c]) || (mCell[r][c] != aColor))
				continue;
			aSeen[r * COLS + c] = true;
			theCells->push_back(r * COLS + c);
		}
	}
}

void PuzzleModel::ClickOutcomes(int theCol, int theRow, std::vector<PuzzleModel>* theOutcomes) const
{
	theOutcomes->clear();
	std::vector<int> aGroup;
	GetGroup(theCol, theRow, &aGroup);
	if (aGroup.size() < 2) // Board::mMinGroup in Puzzle
		return;

	PuzzleModel aBoard = *this;
	for (int aCell : aGroup)
		aBoard.mCell[aCell / COLS][aCell % COLS] = EMPTY;
	int aFall[COLS];
	for (int& aDistance : aFall)
		aDistance = -1;
	aBoard.Compact(aFall);
	aBoard.Settle(aFall, theOutcomes);
}

void PuzzleModel::Settle(const int* theFall, std::vector<PuzzleModel>* theOutcomes) const
{
	int aShortest = INT_MAX;
	for (int c = 0; c < COLS; c++)
	{
		if ((theFall[c] >= 0) && (mCell[ROWS - 1][c] == BAG))
			aShortest = std::min(aShortest, theFall[c]);
	}
	if (aShortest == INT_MAX)
	{
		uint64_t aHash = Hash();
		for (const PuzzleModel& anOutcome : *theOutcomes)
		{
			if (anOutcome.Hash() == aHash)
				return;
		}
		theOutcomes->push_back(*this);
		return;
	}

	int aLanding[COLS];
	int aCount = 0;
	for (int c = 0; c < COLS; c++)
	{
		if ((theFall[c] == aShortest) && (mCell[ROWS - 1][c] == BAG))
			aLanding[aCount++] = c;
	}

	// Any of them can land first, alone or with others: every non-empty subset, all of them first
	int anAll = (1 << std::min(aCount, (int) MAX_ORDERED_BAGS)) - 1;
	for (int aMask = anAll; aMask >= 1; aMask--)
	{
		PuzzleModel aBoard = *this;
		int aFall[COLS];
		memcpy(aFall, theFall, sizeof(aFall));
		for (int i = 0; i < aCount; i++)
		{
			if ((i >= MAX_ORDERED_BAGS) || ((aMask & (1 << i)) != 0))
			{
				aBoard.mCell[ROWS - 1][aLanding[i]] = EMPTY;
				aFall[aLanding[i]] = -1;
				aBoard.mReleased++;
			}
		}
		aBoard.Compact(aFall);
		aBoard.Settle(aFall, theOutcomes);
		if (aCount > MAX_ORDERED_BAGS)
			break;
	}
}

int PuzzleModel::FloodSize(int theCol, int theRow, uint8_t* theMarks) const
{
	// The start cell counts even if marked; then every unmarked coin it reaches through unmarked coins.
	// A bag stops the flood, and a bag as the start counts nothing.
	if (mCell[theRow][theCol] == BAG)
		return 0;
	int aStack[ROWS * COLS];
	int aTop = 0;
	int aCount = 1;
	theMarks[theRow * COLS + theCol] = 1;
	aStack[aTop++] = theRow * COLS + theCol;
	while (aTop > 0)
	{
		int aCell = aStack[--aTop];
		int aRow = aCell / COLS;
		int aCol = aCell % COLS;
		for (const auto& aDir : gDirs)
		{
			int c = aCol + aDir[0];
			int r = aRow + aDir[1];
			if ((c < 0) || (c >= COLS) || (r < 0) || (r >= ROWS))
				continue;
			int8_t v = mCell[r][c];
			if ((v == EMPTY) || (v == BAG) || (theMarks[r * COLS + c] != 0))
				continue;
			theMarks[r * COLS + c] = 1;
			aCount++;
			aStack[aTop++] = r * COLS + c;
		}
	}
	return aCount;
}

void PuzzleModel::Compact(int* theFall)
{
	// Gravity, per column
	for (int c = 0; c < COLS; c++)
	{
		int aDest = ROWS - 1;
		for (int aSrc = ROWS - 1; aSrc >= 0; aSrc--)
		{
			int8_t v = mCell[aSrc][c];
			if (v == EMPTY)
				continue;
			if (aSrc != aDest)
			{
				mCell[aDest][c] = v;
				if ((v == BAG) && (aDest == ROWS - 1))
					theFall[c] = aDest - aSrc;
			}
			aDest--;
		}
		for (int r = aDest; r >= 0; r--)
			mCell[r][c] = EMPTY;
	}

	// Empty bottom cells close up, left to right, as the grid changes
	const int aBottom = ROWS - 1;
	uint8_t aMarks[ROWS * COLS];
	for (int aCol = 0; aCol < COLS; aCol++)
	{
		if (mCell[aBottom][aCol] != EMPTY)
			continue;
		memset(aMarks, 0, sizeof(aMarks));

		int aLeft;
		int aLeftSize = 0;
		for (aLeft = aCol; aLeft >= 0; aLeft--)
		{
			if (mCell[aBottom][aLeft] != EMPTY)
			{
				aLeftSize = FloodSize(aLeft, aBottom, aMarks);
				break;
			}
		}
		int aRight = aCol;
		while ((aRight < COLS) && (mCell[aBottom][aRight] == EMPTY))
			aRight++;
		int aRightSize = 0;
		if (aRight < COLS)
			aRightSize = FloodSize(aRight, aBottom, aMarks);

		int aMoveRight = 0;
		int aMoveLeft = 0;
		if ((aLeft >= 0) && (aRight < COLS))
		{
			int aGap = aRight - aLeft - 1;
			aMoveRight = aGap / 2;
			aMoveLeft = aGap / 2;
			if (aGap % 2 != 0)
			{
				if (aLeftSize < aRightSize)
					aMoveRight++;
				else
					aMoveLeft++;
			}
		}

		int aPos = aCol;
		for (int aStep = 0; aStep < aMoveRight; aStep++)
		{
			for (int k = aPos; k >= 1; k--)
			{
				for (int r = 0; r < ROWS; r++)
				{
					mCell[r][k] = mCell[r][k - 1];
					mCell[r][k - 1] = EMPTY;
				}
				theFall[k] = theFall[k - 1];
				theFall[k - 1] = -1;
			}
			aPos++;
		}
		for (int aStep = 0; aStep < aMoveLeft; aStep++)
		{
			for (int k = aPos; k < COLS - 1; k++)
			{
				for (int r = 0; r < ROWS; r++)
				{
					mCell[r][k] = mCell[r][k + 1];
					mCell[r][k + 1] = EMPTY;
				}
				theFall[k] = theFall[k + 1];
				theFall[k + 1] = -1;
			}
		}
	}
}

uint64_t PuzzleModel::Hash() const
{
	const uint64_t* aKeys = ZobristKeys();
	uint64_t aHash = aKeys[ZOBRIST_RELEASED + (mReleased & 63)];
	for (int i = 0; i < ROWS * COLS; i++)
	{
		int8_t v = mCell[i / COLS][i % COLS];
		if (v == BAG)
			aHash ^= aKeys[i * ZOBRIST_CELL + 5];
		else if ((v >= 5) && (v < COLORS))
			aHash ^= aKeys[ZOBRIST_EXTRA + i * ZOBRIST_EXTRA_CELL + (v - 5)];
		else if (v != EMPTY)
			aHash ^= aKeys[i * ZOBRIST_CELL + v];
	}
	return (aHash == 0) ? 1 : aHash;
}

bool PuzzleModel::Rate(int* theScore) const
{
	int aCount[COLORS] = {};
	int aCoins = 0;
	for (int r = 0; r < ROWS; r++)
	{
		for (int c = 0; c < COLS; c++)
		{
			int8_t v = mCell[r][c];
			if ((v >= 0) && (v < COLORS))
			{
				aCount[v]++;
				aCoins++;
			}
		}
	}

	// The coins under each bag that can still leave. A bag is the top piece of its column (nothing lands on
	// one in Puzzle); one on the bottom row did not fall there and stays.
	int aUnder[COLS];
	int aBags = 0;
	for (int c = 0; c < COLS; c++)
	{
		int r = 0;
		while ((r < ROWS) && (mCell[r][c] == EMPTY))
			r++;
		if ((r >= ROWS - 1) || (mCell[r][c] != BAG))
			continue;
		bool canLeave = true;
		for (int b = r + 1; (b < ROWS) && (canLeave); b++)
		{
			int8_t v = mCell[b][c];
			canLeave = (v >= 0) && (v < COLORS) && (aCount[v] >= 2);
		}
		if (canLeave)
			aUnder[aBags++] = ROWS - 1 - r;
	}

	int aWanted = mNeeded - mReleased;
	if (aWanted <= 0)
	{
		*theScore = INT_MAX;
		return true;
	}
	if (aBags < aWanted)
		return false;
	std::sort(aUnder, aUnder + aBags);
	int aWork = 0;
	for (int i = 0; i < aWanted; i++)
		aWork += aUnder[i];
	*theScore = mReleased * 1000000 - aWork * 1000 - aCoins;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

PuzzleSolver::PuzzleSolver()
{
	mStatus = STATUS_NONE;
	mPositions = 0;
	mBoardHash = 0;
	mSeenCount = 0;
	mBeam = 0;
	mBeamDropped = false;
	mExpand = 0;
}

bool PuzzleSolver::IsSeen(uint64_t theHash) const
{
	size_t aMask = mSeen.size() - 1;
	for (size_t i = (size_t) theHash & aMask; mSeen[i] != 0; i = (i + 1) & aMask)
	{
		if (mSeen[i] == theHash)
			return true;
	}
	return false;
}

bool PuzzleSolver::Remember(uint64_t theHash)
{
	size_t aMask = mSeen.size() - 1;
	size_t i = (size_t) theHash & aMask;
	for (; mSeen[i] != 0; i = (i + 1) & aMask)
	{
		if (mSeen[i] == theHash)
			return false;
	}
	mSeen[i] = theHash;
	mSeenCount++;
	return true;
}

void PuzzleSolver::ClearSeen()
{
	if (mSeen.empty())
		mSeen.resize((size_t) 1 << SEEN_BITS);
	else
		std::fill(mSeen.begin(), mSeen.end(), 0);
	mSeenCount = 0;
}

void PuzzleSolver::Play(PuzzleModel* theBoard, const Move& theMove)
{
	theBoard->ClickOutcomes(theMove.mCol, theMove.mRow, &mOutcomes);
	if (theMove.mOutcome < (int) mOutcomes.size())
		*theBoard = mOutcomes[theMove.mOutcome];
}

bool PuzzleSolver::SetPlan(const PuzzleModel& theBoard, const std::vector<Move>& thePlan)
{
	PuzzleModel aBoard = theBoard;
	std::vector<Move> aPlan;
	std::vector<uint64_t> aHashes;
	std::vector<int> aGroup;
	for (const Move& aMove : thePlan)
	{
		if (aBoard.IsWon())
			break;
		aBoard.GetGroup(aMove.mCol, aMove.mRow, &aGroup);
		if (aGroup.size() < 2)
			return false;
		Play(&aBoard, aMove);
		aPlan.push_back(aMove);
		aHashes.push_back(aBoard.Hash());
	}
	if (!aBoard.IsWon())
		return false;
	mBoard = theBoard;
	mBoardHash = theBoard.Hash();
	mPositions = 0;
	mPlan = aPlan;
	mPlanHashes = aHashes;
	Finish(STATUS_WINNABLE);
	return true;
}

void PuzzleSolver::SetBoard(const PuzzleModel& theBoard)
{
	uint64_t aHash = theBoard.Hash();
	if ((mStatus != STATUS_NONE) && (aHash == mBoardHash) && (theBoard.mNeeded == mBoard.mNeeded))
		return;
	mBoard = theBoard;
	mBoardHash = aHash;

	// The player made the plan's next click (and the bags left in the order it counted on): the rest of the
	// plan still wins
	if ((mStatus == STATUS_WINNABLE) && (!mPlanHashes.empty()) && (mPlanHashes[0] == aHash))
	{
		mPlan.erase(mPlan.begin());
		mPlanHashes.erase(mPlanHashes.begin());
		return;
	}

	mPlan.clear();
	mPlanHashes.clear();
	mPositions = 0;
	mStatus = STATUS_THINKING;
	int aScore;
	if (theBoard.IsWon())
		Finish(STATUS_WINNABLE);
	else if (!theBoard.Rate(&aScore))
		Finish(STATUS_LOST);
	else
		StartBeam(0);
}

void PuzzleSolver::Step(int theBudget)
{
	int aStart = mPositions;
	while ((mStatus == STATUS_THINKING) && (mPositions - aStart < theBudget))
	{
		if (mPositions >= MAX_POSITIONS)
			Finish(STATUS_GAVE_UP);
		else if (mBeam < NUM_BEAMS)
			StepBeam();
		else
			StepDepthFirst();
	}
}

void PuzzleSolver::StartBeam(int theBeam)
{
	mBeam = theBeam;
	mBeamDropped = false;
	ClearSeen();
	Remember(mBoardHash);
	mFrontier.assign(1, mBoard);
	mExpand = 0;
	mCandidates.clear();
	Candidate aRoot = { -1, { -1, -1, 0 }, 0 };
	mLayers.assign(1, std::vector<Candidate>(1, aRoot));
}

// One piece of a beam: the clicks from one frontier position, or the next layer
void PuzzleSolver::StepBeam()
{
	if (mExpand < mFrontier.size())
	{
		const PuzzleModel aBoard = mFrontier[mExpand];
		bool aDone[PuzzleModel::ROWS * PuzzleModel::COLS] = {};
		std::vector<int> aGroup;
		for (int r = 0; r < PuzzleModel::ROWS; r++)
		{
			for (int c = 0; c < PuzzleModel::COLS; c++)
			{
				if (aDone[r * PuzzleModel::COLS + c])
					continue;
				aBoard.GetGroup(c, r, &aGroup);
				for (int aCell : aGroup)
					aDone[aCell] = true;
				if (aGroup.size() < 2)
					continue;
				aBoard.ClickOutcomes(c, r, &mOutcomes);
				for (size_t i = 0; i < mOutcomes.size(); i++)
				{
					const PuzzleModel& aChild = mOutcomes[i];
					mPositions++;
					if (!Remember(aChild.Hash()))
						continue;
					Candidate aCandidate = { (int) mExpand, { (int8_t) c, (int8_t) r, (int8_t) i }, 0 };
					if (aChild.IsWon())
					{
						// The clicks back to the root
						std::vector<Move> aMoves(1, aCandidate.mMove);
						int aIndex = (int) mExpand;
						for (int d = (int) mLayers.size() - 1; d >= 1; d--)
						{
							aMoves.insert(aMoves.begin(), mLayers[d][aIndex].mMove);
							aIndex = mLayers[d][aIndex].mParent;
						}
						Found(aMoves);
						return;
					}
					if (!aChild.Rate(&aCandidate.mScore))
						continue;
					mCandidates.push_back(aCandidate);
				}
			}
		}
		mExpand++;
		return;
	}

	if (mCandidates.empty())
	{
		// A beam that never left out a position has tried them all
		if (!mBeamDropped)
			Finish(STATUS_LOST);
		else if (mBeam + 1 < NUM_BEAMS)
			StartBeam(mBeam + 1);
		else
			StartDepthFirst();
		return;
	}

	size_t aWidth = (size_t) 16 << (2 * mBeam);
	std::stable_sort(mCandidates.begin(), mCandidates.end(),
		[](const Candidate& a, const Candidate& b) { return a.mScore > b.mScore; });
	if (mCandidates.size() > aWidth)
	{
		mCandidates.resize(aWidth);
		mBeamDropped = true;
	}
	std::vector<PuzzleModel> aNext;
	aNext.reserve(mCandidates.size());
	for (const Candidate& aCandidate : mCandidates)
	{
		aNext.push_back(mFrontier[aCandidate.mParent]);
		Play(&aNext.back(), aCandidate.mMove);
	}
	mFrontier.swap(aNext);
	mLayers.push_back(mCandidates);
	mCandidates.clear();
	mExpand = 0;
}

void PuzzleSolver::StartDepthFirst()
{
	mBeam = NUM_BEAMS;
	mFrontier.clear();
	mFrontier.shrink_to_fit();
	mCandidates.clear();
	mLayers.clear();
	ClearSeen();
	Remember(mBoardHash);
	mStack.clear();
	Move aNone = { -1, -1, 0 };
	Expand(mBoard, mBoardHash, aNone);
}

void PuzzleSolver::Expand(const PuzzleModel& theBoard, uint64_t theHash, Move theMove)
{
	Frame aFrame;
	aFrame.mNext = 0;
	aFrame.mMove = theMove;
	aFrame.mHash = theHash;

	bool aDone[PuzzleModel::ROWS * PuzzleModel::COLS] = {};
	std::vector<int> aGroup;
	for (int r = 0; r < PuzzleModel::ROWS; r++)
	{
		for (int c = 0; c < PuzzleModel::COLS; c++)
		{
			if (aDone[r * PuzzleModel::COLS + c])
				continue;
			theBoard.GetGroup(c, r, &aGroup);
			for (int aCell : aGroup)
				aDone[aCell] = true;
			if (aGroup.size() < 2)
				continue;

			theBoard.ClickOutcomes(c, r, &mOutcomes);
			for (size_t i = 0; i < mOutcomes.size(); i++)
			{
				Child aChild;
				aChild.mBoard = mOutcomes[i];
				aChild.mHash = aChild.mBoard.Hash();
				aChild.mMove.mCol = (int8_t) c;
				aChild.mMove.mRow = (int8_t) r;
				aChild.mMove.mOutcome = (int8_t) i;
				mPositions++;
				if (IsSeen(aChild.mHash))
					continue;
				if (!aChild.mBoard.Rate(&aChild.mScore))
				{
					Remember(aChild.mHash); // a dead end, whichever way it is reached
					continue;
				}
				aFrame.mChildren.push_back(aChild);
			}
		}
	}

	std::stable_sort(aFrame.mChildren.begin(), aFrame.mChildren.end(),
		[](const Child& a, const Child& b) { return a.mScore > b.mScore; });
	mStack.push_back(std::move(aFrame));
}

void PuzzleSolver::StepDepthFirst()
{
	if (mStack.empty())
	{
		Finish(STATUS_LOST);
		return;
	}
	Frame& aFrame = mStack.back();
	if (aFrame.mNext >= aFrame.mChildren.size())
	{
		mStack.pop_back();
		return;
	}
	const Child& aChild = aFrame.mChildren[aFrame.mNext++];
	mPositions++;
	if (!Remember(aChild.mHash))
		return;
	if (aChild.mBoard.IsWon())
	{
		std::vector<Move> aMoves;
		for (size_t i = 1; i < mStack.size(); i++)
			aMoves.push_back(mStack[i].mMove);
		aMoves.push_back(aChild.mMove);
		Found(aMoves);
		return;
	}
	if (mSeenCount >= (3 << (SEEN_BITS - 2)))
	{
		Finish(STATUS_GAVE_UP);
		return;
	}
	PuzzleModel aBoard = aChild.mBoard; // the push below can move the frames
	Expand(aBoard, aChild.mHash, aChild.mMove);
}

void PuzzleSolver::Found(const std::vector<Move>& theMoves)
{
	mPlan = theMoves;
	mPlanHashes.clear();
	PuzzleModel aBoard = mBoard;
	for (const Move& aMove : mPlan)
	{
		Play(&aBoard, aMove);
		mPlanHashes.push_back(aBoard.Hash());
	}
	Finish(STATUS_WINNABLE);
}

void PuzzleSolver::Finish(Status theStatus)
{
	mStatus = theStatus;
	mStack.clear();
	mStack.shrink_to_fit();
	mFrontier.clear();
	mFrontier.shrink_to_fit();
	mCandidates.clear();
	mLayers.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

PuzzleClickCheck::PuzzleClickCheck()
{
	mPositions = 0;
	mNext = 0;
	mSearching = false;
	mGaveUp = false;
	mVerdict = VERDICT_NONE;
}

void PuzzleClickCheck::Start(const PuzzleModel& theBoard, int theCol, int theRow)
{
	mOutcomes.clear();
	theBoard.ClickOutcomes(theCol, theRow, &mOutcomes);
	mPositions = 0;
	mNext = 0;
	mSearching = false;
	mGaveUp = false;
	mVerdict = mOutcomes.empty() ? VERDICT_UNKNOWN : VERDICT_NONE;
}

int PuzzleClickCheck::Step(int theBudget)
{
	int aSpent = 0;
	while (mVerdict == VERDICT_NONE)
	{
		if (mSearching)
		{
			int aBefore = mSolver.mPositions;
			mSolver.Step(theBudget - aSpent);
			aSpent += mSolver.mPositions - aBefore;
			mPositions += mSolver.mPositions - aBefore;
			if (mSolver.mStatus == PuzzleSolver::STATUS_THINKING)
				break;
			mSearching = false;
			if (mSolver.mStatus == PuzzleSolver::STATUS_WINNABLE)
			{
				mVerdict = VERDICT_WINS;
				break;
			}
			if (mSolver.mStatus == PuzzleSolver::STATUS_GAVE_UP)
				mGaveUp = true;
			mNext++;
			continue;
		}

		if (mNext >= mOutcomes.size())
		{
			mVerdict = mGaveUp ? VERDICT_UNKNOWN : VERDICT_LOSES;
			break;
		}
		const PuzzleModel& anOutcome = mOutcomes[mNext];
		int aScore;
		if (anOutcome.IsWon())
		{
			mVerdict = VERDICT_WINS;
			break;
		}
		if (!anOutcome.Rate(&aScore))
		{
			mNext++;
			continue;
		}
		if (aSpent >= theBudget)
			break;
		mSolver.SetBoard(anOutcome); // the same board as the last check's keeps its search (or answer)
		mSearching = true;
	}
	return mVerdict;
}
