#ifndef __HAMMERHEADS_LEVELHUD_H__
#define __HAMMERHEADS_LEVELHUD_H__

// The level number in the corner of a classic game (0x44a680-0x44a910) and the clock of a marathon (0x44a910-0x44b070).

#include "Object.h"

namespace Sexy
{

class LevelIndicator : public Object
{
public:
	LevelIndicator();												// 0x44a820 (made by MakeLevelIndicator)
	virtual ~LevelIndicator();										// 0x44a680

	OBJECT_TYPE(LevelIndicator)

	virtual void	Draw(Graphics* g) override;						// 0x44a6b0
};

// 0x44a820: makes the indicator and remembers it
LevelIndicator* MakeLevelIndicator();

// The marathon's clock: the seconds of the game run up in the stats (the elapsed time, stat 44), and every minute that
// ends gives a bonus
class MarathonMeter : public Object
{
public:
	double			mFlash;				// +0x80: the clock lights up after a bonus (1.5 and falling)

public:
	MarathonMeter();												// 0x44a910 (made by MakeMarathonMeter)
	virtual ~MarathonMeter();										// 0x44a980

	OBJECT_TYPE(MarathonMeter)

	virtual void	Update(float theFrac) override;					// 0x44aa90
	virtual void	Draw(Graphics* g) override;						// 0x44ac30
};

// 0x44a9b0: makes the clock and remembers it
MarathonMeter* MakeMarathonMeter();

}

#endif
