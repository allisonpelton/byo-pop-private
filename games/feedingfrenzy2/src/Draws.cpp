#include "Draws.h"
#include "Random.h"

using namespace engine;

float drawIn(const FloatRange& theRange)
{
	return (float)randomFloat(theRange.mLow, theRange.mHigh);
}

int drawWhole(int theLow, int theHigh)
{
	int aDraw = crtRand();
	if (theLow > theHigh)
		return aDraw % (theLow - theHigh + 1) + theLow;
	return aDraw % (theHigh - theLow + 1) + theLow;
}
