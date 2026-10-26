#include "Launcher.h"
#include "AngleTable.h"
#include "Dynomite.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "PortCompat.h"
#include "Sprite.h"

using namespace Sexy;

// 0x401050
Launcher::Launcher(FossilizedApp* theApp, Dynomite* theBoard)
{
	mApp = theApp;
	mBoard = theBoard;
	mAngle = 0;
	mLoaded = NULL;
	mFlying = NULL;
	mField1c = 0;
	mRecoilX = 0;
	mRecoilY = 0;
	mRand.SRand(GetTickCount());
	mField18 = 0;
	mFieldA5c = 0x32;
	int x = mBoard->mField158 + mBoard->mField70;
	mAimX = x;
	mFieldA60 = 0;
	mFieldA64 = 0;
	mFieldA68 = 0;
	mFieldA70 = 0;
	mFrozen = false;
	mHeld = NULL;
	mFieldAac = 0;
	mSlingX = x;
	mSlingY = 0x1b0;
	mAimY = 0x17e;
	mFieldA6c = 0xff;
}

// 0x401160
Launcher::~Launcher()
{
	if (mLoaded != NULL)
	{
		delete mLoaded;
		mLoaded = NULL;
	}
	if (mHeld != NULL)
	{
		delete mHeld;
		mHeld = NULL;
	}
}

// 0x4011f0
void Launcher::SetLoaded(LoadedEgg* theEgg)
{
	if (mLoaded == NULL)
		mLoaded = theEgg;
}

// 0x401210
void Launcher::Load(int theColor)
{
	if (mLoaded == NULL)
	{
		mLoaded = new LoadedEgg();
		mLoaded->Init(mApp, mBoard, theColor);
		mLoaded->mX = 100.0f;
		mLoaded->mY = 100.0f;
	}
}

// 0x4012a0: the loaded egg flies along the launcher's angle (a pixel a step, ten steps a frame) and the launcher
// recoils 20 pixels the other way
void Launcher::Fire()
{
	if ((mLoaded != NULL) && (mFrozen != true))
	{
		mBoard->mField160 = mLoaded->mColor;
		mBoard->mField15c = mBoard->mField160;
		mApp->mSndRubbershot->Play();
		AngleTable* anAngles = mApp->mAngles;
		float aDX = anAngles->mDX[mAngle];
		float aDY = anAngles->mDY[mAngle];
		mRecoilX = -(aDX * 20.0f);
		mRecoilY = -(aDY * 20.0f);
		FlyingEgg* anEgg = new FlyingEgg();
		anEgg->mX = mLoaded->mX;
		anEgg->mY = mLoaded->mY;
		anEgg->mColor = mLoaded->mColor;
		anEgg->mApp = mApp;
		anEgg->mBoard = mBoard;
		anEgg->mVY = aDY;
		anEgg->mVX = aDX;
		anEgg->mKind = mLoaded->mKind;
		mFlying = anEgg;
		mBoard->mFlyingEggs.Add(anEgg);
		if (mLoaded != NULL)
			delete mLoaded;
		mLoaded = NULL;
		// TODO: the board's +0x84 (0x437aa0) hears of it (0x437b20)
	}
}

// 0x4014f0
void Launcher::Freeze(Egg* theEgg)
{
	mFrozen = true;
	mHeld = theEgg;
	mFieldAac = 0;
}

// 0x401940: the slingshot and its loaded egg, drawn lower by the ceiling's depth plus half the slingshot's height
// once the ceiling comes past it
void Launcher::DrawSling(Graphics* g)
{
	Point anOffset;
	anOffset.mX = 0;
	anOffset.mY = 0;
	Pic* aPic = mApp->mPicSlingshot;
	float aLimit = (float) (aPic->mHeight / 2) + mBoard->mGrid->mCeiling;
	if ((float) mSlingY < aLimit)
		anOffset.mY = (int) aLimit;
	AddDrawOffset(anOffset);
	aPic = mApp->mPicSlingshot;
	aPic->Draw(mSlingX - (aPic->mWidth >> 1), mSlingY - (aPic->mHeight >> 1));
	if (mLoaded != NULL)
		mLoaded->Draw();
	anOffset.mY = -anOffset.mY;
	AddDrawOffset(anOffset);
}

// 0x401a10: the rubber band from the slingshot's two posts, round the loaded egg (and back to the pull) or the
// flying one until it is above 388, and the hand; moved down like the slingshot
void Launcher::DrawBand()
{
	Point anOffset;
	anOffset.mX = 0;
	anOffset.mY = 0;
	float aLimit = (float) (mApp->mPicSlingshot->mHeight / 2) + mBoard->mGrid->mCeiling;
	if ((float) mSlingY < aLimit)
		anOffset.mY = (int) aLimit;
	AddDrawOffset(anOffset);
	int aMiddle = mBoard->mField158 + mBoard->mField70;
	if ((mLoaded == NULL) && (mFlying == NULL))
		mApp->mPicRubberband->DrawLine(aMiddle - 0x36, 0x18c, aMiddle + 0x36, 0x18c);
	Cell* aHand;
	if (mLoaded != NULL)
	{
		mApp->mPicRubberband->DrawLine(aMiddle - 0x36, 0x18c, (int) (mLoaded->mX - 16.0f), (int) mLoaded->mY);
		mApp->mPicRubberband->DrawLine(aMiddle + 0x36, 0x18c, (int) (mLoaded->mX + 16.0f), (int) mLoaded->mY);
		mApp->mPicRubberband->DrawLine((int) (mLoaded->mX + 16.0f), (int) mLoaded->mY,
			(int) ((float) mField38[0] + mLoaded->mX), (int) ((float) mField38[1] + mLoaded->mY));
		mApp->mPicRubberband->DrawLine((int) (mLoaded->mX - 16.0f), (int) mLoaded->mY,
			(int) ((float) mField38[0] + mLoaded->mX), (int) ((float) mField38[1] + mLoaded->mY));
		mField20[0] = (int) ((float) (mField38[0] * 2) + mLoaded->mX);
		mField20[1] = (int) ((mLoaded->mY - 10.0f) + (float) mField38[1] + 50.0f);
		aHand = mApp->mHandCells[8 - mField18 / 8];
		aHand->Draw(mField20[0] - (aHand->mWidth >> 1), mField20[1] - (aHand->mHeight >> 1));
	}
	else
	{
		aHand = mApp->mHandCells[8 - mField18 / 8];
		aHand->Draw(mField20[0] - (aHand->mWidth >> 1), (mField20[1] - (aHand->mHeight >> 1)) + 0x1e);
	}
	if (mFlying != NULL)
	{
		mApp->mPicRubberband->DrawLine(aMiddle - 0x36, 0x18c, (int) (mFlying->mX - 16.0f), (int) mFlying->mY);
		mApp->mPicRubberband->DrawLine(aMiddle + 0x36, 0x18c, (int) (mFlying->mX + 16.0f), (int) mFlying->mY);
		int y = (int) mFlying->mY;
		mApp->mPicRubberband->DrawLine((int) (mFlying->mX + 16.0f), y, (int) (mFlying->mX - 16.0f), y);
		if (mFlying->mY < 388.0f)
			mFlying = NULL;
	}
	anOffset.mY = -anOffset.mY;
	AddDrawOffset(anOffset);
}

// 0x401d30: the egg pulled back 25 pixels against the aim, and the band's points
// port: the original takes the egg's +0x18 as a float[2]; the x87 code works in 53 bits and keeps the offsets
// unrounded, so this works in double
void Launcher::PlaceEgg(float& theX, float& theY)
{
	AngleTable* anAngles = mApp->mAngles;
	double aX = (double) anAngles->mDY[mAngle] * 0.0 - (double) anAngles->mDX[mAngle] * 25.0;
	float aY = (float) ((double) anAngles->mDY[mAngle] * 25.0 + (double) anAngles->mDX[mAngle] * 0.0);
	theX = (float) ((double) mSlingX + aX);
	double aBack = (double) mSlingY - (double) aY;
	theY = (float) aBack;
	theY = (float) ((double) mFieldAac + aBack);
	mField30[0] = (int) ((double) mSlingX - aX * 0.4);
	mField30[1] = (int) ((double) aY * 0.4 + (double) mSlingY);
	int anAngle = mAngle;
	if (anAngle >= 0x168)
		anAngle = anAngle - 0x168;
	if (anAngle < 0)
		anAngle = anAngle + 0x168;
	mField38[0] = (int) ((double) anAngles->mDY[anAngle] * 0.0 - (double) anAngles->mDX[anAngle] * 10.0);
	mField38[1] = mFieldAac - (int) ((double) anAngles->mDX[anAngle] * 0.0 + (double) anAngles->mDY[anAngle] * 10.0);
}

// 0x401e40: aims at a point no lower than 10 above the slingshot's middle; the hand turns with the aim
void Launcher::Aim(int theX, int theY)
{
	int aTop = mSlingY - 10;
	mAimX = theX;
	mAimY = theY;
	if (aTop < theY)
		theY = aTop;
	int anAngle = mApp->mAngles->AngleTo(mSlingX, mSlingY, theX, theY);
	mAngle = anAngle;
	anAngle = anAngle & ~1;
	mField18 = anAngle;
	if (anAngle > 0x5a)
		anAngle = (anAngle - 0x11e) >> 1;
	else
		anAngle = (anAngle >> 1) + 0x25;
	mField18 = anAngle;
	if (anAngle < 0)
		mField18 = 0;
	if (mField18 > 0x47)
		mField18 = 0x47;
	if (mLoaded != NULL)
		PlaceEgg(mLoaded->mX, mLoaded->mY);
}
