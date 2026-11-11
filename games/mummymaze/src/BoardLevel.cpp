// The board's level set-up, undo and solver: WinMM.exe 0x404770-0x405580, 0x40e0e0, 0x40e1d0, 0x411640-0x411a90.
// See Board.h and docs/GAME_NOTES.md.

#include "Button2003.h"
#include "Board.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Screens.h"
#include "MummyApp.h"
#include "Compat2004.h"
#include "Dialogs.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "platform/VFS.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

#include "GameLevels.h"
#include "Tables.inc"

bool ChaseState::operator<(const ChaseState& theOther) const
{
	if (mExplorerX != theOther.mExplorerX) return mExplorerX < theOther.mExplorerX;
	if (mExplorerY != theOther.mExplorerY) return mExplorerY < theOther.mExplorerY;
	for (int i = 0; i < 2; i++)
	{
		if (mMummyX[i] != theOther.mMummyX[i]) return mMummyX[i] < theOther.mMummyX[i];
		if (mMummyY[i] != theOther.mMummyY[i]) return mMummyY[i] < theOther.mMummyY[i];
		if (mMummyAlive[i] != theOther.mMummyAlive[i]) return mMummyAlive[i] < theOther.mMummyAlive[i];
	}
	if (mScorpionX != theOther.mScorpionX) return mScorpionX < theOther.mScorpionX;
	if (mScorpionY != theOther.mScorpionY) return mScorpionY < theOther.mScorpionY;
	if (mScorpionAlive != theOther.mScorpionAlive) return mScorpionAlive < theOther.mScorpionAlive;
	return mGateClosed < theOther.mGateClosed;
}

bool ChaseState::operator==(const ChaseState& o) const
{
	return !(*this < o) && !(o < *this);
}

// The state the solver starts from: where everything is now (0x4050b0: the record built from the live objects)
ChaseState Board::GetChaseState()
{
	ChaseState aState;
	aState.mExplorerX = mExplorer->mX;
	aState.mExplorerY = mExplorer->mY;
	for (int i = 0; i < 2; i++)
	{
		if (mMummies[i] == NULL)
		{
			aState.mMummyX[i] = 0;
			aState.mMummyY[i] = 0;
			aState.mMummyAlive[i] = false;
		}
		else
		{
			aState.mMummyX[i] = mMummies[i]->mX;
			aState.mMummyY[i] = mMummies[i]->mY;
			aState.mMummyAlive[i] = mMummies[i]->mAlive;
		}
	}
	aState.mScorpionX = mScorpion->mX;
	aState.mScorpionY = mScorpion->mY;
	aState.mScorpionAlive = mScorpion->mAlive;
	aState.mGateClosed = mGateClosed;
	return aState;
}

// 0x403e80
Board::Board(MMApp* theApp) :
	mApp(theApp),
	mSize(6),
	mCellSize(60),
	mState(STATE_ENTER)
{
	mExplorer = NULL;
	mMummies[0] = mMummies[1] = NULL;
	mScorpion = NULL;
	mDarkImage = NULL;
	mDarkImageValid = false;
	ClearArrows();
	mStartTick = 0;
	mSolveTick = 0;
	mUnsolvableTick = 0;
	mUnsolvable = false;
	for (int i = 0; i < 2; i++)
		mTrapX[i] = mTrapY[i] = 99;
	mTrapDeathTimer = mStungTimer = -1;
	mGateX = mGateY = mKeyX = mKeyY = 99;
	mMaze = 1;
	mFile = 0;
	mGateClosed = true;
	mGateAnim = 0;
	mStarted = false;
	mShowingSolution = false;
	mTrapAnim[0] = mTrapAnim[1] = -1;
	memset(mCells, 0, sizeof(mCells));
	mRed = false;
	mTimeBase = 9999;
	mClickX = mClickY = 99;
	mMoveTick = 0;
	mTorchFrame = 0;
	mKeyFrame = 0;
	mFade = 0.0;
	mRiseFade = 0.0;
	mDark = false;
	mTutorialStep = 0;
	mExitStyle = 0;
	mSolvable = true;
	mEndAlpha = 0;
	mEndFade = 0.0;
	mTipShown = false;
	mIntroPlaying = false;

	// the dust: four pairs of random positions inside the cell
	for (int i = 0; i < 4; i++)
	{
		mDust[0][i] = CrtRand() % (mCellSize - 0xb);
		mDust[1][i] = CrtRand() % (mCellSize - 0xb);
	}

	mTutorialStepButton = new DialogButton2003(mApp->mImages[IMAGE_TUTBUTTON], 0, this);
	mTutorialStepButton->SetFont(mApp->mFont1);
	mTutorialStepButton->SetColor(0, Color(0, 0, 0));
	mTutorialStepButton->mLabel = GT(0x00491ddc);
	mTutorialStepButton->SetVisible(false);
	mPlayNowButton = new DialogButton2003(mApp->mImages[IMAGE_TUTBUTTON], 2, this);
	mPlayNowButton->SetFont(mApp->mFont1);
	mPlayNowButton->SetColor(0, Color(0, 0, 0));
	mPlayNowButton->mLabel = GT(0x00491dd0);
	mPlayNowButton->SetVisible(false);
	mContinueLink = new HyperlinkWidget(3, this);
	int aMode = mApp->mGameMode;
	if (aMode == MODE_ADVENTURE || aMode == MODE_ADVENTURE_DONE)
		mContinueLink->mLabel = (mApp->mAdvPyramid == 0xe) ? GT(0x00491db4) : GT(0x00491d60);
	else if (aMode == MODE_CLASSIC_DONE || aMode == MODE_CLASSIC)
		mContinueLink->mLabel = (mApp->mClassicPyramid == 0xe) ? GT(0x00491db4) : GT(0x00491d88);
	mContinueLink->SetFont(mApp->mHeaderFont);
	mContinueLink->mColor = Color(0xf8, 0x7a, 2);
	mContinueLink->mOverColor = Color(0xff, 0xff, 0xff);
	mContinueLink->SetVisible(false);

	NewGame();
	ClearArrows();
	mTimeBase = 9999;
	mClickX = mClickY = -1;
	mTorchFrame = 0;
	mKeyFrame = 0;
	mEndFade = 0.0;
	// the high half is drawn first (C++ does not order the two calls of one expression)
	int aHigh = CrtRand();
	int aLow = CrtRand();
	SeedMTRand2004(mRand, (aHigh << 16) | (aLow & 0xffff));
}

// 0x404570
Board::~Board()
{
	delete mExplorer;
	delete mMummies[0];
	delete mMummies[1];
	delete mScorpion;
	delete mDarkImage;
	delete mTutorialStepButton;
	delete mPlayNowButton;
	delete mContinueLink;
}

// 0x40e0e0: the key was stepped on: the gate opens or closes
void Board::FlipGate()
{
	mGateClosed = !mGateClosed;
	mApp->PlaySample(SOUND_GATE, GetPanAt(mGateX));
	if (mGateAnim > 0)
	{
		mGateAnim--;
		return;
	}
	if (mGateAnim == 0)
		mGateAnim = 1;
}

// 0x404770: begin a level: load the maze (or set up the tutorial / ending board) and reset the state
void Board::NewGame()
{
	mTimeBase = 9999;
	mRed = false;                       // 0x4b0
	mDark = false;                      // 0x4e0
	mSolvable = true;                   // 0x4ec
	DoResetIdles();
	ClearMoves();
	switch (mApp->mGameMode)
	{
	case MODE_CLASSIC:
		if (mApp->mClassicPyramid == 0xe)
			mDark = true;
		LoadMap();
		break;
	case MODE_ADVENTURE:
		if (mApp->mAdvPyramid == 7)
			mDark = true;
		LoadMap();
		break;
	case MODE_ADVENTURE_DONE:
	case MODE_CLASSIC_DONE:
		mApp->mRestoring = false;
		LoadTreasureMap();
		break;
	case 4:
		LoadTutorialMap(0);
		mApp->DoMusic(0xe, true);
		break;
	default:
		LoadTutorialMap(mApp->mGameMode - 4);
		break;
	}
	mStarted = true;
	if (mApp->mSidePanel != NULL)
		mApp->mSidePanel->MarkDirty();
	mStartTick = mApp->GameTick();
	int aMode = mApp->mGameMode;
	if (aMode == MODE_ADVENTURE_DONE || aMode == MODE_CLASSIC_DONE)
	{
		mFade = 1.0;
		mRiseFade = 1.0;
	}
	else
	{
		mFade = 0.0;
		mRiseFade = 0.0;
	}
	delete mDarkImage;
	mTrapDeathTimer = -1;
	mStungTimer = -1;
	mDarkImage = NULL;
	mDarkImageValid = false;
	mState = STATE_ENTER;
	mClickX = mClickY = 99;
	mTipShown = false;
	if (mApp->mGameMode > 3)
		mTipShown = true;
	if (((mMummies[0] != NULL) && (mMummies[1] != NULL)) && !mTipShown && mMummies[0]->mAlive && mMummies[1]->mAlive)
		mApp->DoWarning(0);
	if (!mTipShown)
	{
		if ((mGateX >= 0) && (mGateX < mSize) && (mGateY >= 0) && (mGateY < mSize))
			mApp->DoWarning(1);
		if (!mTipShown)
		{
			if (((mTrapX[0] >= 0) && (mTrapX[0] < mSize) && (mTrapY[0] >= 0) && (mTrapY[0] < mSize)) ||
				((mTrapX[1] >= 0) && (mTrapX[1] < mSize) && (mTrapY[1] >= 0) && (mTrapY[1] < mSize)))
				mApp->DoWarning(2);
			if (!mTipShown)
			{
				if (mRed)
					mApp->DoWarning(3);
				if (!mTipShown)
				{
					if ((mScorpion != NULL) && mScorpion->mAlive)
						mApp->DoWarning(4);
					if (!mTipShown)
					{
						if (mDark)
							mApp->DoWarning(5);
						if (!mTipShown && (mSize > 6) && mApp->mShowAnkh)
							mApp->DoWarning(6);
					}
				}
			}
		}
	}
}

// 0x404a30: the explorer is lost
void Board::DoGameOver()
{
	mApp->PlaySample(SOUND_MUMMYHOWL);
	mState = STATE_LOST;
	mApp->DoGameOver();
}

// 0x404a50: remember the chase before a move
void Board::DoNewTurn()
{
	if (!mUndo.empty())
	{
		const UndoState& u = mUndo.front();
		if (mExplorer->mX == u.mExplorerX && mExplorer->mY == u.mExplorerY && mMummies[0]->mX == u.mMummyX[0] &&
			mMummies[0]->mY == u.mMummyY[0] && mMummies[1]->mX == u.mMummyX[1] && mMummies[1]->mY == u.mMummyY[1] &&
			mScorpion->mX == u.mScorpionX && mScorpion->mY == u.mScorpionY)
			return;
	}
	UndoState anUndo;
	anUndo.mExplorerX = mExplorer->mX;
	anUndo.mExplorerY = mExplorer->mY;
	anUndo.mExplorerDir = mExplorer->mDir;
	for (int i = 0; i < 2; i++)
	{
		if (mMummies[i] == NULL)
		{
			anUndo.mMummyX[i] = 0;
			anUndo.mMummyY[i] = 0;
			anUndo.mMummyDir[i] = 0;
			anUndo.mMummyAlive[i] = false;
		}
		else
		{
			anUndo.mMummyX[i] = mMummies[i]->mX;
			anUndo.mMummyY[i] = mMummies[i]->mY;
			anUndo.mMummyDir[i] = mMummies[i]->mWantDir;
			anUndo.mMummyAlive[i] = mMummies[i]->mAlive;
		}
	}
	anUndo.mScorpionX = mScorpion->mX;
	anUndo.mScorpionY = mScorpion->mY;
	anUndo.mScorpionDir = mScorpion->mDir;
	anUndo.mScorpionAlive = mScorpion->mAlive;
	anUndo.mGateClosed = mGateClosed;
	mUndo.push_front(anUndo);
}

// 0x404be0: take back the last move
void Board::UndoMove()
{
	if (mUndo.empty() || mApp->mGameMode >= 2 || mState == STATE_DONE || mShowingSolution || mApp->mHighScores != NULL)
		return;
	const UndoState anUndo = mUndo.front();
	mExplorer->mX = anUndo.mExplorerX;
	mExplorer->mY = anUndo.mExplorerY;
	mExplorer->mDir = anUndo.mExplorerDir;
	mExplorer->mTargetX = mExplorer->mX;
	mExplorer->mTargetY = mExplorer->mY;
	for (int i = 0; i < 2; i++)
	{
		if (mMummies[i] == NULL)
			continue;
		mMummies[i]->mX = anUndo.mMummyX[i];
		mMummies[i]->mY = anUndo.mMummyY[i];
		mMummies[i]->mDir = anUndo.mMummyDir[i];
		mMummies[i]->mWantDir = anUndo.mMummyDir[i];
		mMummies[i]->mAlive = anUndo.mMummyAlive[i];
		mMummies[i]->mTargetX = mMummies[i]->mX;
		mMummies[i]->mTargetY = mMummies[i]->mY;
		mMummies[i]->mFighting = false;
	}
	mScorpion->mX = anUndo.mScorpionX;
	mScorpion->mY = anUndo.mScorpionY;
	mScorpion->mDir = anUndo.mScorpionDir;
	mScorpion->mAlive = anUndo.mScorpionAlive;
	mScorpion->mToX = mScorpion->mX;
	mScorpion->mToY = mScorpion->mY;
	mScorpion->mStinging = false;
	if (mGateClosed != anUndo.mGateClosed)
		FlipGate();
	mGateAnim = mGateClosed ? 0 : 7;
	mState = STATE_READY;
	if (!mSolvable)
		CheckSolvable(false);
	mClickX = mClickY = 99;
	mTrapDeathTimer = -1;
	mStungTimer = -1;
	mUndo.pop_front();
}

// 0x404dd0, 0x404e30, 0x404e90
void Board::ClearMoves()
{
	mUndo.clear();
}

void Board::ClearCheckedList()
{
	mVisited.clear();
}

void Board::ClearSolutionList()
{
	mSolution.clear();
}

// 0x404ef0: take every move back
void Board::ResetMaze()
{
	if (mApp->mGameMode < 2 && !mShowingSolution && mState != STATE_DONE && mApp->mHighScores == NULL &&
		mApp->mLevelComplete == NULL)
	{
		mSolvable = true;
		mClickX = mClickY = 99;
		while (!mUndo.empty())
			UndoMove();
	}
}

// ---------------------------------------------------------------------------------------------------------
// The maze files (0x40e1d0)

namespace
{
struct ByteReader
{
	std::vector<uint8_t>	mData;
	size_t					mPos;
	ByteReader() : mPos(0) {}
	int Next()
	{
		if (mPos >= mData.size())
			return 0;
		return mData[mPos++];
	}
};
}

// 0x40e1d0. Left out on purpose: the original also writes debug.txt on every load (a debug file that the player should not
// get).
void Board::LoadMap()
{
	mShowingSolution = false;
	if (!mApp->mRestoring)
		mApp->mElapsedBase = 0;

	// which file
	if (mApp->mFixedFile)
		mFile = mApp->mFixedFileNumber;
	else if (mApp->mRestoring)
		mFile = mApp->mRestoreFile;
	else if (mApp->mGameMode == MODE_ADVENTURE)
		mFile = kAdventureMaze[mApp->mAdvPyramid * 15 + mApp->mAdvChamber];
	else
	{
		double aSum = (double) mApp->mClassicPyramid * (1.0 / 15.0) * 99.0 + (double) mApp->mClassicChamber * (1.0 / 15.0) * 7.0;
		if (100.0 < aSum)
			aSum = 100.0;
		mFile = (int) aSum;
	}
	mRed = true;

	char aPath[256];
	sprintf(aPath, "data/mazes/B-%d.dat", mFile);
	ByteReader aReader;
	Platform::VFS::Get().ReadAll(aPath, aReader.mData);

	int aByte = aReader.Next();
	mSize = aByte & 0xf;
	mRed = (aByte & 0xf0) != 0;
	aReader.Next();
	int aMummies = aReader.Next() & 0xff;
	int aGates = aReader.Next() & 0xff;
	int aTraps = aReader.Next() & 0xff;
	int aScorpions = aReader.Next() & 0xff;
	mCellSize = 0x168 / mSize;

	// which maze of the file
	if (!mApp->mFixedFile)
	{
		if (!mApp->mRestoring)
		{
			if (mApp->mGameMode == MODE_ADVENTURE)
				mMaze = kAdventureLevel[mApp->mAdvPyramid * 15 + mApp->mAdvChamber] + 1;
			else
			{
				bool aExcluded;
				do
				{
					aExcluded = false;
					int aParity = (mApp->mClassicPyramid + mApp->mClassicChamber) % 2;
					int aNumber = abs(CrtRand() % 0x32) + 1 + aParity * 0x32;
					if (aNumber < 100)
					{
						aParity = (mApp->mClassicPyramid + mApp->mClassicChamber) % 2;
						aNumber = abs(CrtRand() % 0x32) + 1 + aParity * 0x32;
					}
					else
						aNumber = 99;
					mMaze = aNumber;
					for (int i = 0; i < 481; i++)
						if ((kExcluded[i * 2] == mFile) && (kExcluded[i * 2 + 1] == aNumber))
							aExcluded = true;
				} while (aExcluded);
			}
		}
		else
		{
			mMaze = mApp->mRestoreMaze;
			mApp->mRestoring = false;
		}
	}
	else
	{
		mMaze = mApp->mFixedMaze + 1;
		mApp->mFixedFile = false;
	}

	for (int aMazeIdx = 0; aMazeIdx < mMaze; aMazeIdx++)
	{
		for (int y = 0; y < mSize; y++)
		{
			for (int x = 0; x < mSize; x++)
			{
				mCells[x][y] = 0;
				if (x == 0)
					mCells[0][y] = 8;
				if (x == mSize - 1)
					mCells[x][y] |= 4;
				if (y == 0)
					mCells[x][0] |= 1;
				if (y == mSize - 1)
					mCells[x][y] |= 2;
			}
		}

		// ---- the positions below are in the file's frame; a mirrored maze (mRed) is read transposed
		delete mExplorer;
		mExplorer = NULL;
		int aSide, aPos;
		int aX, aY;
		if (!mRed)
		{
			// vertical walls: a row of bits per row of cells (west side of the cell)
			for (int y = 0; y < mSize; y++)
			{
				int aBits = aReader.Next() & 0xff;
				int x = 0;
				while (x < mSize)
				{
					if (x > 7)
						break;
					if (aBits & (1 << x))
					{
						mCells[x][y] |= 8;
						if (x > 0)
							mCells[x - 1][y] |= 4;
					}
					x++;
				}
				if (mSize > 8)
				{
					int aBits2 = aReader.Next() & 0xff;
					x = 8;
					while (x < mSize)
					{
						if (aBits2 & (1 << (x - 8)))
						{
							mCells[x][y] |= 8;
							if (x > 0)
								mCells[x - 1][y] |= 4;
						}
						x++;
					}
				}
			}
			// horizontal walls: north side of the cell
			for (int y = 0; y < mSize; y++)
			{
				int aBits = aReader.Next() & 0xff;
				int x = 0;
				while (x < mSize)
				{
					if (x > 7)
						break;
					if (aBits & (1 << x))
					{
						mCells[x][y] |= 1;
						if (y > 0)
							mCells[x][y - 1] |= 2;
					}
					x++;
				}
				if (mSize > 8)
				{
					int aBits2 = aReader.Next() & 0xff;
					x = 8;
					while (x < mSize)
					{
						if (aBits2 & (1 << (x - 8)))
						{
							mCells[x][y] |= 1;
							if (y > 0)
								mCells[x][y - 1] |= 2;
						}
						x++;
					}
				}
			}
			// the exit
			aByte = aReader.Next() & 0xff;
			aSide = aByte & 0xf;
			aPos = aByte >> 4;
			switch (aSide)
			{
			case 0:
				mCells[aPos][0] = (mCells[aPos][0] | EXIT_NORTH) ^ WALL_NORTH;
				break;
			case 1:
				mCells[0][aPos] = (mCells[0][aPos] | EXIT_WEST) ^ WALL_WEST;
				break;
			case 2:
				mCells[mSize - 1][aPos] = (mCells[mSize - 1][aPos] | EXIT_EAST) ^ WALL_EAST;
				break;
			case 3:
				mCells[aPos][mSize - 1] = (mCells[aPos][mSize - 1] | EXIT_SOUTH) ^ WALL_SOUTH;
				break;
			}
			// the explorer's entrance, the mummies, the scorpion, the traps
			aByte = aReader.Next() & 0xff;
			mExplorer = new Player(mApp, aByte & 0xf, aByte >> 4, mSize);
			aByte = aReader.Next() & 0xff;
			delete mMummies[0];
			mMummies[0] = new Mummy(mApp, aByte & 0xf, aByte >> 4, 0);
			delete mMummies[1];
			if (aMummies < 2)
			{
				mMummies[1] = new Mummy(mApp, 99, 99, 1);
				mMummies[1]->mAlive = false;
			}
			else
			{
				aByte = aReader.Next() & 0xff;
				mMummies[1] = new Mummy(mApp, aByte & 0xf, aByte >> 4, 1);
			}
			delete mScorpion;
			if (aScorpions < 1)
			{
				mScorpion = new Scorp(mApp, 99, 99);
				mScorpion->mAlive = false;
			}
			else
			{
				aByte = aReader.Next() & 0xff;
				mScorpion = new Scorp(mApp, aByte & 0xf, aByte >> 4);
			}
			mTrapX[0] = mTrapY[0] = mTrapX[1] = mTrapY[1] = 99;
			if (aTraps > 0)
			{
				aByte = aReader.Next() & 0xff;
				mTrapX[0] = aByte & 0xf;
				mTrapY[0] = aByte >> 4;
			}
			if (aTraps > 1)
			{
				aByte = aReader.Next() & 0xff;
				mTrapX[1] = aByte & 0xf;
				mTrapY[1] = aByte >> 4;
			}
			mTrapDeathTimer = -1;
			mGateClosed = true;
			mGateAnim = 0;
			mGateX = mGateY = mKeyX = mKeyY = 99;
			if (aGates > 0)
			{
				mGateClosed = true;
				mGateAnim = 0;
				aByte = aReader.Next() & 0xff;
				mGateX = aByte & 0xf;
				mGateY = aByte >> 4;
				aByte = aReader.Next() & 0xff;
				mKeyX = aByte & 0xf;
				mKeyY = aByte >> 4;
			}
		}
		else
		{
			// mirrored: file row r, bit i is the cell (size - 1 - r, i)
			for (int aRow = 0; aRow < mSize; aRow++)
			{
				int aBits = aReader.Next() & 0xff;
				int i = 0;
				while (i < mSize)
				{
					if (i > 7)
						break;
					if (aBits & (1 << i))
					{
						mCells[mSize - 1 - aRow][i] |= 1;
						if (i > 0)
							mCells[mSize - 1 - aRow][i - 1] |= 2;
					}
					i++;
				}
				if (mSize > 8)
				{
					int aBits2 = aReader.Next() & 0xff;
					i = 8;
					while (i < mSize)
					{
						if (aBits2 & (1 << (i - 8)))
						{
							mCells[mSize - 1 - aRow][i] |= 1;
							if (i > 0)
								mCells[mSize - 1 - aRow][i - 1] |= 2;
						}
						i++;
					}
				}
			}
			for (int aRow = 0; aRow < mSize; aRow++)
			{
				int aBits = aReader.Next() & 0xff;
				int i = 0;
				while (i < mSize)
				{
					if (i > 7)
						break;
					if (aBits & (1 << i))
					{
						mCells[mSize - 1 - aRow][i] |= 4;
						if (aRow > 0)
							mCells[mSize - aRow][i] |= 8;
					}
					i++;
				}
				if (mSize > 8)
				{
					int aBits2 = aReader.Next() & 0xff;
					i = 8;
					while (i < mSize)
					{
						if (aBits2 & (1 << (i - 8)))
						{
							mCells[mSize - 1 - aRow][i] |= 4;
							if (aRow > 0)
								mCells[mSize - aRow][i] |= 8;
						}
						i++;
					}
				}
			}
			aByte = aReader.Next() & 0xff;
			aSide = aByte & 0xf;
			aPos = aByte >> 4;
			switch (aSide)
			{
			case 0:
				mCells[mSize - 1][aPos] = (mCells[mSize - 1][aPos] | EXIT_EAST) ^ WALL_EAST;
				break;
			case 1:
				mCells[mSize - aPos - 1][0] = (mCells[mSize - aPos - 1][0] | EXIT_NORTH) ^ WALL_NORTH;
				break;
			case 2:
				mCells[mSize - 1 - aPos][mSize - 1] = (mCells[mSize - 1 - aPos][mSize - 1] | EXIT_SOUTH) ^ WALL_SOUTH;
				break;
			case 3:
				mCells[0][aPos] = (mCells[0][aPos] | EXIT_WEST) ^ WALL_WEST;
				break;
			}
			aByte = aReader.Next() & 0xff;
			aX = mSize - (aByte >> 4) - 1;
			aY = aByte & 0xf;
			mExplorer = new Player(mApp, aX, aY, mSize);
			mMummies[0] = NULL;
			mMummies[1] = NULL;
			aByte = aReader.Next() & 0xff;
			mMummies[0] = new Mummy(mApp, mSize - (aByte >> 4) - 1, aByte & 0xf, 0);
			if (aMummies < 2)
			{
				mMummies[1] = new Mummy(mApp, 99, 99, 1);
				mMummies[1]->mAlive = false;
			}
			else
			{
				aByte = aReader.Next() & 0xff;
				mMummies[1] = new Mummy(mApp, mSize - (aByte >> 4) - 1, aByte & 0xf, 1);
			}
			if (aScorpions < 1)
			{
				mScorpion = new Scorp(mApp, 99, 99);
				mScorpion->mAlive = false;
			}
			else
			{
				aByte = aReader.Next() & 0xff;
				mScorpion = new Scorp(mApp, mSize - (aByte >> 4) - 1, aByte & 0xf);
			}
			mTrapX[0] = mTrapY[0] = mTrapX[1] = mTrapY[1] = 99;
			if (aTraps > 0)
			{
				aByte = aReader.Next() & 0xff;
				mTrapX[0] = mSize - (aByte >> 4) - 1;
				mTrapY[0] = aByte & 0xf;
			}
			if (aTraps > 1)
			{
				aByte = aReader.Next() & 0xff;
				mTrapX[1] = mSize - (aByte >> 4) - 1;
				mTrapY[1] = aByte & 0xf;
			}
			mTrapDeathTimer = -1;
			mGateClosed = true;
			mGateAnim = 0;
			mGateX = mGateY = mKeyX = mKeyY = 99;
		}
	}

	int aChamber = (mApp->mGameMode == MODE_ADVENTURE) ? mApp->mAdvChamber : mApp->mClassicChamber;
	mApp->DoMusic(abs(aChamber % 16) + 0x17, true);
}

// ---------------------------------------------------------------------------------------------------------
// The hard-coded boards: the ending (0x40f2f0) and the tutorial's parts (0x40f520)

#include "TutorialMazes.inc"

static void ClearToBorders(Board* theBoard)
{
	theBoard->mShowingSolution = false;
	theBoard->mRed = false;
	theBoard->mSize = 6;
	theBoard->mCellSize = 0x3c;
	for (int y = 0; y < theBoard->mSize; y++)
	{
		for (int x = 0; x < theBoard->mSize; x++)
		{
			theBoard->mCells[x][y] = 0;
			if (x == 0)
				theBoard->mCells[0][y] = 8;
			if (x == theBoard->mSize - 1)
				theBoard->mCells[x][y] |= 4;
			if (y == 0)
				theBoard->mCells[x][0] |= 1;
			if (y == theBoard->mSize - 1)
				theBoard->mCells[x][y] |= 2;
		}
	}
}

void Board::LoadTreasureMap()
{
	ClearToBorders(this);
	for (int i = 0; i < 2; i++)
	{
		delete mMummies[i];
		mMummies[i] = NULL;
	}
	delete mExplorer;
	mExplorer = new Player(mApp, 3, 3, mSize);
	mMummies[0] = new Mummy(mApp, 99, 99, 1);
	mMummies[0]->mAlive = false;
	mMummies[1] = new Mummy(mApp, 99, 99, 1);
	mMummies[1]->mAlive = false;
	delete mScorpion;
	mScorpion = new Scorp(mApp, 99, 99);
	mScorpion->mAlive = false;
	mTrapX[0] = mTrapY[0] = mTrapX[1] = mTrapY[1] = 99;
	mGateX = mGateY = mKeyX = mKeyY = 99;
	mTrapDeathTimer = -1;
	mGateClosed = true;
	mGateAnim = 0;
}

void Board::LoadTutorialMap(int thePart)
{
	ClearToBorders(this);
	for (int i = 0; i < 2; i++)
	{
		delete mMummies[i];
		mMummies[i] = NULL;
	}
	if (thePart < 0 || thePart > 4)
		return;
	const std::vector<TutorialWall>& aWalls = TutorialWalls(thePart);
	if (thePart == 4)
		mRed = true;
	for (size_t i = 0; i < aWalls.size(); i++)
		mCells[aWalls[i].mCell / 10][aWalls[i].mCell % 10] |= aWalls[i].mBits;
	// the exit in the top left corner
	mCells[0][0] = (mCells[0][0] | EXIT_NORTH) ^ WALL_NORTH;

	// the pieces of each part: explorer, first mummy, second mummy, scorpion, traps
	static const int kExplorer[5][2] = {{3, 4}, {3, 3}, {4, 3}, {2, 3}, {2, 2}};
	static const int kMummy0[5][2] = {{0, 2}, {3, 1}, {1, 1}, {3, 2}, {4, 2}};
	delete mExplorer;
	mExplorer = new Player(mApp, kExplorer[thePart][0], kExplorer[thePart][1], mSize);
	mMummies[0] = new Mummy(mApp, kMummy0[thePart][0], kMummy0[thePart][1], 0);
	if (thePart == 3)
		mMummies[1] = new Mummy(mApp, 3, 0, 1);
	else
	{
		mMummies[1] = new Mummy(mApp, 99, 99, 1);
		mMummies[1]->mAlive = false;
	}
	delete mScorpion;
	if (thePart == 3)
		mScorpion = new Scorp(mApp, 5, 3);
	else
	{
		mScorpion = new Scorp(mApp, 99, 99);
		mScorpion->mAlive = false;
	}
	mTrapX[0] = mTrapY[0] = mTrapX[1] = mTrapY[1] = 99;
	if (thePart == 4)
	{
		mTrapX[0] = 5;
		mTrapY[0] = 0;
		mTrapX[1] = 4;
		mTrapY[1] = 0;
	}
	mTrapDeathTimer = -1;
	mGateClosed = true;
	mGateAnim = 0;
	mGateX = mGateY = mKeyX = mKeyY = 99;
	if (thePart == 3)
	{
		mGateX = 5;
		mGateY = 3;
		mKeyX = 3;
		mKeyY = 4;
	}
}
