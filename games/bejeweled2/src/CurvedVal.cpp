#include "CurvedVal.h"
#include <math.h>

using namespace Sexy;

static const double PI = 3.14159;

CurvedVal::CurvedVal()
{
	mMode = MODE_CLAMP;
	mOutMin = 0;
	mRamp = RAMP_SLOW_FAST_SLOW;
	mOutMax = 1.0;
	mInMax = 1.0;
	mInMin = 0;
	mInVal = 0;
}

double CurvedVal::GetOutVal()
{
	double aT = (mInVal - mInMin) / (mInMax - mInMin);
	double aRange = mOutMax - mOutMin;
	double aVal;

	switch (mRamp)
	{
	case RAMP_LINEAR:
		if (mMode == MODE_PING_PONG)
		{
			if ((mInMax - mInMin) * 0.5 < mInVal - mInMin)
				aT = 1.0 - aT;
			aVal = aRange * aT;
			aVal += aVal;
			return aVal + mOutMin;
		}
		return aRange * aT + mOutMin;

	case RAMP_SLOW_TO_FAST:
	{
		double x = aT * PI * 0.5;
		if (mMode == MODE_PING_PONG)
			x += x;
		if (!(1.570795 > x))
			x = PI - x;
		aVal = 1.0 - cos(x);
		break;
	}
	case RAMP_FAST_TO_SLOW:
	{
		double x = aT * PI * 0.5;
		if (mMode == MODE_PING_PONG)
			x += x;
		aVal = sin(x);
		break;
	}
	case RAMP_SLOW_FAST_SLOW:
	{
		double x = aT * PI;
		if (mMode == MODE_PING_PONG)
			x += x;
		aVal = (1.0 - cos(x)) * 0.5;
		break;
	}
	case RAMP_FAST_SLOW_FAST:
	{
		double x = aT * PI;
		if (mMode == MODE_PING_PONG)
			x += x;
		if (PI < x)
			x = 6.28318 - x;
		if (x < 1.570795)
			aVal = sin(x);
		else
			aVal = 2.0 - sin(x);
		aVal *= 0.5;
		break;
	}
	default:
		return mOutMin;
	}

	return aRange * aVal + mOutMin;
}

bool CurvedVal::CheckClamping()
{
	if (mMode == MODE_CLAMP)
	{
		if (mInVal < mInMin)
		{
			mInVal = mInMin;
			return false;
		}
		if (mInVal > mInMax)
		{
			mInVal = mInMax;
			return false;
		}
		return true;
	}

	if (mInVal < mInMin || mInVal > mInMax)
		mInVal = fmod((mInVal - mInMin) + (mInMax - mInMin), mInMax - mInMin) + mInMin;
	return true;
}

void CurvedVal::SetInVal(double theVal)
{
	mInVal = theVal;
	CheckClamping();
}

bool CurvedVal::IncInVal(double theInc)
{
	mInVal += theInc;
	return CheckClamping();
}
