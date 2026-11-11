#include "StealthShark.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructors): 0x408400 (part destructor), 0x4084a0, 0x4084b0, 0x4084c0,
// 0x4084d0, 0x4084e0, 0x4084f0, 0x408510, 0x408520, 0x40e020 (jump to the functions below, Shark's or GameObject's),
// 0x409a10 (deleting destructor), 0x409a40 (destructor of the whole object)

// 0x410770: Shark (0x4101d0) first, then the StealthShark part
StealthShark::StealthShark(WinWSApp* theApp) :
	Shark(theApp),
	mBaseSpeed(1.0),
	mEventSent(false),
	mAlpha(50)
{
	mStealthy = true;
	mTypedColor = Color(255, 255, 100);
	mTextColor = Color(0, 0, 0);
	mStealthImages.resize(6);
	mStealthImages[0] = mApp->mSharkImages[3][0];
	mStealthImages[2] = mApp->mSharkImages[3][2];
	mStealthImages[3] = mApp->mSharkImages[0][3];
	mImage = mStealthImages[0];
	mBaseSpeed = 1.0;
	mVelX = 3.0;
	mEventSent = false;
	mAlpha = 50;
}

StealthShark::~StealthShark()
{
}

// 0x404f60 (the same code as StealthPiranha's): switching it off shows the normal picture
void StealthShark::SetStealth(bool theStealth)
{
	mStealthy = theStealth;
	if (!theStealth)
		mImage = mImages[0];
}

// 0x408480
void StealthShark::ResetSpeed()
{
	mVelX = mBaseSpeed;
}

// 0x40c0a0: Shark::Update, then: while stealthy it turns solid at x <= 400; once solid it eases mVelX down to
// mBaseSpeed (0.02 a tick, only swimming); the first time x <= 600 it pushes event 8
void StealthShark::Update()
{
	Shark::Update();
	bool aStealth = mStealthy;
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

	if (!mEventSent && mX <= 600.0)
	{
		mEventSent = true;
		mEvents.push_back(8);
	}
}

// 0x4029a0: Shark::Draw with the word's colours at alpha mAlpha while stealthy (the state test comes before the
// paused test here, which makes no difference)
void StealthShark::Draw(Graphics* g)
{
	if (!mVisible)
		return;

	GameObject::Draw(g);
	Rect aSrc(mFrameOffset, 0, mFrameWidth, mFrameHeight);
	g->DrawImage(mImage, (int)mLastX, (int)mLastY, aSrc);

	int aCenterX = (mFrameWidth >> 1) + mTextOffsetX;
	if (mState != 0 && mState != 4 && mState != 6)
		return;
	Board* aBoard = mApp->mBoard;
	if (aBoard != NULL && aBoard->mPaused)
		return;

	if (mNumTyped > 0)
	{
		g->SetFont(mTypedFont);
		if (mStealthy)
			g->SetColor(Color(mTypedColor.GetRed(), mTypedColor.GetGreen(), mTypedColor.GetBlue(), mAlpha));
		else
			g->SetColor(mTypedColor);
		int aY = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		double aX = (double)aCenterX + mLastX;
		int aTypedX = (int)(aX - (double)(mTypedFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mTyped, aTypedX, aY);

		g->SetFont(mFont);
		if (mStealthy)
			g->SetColor(Color(mTextColor.GetRed(), mTextColor.GetGreen(), mTextColor.GetBlue(), mAlpha));
		else
			g->SetColor(mTextColor);
		int aY2 = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		double aTypedWidth = (double)mFont->StringWidth(mTyped);
		int aRemainingX = (int)((aTypedWidth + mLastX + (double)aCenterX) - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mRemaining, aRemainingX, aY2);
	}
	else
	{
		g->SetFont(mFont);
		if (mStealthy)
			g->SetColor(Color(mTextColor.GetRed(), mTextColor.GetGreen(), mTextColor.GetBlue(), mAlpha));
		else
			g->SetColor(mTextColor);
		int aY = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		int aWordX = (int)(((double)aCenterX + mLastX) - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mWord, aWordX, aY);
	}
}

// 0x404f90: Shark::SetState with the same refusals (6 only while swimming, 5 not while chased off), then the picture
// of the new state, the stealth one while stealthy
void StealthShark::SetState(int theState)
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

	Shark::SetState(theState);
	if (theState == 0)
	{
		if (mStealthy)
			mImage = mStealthImages[0];
		else
			mImage = mImages[0];
	}
	else if (theState == 3)
	{
		if (mStealthy)
			mImage = mStealthImages[2];
		else
			mImage = mImages[2];
	}
	mFrameOffset = mFrame * mFrameWidth;
}

// 0x401ab0 (shared with StealthPiranha): the speed is kept in mBaseSpeed and reaches mVelX at once only when solid
void StealthShark::SetVelX(double v)
{
	mBaseSpeed = v;
	if (!mStealthy)
		mVelX = v;
}

// 0x408500: not mVelX
double StealthShark::GetSpeed()
{
	return mBaseSpeed;
}
