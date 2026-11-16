// ThunderMath.cpp: see ThunderMath.h.

#include "ThunderMath.h"

#include <cmath>

using namespace Sexy;

const double Sexy::TWO_PI_AS_FLOAT = 6.2831854820251465;
const double Sexy::HALF_PI_AS_FLOAT = 1.5707963705062866;
const float Sexy::PI_AS_FLOAT = 3.1415927410125732f;

// 0x4bc2d0
void Sexy::RotateXY(float* theX, float* theY, float theCenterX, float theCenterY, float theAngle)
{
	float aDeltaX = (float)((double)*theX - theCenterX);
	float aDeltaY = (float)((double)*theY - theCenterY);
	float aCos = (float)std::cos((double)theAngle);
	float aSin = (float)std::sin((double)theAngle);
	*theX = (float)((double)aCos * aDeltaX + theCenterX + (double)aSin * aDeltaY);
	*theY = (float)(((double)aCos * aDeltaY + theCenterY) - (double)aSin * aDeltaX);
}

// 0x432660
SexyVector2 Sexy::NormalizeVector(const SexyVector2& theVector)
{
	float aLength = (float)std::sqrt((double)(float)((double)theVector.y * theVector.y + (double)theVector.x * theVector.x));
	if (aLength == 0)
		return theVector;
	return SexyVector2(theVector.x / aLength, theVector.y / aLength);
}

// 0x4bc340
float Sexy::GetCanonicalAngleDeg(float theAngle)
{
	if (theAngle >= 0 && theAngle < 360.0)
		return theAngle;
	if (theAngle < 0)
		return (float)(360.0 - (float)std::fmod(-(double)theAngle, 360.0));
	return (float)std::fmod((double)theAngle, 360.0);
}

// 0x4bc3b0
float Sexy::GetCanonicalAngleRad(float theAngle)
{
	if (theAngle >= 0 && theAngle < TWO_PI_AS_FLOAT)
		return theAngle;
	if (theAngle < 0)
		return (float)(TWO_PI_AS_FLOAT - (float)std::fmod(-(double)theAngle, TWO_PI_AS_FLOAT));
	return (float)std::fmod((double)theAngle, TWO_PI_AS_FLOAT);
}
