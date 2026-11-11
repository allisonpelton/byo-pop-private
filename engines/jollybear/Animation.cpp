#include "Animation.h"

#include "Manager.h"

namespace jb
{

// 0x429e10 (mahjongchina), 0x4256d0 (mahjongjapan)
void Animation::SetImage(int theX, int theY, int theImage, int theFrameWidth)
{
	mY = theY;
	mX = theX;
	mImage = theImage;
	mFrameWidth = theFrameWidth;
	ImageInfo anInfo;
	GetImageInfo(theImage, anInfo);
	mHeight = anInfo.mHeight;
	mFrames = anInfo.mWidth / mFrameWidth;
}

// 0x429e60 (mahjongchina), 0x425720 (mahjongjapan)
void Animation::Setup(int thePeriod, int theMiddle, int theBelow, int theAbove, int theSwingDelay, int theCycles,
	int theDelay, bool theDrawDelayed)
{
	mBelow = theBelow;
	mAbove = theAbove;
	mPeriod = thePeriod;
	mCycles = theCycles;
	mMiddle = theMiddle;
	mSwingDelay = theSwingDelay;
	SetDelay(theDelay);
	mDrawDelayed = theDrawDelayed;
	mTarget = -1;
	int aFrame = Random(mFrames - 1);
	mFrame = aFrame;
	if (aFrame == theMiddle)
	{
		mFrame = aFrame - 1;
		mForward = true;
		Start();
		return;
	}
	if (aFrame < theMiddle)
	{
		mForward = true;
		Start();
		return;
	}
	mForward = false;
	Start();
}

// 0x429f10 (mahjongchina), 0x4257c0 (mahjongjapan)
void Animation::Start()
{
	mLast = 0;
	mRunning = true;
}

// 0x429f60 (mahjongchina), 0x425810 (mahjongjapan)
void Animation::SetPaused(bool thePaused)
{
	if (mPaused != thePaused)
	{
		mPaused = thePaused;
		UpdateHold();
		if (!mPaused)
			Invalidate();
	}
}

// 0x429f90 (mahjongchina), 0x425840 (mahjongjapan)
void Animation::UpdateHold()
{
	if (mLast != 0)
	{
		if (!mHolding)
		{
			if (mPaused)
			{
				mHoldStart = mOwner->ManagerTime();
				mHolding = true;
			}
			if (!mHolding)
				return;
		}
		if (!mPaused)
		{
			int aNow = mOwner->ManagerTime();
			mHolding = false;
			mLast = mLast + (aNow - mHoldStart);
		}
	}
}

// 0x429ff0 (mahjongchina), 0x4258a0 (mahjongjapan)
bool Animation::Update(int theClock)
{
	if (mRunning && !mPaused)
	{
		int i = 0;
		if (mLast == 0)
			mLast = theClock;
		if (mDelay < 1)
		{
			uint32_t anElapsed = theClock - mLast;
			if ((uint32_t)mPeriod <= anElapsed)
			{
				int aSteps = (int)(anElapsed / (uint32_t)mPeriod);
				if (0 < aSteps)
				{
					do
					{
						if (!mRunning || mDelay != 0)
							break;
						StepSwing();
						i = i + 1;
					} while (i < aSteps);
				}
				mLast = theClock;
				return true;
			}
		}
		else if ((uint32_t)mDelay <= (uint32_t)(theClock - mLast))
		{
			mDelay = 0;
			mLast = theClock;
		}
	}
	return false;
}

// 0x42a0e0 (mahjongchina), 0x425990 (mahjongjapan)
void Animation::Draw()
{
	if (mRunning && (mDelay < 1 || mDrawDelayed))
	{
		if (mFrame < 0)
			mFrame = 0;
		if (mFrames <= mFrame)
			mFrame = mFrames - 1;
		Rect aSrc;
		aSrc.top = 0;
		aSrc.left = mFrameWidth * mFrame;
		aSrc.right = mFrameWidth + aSrc.left;
		aSrc.bottom = mHeight;
		mOwner->DrawImage(mX, mY, mImage, &aSrc, true, 100, 100, mAlpha);
	}
}

// 0x42a1f0 (mahjongchina), 0x425aa0 (mahjongjapan)
void Animation::StepSwing()
{
	bool aForward = mForward;
	int aFrame;
	if (!aForward)
		aFrame = mFrame - 1;
	else
		aFrame = mFrame + 1;
	mFrame = aFrame;
	if (aFrame == mMiddle)
	{
		if (!aForward)
		{
			mTarget = mMiddle - mBelow;
			int aRandom = Random(mTarget + 1);
			mTarget = mTarget - aRandom;
		}
		else
		{
			mTarget = mMiddle + mAbove;
			int aRandom = Random(mFrames - mTarget);
			mTarget = mTarget + aRandom;
		}
		if (mForward)
		{
			EndCycle(-1);
			return;
		}
	}
	else if (aFrame == mTarget)
	{
		mForward = !aForward;
		mDelay = mSwingDelay;
	}
}

// 0x42a280 (mahjongchina), 0x425b20 (mahjongjapan)
bool Animation::EndCycle(int theFrame)
{
	if (mCycles != -1)
	{
		mCycles = mCycles - 1;
		if (mCycles == 0)
		{
			mRunning = false;
			if (theFrame != -1)
				mFrame = theFrame;
			return false;
		}
	}
	return true;
}

// 0x42a2d0 (mahjongchina), 0x425b70 (mahjongjapan)
void Animation::SetDelay(int theDelay)
{
	if (theDelay == -1)
		theDelay = Random(mMaxDelay - mMinDelay) + mMinDelay;
	mDelay = theDelay;
}

} // namespace jb
