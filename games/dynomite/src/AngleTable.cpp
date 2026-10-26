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
