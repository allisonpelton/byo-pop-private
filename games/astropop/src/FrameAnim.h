#ifndef __ASTROPOP_FRAMEANIM_H__
#define __ASTROPOP_FRAMEANIM_H__

// FrameAnim: the game's counter that steps through the frames of an animation (class of 0x40 bytes, constructor
// 0x424370; the buttons, menus, ships and effects all keep several). It counts updates; every mDelay updates (or the
// delay of the current frame) it moves the frame by mStep, forwards, backwards or back and forth, inside the frame
// range or an inner range after the first loop, and stops after mMaxLoops loops.

#include <vector>

namespace Sexy
{

class FrameAnim
{
public:
	int					mCounter;           // +0x00 updates so far
	int					mFrame;             // +0x04
	int					mNumFrames;         // +0x08 (0x10000 until SetNumFrames)
	int					mDelay;             // +0x0c updates per frame
	int					mMaxLoops;          // +0x10 0: no limit
	int					mLoopCount;         // +0x14
	int					mRangeMin;          // +0x18 the inner range used after the first loop (mUseRange)
	int					mRangeMax;          // +0x1c
	int					mDirection;         // +0x20 -1: backwards
	int					mStep;              // +0x24 frames per step
	bool				mStopped;           // +0x28 constructed true
	bool				mPingPong;          // +0x29
	bool				mReversing;         // +0x2a ping-pong phase; constructed true
	bool				mHoldLast;          // +0x2b stop at the end of a loop instead of counting loops
	bool				mDone;              // +0x2c (the constructor leaves it unset)
	bool				mFrameChanged;      // +0x2d set when a frame step happened; read and cleared by ConsumeFrameChanged
	bool				mUseRange;          // +0x2e
	std::vector<int>	mFrameDelays;       // +0x34 one delay per frame

	FrameAnim()         // 0x424370
	{
		mCounter = 0;
		mFrame = 0;
		mNumFrames = 0x10000;
		mDelay = 0;
		mMaxLoops = 0;
		mLoopCount = 0;
		mRangeMin = 0;
		mRangeMax = 0;
		mDirection = 1;
		mStep = 1;
		mStopped = true;
		mPingPong = false;
		mReversing = true;
		mHoldLast = false;
		mDone = false;
		mFrameChanged = true;
		mUseRange = false;
	}

	int GetCel() const { return (mNumFrames == 1) ? 0 : mFrame; }      // the cel the draws show (a counter of one frame shows the first)
	void SetFrame(int theFrame) { mFrame = theFrame; }                      // 0x40b0b0
	void SetStopped(bool theStopped) { mStopped = theStopped; }             // 0x40b0c0
	bool ConsumeFrameChanged()                                              // 0x40b120
	{
		if (!mFrameChanged)
			return false;
		mFrameChanged = false;
		return true;
	}
	void SetRange(int theMin, int theMax)                                   // 0x40b140
	{
		mUseRange = true;
		mRangeMin = theMin;
		mRangeMax = theMax;
	}
	void SetDirection(int theDirection)                                     // 0x40b160
	{
		mDirection = theDirection;
		if (theDirection < 0)
			mFrame = mNumFrames - 1;
	}
	void Reset()                                                            // 0x40b180
	{
		mStopped = false;
		mFrame = (mDirection < 0) ? mNumFrames - 1 : 0;
		mDone = false;
		mLoopCount = 0;
		mRangeMin = 0;
		mRangeMax = mNumFrames;
		mUseRange = false;
	}
	void SetFrameDelay(int theDelay, int theFrame)                          // 0x40e2a0
	{
		mFrameDelays[theFrame] = theDelay;
		mDelay = theDelay;
	}
	void SetDelay(int theDelay)                                             // 0x40e2c0 every frame gets it
	{
		mDelay = theDelay;
		for (size_t i = 0; i < mFrameDelays.size(); i++)
			mFrameDelays[i] = theDelay;
	}
	void SetNumFrames(int theNumFrames)                                     // 0x425800
	{
		mNumFrames = theNumFrames;
		mFrame = 0;
		if (mDirection < 0)
			mFrame = theNumFrames - 1;
		mFrameDelays.assign((size_t) theNumFrames, mDelay);
	}

	void Update()                                                           // 0x40e130
	{
		mCounter++;
		int aMin = 0;
		int aMax = mNumFrames;
		int aDelay;
		if (!mFrameDelays.empty() && mNumFrames > 1)
			aDelay = mFrameDelays[mFrame];
		else
			aDelay = mDelay;

		if (mUseRange)
		{
			if ((mLoopCount >= 1 && !mPingPong) || (mPingPong && mLoopCount >= 2))
			{
				aMin = mRangeMin;
				aMax = mRangeMax;
			}
		}

		bool aDone = false;
		if (!mStopped && !mDone && (mCounter % aDelay) == 0 && !(mMaxLoops != 0 && mLoopCount > mMaxLoops))
		{
			mFrameChanged = true;
			if (!mPingPong)
			{
				if (mDirection >= 0)
				{
					mFrame += mStep;
					if (mFrame >= aMax)
					{
						mFrame = mUseRange ? mRangeMin : aMin;
						mLoopCount++;
						aDone = mHoldLast || (mLoopCount >= mMaxLoops && mMaxLoops > 0);
					}
				}
				else
				{
					mFrame -= mStep;
					if (mFrame <= aMin)
					{
						mFrame = mUseRange ? mRangeMax : aMax;
						mLoopCount++;
						aDone = mHoldLast || (mLoopCount >= mMaxLoops && mMaxLoops > 0);
					}
				}
			}
			else if (!mReversing)
			{
				mFrame += mStep;
				if (mFrame >= aMax)
				{
					mLoopCount++;
					mFrame = aMax - 2;
					mReversing = true;
					aDone = (mLoopCount >= mMaxLoops && mMaxLoops > 0);
				}
			}
			else
			{
				mFrame -= mStep;
				if (mFrame < aMin)
				{
					mLoopCount++;
					mFrame = aMin + 1;
					mReversing = false;
					if (mHoldLast || (mLoopCount >= mMaxLoops && mMaxLoops > 0))
					{
						mFrame = aMin;
						aDone = true;
					}
				}
			}
			if (aDone)
			{
				mStopped = true;
				mDone = true;
			}
		}
		if (mNumFrames == 1)
			mFrameChanged = false;
	}
};

}

#endif
