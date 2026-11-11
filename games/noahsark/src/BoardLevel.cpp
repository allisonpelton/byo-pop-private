// The Board, part 1 of 6: construction, the level generator, the pairing rule and the path search
// (WinNoah.exe 0x401000-0x403a90, 0x406a20-0x406cd0, 0x40e280-0x40e300).

#include "Board.h"
#include "NoahApp.h"
#include "Compat2004.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <queue>

using namespace Sexy;

// 0x40e2c0: the column under a pixel, -1 outside the board
int Board::CellAtX(int thePixelX)
{
	if (thePixelX >= CellLeft(0))
	{
		int aCell = (thePixelX - CellLeft(0)) / CELL_SIZE;
		if (aCell < GRID_SIZE)
			return aCell;
	}
	return -1;
}

// 0x40e300: the row under a pixel
int Board::CellAtY(int thePixelY)
{
	if (thePixelY >= CellTop(0))
	{
		int aCell = (thePixelY - CellTop(0)) / CELL_SIZE;
		if (aCell < GRID_SIZE)
			return aCell;
	}
	return -1;
}

// port: a column or a row from the C library's generator, which the original writes out at each place
static inline int Rand9()
{
	return CrtRand() % Board::GRID_SIZE;
}

// 0x401000
Board::Board(NoahApp* theApp, bool theContinue)
{
	mApp = theApp;
	mCtrlDown = false;
	mTidePercent = 100;
	mLoadedGame = theContinue;       // +0xd90 (the original passes the flag on to every vector's allocator byte too)
	mMouseX = mMouseY = 0;
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
		{
			mGrid[x][y] = NULL;
			mClone[x][y] = NULL;
			mCost[x][y] = 0;
		}
	for (int x = 0; x < GRID_SIZE; x++)
		mNextRow[x] = NULL;

	mUpdateCnt = 0;                  // +0x40
	mPaused = false;
	mShowGetReady = true;
	mThunderCue = false;
	mStormTimer = 0;
	mDragging = false;
	mArkX = 0x11;
	mArkY = 0x62;
	mArkStage = 0;
	mFloodSpeed = 0;
	mNoahFrame = 0;
	mNoahSprite = IMAGE_NOAH_HAPPY_STILL1;
	mLostFade = 0;
	mWaterY = 0x1a2;
	mWaterTargetY = 0x1a2;
	mFrontWavePhase = 0;
	mBackWavePhase = 0xf;
	mArkLevelY = 200;
	mBackdropTick = 0;
	mBonusPoints = 0;
	mLastType = PIECE_EMPTY;
	mCombo = 0;
	mComboPairs = 0;
	mBestCombo = 2;
	mHintIndex = 0;
	mHintTimer = 0;
	mHintAlpha = 0;
	mShownCombo = 2;
	mComboTextTimer = 0;
	mDangerPulse = 0;
	mDangerPulseDir = 1;
	mFlashPulse = 5;
	mFlashPulseDir = 1;
	mGravity = false;
	mNoMoves = false;
	mAbandonAlpha = 0;
	mAbandonIndex = -1;
	mAbandonCount = 0;
	mArkLeaving = false;
	mSelected = NULL;
	mState = BOARD_GET_READY;
	mLastThunderUpdate = mLastInputTick = mStartTick = mUnsetAfterStartTick = mUnsetBeforeLastMoveTick = mLastMoveTick = mUnsetAfterShowGetReady = 0;
	mStartUpdate = mApp->mUpdateCount;
	mLightning = false;
	mRowCount = 0;
	mNewRowKinds = 0;
	mAnimTick = 0;
	mNoahWorried = false;
	mLeftBehindFading = false;
	mFlashPulse = 5;
	mPathSegments = -1;
	mBestCost = 100000;
	mSteps = 0;
	mPathLength = 0;
	memset(mPathCells, 0, sizeof(mPathCells));

	// the original seeds this generator with two draws of the C library's rand()
	int aHigh = CrtRand();
	int aLow = CrtRand();
	SeedMTRand2004(mRand, (unsigned long) ((aHigh << 16) | (aLow & 0xffff)));

	mHasTransparencies = true;
	NewLevel();

	int aMode = mApp->mGameMode;
	if (((aMode == GAMEMODE_ACTION) || (aMode == GAMEMODE_STRATEGY)) && mApp->mShowTutorial)
	{
		mPaused = true;
		mApp->ShowTutorial();
	}
}

// 0x401330: the destructor. The original frees the pieces (with their paths), the fish, the bubbles and the preview's
// two paths, and only releases the buffers of the texts' and the sparkles' lists, so the ones still on show are lost;
// it then runs its InitGrid, which makes 90 new pieces that nothing frees. The port frees everything it owns.
Board::~Board()
{
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
		{
			delete mGrid[x][y];
			mGrid[x][y] = NULL;
		}
	FreeClones();
	for (int x = 0; x < GRID_SIZE; x++)
	{
		delete mNextRow[x];
		mNextRow[x] = NULL;
	}
	ClearLink();
	for (size_t i = 0; i < mFish.size(); i++)
		delete mFish[i];
	for (size_t i = 0; i < mBubbles.size(); i++)
		delete mBubbles[i];
	for (size_t i = 0; i < mTexts.size(); i++)
		delete mTexts[i];
	for (size_t i = 0; i < mSparkles.size(); i++)
		delete mSparkles[i];
	for (size_t i = 0; i < mTrail.size(); i++)
		delete mTrail[i];
	for (size_t i = 0; i < mPixelPath.size(); i++)
		delete mPixelPath[i];
}

// 0x401580: the next level (the level counter is the app's). The fish and the bubbles of the level before are deleted
// below where the original calls its two clearing helpers, 0x402de0 (fish) and 0x402e50 (bubbles); the first also ends
// with a call of Widget's slot 9 with 0, a flag of the 2002 framework's dirty-rectangle drawing that the fish's creator
// sets again with 1, which the port does not have.
void Board::NewLevel()
{
	mApp->mLevel++;
	mApp->ResyncEvent('n');
	mFloodSpeed = mApp->mDifficulty + mApp->mLevel;
	if (((mApp->mLevel & 3) == 0) && (mApp->mGameMode == GAMEMODE_ACTION))
	{
		// every fourth level of an action game is the Stampede bonus round
		mApp->mBonusRound = true;
		mApp->mSavedGameMode = mApp->mGameMode;
		mApp->mGameMode = GAMEMODE_STAMPEDE;
		if (!mApp->mPuzzleNoticeShown)
		{
			mApp->mPuzzleNoticeShown = true;
			mApp->ShowModeIntroDialog(mApp->mGameMode);
			mPaused = true;
		}
	}
	mGravity = false;
	ModsNewLevel();
	InitGrid();
	if (mLoadedGame)
	{
		LoadGame();
		return;
	}

	int aKinds = NUM_ANIMALS;                           // the level's animals are of kinds 0 .. aKinds - 1
	if (mFloodSpeed + 3 < NUM_ANIMALS + 1)
		aKinds = mFloodSpeed + 3;
	int aMode = mApp->mGameMode;
	if (aMode == GAMEMODE_PUZZLE)
		aKinds = (mApp->mDifficulty / 4) + 7;
	if (aMode == GAMEMODE_STAMPEDE)
		aKinds = 3;

	int aBlocks = (mFloodSpeed + aMode) * 2 - 7;
	if (aBlocks >= 0x14)
		aBlocks = 0x13;
	else if (aBlocks < 1)
		aBlocks = 1;
	if ((aMode == GAMEMODE_STRATEGY) && (aBlocks > 0xb))
		aBlocks = 0xb;
	int aCrates = mFloodSpeed * 2 - 10;
	if (aCrates < 0)
		aCrates = 0;
	else if (aCrates > 0x13)
		aCrates = 0x14;
	if (aMode < 3)
		aCrates = 0;
	if ((aMode == GAMEMODE_PUZZLE) || (aMode == GAMEMODE_STAMPEDE))
	{
		aCrates = 0;
		aBlocks = 0;
	}

	do
	{
		InitGrid();
		PlaceBlocks(aBlocks);
		PlaceCrates(aCrates);
	}
	while (CheckSolvable());

	if (aMode == GAMEMODE_STAMPEDE)
	{
		// four rows of animals of three kinds (the bottom ones, y 5 to 8), and the row waiting below
		for (int x = 0; x < GRID_SIZE; x++)
			for (int y = 5; y < GRID_SIZE; y++)
				mGrid[x][y]->mType = CrtRand() % aKinds;
		for (int x = 0; x < GRID_SIZE; x++)
			mNextRow[x]->mType = CrtRand() % aKinds;
	}
	else
	{
		// animals come in pairs of one kind, dropped on random empty cells
		int aEmpty = CountEmpty();
		while (aEmpty > 0)
		{
			int aType = CrtRand() % aKinds;
			for (int aPair = 0; aPair < 2; aPair++)
			{
				if (CountEmpty() > 0)
				{
					int x = Rand9();
					int y = Rand9();
					while (mGrid[x][y]->mType != PIECE_EMPTY)
					{
						x = Rand9();
						y = Rand9();
					}
					mGrid[x][y]->mType = aType;
					if (aMode == GAMEMODE_PUZZLE)
						MarkRuns(false);
				}
			}
			aEmpty = CountEmpty();
		}
	}

	mGravity = aMode > 1;
	if ((aMode != GAMEMODE_PUZZLE) && (aMode != GAMEMODE_STAMPEDE))
		EnsureMoves();

	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
		{
			mGrid[x][y]->mX = x;
			mGrid[x][y]->mY = y;
		}

	if ((mApp->mGameMode != GAMEMODE_STRATEGY) || (mApp->mLevel == 1))
	{
		int aWater = (0xd2 - mFloodSpeed) * 2;
		mWaterY = aWater;
		mWaterTargetY = mWaterY;
	}
	mCombo = 1;
	mShownCombo = 2;
	mComboTextTimer = 0;
	mComboPairs = 0;
	mBestCombo = 1;
	mNewRowKinds = 3;
	mRowCount = 1;
	mLastType = PIECE_EMPTY;
	mAbandonAlpha = 0;
	mAbandonCount = 0;
	mBonusPoints = 0;
	mLightning = false;
	mThunderCue = false;
	mStormTimer = 0;
	for (size_t i = 0; i < mFish.size(); i++)
		delete mFish[i];
	mFish.clear();
	for (size_t i = 0; i < mBubbles.size(); i++)
		delete mBubbles[i];
	mBubbles.clear();
	mArkX = 3;
	mLastMoveTick = mUpdateCnt;
	mArkStage = 0;
	mNoahFrame = 0;
	mNoahSprite = IMAGE_NOAH_HAPPY_STILL1;
	mArkLeaving = false;
	if ((mApp->mLevel > 1) && (mApp->mGameMode != GAMEMODE_STRATEGY))
		mWaterY = 0;
	mState = BOARD_GET_READY;
	mNoahWorried = false;
	mLeftBehindFading = false;
	mNoMoves = false;
	mStartTick = GameTickCount();
	mStartUpdate = mApp->mUpdateCount;
	if ((mApp->mGameMode == GAMEMODE_STRATEGY) && (mWaterY < 0x109))
	{
		mNoahWorried = true;
		mNoahFrame = 0x32;
	}
	if (mApp->mGameMode == GAMEMODE_PUZZLE)
	{
		mApp->ShowModeIntroDialog(mApp->mGameMode);
		mPaused = true;
		ShowHint();
	}
	mApp->PlaySample(SOUND_GETREADY);
}

// 0x401a50: nine columns of nine empty cells, and the row waiting to rise
void Board::InitGrid()
{
	ClearLinkList();
	for (int x = 0; x < GRID_SIZE; x++)
	{
		for (int y = 0; y < GRID_SIZE; y++)
		{
			delete mGrid[x][y];
			Piece* aPiece = new Piece();
			mGrid[x][y] = aPiece;
			aPiece->mX = x;
			aPiece->mY = y;
			aPiece->mDestX = x;
			aPiece->mDestY = y;
			aPiece->mPixelX = (float) CellLeft(x);
			aPiece->mPixelY = (float) CellTop(y);
			aPiece->mType = PIECE_EMPTY;
			aPiece->mPartner = NULL;
		}
	}
	for (int x = 0; x < GRID_SIZE; x++)
	{
		delete mNextRow[x];
		Piece* aPiece = new Piece();
		mNextRow[x] = aPiece;
		aPiece->mX = x;
		aPiece->mY = 0;
		aPiece->mDestX = x;
		aPiece->mDestY = 0;
		aPiece->mPixelX = (float) CellLeft(x);
		aPiece->mPixelY = (float) CellTop(0);
		aPiece->mType = PIECE_EMPTY;
		aPiece->mOffsetY = CELL_SIZE;
	}
	ClearLink();
}

// 0x401ce0 / 0x401d90: blocks and crates on random empty cells
void Board::PlaceBlocks(int theCount)
{
	while (theCount > 0)
	{
		int x = Rand9();
		int y = Rand9();
		while (mGrid[x][y]->mType != PIECE_EMPTY)
		{
			x = Rand9();
			y = Rand9();
		}
		theCount--;
		mGrid[x][y]->mType = PIECE_BLOCK;
	}
}

void Board::PlaceCrates(int theCount)
{
	while (theCount > 0)
	{
		int x = Rand9();
		int y = Rand9();
		while (mGrid[x][y]->mType != PIECE_EMPTY)
		{
			x = Rand9();
			y = Rand9();
		}
		theCount--;
		mGrid[x][y]->mType = PIECE_CRATE;
	}
}

// 0x401e40: true when the crates and blocks leave the board unsolvable: fill the empty cells with a filler
// kind (4) in the clones and try to join pairs of fillers; two fillers left over mean no
bool Board::CheckSolvable()
{
	RebuildClones();
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
			if (mClone[x][y]->mType == PIECE_EMPTY)
				mClone[x][y]->mType = 4;
	int aFailed = 0;
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
			if ((mClone[x][y]->mType == 4) && (aFailed < 2))
				if (!TryRemoveClonePair(mClone[x][y]))
					aFailed++;
	return aFailed > 1;
}

// 0x401ee0
int Board::CountEmpty()
{
	int aCount = 0;
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
			if (mGrid[x][y]->mType == PIECE_EMPTY)
				aCount++;
	return aCount;
}

// 0x401f10: make sure the board has a move (the hint and shuffle machinery). Its only caller (NewLevel) leaves out the puzzle mode and
// the Stampede, so the original's puzzle branch (clear the runs and let the animals fall again, 0x4061d0) never runs and is not
// translated.
void Board::EnsureMoves()
{
	RebuildClones();
	while (AllClonesGone() == false)
	{
		bool aAnyPair = false;
		for (int x = 0; x < GRID_SIZE; x++)
			for (int y = 0; y < GRID_SIZE; y++)
				if ((mClone[x][y]->mType < NUM_ANIMALS) && TryRemoveClonePair(mClone[x][y]))
					aAnyPair = true;
		if (!aAnyPair)
			SwapRandom();
	}
}

// 0x401fc0 / 0x402000: the clone grid is rebuilt from the real one (0x4060d0 copies one piece)
void Board::RebuildClones()
{
	FreeClones();
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
			mClone[x][y] = new Piece(*mGrid[x][y]);
}

void Board::FreeClones()
{
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
		{
			delete mClone[x][y];
			mClone[x][y] = NULL;
		}
}

// 0x4020e0: join the piece with the first reachable clone of its kind (both become empty); with gravity the
// cells above fall one place
bool Board::TryRemoveClonePair(Piece* thePiece)
{
	bool aDone = false;
	for (int aOtherX = 0; aOtherX < GRID_SIZE && !aDone; aOtherX++)
	{
		for (int aOtherY = 0; aOtherY < GRID_SIZE && !aDone; aOtherY++)
		{
			Piece* aOther = mClone[aOtherX][aOtherY];
			if ((aOther != thePiece) && (aOther->mType == thePiece->mType) && CloneReachable(aOther, thePiece))
			{
				aOther->mType = PIECE_EMPTY;
				thePiece->mType = PIECE_EMPTY;
				if (mGravity)
				{
					for (int aAboveY = aOtherY - 1; aAboveY >= 0; aAboveY--)
					{
						int aType = mClone[aOtherX][aAboveY]->mType;
						if ((aType < NUM_ANIMALS) || (aType == PIECE_CRATE))
						{
							Piece* aBelow = mClone[aOtherX][aAboveY + 1];
							if (aBelow->mType == PIECE_EMPTY)
							{
								aBelow->mType = aType;
								aBelow->mDestX = mClone[aOtherX][aAboveY]->mDestX;
								aBelow->mDestY = mClone[aOtherX][aAboveY]->mDestY;
								mClone[aOtherX][aAboveY]->mType = PIECE_EMPTY;
							}
						}
					}
					int aPieceX = thePiece->mX, aPieceY = thePiece->mY;
					for (int aAboveY = aPieceY - 1; aAboveY >= 0; aAboveY--)
					{
						int aType = mClone[aPieceX][aAboveY]->mType;
						if ((aType < NUM_ANIMALS) || (aType == PIECE_CRATE))
						{
							Piece* aBelow = mClone[aPieceX][aAboveY + 1];
							if (aBelow->mType == PIECE_EMPTY)
							{
								aBelow->mType = aType;
								aBelow->mDestX = mClone[aPieceX][aAboveY]->mDestX;
								aBelow->mDestY = mClone[aPieceX][aAboveY]->mDestY;
								mClone[aPieceX][aAboveY]->mType = PIECE_EMPTY;
							}
						}
					}
				}
				aDone = true;
			}
		}
	}
	return aDone;
}

// 0x402590: no animal left in the clones
bool Board::AllClonesGone()
{
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
			if (mClone[x][y]->mType < NUM_ANIMALS)
				return false;
	return true;
}

// 0x4025c0: swap the kinds of two random clones of different kinds (and write them back to the real grid)
void Board::SwapRandom()
{
	Piece* aFirst = mClone[Rand9()][Rand9()];
	while ((aFirst->mType > PIECE_ANIMAL_LAST) && (aFirst->mType != PIECE_CRATE))
		aFirst = mClone[Rand9()][Rand9()];
	for (;;)
	{
		Piece* aSecond = mClone[Rand9()][Rand9()];
		if ((aSecond->mType > PIECE_ANIMAL_LAST) && (aSecond->mType != PIECE_CRATE))
			continue;
		if (aSecond->mType != aFirst->mType)
		{
			std::swap(aFirst->mType, aSecond->mType);
			mGrid[aSecond->mDestX][aSecond->mDestY]->mType = aSecond->mType;
			mGrid[aFirst->mDestX][aFirst->mDestY]->mType = aFirst->mType;
			return;
		}
	}
}

// 0x4026d0 / 0x402a70: can one piece reach the other? A breadth-first search over the four neighbours through empty cells (and
// cells whose animal is leaving). The real grid also lets a piece walk through one that is moving; the clones do not.
bool Board::Reachable(Piece* theFrom, Piece* theTo)
{
	bool aSeen[GRID_SIZE][GRID_SIZE];
	memset(aSeen, 0, sizeof(aSeen));
	std::queue<std::pair<int, int> > aOpen;
	aOpen.push(std::make_pair(theFrom->mX, theFrom->mY));
	aSeen[theFrom->mX][theFrom->mY] = true;
	static const int aDX[4] = {1, 0, -1, 0};
	static const int aDY[4] = {0, 1, 0, -1};
	while (!aOpen.empty())
	{
		std::pair<int, int> aCell = aOpen.front();
		aOpen.pop();
		for (int aDir = 0; aDir < 4; aDir++)
		{
			int aNextX = aCell.first + aDX[aDir];
			int aNextY = aCell.second + aDY[aDir];
			if ((aNextX < 0) || (aNextX > GRID_LAST) || (aNextY < 0) || (aNextY > GRID_LAST) || aSeen[aNextX][aNextY])
				continue;
			Piece* aPiece = mGrid[aNextX][aNextY];
			if (aPiece == theTo)
				return true;
			if ((aPiece->mType == PIECE_EMPTY) || aPiece->mRemoved || aPiece->mFading || aPiece->mMoving)
			{
				aSeen[aNextX][aNextY] = true;
				aOpen.push(std::make_pair(aNextX, aNextY));
			}
		}
	}
	return false;
}

bool Board::CloneReachable(Piece* theFrom, Piece* theTo)
{
	bool aSeen[GRID_SIZE][GRID_SIZE];
	memset(aSeen, 0, sizeof(aSeen));
	std::queue<std::pair<int, int> > aOpen;
	aOpen.push(std::make_pair(theFrom->mX, theFrom->mY));
	aSeen[theFrom->mX][theFrom->mY] = true;
	static const int aDX[4] = {1, 0, -1, 0};
	static const int aDY[4] = {0, 1, 0, -1};
	while (!aOpen.empty())
	{
		std::pair<int, int> aCell = aOpen.front();
		aOpen.pop();
		for (int aDir = 0; aDir < 4; aDir++)
		{
			int aNextX = aCell.first + aDX[aDir];
			int aNextY = aCell.second + aDY[aDir];
			if ((aNextX < 0) || (aNextX > GRID_LAST) || (aNextY < 0) || (aNextY > GRID_LAST) || aSeen[aNextX][aNextY])
				continue;
			Piece* aPiece = mClone[aNextX][aNextY];
			if (aPiece == theTo)
				return true;
			if ((aPiece->mType == PIECE_EMPTY) || aPiece->mRemoved || aPiece->mFading)
			{
				aSeen[aNextX][aNextY] = true;
				aOpen.push(std::make_pair(aNextX, aNextY));
			}
		}
	}
	return false;
}

// 0x403910: forget the path preview and the highlight flags
void Board::ClearLink()
{
	for (size_t i = 0; i < mTrail.size(); i++)
		delete mTrail[i];
	mTrail.clear();
	for (size_t i = 0; i < mPixelPath.size(); i++)
		delete mPixelPath[i];
	mPixelPath.clear();
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
			if (mGrid[x][y] != NULL)
				mGrid[x][y]->mSelected = false;
}

// 0x403a00 / 0x403a50: is row / column empty of pieces that count?
bool Board::RowEmpty(int theRow)
{
	for (int x = 0; x < GRID_SIZE; x++)
	{
		Piece* aPiece = mGrid[x][theRow];
		if ((aPiece != NULL) && (aPiece->mType != PIECE_EMPTY) && !aPiece->mRemoved && !aPiece->mFading && !aPiece->mMoving)
			return false;
	}
	return true;
}

bool Board::ColumnEmpty(int theColumn)
{
	for (int y = 0; y < GRID_SIZE; y++)
	{
		Piece* aPiece = mGrid[theColumn][y];
		if ((aPiece != NULL) && (aPiece->mType != PIECE_EMPTY) && !aPiece->mRemoved && !aPiece->mFading && !aPiece->mMoving)
			return false;
	}
	return true;
}

// 0x403370: the cheapest path of straight runs from one cell to another; the corners go to mPathCells and the
// path in pixels to mPixelPath. Without theWantPath it only tells whether one exists.
bool Board::FindPath(int theFromX, int theFromY, int theToX, int theToY, int theMaxSegments, bool theWantPath)
{
	ClearLink();
	for (int x = 0; x < GRID_SIZE; x++)
		for (int y = 0; y < GRID_SIZE; y++)
			mCost[x][y] = 999999;
	mBestCost = 100000;
	mPathSegments = -1;
	mSteps = 0;
	int aCorners[100];
	if (!SearchPath(theFromX, theFromY, theToX, theToY, 0, theMaxSegments, 0, theWantPath ? (int (*)[2]) aCorners : NULL))
		return false;
	if (!theWantPath)
		return true;
	if (mPathSegments == -1)
		return false;
	mPathLength = 0;
	int aPrevX = CellLeft(theFromX);
	int aPrevY = CellTop(theFromY);
	for (int i = 0; i <= mPathSegments; i++)
	{
		int aCellX = (i == mPathSegments) ? theToX : mPathCells[i][0];
		int aCellY = (i == mPathSegments) ? theToY : mPathCells[i][1];
		int aX = CellLeft(aCellX);
		int aY = CellTop(aCellY);
		mPathLength += abs(aY - aPrevY) + abs(aX - aPrevX);
		PathNode* aNode = new PathNode();
		memset(aNode, 0, sizeof(PathNode));
		aNode->mPixelX = aX;
		aNode->mPixelY = aY;
		mPixelPath.push_back(aNode);
		aPrevX = aX;
		aPrevY = aY;
	}
	return true;
}

// 0x4036f0: depth-first search along straight runs in the four directions; a run may end on any cell and a
// new run starts there. A step costs 1 and starting a run 2; the cost to each cell is kept to prune. A piece
// blocks a run while the game is on (mState 1) unless it is leaving.
bool Board::SearchPath(int x, int y, int theToX, int theToY, int theDepth, int theMaxDepth, int theCost, int (*thePath)[2])
{
	static const int aDirs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
	bool aFound = false;
	for (int aDir = 0; aDir < 4; aDir++)
	{
		int aStepX = aDirs[aDir][0];
		int aStepY = aDirs[aDir][1];
		int aNextX = x + aStepX;
		int aNextY = y;
		int aSteps = 2;
		int aCost = theCost + 2;
		while (aNextX >= 0)
		{
			aNextY += aStepY;
			if ((aNextY < 0) || (aNextX > GRID_LAST) || (aNextY > GRID_LAST))
				break;
			mSteps++;
			if ((aNextX == theToX) && (aNextY == theToY))
			{
				if (thePath != NULL)
				{
					if (aSteps + theCost < mBestCost)
					{
						mPathSegments = theDepth;
						for (int i = 0; i < theMaxDepth; i++)
						{
							mPathCells[i][0] = thePath[i][0];
							mPathCells[i][1] = thePath[i][1];
						}
						mBestCost = aSteps + theCost;
					}
				}
				return true;
			}
			if (mState == BOARD_PLAYING)
			{
				Piece* aPiece = mGrid[aNextX][aNextY];
				int aType = aPiece->mType;
				if (((aType < NUM_ANIMALS) || (aType == PIECE_BLOCK) || (aType == PIECE_CRATE)) && !aPiece->mRemoved && !aPiece->mFading)
					break;
			}
			if (theDepth < theMaxDepth)
			{
				if (thePath != NULL)
				{
					thePath[theDepth][0] = aNextX;
					thePath[theDepth][1] = aNextY;
				}
				if (aCost < mCost[aNextX][aNextY])
				{
					mCost[aNextX][aNextY] = aCost;
					if (SearchPath(aNextX, aNextY, theToX, theToY, theDepth + 1, theMaxDepth, aCost, thePath))
					{
						if (thePath == NULL)
							return true;
						aFound = true;
					}
				}
			}
			aNextX += aStepX;
			aSteps++;
			aCost++;
		}
	}
	return aFound;
}
