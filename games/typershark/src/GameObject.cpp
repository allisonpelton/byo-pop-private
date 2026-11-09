#include "GameObject.h"

using namespace Sexy;

// 0x4611b0: everything zero, except the frame size (+0xd0, +0xd4), which the subclass constructors set
GameObject::GameObject() :
	mDXIndex(0),
	mPrevX(0),
	mDYIndex(0),
	mPrevY(0),
	mX(0),
	mY(0),
	mLastX(0),
	mLastY(0),
	mState(0),
	mVelX(0),
	mVelY(0),
	mFrameWidth(0),
	mFrameHeight(0),
	mFrame(0),
	mFrameDelay(0),
	mNumFrames(0),
	mVisible(false),
	mTick(0)
{
	for (int i = 0; i < 6; i++)
	{
		mDXHistory[i] = 0;
		mDYHistory[i] = 0;
	}
}

GameObject::~GameObject()
{
}

// 0x461270 slot 2 (takes the Graphics* and ignores it): the position history. x and y are stored one after the other
void GameObject::Draw(Graphics* g)
{
	(void)g;
	RecordHistory();
}

void GameObject::RecordHistory()
{
	double aPrevX = mPrevX;
	mPrevX = mX;
	mDXHistory[mDXIndex] = mX - aPrevX;
	double aPrevY = mPrevY;
	mPrevY = mY;
	mDXIndex = (mDXIndex + 1) % 6;
	mDYHistory[mDYIndex] = mY - aPrevY;
	mLastX = mX;
	mLastY = mY;
	mDYIndex = (mDYIndex + 1) % 6;
}
