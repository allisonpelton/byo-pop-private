#include "StealthBlackShark.h"
#include "WinWSApp.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructors): 0x4031c0 (Draw: StealthShark's), 0x40bf90 (TypeLetter:
// BlackShark's), 0x40f850 (Update: StealthShark's), 0x411930, 0x411940 (GetSpeed: StealthShark's), 0x411950 (SetVelX:
// StealthShark's), 0x411970, 0x411980, 0x411990 (IsSwimming: BlackShark's), 0x4119a0, 0x411ae0, 0x411af0, 0x411a50 (part
// destructor), 0x4119b0 (deleting destructor), 0x4119e0 (destructor of the whole object)

// 0x411710: Shark (0x4101d0), then the StealthShark part (0x410770) and the BlackShark part (0x410f50), then its own
StealthBlackShark::StealthBlackShark(WinWSApp* theApp) :
	Shark(theApp),
	StealthShark(theApp),
	BlackShark(theApp)
{
	mFlag19 = true;
	mStealthImages.resize(6);
	mStealthImages[0] = mApp->mSharkImages[9][0];
	mStealthImages[2] = mApp->mSharkImages[9][2];
	mStealthImages[3] = mApp->mSharkImages[2][3];
	mImage = mStealthImages[0];
}

StealthBlackShark::~StealthBlackShark()
{
}

// 0x4068c0: like StealthShark::SetState, with BlackShark::SetState underneath. Unlike StealthShark's, the picture is
// chosen by the state the shark ended up in, not by the argument (BlackShark::SetState(3) can leave it at 0)
void StealthBlackShark::SetState(int theState)
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
