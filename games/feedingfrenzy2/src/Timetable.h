#ifndef __FEEDINGFRENZY2_TIMETABLE_H__
#define __FEEDINGFRENZY2_TIMETABLE_H__

#include <vector>

// A list of times to wait for, one after another (0x28 bytes inside the states; made by 0x421c00 (feedingfrenzy2), or
// with a count by 0x47ae00 (feedingfrenzy2)): each step adds the frame's time, and once the time reaches the next of
// the list, that one is passed. The fish's states use it with one time at a time, starting it over when it passes;
// the stage's completion waits for three in turn.
struct Timetable
{
	Timetable() : mOn(false), mNext(0), mTime(0.0f) {}
	// 0x47ae00 (feedingfrenzy2): the count of times, each 0, off
	explicit Timetable(int theCount) : mOn(false), mTimes(theCount, 0.0f), mNext(0), mTime(0.0f) {}

	// 0x473150 (feedingfrenzy2) with the clock cleared and the timetable on, as each state does after it: the one time
	// waited for, from now
	void start(float theSeconds)
	{
		mTimes.clear();
		mTimes.push_back(theSeconds);
		mNext = 0;
		mTime = 0.0f;
		mOn = true;
	}
	// The same time waited for again, from now
	void restart()
	{
		mNext = 0;
		mTime = 0.0f;
		mOn = true;
	}
	// The frame's time added while it has a time left to wait for; true when the time has reached it, which the
	// caller then passes
	bool advance(float theElapsed)
	{
		if (!mOn || mNext >= mTimes.size())
			return false;
		mTime = (float)((double)mTime + theElapsed);
		return mTimes[mNext] <= mTime;
	}
	// 0x45f040 (feedingfrenzy2): the same, the time reached passed at once; true as one is
	bool step(float theElapsed)
	{
		if (!advance(theElapsed))
			return false;
		mNext++;
		return true;
	}
	// 0x426ba0 (feedingfrenzy2): the share of the way from the time last passed (0 before the first) to the next, 1
	// when none is left
	float getShare() const
	{
		if (mNext >= mTimes.size())
			return 1.0f;
		float aFrom = mNext == 0 ? 0.0f : mTimes[mNext - 1];
		return (float)(((double)mTime - aFrom) / ((double)mTimes[mNext] - aFrom));
	}
	// 0x420090 (feedingfrenzy2): on with a time left to wait for
	bool isWaiting() const { return mOn && mNext < mTimes.size(); }

	bool mOn;                    // +0x00
	std::vector<float> mTimes;   // +0x08
	unsigned int mNext;          // +0x14: the first time not yet passed
	float mTime;                 // +0x18: the seconds since the start
	// +0x20 and +0x24: zeroed by the constructor, read by nothing translated yet
};

#endif
