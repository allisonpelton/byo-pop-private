// port: the x87's fsin, fcos, fpatan and fyl2x to 64 bits, and the one rounding of the operation after them (X87.h). The
// arithmetic is double-double, a value as the unevaluated sum of two doubles, good to about 100 bits, then rounded to
// the x87's 64. No operation here may be fused: each product and sum is the double's own.

#include "X87.h"

#include <math.h>

#pragma clang fp contract(off)

namespace Sexy
{

namespace
{

// a value to about 100 bits as the sum of two doubles, mHigh the nearest double to it
struct DoubleDouble
{
	double mHigh;
	double mLow;
};

// Dekker's splitting factor, 2^27 + 1: it cuts a double into two halves of 26 bits whose products are exact
const double SPLIT_FACTOR = 134217729.0;

DoubleDouble TwoSum(double a, double b)
{
	double aSum = a + b;
	double aPart = aSum - a;
	return { aSum, (a - (aSum - aPart)) + (b - aPart) };
}

// the sum of a and b, |a| >= |b|
DoubleDouble QuickTwoSum(double a, double b)
{
	double aSum = a + b;
	return { aSum, b - (aSum - a) };
}

DoubleDouble TwoProduct(double a, double b)
{
	double aProduct = a * b;
	double aSplitA = SPLIT_FACTOR * a;
	double aHighA = aSplitA - (aSplitA - a);
	double aLowA = a - aHighA;
	double aSplitB = SPLIT_FACTOR * b;
	double aHighB = aSplitB - (aSplitB - b);
	double aLowB = b - aHighB;
	return { aProduct, ((aHighA * aHighB - aProduct) + aHighA * aLowB + aLowA * aHighB) + aLowA * aLowB };
}

DoubleDouble Add(DoubleDouble a, DoubleDouble b)
{
	DoubleDouble aSum = TwoSum(a.mHigh, b.mHigh);
	DoubleDouble aLowSum = TwoSum(a.mLow, b.mLow);
	aSum = QuickTwoSum(aSum.mHigh, aSum.mLow + aLowSum.mHigh);
	return QuickTwoSum(aSum.mHigh, aSum.mLow + aLowSum.mLow);
}

DoubleDouble Negate(DoubleDouble a)
{
	return { -a.mHigh, -a.mLow };
}

DoubleDouble Multiply(DoubleDouble a, DoubleDouble b)
{
	DoubleDouble aProduct = TwoProduct(a.mHigh, b.mHigh);
	return QuickTwoSum(aProduct.mHigh, aProduct.mLow + (a.mHigh * b.mLow + a.mLow * b.mHigh));
}

DoubleDouble Divide(DoubleDouble a, DoubleDouble b)
{
	double aQuotient = a.mHigh / b.mHigh;
	DoubleDouble aRemainder = Add(a, Negate(Multiply({ aQuotient, 0.0 }, b)));
	double aCorrection = aRemainder.mHigh / b.mHigh;
	aRemainder = Add(aRemainder, Negate(Multiply({ aCorrection, 0.0 }, b)));
	return Add(QuickTwoSum(aQuotient, aCorrection), { aRemainder.mHigh / b.mHigh, 0.0 });
}

DoubleDouble SquareRoot(DoubleDouble a)
{
	double aRoot = sqrt(a.mHigh);
	DoubleDouble aSquare = TwoProduct(aRoot, aRoot);
	double aCorrection = (((a.mHigh - aSquare.mHigh) - aSquare.mLow) + a.mLow) / (2.0 * aRoot);
	return QuickTwoSum(aRoot, aCorrection);
}

// the x87's own pi, 66 bits (0xc90fdaa22168c234c times 2^-66), which fsin and fcos reduce their angle by: half of it
// in two pieces of 33 bits, so that a whole number of quarter turns up to 2^20 times either is exact
const double X87_HALF_PI_HIGH = 0x1.921fb544p+0;
const double X87_HALF_PI_LOW = 0x1.0b4611a6p-34;
// 2 / pi, to pick the quarter turn
const double TWO_OVER_PI = 0.63661977236758134;
// pi to about 106 bits, for the quarter of the plane an arc tangent is in
const DoubleDouble PI = { 0x1.921fb54442d18p+1, 0x1.1a62633145c07p-53 };
const DoubleDouble HALF_PI = { 0x1.921fb54442d18p+0, 0x1.1a62633145c07p-54 };
// ln 2 to about 106 bits, and as fldln2 loads it, to 64 (0xb17217f7d1cf79ac times 2^-64)
const DoubleDouble LN2 = { 0x1.62e42fefa39efp-1, 0x1.abc9e3b39803fp-56 };
const DoubleDouble X87_LN2 = { 0x1.62e42fefa39efp-1, 0x1.acp-56 };
// sqrt(1/2), below which a log's significand is doubled so that its series starts nearer 1
const double HALF_ROOT_TWO = 0.70710678118654752;
// a term of a series this far below the sum no longer reaches the x87's 64 bits
const double SERIES_END = 0x1p-80;

// the sine (or with theCosine the cosine) of a reduced angle, |theAngle| <= pi / 4 or a little more, by its Taylor
// series
DoubleDouble SeriesSinCos(DoubleDouble theAngle, bool theCosine)
{
	DoubleDouble aSquare = Multiply(theAngle, theAngle);
	DoubleDouble aTerm = theCosine ? DoubleDouble{ 1.0, 0.0 } : theAngle;
	DoubleDouble aSum = aTerm;
	int aPower = theCosine ? 0 : 1;
	while (fabs(aTerm.mHigh) > SERIES_END * fabs(aSum.mHigh))
	{
		aTerm = Divide(Negate(Multiply(aTerm, aSquare)), { (double)((aPower + 1) * (aPower + 2)), 0.0 });
		aSum = Add(aSum, aTerm);
		aPower += 2;
	}
	return aSum;
}

// the x87's rounding of a double-double to 64 bits of significand, to the nearest and to even on a tie
X87Value RoundTo64(DoubleDouble theValue)
{
	DoubleDouble aValue = QuickTwoSum(theValue.mHigh, theValue.mLow);
	if (aValue.mHigh == 0.0 || aValue.mLow == 0.0)
		return X87Value(aValue.mHigh, 0.0);
	int anExponent;
	double aFraction = frexp(aValue.mHigh, &anExponent);
	// the value lies in [2^(anExponent - 1), 2^anExponent), unless the high part is a power of two that the low part
	// takes below it
	if (fabs(aFraction) == 0.5 && (aValue.mLow < 0.0) != (aValue.mHigh < 0.0))
		anExponent--;
	double aLastBit = ldexp(1.0, anExponent - 64);
	double aLow = nearbyint(aValue.mLow / aLastBit) * aLastBit;
	DoubleDouble aRounded = QuickTwoSum(aValue.mHigh, aLow);
	return X87Value(aRounded.mHigh, aRounded.mLow);
}

// the sine or the cosine of an angle as fsin and fcos take it: an angle beyond a quarter turn's half is first reduced
// by whole quarter turns of the x87's pi, exactly
X87Value SinCos(double theAngle, bool theCosine)
{
	double aTurns = nearbyint(theAngle * TWO_OVER_PI);
	DoubleDouble anAngle = { theAngle, 0.0 };
	if (aTurns != 0.0)
		anAngle = TwoSum(theAngle - aTurns * X87_HALF_PI_HIGH, -(aTurns * X87_HALF_PI_LOW));
	int aQuarter = (int)fmod(aTurns, 4.0);
	if (aQuarter < 0)
		aQuarter += 4;
	// sin(r + q pi/2) and cos(r + q pi/2) by the quarter q
	bool aUseCosine = theCosine != (aQuarter % 2 == 1);
	DoubleDouble aResult = SeriesSinCos(anAngle, aUseCosine);
	bool aNegate = theCosine ? (aQuarter == 1 || aQuarter == 2) : (aQuarter >= 2);
	return RoundTo64(aNegate ? Negate(aResult) : aResult);
}

// the arc tangent of 0 <= theRatio <= 1: halved three times (atan z = 2 atan(z / (1 + sqrt(1 + z^2)))) to below 0.1,
// then its Taylor series
DoubleDouble SeriesAtan(DoubleDouble theRatio)
{
	DoubleDouble aRatio = theRatio;
	const int HALVINGS = 3;
	for (int i = 0; i < HALVINGS; i++)
	{
		DoubleDouble aRoot = SquareRoot(Add({ 1.0, 0.0 }, Multiply(aRatio, aRatio)));
		aRatio = Divide(aRatio, Add({ 1.0, 0.0 }, aRoot));
	}
	DoubleDouble aSquare = Multiply(aRatio, aRatio);
	DoubleDouble aPower = aRatio;
	DoubleDouble aSum = aRatio;
	int aDegree = 1;
	while (aSum.mHigh != 0.0 && fabs(aPower.mHigh) / aDegree > SERIES_END * fabs(aSum.mHigh))
	{
		aPower = Negate(Multiply(aPower, aSquare));
		aDegree += 2;
		aSum = Add(aSum, Divide(aPower, { (double)aDegree, 0.0 }));
	}
	return Multiply(aSum, { (double)(1 << HALVINGS), 0.0 });
}

// the natural log of 1 <= theValue < 2, by its series in (v - 1) / (v + 1): ln v = 2 atanh((v - 1) / (v + 1)), the
// ratio below 0.172 from sqrt(1/2) up
DoubleDouble SeriesLog(DoubleDouble theValue)
{
	DoubleDouble aRatio = Divide(Add(theValue, { -1.0, 0.0 }), Add(theValue, { 1.0, 0.0 }));
	DoubleDouble aSquare = Multiply(aRatio, aRatio);
	DoubleDouble aPower = aRatio;
	DoubleDouble aSum = aRatio;
	int aDegree = 1;
	while (aSum.mHigh != 0.0 && fabs(aPower.mHigh) / aDegree > SERIES_END * fabs(aSum.mHigh))
	{
		aPower = Multiply(aPower, aSquare);
		aDegree += 2;
		aSum = Add(aSum, Divide(aPower, { (double)aDegree, 0.0 }));
	}
	return Multiply(aSum, { 2.0, 0.0 });
}

}

X87Value X87Sin(double theAngle)
{
	return SinCos(theAngle, false);
}

X87Value X87Cos(double theAngle)
{
	return SinCos(theAngle, true);
}

X87Value X87Atan2(double theY, double theX)
{
	double aY = fabs(theY);
	double aX = fabs(theX);
	DoubleDouble anAngle;
	if (aX == 0.0 && aY == 0.0)
		anAngle = { 0.0, 0.0 };
	else if (aY <= aX)
		anAngle = SeriesAtan(Divide({ aY, 0.0 }, { aX, 0.0 }));
	else
		anAngle = Add(HALF_PI, Negate(SeriesAtan(Divide({ aX, 0.0 }, { aY, 0.0 }))));
	if (theX < 0.0)
		anAngle = Add(PI, Negate(anAngle));
	if (theY < 0.0)
		anAngle = Negate(anAngle);
	return RoundTo64(anAngle);
}

X87Value X87Log(double theValue)
{
	int anExponent;
	double aSignificand = frexp(theValue, &anExponent);
	if (aSignificand < HALF_ROOT_TWO)
	{
		aSignificand *= 2.0;
		anExponent--;
	}
	// log2 of the value, which fyl2x multiplies by the ln 2 that fldln2 put below it
	DoubleDouble aLog2 = Add({ (double)anExponent, 0.0 }, Divide(SeriesLog({ aSignificand, 0.0 }), LN2));
	return RoundTo64(Multiply(aLog2, X87_LN2));
}

double operator*(const X87Value& theValue, double theFactor)
{
	DoubleDouble aProduct = TwoProduct(theValue.mHigh, theFactor);
	return aProduct.mHigh + (aProduct.mLow + theValue.mLow * theFactor);
}

double operator+(const X87Value& theValue, double theAddend)
{
	DoubleDouble aSum = TwoSum(theValue.mHigh, theAddend);
	return aSum.mHigh + (aSum.mLow + theValue.mLow);
}

double operator/(const X87Value& theValue, double theDivisor)
{
	if (theDivisor == 0.0)
		return theValue.mHigh / theDivisor;
	DoubleDouble aQuotient = Divide({ theValue.mHigh, theValue.mLow }, { theDivisor, 0.0 });
	return aQuotient.mHigh + aQuotient.mLow;
}

double operator/(double theDividend, const X87Value& theValue)
{
	if (theValue.mHigh == 0.0)
		return theDividend / theValue.mHigh;
	DoubleDouble aQuotient = Divide({ theDividend, 0.0 }, { theValue.mHigh, theValue.mLow });
	return aQuotient.mHigh + aQuotient.mLow;
}

double operator+(const X87Value& theValue, const X87Value& theAddend)
{
	DoubleDouble aSum = Add({ theValue.mHigh, theValue.mLow }, { theAddend.mHigh, theAddend.mLow });
	return aSum.mHigh + aSum.mLow;
}

bool operator<=(double theLeft, const X87Value& theRight)
{
	return Add({ theLeft, 0.0 }, { -theRight.mHigh, -theRight.mLow }).mHigh <= 0.0;
}

bool operator<=(const X87Value& theLeft, double theRight)
{
	return Add({ theLeft.mHigh, theLeft.mLow }, { -theRight, 0.0 }).mHigh <= 0.0;
}

}
