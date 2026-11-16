#include "ParticleSystem.h"

#include "DrawContext.h"
#include "Engine.h"
#include "Image.h"
#include "Modifiers.h"
#include "Random.h"

#include <cmath>

namespace engine
{

// 0x43cde0 (feedingfrenzy2): a vector's length, each square and their sum rounded to floats
static float lengthOf(const Vector2f& theVector)
{
	float aSquareY = theVector.y * theVector.y;
	float aSquareX = theVector.x * theVector.x;
	float aSum = (float)((double)aSquareX + aSquareY);
	return (float)sqrt((double)aSum);
}

// 0x4345f0 (feedingfrenzy2): a vector made a unit long, left as it is when its length is 0
static void normalize(Vector2f& theVector)
{
	float aLength = lengthOf(theVector);
	if (aLength == 0.0f)
		return;
	double anInverse = 1.0 / aLength;
	theVector.x = (float)(theVector.x * anInverse);
	theVector.y = (float)(anInverse * theVector.y);
}

Particle::Particle()
	: mStream(nullptr), mVelocity(0, 0), mAcceleration(0), mStartScale(1), mEndScale(1), mStartOpacity(1),
	  mEndOpacity(1), mExternalForce(0, 0), mBlackHole(false), mLifeTime(0), mLife(0), mOffsetParent(nullptr),
	  mOffsetType(PARTICLE_OFFSET_INDEPENDENT), mHasOpacityKeys(false)
{
	addOwnState(STATE_ANIMATED);
}

void Particle::applyTransform(DrawContext* theContext)
{
	Container::applyTransform(theContext);
	if (mOffsetType != PARTICLE_OFFSET_INDEPENDENT)
		return;
	DrawState& aState = theContext->back();
	float aDeltaX;
	float aDeltaY;
	if (mOffsetParent == nullptr)
	{
		aDeltaX = (float)((double)mPosition.x - aState.mPosition.x);
		aDeltaY = (float)((double)mPosition.y - aState.mPosition.y);
	}
	else
	{
		// The original asks the parent for its position twice, once for each axis
		Vector2f aParentPosition = mOffsetParent->getAbsolutePosition();
		aDeltaX = (float)(((double)mPosition.x - aState.mPosition.x) + aParentPosition.x);
		aDeltaY = (float)(((double)aParentPosition.y + mPosition.y) - aState.mPosition.y);
	}
	// 0x4523a0 (feedingfrenzy2)
	aState.mPosition.x = aState.mPosition.x + aDeltaX;
	aState.mPosition.y = aState.mPosition.y + aDeltaY;
}

void Particle::update(UpdateContext& theContext)
{
	// The original keeps each step on the x87 stack (53 bits) and rounds where it stores
	float anElapsed = theContext.mElapsed;
	mLife = (float)((double)mLife - anElapsed);
	Vector2f aVelocity = mVelocity;
	float x = getX();
	float y = getY();
	float aSpeed = lengthOf(aVelocity);
	double aDirectionX = aVelocity.x;
	double aDirectionY = aVelocity.y;
	if (aSpeed != 0.0f)
	{
		double anInverse = 1.0 / aSpeed;
		aDirectionX = aVelocity.x * anInverse;
		aDirectionY = aVelocity.y * anInverse;
	}
	double anAcceleration = (double)mAcceleration * anElapsed;
	double aVelocityX = ((double)aVelocity.x + aDirectionX * anAcceleration) + (double)mExternalForce.x * anElapsed;
	double aVelocityY = (aDirectionY * anAcceleration + aVelocity.y) + (double)mExternalForce.y * anElapsed;
	if (mBlackHole)
	{
		float aVelocityXFloat = (float)aVelocityX;
		float aVelocityYFloat = (float)aVelocityY;
		Vector2f aHole = mStream->mBlackHolePosition;
		double aHoleY = aHole.y;
		if (mOffsetType == PARTICLE_OFFSET_INDEPENDENT)
		{
			Vector2f aStreamPosition = mStream->getAbsolutePosition();
			aHole.x = (float)((double)aHole.x + aStreamPosition.x);
			aHoleY = (double)aStreamPosition.y + aHole.y;
		}
		aHole.x = (float)((double)aHole.x - mPosition.x);
		aHole.y = (float)(aHoleY - mPosition.y);
		normalize(aHole);
		double aPull = mStream->mBlackHoleAcceleration;
		aVelocityX = ((double)aHole.x * aPull) * anElapsed + aVelocityXFloat;
		aVelocityY = ((double)aHole.y * aPull) * anElapsed + aVelocityYFloat;
	}
	mVelocity.x = (float)aVelocityX;
	mVelocity.y = (float)aVelocityY;
	setPosition((float)(aVelocityX * anElapsed + x), (float)(aVelocityY * anElapsed + y));

	double aFraction = ((double)mLifeTime - mLife) / mLifeTime;
	float aScale = (float)(((double)mEndScale - mStartScale) * aFraction + mStartScale);
	float anOpacity = (float)(((double)mEndOpacity - mStartOpacity) * aFraction + mStartOpacity);
	if (mHasOpacityKeys)
		anOpacity = (float)(mOpacityKeys.getValue((float)aFraction) * anOpacity);
	// 0x428090 and 0x46d360 (feedingfrenzy2)
	aScale = maxOf(0.0f, aScale);
	setScale(aScale, aScale);
	setOpacity(minOf(1.0f, maxOf(0.0f, anOpacity)));
	Container::update(theContext);
}

ParticleStream::ParticleStream()
	: mEmitRate(0), mMaxParticles(0), mConstantScale(false), mConstantOpacity(false), mBlackHole(false),
	  mBlackHoleAcceleration(0), mBlackHolePosition(0, 0), mParticleBounds(0, 0, 0, 0), mExternalForce(0, 0),
	  mPivot(PIVOT_CENTER), mAbsolutePivot(false), mPivotPoint(0, 0), mFilterMode(0), mCarry(1.0f), mEmitting(false),
	  mOffsetType(PARTICLE_OFFSET_INDEPENDENT), mSequence(PARTICLE_SEQUENCE_RANDOM), mAutoStop(-1.0f),
	  mAutoStopCountdown(0), mStartDelay(-1.0f), mStartDelayCountdown(0), mOffsetParent(nullptr)
{
	addOwnState(STATE_ANIMATED);
}

void ParticleStream::fillPool(ParticleStreamDef* theDef)
{
	float aSpan = 1.5f;
	if (theDef->mAutoStop > 0.0f)
		aSpan = minOf(1.5f, theDef->mAutoStop);
	// _ftol2 truncates
	int aCount = (int)((double)theDef->mLifeTime.mHigh * theDef->mEmitRate * aSpan + 1.0);
	if (theDef->mMaxParticles > 0 && theDef->mMaxParticles < aCount)
		aCount = theDef->mMaxParticles;
	size_t anImage = 0;
	while ((int)mPool.size() < aCount)
	{
		Ref<Particle> aParticle = new Particle();
		if (theDef->mSequence == PARTICLE_SEQUENCE_ANIMATION)
		{
			// Each particle an animation of the path (AnimImage, its constructor 0x413400 (feedingfrenzy2)): not yet
			// translated, the particle kept without one so that the stream's draws stay the original's
			getEngine()->log("ParticleStream: animated particles of %s not yet translated\n", theDef->mPath.c_str());
		}
		else if (!theDef->mImages.empty())
		{
			aParticle->addChild(new Image(theDef->mImages[anImage]));
			anImage = (anImage + 1) % theDef->mImages.size();
		}
		// (with no image the original's checked vector fails)
		aParticle->setFilterMode(theDef->mFilterMode);
		mPool.push_back(aParticle);
	}
}

void ParticleStream::start()
{
	if (mAutoStop > 0.0f)
		mAutoStopCountdown = mAutoStop;
	if (mStartDelay > 0.0f)
		mStartDelayCountdown = mStartDelay;
	else
		mEmitting = true;
	if (!mParentName.empty())
		mOffsetParent = getEngine()->findNamedComponent(mParentName);
}

void ParticleStream::reset()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->clearParent();
	mChildren.clear();
	for (size_t i = 0; i < mPool.size(); i++)
		mPool[i]->setVisible(false);
	mCarry = 1.0f;
}

void ParticleStream::emit(Vector2f theBase)
{
	Particle* aParticle = nullptr;
	for (int aTries = 25; aTries > 0 && !mPool.empty() && aParticle == nullptr; aTries--)
	{
		int anIndex = crtRand() % (int)mPool.size();
		if (mPool[anIndex]->refCount() == 1)
			aParticle = mPool[anIndex];
	}
	if (aParticle == nullptr)
		return;

	aParticle->mStream = this;
	aParticle->mBlackHole = mBlackHole;
	aParticle->mAcceleration = (float)randomFloat(mAcceleration.mLow, mAcceleration.mHigh);
	float x = (float)randomFloat(mPositionX.mLow, mPositionX.mHigh);
	double y = randomFloat(mPositionY.mLow, mPositionY.mHigh);
	if (mOffsetType == PARTICLE_OFFSET_INDEPENDENT)
	{
		if (mOffsetParent == nullptr)
		{
			x = (float)((double)theBase.x + x);
			y = y + theBase.y;
		}
		else
		{
			float aSceneX = (float)((double)theBase.x + x);
			float aSceneY = (float)(y + theBase.y);
			Vector2f aParentPosition = mOffsetParent->getAbsolutePosition();
			x = (float)((double)aSceneX - aParentPosition.x);
			aParticle->mOffsetParent = mOffsetParent;
			y = (double)aSceneY - aParentPosition.y;
		}
	}
	aParticle->mOffsetType = mOffsetType;
	aParticle->setPosition(x, (float)y);
	aParticle->mStartScale = (float)randomFloat(mStartScale.mLow, mStartScale.mHigh);
	if (!mConstantScale)
		aParticle->mEndScale = (float)randomFloat(mEndScale.mLow, mEndScale.mHigh);
	else
		aParticle->mEndScale = aParticle->mStartScale;
	float aLife = (float)randomFloat(mLifeTime.mLow, mLifeTime.mHigh);
	aParticle->mLifeTime = aLife;
	aParticle->mLife = aLife;
	float aVelocityX = (float)randomFloat(mVelocityX.mLow, mVelocityX.mHigh);
	float aVelocityY = (float)randomFloat(mVelocityY.mLow, mVelocityY.mHigh);
	// The "degrees" go to cos and sin as they are (0x45e790 and 0x43cdb0 (feedingfrenzy2), each rounded to a float)
	float aRotation = (float)randomFloat(mRotation.mLow, mRotation.mHigh);
	float aCos = (float)cos((double)aRotation);
	float aSin = (float)sin((double)aRotation);
	aParticle->mVelocity.x = (float)((double)aVelocityX * aCos - (double)aVelocityY * aSin);
	aParticle->mVelocity.y = (float)((double)aVelocityX * aSin + (double)aVelocityY * aCos);
	aParticle->mStartOpacity = (float)randomFloat(mStartOpacity.mLow, mStartOpacity.mHigh);
	if (!mConstantOpacity)
		aParticle->mEndOpacity = (float)randomFloat(mEndOpacity.mLow, mEndOpacity.mHigh);
	else
		aParticle->mEndOpacity = aParticle->mStartOpacity;
	aParticle->mOpacityKeys.clear();
	for (size_t i = 0; i < mOpacityKeys.size(); i++)
		aParticle->mOpacityKeys.addKey(mOpacityKeys[i]);
	aParticle->mHasOpacityKeys = !mOpacityKeys.empty();
	float anAmplitudeX = (float)randomFloat(mJitterAmplitudeX.mLow, mJitterAmplitudeX.mHigh);
	float anAmplitudeY = (float)randomFloat(mJitterAmplitudeY.mLow, mJitterAmplitudeY.mHigh);
	if (anAmplitudeX != 0.0f || anAmplitudeY != 0.0f)
	{
		aParticle->removeAllModifiers();
		float aPeriodY = (float)randomFloat(mJitterPeriodY.mLow, mJitterPeriodY.mHigh);
		float aPeriodX = (float)randomFloat(mJitterPeriodX.mLow, mJitterPeriodX.mHigh);
		aParticle->addModifier(new WaveModifier(aPeriodX, aPeriodY, anAmplitudeX, anAmplitudeY, 0.0f));
	}
	aParticle->setVisible(true);
	aParticle->mExternalForce = mExternalForce;
	addChild(aParticle);
}

void ParticleStream::update(UpdateContext& theContext)
{
	float anElapsed = theContext.mElapsed;
	size_t i = 0;
	while (i < mChildren.size())
	{
		Particle* aParticle = static_cast<Particle*>(mChildren[i]);
		bool aGone = false;
		if (mParticleBounds.left < mParticleBounds.right && mParticleBounds.top < mParticleBounds.bottom)
		{
			const Vector2f& aStreamPosition = getPosition();
			const Vector2f& aPosition = aParticle->getPosition();
			// _ftol2 truncates the differences, kept at the x87's precision
			int aDeltaX = (int)((double)aPosition.x - aStreamPosition.x);
			int aDeltaY = (int)((double)aPosition.y - aStreamPosition.y);
			aGone = !mParticleBounds.contains(aDeltaX, aDeltaY);
		}
		if (!aGone)
			aGone = aParticle->mLife < 0.0f;
		if (aGone)
		{
			// Taken out without a word to the container (no childRemoved, no state)
			aParticle->clearParent();
			mChildren.erase(mChildren.begin() + i);
		}
		else
			i++;
	}

	if (mStartDelay > 0.0f && mStartDelayCountdown > 0.0f)
	{
		mStartDelayCountdown = mStartDelayCountdown - anElapsed;
		if (mStartDelayCountdown < 0.0f)
			mEmitting = true;
	}
	if (!mEmitting)
	{
		Container::update(theContext);
		return;
	}
	if (mAutoStop > 0.0f)
	{
		mAutoStopCountdown = mAutoStopCountdown - anElapsed;
		if (mAutoStopCountdown < 0.0f)
			mEmitting = false;
	}

	double aPending = (double)mEmitRate * anElapsed + mCarry;
	float aPendingFloat = (float)aPending;
	// A group-relative stream's emission ignores the base (the original passes what is on its stack)
	Vector2f aBase(0, 0);
	if (mOffsetType == PARTICLE_OFFSET_INDEPENDENT)
		aBase = Vector2f(mPosition.x + theContext.mPosition.x, mPosition.y + theContext.mPosition.y);
	if (aPending >= 1.0)
	{
		for (;;)
		{
			if (mMaxParticles != 0 && (int)mChildren.size() >= mMaxParticles)
				break;
			emit(aBase);
			double aNext = (double)aPendingFloat - 1.0;
			aPendingFloat = (float)aNext;
			aPending = aNext;
			if (aNext < 1.0)
				break;
		}
	}
	mCarry = (float)aPending;
	Container::update(theContext);
}

void ParticleStream::computeBounds()
{
	mParticleBounds = RectI(0, 0, 0, 0);
	removeState(STATE_BOUNDS_DIRTY);
}

ParticleSystem::ParticleSystem()
	: mStarted(false), mDeleteWhenDone(false), mAutoStop(-1.0f), mAutoStopCountdown(0), mWarmup(0)
{
}

void ParticleSystem::update(UpdateContext& theContext)
{
	if (mStarted && mAutoStop > 0.0f)
	{
		mAutoStopCountdown = mAutoStopCountdown - theContext.mElapsed;
		if (mAutoStopCountdown < 0.0f)
			stop();
	}
	Container::update(theContext);
	if (mDeleteWhenDone && isDone())
		addOwnState(STATE_DELETE);
}

void ParticleSystem::computeBounds()
{
	mBounds = RectF(0, 0, 0, 0);
	removeState(STATE_BOUNDS_DIRTY);
}

bool ParticleSystem::load(const std::string& thePath)
{
	Ref<ParticleSystemDocHandler> aHandler = new ParticleSystemDocHandler();
	setName(thePath);
	getEngine()->parseXml(thePath, aHandler);
	ParticleSystemDef* aDef = aHandler->getSystem();
	// (a file without a <particleSystem> leaves the original with none to read)
	if (aDef == nullptr)
		return false;
	// 0x4761e0 (feedingfrenzy2)
	for (size_t i = 0; i < aDef->mStreams.size(); i++)
		aDef->mStreams[i]->loadImages();
	return init(aDef);
}

bool ParticleSystem::init(ParticleSystemDef* theDef)
{
	removeAllChildren();
	mAutoStop = theDef->mAutoStop;
	mWarmup = theDef->mWarmup;
	for (size_t i = 0; i < theDef->mStreams.size(); i++)
	{
		if (!addStream(theDef->mStreams[i]))
			return false;
	}
	return true;
}

bool ParticleSystem::addStream(ParticleStreamDef* theDef)
{
	ParticleStream* aStream = new ParticleStream();
	addChild(aStream);
	aStream->mParentName = theDef->mParentName;
	aStream->mEmitRate = theDef->mEmitRate;
	aStream->mMaxParticles = theDef->mMaxParticles;
	aStream->mVelocityX = theDef->mVelocityX;
	aStream->mVelocityY = theDef->mVelocityY;
	aStream->mAcceleration = theDef->mAcceleration;
	aStream->mPositionX = theDef->mPositionX;
	aStream->mPositionY = theDef->mPositionY;
	aStream->mStartScale = theDef->mStartScale;
	aStream->mEndScale = theDef->mEndScale;
	aStream->mRotation = theDef->mRotation;
	aStream->mJitterPeriodX = theDef->mJitterPeriodX;
	aStream->mJitterAmplitudeX = theDef->mJitterAmplitudeX;
	aStream->mJitterPeriodY = theDef->mJitterPeriodY;
	aStream->mJitterAmplitudeY = theDef->mJitterAmplitudeY;
	aStream->mConstantScale = theDef->mConstantScale;
	aStream->mConstantOpacity = theDef->mConstantOpacity;
	aStream->mBlackHole = theDef->mBlackHole;
	aStream->mBlackHoleAcceleration = theDef->mBlackHoleAcceleration;
	aStream->mBlackHolePosition = theDef->mBlackHolePosition;
	aStream->mLifeTime = theDef->mLifeTime;
	aStream->mParticleBounds = theDef->mBounds;
	aStream->mStartOpacity = theDef->mStartOpacity;
	aStream->mEndOpacity = theDef->mEndOpacity;
	aStream->mExternalForce = theDef->mExternalForce;
	aStream->mFilterMode = theDef->mFilterMode;
	aStream->mOffsetType = theDef->mOffsetType;
	aStream->mSequence = theDef->mSequence;
	aStream->mAutoStop = theDef->mAutoStop;
	aStream->mStartDelay = theDef->mStartDelay;
	aStream->mPath = theDef->mPath;
	aStream->mPivot = theDef->mPivot;
	aStream->mAbsolutePivot = theDef->mAbsolutePivot;
	aStream->mPivotPoint = theDef->mPivotPoint;
	for (size_t i = 0; i < theDef->mOpacityKeys.size(); i++)
		aStream->mOpacityKeys.push_back(theDef->mOpacityKeys[i]);
	aStream->fillPool(theDef);
	return true;
}

void ParticleSystem::start()
{
	// 0x42a650 (feedingfrenzy2)
	if (!mStarted)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
			static_cast<ParticleStream*>(mChildren[i])->reset();
	}
	for (size_t i = 0; i < mChildren.size(); i++)
		static_cast<ParticleStream*>(mChildren[i])->start();
	if (mAutoStop > 0.0f)
		mAutoStopCountdown = mAutoStop;
	if (mWarmup > 0.0f)
		warmup(mWarmup);
	mStarted = true;
}

void ParticleSystem::stop()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		static_cast<ParticleStream*>(mChildren[i])->mEmitting = false;
	mStarted = false;
}

bool ParticleSystem::isDone()
{
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		if (!static_cast<ParticleStream*>(mChildren[i])->isDone())
			return false;
	}
	return true;
}

void ParticleSystem::warmup(float theSeconds)
{
	// 0x435b30 (feedingfrenzy2) zeroes the position; the frame and detail level are left as the stack had them
	UpdateContext aContext;
	aContext.mElapsed = theSeconds;
	Container::update(aContext);
}

void ParticleSystem::setParticleBounds(const RectI& theBounds)
{
	for (size_t i = 0; i < mChildren.size(); i++)
		static_cast<ParticleStream*>(mChildren[i])->mParticleBounds = theBounds;
}

} // namespace engine
