// The game's random numbers and the maths calls that return their results unrounded in the original.

#ifndef VENICE_MATH_H
#define VENICE_MATH_H

#include <cmath>
#include <cstdint>

// The game's pseudo-random generator: the original runs it on one global seed (0x5b5174), starting at 93186752. It is
// Schrage's method of a multiplicative generator of modulus 4294967291 with its arithmetic done in 32-bit wrapping integers,
// so the numbers are those of the original's code and not those of the generator it was taken from.
const uint32_t RandomSeedMultiplier = 1588635695u; // 0x5b517c
const uint32_t RandomSeedQuotient = 2u; // 0x5b5180
const uint32_t RandomSeedRemainder = 1117695901u; // 0x5b5178
const uint32_t RandomSeedInitial = 93186752u;

extern uint32_t gRandomSeed;

// 0x439d70
inline uint32_t NextRandom()
{
	gRandomSeed = (gRandomSeed % RandomSeedQuotient) * RandomSeedMultiplier -
		(gRandomSeed / RandomSeedQuotient) * RandomSeedRemainder;
	return gRandomSeed;
}

// 0x439db0
inline void SeedRandom(uint32_t theSeed) { gRandomSeed = theSeed; }

// 0x439dc0: a number from 0 to theLimit - 1
inline uint32_t RandomBelow(uint32_t theLimit) { return NextRandom() % theLimit; }

// 0x439de0: a number from theLow to theHigh
inline int RandomBetween(int theLow, int theHigh)
{
	uint32_t aRandom = NextRandom();
	return (int)(aRandom % ((uint32_t)(theHigh - theLow) + 1u) + (uint32_t)theLow);
}

// 0x439e10: a number from theLow to theHigh in steps of a hundredth. The original returns it as an x87 value that is
// rounded to a float only where the caller stores it.
inline double RandomFloat(float theLow, float theHigh)
{
	uint32_t aRandom = NextRandom();
	uint32_t aRange = (uint32_t)((int)(theHigh * 100.0f) - (int)(theLow * 100.0f) + 1);
	float aValue = (float)(int)(aRandom % aRange) + theLow * 100.0f;
	return (double)aValue / 100.0;
}

// The CRT calls 0x403400 (cos), 0x403480 (sin), 0x4034c0 (sqrt) and 0x403440 (fabs) take a float and return the double
// result; the callers keep it unrounded. The sine and cosine are VeniceMath.cpp's, the same bits on every platform.
double VeniceCos(float theAngle);
double VeniceSin(float theAngle);
inline double VeniceSqrt(float theValue) { return sqrt((double)theValue); }
inline double VeniceAbs(float theValue) { return fabs((double)theValue); }
// 0x40c0a0: the arc tangent of theY over theX in radians, returned unrounded like the CRT calls above
inline double VeniceArcTangent(float theY, float theX) { return atan2((double)theY, (double)theX); }
// the factor that turns radians into degrees in the original's code (the float 57.29578)
const float RadiansToDegrees = 57.29578f;

// The degrees-to-radians factor of the original's code (the float 0.017453292)
const float DegreesToRadians = 0.017453292f;

#endif
