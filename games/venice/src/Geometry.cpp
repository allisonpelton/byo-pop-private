// The geometry of the jewels' flight (0x438db0-0x43a4f0).

#include "Geometry.h"

#include "VeniceMath.h"

// 0x43a030
double PointDistance(float theX1, float theY1, float theX2, float theY2)
{
	float aDeltaX = theX2 - theX1;
	float aDeltaY = theY2 - theY1;
	return VeniceSqrt(aDeltaX * aDeltaX + aDeltaY * aDeltaY);
}

// 0x43a0b0
double SegmentDistance(float thePointX, float thePointY, float theStartX, float theStartY, float theEndX, float theEndY)
{
	float aLength = (float)PointDistance(theStartX, theStartY, theEndX, theEndY);
	float anAlong = ((thePointX - theStartX) * (theEndX - theStartX) + (thePointY - theStartY) * (theEndY - theStartY)) /
		(aLength * aLength);
	if (anAlong < 0.0f || 1.0f < anAlong) {
		float aStartDistance = (float)PointDistance(thePointX, thePointY, theStartX, theStartY);
		double anEndDistance = PointDistance(thePointX, thePointY, theEndX, theEndY);
		if (anEndDistance <= (double)aStartDistance)
			return (double)(float)PointDistance(thePointX, thePointY, theEndX, theEndY);
		return (double)(float)PointDistance(thePointX, thePointY, theStartX, theStartY);
	}
	float aNearestX = (theEndX - theStartX) * anAlong + theStartX;
	float aNearestY = (theEndY - theStartY) * anAlong + theStartY;
	return PointDistance(thePointX, thePointY, aNearestX, aNearestY);
}

// 0x43a2d0
void SegmentNormal(float theStartX, float theStartY, float theEndX, float theEndY, float* theNormalX, float* theNormalY)
{
	*theNormalX = theEndY - theStartY;
	*theNormalY = theStartX - theEndX;
	float aLength = (float)VeniceSqrt(*theNormalX * *theNormalX + *theNormalY * *theNormalY);
	*theNormalX = *theNormalX / aLength;
	*theNormalY = *theNormalY / aLength;
}

// 0x43a360
void ReflectOffSegment(float* theVelocityX, float* theVelocityY, float theStartX, float theStartY, float theEndX,
	float theEndY)
{
	float aNormalX;
	float aNormalY;
	SegmentNormal(theStartX, theStartY, theEndX, theEndY, &aNormalX, &aNormalY);
	float aDot = *theVelocityX * aNormalX + *theVelocityY * aNormalY;
	if (aDot < 0.0f) {
		aDot = aDot * -2.0f;
		*theVelocityX = aNormalX * aDot + *theVelocityX;
		*theVelocityY = aNormalY * aDot + *theVelocityY;
	}
}

// 0x43a400, 0x43a490
void ReflectOffBall(float* theVelocityX, float* theVelocityY, float theX, float theY, float theBallX, float theBallY)
{
	float aNormalX = theX - theBallX;
	float aNormalY = theY - theBallY;
	float aLength = (float)VeniceSqrt(aNormalX * aNormalX + aNormalY * aNormalY);
	aNormalX = aNormalX / aLength;
	aNormalY = aNormalY / aLength;
	float aDot = *theVelocityX * aNormalX + *theVelocityY * aNormalY;
	if (aDot < 0.0f) {
		aDot = aDot * -2.0f;
		*theVelocityX = aNormalX * aDot + *theVelocityX;
		*theVelocityY = aNormalY * aDot + *theVelocityY;
	}
}

// 0x438e30
float CircleOverlap(float theX1, float theY1, float theRadius1, float theX2, float theY2, float theRadius2)
{
	float anOverlap = (theRadius1 + theRadius2) * (theRadius1 + theRadius2) -
		((theX1 - theX2) * (theX1 - theX2) + (theY1 - theY2) * (theY1 - theY2));
	if (anOverlap < 0.0f)
		anOverlap = 0.0f;
	return anOverlap;
}

// 0x438db0
bool BoxesOverlap(float theX1, float theY1, float theWidth1, float theHeight1, float theX2, float theY2, float theWidth2,
	float theHeight2)
{
	return VeniceAbs(theX1 - theX2) <= (double)theWidth2 * 0.5 + (double)theWidth1 * 0.5 &&
		VeniceAbs(theY1 - theY2) <= (double)theHeight2 * 0.5 + (double)theHeight1 * 0.5;
}
