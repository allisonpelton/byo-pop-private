#ifndef __FEEDINGFRENZY2_VECTORMATH_H__
#define __FEEDINGFRENZY2_VECTORMATH_H__

// The game's small vector helpers, each kept to the original's x87 rounding: a value is computed at double precision
// and rounded to a float where the original stores it.

#include "Geometry.h"

#include <cmath>

// 0x502b7a (feedingfrenzy2): a square, rounded to a float
inline float squareOf(float theValue)
{
	return (float)((double)theValue * theValue);
}

// 0x43d450 (feedingfrenzy2): the vector's length squared, each square rounded to a float, their sum left at the
// x87's precision
inline double lengthSquared(const engine::Vector2f& theVector)
{
	float aY = squareOf(theVector.y);
	float aX = squareOf(theVector.x);
	return (double)aX + aY;
}

// 0x43a890 (feedingfrenzy2): a square root, rounded to a float
inline float squareRootOf(float theValue)
{
	return (float)sqrt((double)theValue);
}

// 0x43cde0 (feedingfrenzy2): its length, the sum rounded to a float first and the root too
inline float length(const engine::Vector2f& theVector)
{
	return squareRootOf((float)lengthSquared(theVector));
}

// 0x451e00 (feedingfrenzy2): the dot product, at the x87's precision (the caller rounds it as it stores it)
inline double dot(const engine::Vector2f& theFirst, const engine::Vector2f& theSecond)
{
	return (double)theFirst.y * theSecond.y + (double)theFirst.x * theSecond.x;
}

// 0x43c1a0 (feedingfrenzy2): the vector times a number, each coordinate rounded to a float
inline engine::Vector2f scaled(const engine::Vector2f& theVector, float theScale)
{
	return engine::Vector2f((float)((double)theVector.x * theScale), (float)((double)theScale * theVector.y));
}

// 0x43dc80 (feedingfrenzy2): each coordinate times its own number
inline engine::Vector2f scaled(const engine::Vector2f& theVector, float theScaleX, float theScaleY)
{
	return engine::Vector2f((float)((double)theVector.x * theScaleX), (float)((double)theVector.y * theScaleY));
}

// 0x43dca0 and 0x43aa30 (feedingfrenzy2): the vector moved by an offset
inline engine::Vector2f moved(const engine::Vector2f& theVector, float theOffsetX, float theOffsetY)
{
	return engine::Vector2f((float)((double)theVector.x + theOffsetX), (float)((double)theVector.y + theOffsetY));
}

// 0x43d050 (feedingfrenzy2): the first less the second
inline engine::Vector2f difference(const engine::Vector2f& theFirst, const engine::Vector2f& theSecond)
{
	return engine::Vector2f((float)((double)theFirst.x - theSecond.x), (float)((double)theFirst.y - theSecond.y));
}

// 0x43d430 (feedingfrenzy2): the vector moved by another times a number (a velocity by an acceleration over a time)
inline void addScaled(engine::Vector2f& theVector, const engine::Vector2f& theOther, float theScale)
{
	theVector.x = (float)((double)theOther.x * theScale + theVector.x);
	theVector.y = (float)((double)theScale * theOther.y + theVector.y);
}

// 0x5143d2 (feedingfrenzy2): the vector shortened to the length when it is longer
inline void clampLength(engine::Vector2f& theVector, float theLength)
{
	double aSquare = lengthSquared(theVector);
	if ((double)theLength * theLength < aSquare)
	{
		double aScale = theLength / (double)squareRootOf((float)aSquare);
		theVector.x = (float)(theVector.x * aScale);
		theVector.y = (float)(aScale * theVector.y);
	}
}

// 0x51440f (feedingfrenzy2): the vector brought back onto the ellipse of the half axes when it is outside it
inline void clampToEllipse(engine::Vector2f& theVector, const engine::Vector2f& theAxes)
{
	engine::Vector2f aShare((float)((double)theVector.x / theAxes.x), (float)((double)theVector.y / theAxes.y));
	double aSquare = lengthSquared(aShare);
	if (1.0 < aSquare)
	{
		double aScale = 1.0 / (double)squareRootOf((float)aSquare);
		theVector.x = (float)(theVector.x * aScale);
		theVector.y = (float)(aScale * theVector.y);
	}
}

// 0x4345f0 (feedingfrenzy2): the vector brought to length 1 by its length's inverse; left as it is at length 0
inline void normalize(engine::Vector2f& theVector)
{
	float aLength = length(theVector);
	if (0.0f != aLength)
	{
		double anInverse = 1.0 / aLength;
		theVector.x = (float)(theVector.x * anInverse);
		theVector.y = (float)(anInverse * theVector.y);
	}
}

// 0x45e790 (feedingfrenzy2): the cosine of a float, rounded to a float
inline float cosOf(float theAngle)
{
	return (float)cos((double)theAngle);
}

// 0x5085b3 (feedingfrenzy2): the vector turned by the angle, its cosine and sine rounded to floats first
inline engine::Vector2f rotated(const engine::Vector2f& theVector, float theAngle)
{
	double aCos = cosOf(theAngle);
	double aSin = engine::sinOf(theAngle);
	return engine::Vector2f((float)(aCos * theVector.x - aSin * theVector.y),
	                        (float)(theVector.y * aCos + theVector.x * aSin));
}

#endif
