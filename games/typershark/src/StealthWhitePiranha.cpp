#include "StealthWhitePiranha.h"
#include "WinWSApp.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructors): 0x402d60 (Draw: StealthPiranha's), 0x40c190 (Update:
// StealthPiranha's), 0x4121c0 (SetStealth: StealthPiranha's), 0x412180, 0x4121b0, 0x4121d0, 0x4121e0 (IsSwimming:
// WhitePiranha's), 0x412300, 0x412310, 0x412320 (TypeLetter: WhitePiranha's), 0x412260 (part destructor), 0x412200
// (deleting destructor), 0x411940 and 0x411950 (GetSpeed and SetVelX: StealthPiranha's)

// 0x411f50: Piranha (0x40fca0) first, then the StealthPiranha part (0x410530) and the WhitePiranha part (0x410c70),
// then its own
StealthWhitePiranha::StealthWhitePiranha(WinWSApp* theApp, int theType) :
	Piranha(theApp, theType),
	StealthPiranha(theApp, theType),
	WhitePiranha(theApp, theType)
{
	mFlag19 = true;
	mStealthImages.resize(6);
	mStealthImages[0] = mApp->mPiranhaImages[7][0];
	mStealthImages[2] = mApp->mPiranhaImages[7][2];
	mStealthImages[3] = mApp->mPiranhaImages[6][3];
	mImage = mStealthImages[0];
}

StealthWhitePiranha::~StealthWhitePiranha()
{
}

// 0x406a20: WhitePiranha::SetState (0x405e10), then by the state the piranha ended up in (not the argument: dying can
// leave it swimming) the picture, stealth or normal, and the frame size taken from it: 1/20 of the width for the swim
// pictures, 1/10 for the death picture
void StealthWhitePiranha::SetState(int theState)
{
	WhitePiranha::SetState(theState);
	if (mState == 0)
	{
		if (mFlag19)
			mImage = mStealthImages[0];
		else
			mImage = mImages[0];
		mFrameWidth = mImage->GetWidth() / 20;
		mFrameHeight = mImage->GetHeight();
	}
	else if (mState == 3)
	{
		if (mFlag19)
			mImage = mStealthImages[2];
		else
			mImage = mImages[2];
		mFrameWidth = mImage->GetWidth() / 10;
		mFrameHeight = mImage->GetHeight();
	}
	mFrameOffset = mFrame * mFrameWidth;
}
