#ifndef __FEEDINGFRENZY2_DRAWS_H__
#define __FEEDINGFRENZY2_DRAWS_H__

// The game's draws of the C library's random numbers (engine::crtRand) over a range

#include "Geometry.h"

// 0x422cb0 (feedingfrenzy2): a number drawn in the range
float drawIn(const engine::FloatRange& theRange);
// 0x41ce50 (feedingfrenzy2): a whole number drawn from the low end to the high one (to the low from the high when
// they are turned about)
int drawWhole(int theLow, int theHigh);
inline int drawWhole(const engine::IntRange& theRange)
{
	return drawWhole(theRange.mLow, theRange.mHigh);
}

#endif
