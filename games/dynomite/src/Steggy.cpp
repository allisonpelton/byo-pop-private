#include "Steggy.h"
#include "AngleTable.h"
#include "Dynomite.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "Launcher.h"
#include "PortCompat.h"
#include "Sprite.h"

using namespace Sexy;

// 0x437aa0
Steggy::Steggy(FossilizedApp* theApp, Dynomite* theBoard)
{
	mApp = theApp;
	mBoard = theBoard;
	mBob = 0;
	mDrop = 0;
	mBobAngle = 0;
	mNext = NULL;
	mToss = NULL;
	mField2c = 2000;
	mAlerting = false;
	mField28 = true;
	mWantEgg = true;
	mDropEgg = false;
	mBlinkCount = 0;
	// port: the original leaves these unset until a toss and the first blink
	mTossVX = 0;
	mTossVY = 0;
	mBlink = false;
}

// 0x437af0
Steggy::~Steggy()
{
	if (mNext != NULL)
		delete mNext;
	if (mToss != NULL)
		delete mToss;
}

// 0x437b20: the egg he holds goes up and right toward the slingshot
void Steggy::Toss()
{
	if (mNext != NULL)
	{
		mToss = mNext;
		mTossVX = 4.0f;
		mTossVY = -8.0f;
		mNext = NULL;
		mField2c = 1000;
		mAlerting = false;
	}
}

// 0x437ea0: lowered with the slingshot once the ceiling comes past 372
// port: the x87 code keeps the sums unrounded, so they work in double
void Steggy::Draw()
{
	Point anOffset;
	anOffset.mX = 0;
	anOffset.mY = 0;
	if (372.0f < mBoard->mGrid->mCeiling)
		anOffset.mY = (int) mBoard->mGrid->mCeiling;
	AddDrawOffset(anOffset);
	if (mNext != NULL)
		mNext->Draw();
	if (mToss != NULL)
		mToss->Draw();
	Dynomite* aBoard = mBoard;
	if ((0 < aBoard->mGrid->mPastLineFrames) && (aBoard->mField149 == true))
		mApp->mPicSteggyterror->Draw((aBoard->mField158 + aBoard->mField70) - 0xb0,
			(int) ((double) mDrop + (double) mBob + 372.0));
	else
		mApp->mPicSteggy->Draw((aBoard->mField158 + aBoard->mField70) - 0xb0,
			(int) ((double) mDrop + (double) mBob + 372.0));
	anOffset.mY = -anOffset.mY;
	AddDrawOffset(anOffset);
	aBoard = mBoard;
	if ((0 < aBoard->mGrid->mPastLineFrames) && (mBlink == true) && (aBoard->mField149 == true) &&
		(aBoard->mField130 == false))
	{
		// 0x437e60 draws a Pic at a float y
		float aY = (float) ((double) mDrop + (double) mBob + 372.0 - 12.0);
		mApp->mPicAlert->Draw((aBoard->mField158 + aBoard->mField70) - 99, (int) aY);
	}
}

// 0x438010: he ducks 8 a frame for an egg (and drops the one he holds when told to) and rises again with it; the
// tossed egg falls 0.75 a frame faster until it reaches the slingshot's middle plus 16
void Steggy::Update()
{
	if (mBoard->mField131 != true)
	{
		if (mWantEgg == true)
		{
			mNext = mBoard->NextEgg();
			mWantEgg = false;
		}
		if ((mNext != NULL) && (mDropEgg != true))
		{
			if (mDrop > 0.0f)
				mDrop = mDrop - 8.0f;
			if (mDrop < 0.0f)
				mDrop = 0;
		}
		else
		{
			mDrop = mDrop + 8.0f;
			if (mDrop > 160.0f)
			{
				if (mDropEgg == true)
				{
					if (mNext != NULL)
						delete mNext;
					mDropEgg = false;
				}
				mNext = mBoard->NextEgg();
			}
		}
		int anAngle = mBobAngle + 5;
		if (anAngle >= 0x168)
			anAngle = anAngle - 0x168;
		if (anAngle < 0)
			anAngle = anAngle + 0x168;
		mBobAngle = anAngle;
		mBob = mApp->mAngles->mDY[anAngle] * 5.0f;
		if (mNext != NULL)
		{
			mNext->mX = (float) ((mBoard->mField158 + mBoard->mField70) - 0x61);
			mNext->mY = (float) ((double) mBob + (double) mDrop + 424.0);
		}
		if (mToss != NULL)
		{
			mToss->mX = mTossVX + mToss->mX;
			mToss->mY = mToss->mY + mTossVY;
			mTossVY = mTossVY + 0.75f;
			if ((float) (mBoard->mField158 + 0x10 + mBoard->mField70) <= mToss->mX)
			{
				mBoard->mLauncher->SetLoaded(mToss);
				mToss = NULL;
			}
		}
		if ((mDrop <= 0.0f) && (mBoard->mFieldB0 == true))
		{
			mField2c = 1000;
			if (mAlerting == true)
			{
				mBlinkCount--;
				if (mBlinkCount < 1)
				{
					mBlink = mBlink ^ 1;
					mBlinkCount = 0x23;
					if (mBlink == true)
						mApp->mSndAlert->Play();
				}
			}
		}
		mBlinkCount--;
		if (mBlinkCount < 1)
		{
			mBlink = mBlink != true;
			mBlinkCount = 5;
		}
	}
}
