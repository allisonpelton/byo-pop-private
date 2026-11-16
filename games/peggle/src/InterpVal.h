// InterpVal.h: InterpValF (0x4bc4b0), the game's piecewise-linear curve: from theStart at 0 up to theMiddle at
// theFirstLength, then on to theEnd over the next theSecondLength, then theEnd. The loading screen and others use it for fades.

#ifndef __INTERPVAL_H__
#define __INTERPVAL_H__

namespace Sexy
{

inline float InterpValF(int theValue, int theFirstLength, int theSecondLength, float theStart, float theMiddle, float theEnd)
{
	if (theValue < theFirstLength)
	{
		float aFraction = (float)((double)theValue / (double)theFirstLength);
		return (float)((1.0 - (double)aFraction) * (double)theStart + (double)theMiddle * (double)aFraction);
	}
	if (theValue < theFirstLength + theSecondLength)
	{
		float aFraction = (float)((double)(theValue - theFirstLength) / (double)theSecondLength);
		return (float)((1.0 - (double)aFraction) * (double)theMiddle + (double)theEnd * (double)aFraction);
	}
	return theEnd;
}

} // namespace Sexy

#endif
