// Sexy::CurvedVal (CurvedVal.h). The original computes on the x87 and stores floats and doubles where its fields are;
// the port computes in double and stores at the same places. In the 3D mode the x87 runs in single precision (X87
// below), and the input's steps round as the original's do.
#include "CurvedVal.h"

#include "BSpline.h"
#include "SexyAppBase.h"
#include "X86Math.h"

#include <cmath>

namespace Sexy {

bool gCurveSinglePrecision = false;

std::map<std::string, CurvedVal::CurveCacheRecord*> CurvedVal::mCurveCacheMap;

namespace {

// With gCurveSinglePrecision each sum or product of a curve's input is rounded to a float; without it the x87 keeps
// double precision
double X87(double theValue)
{
	return gCurveSinglePrecision ? (double)(float)theValue : theValue;
}

// 0x43b020 (bejeweled3): a number of a curve string, its digits summed on the x87 (the fraction's place value starts
// at 0.1 and is divided by 10 for each digit); 0 when the text is not a plain decimal number
double CurveNumber(const std::string& theString)
{
	double aValue = 0.0;
	if (theString.empty())
		return 0.0;
	size_t i = theString[0] == '-' ? 1 : 0;
	for (; i < theString.length(); i++) {
		char aChar = theString[i];
		if (aChar >= '0' && aChar <= '9')
			aValue = X87(X87(aValue * 10.0) + (double)(aChar - '0'));
		else if (aChar == '.') {
			i++;
			break;
		} else
			return 0.0;
	}
	double aPlace = 0.1;
	for (; i < theString.length(); i++) {
		char aChar = theString[i];
		if (aChar < '0' || aChar > '9')
			return 0.0;
		aValue = X87(X87((double)(aChar - '0') * aPlace) + aValue);
		aPlace = X87(aPlace / 10.0);
	}
	return theString[0] == '-' ? -aValue : aValue;
}

// 0x436870 (bejeweled3): '#' is 0 and '~' is 1, skipping the backslash
float CVCharToFloat(char c)
{
	if (c > '[')
		c--;
	return (float)((double)(c - '#') / 90.0);
}

// 0x4368a0 (bejeweled3)
int CVCharToInt(char c)
{
	if (c > '[')
		c--;
	return c - '#';
}

// 0x4368b0 (bejeweled3): three digits base 90, in degrees
float CVStrToAngle(const std::string& theString)
{
	int aValue = (CVCharToInt(theString[0]) * 90 + CVCharToInt(theString[1])) * 90 + CVCharToInt(theString[2]);
	return (float)((double)aValue * 360.0 / 729000.0);
}

float Clamp01(float theValue)
{
	if (theValue <= 0.0f)
		return 0.0f;
	if (theValue >= 1.0f)
		return 1.0f;
	return theValue;
}

// The original's x step: 0.1f widened to a double
const double CURVE_X_STEP = 0.10000000149011612;

}  // namespace

// Samples the spline through the points into the table, filling gaps by straight lines
void CurvedVal::GenerateTable(const std::vector<DataPoint>& theDataPoints, float* theTable, int theSize, bool theNoClip)
{
	BSpline aSpline;
	for (const DataPoint& aPoint : theDataPoints)
		aSpline.AddPoint(aPoint.mX, aPoint.mY);
	aSpline.CalculateSpline();
	double aScale = (double)(theSize - 1);
	bool aFirst = true;
	float aLastY = 0.0f;
	int aLastIndex = 0;
	for (int i = 1; i < (int)theDataPoints.size(); i++) {
		int aStart = Ftol((double)theDataPoints[i - 1].mX * aScale + 0.5);
		int anEnd = Ftol((double)theDataPoints[i].mX * aScale + 0.5);
		if (aStart > anEnd)
			continue;
		float aSpan = (float)(anEnd - aStart);
		float aBase = (float)(i - 1);
		for (int aStep = 0; aStep <= anEnd - aStart; aStep++) {
			float t = (float)((double)(float)aStep / aSpan + aBase);
			float y = aSpline.GetYPoint(t);
			int anIndex = Ftol((double)aSpline.GetXPoint(t) * aScale + 0.5);
			if (anIndex < aLastIndex || anIndex > anEnd)
				continue;
			if (!aFirst) {
				if (aLastIndex + 1 < anIndex) {
					float aGap = (float)(anIndex - aLastIndex);
					for (int k = 0; aLastIndex + k <= anIndex; k++) {
						float f = (float)((double)(float)k / aGap);
						float aValue = (float)((1.0 - f) * aLastY + (double)f * y);
						theTable[aLastIndex + k] = theNoClip ? aValue : Clamp01(aValue);
					}
				} else {
					theTable[anIndex] = theNoClip ? y : Clamp01(y);
				}
			}
			aFirst = false;
			aLastY = y;
			aLastIndex = anIndex;
		}
	}
	for (const DataPoint& aPoint : theDataPoints)
		theTable[Ftol((double)aPoint.mX * aScale + 0.5)] = aPoint.mY;
}

CurvedVal::CurvedVal()
{
	mMode = MODE_CLAMP;
	mRamp = RAMP_NONE;
	mIncRate = 0.0;
	mOutMin = 0.0;
	mOutMax = 1.0;
	mCurveRef = nullptr;
	mCurveRefLast = nullptr;
	mInitAppUpdateCount = 0;
	mAppUpdateCountSrc = gSexyAppBase ? &gSexyAppBase->mUpdateCount : nullptr;
	mLinkedVal = nullptr;
	mCurveCacheRecord = nullptr;
	mOutVal = 0.0;
	mPrevOutVal = 0.0;
	mInMin = 0.0;
	mInMax = 1.0;
	mNoClip = false;
	mSingleTrigger = false;
	mOutputSync = false;
	mTriggered = false;
	mIsHermite = false;
	mAutoInc = false;
	mPrevInVal = 0.0;
	mInVal = 0.0;
}

CurvedVal::CurvedVal(const std::string& theCurve, CurvedVal* theLinkedVal) : CurvedVal()
{
	SetCurve(theCurve, theLinkedVal);
}

void CurvedVal::SetConstant(double theValue)
{
	mInVal = 0.0;
	mTriggered = false;
	mInMax = 0.0;
	mLinkedVal = nullptr;
	mInMin = 0.0;
	mRamp = RAMP_LINEAR;
	mOutMax = theValue;
	mOutMin = theValue;
}

void CurvedVal::SetCurve(const std::string& theCurve, CurvedVal* theLinkedVal)
{
	mCurveRef = nullptr;
	mCurveRefLast = nullptr;
	if (mAppUpdateCountSrc != nullptr)
		mInitAppUpdateCount = *mAppUpdateCountSrc;
	mTriggered = false;
	mLinkedVal = theLinkedVal;
	mRamp = RAMP_CURVEDATA;
	ParseDataString(theCurve);
	mInVal = mInMin;
}

void CurvedVal::SetCurveMult(const std::string& theCurve)
{
	double aValue = GetOutVal();
	SetCurve(theCurve, nullptr);
	mOutMax = aValue * mOutMax;
}

void CurvedVal::SetCurveSmooth(const std::string& theCurve, double theSearchInc)
{
	double aValue = GetOutVal();
	SetCurve(theCurve, nullptr);
	SetInVal(FindClosestInToOutVal(aValue, theSearchInc, 0.0, 1.0), true);
}

double CurvedVal::FindClosestInToOutVal(double theOutVal, double theInc, double theStartInVal, double theEndInVal)
{
	double aRange = mInMax - mInMin;
	double anEnd = theEndInVal * aRange + mInMin;
	double aBest = -1.0;
	double anInVal = aRange * theStartInVal + mInMin;
	if (!(anInVal <= anEnd))
		return aBest;
	double aStep = aRange * theInc;
	double aBestDiff = 0.0;
	do {
		double aDiff = std::fabs(theOutVal - GetOutVal(anInVal));
		if (aBest < 0.0 || aDiff < aBestDiff) {
			aBestDiff = aDiff;
			aBest = anInVal;
		}
		anInVal = aStep + anInVal;
	} while (anInVal <= anEnd);
	return aBest;
}

bool CurvedVal::CheckCurveChange()
{
	if (mCurveRef == nullptr || *mCurveRef == mCurveRefLast)
		return false;
	mCurveRefLast = *mCurveRef;
	ParseDataString(*mCurveRef);
	return true;
}

void CurvedVal::ParseDataString(const std::string& theString)
{
	mIncRate = 0.0;
	mOutMin = 0.0;
	mOutMax = 1.0;
	mSingleTrigger = false;
	mNoClip = false;
	mOutputSync = false;
	mIsHermite = false;
	mAutoInc = false;

	int aVersion = 0;
	if (!theString.empty() && theString[0] > '`' && theString[0] < 'c')
		aVersion = theString[0] - 'a';
	size_t aPos = 1;
	if (aVersion > 0) {
		int aFlags = CVCharToInt(theString[1]);
		mNoClip = (aFlags & 1) != 0;
		mSingleTrigger = (aFlags & 2) != 0;
		mOutputSync = (aFlags & 4) != 0;
		mIsHermite = (aFlags & 8) != 0;
		mAutoInc = (aFlags & 0x10) != 0;
		aPos = 2;
	}

	size_t aComma = theString.find(',', aPos);
	if (aComma == std::string::npos) {
		mIsHermite = true;
		return;
	}
	mOutMin = (float)CurveNumber(theString.substr(aPos, aComma - aPos));
	aPos = aComma + 1;
	aComma = theString.find(',', aPos);
	if (aComma == std::string::npos)
		return;
	mOutMax = (float)CurveNumber(theString.substr(aPos, aComma - aPos));
	aPos = aComma + 1;
	aComma = theString.find(',', aPos);
	if (aComma == std::string::npos)
		return;
	mIncRate = (float)CurveNumber(theString.substr(aPos, aComma - aPos));
	aPos = aComma + 1;
	if (aVersion > 0) {
		aComma = theString.find(',', aPos);
		if (aComma == std::string::npos)
			return;
		mInMax = (float)CurveNumber(theString.substr(aPos, aComma - aPos));
		aPos = aComma + 1;
	}

	std::string aCurveString = theString.substr(aPos);
	std::map<std::string, CurveCacheRecord*>::iterator anItr = mCurveCacheMap.find(aCurveString);
	if (anItr != mCurveCacheMap.end()) {
		mCurveCacheRecord = anItr->second;
		return;
	}
	// The original builds the record on its stack without clearing the table, so a table entry beyond the last point
	// is whatever was there; the port's is 0
	CurveCacheRecord* aRecord = new CurveCacheRecord();
	mCurveCacheMap[aCurveString] = aRecord;
	mCurveCacheRecord = aRecord;

	std::vector<DataPoint> aDataPoints;
	float x = 0.0f;
	size_t i = aPos;
	while (i < theString.size()) {
		DataPoint aPoint;
		aPoint.mX = x;
		aPoint.mY = CVCharToFloat(theString[i]);
		if (mIsHermite) {
			aPoint.mAngleDeg = CVStrToAngle(theString.substr(i + 1, 3));
			i += 4;
		} else {
			aPoint.mAngleDeg = 0.0f;
			i += 1;
		}
		aDataPoints.push_back(aPoint);
		while (i < theString.size()) {
			char aChar = theString[i++];
			if (aChar != ' ') {
				double aNext = (double)CVCharToFloat(aChar) * CURVE_X_STEP + x;
				x = (float)(aNext < 1.0 ? aNext : 1.0);
				break;
			}
			x = (float)(x + CURVE_X_STEP);
		}
	}
	GenerateTable(aDataPoints, aRecord->mTable, CURVE_TABLE_SIZE, mNoClip);
	for (const DataPoint& aPoint : aDataPoints) {
		float anAngle = (float)((double)aPoint.mAngleDeg * 3.1415927410125732 / 180.0);
		SexyMathHermite::SPoint aHermitePoint = {aPoint.mX, aPoint.mY, (float)std::tan((double)anAngle)};
		aRecord->mHermiteCurve.mPoints.push_back(aHermitePoint);
	}
	aRecord->mHermiteCurve.mIsBuilt = false;
}

bool CurvedVal::CheckClamping()
{
	CheckCurveChange();
	if (mMode == MODE_CLAMP) {
		if (mInVal < mInMin) {
			mInVal = mInMin;
			return false;
		}
		if (mInVal > mInMax) {
			mInVal = mInMax;
			return false;
		}
		return true;
	}
	if (mMode == MODE_REPEAT || mMode == MODE_PING_PONG) {
		if (mInVal <= mInMax && mInMin <= mInVal)
			return true;
		mInVal = std::fmod((mInVal - mInMin) + (mInMax - mInMin), mInMax - mInMin) + mInMin;
	}
	return true;
}

double CurvedVal::GetOutVal(double theInVal)
{
	double aRange = mInMax - mInMin;
	double aValue;
	switch (mRamp) {
	case RAMP_NONE:
	case RAMP_LINEAR:
		if (mMode == MODE_PING_PONG) {
			double f = (theInVal - mInMin) / aRange;
			if (aRange * 0.5 < theInVal - mInMin)
				f = 1.0 - f;
			double d = (mOutMax - mOutMin) * f;
			return d + d + mOutMin;
		}
		if (mInMax == mInMin)
			return mOutMin;
		return (mOutMax - mOutMin) * ((theInVal - mInMin) / aRange) + mOutMin;
	case RAMP_SLOW_TO_FAST: {
		double a = (theInVal - mInMin) / aRange * 3.14159 * 0.5;
		if (mMode == MODE_PING_PONG)
			a = a + a;
		if (1.570795 < a)
			a = 3.14159 - a;
		aValue = 1.0 - std::cos(a);
		break;
	}
	case RAMP_FAST_TO_SLOW: {
		double a = (theInVal - mInMin) / aRange * 3.14159 * 0.5;
		if (mMode == MODE_PING_PONG)
			a = a + a;
		aValue = std::sin(a);
		break;
	}
	case RAMP_SLOW_FAST_SLOW: {
		double a = (theInVal - mInMin) / aRange * 3.14159;
		if (mMode == MODE_PING_PONG)
			a = a + a;
		aValue = (1.0 - std::cos(a)) * 0.5;
		break;
	}
	case RAMP_FAST_SLOW_FAST: {
		double a = (theInVal - mInMin) / aRange * 3.14159;
		if (mMode == MODE_PING_PONG)
			a = a + a;
		if (3.14159 < a)
			a = 6.28318 - a;
		if (1.570795 <= a)
			aValue = (2.0 - std::sin(a)) * 0.5;
		else
			aValue = std::sin(a) * 0.5;
		break;
	}
	case RAMP_CURVEDATA: {
		CheckCurveChange();
		if (mCurveCacheRecord == nullptr)
			return 0.0;
		if (aRange == 0.0)
			return 0.0;
		float aProgress = (float)((theInVal - mInMin) / aRange);
		if (!(aProgress < 1.0f))
			aProgress = 1.0f;
		if (mMode == MODE_PING_PONG) {
			if (aProgress <= 0.5f)
				aProgress = aProgress + aProgress;
			else
				aProgress = (float)((1.0 - aProgress) + (1.0 - aProgress));
		}
		if (mIsHermite) {
			double h = (mOutMax - mOutMin) * mCurveCacheRecord->mHermiteCurve.Evaluate(aProgress) + mOutMin;
			if (mNoClip)
				return h;
			double aLow = mOutMin < mOutMax ? mOutMin : mOutMax;
			double aHigh = mOutMin < mOutMax ? mOutMax : mOutMin;
			return h < aLow ? aLow : (h > aHigh ? aHigh : h);
		}
		// the table position is stored as a float first (Bejeweled Twist's stayed on the x87)
		float aPosition = (float)((double)aProgress * 8191.0);
		int anIndex = Ftol(aPosition);
		const float* aTable = mCurveCacheRecord->mTable;
		if (anIndex == CURVE_TABLE_SIZE - 1)
			return (mOutMax - mOutMin) * aTable[CURVE_TABLE_SIZE - 1] + mOutMin;
		float f = (float)((double)aPosition - anIndex);
		aValue = (1.0 - f) * aTable[anIndex] + (double)f * aTable[anIndex + 1];
		break;
	}
	default:
		return mOutMin;
	}
	return (mOutMax - mOutMin) * aValue + mOutMin;
}

double CurvedVal::GetInVal()
{
	double aValue = mInVal;
	if (mLinkedVal == nullptr) {
		if (mAutoInc) {
			if (mAppUpdateCountSrc != nullptr)
				aValue = X87(X87((double)(*mAppUpdateCountSrc - mInitAppUpdateCount) * mIncRate) + mInMin);
			if (mMode == MODE_REPEAT || mMode == MODE_PING_PONG)
				aValue = std::fmod(aValue - mInMin, mInMax - mInMin) + mInMin;
			else if (mInMax <= aValue)
				aValue = mInMax;
		}
	} else if (!mLinkedVal->mOutputSync) {
		aValue = mLinkedVal->GetInVal();
	} else {
		aValue = mLinkedVal->GetOutVal();
	}
	if (mMode == MODE_PING_PONG) {
		double f = (float)((aValue - mInMin) / (mInMax - mInMin));
		if (0.5 < f)
			f = 1.0 - f;
		aValue = (f + f) * (mInMax - mInMin) + mInMin;
	}
	return aValue;
}

double CurvedVal::GetOutVal()
{
	mOutVal = GetOutVal(GetInVal());
	return mOutVal;
}

bool CurvedVal::SetInVal(double theInVal, bool theRealign)
{
	mPrevOutVal = GetOutVal();
	mPrevInVal = theInVal;
	mTriggered = false;
	// an automatic curve's start moves by 100 updates for each unit the input moves back
	if (mAutoInc && theRealign)
		mInitAppUpdateCount += Ftol((theInVal - mInVal) * -100.0);
	mInVal = theInVal;
	if (CheckClamping())
		return true;
	if (!mTriggered) {
		mTriggered = true;
		return false;
	}
	return mSingleTrigger;
}

bool CurvedVal::IncInVal(double theInc)
{
	mPrevOutVal = GetOutVal();
	mPrevInVal = mInVal;
	mInVal = X87(mInVal + theInc);
	if (CheckClamping())
		return true;
	if (!mTriggered) {
		mTriggered = true;
		return false;
	}
	return mSingleTrigger;
}

bool CurvedVal::IncInVal()
{
	if (mIncRate == 0.0)
		return false;
	return IncInVal(mIncRate);
}

int CurvedVal::GetLengthInUpdates() const
{
	if (mIncRate == 0.0)
		return -1;
	return Ftol(std::ceil(X87(X87(mInMax - mInMin) / mIncRate)));
}

double CurvedVal::GetInValAtUpdate(int theUpdateCount) const
{
	return X87(X87(theUpdateCount * mIncRate) + mInMin);
}

bool CurvedVal::CheckInThreshold(double theInVal)
{
	double aCurrent = mInVal;
	double aPrevious = mPrevInVal;
	if (mAutoInc) {
		aCurrent = GetInVal();
		aPrevious = X87(aCurrent - X87(mIncRate * 1.5));
	}
	return aPrevious < theInVal && theInVal <= aCurrent;
}

bool CurvedVal::CheckUpdatesFromEndThreshold(int theUpdates)
{
	return CheckInThreshold(GetInValAtUpdate(GetLengthInUpdates() - theUpdates));
}

bool CurvedVal::HasBeenTriggered()
{
	if (mAutoInc) {
		if (GetInVal() == mInMax) {
			mTriggered = true;
			return true;
		}
		mTriggered = false;
	}
	return mTriggered;
}

bool CurvedVal::IsDoingCurve()
{
	return GetInVal() != mInMax && mRamp != RAMP_NONE;
}

}  // namespace Sexy
