#include "CurvedValue.h"

#include <cmath>

using namespace Sexy;

// The original's literals (not the exact pi)
static const double kPi = 3.14159;
static const double kHalfPi = 1.570795;
static const double kTwoPi = 6.28318;

bool CurvedValue::Step()
{
	if (mMode == MODE_ONCE || mMode == MODE_ONCE_PINGPONG) {
		if (!(mTStart < mT)) {
			mT = mTStart;
			return false;
		}
		if (!(mT < mTEnd)) {
			mT = mTEnd;
			return false;
		}
		return true;
	}
	if (mMode == MODE_LOOP || mMode == MODE_LOOP_PINGPONG) {
		double aLength = mTEnd - mTStart;
		if (mTEnd <= mT || !(mTStart < mT))
			mT = std::fmod(mT - mTStart + aLength, aLength) + mTStart;
		return true;
	}
	return true;
}

double CurvedValue::Value() const
{
	bool aDoubled = mMode == MODE_LOOP_PINGPONG || mMode == MODE_ONCE_PINGPONG;
	double aRange = mEnd - mStart;
	double aFrac = (mT - mTStart) / (mTEnd - mTStart);
	double x;
	switch (mCurve) {
	case CURVE_LINEAR:
		if (!aDoubled)
			return aFrac * aRange + mStart;
		// up in the first half of the time range and back down in the second
		if ((mTEnd - mTStart) * 0.5 < mT - mTStart)
			aFrac = 1.0 - aFrac;
		return aFrac * aRange * 2.0 + mStart;
	case CURVE_EASE_IN:
		x = aFrac * kPi * 0.5;
		if (aDoubled)
			x += x;
		if (kHalfPi < x)
			x = kPi - x;
		return (1.0 - std::cos(x)) * aRange + mStart;
	case CURVE_EASE_OUT:
		x = aFrac * kPi * 0.5;
		if (aDoubled)
			x += x;
		return std::sin(x) * aRange + mStart;
	case CURVE_EASE_IN_OUT:
		x = aFrac * kPi;
		if (aDoubled)
			x += x;
		return (1.0 - std::cos(x)) * 0.5 * aRange + mStart;
	case CURVE_SINE_BUMP:
		x = aFrac * kPi;
		if (aDoubled)
			x += x;
		if (kPi < x)
			x = kTwoPi - x;
		if (kHalfPi <= x)
			return (2.0 - std::sin(x)) * 0.5 * aRange + mStart;
		return std::sin(x) * 0.5 * aRange + mStart;
	}
	return mStart;
}
