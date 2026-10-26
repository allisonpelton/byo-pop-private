#include "AngleTable.h"
#include <cmath>

using namespace Sexy;

// 0x42bb50 (the x87 code works in long double)
AngleTable::AngleTable()
{
	mPi = 3.1415927f;
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
