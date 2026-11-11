#include "Shark.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructor): 0x4011b0 (own-part destructor), 0x402210, 0x402220,
// 0x402240, 0x402260, 0x403120, 0x406590, 0x40e050 (jump to the virtual functions below or to GameObject's)

// 0x4101d0: GameObject (0x4611b0) and Enemy (0x40a3f0) first
Shark::Shark(WinWSApp* theApp) :
	Enemy(theApp),
	mFlashCount(0),
	mFlashPeriod(5),
	mSavedVelX(-1.0),
	mFlashed(false),
	mEntered(false),
	mFrameOffset(0)
{
	mTextOffsetY = 0;
	mTextOffsetX = 11;
	mFrameDelay = 8;
	mState = 0;
	mBiteEventSent = false;
	mNumFrames = 20;
	mPlayerDiesSoundPlayed = false;
	mFrame = gGameRand->NextNoAssert() % mNumFrames;
	mTextColor = Color(0, 0, 0);
	mTypedColor = Color(255, 255, 100);
	mFrameWidth = mApp->mImages[IMAGE_SHARK_BASIC_SWIM]->GetWidth() / 20;
	mFrameHeight = mApp->mImages[IMAGE_SHARK_BASIC_SWIM]->GetHeight();
	mImages[0] = mApp->mSharkImages[0][0];
	mImages[3] = mApp->mSharkImages[0][3];
	mImages[2] = mApp->mSharkImages[0][2];
	mImages[4] = mApp->mSharkImages[0][4];
	mImages[1] = mApp->mImages[IMAGE_SHARKSHOCK];
	mImage = mImages[0];
	mFrameOffset = 0;
}

// port: nothing to free here; the compiler's destructor chain does the rest
Shark::~Shark()
{
}

// 0x401200
bool Shark::IsSwimming()
{
	return mState == 0 || mState == 6;
}

// 0x40b930: one tick. States 0 and 6 swim left (a dash at the right edge, a turn at the left edge into state 1), 1
// comes back from the left, 3 falls up, 4 is chased off, 5 flashes the shock picture
void Shark::Update()
{
	IncTick();
	if (mTick % mFrameDelay == 0 && mState != 5)
	{
		mFrame++;
		if (mFrame >= mNumFrames)
		{
			if (mState == 3)
			{
				mFrame = 5;
				if (!mDeathFinished)
				{
					mDeathFinished = true;
					mEvents.push_back(6);
				}
			}
			else
				mFrame = 0;
		}
		mFrameOffset = mFrame * mFrameWidth;
	}

	if (mState == 0 || mState == 6)
	{
		if (mX <= 40.0)
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
			mVelX -= 0.009;
			if (mVelX < mSavedVelX)
				mVelX = mSavedVelX;
		}

		mX -= mVelX;
		if ((double)(-mFrameWidth) >= mX)
		{
			// gone off the left edge: turn round
			SetState(1);
			mEvents.push_back(7);
			mEvents.push_back(4);
			mNumFrames = 20;
			mFrame = 0;
			mX = (double)(-mFrameWidth);
			mVelX = 5.0;
			mFrameWidth = mImages[3]->GetWidth() / 20;
			mFrameHeight = mImages[3]->GetHeight();
			return;
		}

		if (mFlashCount > 0 && mTick % mFlashPeriod == 0)
		{
			mFlashed = !mFlashed;
			mFlashCount--;
			if (mFlashCount == 0)
			{
				SetState(0);
				return;
			}
			if (mFlashed)
				mImage = mImages[4];
			else
				mImage = mImages[0];
		}
	}
	else if (mState == 1 && !mEntered)
	{
		mX += mVelX;
		if (!mBiteEventSent)
		{
			if (mX >= -70.0)
			{
				if (!mPlayerDiesSoundPlayed)
				{
					mApp->PlaySample(SOUND_PLAYERDIES);
					mPlayerDiesSoundPlayed = true;
				}
				if (mX >= -110.0)
				{
					mEvents.push_back(2);
					mBiteEventSent = true;
					mEntered = true;
				}
			}
		}
		if ((double)mApp->mWidth <= mX)
			mEvents.push_back(5);
	}
	else
	{
		if (mState == 5)
		{
			if (mShockTogglesLeft <= 0)
			{
				mVelY = 1.0;
				mFrameDelay = 4;
				mNumFrames = 6;
				SetState(3);
				return;
			}
			if (mTick % mShockPeriod == 0)
			{
				mShockShowing = !mShockShowing;
				mShockTogglesLeft--;
			}
			if (mShockShowing)
			{
				mFrameHeight = mApp->mImages[IMAGE_SHARKSHOCK]->GetHeight();
				mFrameWidth = mApp->mImages[IMAGE_SHARKSHOCK]->GetWidth();
				mImage = mApp->mImages[IMAGE_SHARKSHOCK];
				return;
			}
			mFrameHeight = mApp->mImages[IMAGE_SHARK_BASIC_SWIM]->GetHeight();
			mFrameWidth = mApp->mImages[IMAGE_SHARK_BASIC_SWIM]->GetWidth() / 20;
			mImage = mImages[0];
			return;
		}

		if (mState == 3)
		{
			mY -= mVelY;
			if (mY < -70.0)
				mEvents.push_back(5);
		}
		else if (mState == 4)
		{
			mVelX += 0.2;
			mX -= mVelX;
			if ((double)(-mFrameWidth) > mX)
				mEvents.push_back(5);
		}
	}
}

// 0x4024e0: the picture frame, then the word over it. Not while the board is paused, and only in states 0, 4 and 6.
// With letters typed the typed part is drawn in the typed font and colour, the rest after it, both placed so that the
// whole word is centred
void Shark::Draw(Graphics* g)
{
	if (!mVisible)
		return;

	GameObject::Draw(g);
	Rect aSrc(mFrameOffset, 0, mFrameWidth, mFrameHeight);
	g->DrawImage(mImage, (int)mLastX, (int)mLastY, aSrc);

	int aCenterX = (mFrameWidth >> 1) + mTextOffsetX;
	Board* aBoard = mApp->mBoard;
	if (aBoard != NULL && aBoard->mPaused)
		return;
	if (mState != 0 && mState != 4 && mState != 6)
		return;

	if (mNumTyped > 0)
	{
		g->SetFont(mTypedFont);
		g->SetColor(mTypedColor);
		int aY = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		double aX = (double)aCenterX + mLastX;
		int aTypedX = (int)(aX - (double)(mTypedFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mTyped, aTypedX, aY);

		g->SetFont(mFont);
		g->SetColor(mTextColor);
		int aY2 = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		double aTypedWidth = (double)mFont->StringWidth(mTyped);
		int aRemainingX = (int)((aTypedWidth + mLastX + (double)aCenterX) - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mRemaining, aRemainingX, aY2);
	}
	else
	{
		g->SetFont(mFont);
		g->SetColor(mTextColor);
		int aY = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mLastY + 5.0);
		int aWordX = (int)(((double)aCenterX + mLastX) - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mWord, aWordX, aY);
	}
}

// 0x404c20: the pictures and sizes of a state. State 6 (hit) only takes a swimming shark, state 5 (shocked) no chased
// one; the shocked state also plays sample 3 (the shark dies)
void Shark::SetState(int theState)
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

	mState = theState;
	if (theState == 0)
		mImage = mImages[0];
	else
	{
		mFrame = 0;
		if (theState == 6)
		{
			mFlashed = true;
			mFlashPeriod = 5;
			mFlashCount = 10;
			mImage = mImages[0];
			mShockShowing = false;
			mShockTogglesLeft = 0;
			mFrameOffset = mFrame * mFrameWidth;
			return;
		}

		if (theState == 5)
		{
			mFrameOffset = 0;
			mFrame = 0;
			mShockShowing = true;
			mShockPeriod = 7;
			mShockTogglesLeft = 6;
			if (!mKilledByEmp)
				mApp->PlaySample(SOUND_SHARKDIES);
			mFrameHeight = mImages[1]->GetHeight();
			mFrameWidth = mImages[1]->GetWidth();
			mImage = mApp->mImages[IMAGE_SHARKSHOCK];
		}
		else if (theState == 3)
		{
			mFrameHeight = mApp->mImages[IMAGE_SHARK_BASIC_DEATH]->GetHeight();
			mFrameWidth = mApp->mImages[IMAGE_SHARK_BASIC_DEATH]->GetWidth() / 6;
			mFrameDelay = 6;
			mVelY = 1.5;
			mImage = mImages[2];
		}
		else if (theState == 1)
		{
			mFrameHeight = mImages[3]->GetHeight();
			mFrameWidth = mImages[3]->GetWidth() / 20;
			mImage = mImages[3];
		}
		else if (theState == 4)
			mImage = mImages[0];
	}

	mFlashed = false;
	mFlashCount = 0;
	if (theState != 5)
	{
		mShockShowing = false;
		mShockTogglesLeft = 0;
	}
	mFrameOffset = mFrame * mFrameWidth;
}
