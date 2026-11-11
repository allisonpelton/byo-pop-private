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

bool ChaseState::operator<(const ChaseState& o) const
{
	if (mExplorerX != o.mExplorerX) return mExplorerX < o.mExplorerX;
	if (mExplorerY != o.mExplorerY) return mExplorerY < o.mExplorerY;
	for (int i = 0; i < 2; i++)
	{
		if (mMummyX[i] != o.mMummyX[i]) return mMummyX[i] < o.mMummyX[i];
		if (mMummyY[i] != o.mMummyY[i]) return mMummyY[i] < o.mMummyY[i];
		if (mMummyAlive[i] != o.mMummyAlive[i]) return mMummyAlive[i] < o.mMummyAlive[i];
	}
	if (mScorpionX != o.mScorpionX) return mScorpionX < o.mScorpionX;
	if (mScorpionY != o.mScorpionY) return mScorpionY < o.mScorpionY;
	if (mScorpionAlive != o.mScorpionAlive) return mScorpionAlive < o.mScorpionAlive;
	return mGateClosed < o.mGateClosed;
}

bool ChaseState::operator==(const ChaseState& o) const
{
	return !(*this < o) && !(o < *this);
}

// The state the solver starts from: where everything is now (0x4050b0: the record built from the live objects)
ChaseState Board::GetChaseState()
{
	ChaseState s;
	s.mExplorerX = mExplorer->mX;
	s.mExplorerY = mExplorer->mY;
	for (int i = 0; i < 2; i++)
	{
		if (mMummies[i] == NULL)
		{
			s.mMummyX[i] = 0;
			s.mMummyY[i] = 0;
			s.mMummyAlive[i] = false;
		}
		else
		{
			s.mMummyX[i] = mMummies[i]->mX;
			s.mMummyY[i] = mMummies[i]->mY;
			s.mMummyAlive[i] = mMummies[i]->mAlive;
		}
	}
	s.mScorpionX = mScorpion->mX;
	s.mScorpionY = mScorpion->mY;
	s.mScorpionAlive = mScorpion->mAlive;
	s.mGateClosed = mGateClosed;
	return s;
}

// 0x403e80
Board::Board(MummyApp* theApp) :
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
	ClearHover();
	mStartTick = 0;
	m270 = 0;
	m274 = 0;
	mUnsolvable = false;
	for (int i = 0; i < 2; i++)
		mTrapX[i] = mTrapY[i] = 99;
	mDeathTimer = mDeathTimer2 = -1;
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
	mFade2 = 0.0;
	mDark = false;
	mTutorialStep = 0;
	mExitStyle = 0;
	mSolvable = true;
	mEndAlpha = 0;
	mEndFade = 0.0;
	mFlagEc0 = false;
	mFlagEc1 = false;

	// the dust: four pairs of random positions inside the cell
	for (int i = 0; i < 4; i++)
	{
		mDust[0][i] = CrtRand() % (mCellSize - 0xb);
		mDust[1][i] = CrtRand() % (mCellSize - 0xb);
	}

	mPlayNowButton0 = new DialogButton2003(mApp->mImages[IMAGE_TUTBUTTON], 0, this);
	mPlayNowButton0->SetFont(mApp->mFont1);
	mPlayNowButton0->SetColor(0, Color(0, 0, 0));
	mPlayNowButton0->mLabel = GT(0x00491ddc);
	mPlayNowButton0->SetVisible(false);
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

	StartLevel();
	ClearHover();
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
	delete mPlayNowButton0;
	delete mPlayNowButton;
	delete mContinueLink;
}

// 0x40e0e0: the key was stepped on: the gate opens or closes
void Board::ToggleGate()
{
	mGateClosed = !mGateClosed;
	mApp->PlaySample(SOUND_GATE, Pan(mGateX));
	if (mGateAnim > 0)
	{
		mGateAnim--;
		return;
	}
	if (mGateAnim == 0)
		mGateAnim = 1;
}

// 0x404770: begin a level: load the maze (or set up the tutorial / ending board) and reset the state
void Board::StartLevel()
{
	mTimeBase = 9999;
	mRed = false;                       // 0x4b0
	mDark = false;                      // 0x4e0
	mSolvable = true;                   // 0x4ec
	ResetIdle();
	ClearUndo();
	switch (mApp->mGameMode)
	{
	case MODE_CLASSIC:
		if (mApp->mClassicPyramid == 0xe)
			mDark = true;
		LoadMaze();
		break;
	case MODE_ADVENTURE:
		if (mApp->mAdvPyramid == 7)
			mDark = true;
		LoadMaze();
		break;
	case MODE_ADVENTURE_DONE:
	case MODE_CLASSIC_DONE:
		mApp->mRestoring = false;
		SetupEnding();
		break;
	case 4:
		SetupTutorial(0);
		mApp->SetMusicOrder(0xe, true);
		break;
	default:
		SetupTutorial(mApp->mGameMode - 4);
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
		mFade2 = 1.0;
	}
	else
	{
		mFade = 0.0;
		mFade2 = 0.0;
	}
	delete mDarkImage;
	mDeathTimer = -1;
	mDeathTimer2 = -1;
	mDarkImage = NULL;
	mDarkImageValid = false;
	mState = STATE_ENTER;
	mClickX = mClickY = 99;
	mFlagEc0 = false;
	if (mApp->mGameMode > 3)
		mFlagEc0 = true;
	if (((mMummies[0] != NULL) && (mMummies[1] != NULL)) && !mFlagEc0 && mMummies[0]->mAlive && mMummies[1]->mAlive)
		mApp->ShowTip(0);
	if (!mFlagEc0)
	{
		if ((mGateX >= 0) && (mGateX < mSize) && (mGateY >= 0) && (mGateY < mSize))
			mApp->ShowTip(1);
		if (!mFlagEc0)
		{
			if (((mTrapX[0] >= 0) && (mTrapX[0] < mSize) && (mTrapY[0] >= 0) && (mTrapY[0] < mSize)) ||
				((mTrapX[1] >= 0) && (mTrapX[1] < mSize) && (mTrapY[1] >= 0) && (mTrapY[1] < mSize)))
				mApp->ShowTip(2);
			if (!mFlagEc0)
			{
				if (mRed)
					mApp->ShowTip(3);
				if (!mFlagEc0)
				{
					if ((mScorpion != NULL) && mScorpion->mAlive)
						mApp->ShowTip(4);
					if (!mFlagEc0)
					{
						if (mDark)
							mApp->ShowTip(5);
						if (!mFlagEc0 && (mSize > 6) && mApp->mShowAnkh)
							mApp->ShowTip(6);
					}
				}
			}
		}
	}
}

// 0x404a30: the explorer is lost
void Board::Lose()
{
	mApp->PlaySample(SOUND_MUMMYHOWL);
	mState = STATE_LOST;
	mApp->ShowGamePanel();
}

// 0x404a50: remember the chase before a move
void Board::PushUndo()
{
	if (!mUndo.empty())
	{
		const UndoState& u = mUndo.front();
		if (mExplorer->mX == u.mExplorerX && mExplorer->mY == u.mExplorerY && mMummies[0]->mX == u.mMummyX[0] &&
			mMummies[0]->mY == u.mMummyY[0] && mMummies[1]->mX == u.mMummyX[1] && mMummies[1]->mY == u.mMummyY[1] &&
			mScorpion->mX == u.mScorpionX && mScorpion->mY == u.mScorpionY)
			return;
	}
	UndoState u;
	u.mExplorerX = mExplorer->mX;
	u.mExplorerY = mExplorer->mY;
	u.mExplorerDir = mExplorer->mDir;
	for (int i = 0; i < 2; i++)
	{
		if (mMummies[i] == NULL)
		{
			u.mMummyX[i] = 0;
			u.mMummyY[i] = 0;
			u.mMummyDir[i] = 0;
			u.mMummyAlive[i] = false;
		}
		else
		{
			u.mMummyX[i] = mMummies[i]->mX;
			u.mMummyY[i] = mMummies[i]->mY;
			u.mMummyDir[i] = mMummies[i]->mWantDir;
			u.mMummyAlive[i] = mMummies[i]->mAlive;
		}
	}
	u.mScorpionX = mScorpion->mX;
	u.mScorpionY = mScorpion->mY;
	u.mScorpionDir = mScorpion->mDir;
	u.mScorpionAlive = mScorpion->mAlive;
	u.mGateClosed = mGateClosed;
	mUndo.push_front(u);
}

// 0x404be0: take back the last move
void Board::Undo()
{
	if (mUndo.empty() || mApp->mGameMode >= 2 || mState == STATE_DONE || mShowingSolution || mApp->mHighScores != NULL)
		return;
	const UndoState u = mUndo.front();
	mExplorer->mX = u.mExplorerX;
	mExplorer->mY = u.mExplorerY;
	mExplorer->mDir = u.mExplorerDir;
	mExplorer->mTargetX = mExplorer->mX;
	mExplorer->mTargetY = mExplorer->mY;
	for (int i = 0; i < 2; i++)
	{
		if (mMummies[i] == NULL)
			continue;
		mMummies[i]->mX = u.mMummyX[i];
		mMummies[i]->mY = u.mMummyY[i];
		mMummies[i]->mDir = u.mMummyDir[i];
		mMummies[i]->mWantDir = u.mMummyDir[i];
		mMummies[i]->mAlive = u.mMummyAlive[i];
		mMummies[i]->mTargetX = mMummies[i]->mX;
		mMummies[i]->mTargetY = mMummies[i]->mY;
		mMummies[i]->mFighting = false;
	}
	mScorpion->mX = u.mScorpionX;
	mScorpion->mY = u.mScorpionY;
	mScorpion->mDir = u.mScorpionDir;
	mScorpion->mAlive = u.mScorpionAlive;
	mScorpion->mToX = mScorpion->mX;
	mScorpion->mToY = mScorpion->mY;
	mScorpion->mStinging = false;
	if (mGateClosed != u.mGateClosed)
		ToggleGate();
	mGateAnim = mGateClosed ? 0 : 7;
	mState = STATE_READY;
	if (!mSolvable)
		CheckSolvable(false);
	mClickX = mClickY = 99;
	mDeathTimer = -1;
	mDeathTimer2 = -1;
	mUndo.pop_front();
}

// 0x404dd0, 0x404e30, 0x404e90
void Board::ClearUndo()
{
	mUndo.clear();
}

void Board::ClearVisited()
{
	mVisited.clear();
}

void Board::ClearSolution()
{
	mSolution.clear();
}

// 0x404ef0: take every move back
void Board::Retry()
{
	if (mApp->mGameMode < 2 && !mShowingSolution && mState != STATE_DONE && mApp->mHighScores == NULL &&
		mApp->mLevelComplete == NULL)
	{
		mSolvable = true;
		mClickX = mClickY = 99;
		while (!mUndo.empty())
			Undo();
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
void Board::LoadMaze()
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
			mExplorer = new Explorer(mApp, aByte & 0xf, aByte >> 4, mSize);
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
				mScorpion = new Scorpion(mApp, 99, 99);
				mScorpion->mAlive = false;
			}
			else
			{
				aByte = aReader.Next() & 0xff;
				mScorpion = new Scorpion(mApp, aByte & 0xf, aByte >> 4);
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
			mDeathTimer = -1;
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
			for (int r = 0; r < mSize; r++)
			{
				int aBits = aReader.Next() & 0xff;
				int i = 0;
				while (i < mSize)
				{
					if (i > 7)
						break;
					if (aBits & (1 << i))
					{
						mCells[mSize - 1 - r][i] |= 1;
						if (i > 0)
							mCells[mSize - 1 - r][i - 1] |= 2;
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
							mCells[mSize - 1 - r][i] |= 1;
							if (i > 0)
								mCells[mSize - 1 - r][i - 1] |= 2;
						}
						i++;
					}
				}
			}
			for (int r = 0; r < mSize; r++)
			{
				int aBits = aReader.Next() & 0xff;
				int i = 0;
				while (i < mSize)
				{
					if (i > 7)
						break;
					if (aBits & (1 << i))
					{
						mCells[mSize - 1 - r][i] |= 4;
						if (r > 0)
							mCells[mSize - r][i] |= 8;
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
							mCells[mSize - 1 - r][i] |= 4;
							if (r > 0)
								mCells[mSize - r][i] |= 8;
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
			mExplorer = new Explorer(mApp, aX, aY, mSize);
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
				mScorpion = new Scorpion(mApp, 99, 99);
				mScorpion->mAlive = false;
			}
			else
			{
				aByte = aReader.Next() & 0xff;
				mScorpion = new Scorpion(mApp, mSize - (aByte >> 4) - 1, aByte & 0xf);
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
			mDeathTimer = -1;
			mGateClosed = true;
			mGateAnim = 0;
			mGateX = mGateY = mKeyX = mKeyY = 99;
		}
	}

	int aChamber = (mApp->mGameMode == MODE_ADVENTURE) ? mApp->mAdvChamber : mApp->mClassicChamber;
	mApp->SetMusicOrder(abs(aChamber % 16) + 0x17, true);
}

// ---------------------------------------------------------------------------------------------------------
// The hard-coded boards: the ending (0x40f2f0) and the tutorial's parts (0x40f520)

#include "TutorialMazes.inc"

static void ClearToBorders(Board* b)
{
	b->mShowingSolution = false;
	b->mRed = false;
	b->mSize = 6;
	b->mCellSize = 0x3c;
	for (int y = 0; y < b->mSize; y++)
	{
		for (int x = 0; x < b->mSize; x++)
		{
			b->mCells[x][y] = 0;
			if (x == 0)
				b->mCells[0][y] = 8;
			if (x == b->mSize - 1)
				b->mCells[x][y] |= 4;
			if (y == 0)
				b->mCells[x][0] |= 1;
			if (y == b->mSize - 1)
				b->mCells[x][y] |= 2;
		}
	}
}

void Board::SetupEnding()
{
	ClearToBorders(this);
	for (int i = 0; i < 2; i++)
	{
		delete mMummies[i];
		mMummies[i] = NULL;
	}
	delete mExplorer;
	mExplorer = new Explorer(mApp, 3, 3, mSize);
	mMummies[0] = new Mummy(mApp, 99, 99, 1);
	mMummies[0]->mAlive = false;
	mMummies[1] = new Mummy(mApp, 99, 99, 1);
	mMummies[1]->mAlive = false;
	delete mScorpion;
	mScorpion = new Scorpion(mApp, 99, 99);
	mScorpion->mAlive = false;
	mTrapX[0] = mTrapY[0] = mTrapX[1] = mTrapY[1] = 99;
	mGateX = mGateY = mKeyX = mKeyY = 99;
	mDeathTimer = -1;
	mGateClosed = true;
	mGateAnim = 0;
}

void Board::SetupTutorial(int thePart)
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
	mExplorer = new Explorer(mApp, kExplorer[thePart][0], kExplorer[thePart][1], mSize);
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
		mScorpion = new Scorpion(mApp, 5, 3);
	else
	{
		mScorpion = new Scorpion(mApp, 99, 99);
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
	mDeathTimer = -1;
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
