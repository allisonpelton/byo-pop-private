#include "Whirley.h"
#include "BinFile.h"
#include "DynomitePanic.h"
#include "Egg.h"
#include "FossilizedApp.h"
#include "Sprite.h"
#include <cmath>

using namespace Sexy;

// 0x52b400: the egg's sway under the whirley, by frame
static const int gWhirleySway[12][2] =
{
	{0, 0}, {-1, 1}, {-2, 2}, {-3, 2}, {-4, 2}, {-3, 2}, {-3, 1}, {-3, 1}, {-3, 0}, {-3, 0}, {-2, 0}, {-1, 0}
};

// 0x435cd0
CeilingSpeed::CeilingSpeed(FossilizedApp* theApp, DynomitePanic* theBoard, int theSkill)
{
	mApp = theApp;
	mLow = 100;
	mWaitReset = 100;
	mBoard = theBoard;
	mHigh = 200;
	mMinSpeed = 0.04f;
	mWait = 0;
	mSpeed = 0.04f;
}

// 0x435d10
CeilingSpeed::~CeilingSpeed()
{
}

// 0x435d20: where the lowest row's eggs are on the screen
int CeilingSpeed::Update()
{
	if (mWait > 0)
	{
		mWait--;
		return 0;
	}
	EggGrid* aGrid = mBoard->mGrid;
	GridPoint aPoint;
	aGrid->GridToScreen(0, aGrid->mBottom, &aPoint);
	if (aPoint.mY > mHigh)
	{
		double aSpeed = (double) mSpeed * 0.5;
		mWait = mWaitReset;
		mSpeed = (float) aSpeed;
		if (aSpeed < mMinSpeed)
			mSpeed = mMinSpeed;
		return 2;
	}
	if (aPoint.mY < mLow)
	{
		double aSpeed = (double) mSpeed * (double) 1.25f;
		mWait = mWaitReset;
		mSpeed = (float) aSpeed;
		if (aSpeed > 0.5)
			mSpeed = 0.5f;
		return 1;
	}
	return 0;
}

// 0x435dc0
void CeilingSpeed::ResetSpeed()
{
	mSpeed = mMinSpeed;
}

// 0x435e70
void CeilingSpeed::LevelUp()
{
	double aSpeed = (double) mMinSpeed * (double) 1.2f;
	mMinSpeed = (float) aSpeed;
	if (aSpeed > (double) 0.6f)
		mMinSpeed = 0.6f;
	mHigh += 5;
	if (mHigh > 0xfa)
		mHigh = 0xfa;
}

// 0x435eb0
void CeilingSpeed::Reset()
{
	mMinSpeed = 0.06f;
	mHigh = 200;
}

// 0x441c90: below the board, at most half its width less 100 to either side of its middle
Whirley::Whirley(FossilizedApp* theApp, DynomitePanic* theBoard, EggGrid* theGrid, int theLevel, int theColor)
{
	mBoard = theBoard;
	mGrid = theGrid;
	mLevel = theLevel;
	mApp = theApp;
	mColor = theColor;
	mFrame = 0.0f;
	int aX;
	if ((mApp->mRand.NextNoAssert() & 1) == 0)
		aX = mApp->mRand.NextNoAssert() % (unsigned int) (mBoard->mField6c - 100) + mBoard->mField70;
	else
		aX = mBoard->mField70 - mApp->mRand.NextNoAssert() % (unsigned int) (mBoard->mField6c - 100);
	mX = (float) (unsigned int) aX;
	mY = 570.0f;
	mVX = 0.0f;
	mVY = -20.0f;
	if (mLevel != 0)
	{
		mSpeed = 1.0f;
		mClimb = 1.0f;
		int i = 0;
		if (0 < mLevel)
		{
			do
			{
				i++;
				mSpeed = (float) ((double) mSpeed * (double) 1.2f);
				mClimb = (float) ((double) mClimb + (double) 0.35f);
			} while (i < mLevel);
		}
		if (mClimb > 12.0f)
			mClimb = 12.0f;
		if ((mApp->mRand.NextNoAssert() & 1) == 0)
		{
			mSpeed = -mSpeed;
			mVX = mSpeed;
		}
	}
	else
	{
		mSpeed = 0.0f;
		mClimb = 1.0f;
	}
	mState = 0;
	mTarget.mHit = false;
	mTarget.mHitBits = 8;
	// port: the original leaves the rectangle to the first update
	mTarget.mX1 = 0;
	mTarget.mY1 = 0;
	mTarget.mX2 = 0;
	mTarget.mY2 = 0;
	mGrid->mTargets.Add(&mTarget);
}

// 0x441de0
Whirley::~Whirley()
{
	mGrid->mTargets.Remove(&mTarget);
}

// 0x441e00: the target is 44 pixels square round its egg
bool Whirley::Update()
{
	int aFrame;
	double anEggX;
	if (mVX < 0.0f)
	{
		aFrame = (int) mFrame;
		anEggX = ((double) gWhirleySway[aFrame][0] + mX) + 11.0;
	}
	else
	{
		aFrame = (int) mFrame;
		anEggX = ((double) mX - 11.0) - gWhirleySway[aFrame][0];
	}
	int x = (int) anEggX;
	int y = (int) (((double) gWhirleySway[aFrame][1] + mY) + 48.0);
	if (mTarget.mHit == true)
	{
		// port: the original keeps the high bits of a stack slot above the colour's, which Break does not read
		GridCell aCell;
		aCell.mBits = mColor & 0xff;
		mState = 2;
		mGrid->Break(x - 0x10, y - 0x10, &aCell);
		mTarget.mHit = false;
		mApp->mSndWhirleyhit->Play();
		mBoard->mCeilingSpeed->LevelUp();
		y -= 0x32;
		mBoard->AddPoints(x, y, 0x19, 0, true);
		mBoard->mWhirleysHit++;
	}
	if (mState == 2)
		mTarget.mX2 = -100;
	else
	{
		mTarget.mX1 = x - 0x16;
		mTarget.mY1 = y - 0x16;
		mTarget.mX2 = x + 0x16;
		mTarget.mY2 = y + 0x16;
		if (fabsf(mVY) < mClimb)
		{
			mVX = mSpeed;
			mVY = -mClimb;
			mState = 1;
		}
	}
	if (mState == 0)
	{
		mY = (float) ((double) mY + mVY);
		mVY = (float) ((double) mVY * (double) 0.9f);
	}
	else if (mState == 1)
	{
		double aX = (double) mX + mVX;
		mX = (float) aX;
		mY = (float) ((double) mVY + mY);
		if (!(aX < (double) (mBoard->mField70 - 0x32 + mBoard->mField6c)))
			mVX = -fabsf(mVX);
		if (!((float) (mBoard->mField70 - mBoard->mField6c + 0x32) < mX))
			mVX = fabsf(mVX);
	}
	else if (mState == 2)
	{
		mFrame = (float) ((double) mFrame + 1.0);
		mY = (float) ((double) mY - 7.0);
	}
	double aFrameNext = (double) mFrame + 0.25;
	mFrame = (float) aFrameNext;
	if (!(aFrameNext < 12.0))
		mFrame = (float) (aFrameNext - 12.0);
	if (!(mY < -50.0f))
		return true;
	if (mState == 2)
	{
		mBoard->mWhirleyLevel++;
		return false;
	}
	if (mGrid->mMaxColor < mColor)
		mGrid->mMaxColor = mColor;
	mBoard->mCeilingSpeed->Reset();
	mBoard->mWhirleyLevel = 0;
	EggGrid* aGrid = mGrid;
	float aFactor;
	switch (aGrid->mMaxColor)
	{
	case 3:
		aFactor = 10.0f;
		break;
	case 4:
		aFactor = 5.0f;
		break;
	case 5:
		aFactor = 2.5f;
		break;
	case 6:
		aFactor = 1.0f;
		break;
	case 7:
		aFactor = 0.5f;
		break;
	default:
		aFactor = 0.25f;
		break;
	}
	aGrid->mBabyEggChance = (int) ((double) aGrid->mField46b4 * aFactor);
	return false;
}

// 0x4421e0: its egg sways under it while it rises and crosses
void Whirley::Draw()
{
	if (mState < 0)
		return;
	if (mState <= 1)
	{
		if (mVX < 0.0f)
		{
			int aFrame = (int) mFrame;
			float aY = (float) (((double) gWhirleySway[aFrame][1] + mY) + 38.0);
			float aX = (float) (((double) gWhirleySway[aFrame][0] + mX) + 8.0);
			DrawCellAt(EggCell(mApp, mColor), aX, aY);
			DrawCellAt(mApp->mWhirleyCells[(int) mFrame], mX, mY);
		}
		else
		{
			int aFrame = (int) mFrame;
			float aY = (float) (((double) gWhirleySway[aFrame][1] + mY) + 38.0);
			float aX = (float) (((double) mX - 8.0) - gWhirleySway[aFrame][0]);
			DrawCellAt(EggCell(mApp, mColor), aX, aY);
			DrawCellAt(mApp->mWhirleyCells[(int) mFrame + 12], mX, mY);
		}
	}
	else if (mState == 2)
	{
		if (mVX < 0.0f)
			DrawCellAt(mApp->mWhirleyCells[(int) mFrame], mX, mY);
		else
			DrawCellAt(mApp->mWhirleyCells[(int) mFrame + 12], mX, mY);
	}
}

// 0x435dd0
void CeilingSpeed::SaveState(BinFile* theFile)
{
	theFile->WriteInt(mHigh);
	theFile->WriteInt(mLow);
	theFile->WriteInt(mWait);
	theFile->WriteInt(mWaitReset);
	theFile->WriteFloat(mSpeed);
	theFile->WriteFloat(mMinSpeed);
}

// 0x435e20
void CeilingSpeed::LoadState(BinFile* theFile)
{
	mHigh = theFile->ReadInt();
	mLow = theFile->ReadInt();
	mWait = theFile->ReadInt();
	mWaitReset = theFile->ReadInt();
	mSpeed = theFile->ReadFloat();
	mMinSpeed = theFile->ReadFloat();
}

// 0x4420e0: its grid target as the 0x18 bytes it is
void Whirley::SaveState(BinFile* theFile)
{
	theFile->WriteFloat(mX);
	theFile->WriteFloat(mY);
	theFile->WriteFloat(mVX);
	theFile->WriteFloat(mVY);
	theFile->WriteFloat(mSpeed);
	theFile->WriteFloat(mClimb);
	theFile->WriteInt(mLevel);
	theFile->WriteInt(mColor);
	theFile->WriteInt(mState);
	theFile->WriteFloat(mFrame);
	theFile->Write(&mTarget, 0x18);
}

// 0x442150
void Whirley::LoadState(BinFile* theFile)
{
	mX = theFile->ReadFloat();
	mY = theFile->ReadFloat();
	mVX = theFile->ReadFloat();
	mVY = theFile->ReadFloat();
	mSpeed = theFile->ReadFloat();
	mClimb = theFile->ReadFloat();
	mLevel = theFile->ReadInt();
	mColor = theFile->ReadInt();
	mState = theFile->ReadInt();
	mFrame = theFile->ReadFloat();
	theFile->Read(&mTarget, 0x18);
}
