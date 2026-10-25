// Board.cpp, part 2: the small helpers of the board's lists and cells (WinSS.exe 0x402540-0x403580, 0x409e90,
// 0x40a000-0x40a440, 0x40aec0-0x40b490).

#include "Board.h"
#include "SevenSeasApp.h"
#include "Screens.h"
#include "Anims.h"
#include "Compat2004.h"
#include "WidgetManager.h"

#include <cstdlib>

using namespace Sexy;

// 0x40a3f0
int Board::CellToPixel(int theCell)
{
	return theCell * CELL_SIZE;
}

// 0x40a3b0
int Board::PixelToCellX(int theX)
{
	return theX / CELL_SIZE;
}

// 0x40a3d0
int Board::PixelToCellY(int theY)
{
	return (theY - 0xc) / CELL_SIZE;
}

// 0x40a440: the stereo position of a sound made at a cell column (the board's middle is 0; the original's sign:
// the left of the board gives a positive value)
int Board::Pan(int theCell)
{
	int aX = CellToPixel(theCell);
	int aSum = CELL_SIZE - CellToPixel(0) + CellToPixel(0xb);
	int aCenter = aSum / 2;
	return (int) ((double) (aCenter - aX) / (double) (aSum * 2) * 800.0);
}

// 0x402540: the first click on the bonus screen buries the treasure and the rocks (the clicked square stays empty)
void Board::SetupBonus(int theX, int theY)
{
	mState = STATE_IDLE;
	for (int i = 0; i < 81; i++)
		(&mBonusGrid[0][0])[i] = -1;
	int aRocks = mLevel / 10 + 0xd;
	int aPlaced = 0;
	if (0 < aRocks)
	{
		for (;;)
		{
			int aCol;
			int aRow;
			int anIdx;
			do
			{
				do
				{
					aCol = CrtRand() % 9;
					aRow = CrtRand() % 9;
					anIdx = aCol + aRow % 9 + aCol * 8;
				}
				while ((aCol == theX) && (aRow % 9 == theY));
			}
			while ((&mBonusGrid[0][0])[anIdx] != -1);
			aPlaced++;
			(&mBonusGrid[0][0])[anIdx] = -3;
			if (aPlaced >= aRocks)
				break;
		}
	}
	for (;;)
	{
		int aCol = CrtRand() % 9;
		int aRow = CrtRand() % 9;
		if ((aCol == theX) && (aRow == theY))
			continue;
		if ((&mBonusGrid[0][0])[aCol + aRow + aCol * 8] == -1)
		{
			(&mBonusGrid[0][0])[aCol + aRow + aCol * 8] = -2;
			mTreasureX = aCol;
			mTreasureY = aRow;
			mShovelX = -1;
			mShovelY = -1;
			mBonusReady = true;
			return;
		}
	}
}

// 0x402650: out of lives
void Board::GameOver()
{
	mState = STATE_LEVEL_END;
	mDirty = true;
	mGameStartCnt = mUpdateCnt;
	mApp->StartTalk();
	mApp->SetMusicOrder(0xe, false);
	mApp->SetMusicOrder(0xe, false);
}

// 0x403200: a ship was hit
void Board::LoseLife()
{
	ClearUndo(2);
	mLives--;
	mApp->PlaySample(SOUND_SINK);
	mLevelOver = true;
	mPlayerX = -1;
	mPlayerY = -1;
}

// 0x403240: points for a ship or a wreck (an extra life for every thousand)
void Board::AddPoints(int thePoints, int theX, int theY)
{
	if (mScore / 1000 < (mScore + thePoints) / 1000)
	{
		mApp->PlaySample(SOUND_BONUSSHIP);
		mLifeFlash = 0x82;
		mLives++;
	}
	mScore += thePoints;
	mPoints[theX][theY] += thePoints;
}

// 0x4032e0: the score check's next value (internet high scores): not ported
void Board::SetScoreCheck(int theValue)
{
	(void) theValue;
}

// 0x4032f0: starts an animation of data\anims.txt at a pixel position
void Board::AddAnim(const std::string& theName, int theX, int theY, bool theFlag)
{
	AnimDef* aDef = mApp->GetAnimDef(theName);
	if (aDef == NULL)
		return;
	AnimInstance* anAnim = new AnimInstance();
	anAnim->mDef = aDef;
	anAnim->mX = theX;
	anAnim->mY = theY;
	anAnim->mFrameIdx = 0;
	anAnim->mFlag = theFlag;
	if (aDef->mFrames.empty())
		anAnim->mDelay = 0;
	else
		anAnim->mDelay = aDef->mFrames[0].mDelay;
	mAnims.push_back(anAnim);
}

// 0x4033c0: true when no animation of the list is one the turn waits for (flags A and B both 0)
bool Board::AllPortsDone()
{
	bool aResult = true;
	for (std::list<AnimInstance*>::iterator anItr = mAnims.begin(); anItr != mAnims.end(); ++anItr)
	{
		if (((*anItr)->mDef->mFlagA == 0) && ((*anItr)->mDef->mFlagB == 0))
			aResult = false;
	}
	return aResult;
}

// 0x4033f0: the ship on a cell (not theExcept)
Ship* Board::ShipAt(int theX, int theY, Ship* theExcept)
{
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
	{
		Ship* aShip = *anItr;
		if ((aShip != theExcept) && (theX == aShip->mNextX) && (theY == aShip->mNextY))
			return aShip;
	}
	return NULL;
}

// 0x403430
void Board::RemoveShip(Ship* theShip)
{
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
	{
		if (*anItr == theShip)
		{
			delete theShip;
			mShips.erase(anItr);
			return;
		}
	}
}

// 0x403490
void Board::ClearPorts()
{
	for (std::list<Port*>::iterator anItr = mPorts.begin(); anItr != mPorts.end(); ++anItr)
		delete *anItr;
	mPorts.clear();
}

// 0x4034e0
void Board::ClearAnims()
{
	for (std::list<AnimInstance*>::iterator anItr = mAnims.begin(); anItr != mAnims.end(); ++anItr)
		delete *anItr;
	mAnims.clear();
}

// 0x403530
void Board::ClearShots()
{
	for (std::list<Shot*>::iterator anItr = mShots.begin(); anItr != mShots.end(); ++anItr)
		delete *anItr;
	mShots.clear();
}

// 0x403580
void Board::ClearShips()
{
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
		delete *anItr;
	mShips.clear();
}

// 0x405ef0
void Board::ClearCellFlash()
{
	for (int i = 0; i < GRID * GRID; i++)
		(&mCellFlash[0][0])[i] = 0;
}

// 0x402be0: an explosion at a cell (the big one with its own sound)
void Board::SpawnExplosion(int theX, int theY, bool theBig)
{
	AddAnim("EXPLODE", CellToPixel(theX) + 0x14, CellToPixel(theY) + 0x14, false);
	if (!theBig)
		mApp->PlaySample(SOUND_EXPLODE, Pan(theX));
	else
		mApp->PlaySample(SOUND_EXPLODE2, Pan(theX));
}

// 0x402d10
void Board::SpawnSplash(int theX, int theY)
{
	AddAnim("SPLASH", CellToPixel(theX) + 0x12, CellToPixel(theY) + 0x12, false);
}

// 0x402e10: the ghost that rises from a sunk ship
void Board::SpawnGhost(int theX, int theY)
{
	AddAnim("GHOST", CellToPixel(theX) + 0xf, CellToPixel(theY) + 0xf, false);
}

// 0x402f10: a wake behind a ship (pixel position)
void Board::SpawnWake(int theX, int theY)
{
	AddAnim("WAKE", theX + 0x14, theY + 0x14, true);
}

// 0x403000: a fainter wake
void Board::SpawnSmoke(int theX, int theY)
{
	AddAnim("WAKEDIM", theX + 0x14, theY + 0x14, true);
}

// 0x4030f0: a sparkle somewhere in the cell
void Board::SpawnSparkle(int theX, int theY)
{
	int aY = abs(CrtRand() % 0x20) + theY;
	int aX = abs(CrtRand() % 0x20) + theX;
	AddAnim("SPARKLE", aX, aY, false);
}

// 0x409e90: puts the player's ship on a random free cell away from every ship (the tutorial has a fixed one)
void Board::PlaceShip()
{
	int aX = 0;
	int aY = 0;
	bool aFree = false;
	bool aAway = false;
	while (!aFree || !aAway)
	{
		aFree = false;
		aAway = false;
		aX = CrtRand() % 0xb;
		aY = CrtRand() % 0xb;
		if ((mTerrain[aX][aY] == 0) && !mVisited[aX][aY] &&
			(((aX != 0) && (aX != 10)) || ((aY != 0) && (aY != 10))))
		{
			aFree = true;
			aAway = true;
			for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
			{
				for (int x = aX - 1; x <= aX + 1; x++)
					for (int y = aY - 1; y <= aY + 1; y++)
						if (((*anItr)->mNextX == x) && ((*anItr)->mNextY == y))
							aAway = false;
			}
		}
	}
	if (!mApp->mTutorial)
	{
		mPlayerX = aX;
		mPlayerY = aY;
	}
	else
	{
		mPlayerX = 4;
		mPlayerY = 8;
	}
	mDestX = mPlayerX;
	mDestY = mPlayerY;
	mHeading = 4;
	mWantHeading = 4;
	mMoveProgress = 0;
}

// 0x40a000: a cell the player may sail to
bool Board::CellFree(int theX, int theY)
{
	if ((mTerrain[theX][theY] == 0) && !mVisited[theX][theY])
		return ShipAt(theX, theY, NULL) == NULL;
	return false;
}

// 0x40a390: some animation of the list was added without the flag
bool Board::AnyBlockingAnim()
{
	for (std::list<AnimInstance*>::iterator anItr = mAnims.begin(); anItr != mAnims.end(); ++anItr)
	{
		if (!(*anItr)->mFlag)
			return true;
	}
	return false;
}

// 0x40b490: forgets the undo list; the score check takes the reason
void Board::ClearUndo(int theScoreCheck)
{
	mScoreCheck = theScoreCheck;
	mUndo.clear();
}

// 0x40aec0: pushes the position on the undo list (the score, the ships, the lives, the player, the ports, the
// cells with a wreck)
void Board::SaveUndo()
{
	mUndo.push_back(mScore);
	mUndo.push_back(mKills);
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
	{
		Ship* aShip = *anItr;
		mUndo.push_back(aShip->mX);
		mUndo.push_back(aShip->mY);
		mUndo.push_back(aShip->mHeading);
		mUndo.push_back(aShip->mType);
	}
	mUndo.push_back((int) mShips.size());
	mUndo.push_back(mLives);
	mUndo.push_back(mPlayerX);
	mUndo.push_back(mPlayerY);
	mUndo.push_back(mHeading);
	for (std::list<Port*>::iterator anItr = mPorts.begin(); anItr != mPorts.end(); ++anItr)
		mUndo.push_back((*anItr)->mActive ? 1 : 0);
	for (int x = 0; x < GRID; x++)
		for (int y = 0; y < GRID; y++)
			mUndo.push_back(mVisited[x][y] ? 1 : 0);
}

// 0x40b0b0: takes the last move back
bool Board::Undo()
{
	if (mBonus)
		return false;
	if (!mUndo.empty())
	{
		for (int x = GRID - 1; x >= 0; x--)
			for (int y = GRID - 1; y >= 0; y--)
			{
				mVisited[x][y] = mUndo.back() != 0;
				mUndo.pop_back();
			}
		int aPortFlags[4];
		for (int i = 3; i >= 0; i--)
		{
			aPortFlags[i] = mUndo.back() != 0;
			mUndo.pop_back();
		}
		int anIdx = 0;
		for (std::list<Port*>::iterator anItr = mPorts.begin(); anItr != mPorts.end(); ++anItr, anIdx++)
			(*anItr)->mActive = aPortFlags[anIdx] != 0;
		mHeading = mUndo.back();
		mUndo.pop_back();
		mPlayerY = mUndo.back();
		mUndo.pop_back();
		mPlayerX = mUndo.back();
		mUndo.pop_back();
		mLives = mUndo.back();
		mUndo.pop_back();
		ClearShips();
		int aCount = mUndo.back();
		mUndo.pop_back();
		while (aCount > 0)
		{
			int aType = mUndo.back();
			mUndo.pop_back();
			int aHeading = mUndo.back();
			mUndo.pop_back();
			int aY = mUndo.back();
			mUndo.pop_back();
			int aX = mUndo.back();
			mUndo.pop_back();
			Ship* aShip = new Ship();
			aShip->mWantHeading = aHeading;
			aShip->mHeading = aHeading;
			aShip->mNextX = aX;
			aShip->mX = aX;
			aShip->mNextY = aY;
			aShip->mY = aY;
			aShip->mType = aType;
			aShip->mMoveProgress = 0;
			aShip->mAnim = CrtRand() % 8;
			for (int i = 0; i < 9; i++)
				aShip->mTrail[i] = -1;
			mShips.push_back(aShip);
			aCount--;
		}
		mKills = mUndo.back();
		mUndo.pop_back();
		mScore = mUndo.back();
		mUndo.pop_back();
		return true;
	}
	mHintTimer = 600;
	mHintReason = mScoreCheck;
	return false;
}
