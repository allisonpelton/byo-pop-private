#include "StealthRedShark.h"
#include "WinWSApp.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructors): 0x4031d0 (Draw: StealthShark's), 0x40c090 (TypeLetter:
// RedShark's), 0x40f860 (Update: StealthShark's), 0x411d80, 0x411d90, 0x411da0 (SetVelX: StealthShark's), 0x411dc0,
// 0x411dd0, 0x411de0 (IsSwimming: BlackShark's), 0x411df0, 0x411f30, 0x411f40, 0x411ea0 (part destructor), 0x411e00
// (deleting destructor), 0x411e30 (destructor of the whole object)

// 0x411b00: Shark (0x4101d0), then the RedShark part (0x411400) and the StealthShark part (0x410770), then its own
StealthRedShark::StealthRedShark(WinWSApp* theApp) :
	Shark(theApp),
	RedShark(theApp),
	StealthShark(theApp)
{
	mFlag19 = true;
	mTypedColor = Color(255, 255, 255);
	mTextColor = Color(0, 0, 0);
	mStealthImages.resize(6);
	mStealthImages[0] = mApp->mSharkImages[10][0];
	mStealthImages[2] = mApp->mSharkImages[10][2];
	mStealthImages[3] = mApp->mSharkImages[1][3];
	mImage = mStealthImages[0];
}

StealthRedShark::~StealthRedShark()
{
}

// 0x406970: as StealthBlackShark::SetState (BlackShark::SetState underneath, the picture chosen by the final state)
void StealthRedShark::SetState(int theState)
{
	if (theState == 6)
	{
		if (mState != 0 && mState != 6)
			return;
	}
	else if (theState == 5)
	{
		if (mState == 4)
			return;
	}

	BlackShark::SetState(theState);
	if (mState == 0)
	{
		if (mFlag19)
			mImage = mStealthImages[0];
		else
			mImage = mImages[0];
	}
	else if (mState == 3)
	{
		if (mFlag19)
			mImage = mStealthImages[2];
		else
			mImage = mImages[2];
	}
	mFrameOffset = mFrame * mFrameWidth;
}
