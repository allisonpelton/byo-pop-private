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

namespace
{

// 0x435850 and 0x4183b0 (feedingfrenzy2): the curve of the stretch from the key at the first time, leaving it at its
// out speed over its out time, to the one at the second, reaching it at its in speed over its in time
void setStretchCurve(EaseCurve& theCurve, float theStartTime, float theOutSpeed, float theOutTime, float theEndTime,
	float theInSpeed, float theInTime)
{
	float aSpan = (float)((double)theEndTime - theStartTime);
	float anInverse = (float)(1.0 / aSpan);
	float anAccelEnd = (float)((double)theOutTime * anInverse);
	float aDecelStart = (float)(1.0 - (double)theInTime * anInverse);
	theCurve.set(theOutSpeed, theInSpeed, anAccelEnd, maxOf(anAccelEnd, aDecelStart));
}

} // namespace

// 0x4283f0 and 0x4280c0 (feedingfrenzy2): the key's out speed and time and the next one's in speed and time for the
// stretch's ease
template <class KeyValue>
void easeKeys(float theStretch, KeyValue& theKey, KeyValue& theNext, bool theEaseOut, bool theEaseIn,
	float theStraightShare)
{
	float anEased = theStretch - theStretch * theStraightShare;
	if (!theEaseOut)
	{
		theKey.mOutSpeed = 1.0f;
		theKey.mOutTime = 0.0f;
		theNext.mInSpeed = theEaseIn ? 0.0f : 1.0f;
		theNext.mInTime = theEaseIn ? anEased : 0.0f;
		return;
	}
	theKey.mOutSpeed = 0.0f;
	if (theEaseIn)
	{
		anEased = anEased * 0.5f;
		theKey.mOutTime = anEased;
		theNext.mInSpeed = 0.0f;
		theNext.mInTime = anEased;
		return;
	}
	theKey.mOutTime = anEased;
	theNext.mInSpeed = 1.0f;
	theNext.mInTime = 0.0f;
}

void FloatKeyInterpolator::easeStretch(int theIndex, bool theEaseOut, bool theEaseIn, float theStraightShare)
{
	KeyMap::iterator aKey = mKeys.begin();
	for (int anIndex = 0; anIndex < theIndex; anIndex++)
		++aKey;
	KeyMap::iterator aNext = aKey;
	++aNext;
	easeKeys(aNext->first - aKey->first, aKey->second, aNext->second, theEaseOut, theEaseIn, theStraightShare);
}

void FloatKeyInterpolator::setCurve(KeyMap::iterator theCurrent, KeyMap::iterator theNext)
{
	setStretchCurve(mCurve, theCurrent->first, theCurrent->second.mOutSpeed, theCurrent->second.mOutTime,
		theNext->first, theNext->second.mInSpeed, theNext->second.mInTime);
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

void ColorKeyInterpolator::addKey(const ColorKey& theKey)
{
	KeyValue aValue;
	aValue.mValue = theKey.mValue;
	aValue.mInTime = theKey.mInTime;
	aValue.mInSpeed = theKey.mInSpeed;
	aValue.mOutTime = theKey.mOutTime;
	aValue.mOutSpeed = theKey.mOutSpeed;
	mCurrent = mKeys.insert(KeyMap::value_type(theKey.mTime, aValue)).first;
}

void ColorKeyInterpolator::clear()
{
	mKeys.clear();
	mCurrent = mKeys.end();
}

void ColorKeyInterpolator::setCurve(KeyMap::iterator theCurrent, KeyMap::iterator theNext)
{
	setStretchCurve(mCurve, theCurrent->first, theCurrent->second.mOutSpeed, theCurrent->second.mOutTime,
		theNext->first, theNext->second.mInSpeed, theNext->second.mInTime);
}

Color ColorKeyInterpolator::getValue(float theTime)
{
	// With no keys the original's checked iterator fails (no caller reads an empty one)
	if (mKeys.empty())
		return Color();
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
	float aSpan = (float)((double)aNext->first - mCurrent->first);
	float aFraction = (float)(((double)theTime - mCurrent->first) / aSpan);
	float anEased = (float)mCurve.get(aFraction);
	const Color& aFrom = mCurrent->second.mValue;
	const Color& aTo = aNext->second.mValue;
	return Color((float)(((double)aTo.r - aFrom.r) * anEased + aFrom.r),
		(float)(((double)aTo.g - aFrom.g) * anEased + aFrom.g), (float)(((double)aTo.b - aFrom.b) * anEased + aFrom.b),
		(float)(((double)aTo.a - aFrom.a) * anEased + aFrom.a));
}

void Vector2KeyInterpolator::addKey(const Vector2Key& theKey)
{
	KeyValue aValue;
	aValue.mValue = theKey.mValue;
	aValue.mInTime = theKey.mInTime;
	aValue.mInSpeed = theKey.mInSpeed;
	aValue.mOutTime = theKey.mOutTime;
	aValue.mOutSpeed = theKey.mOutSpeed;
	mCurrent = mKeys.insert(KeyMap::value_type(theKey.mTime, aValue)).first;
}

void Vector2KeyInterpolator::clear()
{
	mKeys.clear();
	mCurrent = mKeys.end();
}

void Vector2KeyInterpolator::easeStretch(int theIndex, bool theEaseOut, bool theEaseIn, float theStraightShare)
{
	KeyMap::iterator aKey = mKeys.begin();
	for (int anIndex = 0; anIndex < theIndex; anIndex++)
		++aKey;
	KeyMap::iterator aNext = aKey;
	++aNext;
	easeKeys(aNext->first - aKey->first, aKey->second, aNext->second, theEaseOut, theEaseIn, theStraightShare);
}

void Vector2KeyInterpolator::setCurve(KeyMap::iterator theCurrent, KeyMap::iterator theNext)
{
	setStretchCurve(mCurve, theCurrent->first, theCurrent->second.mOutSpeed, theCurrent->second.mOutTime,
		theNext->first, theNext->second.mInSpeed, theNext->second.mInTime);
}

Vector2f Vector2KeyInterpolator::getValue(float theTime)
{
	// With no keys the original's checked iterator fails (no caller reads an empty one)
	if (mKeys.empty())
		return Vector2f();
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
	float aSpan = (float)((double)aNext->first - mCurrent->first);
	float aFraction = (float)(((double)theTime - mCurrent->first) / aSpan);
	float anEased = (float)mCurve.get(aFraction);
	const Vector2f& aFrom = mCurrent->second.mValue;
	const Vector2f& aTo = aNext->second.mValue;
	return Vector2f((float)(((double)aTo.x - aFrom.x) * anEased + aFrom.x),
		(float)(((double)aTo.y - aFrom.y) * anEased + aFrom.y));
}

} // namespace engine
