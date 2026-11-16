#ifndef __SPROUT_RAMP_H__
#define __SPROUT_RAMP_H__

// The signal build's ramp (Pizza Frenzy's; 0x24 bytes, vtable 0x4f9470 (pizzafrenzy), an Object whose constructor does
// nothing more): a time that runs up from 0 to its duration, or back down to 0, while it runs, and the share of the
// way it has gone as a number from 0 to its scale's top. A screen drives the motion of its groups with it.

namespace engine
{

class Ramp
{
public:
	// 0x481180 (pizzafrenzy): a ramp of theDuration seconds whose progress reaches theTop at the end; it starts at 0,
	// running forward
	void set(float theDuration, float theTop)
	{
		mDuration = theDuration;
		if (theDuration != 0.0f)
		{
			mTime = 0;
			mRunning = true;
			mForward = true;
			mScale = theTop / theDuration;
			return;
		}
		mScale = 0;
		mTime = 0;
		mRunning = true;
		mForward = true;
	}
	// 0x481170 (pizzafrenzy): back at 0, running forward
	void reset()
	{
		mTime = 0;
		mRunning = true;
		mForward = true;
	}
	void start() { mRunning = true; }                    // 0x4810e0 (pizzafrenzy)
	void stop() { mRunning = false; }                    // 0x4810f0 (pizzafrenzy)
	void setForward(bool theForward) { mForward = theForward; }  // 0x481100 (pizzafrenzy)
	// 0x481110 (pizzafrenzy): while it runs, the time goes up or down by the seconds, kept from 0 to the duration
	void update(float theSeconds)
	{
		if (!mRunning)
			return;
		if (!mForward)
			theSeconds = -theSeconds;
		float aTime = theSeconds + mTime;
		mTime = aTime;
		if (mDuration < aTime)
			aTime = mDuration;
		mTime = aTime;
		if (aTime <= 0.0f)
			aTime = 0.0f;
		mTime = aTime;
	}
	// 0x4810a0 (pizzafrenzy)
	float getProgress() const { return mTime * mScale; }
	// 0x4810b0 (pizzafrenzy): the time has reached the duration, running forward, or 0, running back
	bool isFinished() const { return mForward ? !(mTime < mDuration) : !(0.0f < mTime); }

private:
	float mDuration = 0;    // +0x0c
	float mScale = 0;       // +0x10: the progress one second of time is worth
	float mTime = 0;        // +0x14
	bool mRunning = false;  // +0x18
	bool mForward = false;  // +0x19
};

} // namespace engine

#endif
