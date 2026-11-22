// Sexy::PIEffect (PIEffect.h): UpdateParticleGroup, which ages, moves and ends the particles of one group (an emitter
// instance's particles, its free emitters, or a free emitter's particles). The exe's x87 arithmetic is kept: sums and
// products are done in double and rounded to a float where the exe stores a float.
#include "PIEffect.h"

#include "Common.h"
#include "MemoryImage.h"
#include "PIEffectMath.h"
#include "X86Math.h"

#include <cmath>

namespace Sexy {

namespace {

const double kHalf = 0.5;                            // 0x8d3370
const double kTwo = 2.0;                             // 0x880d48
const double kHundred = 100.0;                       // 0x880d90
const double kNearlyOne = 0.9999998807907104;        // 0x881508: a life ratio at or past this ends the particle
const float kTinyLife = 9.99999993922529e-09f;       // 0x881500: a life at or below this ends the particle
const float kRestingLifeRatio = 0.019999999552965164f;   // 0x8810f0: a single particle outside its keys
const double kActiveThreshold = 0.9900000095367432;  // 0x8814e8: a deflector or force is on at or above this
const float kSmallestBounce = 0.0010000000474974513f;    // 0x7d43a0 (bejeweled3)
const double kGravityPerFramerate = 0.0005000000237487257;   // 0x8814f0
const double kForceFramerateScale = 0.08500000089406967;     // 0x8814e0
const double kForceFramerateSlope = 0.004000000189989805;    // 0x8814d8
const double kFreeEmitterMotionFactor = 30.0;        // 0x8814f8
const double kFreeEmitterSpinFactor = 160.0;         // 0x881240

// 0x4294f0 (bejeweled3): a float vector turned into a unit one (left alone when it has no length)
void NormalizeVector(float& theX, float& theY)
{
	float aLength = PIMath::Sqrt((float)((double)theX * (double)theX + (double)theY * (double)theY));
	if (aLength != 0.0f) {
		theX = (float)((double)theX / (double)aLength);
		theY = (float)((double)theY / (double)aLength);
	}
}

// 0x565e40 (bejeweled3): whether the segment a particle moved along crosses an edge
bool SegmentsCross(const PIPoint& theEdgeStart, const PIPoint& theEdgeEnd, const PIPoint& theMoveStart,
                   const PIPoint& theMoveEnd)
{
	double aMoveX = theMoveEnd.mX - theMoveStart.mX;
	double aDenominator = (theEdgeEnd.mY - theEdgeStart.mY) * aMoveX -
	                      (theMoveEnd.mY - theMoveStart.mY) * (theEdgeEnd.mX - theEdgeStart.mX);
	if (aDenominator == 0.0)
		return false;
	double aAlongMove = ((theMoveStart.mY - theEdgeStart.mY) * (theEdgeEnd.mX - theEdgeStart.mX) -
	                     (theEdgeEnd.mY - theEdgeStart.mY) * (theMoveStart.mX - theEdgeStart.mX)) / aDenominator;
	if (!(aAlongMove >= 0.0) || 1.0 < aAlongMove)
		return false;
	double aAlongEdge = ((theMoveStart.mY - theEdgeStart.mY) * aMoveX -
	                     (theMoveEnd.mY - theMoveStart.mY) * (theMoveStart.mX - theEdgeStart.mX)) / aDenominator;
	if (std::isnan(aAlongEdge) || aAlongEdge < 0.0)
		return false;
	return aAlongEdge <= 1.0;
}

// the one test of the ray that goes left from a point against an edge of a force's rectangle
bool RayCrossesEdge(const PIPoint& theFrom, const PIPoint& theTo, double thePointX, double thePointY)
{
	if (!((theFrom.mY <= thePointY && thePointY < theTo.mY) || (theTo.mY <= thePointY && thePointY < theFrom.mY)))
		return false;
	return thePointX < ((thePointY - theFrom.mY) * (theTo.mX - theFrom.mX)) / (theTo.mY - theFrom.mY) + theFrom.mX;
}

}

// 0x571ad0 (bejeweled3)
void PIEffect::UpdateParticleGroup(PILayer* theLayer, PIEmitterInstance* theInstance, PIParticleGroup* theGroup)
{
	float aStepDivisor = (float)(100.0 / (double)mAnimSpeed);
	PILayerDef* aLayerDef = theLayer->mDef;
	PIEmitterInstanceDef* anInstanceDef = theInstance->mDef;
	if (theGroup->mHead == NULL)
		return;

	double aStep = 1.0 / (double)aStepDivisor;
	PIParticleInstance* aParticle = theGroup->mHead;
	do {
		PIParticleInstance* aNext = aParticle->mNext;
		PIEmitter* anEmitter = aParticle->mEmitter;
		PIParticleDef* aParticleDef = aParticle->mParticleDef;
		bool anIsNew = aParticle->mAge == 0.0f;
		aParticle->mAge = (float)((double)aParticle->mAge + aStep);

		// how far through its life it is
		float aLifeRatio;
		if (aParticleDef == NULL || !aParticleDef->mSingleParticle) {
			aLifeRatio = (float)((double)aParticle->mAge / (double)aParticle->mLife);
		} else {
			// a single particle lives as long as its instance's keys: its ratio is how far along the active curve's
			// first span the effect is
			float aNextKeyTime = anInstanceDef->mActive.GetNextKeyTime(mFrameNum);
			int aNextKeyIndex = anInstanceDef->mActive.GetNextKeyIndex(mFrameNum);
			if (!(mFrameNum <= aNextKeyTime) || aNextKeyIndex != 1) {
				aLifeRatio = kRestingLifeRatio;
			} else {
				float aSpan = 1.0f <= aNextKeyTime ? aNextKeyTime : 1.0f;
				double aProgress = (double)anInstanceDef->mStartOffset + (double)mFrameNum;
				if (aProgress / (double)aSpan <= 1.0)
					aLifeRatio = (float)(aProgress / (double)aSpan);
				else
					aLifeRatio = 1.0f;
			}
		}
		aParticle->mLifeRatio = aLifeRatio;

		// the end of its life
		bool anEnded = !((double)aLifeRatio < kNearlyOne) || !(kTinyLife < aParticle->mLife) ||
		               (!theInstance->mWasInRange && !anInstanceDef->mHasFreeEmitters);
		if (anEnded) {
			bool aDelete;
			if (!theGroup->mIsFreeEmitterList)
				aDelete = !(aParticleDef->mSingleParticle && theInstance->mWasInRange);
			else
				aDelete = static_cast<PIFreeEmitterInstance*>(aParticle)->mParticles.mHead == NULL;
			if (aDelete) {
				theGroup->Unlink(aParticle);
				delete aParticle;
			}
			if (aDelete || theGroup->mIsFreeEmitterList) {
				aParticle = aNext;
				continue;
			}
		}

		// which frame of the texture
		if (aParticleDef != NULL) {
			PITexture& aTexture = mDef.mTextures[aParticleDef->mTextureIndex];
			int aFrameSource;
			if (aParticleDef->mFrameOffset == -1)
				aFrameSource = aParticle->mRandomFrame;
			else
				aFrameSource = Ftol((double)mFramerate * (double)aParticle->mAge / (double)(aParticleDef->mFrameOffset + 1)) +
				               aParticle->mRandomFrame;
			aParticle->mFrame = aFrameSource % aTexture.mFrameCount;
		}

		if (!theGroup->mIsFreeEmitterList && aParticleDef->mSingleParticle) {
			// a single particle stays where its emitter puts it
			int aPointCount = 0;
			if (anInstanceDef->mShape == 1 || anInstanceDef->mShape == 4)
				aPointCount = anInstanceDef->mShapeCount;
			else if (anInstanceDef->mShape == 3)
				aPointCount = anInstanceDef->mShapeRows * anInstanceDef->mShapeCount;
			if (aPointCount != 0) {
				PIPoint aGeomPos = GetGeomPos(theInstance, aParticle, NULL, NULL);
				PIPoint anEmitterPos = GetEmitterPos(theInstance, true);
				aParticle->mOriginX = anEmitterPos.mX;
				aParticle->mOriginY = anEmitterPos.mY;
				aParticle->mAttachedX = aParticle->mOriginX;
				aParticle->mAttachedY = aParticle->mOriginY;
				aParticle->mSpawnOffsetX = aGeomPos.mX - aParticle->mOriginX;
				aParticle->mSpawnOffsetY = aGeomPos.mY - aParticle->mOriginY;
				SexyMatrix3 anEmitterTurn;
				anEmitterTurn.LoadIdentity();
				PIMath::RotateDeg(anEmitterTurn, anInstanceDef->mRotation.GetValueAt(mFrameNum, 0.0f));
				// (the exe turns the point the shape gave, not the offset from the emitter: kept)
				PIPoint aTurned = PIMath::TransformPoint(anEmitterTurn, aGeomPos.mX, aGeomPos.mY);
				aParticle->mOriginX = aParticle->mOriginX + aTurned.mX;
				aParticle->mOriginY = aTurned.mY + aParticle->mOriginY;
			}
			if (aParticleDef->mAlignToEmitterAngle && !aParticleDef->mAttachToEmitter) {
				PIPoint aVelocity = anInstanceDef->mPosition.GetVelocityAt(mFrameNum);
				double aSpeed = sqrt(aVelocity.mY * aVelocity.mY + aVelocity.mX * aVelocity.mX);
				double aDirection;
				if (aSpeed == 0.0)
					aDirection = 0.0;
				else
					aDirection = -atan2(aVelocity.mY, aVelocity.mX);
				aParticle->mAngle = (float)aDirection;
				float anAlignOffset = PIMath::DegreesToRadians((float)-aParticleDef->mAlignOffsetDegrees);
				aParticle->mAngle = (float)((double)anAlignOffset + (double)aParticle->mAngle);
			}
		} else {
			if (mFrameChanged) {
				// the particle's motion wanders a little each whole frame
				float aDrawA = GetRandFloat();
				float aDrawB = GetRandFloat();
				float aDrawC = GetRandFloat();
				float aDrawD = GetRandFloat();
				float aWanderX = (float)((double)aDrawB * (double)aDrawA);
				float aWanderY = (float)((double)aDrawD * (double)aDrawC);
				PIValue& aScaleValue = theGroup->mIsFreeEmitterParticles ? anEmitter->mMotionRandomnessScale
				                                                         : anInstanceDef->mMotionRandomnessScale;
				float aWanderAmount;
				if (!theGroup->mIsFreeEmitterList) {
					float aScale = aScaleValue.GetValueAt(mFrameNum, 0.0f);
					double aBase = (double)aParticleDef->mMotionRandomness.GetValueAt(mFrameNum, 0.0f) +
					               (double)aParticle->mMotionRandomnessVariation;
					float anOverLife = aParticleDef->mMotionRandomnessOverLife.GetValueAt(aLifeRatio, 0.0f);
					double aProduct = ((double)anOverLife * (double)aScale) * aBase;
					aWanderAmount = (float)(aProduct < 0.0 ? 0.0 : aProduct);
				} else {
					float aScale = aScaleValue.GetValueAt(mFrameNum, 0.0f);
					double aBase = (double)anEmitter->mMotionRandomness.GetValueAt(mFrameNum, 0.0f) +
					               (double)aParticle->mMotionRandomnessVariation;
					float anOverLife = anEmitter->mMotionRandomnessOverLife.GetValueAt(aLifeRatio, 1.0f);
					double aProduct = ((double)anOverLife * (double)aScale) * aBase;
					aWanderAmount = (float)((aProduct < 0.0 ? 0.0 : aProduct) * kFreeEmitterMotionFactor);
				}
				aParticle->mVelocityX = (double)aWanderAmount * (double)aWanderX + aParticle->mVelocityX;
				aParticle->mVelocityY = (double)aWanderAmount * (double)aWanderY + aParticle->mVelocityY;
			}

			// gravity
			double aWeightFactor;
			if (!theGroup->mIsFreeEmitterList) {
				PIValue& aScaleValue = theGroup->mIsFreeEmitterParticles ? anEmitter->mWeightScale : anInstanceDef->mWeightScale;
				float aScale = aScaleValue.GetValueAt(mFrameNum, 0.0f);
				double aWeight = (double)aParticleDef->mWeight.GetValueAt(mFrameNum, 0.0f) + (double)aParticle->mWeightVariation;
				float anOverLife = aParticleDef->mWeightOverLife.GetValueAt(aLifeRatio, 0.0f);
				aWeightFactor = (((double)anOverLife - 1.0) * (double)aScale) * aWeight;
			} else {
				PIValue& aScaleValue = theGroup->mIsFreeEmitterParticles ? anEmitter->mWeightScale : anInstanceDef->mWeightScale;
				float aScale = aScaleValue.GetValueAt(mFrameNum, 0.0f);
				double aWeight = (double)anEmitter->mWeight.GetValueAt(mFrameNum, 0.0f) + (double)aParticle->mWeightVariation;
				float anOverLife = anEmitter->mWeightOverLife.GetValueAt(aLifeRatio, 1.0f);
				aWeightFactor = ((((double)anOverLife - 1.0) * (double)aScale) * aWeight) * kHalf;
			}
			float aGravityPerSecond = (float)(aWeightFactor * kHundred);
			float aGravityStep = (float)((((double)mFramerate - 100.0) * kGravityPerFramerate + 1.0) * (double)aGravityPerSecond);
			aParticle->mVelocityY = (double)aGravityStep / (double)aStepDivisor + aParticle->mVelocityY;

			double aStepX = aParticle->mVelocityX / (double)aStepDivisor;
			double aStepY = aParticle->mVelocityY / (double)aStepDivisor;
			float aVelocityOverLife;
			if (!theGroup->mIsFreeEmitterList)
				aVelocityOverLife = aParticleDef->mVelocityOverLife.GetValueAt(aLifeRatio, 0.0f);
			else
				aVelocityOverLife = anEmitter->mVelocityOverLife.GetValueAt(aLifeRatio, 1.0f);
			aStepX = aStepX * (double)aVelocityOverLife;
			aStepY = aStepY * (double)aVelocityOverLife;

			// where it is now, in the effect's own plane (for the deflectors and forces)
			PIPoint aWorld;
			aWorld.mX = 0.0;
			aWorld.mY = 0.0;
			bool aHasForces = !aLayerDef->mForces.empty();
			bool aHasDeflectors = !aLayerDef->mDeflectors.empty();
			if (anIsNew || !aHasDeflectors) {
				aParticle->mPositionX = aStepX + aParticle->mPositionX;
				aParticle->mPositionY = aStepY + aParticle->mPositionY;
				if (aHasForces) {
					CalcParticleTransform(theLayer, theInstance, anEmitter, aParticleDef, theGroup, aParticle);
					aWorld = PIMath::TransformPoint(aParticle->mTransform, 0.0, 0.0);
				}
			} else {
				PIPoint aPreviousWorld = PIMath::TransformPoint(aParticle->mTransform, 0.0, 0.0);
				double aPreviousX = aParticle->mPositionX;
				double aPreviousY = aParticle->mPositionY;
				aParticle->mPositionX = aParticle->mPositionX + aStepX;
				aParticle->mPositionY = aStepY + aParticle->mPositionY;
				CalcParticleTransform(theLayer, theInstance, anEmitter, aParticleDef, theGroup, aParticle);
				aWorld = PIMath::TransformPoint(aParticle->mTransform, 0.0, 0.0);

				double aDrawX = (double)mDrawTransform.m02;
				double aDrawY = (double)mDrawTransform.m12;
				for (size_t aDeflectorIndex = 0; aDeflectorIndex < aLayerDef->mDeflectors.size(); aDeflectorIndex++) {
					PIDeflector& aDeflector = aLayerDef->mDeflectors[aDeflectorIndex];
					if (aDeflector.mActive.GetLastKeyValue(mFrameNum) < kActiveThreshold)
						continue;
					if (aDeflector.mTransformedPoints.size() <= 1)
						continue;
					for (size_t aPointIndex = 1; aPointIndex < aDeflector.mTransformedPoints.size(); aPointIndex++) {
						double aFromX = aDeflector.mTransformedPoints[aPointIndex - 1].mX - aDrawX;
						double aFromY = aDeflector.mTransformedPoints[aPointIndex - 1].mY - aDrawY;
						double aToX = aDeflector.mTransformedPoints[aPointIndex].mX - aDrawX;
						double aToY = aDeflector.mTransformedPoints[aPointIndex].mY - aDrawY;
						// the edge's unit normal, pushed out by the deflector's allowed direction and the particle's
						// own random value (so particles hit slightly different lines)
						float anEdgeX = (float)(aToX - aFromX);
						float anEdgeY = (float)(aToY - aFromY);
						NormalizeVector(anEdgeX, anEdgeY);
						float aNormalX = -anEdgeY;
						float aNormalY = anEdgeX;
						double aPushX = ((double)aNormalX * (double)aDeflector.mAllowedDirection) * (double)aParticle->mRandomValue;
						double aPushY = ((double)aNormalY * (double)aDeflector.mAllowedDirection) * (double)aParticle->mRandomValue;
						PIPoint anEdgeStart;
						anEdgeStart.mX = aPushX + aFromX;
						anEdgeStart.mY = aFromY + aPushY;
						PIPoint anEdgeEnd;
						anEdgeEnd.mX = aPushX + aToX;
						anEdgeEnd.mY = aToY + aPushY;
						if (!SegmentsCross(anEdgeStart, anEdgeEnd, aPreviousWorld, aWorld))
							continue;
						float aHitDraw = GetRandFloatU();
						if ((double)aDeflector.mHits < (double)aHitDraw)
							continue;

						// bounce: the step is mirrored about the edge; a weak bounce is softened toward its square root
						float aBounceScale;
						double aBounceBase;
						float aBounceOverLife;
						PIValue& aScaleValue = theGroup->mIsFreeEmitterParticles ? anEmitter->mBounceScale : anInstanceDef->mBounceScale;
						aBounceScale = aScaleValue.GetValueAt(mFrameNum, 0.0f);
						if (!theGroup->mIsFreeEmitterList) {
							aBounceBase = (double)aParticleDef->mBounce.GetValueAt(mFrameNum, 0.0f) + (double)aParticle->mBounceVariation;
							aBounceOverLife = aParticleDef->mBounceOverLife.GetValueAt(aLifeRatio, 0.0f);
						} else {
							aBounceBase = (double)anEmitter->mBounce.GetValueAt(mFrameNum, 0.0f) + (double)aParticle->mBounceVariation;
							aBounceOverLife = anEmitter->mBounceOverLife.GetValueAt(aLifeRatio, 1.0f);
						}
						float aStrength = (float)((((double)aBounceOverLife * (double)aBounceScale) * aBounceBase) *
						                          (double)aDeflector.mBounce);

						float aStepFloatX = (float)aStepX;
						float aStepFloatY = (float)aStepY;
						float aAlongNormal = (float)((double)aNormalY * (double)aStepFloatY + (double)aNormalX * (double)aStepFloatX);
						float aDoubledNormalX = (float)((double)aNormalX * kTwo);
						float aDoubledNormalY = (float)((double)aNormalY * kTwo);
						float aMirrorX = (float)((double)aDoubledNormalX * (double)aAlongNormal);
						float aMirrorY = (float)((double)aAlongNormal * (double)aDoubledNormalY);
						float aReflectedX = (float)((double)aStepFloatX - (double)aMirrorX);
						float aReflectedY = (float)((double)aStepFloatY - (double)aMirrorY);
						float aRatio = (float)((double)aReflectedY / (double)aReflectedX);
						aRatio = fabsf(aRatio);
						if ((double)aRatio > 1.0)
							aRatio = 1.0f;
						float aRoot = (float)pow((double)aStrength, kHalf);
						float aSoftenedY = (float)((((1.0 - (double)aRatio) + (double)aRoot * (double)aRatio)) * (double)aReflectedY);
						aParticle->mVelocityX = (double)aReflectedX * kHundred;
						aParticle->mVelocityY = (double)aSoftenedY * kHundred;
						if ((double)kSmallestBounce < (double)aStrength) {
							aParticle->mPositionX = aPreviousX;
							aParticle->mPositionY = aPreviousY;
						}
						CalcParticleTransform(theLayer, theInstance, anEmitter, aParticleDef, theGroup, aParticle);
						aWorld = PIMath::TransformPoint(aParticle->mTransform, 0.0, 0.0);
					}
				}
			}

			// forces: a particle inside a force's rectangle is pushed along its direction
			for (size_t aForceIndex = 0; aForceIndex < aLayerDef->mForces.size(); aForceIndex++) {
				PIForce& aForce = aLayerDef->mForces[aForceIndex];
				if (aForce.mCurves[2].GetLastKeyValue(mFrameNum) < kActiveThreshold)
					continue;
				bool anInside = false;
				if (RayCrossesEdge(aForce.mCorners[0], aForce.mCorners[3], aWorld.mX, aWorld.mY))
					anInside = true;
				if (RayCrossesEdge(aForce.mCorners[1], aForce.mCorners[0], aWorld.mX, aWorld.mY))
					anInside = !anInside;
				if (RayCrossesEdge(aForce.mCorners[2], aForce.mCorners[1], aWorld.mX, aWorld.mY))
					anInside = !anInside;
				if (RayCrossesEdge(aForce.mCorners[3], aForce.mCorners[2], aWorld.mX, aWorld.mY))
					anInside = !anInside;
				if (!anInside)
					continue;

				float aRectangleAngle = PIMath::DegreesToRadians(-aForce.mCurves[3].GetValueAt(mFrameNum, 0.0f));
				float aDirection = PIMath::DegreesToRadians(-aForce.mCurves[1].GetValueAt(mFrameNum, 0.0f));
				float aTurn = (float)((double)aDirection + (double)aRectangleAngle);
				float aFramerate = (float)mFramerate;
				float aFramerateShare = (float)(((double)aFramerate * kForceFramerateScale) / kHundred);
				float aFramerateFactor = (float)((double)aFramerateShare *
				                                 (((double)aFramerate - 100.0) * kForceFramerateSlope + 1.0));
				float aPush = (float)((double)aForce.mCurves[0].GetValueAt(mFrameNum, 0.0f) * (double)aFramerateFactor);
				aParticle->mVelocityX = ((double)PIMath::Cos(aTurn) * (double)aPush) * kHundred + aParticle->mVelocityX;
				aParticle->mVelocityY = ((double)PIMath::Sin(aTurn) * (double)aPush) * kHundred + aParticle->mVelocityY;
			}

			if (!theGroup->mIsFreeEmitterList && aParticleDef->mAlignToMotion && aParticleDef->mAlignToEmitterAngle) {
				float anAlignOffset = PIMath::DegreesToRadians((float)-aParticleDef->mAlignOffsetDegrees);
				aParticle->mAngle = (float)((double)anAlignOffset - atan2(aStepY, aStepX));
			}
		}

		// the color the layer's picture gives it
		if (aParticleDef != NULL) {
			bool aTakesColor = (!aParticle->mDrawn && aParticleDef->mUseInstanceColor) || aParticleDef->mFollowInstanceColor;
			bool aTakesAlpha = (!aParticle->mDrawn && aParticleDef->mUseInstanceAlpha) || aParticleDef->mFollowInstanceAlpha;
			if (aTakesColor || aTakesAlpha) {
				PIPoint aWhere = PIMath::TransformPoint(aParticle->mTransform, 0.0, 0.0);
				int aPixelX = Ftol(aWhere.mX) + theLayer->mTintOffsetX;
				int aPixelY = Ftol(aWhere.mY) + theLayer->mTintOffsetY;
				unsigned int aPixel = 0;
				Image* aTintImage = theLayer->mTintImage;
				if (aTintImage != NULL && aPixelX >= 0 && aPixelY >= 0 && aPixelX < aTintImage->mWidth &&
				    aPixelY < aTintImage->mHeight) {
					unsigned long* aBits = ((MemoryImage*)aTintImage)->GetBits();
					aPixel = (unsigned int)aBits[aPixelY * aTintImage->mWidth + aPixelX];
				}
				if (aTakesColor)
					aParticle->mInstanceColor = aParticle->mInstanceColor ^ ((aParticle->mInstanceColor ^ aPixel) & 0xffffff);
				if (aTakesAlpha)
					aParticle->mInstanceColor = ((aParticle->mInstanceColor ^ aPixel) & 0xffffff) ^ aPixel;
			}
		}

		// spin
		if (!theGroup->mIsFreeEmitterList) {
			if (!aParticleDef->mAlignToEmitterAngle) {
				PIValue& aScaleValue = theGroup->mIsFreeEmitterParticles ? anEmitter->mSpinScale : anInstanceDef->mSpinScale;
				float aScale = aScaleValue.GetValueAt(mFrameNum, 0.0f);
				double aSpin = (double)aParticleDef->mSpin.GetValueAt(mFrameNum, 0.0f) + (double)aParticle->mSpinVariation;
				float anOverLife = aParticleDef->mSpinOverLife.GetValueAt(aLifeRatio, 0.0f);
				float aTurnPerSecond = (float)-((((double)anOverLife - 1.0) * (double)aScale) * aSpin);
				float aTurn = PIMath::DegreesToRadians(aTurnPerSecond);
				aParticle->mAngle = (float)((double)aTurn / (double)aStepDivisor + (double)aParticle->mAngle);
			}
		} else {
			PIValue& aScaleValue = theGroup->mIsFreeEmitterParticles ? anEmitter->mSpinScale : anInstanceDef->mSpinScale;
			float aScale = aScaleValue.GetValueAt(mFrameNum, 0.0f);
			double aSpin = (double)anEmitter->mSpin.GetValueAt(mFrameNum, 0.0f) + (double)aParticle->mSpinVariation;
			float anOverLife = anEmitter->mSpinOverLife.GetValueAt(aLifeRatio, 1.0f);
			float aTurnPerSecond = (float)-((((double)anOverLife - 1.0) * (double)aScale) * aSpin);
			float aTurn = PIMath::DegreesToRadians(aTurnPerSecond);
			aParticle->mAngle = (float)(((double)aTurn / (double)aStepDivisor) * kFreeEmitterSpinFactor + (double)aParticle->mAngle);
		}

		aParticle = aNext;
	} while (aParticle != NULL);
}

}
