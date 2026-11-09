// Board.cpp, part 1: construction, the level generator, the pairing rule and the path search
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

// 0x40e2c0 / 0x40e300: the cell under a pixel, -1 outside the board
int Board::CellAtX(int thePixelX)
{
	if (thePixelX >= CellLeft(0))
	{
		int aCell = (thePixelX - CellLeft(0)) / 50;
		if (aCell < 9)
			return aCell;
	}
	return -1;
}

int Board::CellAtY(int thePixelY)
{
	if (thePixelY >= CellTop(0))
	{
		int aCell = (thePixelY - CellTop(0)) / 50;
		if (aCell < 9)
			return aCell;
	}
	return -1;
}

static inline int Rand9()
{
	return CrtRand() % 9;
}

// 0x401000
Board::Board(NoahApp* theApp, bool theContinue)
{
	mApp = theApp;
	mCtrlDown = false;
	mTidePercent = 100;
	mLoadedGame = theContinue;       // +0xd90 (the original passes the flag on to every vector's allocator byte too)
	mMouseX = mMouseY = 0;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
		{
			mGrid[x][y] = NULL;
			mClone[x][y] = NULL;
			mCost[x][y] = 0;
		}
	for (int x = 0; x < 9; x++)
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
	mFade = 0;
	mWaterY = 0x1a2;
	mWaterTargetY = 0x1a2;
	mWavePhaseB = 0;
	mWavePhaseA = 0xf;
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
	mComboShownAt = 2;
	mComboShown = 0;
	mPulseA = 0;
	mPulseADir = 1;
	mPulseB = 5;
	mPulseBDir = 1;
	mGravity = false;
	mNoMoves = false;
	mAbandonAlpha = 0;
	mAbandonIndex = -1;
	mAbandonCount = 0;
	mFlag14 = false;
	mArkLeaving = false;
	mPathStartX = mPathStartY = -1;
	mPathValid = false;
	mSelected = NULL;
	mState = BOARD_GET_READY;
	mState314 = mLastInputTick = mStartTick = mState320 = mState324 = mLastMoveTick = mState330 = 0;
	mStartUpdate = mApp->mUpdateCount;
	mLightning = false;
	mRowCount = 0;
	mSpawnRate = 0;
	mAnimTick = 0;
	mNoahWorried = false;
	mSetUp3ac = false;
	mPulseB = 5;
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
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
		{
			delete mGrid[x][y];
			mGrid[x][y] = NULL;
		}
	FreeClones();
	for (int x = 0; x < 9; x++)
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

	int aKinds = 0xd;                                   // animal kinds: 0..aKinds-1... (local_8)
	if (mFloodSpeed + 3 < 0xe)
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
		for (int x = 0; x < 9; x++)
			for (int y = 5; y < 9; y++)
				mGrid[x][y]->mType = CrtRand() % aKinds;
		for (int x = 0; x < 9; x++)
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

	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
		{
			mGrid[x][y]->mX = x;
			mGrid[x][y]->mY = y;
		}

	if ((mApp->mGameMode != GAMEMODE_STRATEGY) || (mApp->mLevel == 1))
	{
		int aWater = (0xd2 - mFloodSpeed) * 2;
		mWaterY = aWater;
		mApp->mFloodStart = aWater;                 // app+0x47c
		mWaterTargetY = mWaterY;
	}
	mCombo = 1;
	mComboShownAt = 2;
	mComboShown = 0;
	mComboPairs = 0;
	mBestCombo = 1;
	mSpawnRate = 3;
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
	mFlag14 = false;
	mArkLeaving = false;
	if ((mApp->mLevel > 1) && (mApp->mGameMode != GAMEMODE_STRATEGY))
		mWaterY = 0;
	mState = BOARD_GET_READY;
	mNoahWorried = false;
	mSetUp3ac = false;
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
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
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
	for (int x = 0; x < 9; x++)
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
		aPiece->mOffsetY = 0x32;
	}
	ClearLink();
}

// 0x401ce0 / 0x401d90: crates (0xf) and blocks (0x10) on random empty cells
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
		mGrid[x][y]->mType = PIECE_CRATE;
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
		mGrid[x][y]->mType = PIECE_BLOCK;
	}
}

// 0x401e40: true when the crates and blocks leave the board unsolvable: fill the empty cells with a filler
// kind (4) in the clones and try to join pairs of fillers; two fillers left over mean no
bool Board::CheckSolvable()
{
	RebuildClones();
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if (mClone[x][y]->mType == PIECE_EMPTY)
				mClone[x][y]->mType = 4;
	int aFailed = 0;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if ((mClone[x][y]->mType == 4) && (aFailed < 2))
				if (!TryRemoveClonePair(mClone[x][y]))
					aFailed++;
	return aFailed > 1;
}

// 0x401ee0
int Board::CountEmpty()
{
	int aCount = 0;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
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
		for (int x = 0; x < 9; x++)
			for (int y = 0; y < 9; y++)
				if ((mClone[x][y]->mType < 0xd) && TryRemoveClonePair(mClone[x][y]))
					aAnyPair = true;
		if (!aAnyPair)
			SwapRandom();
	}
}

// 0x401fc0 / 0x402000: the clone grid is rebuilt from the real one (0x4060d0 copies one piece)
void Board::RebuildClones()
{
	FreeClones();
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			mClone[x][y] = new Piece(*mGrid[x][y]);
}

void Board::FreeClones()
{
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
		{
			delete mClone[x][y];
			mClone[x][y] = NULL;
		}
}

// 0x4020e0: join the first reachable clone of the same kind with p (both become empty); with gravity the
// cells above fall one place
bool Board::TryRemoveClonePair(Piece* p)
{
	bool aDone = false;
	for (int qx = 0; qx < 9 && !aDone; qx++)
	{
		for (int qy = 0; qy < 9 && !aDone; qy++)
		{
			Piece* q = mClone[qx][qy];
			if ((q != p) && (q->mType == p->mType) && CloneReachable(q, p))
			{
				q->mType = PIECE_EMPTY;
				p->mType = PIECE_EMPTY;
				if (mGravity)
				{
					for (int yy = qy - 1; yy >= 0; yy--)
					{
						int aType = mClone[qx][yy]->mType;
						if ((aType < 0xd) || (aType == PIECE_BLOCK))
						{
							Piece* aBelow = mClone[qx][yy + 1];
							if (aBelow->mType == PIECE_EMPTY)
							{
								aBelow->mType = aType;
								aBelow->mDestX = mClone[qx][yy]->mDestX;
								aBelow->mDestY = mClone[qx][yy]->mDestY;
								mClone[qx][yy]->mType = PIECE_EMPTY;
							}
						}
					}
					int px = p->mX, py = p->mY;
					for (int yy = py - 1; yy >= 0; yy--)
					{
						int aType = mClone[px][yy]->mType;
						if ((aType < 0xd) || (aType == PIECE_BLOCK))
						{
							Piece* aBelow = mClone[px][yy + 1];
							if (aBelow->mType == PIECE_EMPTY)
							{
								aBelow->mType = aType;
								aBelow->mDestX = mClone[px][yy]->mDestX;
								aBelow->mDestY = mClone[px][yy]->mDestY;
								mClone[px][yy]->mType = PIECE_EMPTY;
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
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if (mClone[x][y]->mType < 0xd)
				return false;
	return true;
}

// 0x4025c0: swap the kinds of two random clones of different kinds (and write them back to the real grid)
void Board::SwapRandom()
{
	Piece* a = mClone[Rand9()][Rand9()];
	while ((a->mType > 0xc) && (a->mType != PIECE_BLOCK))
		a = mClone[Rand9()][Rand9()];
	for (;;)
	{
		Piece* b = mClone[Rand9()][Rand9()];
		if ((b->mType > 0xc) && (b->mType != PIECE_BLOCK))
			continue;
		if (b->mType != a->mType)
		{
			std::swap(a->mType, b->mType);
			mGrid[b->mDestX][b->mDestY]->mType = b->mType;
			mGrid[a->mDestX][a->mDestY]->mType = a->mType;
			return;
		}
	}
}

// 0x4026d0 / 0x402a70: can a reach b? A breadth-first search over the four neighbours through empty cells (and
// cells whose animal is leaving). The real grid also lets a piece walk through one that is moving; the clones do not.
bool Board::Reachable(Piece* a, Piece* b)
{
	bool aSeen[9][9];
	memset(aSeen, 0, sizeof(aSeen));
	std::queue<std::pair<int, int> > aOpen;
	aOpen.push(std::make_pair(a->mX, a->mY));
	aSeen[a->mX][a->mY] = true;
	static const int aDX[4] = {1, 0, -1, 0};
	static const int aDY[4] = {0, 1, 0, -1};
	while (!aOpen.empty())
	{
		std::pair<int, int> aCell = aOpen.front();
		aOpen.pop();
		for (int d = 0; d < 4; d++)
		{
			int nx = aCell.first + aDX[d];
			int ny = aCell.second + aDY[d];
			if ((nx < 0) || (nx > 8) || (ny < 0) || (ny > 8) || aSeen[nx][ny])
				continue;
			Piece* p = mGrid[nx][ny];
			if (p == b)
				return true;
			if ((p->mType == PIECE_EMPTY) || p->mRemoved || p->mFading || p->mMoving)
			{
				aSeen[nx][ny] = true;
				aOpen.push(std::make_pair(nx, ny));
			}
		}
	}
	return false;
}

bool Board::CloneReachable(Piece* a, Piece* b)
{
	bool aSeen[9][9];
	memset(aSeen, 0, sizeof(aSeen));
	std::queue<std::pair<int, int> > aOpen;
	aOpen.push(std::make_pair(a->mX, a->mY));
	aSeen[a->mX][a->mY] = true;
	static const int aDX[4] = {1, 0, -1, 0};
	static const int aDY[4] = {0, 1, 0, -1};
	while (!aOpen.empty())
	{
		std::pair<int, int> aCell = aOpen.front();
		aOpen.pop();
		for (int d = 0; d < 4; d++)
		{
			int nx = aCell.first + aDX[d];
			int ny = aCell.second + aDY[d];
			if ((nx < 0) || (nx > 8) || (ny < 0) || (ny > 8) || aSeen[nx][ny])
				continue;
			Piece* p = mClone[nx][ny];
			if (p == b)
				return true;
			if ((p->mType == PIECE_EMPTY) || p->mRemoved || p->mFading)
			{
				aSeen[nx][ny] = true;
				aOpen.push(std::make_pair(nx, ny));
			}
		}
	}
	return false;
}

// 0x403910: forget the path preview and the highlight flags
void Board::ClearLink()
{
	mPathStartX = -1;
	mPathStartY = -1;
	mPathValid = false;
	for (size_t i = 0; i < mTrail.size(); i++)
		delete mTrail[i];
	mTrail.clear();
	for (size_t i = 0; i < mPixelPath.size(); i++)
		delete mPixelPath[i];
	mPixelPath.clear();
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if (mGrid[x][y] != NULL)
				mGrid[x][y]->mSelected = false;
}

// 0x403a00 / 0x403a50: is row / column empty of pieces that count?
bool Board::RowEmpty(int theRow)
{
	for (int x = 0; x < 9; x++)
	{
		Piece* p = mGrid[x][theRow];
		if ((p != NULL) && (p->mType != PIECE_EMPTY) && !p->mRemoved && !p->mFading && !p->mMoving)
			return false;
	}
	return true;
}

bool Board::ColumnEmpty(int theColumn)
{
	for (int y = 0; y < 9; y++)
	{
		Piece* p = mGrid[theColumn][y];
		if ((p != NULL) && (p->mType != PIECE_EMPTY) && !p->mRemoved && !p->mFading && !p->mMoving)
			return false;
	}
	return true;
}

// 0x403370: the cheapest path of straight runs from (x1,y1) to (x2,y2); the corners go to mPathCells and the
// path in pixels to mPixelPath. Without theWantPath it only tells whether one exists.
bool Board::FindPath(int x1, int y1, int x2, int y2, int theMaxSegments, bool theWantPath)
{
	ClearLink();
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			mCost[x][y] = 999999;
	mBestCost = 100000;
	mPathSegments = -1;
	mSteps = 0;
	int aCorners[100];
	if (!SearchPath(x1, y1, x2, y2, 0, theMaxSegments, 0, theWantPath ? (int (*)[2]) aCorners : NULL))
		return false;
	if (!theWantPath)
		return true;
	if (mPathSegments == -1)
		return false;
	mPathLength = 0;
	int aPrevX = CellLeft(x1);
	int aPrevY = CellTop(y1);
	for (int i = 0; i <= mPathSegments; i++)
	{
		int aCellX = (i == mPathSegments) ? x2 : mPathCells[i][0];
		int aCellY = (i == mPathSegments) ? y2 : mPathCells[i][1];
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
bool Board::SearchPath(int x, int y, int x2, int y2, int theDepth, int theMaxDepth, int theCost, int (*thePath)[2])
{
	static const int aDirs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
	bool aFound = false;
	for (int d = 0; d < 4; d++)
	{
		int dx = aDirs[d][0];
		int dy = aDirs[d][1];
		int nx = x + dx;
		int ny = y;
		int aSteps = 2;
		int aCost = theCost + 2;
		while (nx >= 0)
		{
			ny += dy;
			if ((ny < 0) || (nx > 8) || (ny > 8))
				break;
			mSteps++;
			if ((nx == x2) && (ny == y2))
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
				Piece* p = mGrid[nx][ny];
				int aType = p->mType;
				if (((aType < 0xd) || (aType == PIECE_CRATE) || (aType == PIECE_BLOCK)) && !p->mRemoved && !p->mFading)
					break;
			}
			if (theDepth < theMaxDepth)
			{
				if (thePath != NULL)
				{
					thePath[theDepth][0] = nx;
					thePath[theDepth][1] = ny;
				}
				if (aCost < mCost[nx][ny])
				{
					mCost[nx][ny] = aCost;
					if (SearchPath(nx, ny, x2, y2, theDepth + 1, theMaxDepth, aCost, thePath))
					{
						if (thePath == NULL)
							return true;
						aFound = true;
					}
				}
			}
			nx += dx;
			aSteps++;
			aCost++;
		}
	}
	return aFound;
}
