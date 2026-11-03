#include "Tank.h"
#include "Dynomite.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "Sprite.h"
#include "Steggy.h"

using namespace Sexy;

// 0x438400
Tank::Tank(FossilizedApp* theApp, Dynomite* theBoard)
{
	mApp = theApp;
	mBoard = theBoard;
	mEye = 0;
	mEyeTime = 0;
	mState = 0;
	// port: the original leaves these unset until his first call
	mTime = 0;
	mPhrase = 0;
	mWord = NULL;
	mMouthCount = 0;
	for (int i = 0; i < 10; i++)
	{
		mMouth[i].mFrame = 0;
		mMouth[i].mFrames = 0;
	}
}

// 0x438430
Tank::~Tank()
{
}

// 0x438440
void Tank::Update()
{
	if (mEyeTime < 1)
	{
		mEyeTime = mApp->mRand.NextNoAssert() % 100 + 100;
		mEye = mApp->mRand.NextNoAssert() & 3;
	}
	else
		mEyeTime--;
	if (mState == 1)
	{
		mTime--;
		if (mTime < 1)
			mState = 0;
	}
	if (mState == 2)
	{
		mMouth[mTime].mFrames--;
		if (mMouth[mTime].mFrames < 1)
		{
			mTime++;
			if (mMouthCount <= mTime)
				mState = 0;
		}
	}
}

// 0x4384d0: he flinches for 40 updates; for five or more (not while the eggs are past the line) he calls out a
// phrase at random instead, the blast's always the third
void Tank::Flinch(int theCount)
{
	mState = 1;
	mTime = 0x28;
	mEyeTime = 0;
	if ((0 < mBoard->mGrid->mPastLineFrames) && (mBoard->mField149 == true))
		return;
	if (theCount < 5)
		return;
	mState = 2;
	mPhrase = mApp->mRand.NextNoAssert() & 3;
	if (theCount == 100)
		mPhrase = 2;
	switch (mPhrase)
	{
	case 0:
		mMouth[3].mFrame = 3;
		mMouth[0].mFrame = 0;
		mMouth[0].mFrames = 10;
		mMouth[1].mFrame = 1;
		mMouth[1].mFrames = 0x23;
		mMouth[2].mFrame = 0;
		mMouth[2].mFrames = 10;
		mMouth[3].mFrames = 0x28;
		mMouth[4].mFrame = 0;
		mMouth[4].mFrames = 0xf;
		mMouthCount = 5;
		mWord = mApp->mPicTextwords_c4;
		break;
	case 1:
		mMouth[0].mFrame = 1;
		mMouth[0].mFrames = 0x23;
		mMouth[1].mFrame = 0;
		mMouth[1].mFrames = 0xf;
		mMouthCount = 2;
		mWord = mApp->mPicTextwords_c2;
		break;
	case 2:
		mMouthCount = 3;
		mMouth[0].mFrame = 1;
		mMouth[0].mFrames = 0xf;
		mMouth[1].mFrame = 2;
		mMouth[1].mFrames = 0x23;
		mMouth[2].mFrame = 0;
		mMouth[2].mFrames = 0x23;
		mWord = mApp->mPicTextwords_c3;
		break;
	case 3:
		mMouth[1].mFrame = 3;
		mMouth[3].mFrame = 3;
		mMouth[0].mFrame = 1;
		mMouth[0].mFrames = 0x12;
		mMouth[1].mFrames = 6;
		mMouth[2].mFrame = 0;
		mMouth[2].mFrames = 5;
		mMouth[3].mFrames = 10;
		mMouth[4].mFrame = 1;
		mMouth[4].mFrames = 0x14;
		mMouth[5].mFrame = 2;
		mMouth[5].mFrames = 0x1e;
		mMouth[6].mFrame = 0;
		mMouth[6].mFrames = 0xf;
		mMouthCount = 7;
		mWord = mApp->mPicTextwords_c1;
		break;
	}
	mTime = 0;
	// port: the original's app +0x1a00 + phrase * 4 is the four voices from Blastit on
	GameSound* aVoices[4] = { mApp->mSndBlastit, mApp->mSndCharge, mApp->mSndKaboom, mApp->mSndFireinthehole };
	aVoices[mPhrase]->Play();
}

// 0x438690: lowered with the slingshot once the ceiling comes past 433
void Tank::Draw()
{
	Cell* aCell = mApp->mPicTanksheet_c0;
	Dynomite* aBoard = mBoard;
	int y = 0x1b1;
	float aLowest = (float) (aCell->mHeight / 2) + aBoard->mGrid->mCeiling;
	if (433.0f < aLowest)
		y = (int) aLowest;
	if ((aBoard->mGrid->mPastLineFrames < 1) || (aBoard->mField149 != true))
	{
		if (mState == 0)
		{
			aCell->Draw((aBoard->mField158 - (aCell->mWidth >> 1)) + 0x78 + aBoard->mField70, y - (aCell->mHeight >> 1));
			mApp->mTankEyeCells[mEye]->Draw(mBoard->mField158 + 0x5c + mBoard->mField70, y - 0x15);
		}
		else if (mState == 1)
		{
			aCell = mApp->mPicTanksheet_c1;
			aCell->Draw((aBoard->mField158 - (aCell->mWidth >> 1)) + 0x78 + aBoard->mField70, y - (aCell->mHeight >> 1));
		}
		else if (mState == 2)
		{
			aCell = mApp->mPicTanksheet_c2;
			aCell->Draw((aBoard->mField158 - (aCell->mWidth >> 1)) + 0x78 + aBoard->mField70, y - (aCell->mHeight >> 1));
			mApp->mTankMouthCells[mMouth[mTime].mFrame]->Draw(mBoard->mField158 + 0x53 + mBoard->mField70, y - 0xc);
			aCell = mApp->mPicTextballoons_c1;
			aCell->Draw((mBoard->mField158 - (aCell->mWidth >> 1)) + 0xb4 + mBoard->mField70,
				(y - (aCell->mHeight >> 1)) - 0x28);
			aCell = mWord;
			aCell->Draw((mBoard->mField158 - (aCell->mWidth >> 1)) + 0xb3 + mBoard->mField70,
				(y - (aCell->mHeight >> 1)) - 0x2e);
		}
	}
	else
	{
		Pic* aPic = mApp->mPicTankterror;
		aPic->Draw((aBoard->mField158 - (aPic->mWidth >> 1)) + 0x78 + aBoard->mField70, y - (aPic->mHeight >> 1));
		aBoard = mBoard;
		if (aBoard->mSteggy->mBlink == true)
		{
			aPic = mApp->mPicTanksweat;
			aPic->Draw((aBoard->mField158 - (aPic->mWidth >> 1)) + 100 + aBoard->mField70,
				(y - (aPic->mHeight >> 1)) - 0x3c);
		}
	}
}
