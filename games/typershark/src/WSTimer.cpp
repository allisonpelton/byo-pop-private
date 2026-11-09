#include "WSTimer.h"

using namespace Sexy;

// inlined in TooltipWidget's constructor 0x443010: Widget(), the three fields, Reset(), then mUpdateCnt = 0
WSTimer::WSTimer()
{
	mPausedTotal = 0;
	mPaused = false;
	mStart = 0;
	Reset();
	mUpdateCnt = 0;
}

// 0x401360
WSTimer::~WSTimer()
{
}

// 0x401320 slot 10: replaces Widget::SetDisabled; mDisabled is not touched
void WSTimer::SetDisabled(bool isDisabled)
{
	mPaused = isDisabled;
}

// 0x441910 slot 17: no Widget::Update; the timer counts 10 per update
void WSTimer::Update()
{
	mUpdateCnt += 10;
}

// 0x4418e0 slot 47
void WSTimer::Reset()
{
	mStart = mUpdateCnt;
	mPausedTotal = 0;
}

// 0x4418f0 slot 48: 0 while paused (the export's decompile loses this; the machine code returns 0)
int WSTimer::Elapsed()
{
	if (mPaused)
		return 0;
	return (mUpdateCnt - mStart) - mPausedTotal;
}
