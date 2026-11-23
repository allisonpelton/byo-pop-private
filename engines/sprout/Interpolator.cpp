#include "Interpolator.h"
#include "Node.h"

using namespace engine;

// 0x494710 (pizzafrenzy): the run speed makes the whole curve end at 1
void EaseCurve::set(float theStartSpeed, float theEndSpeed, float theAccelEnd, float theDecelStart)
{
	// the decel time is stored; the run speed, the acceleration and the accel value are stored and also used unrounded
	float aDecelTime = 1.0f - theDecelStart;
	mStartSpeed = theStartSpeed;
	mAccelEnd = theAccelEnd;
	mEndSpeed = theEndSpeed;
	mDecelStart = theDecelStart;
	double aRunSpeed = ((2.0 - (double)theStartSpeed * theAccelEnd) - (double)aDecelTime * theEndSpeed) /
	                   (((double)theDecelStart - theAccelEnd) + 1.0);
	mRunSpeed = (float)aRunSpeed;
	double anAccel;
	if (theAccelEnd == 0.0f)
		anAccel = 0.0;
	else
		anAccel = (aRunSpeed - theStartSpeed) / ((double)theAccelEnd + theAccelEnd);
	mAccel = (float)anAccel;
	if (theDecelStart == 1.0f)
		mDecel = 0.0f;
	else
		mDecel = (float)((theEndSpeed - aRunSpeed) / ((double)aDecelTime + aDecelTime));
	double anAccelValue = (anAccel * theAccelEnd + theStartSpeed) * theAccelEnd;
	mAccelValue = (float)anAccelValue;
	mDecelValue = (float)(aRunSpeed * ((double)theDecelStart - theAccelEnd) + anAccelValue);
}

// 0x4947e0 (pizzafrenzy)
double EaseCurve::get(float theTime) const
{
	if (theTime < mAccelEnd)
		return ((double)theTime * mAccel + mStartSpeed) * theTime;
	if (theTime < mDecelStart)
		return ((double)theTime - mAccelEnd) * mRunSpeed + mAccelValue;
	double aTime = (double)theTime - mDecelStart;
	return (aTime * mDecel + mRunSpeed) * aTime + mDecelValue;
}

// 0x42c890 (pizzafrenzy), 0x480eb0 (pizzafrenzy)
PositionInterpolator::PositionInterpolator() : mCurrent(0)
{
}

// 0x480c70 (pizzafrenzy)
void PositionInterpolator::addKey(const PositionKey& theKey)
{
	size_t i = 0;
	while (i < mKeys.size() && mKeys[i].mTime < theKey.mTime)
		i++;
	if (i == mKeys.size() || theKey.mTime < mKeys[i].mTime)
		mKeys.insert(mKeys.begin() + i, theKey);
	mCurrent = i;
}

// 0x47fe60 (pizzafrenzy)
void PositionInterpolator::clear()
{
	mKeys.clear();
	mCurrent = 0;
}

// 0x47fea0 (pizzafrenzy)
Point PositionInterpolator::get(float theTime)
{
	size_t anEnd = mKeys.size();
	size_t aCurrent = mCurrent;
	bool aMoved = false;
	size_t aNext = aCurrent < anEnd ? aCurrent + 1 : anEnd;
	if (aNext != anEnd)
	{
		do
		{
			if (theTime <= mKeys[aNext].mTime)
				break;
			aCurrent = aNext;
			aNext++;
			aMoved = true;
		} while (aNext != anEnd);
	}
	bool aMovedBack = aMoved;
	if (aCurrent != 0)
	{
		do
		{
			aMoved = aMovedBack;
			if (mKeys[aCurrent].mTime <= theTime)
				break;
			aNext = aCurrent;
			aCurrent--;
			aMoved = true;
			aMovedBack = true;
		} while (aCurrent != 0);
	}
	mCurrent = aCurrent;
	if (anEnd == 0)
		return Point(0, 0);
	if (aNext == anEnd)
		return mKeys[aCurrent].mValue;
	if (aMoved)
		setCurve(mKeys[aCurrent], mKeys[aNext]);
	const PositionKey& aKey = mKeys[aCurrent];
	if (theTime < aKey.mTime)
		return aKey.mValue;
	// the stretch's time and the curve's value are stored as floats; each coordinate is worked out unrounded
	float aTime = (float)(((double)theTime - aKey.mTime) / ((double)mKeys[aNext].mTime - aKey.mTime));
	float aValue = (float)mCurve.get(aTime);
	const PositionKey& aNextKey = mKeys[aNext];
	return Point((float)(((double)aNextKey.mValue.x - aKey.mValue.x) * aValue + aKey.mValue.x),
	             (float)(((double)aNextKey.mValue.y - aKey.mValue.y) * aValue + aKey.mValue.y));
}

// 0x42c930 (pizzafrenzy)
void PositionInterpolator::apply(Node* theNode, const Point& theValue)
{
	theNode->setPosition(theValue);
}

// 0x47fc70 (pizzafrenzy)
void PositionInterpolator::setCurve(const PositionKey& theKey, const PositionKey& theNext)
{
	// the scale is used unrounded; the two ends are passed as floats
	double aScale = 1.0 / ((double)theNext.mTime - theKey.mTime);
	float anAccelEnd = (float)(aScale * theKey.mOutTime);
	double aDecelStart = 1.0 - aScale * theNext.mInTime;
	float aStart = anAccelEnd;
	if (anAccelEnd < aDecelStart)
		aStart = (float)aDecelStart;
	mCurve.set(theKey.mOutSpeed, theNext.mInSpeed, anAccelEnd, aStart);
}

// 0x480c40 (pizzafrenzy)
void ScalarInterpolator::addKey(const ScalarKey& theKey)
{
	size_t i = 0;
	while (i < mKeys.size() && mKeys[i].mTime < theKey.mTime)
		i++;
	if (i == mKeys.size() || theKey.mTime < mKeys[i].mTime)
		mKeys.insert(mKeys.begin() + i, theKey);
	mCurrent = i;
}

// 0x47fcf0 (pizzafrenzy)
void ScalarInterpolator::clear()
{
	mKeys.clear();
	mCurrent = 0;
}

// 0x47fd30 (pizzafrenzy)
float ScalarInterpolator::get(float theTime)
{
	size_t anEnd = mKeys.size();
	size_t aCurrent = mCurrent;
	bool aMoved = false;
	size_t aNext = aCurrent < anEnd ? aCurrent + 1 : anEnd;
	if (aNext != anEnd)
	{
		do
		{
			if (theTime <= mKeys[aNext].mTime)
				break;
			aCurrent = aNext;
			aNext++;
			aMoved = true;
		} while (aNext != anEnd);
	}
	bool aMovedBack = aMoved;
	if (aCurrent != 0)
	{
		do
		{
			aMoved = aMovedBack;
			if (mKeys[aCurrent].mTime <= theTime)
				break;
			aNext = aCurrent;
			aCurrent--;
			aMoved = true;
			aMovedBack = true;
		} while (aCurrent != 0);
	}
	mCurrent = aCurrent;
	if (anEnd == 0)
		return 0.0f;
	if (aNext == anEnd)
		return mKeys[aCurrent].mValue;
	if (aMoved)
		setCurve(mKeys[aCurrent], mKeys[aNext]);
	const ScalarKey& aKey = mKeys[aCurrent];
	if (theTime < aKey.mTime)
		return aKey.mValue;
	// the stretch's time is stored as a float; the curve's value is used unrounded (unlike the position's blend)
	float aTime = (float)(((double)theTime - aKey.mTime) / ((double)mKeys[aNext].mTime - aKey.mTime));
	double aValue = mCurve.get(aTime);
	return (float)(((double)mKeys[aNext].mValue - aKey.mValue) * aValue + aKey.mValue);
}

// 0x47fbf0 (pizzafrenzy)
void ScalarInterpolator::setCurve(const ScalarKey& theKey, const ScalarKey& theNext)
{
	// the scale is used unrounded; the two ends are passed as floats
	double aScale = 1.0 / ((double)theNext.mTime - theKey.mTime);
	float anAccelEnd = (float)(aScale * theKey.mOutTime);
	double aDecelStart = 1.0 - aScale * theNext.mInTime;
	float aStart = anAccelEnd;
	if (anAccelEnd < aDecelStart)
		aStart = (float)aDecelStart;
	mCurve.set(theKey.mOutSpeed, theNext.mInSpeed, anAccelEnd, aStart);
}

// 0x42c7b0 (pizzafrenzy)
void ScaleInterpolator::apply(Node* theNode, float theValue)
{
	theNode->setScale(theValue);
}

// 0x42c870 (pizzafrenzy)
void OpacityInterpolator::apply(Node* theNode, float theValue)
{
	theNode->setOpacity(theValue);
}
