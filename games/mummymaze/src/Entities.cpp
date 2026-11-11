// The board's three kinds of pieces: the explorer (WinMM.exe 0x426e70), the mummies (0x421b40) and the scorpion (0x429120).
// In the original they are Widgets that the board draws itself; only Update is used.

#include "Board.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Screens.h"
#include "MummyApp.h"
#include "Compat2004.h"

#include <cstdlib>

using namespace Sexy;

#include "Tables.inc"

// 0x426e70: the entrance of the maze is a cell on its border; the explorer starts just outside it, on the nearest
// side (always the bottom one on the ending board)
Player::Player(MMApp* theApp, int theX, int theY, int theSize) :
	mApp(theApp)
{
	mDir = 2;
	int aDist[4] = {theX, theSize - theX, theY, theSize - theY};
	int aSide = 0;
	int aMin = aDist[0];
	for (int i = 1; i < 4; i++)
	{
		if (aDist[i] < aMin)
		{
			aSide = i;
			aMin = aDist[i];
		}
	}
	int aMode = mApp->mGameMode;
	if ((aMode == MODE_ADVENTURE_DONE) || (aMode == MODE_CLASSIC_DONE))
		aSide = 3;
	switch (aSide)
	{
	case 0:
		mX = -1;
		mY = theY;
		break;
	case 1:
		mX = theSize;
		mY = theY;
		break;
	case 2:
		mX = theX;
		mY = -1;
		break;
	default:
		mX = theX;
		mY = theSize;
		break;
	}
	mProgress = 0.0;
	mIdleTimer = 0;
	mIdle = false;
	mIdleKind = 0;
	mIdleFrame = 0;
	mTargetX = theX;
	mTargetY = theY;
	mDirty = false;
}

// 0x426f70: now and then the explorer looks around
void Player::Update()
{
	mIdleTimer++;
	if (mIdle)
	{
		mIdleFrame++;
		if (mIdleFrame < kExplorerIdleLen[mIdleKind] * 5)
		{
			if (mIdleFrame % 5 == 0)
				mDirty = true;
		}
		else
		{
			mIdleFrame = 0;
			mIdleTimer = 0;
			mIdle = false;
			mDirty = true;
		}
	}
	if ((mIdleTimer > 900) && !mIdle)
	{
		if (CrtRand() % 0xfa == 0)
		{
			mIdle = true;
			mIdleKind = CrtRand() % kExplorerIdleCount[0];
			mIdleFrame = 0;
			mDir = 2;
			mDirty = true;
		}
	}
}

// 0x421b40
Mummy::Mummy(MMApp* theApp, int theX, int theY, int theIndex) :
	mApp(theApp)
{
	mX = theX;
	mY = theY;
	mTargetX = theX;
	mTargetY = theY;
	mProgress = 0.0;
	mDir = 2;
	mWantDir = 2;
	mTurn = 0;
	mIndex = theIndex;
	mFighting = false;
	mFightFrame = -1;
	mAlive = true;
	mIdleTimer = 0;
	mIdle = false;
	mIdleKind = 0;
	mIdleFrame = 0;
	mDirty = false;
}

// 0x421bf0
void Mummy::Update()
{
	mIdleTimer++;
	if (mIdle)
	{
		mIdleFrame++;
		if (mIdleFrame < kMummyIdleLen[mIdleKind] * 5)
		{
			if (mIdleFrame % 5 == 0)
				mDirty = true;
		}
		else
		{
			mIdleFrame = 0;
			mIdleTimer = 0;
			mIdle = false;
			mDirty = true;
		}
	}
	if ((mIdleTimer > 900) && !mIdle)
	{
		if (CrtRand() % 300 == 0)
		{
			mIdle = true;
			mIdleKind = CrtRand() % kMummyIdleCount[0];
			mIdleFrame = 0;
			mDir = 2;
			mDirty = true;
		}
	}
}

// 0x421cc0: start an idle animation now (never kind 0)
void Mummy::DoIdle()
{
	mIdle = true;
	do
	{
		mIdleKind = CrtRand() % kMummyIdleCount[0];
	} while (mIdleKind == 0);
	mIdleFrame = 0;
	mDir = 2;
	mDirty = true;
}

// port: one mummy's part of Board::DoResetIdles (0x407960)
void Mummy::ResetIdle()
{
	mIdleTimer = 0;
	mIdle = false;
	mIdleFrame = 0;
}

// 0x429120
Scorp::Scorp(MMApp* theApp, int theX, int theY) :
	mApp(theApp)
{
	mX = theX;
	mY = theY;
	mFromX = theX;
	mFromY = theY;
	mToX = theX;
	mToY = theY;
	mProgress = 0;
	mDir = 2;
	mStinging = false;
	mStingFrame = 0;
	mAlive = true;
}
