// Sexy::CurvedVal (CurvedVal.h). The original computes in the x87's precision and stores floats and doubles where its
// fields are; the port computes in double and stores at the same places.
#include "CurvedVal.h"

#include "BSpline.h"
#include "SexyAppBase.h"
#include "X86Math.h"

#include <cmath>
#include <map>
#include <vector>

namespace Sexy {

namespace {

const int CURVE_TABLE_SIZE = 0x2000;

// A control point of a curve string: x, y and, for a Hermite curve, the slope there
struct CurvePoint {
	float mX, mY, mSlope;
};

// The Hermite spline of a curve's points (0x6e9611-0x6e96e6, 0x6ebc5b-0x6ebdd9): each segment's four Newton
// coefficients on the nodes x0, x0, x1, x1.
struct HermiteSpline {
	std::vector<CurvePoint> mPoints;
	std::vector<float> mCoef;   // four per segment
	bool mBuilt = false;

	static void Segment(const CurvePoint& p0, const CurvePoint& p1, float* theCoef)  // 0x6e9611
	{
		float z[4] = {p0.mX, p0.mX, p1.mX, p1.mX};
		float r[4][4] = {};
		r[0][0] = p0.mY;
		r[1][0] = p0.mY;
		r[1][1] = p0.mSlope;
		r[2][0] = p1.mY;
		r[2][1] = (float)(((double)r[2][0] - r[1][0]) / ((double)z[2] - z[1]));
		r[3][0] = p1.mY;
		r[3][1] = p1.mSlope;
		r[2][2] = (float)(((double)r[2][1] - r[1][1]) / ((double)z[2] - z[0]));
		r[3][2] = (float)(((double)r[3][1] - r[2][1]) / ((double)z[3] - z[1]));
		r[3][3] = (float)(((double)r[3][2] - r[2][2]) / ((double)z[3] - z[0]));
		for (int k = 0; k < 4; k++)
			theCoef[k] = r[k][k];
	}

	bool Build()  // 0x6ebc5b
	{
		mCoef.clear();
		if (mPoints.size() < 2)
			return false;
		size_t aSegs = mPoints.size() - 1;
		mCoef.resize(aSegs * 4);
		for (size_t i = 0; i < aSegs; i++)
			Segment(mPoints[i], mPoints[i + 1], &mCoef[i * 4]);
		return true;
	}

	float Evaluate(float t)  // 0x6ebdd9
	{
		if (!mBuilt) {
			if (!Build())
				return 0.0f;
			mBuilt = true;
		}
		size_t aSegs = mCoef.size() / 4;
		for (size_t i = 0; i < aSegs; i++) {
			if (t < mPoints[i + 1].mX) {  // 0x6e96e6
				const float* c = &mCoef[i * 4];
				float d[2] = {t - mPoints[i].mX, t - mPoints[i + 1].mX};
				double aProd = 1.0;
				double aSum = c[0];
				for (int k = 1; k < 4; k++) {
					aProd = d[(k - 1) >> 1] * aProd;
					aSum = c[k] * aProd + aSum;
				}
				return (float)aSum;
			}
		}
		return mPoints.back().mY;
	}
};

// The tables, by curve data (the original's map at 0x85d5f0)
std::map<std::string, CurvedVal::CurveRecord*>& CurveCache()
{
	static std::map<std::string, CurvedVal::CurveRecord*> aCache;
	return aCache;
}

float DecodeValue(char c)  // 0x6e9737: '#' is 0 and '~' is 1, skipping the backslash
{
	if (c > '[')
		c--;
	return (float)(c - '#') / 90.0f;
}

int DecodeInt(char c)  // 0x6e975e
{
	if (c > '[')
		c--;
	return c - '#';
}

float DecodeAngle(const char* s)  // 0x6ea51d: three digits base 90, in degrees
{
	return (float)((DecodeInt(s[2]) + (DecodeInt(s[1]) + DecodeInt(s[0]) * 90) * 90) * 360.0f) / 729000.0f;
}

float Clamp01(float v)
{
	if (v <= 0.0f)
		return 0.0f;
	if (v >= 1.0f)
		return 1.0f;
	return v;
}

}  // namespace

struct CurvedVal::CurveRecord {
	float mTable[CURVE_TABLE_SIZE];
	HermiteSpline mHermite;
};

namespace {

// 0x6eaebf: samples the spline through the points into the table, filling gaps by straight lines
void BuildTable(const std::vector<CurvePoint>& thePoints, float* theTable, int theSize, bool theNoClip)
{
	BSpline aSpline;
	for (const CurvePoint& p : thePoints)
		aSpline.AddPoint(p.mX, p.mY);
	aSpline.CalculateSpline(false);
	float aScale = (float)(theSize - 1);
	bool aFirst = true;
	float aLastY = 0.0f;
	int aLastIdx = 0;
	for (int i = 1; i < (int)thePoints.size(); i++) {
		int aStart = Ftol((double)thePoints[i - 1].mX * aScale + 0.5);
		int anEnd = Ftol((double)thePoints[i].mX * aScale + 0.5);
		if (aStart > anEnd)
			continue;
		float aSpan = (float)(anEnd - aStart);
		float aBase = (float)(i - 1);
		for (int s = 0; s <= anEnd - aStart; s++) {
			float t = (float)((double)(float)s / aSpan + aBase);
			float y = aSpline.GetYPoint(t);
			int anIdx = Ftol((double)aSpline.GetXPoint(t) * aScale + 0.5);
			if (anIdx < aLastIdx || anIdx > anEnd)
				continue;
			if (!aFirst) {
				if (aLastIdx + 1 < anIdx) {
					float aGap = (float)(anIdx - aLastIdx);
					for (int k = 0; aLastIdx + k <= anIdx; k++) {
						float f = (float)k / aGap;
						float v = (float)((double)y * f + (1.0 - f) * aLastY);
						theTable[aLastIdx + k] = theNoClip ? v : Clamp01(v);
					}
				} else {
					theTable[anIdx] = theNoClip ? y : Clamp01(y);
				}
			}
			aFirst = false;
			aLastY = y;
			aLastIdx = anIdx;
		}
	}
	for (const CurvePoint& p : thePoints)
		theTable[Ftol((double)p.mX * aScale + 0.5)] = p.mY;
}

}  // namespace

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

void CurvedVal::SetConstant(double theValue)
{
	mLinkedVal = nullptr;
	mInVal = 0.0;
	mTriggered = false;
	mInMax = 1.0;
	mRamp = RAMP_LINEAR;
	mInMin = 0.0;
	mOutMax = theValue;
	mOutMin = theValue;
}

void CurvedVal::SetCurve(const std::string& theCurve, CurvedVal* theLinkedVal)
{
	mCurveRef = nullptr;
	mCurveRefLast = nullptr;
	if (gSexyAppBase)
		mInitAppUpdateCount = gSexyAppBase->mUpdateCount;
	mTriggered = false;
	mLinkedVal = theLinkedVal;
	mRamp = RAMP_CURVEDATA;
	ParseDataString(theCurve);
	mInVal = mInMin;
}

void CurvedVal::SetCurveRef(const char* const* theCurveRef, CurvedVal* theLinkedVal)
{
	SetCurve(*theCurveRef, theLinkedVal);
	mCurveRef = theCurveRef;
	mCurveRefLast = *theCurveRef;
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
		int aFlags = DecodeInt(theString[1]);
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
	mOutMin = (float)atof(theString.substr(aPos, aComma - aPos).c_str());
	aPos = aComma + 1;
	aComma = theString.find(',', aPos);
	if (aComma == std::string::npos)
		return;
	mOutMax = (float)atof(theString.substr(aPos, aComma - aPos).c_str());
	aPos = aComma + 1;
	aComma = theString.find(',', aPos);
	if (aComma == std::string::npos)
		return;
	mIncRate = (float)atof(theString.substr(aPos, aComma - aPos).c_str());
	aPos = aComma + 1;
	if (aVersion > 0) {
		aComma = theString.find(',', aPos);
		if (aComma == std::string::npos)
			return;
		mInMax = (float)atof(theString.substr(aPos, aComma - aPos).c_str());
		aPos = aComma + 1;
	}

	std::string aData = theString.substr(aPos);
	std::map<std::string, CurveRecord*>& aCache = CurveCache();
	std::map<std::string, CurveRecord*>::iterator it = aCache.find(aData);
	if (it != aCache.end()) {
		mCurveCacheRecord = it->second;
		return;
	}
	CurveRecord* aRecord = new CurveRecord();
	aCache[aData] = aRecord;
	mCurveCacheRecord = aRecord;

	std::vector<CurvePoint> aPoints;
	float x = 0.0f;
	size_t i = aPos;
	while (i < theString.size()) {
		CurvePoint p;
		p.mX = x;
		p.mY = DecodeValue(theString[i]);
		if (mIsHermite) {
			p.mSlope = DecodeAngle(theString.c_str() + i + 1);
			i += 4;
		} else {
			p.mSlope = 0.0f;
			i += 1;
		}
		aPoints.push_back(p);
		while (i < theString.size()) {
			char c = theString[i++];
			if (c != ' ') {
				float v = (float)((double)DecodeValue(c) * 0.1f + x);
				x = v < 1.0f ? v : 1.0f;
				break;
			}
			x = (float)(x + 0.1);
		}
	}
	BuildTable(aPoints, aRecord->mTable, CURVE_TABLE_SIZE, mNoClip);
	for (CurvePoint p : aPoints) {
		p.mSlope = std::tan((float)(p.mSlope * 3.1415927f / 180.0f));
		aRecord->mHermite.mPoints.push_back(p);
	}
	aRecord->mHermite.mBuilt = false;
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
	double v;
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
		v = (theInVal - mInMin) / aRange;
		break;
	case RAMP_SLOW_TO_FAST: {
		double a = (theInVal - mInMin) / aRange * 3.14159 * 0.5;
		if (mMode == MODE_PING_PONG)
			a = a + a;
		if (1.570795 < a)
			a = 3.14159 - a;
		v = 1.0 - std::cos(a);
		break;
	}
	case RAMP_FAST_TO_SLOW: {
		double a = (theInVal - mInMin) / aRange * 3.14159 * 0.5;
		if (mMode == MODE_PING_PONG)
			a = a + a;
		v = std::sin(a);
		break;
	}
	case RAMP_SLOW_FAST_SLOW: {
		double a = (theInVal - mInMin) / aRange * 3.14159;
		if (mMode == MODE_PING_PONG)
			a = a + a;
		v = (1.0 - std::cos(a)) * 0.5;
		break;
	}
	case RAMP_FAST_SLOW_FAST: {
		double a = (theInVal - mInMin) / aRange * 3.14159;
		if (mMode == MODE_PING_PONG)
			a = a + a;
		if (3.14159 < a)
			a = 6.28318 - a;
		if (1.570795 <= a)
			v = (2.0 - std::sin(a)) * 0.5;
		else
			v = std::sin(a) * 0.5;
		break;
	}
	case RAMP_CURVEDATA: {
		CheckCurveChange();
		if (mCurveCacheRecord == nullptr)
			return 0.0;
		if (aRange == 0.0)
			return 0.0;
		float t = (float)((theInVal - mInMin) / aRange);
		if (!(t < 1.0f))
			t = 1.0f;
		if (mMode == MODE_PING_PONG) {
			if (0.5f < t)
				t = 1.0f - t;
			t = t + t;
		}
		if (mIsHermite) {
			double h = (mOutMax - mOutMin) * mCurveCacheRecord->mHermite.Evaluate(t) + mOutMin;
			if (mNoClip)
				return h;
			double lo = mOutMin < mOutMax ? mOutMin : mOutMax;
			double hi = mOutMin < mOutMax ? mOutMax : mOutMin;
			return h < lo ? lo : (h > hi ? hi : h);
		}
		int anIdx = Ftol((double)t * 8191.0);
		const float* aTable = mCurveCacheRecord->mTable;
		if (anIdx == CURVE_TABLE_SIZE - 1)
			return (mOutMax - mOutMin) * aTable[CURVE_TABLE_SIZE - 1] + mOutMin;
		float f = (float)((double)t * 8191.0 - anIdx);
		v = (double)aTable[anIdx + 1] * f + (1.0 - f) * aTable[anIdx];
		break;
	}
	default:
		return mOutMin;
	}
	return (mOutMax - mOutMin) * v + mOutMin;
}

double CurvedVal::GetInVal()
{
	double v = mInVal;
	if (mLinkedVal == nullptr) {
		if (mAutoInc) {
			if (gSexyAppBase)
				v = (double)(gSexyAppBase->mUpdateCount - mInitAppUpdateCount) * mIncRate + mInMin;
			if (mMode == MODE_REPEAT || mMode == MODE_PING_PONG)
				v = std::fmod(v - mInMin, mInMax - mInMin) + mInMin;
			else if (mInMax <= v)
				v = mInMax;
		}
	} else if (!mLinkedVal->mOutputSync) {
		v = mLinkedVal->GetInVal();
	} else {
		v = mLinkedVal->GetOutVal();
	}
	if (mMode == MODE_PING_PONG) {
		double f = (float)((v - mInMin) / (mInMax - mInMin));
		if (f <= 0.5)
			v = (f + f) * (mInMax - mInMin) + mInMin;
		else
			v = (mInMax - mInMin) * ((1.0 - f) + (1.0 - f)) + mInMin;
	}
	return v;
}

double CurvedVal::GetOutVal()
{
	mOutVal = GetOutVal(GetInVal());
	return mOutVal;
}

bool CurvedVal::SetInVal(double theInVal)
{
	mPrevOutVal = GetOutVal();
	mPrevInVal = theInVal;
	mTriggered = false;
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
	mInVal += theInc;
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
	return Ftol(std::ceil((mInMax - mInMin) / mIncRate));
}

bool CurvedVal::CheckInThreshold(double theInVal)
{
	double aCur = mInVal;
	double aPrev = mPrevInVal;
	if (mAutoInc) {
		aCur = GetInVal();
		aPrev = aCur - mIncRate * 1.5;
	}
	return aPrev < theInVal && theInVal <= aCur;
}

bool CurvedVal::HasBeenTriggered()
{
	if (mAutoInc)
		mTriggered = GetInVal() == mInMax;
	return mTriggered;
}

}  // namespace Sexy
