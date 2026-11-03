#include "Random.h"

using namespace engine;

static unsigned int gHoldRand = 1;  // 0x49729c

// 0x44673f
void engine::crtSrand(unsigned int theSeed)
{
	gHoldRand = theSeed;
}

unsigned int engine::crtRandState()
{
	return gHoldRand;
}

// 0x446749
int engine::crtRand()
{
	gHoldRand = gHoldRand * 214013u + 2531011u;
	return (int)((gHoldRand >> 16) & 0x7fff);
}

// 3.051851e-05 (0x4802c8), a little under 1/32767 as a float
static const float RAND_SCALE = 3.0518509e-05f;

// 0x42bce0 (the x87 sum in double, cut with _ftol)
int engine::randomInt(int theLow, int theHigh)
{
	float aLow = (float)theLow;
	int aRand = crtRand();
	return (int)((double)aRand * RAND_SCALE * ((double)theHigh - aLow) + aLow);
}

// 0x401050
int engine::randomIndex(int theLow, int theHigh)
{
	float aLow = (float)theLow;
	int aRand = crtRand();
	return (int)((double)aRand * RAND_SCALE * ((double)(theHigh + 1) - aLow) + aLow);
}

// 0x42bd10 (the x87 sum in double)
float engine::randomFloat(const Point& theRange)
{
	float aLow = theRange.x;
	float aHigh = theRange.y;
	int aRand = crtRand();
	return (float)((double)aRand * RAND_SCALE * ((double)aHigh - aLow) + aLow);
}
