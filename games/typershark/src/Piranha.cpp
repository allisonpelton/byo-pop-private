#include "Piranha.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "Res.h"
#include <stdlib.h>

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructor): 0x401730 (own-part destructor), 0x4021b0, 0x4021c0, 0x402200, 0x403110,
// 0x406580, 0x40e040 (jump to the virtual functions below or to GameObject's)

// 0x40fca0: GameObject (0x4611b0) and Enemy (0x40a3f0) first
Piranha::Piranha(WinWSApp* theApp, int theType) :
	Enemy(theApp),
	mPlayedSound(false),
	mFlag11(false),
	mFlag12(false),
	mType(0),
	mSavedVelX(-1.0),
	mEntered(false),
	mFrameOffset(0)
{
	mTextColor = Color(0, 0, 0);
	mFrameDelay = 5;
	mState = 0;
	mNumFrames = 20;
	mPlayedSound = false;
	mFlag12 = false;
	mSavedVelX = -1.0;
	mFlag11 = false;
	mFrame = rand() % mNumFrames;			// the C library's generator, not the game's MTRand
	mType = theType;
	mFlagD0 = false;
	mEntered = false;
	mFrameWidth = mApp->mImages[IMAGE_PIRANHA_BASIC_SWIM]->GetWidth() / 20;
	mFrameHeight = mApp->mImages[IMAGE_PIRANHA_BASIC_SWIM]->GetHeight();
	mImages[0] = mApp->mImages[IMAGE_PIRANHA_BASIC_SWIM];
	mImages[2] = mApp->mImages[IMAGE_PIRANHA_BASIC_DEATH];
	mImages[3] = mApp->mPiranhaImages[0][3];
	mImages[1] = mApp->mImages[IMAGE_PIRANHASHOCK];
	mTextOffsetX = 0;
	mTextOffsetY = 6;
	mFont = mApp->mFontCourierFinalDraft15Bold;
	mImage = mApp->mImages[IMAGE_PIRANHA_BASIC_SWIM];
	mFrameOffset = 0;
}

Piranha::~Piranha()
{
}

// 0x401770
bool Piranha::IsSwimming()
{
	return mState == 0 && mVisible;
}

// 0x40a6a0: one tick. State 0 swims in from the right (speeding up, a dash at the right edge) and leaves at the left,
// where SetState(1) turns it round; 1 comes back to the right; 3 falls up; 4 is chased off; 5 flashes
void Piranha::Update()
{
	IncTick();
	if (mTick % mFrameDelay == 0)
	{
		mFrame++;
		if (mFrame >= mNumFrames)
		{
			if (mState == 3)
			{
				mFrame = 9;
				if (!mFlag18)
				{
					mFlag18 = true;
					mEvents.push_back(6);
				}
			}
			else
				mFrame = 0;
		}
		mFrameOffset = mFrame * mFrameWidth;
	}

	if (mState == 0)
	{
		if (mX < 40.0)
			mVelX += 0.05;

		if (mSavedVelX == -1.0)
		{
			int aWidth = mApp->mWidth;
			if (!((double)aWidth < mX) && (double)(aWidth - (mFrameWidth >> 1)) <= mX)
			{
				// the dash at the right edge: remember the speed, go slower at once, then ease back to it
				mSavedVelX = mVelX;
				mVelX = 1.5;
			}
		}

		if (mVelX > mSavedVelX && mSavedVelX != -1.0)
		{
			mVelX -= 0.03;
			if (mVelX < mSavedVelX)
				mVelX = mSavedVelX;
		}

		if ((double)(-mFrameWidth) > mX)
		{
			// gone off the left edge: turn round
			SetState(1);
			mEvents.push_back(7);
			mEvents.push_back(4);
			mNumFrames = 20;
			mFrame = 0;
			mX = (double)(-mFrameWidth);
			mVelX = 5.0;
			mFrameWidth = mImages[3]->GetWidth() / mNumFrames;
			mFrameHeight = mImages[3]->GetHeight();
		}
		else
			mX -= mVelX;
	}
	else if (mState == 4)
	{
		mVelX += 0.075;
		mX -= mVelX;
		if ((double)(-mFrameWidth) > mX)
			mEvents.push_back(5);
	}
	else if (mState == 1 && !mEntered)
	{
		mX += mVelX;
		if (!(mX < 0.0) && !mFlagD0)
		{
			if (!mPlayedSound)
			{
				mApp->PlaySample(SOUND_PLAYERDIES);
				mPlayedSound = true;
			}
			if (!(mX < 20.0))
			{
				mEvents.push_back(2);
				mFlagD0 = true;
				mEntered = true;
			}
		}
		if ((double)mApp->mWidth <= mX)
			mEvents.push_back(5);
	}
	else if (mState == 3)
	{
		mY -= mVelY;
		if (mY < -70.0)
			mEvents.push_back(5);
	}
	else if (mState == 5)
	{
		if (mField2c > 0)
		{
			if (mTick % mField28 == 0)
			{
				mFlag30 = !mFlag30;
				mField2c--;
			}
			if (mFlag30)
			{
				mFrame = 0;
				mFrameHeight = mApp->mImages[IMAGE_PIRANHASHOCK]->GetHeight();
				mFrameWidth = mApp->mImages[IMAGE_PIRANHASHOCK]->GetWidth();
				mImage = mImages[1];
				mFrameOffset = 0;
				return;
			}
			mFrameHeight = mApp->mImages[IMAGE_PIRANHA_BASIC_SWIM]->GetHeight();
			mFrameWidth = mApp->mImages[IMAGE_PIRANHA_BASIC_SWIM]->GetWidth() / 20;
			mImage = mImages[0];
			mFrameOffset = mFrame * mFrameWidth;
			return;
		}
		mVelY = 1.0;
		mFrameDelay = 3;
		mNumFrames = 10;
		SetState(3);
	}
}

// 0x402360: the picture frame, then the word over it (not while the board is paused, and only in states 0 and 4)
void Piranha::Draw(Graphics* g)
{
	if (mVisible && mState != 2)
	{
		GameObject::Draw(g);
		Rect aSrc(mFrameOffset, 0, mFrameWidth, mFrameHeight);
		g->DrawImage(mImage, (int)mLastX, (int)mLastY, aSrc);
		g->SetFont(mFont);
		Board* aBoard = mApp->mBoard;
		if ((aBoard == NULL || !aBoard->mPaused) && (mState == 0 || mState == 4))
		{
			g->SetColor(mTextColor);
			Font* aFont = g->GetFont();
			double aCenterX = (double)(mFrameWidth >> 1) + (double)mTextOffsetX + mLastX;
			int aY = (int)((double)mTextOffsetY + mLastY + 28.0);
			int aWidth = aFont->StringWidth(mWord);
			int aX = (int)(aCenterX - (double)(aWidth >> 1) - 5.0);
			g->DrawString(mWord, aX, aY);
		}
	}
}

// 0x404950: the pictures and sizes of a state; the shocked state (5) also plays sample 0x25
void Piranha::SetState(int theState)
{
	if (theState == 6)
		return;
	mState = theState;
	if (theState != 0)
		mFrame = 0;

	if (theState == 3)
	{
		mImage = mImages[2];
		mFrameHeight = mImage->GetHeight();
		mFrameWidth = mImage->GetWidth() / 10;
		mFrameDelay = 5;
		mVelY = 1.5;
	}
	else if (theState == 2)
	{
		mNumFrames = 20;
		mFrameWidth = mImage->GetWidth() / 20;
		mFrameHeight = mImage->GetHeight();
	}
	else if (theState == 5)
	{
		mImage = mApp->mImages[IMAGE_PIRANHASHOCK];
		mFrame = 0;
		mFlag30 = true;
		mField28 = 7;
		mField2c = 6;
		if (!mFlag50)
			mApp->PlaySample(SOUND_PIRANHADIES1);
		mFrameHeight = mImage->GetHeight();
		mFrameWidth = mImage->GetWidth();
	}
	else
	{
		if (mState == 1)
			mImage = mImages[3];
		else if (mState == 4)
			mImage = mImages[0];
	}
	mFrameOffset = mFrame * mFrameWidth;
}
