// The matrix and angle arithmetic the particle player does, as the exe's x87 code does it (no function of its own in
// the exe unless an address is given): sums are rounded to a float as they grow, a value the exe stores as a float is
// a float. SexyTransform2D's own functions round differently, so the player does not use them.
#pragma once

#include "PIBezier.h"
#include "SexyMatrix.h"

#include <cmath>

namespace Sexy {
namespace PIMath {

// 0x583230 (bejeweled3): theLeft * theRight
inline SexyMatrix3 Multiply(const SexyMatrix3& theLeft, const SexyMatrix3& theRight)
{
	SexyMatrix3 r;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			float aSum = (float)((double)theLeft.m[i][0] * theRight.m[0][j]);
			aSum = (float)(aSum + (double)theLeft.m[i][1] * theRight.m[1][j]);
			r.m[i][j] = (float)(aSum + (double)theLeft.m[i][2] * theRight.m[2][j]);
		}
	}
	return r;
}

// 0x583490 (bejeweled3): the matrix with a translation put on after it
inline void Translate(SexyMatrix3& theMatrix, float theX, float theY)
{
	SexyMatrix3 aStep;
	aStep.LoadIdentity();
	aStep.m02 = theX;
	aStep.m12 = theY;
	theMatrix = Multiply(aStep, theMatrix);
}

// 0x5834d0 (bejeweled3): a rotation put on after it, in radians
inline void RotateRad(SexyMatrix3& theMatrix, float theRadians)
{
	SexyMatrix3 aStep;
	aStep.LoadIdentity();
	float aSin = (float)sin((double)theRadians);
	float aCos = (float)cos((double)theRadians);
	aStep.m00 = aCos;
	aStep.m01 = aSin;
	aStep.m10 = -aSin;
	aStep.m11 = aCos;
	theMatrix = Multiply(aStep, theMatrix);
}

// 0x583540 (bejeweled3)
inline void RotateDeg(SexyMatrix3& theMatrix, float theDegrees)
{
	RotateRad(theMatrix, (float)((double)theDegrees * 3.1415927410125732 / 180.0));
}

// 0x583570 (bejeweled3)
inline void Scale(SexyMatrix3& theMatrix, float theX, float theY)
{
	SexyMatrix3 aStep;
	aStep.LoadIdentity();
	aStep.m00 = theX;
	aStep.m11 = theY;
	theMatrix = Multiply(aStep, theMatrix);
}

// 0x565cd0 (bejeweled3): a point through a matrix; the point is made a float first and the answer is one
inline PIPoint TransformPoint(const SexyMatrix3& theMatrix, double theX, double theY)
{
	float aX = (float)theX;
	float aY = (float)theY;
	PIPoint aPoint;
	aPoint.mX = (float)(((double)theMatrix.m01 * aY + (double)aX * theMatrix.m00) + theMatrix.m02);
	aPoint.mY = (float)(((double)theMatrix.m10 * aX + (double)theMatrix.m11 * aY) + theMatrix.m12);
	return aPoint;
}

// 0x565d50 (bejeweled3): degrees to radians (with pi as the float 3.14159)
inline float DegreesToRadians(float theDegrees)
{
	return (float)((double)theDegrees * 3.141590118408203 / 180.0);
}

// 0x409130 (bejeweled3), 0x409150 (bejeweled3) and 0x409170 (bejeweled3): cosine, sine and square root, each rounded to a float
inline float Cos(float theValue) { return (float)cos((double)theValue); }
inline float Sin(float theValue) { return (float)sin((double)theValue); }
inline float Sqrt(float theValue) { return (float)sqrt((double)theValue); }

}
}
