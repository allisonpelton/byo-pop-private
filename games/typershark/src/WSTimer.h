#ifndef __TYPERSHARK_WSTIMER_H__
#define __TYPERSHARK_WSTIMER_H__

// WSTimer.cpp: a stopwatch that the widget manager keeps running by updating it. TooltipWidget uses one for its
// show and hide delays. Each Update adds 10 to the update counter, so Elapsed() counts milliseconds of an
// assumed 100 updates a second (the game runs at 100 updates a second).
// Original: class Sexy::WSTimer : public Widget, vftable 0x529464, sizeof 0x64 (new(100) in TooltipWidget's
// constructor 0x443010, where the constructor is inlined). Virtuals 47 (Reset) and 48 (Elapsed) are its own.

#include "Widget.h"

namespace Sexy
{

class WSTimer : public Widget
{
public:
	int						mPausedTotal;	// +0x58 subtracted from the elapsed time; Board::SetPaused adds the pause time to six timers
	int						mStart;			// +0x5c update count at the last Reset
	bool					mPaused;		// +0x60 set by SetDisabled (slot 10): a paused timer reads 0

public:
	WSTimer();																// inlined in 0x443010
	virtual ~WSTimer() override;											// 0x401360, slot 0

	virtual void			SetDisabled(bool isDisabled) override;			// 0x401320 slot 10: only sets mPaused
	virtual void			Update() override;								// 0x441910 slot 17: mUpdateCnt += 10
	virtual void			Reset();										// 0x4418e0 slot 47
	virtual int				Elapsed();										// 0x4418f0 slot 48
};

}

#endif
