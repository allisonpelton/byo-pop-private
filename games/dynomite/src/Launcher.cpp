#include "Launcher.h"
#include "AngleTable.h"
#include "Dynomite.h"
#include "Egg.h"
#include "FossilizedApp.h"
#include "PortCompat.h"

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
	mFieldA4c = x;
	mFieldA60 = 0;
	mFieldA64 = 0;
	mFieldA68 = 0;
	mFieldA70 = 0;
	mFrozen = false;
	mHeld = NULL;
	mFieldAac = 0;
	mFieldA54 = x;
	mFieldA58 = 0x1b0;
	mFieldA50 = 0x17e;
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
