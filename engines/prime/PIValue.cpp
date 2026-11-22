// Sexy::PIValue and Sexy::PIValue2D (PIValue.h). The original computes on the x87 in double precision and rounds to a
// float where it stores one; the port does the same, with the original's float-to-int conversion (Ftol).
#include "PIValue.h"

#include "X86Math.h"

#include <cmath>

namespace Sexy {

// 0x5654c0 (bejeweled3)
PIValue::PIValue() : mLastTime(-1.0f), mLastValue(0.0f), mLastParam(0.0f), mParamStep(0.01f)
{
}

// 0x566070 (bejeweled3): samples the curve into mQuantized, 256 entries that hold the value at an equal step of the
// keyframes' time span. The curve is walked in half-steps; each place it lands in the table takes its value, and the
// gaps between places are filled in a straight line
void PIValue::QuantizeCurve()
{
	float aFirstTime = mKeys.front().mTime;
	float aLastTime = mKeys.back().mTime;
	mQuantized.clear();
	mQuantized.resize(256, 0.0f);

	double aSpan = (double)aLastTime - (double)aFirstTime;
	float aStep = (float)(aSpan * 0.00390625 * 0.5);
	float aTime = aFirstTime;
	bool aFirst = true;
	int aKey = 0;
	int aPreviousIndex = 0;
	float aPreviousValue = 0.0f;

	for (;;) {
		PIPoint aPoint = mBezier.Evaluate(aTime);
		int anIndex = Ftol((aPoint.mX - (double)aFirstTime) / aSpan * 255.0 + 0.5);

		// the keyframe segment the curve is in; past the last one the keyframes themselves are written and the
		// walk ends
		for (;;) {
			if ((double)mKeys[aKey + 1].mTime > aPoint.mX)
				break;
			aKey++;
			if (aKey >= (int)mKeys.size() - 1) {
				for (size_t i = 0; i < mKeys.size(); i++) {
					int anEntry = Ftol(((double)mKeys[i].mTime - (double)aFirstTime) / aSpan * 255.0 + 0.5);
					mQuantized[anEntry] = mKeys[i].mValue;
				}
				return;
			}
		}

		if ((double)mKeys[aKey].mTime <= aPoint.mX) {
			if (aFirst || anIndex <= aPreviousIndex + 1) {
				mQuantized[anIndex] = (float)aPoint.mY;
			}
			else if (aPreviousIndex <= anIndex) {
				int aCount = anIndex - aPreviousIndex;
				for (int i = 0; aPreviousIndex + i <= anIndex; i++) {
					float aFraction = (float)i / (float)aCount;
					mQuantized[aPreviousIndex + i] =
						(float)((1.0 - (double)aFraction) * (double)aPreviousValue + (double)aFraction * aPoint.mY);
				}
			}
			aPreviousValue = (float)aPoint.mY;
			aPreviousIndex = anIndex;
		}
		aFirst = false;
		aTime = (float)((double)aStep + (double)aTime);
	}
}

// 0x566360 (bejeweled3)
float PIValue::GetValueAt(float theTime, float theDefault)
{
	if (theTime == mLastTime)
		return mLastValue;

	float anOldTime = mLastTime;
	mLastTime = theTime;

	if (mKeys.size() == 1) {
		mLastValue = mKeys[0].mValue;
		return mLastValue;
	}

	if (mBezier.mPointCount < 1) {
		// straight between the keyframes
		for (int i = 1; i < (int)mKeys.size(); i++) {
			const KeyFrame& aFrom = mKeys[i - 1];
			const KeyFrame& aTo = mKeys[i];
			if ((aFrom.mTime < theTime && theTime <= aTo.mTime) || i == (int)mKeys.size() - 1) {
				double aFraction = ((double)theTime - (double)aFrom.mTime) / ((double)aTo.mTime - (double)aFrom.mTime);
				if (1.0 < aFraction)
					aFraction = 1.0;
				mLastValue = (float)(((double)aTo.mValue - (double)aFrom.mValue) * aFraction + (double)aFrom.mValue);
				return mLastValue;
			}
		}
		mLastValue = theDefault;
		return mLastValue;
	}

	float aLow = mKeys.front().mTime;
	float aHigh = mKeys.back().mTime;

	if (aHigh < 1.001f) {
		// a curve over the unit span: read it from the sampled table
		if (mQuantized.empty())
			QuantizeCurve();
		float aPosition = (float)(((double)theTime - (double)aLow) / ((double)aHigh - (double)aLow) * 255.0 + 0.5);
		if (aPosition < 0.0f) {
			mLastValue = mKeys.front().mValue;
			return mLastValue;
		}
		if (255.0f <= aPosition) {
			mLastValue = mKeys.back().mValue;
			return mLastValue;
		}
		int anIndex = Ftol(aPosition);
		float aFraction = (float)((double)aPosition - (double)anIndex);
		mLastValue = (float)((1.0 - (double)aFraction) * (double)mQuantized[anIndex] +
		                     (double)mQuantized[anIndex + 1] * (double)aFraction);
		return mLastValue;
	}

	// a longer span: search along the curve for the point at theTime
	double aToleranceValue = ((double)aHigh - (double)aLow) / 1000.0;
	if ((double)0.1f < aToleranceValue)
		aToleranceValue = (double)0.1f;
	float aTolerance = (float)aToleranceValue;
	if (theTime < aLow) {
		mLastValue = mKeys.front().mValue;
		return mLastValue;
	}
	if (aHigh <= theTime) {
		mLastValue = mKeys.back().mValue;
		return mLastValue;
	}

	static const float aToleranceScale[4] = {0.1f, 0.1f, 0.1f, 0.5f};
	static const float aGuessScale[3] = {1.0f, 0.75f, 1.25f};
	double aValue = 0.0;
	bool aFarFromLast = ((double)theTime - (double)anOldTime) / ((double)aHigh - (double)aLow) > (double)0.05f;
	float aGuess = aLow;
	for (int i = 0; i < 1000; i++) {
		float aCurrentTolerance = aTolerance;
		if (i < 4 && !aFarFromLast)
			aCurrentTolerance = (float)((double)aTolerance * (double)aToleranceScale[i]);
		if (i < 3 && mParamStep != 0.0f && !aFarFromLast)
			aGuess = (float)((double)aGuessScale[i] * (double)mParamStep + (double)mLastParam);
		else
			aGuess = (float)(((double)aHigh - (double)aLow) * 0.5 + (double)aLow);
		if (aLow <= aGuess && aGuess <= aHigh) {
			PIPoint aPoint = mBezier.Evaluate(aGuess);
			aValue = aPoint.mY;
			float aDifference = (float)(aPoint.mX - (double)theTime);
			if (!(aCurrentTolerance < std::fabs(aDifference)))
				break;
			if (0.0f <= aDifference)
				aHigh = aGuess;
			else
				aLow = aGuess;
		}
	}
	mParamStep = (float)(((double)aGuess - (double)mLastParam) * 0.5 + 0.5 * (double)mParamStep);
	mLastParam = aGuess;
	mLastValue = (float)aValue;
	return mLastValue;
}

// 0x566810 (bejeweled3)
float PIValue::GetLastKeyValue(float theTime) const
{
	for (int i = (int)mKeys.size() - 1; i >= 0; i--) {
		if (mKeys[i].mTime <= theTime)
			return mKeys[i].mValue;
	}
	return 0.0f;
}

// 0x566860 (bejeweled3)
float PIValue::GetLastKeyTime(float theTime) const
{
	for (int i = (int)mKeys.size() - 1; i >= 0; i--) {
		if (mKeys[i].mTime <= theTime)
			return mKeys[i].mTime;
	}
	return 0.0f;
}

// 0x5668b0 (bejeweled3)
float PIValue::GetNextKeyTime(float theTime) const
{
	for (size_t i = 0; i < mKeys.size(); i++) {
		if (theTime < mKeys[i].mTime)
			return mKeys[i].mTime;
	}
	return 0.0f;
}

// 0x566910 (bejeweled3)
int PIValue::GetNextKeyIndex(float theTime) const
{
	for (size_t i = 0; i < mKeys.size(); i++) {
		if (theTime < mKeys[i].mTime)
			return (int)i;
	}
	return -1;
}

// The constructor (not found as its own function: the compiler built it into its owners)
PIValue2D::PIValue2D()
	: mLastTime(-1.0f), mLastX(0.0), mLastY(0.0), mLastVelocityTime(-1.0f), mLastVelocityX(0.0), mLastVelocityY(0.0)
{
}

// 0x566b10 (bejeweled3)
PIPoint PIValue2D::GetValueAt(float theTime)
{
	if (theTime != mLastTime) {
		mLastTime = theTime;
		size_t aCount = mKeys.size();
		if (aCount == 1) {
			mLastX = mKeys[0].mX;
			mLastY = mKeys[0].mY;
		}
		else if (mBezier.mPointCount < 1) {
			mLastX = 0.0;
			mLastY = 0.0;
			for (size_t i = 1; i < aCount; i++) {
				const KeyFrame& aFrom = mKeys[i - 1];
				const KeyFrame& aTo = mKeys[i];
				double aTimeNow = (double)theTime;
				if (((double)aFrom.mTime < aTimeNow && aTimeNow <= (double)aTo.mTime) || i == aCount - 1) {
					double aFraction = (aTimeNow - (double)aFrom.mTime) / ((double)aTo.mTime - (double)aFrom.mTime);
					if (1.0 < aFraction)
						aFraction = 1.0;
					mLastY = aFrom.mY + (aTo.mY - aFrom.mY) * aFraction;
					mLastX = aFraction * (aTo.mX - aFrom.mX) + aFrom.mX;
					break;
				}
			}
		}
		else {
			PIPoint aPoint = mBezier.Evaluate(theTime);
			mLastX = aPoint.mX;
			mLastY = aPoint.mY;
		}
	}
	PIPoint aResult = {mLastX, mLastY};
	return aResult;
}

// 0x566cc0 (bejeweled3)
PIPoint PIValue2D::GetVelocityAt(float theTime)
{
	if (theTime != mLastVelocityTime) {
		mLastVelocityTime = theTime;
		size_t aCount = mKeys.size();
		if (aCount < 2) {
			PIPoint aZero = {0.0, 0.0};
			return aZero;
		}
		if (mBezier.mPointCount < 1) {
			mLastVelocityX = 0.0;
			mLastVelocityY = 0.0;
			for (size_t i = 1; i < aCount; i++) {
				const KeyFrame& aFrom = mKeys[i - 1];
				const KeyFrame& aTo = mKeys[i];
				if ((aFrom.mTime < theTime && theTime <= aTo.mTime) || i == aCount - 1) {
					mLastVelocityX = aTo.mX - aFrom.mX;
					mLastVelocityY = aTo.mY - aFrom.mY;
					break;
				}
			}
		}
		else {
			PIPoint aVelocity = mBezier.Velocity(theTime);
			mLastVelocityX = aVelocity.mX;
			mLastVelocityY = aVelocity.mY;
		}
	}
	PIPoint aResult = {mLastVelocityX, mLastVelocityY};
	return aResult;
}

}
