#include "Bubble.h"
#include "WinWSApp.h"
#include "Graphics.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// 0x461d50: the three pictures, then four draws from the game's generator in this order: which picture (mFrame, 0 to
// 2), x (20 to 59), y (400 to 424) and the rising speed (1.0 to 3.999 in steps of 0.001). The bubble is visible at once
Bubble::Bubble(WinWSApp* theApp)
{
	mBubbleImages[0] = theApp->mImages[IMAGE_BUBBLE1];
	mBubbleImages[1] = theApp->mImages[IMAGE_BUBBLE2];
	mBubbleImages[2] = theApp->mImages[IMAGE_BUBBLE3];
	unsigned int aPicture = (unsigned int)gGameRand->NextNoAssert() % 3;
	mVisible = true;
	mFrame = (int)aPicture;
	mX = (double)((unsigned int)gGameRand->NextNoAssert() % 0x28) + 20.0;
	mY = (double)((unsigned int)gGameRand->NextNoAssert() % 0x19) + 400.0;
	mVelY = (double)((unsigned int)gGameRand->NextNoAssert() % 3000) * 0.001 + 1.0;
}

Bubble::~Bubble()
{
}

// 0x461140 slot 1: every 200 ticks the bubble drifts 0.125 to the left, not at all, or to the right (one draw from the
// generator); it always rises by mVelY
void Bubble::Update()
{
	mTick++;
	if (mTick % 200 == 0)
		mX = (double)((int)((unsigned int)gGameRand->NextNoAssert() % 3) - 1) * 0.125 + mX;
	mY = mY - mVelY;
}

// 0x461eb0 slot 2: the picture at the recorded position
void Bubble::Draw(Graphics* g)
{
	GameObject::Draw(g);
	g->DrawImage(mBubbleImages[mFrame], (int)mLastX, (int)mLastY);
}
