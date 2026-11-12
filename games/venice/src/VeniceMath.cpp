// Sine and cosine that give the same bits on every platform.
//
// The original takes its sines and cosines from the x87's fsin and fcos (0x567324, 0x5671c4), which give the value to 64 bits, so
// that it is about the correctly rounded one. The libraries of the two builds do not: Windows' C library and the one of the web
// build (musl) differ in the last bit of the double for about 3 in 100 angles, each of them rounded the other way in about 3 in 100
// as well, so that a particle's frame (a float cut to an integer where a filter leaves it a hair under the whole number) came
// out different in the browser. These are computed in double-double arithmetic (two doubles, about 106 bits) from a reduction by
// a multiple of a quarter turn and the Taylor series, and rounded once at the end, so that the result is the correctly rounded
// one on every platform (it is the same code on each, and no contraction to a fused multiply-add is allowed).

#include "VeniceMath.h"

#include <cmath>

#pragma clang fp contract(off)
#pragma STDC FP_CONTRACT OFF

namespace {

struct DoubleDouble {
	double mHigh;
	double mLow;
};

// the sum of two doubles with |a| >= |b|, as a double and its error
DoubleDouble QuickTwoSum(double theA, double theB)
{
	double aSum = theA + theB;
	return {aSum, theB - (aSum - theA)};
}

DoubleDouble TwoSum(double theA, double theB)
{
	double aSum = theA + theB;
	double aVirtual = aSum - theA;
	return {aSum, (theA - (aSum - aVirtual)) + (theB - aVirtual)};
}

// the product of two doubles and its error, by Dekker's split into halves of 26 bits
DoubleDouble TwoProduct(double theA, double theB)
{
	const double aSplitter = 134217729.0; // 2^27 + 1
	double aProduct = theA * theB;
	double aScaledA = aSplitter * theA;
	double aHighA = aScaledA - (aScaledA - theA);
	double aLowA = theA - aHighA;
	double aScaledB = aSplitter * theB;
	double aHighB = aScaledB - (aScaledB - theB);
	double aLowB = theB - aHighB;
	return {aProduct, ((aHighA * aHighB - aProduct) + aHighA * aLowB + aLowA * aHighB) + aLowA * aLowB};
}

DoubleDouble Add(DoubleDouble theX, DoubleDouble theY)
{
	DoubleDouble aHighSum = TwoSum(theX.mHigh, theY.mHigh);
	DoubleDouble aLowSum = TwoSum(theX.mLow, theY.mLow);
	aHighSum.mLow += aLowSum.mHigh;
	aHighSum = QuickTwoSum(aHighSum.mHigh, aHighSum.mLow);
	aHighSum.mLow += aLowSum.mLow;
	return QuickTwoSum(aHighSum.mHigh, aHighSum.mLow);
}

DoubleDouble Multiply(DoubleDouble theX, DoubleDouble theY)
{
	DoubleDouble aProduct = TwoProduct(theX.mHigh, theY.mHigh);
	aProduct.mLow += theX.mHigh * theY.mLow + theX.mLow * theY.mHigh;
	return QuickTwoSum(aProduct.mHigh, aProduct.mLow);
}

// 1/(2k+1)! with the sign of the sine series, and 1/(2k)! with that of the cosine series
const DoubleDouble SineCoefficients[15] = {
	{0x1.0000000000000p+0, 0x0.0p+0}, // 1/1!
	{-0x1.5555555555555p-3, -0x1.5555555555555p-57}, // -1/3!
	{0x1.1111111111111p-7, 0x1.1111111111111p-63}, // 1/5!
	{-0x1.a01a01a01a01ap-13, -0x1.a01a01a01a01ap-73}, // -1/7!
	{0x1.71de3a556c734p-19, -0x1.c154f8ddc6c00p-73}, // 1/9!
	{-0x1.ae64567f544e4p-26, 0x1.c062e06d1f209p-80}, // -1/11!
	{0x1.6124613a86d09p-33, 0x1.f28e0cc748ebep-87}, // 1/13!
	{-0x1.ae7f3e733b81fp-41, -0x1.1d8656b0ee8cbp-97}, // -1/15!
	{0x1.952c77030ad4ap-49, 0x1.ac981465ddc6cp-103}, // 1/17!
	{-0x1.2f49b46814157p-57, -0x1.2650f61dbdcb4p-112}, // -1/19!
	{0x1.71b8ef6dcf572p-66, -0x1.d043ae40c4647p-120}, // 1/21!
	{-0x1.761b41316381ap-75, 0x1.3423c7d91404fp-130}, // -1/23!
	{0x1.3f3ccdd165fa9p-84, -0x1.58ddadf344487p-139}, // 1/25!
	{-0x1.d1ab1c2dccea3p-94, -0x1.054d0c78aea14p-149}, // -1/27!
	{0x1.259f98b4358adp-103, 0x1.eaf8c39dd9bc5p-157}, // 1/29!
};

const DoubleDouble CosineCoefficients[16] = {
	{0x1.0000000000000p+0, 0x0.0p+0}, // 1/0!
	{-0x1.0000000000000p-1, 0x0.0p+0}, // -1/2!
	{0x1.5555555555555p-5, 0x1.5555555555555p-59}, // 1/4!
	{-0x1.6c16c16c16c17p-10, 0x1.f49f49f49f49fp-65}, // -1/6!
	{0x1.a01a01a01a01ap-16, 0x1.a01a01a01a01ap-76}, // 1/8!
	{-0x1.27e4fb7789f5cp-22, -0x1.cbbc05b4fa99ap-76}, // -1/10!
	{0x1.1eed8eff8d898p-29, -0x1.2aec959e14c06p-83}, // 1/12!
	{-0x1.93974a8c07c9dp-37, -0x1.05d6f8a2efd1fp-92}, // -1/14!
	{0x1.ae7f3e733b81fp-45, 0x1.1d8656b0ee8cbp-101}, // 1/16!
	{-0x1.6827863b97d97p-53, -0x1.eec01221a8b0bp-107}, // -1/18!
	{0x1.e542ba4020225p-62, 0x1.ea72b4afe3c2fp-120}, // 1/20!
	{-0x1.0ce396db7f853p-70, 0x1.aebcdbd20331cp-124}, // -1/22!
	{0x1.f2cf01972f578p-80, -0x1.9ada5fcc1ab14p-135}, // 1/24!
	{-0x1.88e85fc6a4e5ap-89, 0x1.71c37ebd16540p-143}, // -1/26!
	{0x1.0a18a2635085dp-98, 0x1.b9e2e28e1aa54p-153}, // 1/28!
	{-0x1.3932c5047d60ep-108, -0x1.832b7b530a627p-162}, // -1/30!
};

// the terms from this one on are small enough for plain doubles
const int FirstPlainTerm = 9;

const double TwoOverPi = 0x1.45f306dc9c883p-1;
// pi/2 in pieces of 33 bits, so that a quarter-turn count of up to 20 bits multiplies each without rounding, and the rest of it
const double HalfPiPart1 = 1.57079632673412561417e+00;
const double HalfPiPart2 = 6.07710050630396597660e-11;
const double HalfPiPart3 = 2.02226624871116645580e-21;
const double HalfPiRest = 8.47842766036889956997e-32;
// beyond this the quarter-turn count needs more than 20 bits, and the library's functions are used
const double LargestAngle = 1000000.0;

// sin of a reduced angle (a double-double of at most about pi/4)
DoubleDouble SineOfReduced(DoubleDouble theAngle)
{
	DoubleDouble aSquare = Multiply(theAngle, theAngle);
	double aTail = SineCoefficients[14].mHigh;
	for (int aTerm = 13; aTerm >= FirstPlainTerm; aTerm--)
		aTail = aTail * aSquare.mHigh + SineCoefficients[aTerm].mHigh;
	DoubleDouble aSum = {aTail, 0.0};
	for (int aTerm = FirstPlainTerm - 1; aTerm >= 0; aTerm--)
		aSum = Add(Multiply(aSum, aSquare), SineCoefficients[aTerm]);
	return Multiply(aSum, theAngle);
}

DoubleDouble CosineOfReduced(DoubleDouble theAngle)
{
	DoubleDouble aSquare = Multiply(theAngle, theAngle);
	double aTail = CosineCoefficients[15].mHigh;
	for (int aTerm = 14; aTerm >= FirstPlainTerm; aTerm--)
		aTail = aTail * aSquare.mHigh + CosineCoefficients[aTerm].mHigh;
	DoubleDouble aSum = {aTail, 0.0};
	for (int aTerm = FirstPlainTerm - 1; aTerm >= 0; aTerm--)
		aSum = Add(Multiply(aSum, aSquare), CosineCoefficients[aTerm]);
	return aSum;
}

// the angle less its nearest multiple of a quarter turn, which is returned
long long ReduceAngle(double theAngle, DoubleDouble& theReduced)
{
	double aTurns = floor(theAngle * TwoOverPi + 0.5);
	DoubleDouble aReduced = {theAngle - aTurns * HalfPiPart1, 0.0};
	aReduced = Add(aReduced, {-(aTurns * HalfPiPart2), 0.0});
	aReduced = Add(aReduced, {-(aTurns * HalfPiPart3), 0.0});
	aReduced = Add(aReduced, {-(aTurns * HalfPiRest), 0.0});
	theReduced = aReduced;
	return (long long)aTurns;
}

double Rounded(DoubleDouble theValue)
{
	return theValue.mHigh + theValue.mLow;
}

} // namespace

// port: the original's x87 fsin of the angle as a double (0x567324), computed so that every platform gives the same bits
double VeniceSin(float theAngle)
{
	double anAngle = (double)theAngle;
	if (anAngle == 0.0 || !(fabs(anAngle) < LargestAngle))
		return sin(anAngle);
	DoubleDouble aReduced;
	long long aQuarters = ReduceAngle(anAngle, aReduced);
	switch (aQuarters & 3) {
	case 0:
		return Rounded(SineOfReduced(aReduced));
	case 1:
		return Rounded(CosineOfReduced(aReduced));
	case 2:
		return -Rounded(SineOfReduced(aReduced));
	}
	return -Rounded(CosineOfReduced(aReduced));
}

// port: the original's x87 fcos (0x5671c4), as VeniceSin
double VeniceCos(float theAngle)
{
	double anAngle = (double)theAngle;
	if (!(fabs(anAngle) < LargestAngle))
		return cos(anAngle);
	DoubleDouble aReduced;
	long long aQuarters = ReduceAngle(anAngle, aReduced);
	switch (aQuarters & 3) {
	case 0:
		return Rounded(CosineOfReduced(aReduced));
	case 1:
		return -Rounded(SineOfReduced(aReduced));
	case 2:
		return -Rounded(CosineOfReduced(aReduced));
	}
	return Rounded(SineOfReduced(aReduced));
}
