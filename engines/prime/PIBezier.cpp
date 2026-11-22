// Sexy::Bezier of the particle effect (PIBezier.h). The original computes on the x87 in double precision and rounds to
// a float where it stores one; the port does the same.
#include "PIBezier.h"

namespace Sexy {

// 0x5a19f0 (bejeweled3)
void PIBezier::Init(const std::vector<PIPoint>& thePoints, const std::vector<PIPoint>& theControls,
                    const std::vector<float>& theTimes, int theCount)
{
	if (mPointCount != 0 || theCount <= 1 || thePoints.empty() || theControls.empty() || theTimes.empty())
		return;
	mPoints.assign(thePoints.begin(), thePoints.begin() + theCount);
	mControls.assign(theControls.begin(), theControls.begin() + (theCount - 1) * 2);
	mTimes.assign(theTimes.begin(), theTimes.begin() + theCount);
	mPointCount = theCount;
}

// The first segment whose end time is after theTime (the original's unrolled search, as a plain one). A time equal to
// the last point's time runs past the last segment in the original, which reads the next heap block; the port takes
// the last segment
int PIBezier::FindSegment(float theTime) const
{
	int aSegment = 0;
	while (aSegment < mPointCount - 1 && !(theTime < mTimes[aSegment + 1]))
		aSegment++;
	if (aSegment > mPointCount - 2)
		aSegment = mPointCount - 2;
	return aSegment;
}

// 0x5a1d70 (bejeweled3)
PIPoint PIBezier::Evaluate(float theTime) const
{
	PIPoint aResult = {0.0, 0.0};
	if (mPointCount < 2)
		return aResult;
	if (theTime <= mTimes[0])
		return mPoints[0];
	if (mTimes[mPointCount - 1] < theTime)
		return mPoints[mPointCount - 1];

	int aSegment = FindSegment(theTime);
	const PIPoint* aControl = &mControls[aSegment * 2];
	const PIPoint* aPoint = &mPoints[aSegment];
	float aFraction = (theTime - mTimes[aSegment]) / (mTimes[aSegment + 1] - mTimes[aSegment]);
	double f = (double)aFraction;

	aResult.mX = ((f * (((aPoint[1].mX - aControl[1].mX * 3.0) + aControl[0].mX * 3.0) - aPoint[0].mX) +
	               (aControl[1].mX * 3.0 - aControl[0].mX * 6.0) + aPoint[0].mX * 3.0) * f +
	              (aControl[0].mX * 3.0 - aPoint[0].mX * 3.0)) * f + aPoint[0].mX;
	aResult.mY = (f * ((aControl[1].mY * 3.0 - aControl[0].mY * 6.0) + aPoint[0].mY * 3.0 +
	                   f * (((aPoint[1].mY - aControl[1].mY * 3.0) + aControl[0].mY * 3.0) - aPoint[0].mY)) +
	              (aControl[0].mY * 3.0 - aPoint[0].mY * 3.0)) * f + aPoint[0].mY;
	return aResult;
}

// 0x5a1f60 (bejeweled3)
PIPoint PIBezier::Velocity(float theTime) const
{
	PIPoint aResult = {0.0, 0.0};
	if (mPointCount < 2 || theTime <= mTimes[0] || mTimes[mPointCount - 1] < theTime)
		return aResult;

	int aSegment = FindSegment(theTime);
	const PIPoint* aControl = &mControls[aSegment * 2];
	const PIPoint* aPoint = &mPoints[aSegment];
	float aFraction = (theTime - mTimes[aSegment]) / (mTimes[aSegment + 1] - mTimes[aSegment]);
	double f = (double)aFraction;

	aResult.mX = ((f * (((aPoint[1].mX - aControl[1].mX * 3.0) + aControl[0].mX * 3.0) - aPoint[0].mX) * 3.0 +
	               (aControl[1].mX * 6.0 - aControl[0].mX * 12.0) + aPoint[0].mX * 6.0) * f) +
	             (aControl[0].mX * 3.0 - aPoint[0].mX * 3.0);
	aResult.mY = f * (f * (((aPoint[1].mY - aControl[1].mY * 3.0) + aControl[0].mY * 3.0) - aPoint[0].mY) * 3.0 +
	                  aPoint[0].mY * 6.0 + (aControl[1].mY * 6.0 - aControl[0].mY * 12.0)) +
	             (aControl[0].mY * 3.0 - aPoint[0].mY * 3.0);
	return aResult;
}

}
