// Sexy::PIEffect (PIEffect.h): UpdateParticleDef, which makes the particles one particle definition (or, for a super
// emitter, one free emitter) is due. The exe's x87 arithmetic is kept: sums and products are done in double and rounded
// to a float where the exe stores a float.
#include "PIEffect.h"

#include "Common.h"
#include "PIEffectMath.h"
#include "X86Math.h"

#include <cmath>

namespace Sexy {

namespace {

const double kHalf = 0.5;               // 0x8d3370
const double kRoundCircumference = 6.28318;   // 0x881520
const double kLengthPerParticle = 35.0;       // 0x881528

}

// 0x5707d0 (bejeweled3)
void PIEffect::UpdateParticleDef(PIEmitter* theEmitter, PIEmitterInstance* theInstance, PIParticleDef* theParticleDef,
                                 PIParticleDefInstance* theDefInstance, PIParticleGroup* theGroup,
                                 PIFreeEmitterInstance* theFreeEmitter)
{
	float aStepDivisor = (float)(100.0 / (double)mAnimSpeed);
	PIEmitterInstanceDef* anInstanceDef = theInstance->mDef;
	float aParentLifeRatio = 0.0f;
	if (theFreeEmitter != NULL)
		aParentLifeRatio = theFreeEmitter->mLifeRatio;

	// the number made varies at random, and is drawn again every 25th update
	if (theDefInstance->mUpdateCount % 25 == 0 && !theGroup->mIsFreeEmitterList) {
		if (theDefInstance->mUpdateCount == 0) {
			double aVariation = (double)theParticleDef->mNumberVariation.GetValueAt(mFrameNum, 0.0f);
			double aRandom = (double)GetRandFloat();
			theDefInstance->mNumberVariation = (float)(aRandom * kHalf * aVariation * kHalf);
		} else {
			double aVariation = (double)theParticleDef->mNumberVariation.GetValueAt(mFrameNum, 0.0f);
			double aRandom = (double)GetRandFloat();
			theDefInstance->mNumberVariation = (float)(aRandom * 0.75 * aVariation * kHalf);
		}
	}
	theDefInstance->mUpdateCount++;

	// how many particles are due this step
	float aNumber;
	if (!theGroup->mIsFreeEmitterList) {
		PIValue& aNumberScaleValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mNumberScale : anInstanceDef->mNumberScale;
		float aNumberScale = (float)aNumberScaleValue.GetValueAt(mFrameNum, 0.0f);
		double aBase = ((double)theParticleDef->mNumber.GetValueAt(mFrameNum, 0.0f) + (double)theDefInstance->mNumberVariation) *
		               (double)aNumberScale;
		aNumber = (float)((double)theEmitter->mNumberOverLife.GetValueAt(aParentLifeRatio, 1.0f) * aBase);
		if (aNumber < 0.0f)
			aNumber = 0.0f;
		if (theGroup->mIsFreeEmitterParticles && aParentLifeRatio > 1.0f)
			aNumber = 0.0f;
	} else {
		PIValue& aNumberScaleValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mNumberScale : anInstanceDef->mNumberScale;
		float aNumberScale = (float)aNumberScaleValue.GetValueAt(mFrameNum, 0.0f);
		aNumber = (float)((double)theEmitter->mNumber.GetValueAt(mFrameNum, 0.0f) * (double)aNumberScale);
	}
	aNumber = theInstance->mNumberScale * aNumber;

	// a shape spreads the number over its length (or its count of points)
	if (!theGroup->mIsFreeEmitterList) {
		if (!theGroup->mIsFreeEmitterParticles) {
			bool aKnownShape = true;
			double aShapeFactor = 0.0;
			switch (anInstanceDef->mShape) {
			case 1: {
				if (anInstanceDef->mShapeCount != 0) {
					aShapeFactor = (double)anInstanceDef->mShapeCount;
				} else {
					int aTotalLength = 0;
					int aPointCount = (int)anInstanceDef->mPoints.size();
					for (int i = 0; i < aPointCount - 1; i++) {
						PIPoint aFrom = anInstanceDef->mPoints[i].GetValueAt(mFrameNum);
						PIPoint aTo = anInstanceDef->mPoints[i + 1].GetValueAt(mFrameNum);
						double aDeltaX = aTo.mX - aFrom.mX;
						double aDeltaY = aTo.mY - aFrom.mY;
						float aLength = (float)sqrt(aDeltaY * aDeltaY + aDeltaX * aDeltaX);
						aTotalLength = Ftol((double)aLength + (double)aTotalLength);
					}
					aShapeFactor = (double)aTotalLength / kLengthPerParticle;
				}
				break;
			}
			case 2: {
				float aRadiusX = (float)anInstanceDef->mShapeWidth.GetValueAt(mFrameNum, 0.0f);
				float aRadiusY = (float)anInstanceDef->mShapeHeight.GetValueAt(mFrameNum, 0.0f);
				if (anInstanceDef->mShapeCount != 0) {
					aShapeFactor = (double)anInstanceDef->mShapeCount;
				} else {
					float aMeanSquare = (float)(((double)aRadiusX * aRadiusX + (double)aRadiusY * aRadiusY) * kHalf);
					float aCircumference = (float)((double)PIMath::Sqrt(aMeanSquare) * kRoundCircumference);
					aShapeFactor = (double)aCircumference / kLengthPerParticle;
				}
				break;
			}
			case 3: {
				if (anInstanceDef->mShapeCount != 0) {
					aShapeFactor = (double)(anInstanceDef->mShapeRows * anInstanceDef->mShapeCount);
				} else {
					float aWidth = (float)anInstanceDef->mShapeWidth.GetValueAt(mFrameNum, 0.0f);
					double aHeight = (double)anInstanceDef->mShapeHeight.GetValueAt(mFrameNum, 0.0f);
					aShapeFactor = ((aHeight * (double)aWidth) / 900.0) * 0.25 + 1.0;
				}
				break;
			}
			case 4: {
				float aRadius = (float)anInstanceDef->mShapeWidth.GetValueAt(mFrameNum, 0.0f);
				if (anInstanceDef->mShapeCount != 0) {
					aShapeFactor = (double)anInstanceDef->mShapeCount;
				} else {
					float aSquare = aRadius * aRadius;
					float aCircumference = (float)((double)PIMath::Sqrt(aSquare) * kRoundCircumference);
					aShapeFactor = (double)aCircumference / kLengthPerParticle;
				}
				break;
			}
			default:
				aKnownShape = false;
				break;
			}
			if (aKnownShape)
				aNumber = (float)(aShapeFactor * (double)aNumber);
		}
	} else {
		aNumber = (float)((double)aNumber * 30.0);
	}

	theDefInstance->mSpawnAccumulator = (float)((double)aNumber / (double)aStepDivisor + (double)theDefInstance->mSpawnAccumulator);
	if ((!anInstanceDef->mHasFreeEmitters && !theInstance->mWasInRange) || !theInstance->mCanEmit)
		theDefInstance->mSpawnAccumulator = 0.0f;

	// a single particle is kept alive, not streamed: the accumulator is what is missing
	bool aHasPosition = true;
	if (!theGroup->mIsFreeEmitterList && theParticleDef->mSingleParticle) {
		int aShape = anInstanceDef->mShape;
		int aWanted = 1;
		if (aShape == 1 || aShape == 4 || aShape == 3) {
			int aCount = aShape == 3 ? anInstanceDef->mShapeRows * anInstanceDef->mShapeCount : anInstanceDef->mShapeCount;
			if (aCount != 0)
				aWanted = aCount;
			else
				aHasPosition = false;
		}
		int anAlive = 0;
		for (PIParticleInstance* aParticle = theGroup->mHead; aParticle != NULL; aParticle = aParticle->mNext) {
			if (aParticle->mParticleDef == theParticleDef)
				anAlive++;
		}
		theDefInstance->mSpawnAccumulator = (float)(aWanted - anAlive);
	}

	while (theDefInstance->mSpawnAccumulator > 1.0f) {
		theDefInstance->mSpawnAccumulator = (float)((double)theDefInstance->mSpawnAccumulator - 1.0);

		PIParticleInstance* aParticle;
		if (!theGroup->mIsFreeEmitterList) {
			aParticle = new PIParticleInstance();
		} else {
			PIFreeEmitterInstance* aFreeEmitter = new PIFreeEmitterInstance();
			aFreeEmitter->mDefInstances.resize(theEmitter->mParticleDefs.size());
			aParticle = aFreeEmitter;
		}
		aParticle->mParticleDef = theParticleDef;
		aParticle->mEmitter = theEmitter;
		aParticle->mFreeEmitter = theFreeEmitter;
		aParticle->mIndex = theDefInstance->mParticleCount;
		theDefInstance->mParticleCount++;

		// the direction it leaves in, in degrees first
		double aDirection;
		if (!theGroup->mIsFreeEmitterList) {
			if (!theParticleDef->mAngleFromEmitter) {
				double aVariation = (double)theParticleDef->mAngleVariation.GetValueAt(mFrameNum, 0.0f);
				double aRandom = (double)GetRandFloat();
				double anAngle = (double)theParticleDef->mAngle.GetValueAt(mFrameNum, 0.0f);
				aDirection = anAngle + (aRandom * aVariation * kHalf);
			} else if (!theGroup->mIsFreeEmitterParticles) {
				float anAngle = anInstanceDef->mAngle.GetValueAt(mFrameNum, 0.0f);
				float aVariation = (float)anInstanceDef->mAngleVariation.GetValueAt(mFrameNum, 0.0f);
				double aRandom = (double)GetRandFloat();
				aDirection = aRandom * (double)aVariation * kHalf + (double)anAngle;
			} else {
				double aVariation = (double)theEmitter->mAngleVariation.GetValueAt(mFrameNum, 0.0f);
				double aRandom = (double)GetRandFloat();
				double anAngle = (double)theEmitter->mAngle.GetValueAt(mFrameNum, 0.0f);
				aDirection = anAngle + (aRandom * aVariation * kHalf);
			}
		} else {
			PIValue& anAngleValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mAngle : anInstanceDef->mAngle;
			float anAngle = anAngleValue.GetValueAt(mFrameNum, 0.0f);
			PIValue& aVariationValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mAngleVariation : anInstanceDef->mAngleVariation;
			float aVariation = (float)aVariationValue.GetValueAt(mFrameNum, 0.0f);
			double aRandom = (double)GetRandFloat();
			aDirection = aRandom * (double)aVariation * kHalf + (double)anAngle;
		}
		double aDirectionRadians = (double)PIMath::DegreesToRadians(-(float)aDirection);
		double anEmitterAngle;
		if (theFreeEmitter == NULL)
			anEmitterAngle = (double)PIMath::DegreesToRadians((float)-anInstanceDef->mRotation.GetValueAt(mFrameNum, 0.0f));
		else
			anEmitterAngle = (double)theFreeEmitter->mAngle;
		float anEmitterAngleFloat = (float)anEmitterAngle;
		float aLeaveAngle = (float)((double)anEmitterAngleFloat + (double)(float)aDirectionRadians);
		aParticle->mEmitterRotation = anEmitterAngleFloat;

		if (theParticleDef == NULL || !theParticleDef->mRandomStartFrame)
			aParticle->mRandomFrame = 0;
		else
			aParticle->mRandomFrame = (int)(mRand.NextNoAssert() & 0x7fff);

		// the size scale (the particle's own scale in the exe's numbering)
		PIValue& aScaleSource = theGroup->mIsFreeEmitterParticles ? theEmitter->mScale : anInstanceDef->mScale;
		float aScaleValue = (float)aScaleSource.GetValueAt(mFrameNum, 0.0f);
		aParticle->mScale = (float)((double)theEmitter->mScale.GetValueAt(mFrameNum, 1.0f) * (double)aScaleValue);

		// the random variations this particle keeps
		if (!theGroup->mIsFreeEmitterList) {
			aParticle->mLifeVariation = (float)(GetVariationScalar() * (double)theParticleDef->mLifeVariation.GetValueAt(mFrameNum, 0.0f));
			{
				double aVariation = (double)theParticleDef->mSizeXVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mSizeXVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			double aSizeYVariation = (double)aParticle->mSizeXVariation;
			if (!theParticleDef->mSizeYFollowsX) {
				double aVariation = (double)theParticleDef->mSizeYVariation.GetValueAt(mFrameNum, 0.0f);
				aSizeYVariation = (double)GetVariationScalar() * aVariation;
			}
			aParticle->mSizeYVariation = (float)aSizeYVariation;
			{
				double aVariation = (double)theParticleDef->mVelocityVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mVelocityVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theParticleDef->mWeightVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mWeightVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theParticleDef->mSpinVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mSpinVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theParticleDef->mMotionRandomnessVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mMotionRandomnessVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theParticleDef->mBounceVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mBounceVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			PIValue& aSizeXScaleValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mSizeXScale : anInstanceDef->mSizeXScale;
			float aSizeXScale = (float)aSizeXScaleValue.GetValueAt(mFrameNum, 0.0f);
			double aSizeX = (double)theParticleDef->mSizeX.GetValueAt(mFrameNum, 0.0f);
			aParticle->mSizeX = (float)((aSizeX + (double)aParticle->mSizeXVariation) * (double)aSizeXScale);
			PIValue& aSizeYScaleValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mSizeYScale : anInstanceDef->mSizeYScale;
			float aSizeYScale = (float)aSizeYScaleValue.GetValueAt(mFrameNum, 0.0f);
			double aSizeY = (double)theParticleDef->mSizeY.GetValueAt(mFrameNum, 0.0f);
			aParticle->mSizeY = (float)((aSizeY + (double)aParticle->mSizeYVariation) * (double)aSizeYScale);
			if (theGroup->mIsFreeEmitterParticles) {
				double aWidthOverLife = (double)theEmitter->mSizeOverLife.GetValueAt(aParentLifeRatio, 1.0f);
				aParticle->mSizeX = (float)((((double)theFreeEmitter->mSizeXVariation + 1.0) * aWidthOverLife) * (double)aParticle->mSizeX);
				double aHeightOverLife = (double)theEmitter->mSizeOverLife.GetValueAt(aParentLifeRatio, 1.0f);
				aParticle->mSizeY = (float)((((double)theFreeEmitter->mSizeYVariation + 1.0) * aHeightOverLife) * (double)aParticle->mSizeY);
				double aScaleOverLife = (double)theEmitter->mScaleOverLife.GetValueAt(aParentLifeRatio, 1.0f);
				aParticle->mScale = (float)((((double)theFreeEmitter->mFreeEmitterNumberVariation + 1.0) * aScaleOverLife) * (double)aParticle->mScale);
			}
		} else {
			{
				double aVariation = (double)theEmitter->mLifeVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mLifeVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			double aSizeYVariation;
			{
				double aVariation = (double)theEmitter->mSizeXVariation.GetValueAt(mFrameNum, 0.0f);
				double aRandom = (double)GetRandFloat();
				aParticle->mSizeXVariation = (float)(aRandom * aVariation);
				aSizeYVariation = (double)(float)(aRandom * aVariation);
			}
			if (theParticleDef != NULL && !theParticleDef->mSizeYFollowsX) {
				double aVariation = (double)theEmitter->mSizeYVariation.GetValueAt(mFrameNum, 0.0f);
				double aRandom = (double)GetRandFloat();
				aSizeYVariation = aRandom * aVariation;
			}
			aParticle->mSizeYVariation = (float)aSizeYVariation;
			{
				double aVariation = (double)theEmitter->mVelocityVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mVelocityVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theEmitter->mWeightVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mWeightVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theEmitter->mSpinVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mSpinVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theEmitter->mMotionRandomnessVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mMotionRandomnessVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theEmitter->mBounceVariation.GetValueAt(mFrameNum, 0.0f);
				aParticle->mBounceVariation = (float)((double)GetVariationScalar() * aVariation);
			}
			{
				double aVariation = (double)theEmitter->mValueD.GetValueAt(mFrameNum, 0.0f);
				aParticle->mFreeEmitterNumberVariation = (float)((double)GetVariationScalar() * aVariation);
			}
		}

		aParticle->mColorOffset = (float)GetRandFloatU();
		aParticle->mAge = 0.0f;
		aParticle->mRandomValue = (float)GetRandFloat();
		aParticle->mAngle = 0.0f;

		// how long it lives
		{
			PIValue& aLifeScaleValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mLifeScale : anInstanceDef->mLifeScale;
			float aLifeScale = (float)aLifeScaleValue.GetValueAt(mFrameNum, 0.0f);
			double aLife;
			if (!theGroup->mIsFreeEmitterList) {
				double aBaseLife = (double)theParticleDef->mLife.GetValueAt(mFrameNum, 0.0f);
				aLife = (aBaseLife + (double)aParticle->mLifeVariation) * (double)aLifeScale;
			} else {
				double aBaseLife = (double)theEmitter->mLife.GetValueAt(mFrameNum, 0.0f);
				aLife = ((aBaseLife + (double)aParticle->mLifeVariation) * 5.0) * (double)aLifeScale;
			}
			aParticle->mLife = (float)aLife;
		}

		// where it starts
		double aSpawnX = 0.0;
		double aSpawnY = 0.0;
		bool aPlaced = true;
		if (!theGroup->mIsFreeEmitterParticles) {
			PIPoint anEmitterPos = GetEmitterPos(theInstance, true);
			aParticle->mOriginX = anEmitterPos.mX;
			aParticle->mOriginY = anEmitterPos.mY;
			aParticle->mAttachedX = anEmitterPos.mX;
			aParticle->mAttachedY = anEmitterPos.mY;
			bool anOutside = false;
			if (aHasPosition) {
				PIPoint aGeom = GetGeomPos(theInstance, aParticle, &aLeaveAngle, &anOutside);
				aSpawnX = aGeom.mX - aParticle->mOriginX;
				aSpawnY = aGeom.mY - aParticle->mOriginY;
				if (anOutside)
					aPlaced = false;
			}
		} else {
			double aPlaceX = theFreeEmitter->mPositionX + theFreeEmitter->mAttachedX;
			double aPlaceY = theFreeEmitter->mPositionY + theFreeEmitter->mAttachedY;
			aParticle->mOriginX = aPlaceX;
			aParticle->mAttachedX = aPlaceX;
			aParticle->mOriginY = aPlaceY;
			aParticle->mAttachedY = aPlaceY;
		}
		if (!aPlaced) {
			// port: the exe has already taken the particle from its pool and never gives it back; here it is deleted
			delete aParticle;
			continue;
		}

		// its first velocity
		double aDirectionX = (double)PIMath::Cos(aLeaveAngle);
		double aDirectionY = (double)PIMath::Sin(aLeaveAngle);
		aParticle->mVelocityX = aDirectionX;
		aParticle->mVelocityY = aDirectionY;
		if (!theGroup->mIsFreeEmitterList) {
			PIValue& aVelocityScaleValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mVelocityScale : anInstanceDef->mVelocityScale;
			float aVelocityScale = (float)aVelocityScaleValue.GetValueAt(mFrameNum, 0.0f);
			double aSpeed = ((double)theParticleDef->mVelocity.GetValueAt(mFrameNum, 0.0f) + (double)aParticle->mVelocityVariation) *
			                (double)aVelocityScale;
			double aVelocityX = aSpeed * aParticle->mVelocityX;
			double aVelocityY = aParticle->mVelocityY * aSpeed;
			aParticle->mVelocityX = aVelocityX;
			aParticle->mVelocityY = aVelocityY;
		} else {
			PIValue& aVelocityScaleValue = theGroup->mIsFreeEmitterParticles ? theEmitter->mVelocityScale : anInstanceDef->mVelocityScale;
			float aVelocityScale = (float)aVelocityScaleValue.GetValueAt(mFrameNum, 0.0f);
			double aSpeed = ((double)theEmitter->mVelocity.GetValueAt(mFrameNum, 0.0f) + (double)aParticle->mVelocityVariation) *
			                (double)aVelocityScale;
			double aScaledY = aParticle->mVelocityY * aSpeed;
			double aVelocityX = aSpeed * aParticle->mVelocityX * 160.0;
			double aVelocityY = 160.0 * aScaledY;
			aParticle->mVelocityX = aVelocityX;
			aParticle->mVelocityY = aVelocityY;
		}

		// the particle's own turn
		if (!theGroup->mIsFreeEmitterList) {
			double aTurn;
			if (!theParticleDef->mAlignToMotion) {
				float aDegrees;
				if (!theParticleDef->mRandomStartAngle) {
					aDegrees = (float)-theParticleDef->mStartAngleDegrees;
				} else {
					double aRandom = (double)GetRandFloat();
					aDegrees = (float)-(aRandom * (double)theParticleDef->mRandomAngleRange * kHalf + (double)theParticleDef->mRandomAngleBase);
				}
				aTurn = (double)PIMath::DegreesToRadians(aDegrees);
			} else {
				double aSpeed = sqrt(aParticle->mVelocityY * aParticle->mVelocityY + aParticle->mVelocityX * aParticle->mVelocityX);
				if (aSpeed != 0.0) {
					aParticle->mAngle = -aLeaveAngle;
				} else {
					// the exe takes the cosine of zero here to pick between two angles; it is 1, so the angle is 0
					aParticle->mAngle = 0.0f;
					if (theParticleDef->mSingleParticle && theParticleDef->mAlignToEmitterAngle && !theParticleDef->mAttachToEmitter) {
						double aRotation = (double)PIMath::DegreesToRadians(anInstanceDef->mRotation.GetValueAt(mFrameNum, 0.0f));
						aParticle->mAngle = (float)(aRotation + (double)aParticle->mAngle);
					}
				}
				aTurn = (double)PIMath::DegreesToRadians((float)-theParticleDef->mAlignOffsetDegrees) + (double)aParticle->mAngle;
			}
			aParticle->mAngle = (float)aTurn;
		}

		aParticle->mSpawnOffsetX = aSpawnX;
		aParticle->mSpawnOffsetY = aSpawnY;
		SexyMatrix3 anEmitterTurn;
		anEmitterTurn.LoadIdentity();
		PIMath::RotateDeg(anEmitterTurn, anInstanceDef->mRotation.GetValueAt(mFrameNum, 0.0f));
		PIPoint aTurnedSpawn = PIMath::TransformPoint(anEmitterTurn, aSpawnX, aSpawnY);
		aParticle->mOriginX = aParticle->mOriginX + aTurnedSpawn.mX;
		aParticle->mOriginY = aParticle->mOriginY + aTurnedSpawn.mY;

		if (!theEmitter->mOldestInFront)
			theGroup->Append(aParticle);
		else
			theGroup->Prepend(aParticle);
	}
}

}
