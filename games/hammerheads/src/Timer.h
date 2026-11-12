#ifndef __HAMMERHEADS_TIMER_H__
#define __HAMMERHEADS_TIMER_H__

// The clock of a level (0x44bb80-0x44c170): the seconds of the level run down by the fixed step, in waves. At the free seconds
// at the end of a wave it waits for the gnomes to clear, shows the wave's result, and then runs the free seconds out ten
// times as fast.

#include "Object.h"

namespace Sexy
{

enum TimerPhase
{
	TIMER_PHASE_WAVE = 0,			// in the part of a wave that brings gnomes
	TIMER_PHASE_CLEARING = 1,		// in the free seconds, waiting for the cubbies to be empty
	TIMER_PHASE_RUNNING_OUT = 2		// the result is shown (or was); the time runs ten times as fast while nothing is shown
};

enum TimerSetup
{
	TIMER_SETUP_NEW_LEVEL = 0,		// the seconds of the level, bonus counts cleared
	TIMER_SETUP_RUN = 1,			// runs at one hundredth of a second a step
	TIMER_SETUP_STOP = 2,			// does not run
	TIMER_SETUP_ALMOST_OUT = 3		// a thousandth of a second left
};

class Timer : public Object
{
public:
	double			mTotal;			// +0x80: the seconds of the level
	double			mLeft;			// +0x88
	double			mRate;			// +0x90: seconds a fixed step takes off (0 stops it)
	int				mPhase;			// +0x98

public:
	Timer();														// 0x44bd60
	virtual ~Timer();												// 0x44bb80

	OBJECT_TYPE(Timer)

	virtual void	FixedUpdate() override;							// 0x44bf30

	void			Drain();										// 0x44bbf0
	void			Setup(int theSetup);							// 0x44bc80
	double			GetProgress() const;							// 0x44bd20: how much of the level has gone, 0 to 1
	double			GetTimeUsed() const;							// 0x44bd40
};

// 0x44bc50: the clock of the level on screen
Timer* GetTimer();
// 0x44be50: makes the clock and remembers it
Timer* MakeTimer();
// 0x44bb90: a wave result or level summary is showing
bool IsWaveResultShowing();

}

#endif
