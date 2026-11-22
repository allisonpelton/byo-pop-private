// The run-time classes of Sexy::PIEffect (PIEffectRuntime.h): constructors, and the particle lists.
#include "PIEffectRuntime.h"

namespace Sexy {

// 0x565540 (bejeweled3)
PIParticleInstance::PIParticleInstance()
	: mPrev(NULL), mNext(NULL), mParticleDef(NULL), mEmitter(NULL), mIndex(0), mFreeEmitter(NULL), mPositionX(0.0),
	  mPositionY(0.0), mSpawnOffsetX(0.0), mSpawnOffsetY(0.0), mOriginX(0.0), mOriginY(0.0), mAttachedX(0.0),
	  mAttachedY(0.0), mVelocityX(0.0), mVelocityY(0.0), mAngle(0.0f), mLifeVariation(0.0f), mSizeXVariation(0.0f),
	  mSizeYVariation(0.0f), mVelocityVariation(0.0f), mWeightVariation(0.0f), mSpinVariation(0.0f),
	  mMotionRandomnessVariation(0.0f), mBounceVariation(0.0f), mFreeEmitterNumberVariation(0.0f), mScale(0.0f),
	  mSizeX(1.0f), mSizeY(1.0f), mColorOffset(0.0f), mEmitterRotation(0.0f), mRandomFrame(0), mDrawArea(1.0f),
	  mFrame(0), mRandomValue(0.0f), mAge(0.0f), mLife(0.0f), mLifeRatio(0.0f), mDrawn(false),
	  mInstanceColor(0xffffffffu)
{
	mTransform.LoadIdentity();
}

// 0x5655c0 (bejeweled3)
PIFreeEmitterInstance::PIFreeEmitterInstance()
{
	mParticles.mIsFreeEmitterParticles = true;
}

// port: the exe's list code is written out in UpdateParticleDef (0x5707d0 (bejeweled3)), where a particle is added at the end
void PIParticleGroup::Append(PIParticleInstance* theParticle)
{
	if (mTail != NULL)
		mTail->mNext = theParticle;
	theParticle->mPrev = mTail;
	if (mHead == NULL)
		mHead = theParticle;
	mCount++;
	mTail = theParticle;
}

// port: the other branch of the same code, where the emitter wants the oldest particles in front
void PIParticleGroup::Prepend(PIParticleInstance* theParticle)
{
	if (mHead != NULL)
		mHead->mPrev = theParticle;
	theParticle->mNext = mHead;
	if (mTail == NULL)
		mTail = theParticle;
	mCount++;
	mHead = theParticle;
}

// port: the list code in UpdateParticleGroup (0x571ad0 (bejeweled3)), where a particle ends
void PIParticleGroup::Unlink(PIParticleInstance* theParticle)
{
	if (theParticle->mPrev != NULL)
		theParticle->mPrev->mNext = theParticle->mNext;
	if (theParticle->mNext != NULL)
		theParticle->mNext->mPrev = theParticle->mPrev;
	if (mHead == theParticle)
		mHead = theParticle->mNext;
	if (mTail == theParticle)
		mTail = theParticle->mPrev;
	mCount--;
}

// port: the loops of ResetAnim (0x56f730 (bejeweled3)) that give every particle (and, for a free emitter, its particles) back
void PIParticleGroup::DeleteAll()
{
	PIParticleInstance* aParticle = mHead;
	while (aParticle != NULL) {
		PIParticleInstance* aNext = aParticle->mNext;
		delete aParticle;
		aParticle = aNext;
	}
	mHead = NULL;
	mTail = NULL;
	mCount = 0;
}

// 0x565630 (bejeweled3)
PIEmitterInstance::PIEmitterInstance(PIEmitterInstanceDef* theDef)
	: mDef(theDef), mWasInRange(false), mCanEmit(false), mColor(255, 255, 255, 255), mOffsetX(0.0), mOffsetY(0.0),
	  mNumberScale(1.0f), mVisible(true)
{
	mTransform.LoadIdentity();
	mFreeEmitters.mIsFreeEmitterList = true;
}

PIEmitterInstance::~PIEmitterInstance()
{
	DeleteParticles();
}

// port: the loops of ResetAnim (0x56f730 (bejeweled3)) for one emitter instance
void PIEmitterInstance::DeleteParticles()
{
	mFreeEmitters.DeleteAll();
	mParticles.DeleteAll();
}

// 0x565710 (bejeweled3)
PILayer::PILayer(PILayerDef* theDef)
	: mDef(theDef), mVisible(true), mColor(255, 255, 255, 255), mTintImage(NULL), mTintOffsetX(0), mTintOffsetY(0)
{
}

PILayer::~PILayer()
{
	for (size_t i = 0; i < mEmitterInstances.size(); i++)
		delete mEmitterInstances[i];
}

}
