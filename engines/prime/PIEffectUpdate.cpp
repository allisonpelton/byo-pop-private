// Sexy::PIEffect (PIEffect.h): one Update of the effect, and the functions that start it over or ask whether it is over.
// The exe's x87 arithmetic is kept: sums and products are done in double and rounded to a float where the exe stores a
// float.
#include "PIEffect.h"

#include "Common.h"
#include "PIEffectMath.h"
#include "X86Math.h"

namespace Sexy {

namespace {

const double kHalf = 0.5;                           // 0x8d3370
const double kFirstStepFrame = 9.999999747378752e-05;   // 0x881968: the first Update moves the time this little
const double kActiveThreshold = 0.9900000095367432;     // 0x8814e8: an "active" value above this means on

}

// 0x573290 (bejeweled3)
void PIEffect::Update()
{
	if (!mError.empty())
		return;

	mUpdateCount++;
	bool anAtStart = (mFrameNum == 0.0f);

	if (mNeedsSeed) {
		int aSeed;
		if (mSeeds.empty()) {
			aSeed = Rand();
		} else {
			aSeed = mSeeds[(unsigned int)Rand() % (unsigned int)mSeeds.size()];
		}
		mRand.SRand((unsigned long)aSeed);
		mNeedsSeed = false;
	}

	bool aFirstPass = true;
	while (mFrameNum < (float)mFirstFrameNum || aFirstPass) {
		float aStepDivisor = (float)(100.0 / (double)mAnimSpeed);
		aFirstPass = false;
		mEmitterCount = 0;
		mParticleCount = 0;
		int anOldWholeFrame = Ftol((double)mFrameNum);
		if (anAtStart)
			mFrameNum = (float)((double)mFrameNum + kFirstStepFrame);
		else
			mFrameNum = (float)((double)mFramerate / (double)aStepDivisor + (double)mFrameNum);
		mFrameChanged = anOldWholeFrame != Ftol((double)mFrameNum);

		for (size_t aLayerIndex = 0; aLayerIndex < mDef.mLayers.size(); aLayerIndex++) {
			PILayerDef& aLayerDef = mDef.mLayers[aLayerIndex];
			PILayer* aLayer = mLayers[aLayerIndex];
			if (!aLayer->mVisible)
				continue;

			// where the deflectors' points are now
			for (size_t aDeflectorIndex = 0; aDeflectorIndex < aLayerDef.mDeflectors.size(); aDeflectorIndex++) {
				PIDeflector& aDeflector = aLayerDef.mDeflectors[aDeflectorIndex];
				SexyMatrix3 aMatrix;
				aMatrix.LoadIdentity();
				float aDeflectorAngle = aDeflector.mAngle.GetValueAt(mFrameNum, 0.0f);
				if (aDeflectorAngle != 0.0f)
					PIMath::RotateDeg(aMatrix, aDeflectorAngle);
				PIPoint aDeflectorPos = aDeflector.mPosition.GetValueAt(mFrameNum);
				PIMath::Translate(aMatrix, (float)aDeflectorPos.mX, (float)aDeflectorPos.mY);
				PIPoint aLayerPos = aLayerDef.mPosition.GetValueAt(mFrameNum);
				PIMath::Translate(aMatrix, (float)aLayerPos.mX, (float)aLayerPos.mY);
				float aLayerAngle = aLayerDef.mAngle.GetValueAt(mFrameNum, 0.0f);
				if (aLayerAngle != 0.0f)
					PIMath::RotateDeg(aMatrix, aLayerAngle);
				aMatrix = PIMath::Multiply(mDrawTransform, aMatrix);
				if (aDeflector.mTransformedPoints.size() < aDeflector.mPoints.size())
					aDeflector.mTransformedPoints.resize(aDeflector.mPoints.size());
				for (size_t aPointIndex = 0; aPointIndex < aDeflector.mPoints.size(); aPointIndex++) {
					PIPoint aPoint = aDeflector.mPoints[aPointIndex].GetValueAt(mFrameNum);
					aDeflector.mTransformedPoints[aPointIndex] = PIMath::TransformPoint(aMatrix, aPoint.mX, aPoint.mY);
				}
			}

			// where the forces' rectangles are now: their corners, then their middle
			for (size_t aForceIndex = 0; aForceIndex < aLayerDef.mForces.size(); aForceIndex++) {
				PIForce& aForce = aLayerDef.mForces[aForceIndex];
				SexyMatrix3 aMatrix;
				aMatrix.LoadIdentity();
				float aHalfHeight = (float)((double)aForce.mCurves[5].GetValueAt(mFrameNum, 0.0f) * kHalf);
				float aHalfWidth = (float)((double)aForce.mCurves[4].GetValueAt(mFrameNum, 0.0f) * kHalf);
				PIMath::Scale(aMatrix, aHalfWidth, aHalfHeight);
				float aForceAngle = aForce.mCurves[3].GetValueAt(mFrameNum, 0.0f);
				if (aForceAngle != 0.0f)
					PIMath::RotateDeg(aMatrix, aForceAngle);
				PIPoint aForcePos = aForce.mPosition.GetValueAt(mFrameNum);
				PIMath::Translate(aMatrix, (float)aForcePos.mX, (float)aForcePos.mY);
				PIPoint aLayerPos = aLayerDef.mPosition.GetValueAt(mFrameNum);
				PIMath::Translate(aMatrix, (float)aLayerPos.mX, (float)aLayerPos.mY);
				float aLayerAngle = aLayerDef.mAngle.GetValueAt(mFrameNum, 0.0f);
				if (aLayerAngle != 0.0f)
					PIMath::RotateDeg(aMatrix, aLayerAngle);
				aMatrix = PIMath::Multiply(mDrawTransform, aMatrix);
				static const double kCornerX[5] = {-1.0, 1.0, 1.0, -1.0, 0.0};
				static const double kCornerY[5] = {-1.0, -1.0, 1.0, 1.0, 0.0};
				for (int aCorner = 0; aCorner < 5; aCorner++)
					aForce.mCorners[aCorner] = PIMath::TransformPoint(aMatrix, kCornerX[aCorner], kCornerY[aCorner]);
			}

			for (size_t anInstanceIndex = 0; anInstanceIndex < aLayer->mEmitterInstances.size(); anInstanceIndex++) {
				PIEmitterInstanceDef& anInstanceDef = aLayerDef.mEmitterInstances[anInstanceIndex];
				PIEmitterInstance* anInstance = aLayer->mEmitterInstances[anInstanceIndex];
				int aCatchUp = 1;
				int aParticleCount = 0;
				int anEmitterCount = 0;
				while (anInstance->mVisible && aCatchUp > 0) {
					aCatchUp--;
					anEmitterCount = 0;
					aParticleCount = 0;
					bool anInRange;
					if (anInstanceDef.mActive.GetLastKeyValue(mFrameNum) <= kActiveThreshold) {
						anInRange = false;
						aCatchUp = 0;
					} else {
						anInRange = true;
						if (!anInstance->mWasInRange)
							aCatchUp = Ftol((double)anInstanceDef.mStartOffset * (double)aStepDivisor / (double)mFramerate +
							                (double)aCatchUp);
					}
					anInstance->mWasInRange = anInRange;

					float aFirstKeyTime = anInstanceDef.mActive.GetNextKeyTime(0.0f);
					float aLastFramePlusOne = (float)((double)mLastFrameNum + 1.0);
					float aLastKeyTime = anInstanceDef.mActive.GetLastKeyTime(aLastFramePlusOne);
					float aLastKeyValue = anInstanceDef.mActive.GetLastKeyValue(aLastFramePlusOne);
					bool aCanEmit;
					if (mFrameNum < aFirstKeyTime || (aLastKeyTime <= mFrameNum && (double)aLastKeyValue <= 0.99) ||
					    (!mLoop && (float)mLastFrameNum <= mFrameNum))
						aCanEmit = false;
					else
						aCanEmit = true;
					anInstance->mCanEmit = aCanEmit;

					if (anInRange || aCanEmit)
						anEmitterCount = 1;

					if (!anInstanceDef.mHasFreeEmitters) {
						// the instance's emitter makes its particles itself
						PIEmitter& anEmitter = mDef.mEmitters[anInstanceDef.mEmitterIndex];
						for (size_t aDefIndex = 0; aDefIndex < anEmitter.mParticleDefs.size(); aDefIndex++)
							UpdateParticleDef(&anEmitter, anInstance, &anEmitter.mParticleDefs[aDefIndex],
							                  &anInstance->mDefInstances[aDefIndex], &anInstance->mParticles, NULL);
						UpdateParticleGroup(aLayer, anInstance, &anInstance->mParticles);
						aParticleCount = anInstance->mParticles.mCount;
					} else {
						// a super emitter makes free emitters, and each of those makes particles
						for (size_t aFreeIndex = 0; aFreeIndex < anInstanceDef.mFreeEmitterIndices.size(); aFreeIndex++) {
							PIEmitter& aFreeEmitterDef = mDef.mEmitters[anInstanceDef.mFreeEmitterIndices[aFreeIndex]];
							UpdateParticleDef(&aFreeEmitterDef, anInstance, NULL, &anInstance->mFreeDefInstances[aFreeIndex],
							                  &anInstance->mFreeEmitters, NULL);
						}
						UpdateParticleGroup(aLayer, anInstance, &anInstance->mFreeEmitters);
						PIParticleInstance* aNext;
						for (PIParticleInstance* aParticle = anInstance->mFreeEmitters.mHead; aParticle != NULL; aParticle = aNext) {
							aNext = aParticle->mNext;
							PIFreeEmitterInstance* aFreeEmitter = (PIFreeEmitterInstance*)aParticle;
							PIEmitter* anEmitter = aFreeEmitter->mEmitter;
							for (size_t aDefIndex = 0; aDefIndex < anEmitter->mParticleDefs.size(); aDefIndex++)
								UpdateParticleDef(anEmitter, anInstance, &anEmitter->mParticleDefs[aDefIndex],
								                  &aFreeEmitter->mDefInstances[aDefIndex], &aFreeEmitter->mParticles,
								                  aFreeEmitter);
							UpdateParticleGroup(aLayer, anInstance, &aFreeEmitter->mParticles);
							aParticleCount += aFreeEmitter->mParticles.mCount;
							anEmitterCount++;
						}
					}
				}
				mEmitterCount += anEmitterCount;
				mParticleCount += aParticleCount;
			}
		}
		anAtStart = false;
	}
}

// 0x56f730 (bejeweled3)
void PIEffect::ResetAnim()
{
	mFrameNum = 0.0f;
	for (size_t aLayerIndex = 0; aLayerIndex < mLayers.size(); aLayerIndex++) {
		PILayer* aLayer = mLayers[aLayerIndex];
		for (size_t anInstanceIndex = 0; anInstanceIndex < aLayer->mEmitterInstances.size(); anInstanceIndex++) {
			PIEmitterInstance* anInstance = aLayer->mEmitterInstances[anInstanceIndex];
			anInstance->DeleteParticles();
			for (size_t i = 0; i < anInstance->mFreeDefInstances.size(); i++)
				anInstance->mFreeDefInstances[i] = PIParticleDefInstance();
			for (size_t i = 0; i < anInstance->mDefInstances.size(); i++)
				anInstance->mDefInstances[i] = PIParticleDefInstance();
			anInstance->mCanEmit = true;
			anInstance->mWasInRange = false;
		}
	}
	mEmitterCount = 0;
	mParticleCount = 0;
	mDrawArea = 0;
	mNeedsSeed = true;
}

// 0x56fab0 (bejeweled3)
void PIEffect::Clear()
{
	ResetAnim();
	mDef.mEmitters.clear();
	mDef.mTextures.clear();
	mDef.mLayers.clear();
	for (size_t i = 0; i < mLayers.size(); i++)
		delete mLayers[i];
	mLayers.clear();
	mSeeds.clear();
	mVersion = 0;
	mLoaded = false;
}

// 0x575790 (bejeweled3)
bool PIEffect::IsActive()
{
	for (size_t aLayerIndex = 0; aLayerIndex < mDef.mLayers.size(); aLayerIndex++) {
		PILayerDef& aLayerDef = mDef.mLayers[aLayerIndex];
		PILayer* aLayer = mLayers[aLayerIndex];
		if (!aLayer->mVisible)
			continue;
		for (size_t anInstanceIndex = 0; anInstanceIndex < aLayer->mEmitterInstances.size(); anInstanceIndex++) {
			PIEmitterInstance* anInstance = aLayer->mEmitterInstances[anInstanceIndex];
			if (!anInstance->mVisible)
				continue;
			float aNextKeyTime = aLayerDef.mEmitterInstances[anInstanceIndex].mActive.GetNextKeyTime(mFrameNum);
			if (mFrameNum <= aNextKeyTime || anInstance->mCanEmit || anInstance->mFreeEmitters.mHead != NULL ||
			    anInstance->mParticles.mHead != NULL)
				return true;
		}
	}
	return false;
}

// port: not translated yet (the next step of the PIEffect work: DrawLayer and DrawParticleGroup)
// 0x575650 (bejeweled3)
void PIEffect::Draw(Graphics* /*g*/)
{
}

}
