#include "ResultCounter.h"

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The most digits a step has until the counter is told otherwise
const int DEFAULT_MAX_DIGITS = 4;
// A counter that speeds up takes steps of one until the end is further than this
const int ACCELERATE_FROM_DISTANCE = 10;
// A step grows by this much for each digit
const int STEP_GROWTH = 10;
}

// 0x42a060
ResultCounter::ResultCounter() :
    mStart(0), mEnd(0), mCurrent(0), mSecondsLeft(0.0f), mInterval(0.0f), mRunning(false), mDirection(1), mDone(false),
    mAccelerate(false), mMaxDigits(DEFAULT_MAX_DIGITS)
{
}

// 0x429e50
void ResultCounter::init(int theStart, int theEnd, float theInterval, bool theAccelerate)
{
	mInterval = theInterval;
	mStart = theStart;
	mCurrent = theStart;
	mEnd = theEnd;
	mRunning = false;
	mDirection = theStart < theEnd ? 1 : -1;
	mDone = false;
	mAccelerate = theAccelerate;
}

// 0x429e90
void ResultCounter::begin()
{
	mSecondsLeft = mInterval;
	mRunning = true;
	mDone = false;
}

// 0x429ef0
void ResultCounter::update(UpdateInfo& theInfo)
{
	if (!mRunning)
		return;
	if (mEnd == mCurrent)
	{
		emit(mCurrent);
		mDone = true;
		return;
	}

	mSecondsLeft -= theInfo.mSeconds;
	if (mSecondsLeft > 0.0f)
		return;

	int aDistance = mCurrent - mEnd;
	if (aDistance < 0)
		aDistance = -aDistance;
	int aStep = 1;
	int aDigits = 1;
	if (mAccelerate && aDistance > ACCELERATE_FROM_DISTANCE)
	{
		do
		{
			if (aDigits >= mMaxDigits)
				break;
			aStep *= STEP_GROWTH;
			aDigits++;
		} while (aStep * STEP_GROWTH < aDistance);
	}

	mCurrent += mDirection * aStep;
	emit(mCurrent);
	if (mEnd == mCurrent)
		mDone = true;
	else
		mSecondsLeft = mInterval;
}
