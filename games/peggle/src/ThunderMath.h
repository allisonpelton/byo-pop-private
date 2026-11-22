// ThunderMath.h: the small number functions the game's own classes share (the original keeps them in its Common code). They
// compute in double and store floats where the original's x87 code did, so the results come out the same.

#ifndef __THUNDERMATH_H__
#define __THUNDERMATH_H__

#include "SexyVector.h"

namespace Sexy
{

// The values the original's constants hold: 2 pi, pi and a quarter turn, rounded to a float and widened again.
extern const double TWO_PI_AS_FLOAT;  // 0x5d8fe0
extern const double HALF_PI_AS_FLOAT; // 0x5d4f70
extern const float PI_AS_FLOAT;		  // 0x5f0e64

// 0x4bc2d0: turns the point (*theX, *theY) around (theCenterX, theCenterY) by theAngle (radians)
void RotateXY(float* theX, float* theY, float theCenterX, float theCenterY, float theAngle);

// 0x432660 (the original's SexyVector2::Normalize): the length is the square root of a sum that is rounded once, where the
// framework's own Normalize rounds each square, so the vectors of the physics are normalised with this one
SexyVector2 NormalizeVector(const SexyVector2& theVector);

float GetCanonicalAngleDeg(float theAngle); // 0x4bc340: the same angle in [0, 360)
float GetCanonicalAngleRad(float theAngle); // 0x4bc3b0: the same angle in [0, 2 pi)

// 0x475310: how far to turn from one angle to another, the short way round, in (-pi, pi]
float CalcAngleDiff(float theFrom, float theTo);
// 0x475390: 1, -1 or 0
inline int Sign(float theValue) { return theValue > 0 ? 1 : (theValue < 0 ? -1 : 0); }
// 0x43de50: the distance between two points (the squares are rounded to floats first, as the original's code does)
float Distance(float theX1, float theY1, float theX2, float theY2);
// 0x406710
template <typename T>
T ClampRange(T theValue, T theLow, T theHigh)
{
	if (theValue >= theLow && theValue <= theHigh)
		return theValue;
	return theValue < theLow ? theLow : theHigh;
}

} // namespace Sexy

#endif
