#include "BackgroundFish.h"
#include "WinWSApp.h"
#include "Graphics.h"
#include "Image.h"
#include "MTRand.h"
#include "platform/GameOptions.h"

using namespace Sexy;

// 0x405fc0: a new fish at the left edge (x = -20), at a random height (100 or more from the top; the arithmetic is
// unsigned), with a random speed (0.1 to 0.7). The speed picks the picture (colour) and how fast the fish rises. The
// picture index starts at 3 (the gold fish): the test the original makes of the draw's low bit is true for both values
// of the bit, but the draw is kept
BackgroundFish::BackgroundFish(WinWSApp* theApp) :
	mApp(theApp),
	mImage(NULL),
	mFollower(false)
{
	mNumFrames = 20;
	mVelY = 0.0;
	int aType = 0;
	unsigned int aRand = (unsigned int)gGameRand->NextNoAssert();
	if (((aRand & 1) == 0) || ((aRand & 1) == 1))
		aType = 3;
	mFrameHeight = 20;
	mFrameWidth = 20;
	mX = -20.0;
	// unsigned arithmetic: the divisor is height - frame height - 0x5a
	unsigned int aY = (unsigned int)gGameRand->NextNoAssert() % (unsigned int)((mApp->mHeight - mFrameHeight) - 0x5a) + 100;
	mY = (double)aY;
	unsigned int aSpeed = (unsigned int)gGameRand->NextNoAssert() % 6000 + 1000;
	mVelX = (double)aSpeed * 0.0001;
	if (0.3 <= mVelX)
	{
		if (0.5 <= mVelX)
		{
			if (!(mVelX <= 0.7))
			{
				// no picture is chosen (the speed is below 0.7 in practice, so this is never reached)
				mFrame = 0;
				return;
			}
			mImage = mApp->mColorizedFish[aType + 2];
			mVelY = 0.3;
		}
		else
		{
			mImage = mApp->mColorizedFish[aType + 1];
			mVelY = 0.2;
		}
	}
	else
	{
		mImage = mApp->mColorizedFish[aType];
		mVelY = 0.05;
	}
	mFrame = 0;
}

// 0x401df0: a fish that follows another one: the speeds, the size, the height and the picture are the template's. It
// is put left of the screen by the frame width up to twice that, and a random amount (below the frame height) above or
// below the template. The original computes the x as -(random % width + width) in unsigned arithmetic and converts that
// to a double, which gives a number near 4.29e9 (the board then removes the fish at its first tick, as it is right of
// the screen); kept, and mended by the fixes mod's "schools" toggle (mods/fixes/mod.json)
BackgroundFish::BackgroundFish(WinWSApp* theApp, BackgroundFish* theTemplate) :
	mApp(theApp),
	mImage(NULL),
	mFollower(true)
{
	mNumFrames = 20;
	mVelY = theTemplate->mVelY;
	mVelX = theTemplate->mVelX;
	mFrameWidth = theTemplate->GetFrameWidth();
	mFrameHeight = theTemplate->GetFrameHeight();
	mY = theTemplate->mY;
	unsigned int aWidth = (unsigned int)mFrameWidth;
	mImage = theTemplate->mImage;
	unsigned int anOffset = (unsigned int)gGameRand->NextNoAssert() % aWidth + aWidth;
	unsigned int aX = 0u - anOffset;
	if (Platform::FixOn("schools"))
		mX = -(double)anOffset;		// fixes mod: a follower swims in behind its leader
	else
		mX = (double)aX;
	if (((unsigned int)gGameRand->NextNoAssert() & 1) == 0)
	{
		unsigned int aDY = (unsigned int)gGameRand->NextNoAssert() % (unsigned int)mFrameHeight;
		mY = mY - (double)aDY;
	}
	else
	{
		unsigned int aDY = (unsigned int)gGameRand->NextNoAssert() % (unsigned int)mFrameHeight;
		mY = (double)aDY + mY;
	}
	mFrame = 0;
}

BackgroundFish::~BackgroundFish()
{
}

// 0x401f40 slot 1: the frame advances every 10 ticks; the fish swims right and rises
void BackgroundFish::Update()
{
	mTick++;
	if (mTick % 10 == 0)
	{
		mFrame++;
		if (mFrame >= mNumFrames)
			mFrame = 0;
	}
	mX = mVelX + mX;
	mY = mY - mVelY;
}

// 0x402d70 slot 2: the frame of the picture at the position (no history is recorded)
void BackgroundFish::Draw(Graphics* g)
{
	Rect aSrc(mFrame * mFrameWidth, 0, mFrameWidth, mFrameHeight);
	g->DrawImage(mImage, (int)mX, (int)mY, aSrc);
}
