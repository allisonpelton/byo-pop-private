#include "Thunderbolt.h"
#include "Dynomite.h"
#include "Egg.h"
#include "FossilizedApp.h"
#include "Launcher.h"
#include "Sprite.h"

using namespace Sexy;

// 0x407e00
Thunderbolt::Thunderbolt(FossilizedApp* theApp, Dynomite* theBoard)
{
	mApp = theApp;
	mBoard = theBoard;
	theApp->mSndThunderbolt->Play();
	mTime = 0xaf;
	mBomb = true;
	mFlip = 0xf;
	mColor = mBoard->mLauncher->mLoaded->mColor;
	mBoard->mLauncher->mLoaded->mColor = 0;
	mBoard->mLauncher->mLoaded->mKind = 0;
}

// 0x407e60
Thunderbolt::~Thunderbolt()
{
	mBoard->mLauncher->mLoaded->mColor = 0;
}

// 0x407e80
bool Thunderbolt::Update()
{
	mFlip--;
	if (mFlip < 1)
	{
		mBomb = mBomb != true;
		mFlip = 0xf;
		if (mBomb == true)
		{
			mBoard->mLauncher->mLoaded->mColor = 0;
			mTime--;
			return mTime >= 0;
		}
		mBoard->mLauncher->mLoaded->mColor = mColor;
	}
	mTime--;
	return mTime >= 0;
}

// 0x407f70: the bolt in the middle of the board while the egg shows its colour
void Thunderbolt::Draw()
{
	if (mBomb == false)
	{
		Pic* aPic = mApp->mPicBolt;
		aPic->Draw(0xff - (aPic->mWidth >> 1), 0x15e - (aPic->mHeight >> 1));
	}
}
