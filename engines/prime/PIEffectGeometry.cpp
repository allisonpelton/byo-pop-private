// Sexy::PIEffect (PIEffect.h): where particles start. The exe's x87 arithmetic is kept: sums and products are done in
// double and rounded to a float where the exe stores a float (PIEffectMath.h has the matrix helpers).
#include "PIEffect.h"

#include "Common.h"
#include "Image.h"
#include "MemoryImage.h"
#include "PIEffectMath.h"
#include "X86Math.h"

#include <cmath>

namespace Sexy {

namespace {

const double kHalf = 0.5;                           // 0x8d3370
const double kFloatPi = 3.141590118408203;          // 0x880dc8 (the float 3.14159)
const double kHalfPi = 1.5707950592041016;          // 0x881238
const double kFloatTwoPi = 6.283180236816406;       // 0x881420
const double kEllipseBlend = 0.30000001192092896;   // 0x880ef8

// the squared length of the step between two points, as the exe adds it up: a float
float StepLengthSquared(const PIPoint& theFrom, const PIPoint& theTo)
{
	double aDeltaX = theTo.mX - theFrom.mX;
	double aDeltaY = theTo.mY - theFrom.mY;
	return (float)(aDeltaY * aDeltaY + aDeltaX * aDeltaX);
}

// port: the C runtime's pow on floats, as the exe calls it (0x7932f0 (bejeweled3); the call is identified by its use)
float BendPower(float theBase, float theExponent)
{
	return (float)pow((double)theBase, (double)theExponent);
}

}

// 0x56fda0 (bejeweled3)
PIPoint PIEffect::GetEmitterPos(PIEmitterInstance* theInstance, bool theUseInstance)
{
	PIPoint aPoint = theInstance->mDef->mPosition.GetValueAt(mFrameNum);
	if (theUseInstance) {
		aPoint = PIMath::TransformPoint(theInstance->mTransform, aPoint.mX, aPoint.mY);
		aPoint = PIMath::TransformPoint(mDrawTransform2, aPoint.mX, aPoint.mY);
		aPoint.mX = aPoint.mX + theInstance->mOffsetX;
		aPoint.mY = theInstance->mOffsetY + aPoint.mY;
	}
	return aPoint;
}

// port: the sign a shape turns the particles' direction by: a quarter turn one way, the other, or at random (the exe
// repeats this code in each shape of 0x569f80 (bejeweled3))
float PIEffect::GetShapeTurnSign(PIEmitterInstanceDef* theDef)
{
	if (!theDef->mFlagA)
		return 1.0f;
	if (!theDef->mFlagB)
		return -1.0f;
	return GetRandSign();
}

// 0x569f80 (bejeweled3)
PIPoint PIEffect::GetGeomPos(PIEmitterInstance* theInstance, PIParticleInstance* theParticle, float* theAngle,
                             bool* theOutside)
{
	PIEmitterInstanceDef* anInstanceDef = theInstance->mDef;
	PIPoint aResult;
	aResult.mX = 0.0;
	aResult.mY = 0.0;

	switch (anInstanceDef->mShape) {
	case 1: {   // a line through the points
		int aPointCount = (int)anInstanceDef->mPoints.size();
		if (aPointCount >= 2) {
			int aTotalLength = 0;
			for (int i = 0; i < aPointCount - 1; i++) {
				PIPoint aFrom = anInstanceDef->mPoints[i].GetValueAt(mFrameNum);
				PIPoint aTo = anInstanceDef->mPoints[i + 1].GetValueAt(mFrameNum);
				aTotalLength = Ftol((double)StepLengthSquared(aFrom, aTo) + (double)aTotalLength);
			}
			float aTarget;
			if (anInstanceDef->mShapeCount == 0)
				aTarget = (float)((double)GetRandFloatU() * (double)aTotalLength);
			else
				aTarget = (float)((double)((theParticle->mIndex % anInstanceDef->mShapeCount) * aTotalLength) /
				                  (double)(anInstanceDef->mShapeCount - 1));
			// port: when no segment holds the target (only a rounding at the very end of the line can do that) the
			// exe indexes the points with a wrong number and fails; here the last segment is taken
			int aSegment = aPointCount - 2;
			float aFraction = 1.0f;
			int aWalked = 0;
			for (int i = 0; i < aPointCount - 1; i++) {
				PIPoint aFrom = anInstanceDef->mPoints[i].GetValueAt(mFrameNum);
				PIPoint aTo = anInstanceDef->mPoints[i + 1].GetValueAt(mFrameNum);
				float aLength = StepLengthSquared(aFrom, aTo);
				float aWalkedFloat = (float)aWalked;
				if (aWalkedFloat <= aTarget && aTarget <= (double)aWalkedFloat + (double)aLength) {
					aFraction = (float)(((double)aTarget - (double)aWalked) / (double)aLength);
					aSegment = i;
					break;
				}
				aWalked = Ftol((double)aLength + (double)aWalkedFloat);
			}
			PIPoint aFrom = anInstanceDef->mPoints[aSegment].GetValueAt(mFrameNum);
			PIPoint aTo = anInstanceDef->mPoints[aSegment + 1].GetValueAt(mFrameNum);
			double aDeltaX = aTo.mX - aFrom.mX;
			double aDeltaY = aTo.mY - aFrom.mY;
			double aWeight = (double)aFraction;
			aResult.mX = aFrom.mX * (1.0 - aWeight) + aTo.mX * aWeight;
			aResult.mY = aWeight * aTo.mY + (1.0 - aWeight) * aFrom.mY;
			if (theAngle != NULL) {
				float aSign = GetShapeTurnSign(anInstanceDef);
				float aTurn = (float)((atan2(aDeltaY, aDeltaX) + kHalfPi) + (double)aSign * kFloatPi * kHalf);
				*theAngle = (float)((double)aTurn + (double)*theAngle);
			}
		}
		break;
	}
	case 2: {   // an ellipse
		float aRadiusX = (float)anInstanceDef->mShapeWidth.GetValueAt(mFrameNum, 0.0f);
		float aRadiusY = (float)anInstanceDef->mShapeHeight.GetValueAt(mFrameNum, 0.0f);
		float anAngle;
		if (anInstanceDef->mShapeCount == 0) {
			anAngle = (float)((double)GetRandFloat() * kFloatPi);
		} else {
			double aSteps = (double)(theParticle->mIndex % anInstanceDef->mShapeCount) * kFloatPi;
			anAngle = (float)((aSteps + aSteps) / (double)anInstanceDef->mShapeCount);
			if ((double)anAngle > kFloatPi)
				anAngle = (float)((double)anAngle - kFloatTwoPi);
		}
		// the angle runs round the ellipse as a parameter; the exe bends it toward the polar angle of the point
		if (aRadiusX > aRadiusY) {
			float anExponent = (float)(1.0 + ((double)aRadiusX / (double)aRadiusY - 1.0) * kEllipseBlend);
			if ((double)anAngle < -kHalfPi) {
				float aBase = (float)(((double)anAngle + kFloatPi) / kHalfPi);
				anAngle = (float)(kFloatPi + ((double)BendPower(aBase, anExponent) * kFloatPi) * kHalf);
			} else if ((double)anAngle < 0.0) {
				float aBase = (float)(-(double)anAngle / kHalfPi);
				anAngle = (float)(((-(double)BendPower(aBase, anExponent)) * kFloatPi) * kHalf);
			} else if ((double)anAngle < kHalfPi) {
				float aBase = (float)((double)anAngle / kHalfPi);
				anAngle = (float)(((double)BendPower(aBase, anExponent) * kFloatPi) * kHalf);
			} else {
				float aBase = (float)((kFloatPi - (double)anAngle) / kHalfPi);
				anAngle = (float)(kFloatPi - ((double)BendPower(aBase, anExponent) * kFloatPi) * kHalf);
			}
		} else if (aRadiusX < aRadiusY) {
			float anExponent = (float)(1.0 + ((double)aRadiusY / (double)aRadiusX - 1.0) * kEllipseBlend);
			if ((double)anAngle < -kHalfPi) {
				float aBase = (float)((-kHalfPi - (double)anAngle) / kHalfPi);
				anAngle = (float)(-kHalfPi - ((double)BendPower(aBase, anExponent) * kFloatPi) * kHalf);
			} else if ((double)anAngle < 0.0) {
				float aBase = (float)(((double)anAngle + kHalfPi) / kHalfPi);
				anAngle = (float)(((double)BendPower(aBase, anExponent) * kFloatPi) * kHalf - kHalfPi);
			} else if ((double)anAngle < kHalfPi) {
				float aBase = (float)((kHalfPi - (double)anAngle) / kHalfPi);
				anAngle = (float)(kHalfPi - ((double)BendPower(aBase, anExponent) * kFloatPi) * kHalf);
			} else {
				float aBase = (float)(((double)anAngle - kHalfPi) / kHalfPi);
				anAngle = (float)(kHalfPi + ((double)BendPower(aBase, anExponent) * kFloatPi) * kHalf);
			}
		}
		aResult.mX = (double)PIMath::Cos(anAngle) * (double)aRadiusX;
		aResult.mY = (double)PIMath::Sin(anAngle) * (double)aRadiusY;
		if (theAngle != NULL) {
			float aSign = GetShapeTurnSign(anInstanceDef);
			float aTurn = (float)((double)aSign * kFloatPi * kHalf + (double)anAngle);
			*theAngle = (float)((double)aTurn + (double)*theAngle);
		}
		break;
	}
	case 3: {   // a grid (with no count, a rectangle filled at random)
		float aWidth = (float)anInstanceDef->mShapeWidth.GetValueAt(mFrameNum, 0.0f);
		float aHeight = (float)anInstanceDef->mShapeHeight.GetValueAt(mFrameNum, 0.0f);
		if (anInstanceDef->mShapeCount == 0) {
			double aRandomX = (double)GetRandFloat();
			aResult.mX = (aRandomX * (double)aWidth) * kHalf;
			double aRandomY = (double)GetRandFloat();
			aResult.mY = (aRandomY * (double)aHeight) * kHalf;
		} else {
			int aColumns = anInstanceDef->mShapeCount;
			int aRows = anInstanceDef->mShapeRows;
			int anIndex = theParticle->mIndex;
			int aColumn = anIndex % aColumns;
			float aRow = (float)((anIndex / aColumns) % aRows);
			if (aColumns > 1)
				aResult.mX = ((double)aColumn / (double)(aColumns - 1) - kHalf) * (double)aWidth;
			if (aRows > 1)
				aResult.mY = ((double)aRow / (double)(aRows - 1) - kHalf) * (double)aHeight;
		}
		// a mask picture (when the instance has one) rules some of the points out
		MemoryImage* aMask = (MemoryImage*)theInstance->mMaskImage;
		if (aMask != NULL && theOutside != NULL) {
			float aUnitX = (float)(aResult.mX / (double)aWidth + kHalf);
			float aUnitY = (float)(aResult.mY / (double)aHeight + kHalf);
			int aMaskWidth = aMask->mWidth;
			int aMaskHeight = aMask->mHeight;
			int aPixelX = Ftol((double)aMaskWidth * (double)aUnitX);
			if (aPixelX > aMaskWidth - 1)
				aPixelX = aMaskWidth - 1;
			int aPixelY = Ftol((double)aMaskHeight * (double)aUnitY);
			if (aPixelY > aMaskHeight - 1)
				aPixelY = aMaskHeight - 1;
			unsigned long* aBits = aMask->GetBits();
			bool aTransparent = ((aBits[aPixelY * aMaskWidth + aPixelX] >> 31) & 1) == 0;
			if (aTransparent != anInstanceDef->mFlagC)
				*theOutside = true;
		}
		break;
	}
	case 4: {   // a circle
		float aRadius = (float)anInstanceDef->mShapeWidth.GetValueAt(mFrameNum, 0.0f);
		float anAngle;
		if (anInstanceDef->mShapeCount == 0) {
			anAngle = (float)((double)GetRandFloat() * kFloatPi);
		} else {
			double aSteps = (double)(theParticle->mIndex % anInstanceDef->mShapeCount) * kFloatPi;
			anAngle = (float)((aSteps + aSteps) / (double)anInstanceDef->mShapeCount);
		}
		aResult.mX = (double)PIMath::Cos(anAngle) * (double)aRadius;
		aResult.mY = (double)PIMath::Sin(anAngle) * (double)aRadius;
		if (theAngle != NULL) {
			float aSign = GetShapeTurnSign(anInstanceDef);
			float aTurn = (float)((double)aSign * kFloatPi * kHalf + (double)anAngle);
			*theAngle = (float)((double)aTurn + (double)*theAngle);
		}
		break;
	}
	}

	PIPoint anEmitterPos = GetEmitterPos(theInstance, false);
	aResult.mX = (aResult.mX + anEmitterPos.mX) + theInstance->mOffsetX;
	aResult.mY = (anEmitterPos.mY + aResult.mY) + theInstance->mOffsetY;
	aResult = PIMath::TransformPoint(theInstance->mTransform, aResult.mX, aResult.mY);
	aResult = PIMath::TransformPoint(mDrawTransform2, aResult.mX, aResult.mY);
	return aResult;
}

}
