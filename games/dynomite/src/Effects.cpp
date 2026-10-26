#include "Effects.h"
#include "AngleTable.h"
#include "Dynomite.h"
#include "FossilizedApp.h"
#include "Sprite.h"

using namespace Sexy;

// 0x4034b0
Effect::Effect()
{
	mType = 0;
}

// 0x4034c0
Effect::~Effect()
{
}

// 0x4b1930
void Effect::Draw()
{
}

// 0x4034d0
bool Effect::Update()
{
	return false;
}

// 0x403540
BabyEffect::BabyEffect()
{
	mType = 7;
}

// 0x403560: the reward sound as it leaves, and the board's +0x108 to 20
BabyEffect::~BabyEffect()
{
	mApp->mSndReward->Play();
	mApp->mBoard->mField108 = 0x14;
}

// 0x4035d0
void BabyEffect::Init(int x, int y, int theBaby, FossilizedApp* theApp)
{
	mApp = theApp;
	mX = (float) x;
	mBaby = theBaby;
	mVY = 0;
	mDelay = 0x32;
	mY = (float) y;
	mGravity = (float) y * 0.015151516f * 0.1f + 1.0f;
}

// 0x403620
bool BabyEffect::Update()
{
	if (mDelay < 1)
	{
		mY = mVY * 0.16666667f + mY;
		mVY = mGravity + mVY;
		if (650.0f < mY)
			return false;
	}
	else
		mDelay--;
	return true;
}

// 0x403700
void BabyEffect::Draw()
{
	Cell* aCell = mApp->mBabyCells[mBaby];
	aCell->Draw((int) mX - (aCell->mWidth >> 1), (int) mY - (aCell->mHeight >> 1));
}

// 0x415360
ConfettiEffect::ConfettiEffect()
{
	mType = 9;
}

// 0x415380
ConfettiEffect::~ConfettiEffect()
{
}

// 0x415390: a random colour, and a random push of up to 7 pixels in a random direction
void ConfettiEffect::Init(int x, int y, FossilizedApp* theApp)
{
	mX = (float) x;
	mApp = theApp;
	mY = (float) y;
	mColor = theApp->mRand.NextNoAssert() % 5;
	unsigned int anAngle = mApp->mRand.NextNoAssert() % 0x168;
	float aDist = (float) (mApp->mRand.NextNoAssert() & 7);
	mX = aDist * mApp->mAngles->mDX[anAngle] + mX;
	mY = aDist * mApp->mAngles->mDY[anAngle] + mY;
	mVX = aDist * 0.5f * mApp->mAngles->mDX[anAngle];
	mVY = aDist * 0.5f * mApp->mAngles->mDY[anAngle];
}

// 0x415450
bool ConfettiEffect::Update()
{
	mX = mVX + mX;
	mY = mVY + mY;
	mVY = mVY + 0.1f;
	if (610.0f < mY)
		return false;
	return true;
}

// 0x415510: where it was a frame ago, then where it is
void ConfettiEffect::Draw()
{
	float x = mX;
	float y = mY;
	Cell* aCell = mApp->mWowStarCells[mColor];
	aCell->Draw((int) (mX - mVX) - (aCell->mWidth >> 1), (int) (mY - mVY) - (aCell->mHeight >> 1));
	aCell = mApp->mWowStarCells[mColor];
	aCell->Draw((int) x - (aCell->mWidth >> 1), (int) y - (aCell->mHeight >> 1));
}
