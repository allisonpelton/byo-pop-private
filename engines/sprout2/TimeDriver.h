#ifndef __SPROUT2_TIMEDRIVER_H__
#define __SPROUT2_TIMEDRIVER_H__

// A clock that runs from 0 to a duration and stops there, or back to 0 (TimeDriver, an engine::Object of 0x20 bytes,
// vtable 0x55bc64 (feedingfrenzy2); its constructor 0x41db20 (feedingfrenzy2)): each frame's seconds move it on in
// its direction, and its value is the time times the scale, so a run of the duration covers the start's range. The
// port keeps it as a member of what holds it, uncounted.
//
// The original also turns at the ends (mode 1) or wraps round (mode 2); only start, which sets mode 0, is translated,
// as nothing translated sets another mode. Forward, mode 1 turns back at the duration by the time it passed it and
// mode 2 starts again from 0 by as much; backward, mode 1 turns forward at 0 and mode 2 goes on from the duration.

namespace engine
{

// The direction (+0x18)
enum TimeDirection
{
	TIME_BACKWARD = -1,
	TIME_STOPPED = 0,
	TIME_FORWARD = 1
};

class TimeDriver
{
public:
	// 0x41db20 (feedingfrenzy2): stopped at 0, for no time
	TimeDriver() : mDuration(0.0f), mScale(0.0f), mTime(0.0f), mDirection(TIME_STOPPED), mMode(0) {}

	// 0x480250 (feedingfrenzy2): forward from 0, once, over the duration, the value covering the range (no range for
	// no duration)
	void start(float theDuration, float theRange)
	{
		mMode = 0;
		mDuration = theDuration;
		mDirection = TIME_FORWARD;
		mScale = theDuration == 0.0f ? 0.0f : theRange / theDuration;
		mTime = 0.0f;
	}
	// 0x431ba0 (feedingfrenzy2): the seconds added (or taken away, backward), stopping at the duration (or 0)
	void advance(float theSeconds)
	{
		if (mDirection == TIME_FORWARD)
		{
			double aTime = (double)theSeconds + mTime;
			mTime = (float)aTime;
			if (aTime < mDuration)
				return;
			mDirection = TIME_STOPPED;
			mTime = mDuration;
		}
		else if (mDirection == TIME_BACKWARD)
		{
			double aTime = (double)mTime - theSeconds;
			mTime = (float)aTime;
			if (0.0 < aTime)
				return;
			mDirection = TIME_STOPPED;
			mTime = 0.0f;
		}
	}
	// 0x5011b5 (feedingfrenzy2)
	bool isRunning() const { return mDirection != TIME_STOPPED; }
	// 0x412d20 (feedingfrenzy2)
	void setTime(float theTime) { mTime = theTime; }
	// Written in place by the popup's fade (0x473e90 (feedingfrenzy2))
	void setDirection(int theDirection) { mDirection = theDirection; }
	// Read in place by a fish's visual (0x43b470 (feedingfrenzy2))
	int getDirection() const { return mDirection; }
	// 0x4352b0 (feedingfrenzy2): kept at the x87's precision for the caller to round
	double getValue() const { return (double)mTime * mScale; }

private:
	float mDuration;   // +0x0c
	float mScale;      // +0x10: the range over the duration
	float mTime;       // +0x14
	int mDirection;    // +0x18: TimeDirection
	int mMode;         // +0x1c: 0 once, 1 back and forth, 2 round again
};

} // namespace engine

#endif
