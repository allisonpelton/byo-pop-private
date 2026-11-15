#include "AngleTable.h"
#include <cmath>

using namespace Sexy;

// 0x42bb50 (the x87 code works in long double)
AngleTable::AngleTable()
{
	mPi = 3.1415925f;	// 0x40490fda, the float just below pi (the exe stores these bits)
	int i = 0;
	do
	{
		mDX[i] = (float) sinl((long double) mPi * i * 0.005555555555555556L);
		mDY[i] = (float) -cosl(i * (long double) mPi * 0.005555555555555556L);
		i++;
	} while (i < 0x168);
}

// 0x42bbc0: the whole degree from one point to another, 0 up and 90 right
// port: the x87 code works in 53 bits, so this works in double
int AngleTable::AngleTo(int theFromX, int theFromY, int theToX, int theToY)
{
	int aDX = theToX - theFromX;
	int aDY = theToY - theFromY;
	float aFDX = (float) aDX;
	float aFDY = (float) aDY;
	if (aFDY == 0.0f)
	{
		if ((aFDX < 0.0f) || (aFDX == 0.0f))
			return 0x10e;
		return 0x5a;
	}
	double anAngle = fabs(atan((double) aDX / (double) aFDY) / (double) mPi * 180.0);
	if (!(aFDX < 0.0f) && (aFDY < 0.0f))
		return (int) anAngle;
	if ((aFDX < 0.0f) && (aFDY < 0.0f))
		return (int) (360.0 - anAngle);
	if (!(aFDX < 0.0f) && !((aFDY < 0.0f) || (aFDY == 0.0f)))
		return (int) (180.0 - anAngle);
	return (int) (anAngle + 180.0);
}

// 0x42bcd0
// port: the x87 code keeps the products and sums unrounded (in 53 bits), so this works in double
bool AngleTable::Intersect(const float* theLine1, const float* theLine2, float* thePoint)
{
	float aDY1 = theLine1[3] - theLine1[1];
	double aCross = ((double) theLine2[3] - (double) theLine2[1]) * (double) theLine1[0];
	double aDenom = ((((double) theLine2[1] - (double) theLine2[3]) * (double) theLine1[2] +
		((double) theLine1[1] - (double) theLine1[3]) * (double) theLine2[0]) + (double) aDY1 * (double) theLine2[2]) +
		aCross;
	if (aDenom == 0.0)
		return false;
	double aScale = 1.0 / aDenom;
	double aT1 = (aCross + (((double) theLine2[1] - (double) theLine1[1]) * (double) theLine2[2] +
		((double) theLine1[1] - (double) theLine2[3]) * (double) theLine2[0])) * aScale;
	double aT2 = -(((((double) theLine1[1] - (double) theLine2[1]) * (double) theLine1[2] +
		((double) theLine2[1] - (double) theLine1[3]) * (double) theLine1[0])) + (double) aDY1 * (double) theLine2[0]) *
		aScale;
	thePoint[0] = (float) (((double) theLine1[2] - (double) theLine1[0]) * aT1 + (double) theLine1[0]);
	thePoint[1] = (float) (((double) theLine1[3] - (double) theLine1[1]) * aT1 + (double) theLine1[1]);
	if ((0.0 <= aT1) && (aT1 <= 1.0) && (0.0 <= aT2) && (aT2 <= 1.0))
		return true;
	return false;
}

// 0x42be00
void AngleTable::Normalize(double* theOut, int theX, int theY)
{
	double aScale = 1.0 / sqrt((double) (theX * theX + theY * theY));
	theOut[0] = (double) theX * aScale;
	theOut[1] = (double) theY * aScale;
}
