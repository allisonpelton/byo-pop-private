#ifndef __FEEDINGFRENZY_RESULTCOUNTER_H__
#define __FEEDINGFRENZY_RESULTCOUNTER_H__

// A number that counts from one value to another in steps, for the results of a stage (the screen's five counters,
// 0x3c bytes each, constructor 0x42a060): each step sends the new number to the slots connected to it. When the
// counter is told to speed up, the steps grow to tens and hundreds while the end is far.

#include "Node.h"
#include "Signal.h"

namespace feedingfrenzy
{

class ResultCounter : public engine::Signal<int>
{
public:
	ResultCounter();  // 0x42a060

	// 0x429e50: the counter is set to count from theStart to theEnd, a step every theInterval seconds, and is not
	// running
	void init(int theStart, int theEnd, float theInterval, bool theAccelerate);
	// 0x429eb0: the most digits a step may have (the steps grow by a digit at a time)
	void setMaxDigits(int theMaxDigits) { mMaxDigits = theMaxDigits; }
	// 0x429e90: the counter starts, with its first step an interval away
	void begin();
	// 0x429ea0: the end has been sent
	bool isDone() const { return mDone; }
	// 0x429ef0: a running counter whose number is at the end sends it and is done (the count of nothing); otherwise it
	// counts down the time to its next step, and the step moves the number towards the end (by one, or by a power of
	// ten when the counter speeds up and the end is far) and sends it; the step that reaches the end is the last
	void update(engine::UpdateInfo& theInfo);

private:
	int mStart;           // +0x18
	int mEnd;             // +0x1c
	int mCurrent;         // +0x20
	float mSecondsLeft;   // +0x24: the seconds until the next step
	float mInterval;      // +0x28: the seconds between the steps
	bool mRunning;        // +0x2c
	int mDirection;       // +0x30: 1 to count up, -1 to count down
	bool mDone;           // +0x34
	bool mAccelerate;     // +0x35: the steps grow while the end is far
	int mMaxDigits;       // +0x38: the most digits a step has when the counter speeds up
};

} // namespace feedingfrenzy

#endif
