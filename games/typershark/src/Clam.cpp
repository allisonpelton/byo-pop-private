#include "Clam.h"
#include "WinWSApp.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructor): 0x40e350, 0x40e360, 0x40e370, 0x40e380, 0x40e3e0, 0x40e3f0
// (jump to the virtual functions below or to the Enemy level), 0x40e390 (deleting destructor), 0x40e3d0 (part
// destructor)

// 0x40e100: GameObject (0x4611b0) and Enemy (0x40a3f0) first. The first frame is drawn from the game's generator. All
// five pictures of the creature are the one sheet of clams
Clam::Clam(WinWSApp* theApp, int theType) :
	Enemy(theApp),
	mType(0),
	mSourceY(0),
	mOpened(false),
	mPulseUp(false)
{
	mFrameDelay = 4;
	mType = theType;
	mState = 0;
	mNumFrames = 10;
	mFrame = gGameRand->NextNoAssert() % mNumFrames;
	mOpened = false;
	mTextColor = Color(255, 255, 255);
	mPulseUp = true;
	mFrameWidth = mApp->mImages[IMAGE_CLAMS]->GetWidth() / 10;
	mFrameHeight = mApp->mImages[IMAGE_CLAMS]->GetHeight() / 4;
	mImages[4] = mApp->mImages[IMAGE_CLAMS];
	mImages[0] = mApp->mImages[IMAGE_CLAMS];
	mImages[1] = mApp->mImages[IMAGE_CLAMS];
	mImages[2] = mApp->mImages[IMAGE_CLAMS];
	mImages[3] = mApp->mImages[IMAGE_CLAMS];
	if (mType == 0)
		mSourceY = mFrameHeight << 1;
	else if (mType == 1)
		mSourceY = mFrameHeight;
	else if (mType == 2)
		mSourceY = mFrameHeight * 3;
}

Clam::~Clam()
{
}

// 0x40a2d0 slot 1: every mFrameDelay ticks the frame moves. State 1 (opening) counts up to the last frame, stays
// there and reports event 1; state 2 (closing) counts down to frame 0 and reports event 2 (again at every step while
// the clam stays in state 2, as the original does); states 0, 3 and 4 pulse between frame 0 and the next to last
void Clam::Update()
{
	IncTick();
	if (mTick % mFrameDelay != 0)
		return;

	if (mState != 4 && mState != 0)
	{
		if (mState == 1)
		{
			if (mOpened)
				return;
			mFrame++;
			if (mFrame < mNumFrames)
				return;
			mFrame = mNumFrames - 1;
			mOpened = true;
			mEvents.push_back(1);
			return;
		}
		else if (mState == 2)
		{
			mFrame--;
			if (mFrame >= 0)
				return;
			mFrame = 0;
			mEvents.push_back(2);
			return;
		}
		else if (mState != 3)
			return;
	}

	if (!mPulseUp)
	{
		mFrame--;
		if (mFrame < 0)
		{
			mFrame = 1;
			mPulseUp = true;
		}
	}
	else
	{
		mFrame++;
		if (mFrame >= mNumFrames)
		{
			mFrame = mNumFrames - 2;
			mPulseUp = false;
		}
	}
}

// 0x403660 slot 2: the frame from the sheet (the first row in states 0 and 4, the row of the kind in 1 to 3), at the
// position without recording the history; in state 4 the word as well, 20 right and 41 below the position. A state
// above 4 would take the source y from the Graphics pointer, an uninitialised value: it never happens
void Clam::Draw(Graphics* g)
{
	int aSrcY = 0;
	switch (mState)
	{
	case 0:
	case 4:
		aSrcY = 0;
		break;
	case 1:
	case 2:
	case 3:
		aSrcY = mSourceY;
		break;
	}
	Rect aSrc(mFrame * mFrameWidth, aSrcY, mFrameWidth, mFrameHeight);
	g->DrawImage(mImages[mState], (int)mX, (int)mY, aSrc);
	if (mState == 4)
	{
		g->SetFont(mApp->mFontMyriadCondensed14Bold);
		g->SetColor(mTextColor);
		g->DrawString(mWord, (int)(mX + 20.0), (int)(mY + 41.0));
	}
}

// 0x40a270 slot 7: state 1 starts the opening (and a clam that is not of kind 0 reports event 3), state 2 the closing
void Clam::SetState(int theState)
{
	mState = theState;
	if (mState == 1)
	{
		mFrame = 0;
		mPulseUp = true;
		if (mType != 0)
			mEvents.push_back(3);
	}
	else if (mState == 2)
	{
		mOpened = false;
		mPulseUp = false;
	}
}
