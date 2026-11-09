#include "StealthPiranha.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructor): 0x408530 (part destructor), 0x4085b0, 0x4085c0, 0x4085d0,
// 0x40e070 (jump to the functions below, Piranha's or GameObject's), 0x409ab0 (deleting destructor)

// 0x410530: Piranha (0x40fca0) first, then the StealthPiranha part
StealthPiranha::StealthPiranha(WinWSApp* theApp, int theType) :
	Piranha(theApp, theType),
	mBaseSpeed(1.6),
	mEventSent(false),
	mAlpha(50)
{
	mFlag19 = true;
	mTextColor = Color(0, 0, 0);
	mStealthImages.resize(6);
	mStealthImages[0] = mApp->mPiranhaImages[3][0];
	mStealthImages[2] = mApp->mPiranhaImages[3][2];
	mStealthImages[3] = mApp->mPiranhaImages[0][3];
	mImage = mStealthImages[0];
	mBaseSpeed = 1.6;
	mVelX = 4.0;
	mEventSent = false;
	mAlpha = 50;
}

StealthPiranha::~StealthPiranha()
{
}

// 0x404f60 (the same code as StealthShark's): switching it off shows the normal picture
void StealthPiranha::SetStealth(bool theStealth)
{
	mFlag19 = theStealth;
	if (!theStealth)
		mImage = mImages[0];
}

// 0x408480
void StealthPiranha::ResetSpeed()
{
	mVelX = mBaseSpeed;
}

// 0x40bfa0: Piranha::Update, then as StealthShark::Update: while stealthy it turns solid at x <= 400; once solid it
// eases mVelX down to mBaseSpeed (0.02 a tick, only in state 0); the first time x <= 610 it pushes event 8
void StealthPiranha::Update()
{
	Piranha::Update();
	bool aStealth = mFlag19;
	if (aStealth)
	{
		if (mX <= 400.0)
			SetStealth(false);
	}
	else
	{
		if (mVelX > mBaseSpeed && mState == 0)
		{
			mVelX = mVelX - 0.02;
			if (mVelX < mBaseSpeed)
				mVelX = mBaseSpeed;
		}
	}

	if (!mEventSent && mX <= 610.0)
	{
		mEventSent = true;
		mEvents.push_back(8);
	}
}

// 0x4027e0: Piranha::Draw with the word's colour at alpha mAlpha while stealthy. Unlike Piranha::Draw it leaves out
// mTextOffsetX when it centres the word
void StealthPiranha::Draw(Graphics* g)
{
	if (mVisible && mState != 2)
	{
		GameObject::Draw(g);
		Rect aSrc(mFrameOffset, 0, mFrameWidth, mFrameHeight);
		g->DrawImage(mImage, (int)mLastX, (int)mLastY, aSrc);
		g->SetFont(mFont);
		if (mState == 0 || mState == 4)
		{
			Board* aBoard = mApp->mBoard;
			if (aBoard == NULL || !aBoard->mPaused)
			{
				if (mFlag19)
					g->SetColor(Color(mTextColor.GetRed(), mTextColor.GetGreen(), mTextColor.GetBlue(), mAlpha));
				else
					g->SetColor(mTextColor);
				int aY = (int)((double)mTextOffsetY + mLastY + 28.0);
				double aCenterX = (double)(mFrameWidth >> 1) + mLastX;
				int aWidth = mFont->StringWidth(mWord);
				int aX = (int)(aCenterX - (double)(aWidth >> 1) - 5.0);
				g->DrawString(mWord, aX, aY);
			}
		}
	}
}

// 0x404ed0: Piranha::SetState (which ignores 6), then the picture of the new state, the stealth one while stealthy
void StealthPiranha::SetState(int theState)
{
	Piranha::SetState(theState);
	if (theState == 0)
	{
		if (mFlag19)
			mImage = mStealthImages[0];
		else
			mImage = mImages[0];
	}
	else if (theState == 3)
	{
		if (mFlag19)
			mImage = mStealthImages[2];
		else
			mImage = mImages[2];
	}
	mFrameOffset = mFrame * mFrameWidth;
}

// 0x401ab0 (shared with StealthShark): the speed is kept in mBaseSpeed and reaches mVelX at once only when solid
void StealthPiranha::SetVelX(double v)
{
	mBaseSpeed = v;
	if (!mFlag19)
		mVelX = v;
}

// 0x408500: not mVelX
double StealthPiranha::GetSpeed()
{
	return mBaseSpeed;
}
