#include "KeyInterpolator.h"

#include "Geometry.h"

namespace engine
{

void EaseCurve::set(float theStartSpeed, float theEndSpeed, float theAccelEnd, float theDecelStart)
{
	mStartSpeed = theStartSpeed;
	mEndSpeed = theEndSpeed;
	mAccelEnd = theAccelEnd;
	mDecelStart = theDecelStart;
	// The original rounds the two lengths to floats (it keeps them in its arguments' slots) and keeps the rest on the
	// x87 stack
	float aDecelLength = (float)(1.0 - theDecelStart);
	float aRunLength = (float)((double)theDecelStart - theAccelEnd);
	double aRunSpeed = ((2.0 - (double)theStartSpeed * theAccelEnd) - (double)aDecelLength * theEndSpeed) /
		((double)aRunLength + 1.0);
	mRunSpeed = (float)aRunSpeed;
	double anAccel = 0.0;
	if (theAccelEnd != 0.0f)
		anAccel = (aRunSpeed - theStartSpeed) / ((double)theAccelEnd * 2.0);
	mAccel = (float)anAccel;
	double aDecel = 0.0;
	if (theDecelStart != 1.0f)
		aDecel = ((double)theEndSpeed - aRunSpeed) / (2.0 * aDecelLength);
	mDecel = (float)aDecel;
	double anAccelValue = (anAccel * theAccelEnd + theStartSpeed) * theAccelEnd;
	mAccelValue = (float)anAccelValue;
	mDecelValue = (float)(aRunSpeed * aRunLength + anAccelValue);
}

double EaseCurve::get(double theTime) const
{
	if (theTime < mAccelEnd)
		return ((double)mAccel * theTime + mStartSpeed) * theTime;
	if (theTime >= mDecelStart)
	{
		double aTime = theTime - mDecelStart;
		return ((double)mDecel * aTime + mRunSpeed) * aTime + mDecelValue;
	}
	return (theTime - mAccelEnd) * (double)mRunSpeed + mAccelValue;
}

void FloatKeyInterpolator::addKey(const FloatKey& theKey)
{
	KeyValue aValue;
	aValue.mValue = theKey.mValue;
	aValue.mInTime = theKey.mInTime;
	aValue.mInSpeed = theKey.mInSpeed;
	aValue.mOutTime = theKey.mOutTime;
	aValue.mOutSpeed = theKey.mOutSpeed;
	mCurrent = mKeys.insert(KeyMap::value_type(theKey.mTime, aValue)).first;
}

void FloatKeyInterpolator::clear()
{
	mKeys.clear();
	mCurrent = mKeys.end();
}

void FloatKeyInterpolator::setCurve(KeyMap::iterator theCurrent, KeyMap::iterator theNext)
{
	float aStartSpeed = theCurrent->second.mOutSpeed;
	float anEndSpeed = theNext->second.mInSpeed;
	float aSpan = (float)((double)theNext->first - theCurrent->first);
	float anInverse = (float)(1.0 / aSpan);
	float anAccelEnd = (float)((double)theCurrent->second.mOutTime * anInverse);
	float aDecelStart = (float)(1.0 - (double)theNext->second.mInTime * anInverse);
	mCurve.set(aStartSpeed, anEndSpeed, anAccelEnd, maxOf(anAccelEnd, aDecelStart));
}

double FloatKeyInterpolator::getValue(float theTime)
{
	// With no keys the original's checked iterator fails (no caller reads an empty one)
	if (mKeys.empty())
		return 0.0;
	KeyMap::iterator aNext = mCurrent;
	if (aNext != mKeys.end())
		++aNext;
	bool aMoved = false;
	while (aNext != mKeys.end() && theTime > aNext->first)
	{
		mCurrent = aNext;
		++aNext;
		aMoved = true;
	}
	while (mCurrent != mKeys.begin() && theTime < mCurrent->first)
	{
		aNext = mCurrent;
		--mCurrent;
		aMoved = true;
	}
	if (aNext == mKeys.end())
		return mCurrent->second.mValue;
	if (aMoved)
		setCurve(mCurrent, aNext);
	if (theTime < mCurrent->first)
		return mCurrent->second.mValue;
	double aFraction = ((double)theTime - mCurrent->first) / ((double)aNext->first - mCurrent->first);
	double anEased = mCurve.get(aFraction);
	return ((double)aNext->second.mValue - mCurrent->second.mValue) * anEased + mCurrent->second.mValue;
}

} // namespace engine
