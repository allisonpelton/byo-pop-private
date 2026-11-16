#include "SignalParticles.h"
#include "AnimImage.h"
#include "App.h"
#include "Graphics.h"
#include "ImageNode.h"
#include "Layout.h"
#include "Modifiers.h"
#include "Random.h"
#include "Stream.h"
#include <cstdio>

using namespace engine;

namespace
{
const float EMIT_RATE_DEFAULT = 5.0f;           // particles a second
const float ANIMATION_RATE_DEFAULT = 15.0f;     // pictures a second of an animation
const float NO_TIME = -1.0f;                    // autoStop and startDelay when a file says none
const int POOL_PICK_TRIES = 25;                 // random picks of the pool for a free particle
const double POOL_SIZE_FACTOR = 1.5;            // the pool holds this many of the longest life's emissions
}

// 0x47de40 (pizzafrenzy)
ParticleStreamDef::ParticleStreamDef() :
	mOffset(0, 0),
	mEmitRate(0),
	mMaxParticles(0),
	mSequence(PARTICLE_SEQUENCE_RANDOM),
	mAnimationRate(0),
	mVelocityX(0, 0),
	mVelocityY(0, 0),
	mAcceleration(0, 0),
	mPositionX(0, 0),
	mPositionY(0, 0),
	mStartScale(1.0f, 1.0f),
	mEndScale(1.0f, 1.0f),
	mRotation(0, 0),
	mJitterPeriodX(1.0f, 1.0f),
	mJitterSizeX(0, 0),
	mJitterPeriodY(1.0f, 1.0f),
	mJitterSizeY(0, 0),
	mConstantScale(false),
	mConstantOpacity(true),
	mBlackHole(false),
	mBlackHoleAcceleration(0),
	mBlackHolePosition(0, 0),
	mLifeTime(1.0f, 1.0f),
	mStartOpacity(1.0f, 1.0f),
	mEndOpacity(1.0f, 1.0f),
	mForce(0, 0),
	mSmoothType(0),
	mGroupRelative(0),
	mAutoStop(NO_TIME),
	mStartDelay(0)
{
}

// 0x47e2a0 (pizzafrenzy): each round looks for the picture of the next number (a .png, or else a .jpg)
bool ParticleStreamDef::loadImages()
{
	mImages.clear();
	App* anApp = App::instance();
	char aName[512];
	int aNumber = 0;
	bool aFound;
	do
	{
		aFound = false;
		snprintf(aName, sizeof(aName), "%s%d%s", mImagePath.c_str(), aNumber, ".png");
		if (!anApp->exists(aName))
			snprintf(aName, sizeof(aName), "%s%d%s", mImagePath.c_str(), aNumber, ".jpg");
		if (anApp->exists(aName))
		{
			Image* anImage = anApp->getImage(aName);
			anImage->setPivot(1);
			mImages.push_back(Ref<Image>(anImage));
			aFound = true;
		}
		aNumber++;
	} while (aFound);
	return 0 < aNumber;
}

// 0x47ddb0 (pizzafrenzy)
ParticleSystemDef::ParticleSystemDef() : mAutoStop(NO_TIME), mWarmup(0)
{
}

// 0x47e480 (pizzafrenzy)
bool ParticleSystemDef::loadImages()
{
	for (size_t i = 0; i < mStreams.size(); i++)
	{
		if (!mStreams[i]->loadImages())
			return false;
	}
	return true;
}

// 0x47c9b0 (pizzafrenzy)
void ParticleSystemDocHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("particleSystem") == 0)
		startSystem(theAttributes);
	else if (theName.compare("particleStream") == 0)
		startStream(theAttributes);
	else if (theName.compare("velocity") == 0 && mStreamDef != nullptr)
	{
		mStreamDef->mVelocityX = getScaleAttribute(theAttributes, "startX", Point(0, 10.0f));
		mStreamDef->mVelocityY = getScaleAttribute(theAttributes, "startY", Point(0, 10.0f));
	}
	else if (theName.compare("acceleration") == 0 && mStreamDef != nullptr)
		mStreamDef->mAcceleration = getScaleAttribute(theAttributes, "rate", Point(0, 0));
	else if (theName.compare("extForce") == 0 && mStreamDef != nullptr)
		mStreamDef->mForce = getPointAttribute(theAttributes, "force", Point(0, 0));
	else if (theName.compare("position") == 0 && mStreamDef != nullptr)
	{
		mStreamDef->mPositionX = getScaleAttribute(theAttributes, "startX", Point(0, 0));
		mStreamDef->mPositionY = getScaleAttribute(theAttributes, "startY", Point(0, 0));
	}
	else if (theName.compare("rotation") == 0 && mStreamDef != nullptr)
		mStreamDef->mRotation = getScaleAttribute(theAttributes, "degrees", Point(0, 0));
	else if (theName.compare("scale") == 0 && mStreamDef != nullptr)
	{
		mStreamDef->mStartScale = getScaleAttribute(theAttributes, "startScale", Point(1.0f, 1.0f));
		mStreamDef->mEndScale = getScaleAttribute(theAttributes, "endScale", Point(1.0f, 1.0f));
		std::string aConstant = getStringAttribute(theAttributes, "constant", "false");
		mStreamDef->mConstantScale = aConstant.compare("true") == 0;
	}
	else if (theName.compare("particleLife") == 0 && mStreamDef != nullptr)
		mStreamDef->mLifeTime = getScaleAttribute(theAttributes, "lifeTime", Point(1.0f, 2.0f));
	else if (theName.compare("opacity") == 0 && mStreamDef != nullptr)
	{
		mStreamDef->mStartOpacity = getScaleAttribute(theAttributes, "startOp", Point(1.0f, 1.0f));
		mStreamDef->mEndOpacity = getScaleAttribute(theAttributes, "endOp", Point(1.0f, 1.0f));
		std::string aConstant = getStringAttribute(theAttributes, "constant", "false");
		mStreamDef->mConstantOpacity = aConstant.compare("true") == 0;
	}
	else if (theName.compare("resource") == 0 && mStreamDef != nullptr)
	{
		mStreamDef->mImagePath = getStringAttribute(theAttributes, "path", "INVALID_RES_PATH");
		std::string aFilter = getStringAttribute(theAttributes, "filter", "");
		// unlike Word Harmony's, each of the three gives the same value
		if (aFilter.compare("X") == 0 || aFilter.compare("Y") == 0 || aFilter.compare("XY") == 0)
			mStreamDef->mSmoothType = 1;
		else
			mStreamDef->mSmoothType = 0;
	}
	else if (theName.compare("jitter") == 0 && mStreamDef != nullptr)
	{
		std::string anAxis = getStringAttribute(theAttributes, "axis", "");
		if (anAxis.compare("X") == 0)
		{
			mStreamDef->mJitterPeriodX = getScaleAttribute(theAttributes, "period", Point(0, 0));
			mStreamDef->mJitterSizeX = getScaleAttribute(theAttributes, "amplitude", Point(0, 0));
		}
		else if (anAxis.compare("Y") == 0)
		{
			mStreamDef->mJitterPeriodY = getScaleAttribute(theAttributes, "period", Point(0, 0));
			mStreamDef->mJitterSizeY = getScaleAttribute(theAttributes, "amplitude", Point(0, 0));
		}
	}
	else if (theName.compare("blackHole") == 0 && mStreamDef != nullptr)
	{
		mStreamDef->mBlackHole = true;
		mStreamDef->mBlackHolePosition = getPointAttribute(theAttributes, "pos", Point(0, 0));
		mStreamDef->mBlackHoleAcceleration = getFloatAttribute(theAttributes, "accel", 0.0f);
	}
}

// 0x47be70 (pizzafrenzy)
void ParticleSystemDocHandler::endElement(const std::string& theName)
{
	if (theName.compare("particleStream") == 0)
		mStreamDef = nullptr;
}

// 0x47c820 (pizzafrenzy)
void ParticleSystemDocHandler::startSystem(const XmlAttributes& theAttributes)
{
	mSystemDef = new ParticleSystemDef();
	float anAutoStop = getFloatAttribute(theAttributes, "autoStop", NO_TIME);
	float aWarmup = getFloatAttribute(theAttributes, "warmup", 0.0f);
	mSystemDef->mAutoStop = anAutoStop;
	mSystemDef->mWarmup = aWarmup;
}

// 0x47c130 (pizzafrenzy)
void ParticleSystemDocHandler::startStream(const XmlAttributes& theAttributes)
{
	mStreamDef = new ParticleStreamDef();
	if (mSystemDef != nullptr)
		mSystemDef->mStreams.push_back(mStreamDef);
	mStreamDef->mEmitRate = getFloatAttribute(theAttributes, "emitRate", EMIT_RATE_DEFAULT);
	mStreamDef->mMaxParticles = getIntAttribute(theAttributes, "maxParticles", 0);
	std::string aSequence = getStringAttribute(theAttributes, "sequence", "random");
	if (aSequence.compare("anim") == 0)
		mStreamDef->mSequence = PARTICLE_SEQUENCE_ANIMATION;
	else if (aSequence.compare("sequential") == 0)
		mStreamDef->mSequence = PARTICLE_SEQUENCE_SEQUENTIAL;
	else
		mStreamDef->mSequence = PARTICLE_SEQUENCE_RANDOM;
	mStreamDef->mAnimationRate = getFloatAttribute(theAttributes, "animRate", ANIMATION_RATE_DEFAULT);
	std::string anOffsetType = getStringAttribute(theAttributes, "offsetType", "independent");
	mStreamDef->mGroupRelative = anOffsetType.compare("groupRelative") == 0 ? 1 : 0;
	std::string aParent = getStringAttribute(theAttributes, "parent", "");
	if (aParent.compare("") != 0)
		mStreamDef->mParent = aParent;
	mStreamDef->mAutoStop = getFloatAttribute(theAttributes, "autoStop", NO_TIME);
	mStreamDef->mStartDelay = getFloatAttribute(theAttributes, "startDelay", NO_TIME);
	mStreamDef->mOffset = getPointAttribute(theAttributes, "offset", Point(0, 0));
}

// 0x41de80 (pizzafrenzy)
Ref<ParticleSystemDef> engine::loadParticleSystemDef(const std::string& theFile)
{
	ParticleSystemDocHandler aHandler;
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
	Ref<ParticleSystemDef> aDef = aHandler.getSystemDef();
	if (aDef != nullptr)
		aDef->loadImages();
	return aDef;
}

// 0x498140 (pizzafrenzy)
SignalParticle::SignalParticle() :
	mStream(nullptr),
	mVelocity(0, 0),
	mAcceleration(0),
	mStartScale(0),
	mEndScale(0),
	mStartOpacity(0),
	mEndOpacity(0),
	mForce(0, 0),
	mBlackHole(false),
	mLife(0),
	mLifeLeft(0),
	mGroupRelative(0)
{
	addOwnFlags(NODE_LISTENERS);
}

// 0x497e40 (pizzafrenzy): the node's own transform, and then the translation is set so that the particle's position is
// its place on the screen, whatever its parents' places are
void SignalParticle::applyTransform(Graphics* g)
{
	Node::applyTransform(g);
	if (mGroupRelative == 0)
	{
		Point aTranslation(g->state().mTranslation);
		g->translate(mPosition.x - aTranslation.x, mPosition.y - aTranslation.y);
	}
}

// 0x497f00 (pizzafrenzy): the velocity grows by the acceleration along it, the force and the black hole's pull; the
// scale and the opacity go from their start to their end over the particle's life (the scale not below 0, the opacity
// from 0 to 1)
void SignalParticle::update(UpdateInfo& theInfo)
{
	float aSeconds = theInfo.mSeconds;
	mLifeLeft = mLifeLeft - aSeconds;
	Point aVelocity = mVelocity;
	Point aPosition(getX(), getY());
	Point aStep = aVelocity;
	aStep.normalize();
	aStep.scale(aSeconds * mAcceleration);
	aVelocity.add(aStep);
	aStep = mForce;
	aStep.scale(aSeconds);
	aVelocity.add(aStep);
	if (mBlackHole)
	{
		// the black hole is placed from the stream's position on the screen, unless the stream is group-relative
		Point aPull = mStream->mBlackHolePosition;
		if (mGroupRelative == 0)
			aPull.add(*mStream->getAbsPosition());
		aPull.subtract(mPosition);
		aPull.normalize();
		aPull.scale(mStream->mBlackHoleAcceleration);
		aPull.scale(aSeconds);
		aVelocity.add(aPull);
	}
	mVelocity = aVelocity;
	aVelocity.scale(aSeconds);
	aPosition.add(aVelocity);
	setPosition(aPosition.x, aPosition.y);
	float aGone = (mLife - mLifeLeft) / mLife;
	float aScale = (mEndScale - mStartScale) * aGone + mStartScale;
	float anOpacity = (mEndOpacity - mStartOpacity) * aGone + mStartOpacity;
	if (!(0.0f < anOpacity))
		anOpacity = 0.0f;
	else if (1.0f <= anOpacity)
		anOpacity = 1.0f;
	if (!(0.0f < aScale))
		aScale = 0.0f;
	setScale(aScale, aScale);
	setOpacity(anOpacity);
	Container::update(theInfo);
}

// 0x4942e0 (pizzafrenzy)
SignalParticleStream::SignalParticleStream() :
	mEmitRate(0),
	mMaxParticles(0),
	mConstantScale(false),
	mConstantOpacity(false),
	mBlackHole(false),
	mBlackHoleAcceleration(0),
	mSmoothType(0),
	mEmitCredit(1.0f),
	mEmitting(false),
	mGroupRelative(0),
	mSequence(0),
	mAutoStop(NO_TIME),
	mAutoStopLeft(0),
	mStartDelay(NO_TIME),
	mStartDelayLeft(0)
{
	addOwnFlags(NODE_LISTENERS);
}

// 0x473aa0 (pizzafrenzy)
void SignalParticleStream::layout()
{
	mBounds = Rect();
	clearFlags(NODE_LAYOUT);
}

// 0x4944b0 (pizzafrenzy): the stream is made from its definition (0x4790a0 (pizzafrenzy)); then enough particles for
// the longest life at the emit rate, and half as many again, each with the next picture (or an animation)
void SignalParticleStream::setUp(ParticleStreamDef* theDef)
{
	mEmitRate = theDef->mEmitRate;
	mMaxParticles = (int)theDef->mMaxParticles;
	mVelocityX = theDef->mVelocityX;
	mVelocityY = theDef->mVelocityY;
	mAcceleration = theDef->mAcceleration;
	mPositionX = theDef->mPositionX;
	mPositionY = theDef->mPositionY;
	mStartScale = theDef->mStartScale;
	mEndScale = theDef->mEndScale;
	mRotation = theDef->mRotation;
	mJitterPeriodX = theDef->mJitterPeriodX;
	mJitterSizeX = theDef->mJitterSizeX;
	mJitterPeriodY = theDef->mJitterPeriodY;
	mJitterSizeY = theDef->mJitterSizeY;
	mConstantScale = theDef->mConstantScale;
	mConstantOpacity = theDef->mConstantOpacity;
	mBlackHole = theDef->mBlackHole;
	mBlackHoleAcceleration = theDef->mBlackHoleAcceleration;
	mBlackHolePosition = theDef->mBlackHolePosition;
	mLifeTime = theDef->mLifeTime;
	mStartOpacity = theDef->mStartOpacity;
	mEndOpacity = theDef->mEndOpacity;
	mForce = theDef->mForce;
	mSmoothType = theDef->mSmoothType;
	mGroupRelative = theDef->mGroupRelative;
	mSequence = theDef->mSequence;
	mAutoStop = theDef->mAutoStop;
	mStartDelay = theDef->mStartDelay;
	mImagePath = theDef->mImagePath;

	unsigned int aCount = (unsigned int)(int)((double)mLifeTime.y * (double)mEmitRate * POOL_SIZE_FACTOR + 1.0);
	size_t anImageCount = theDef->mImages.size();
	size_t anImageIndex = 0;
	while (mPool.size() < aCount)
	{
		SignalParticle* aParticle = new SignalParticle();
		if (theDef->mSequence == PARTICLE_SEQUENCE_ANIMATION)
		{
			AnimImage* anAnimation = new AnimImage();
			anAnimation->load(theDef->mImagePath.c_str(), nullptr, 0);
			anAnimation->setLoop(true);
			anAnimation->setRate(theDef->mAnimationRate);
			anAnimation->play();
			aParticle->addChild(anAnimation);
		}
		else
		{
			// the original reads a picture of an empty list (a checked access that throws); the port makes no pool
			if (anImageCount == 0)
			{
				delete aParticle;
				break;
			}
			aParticle->addChild(new ImageNode(theDef->mImages[anImageIndex]));
		}
		aParticle->setSmoothType(theDef->mSmoothType);
		mPool.push_back(Ref<SignalParticle>(aParticle));
		if (anImageCount != 0)
			anImageIndex = (anImageIndex + 1) % anImageCount;
	}
}

// 0x493770 (pizzafrenzy)
void SignalParticleStream::start()
{
	if (0.0f < mAutoStop)
		mAutoStopLeft = mAutoStop;
	if (0.0f < mStartDelay)
	{
		mStartDelayLeft = mStartDelay;
		return;
	}
	mEmitting = true;
}

// 0x4937d0 (pizzafrenzy)
bool SignalParticleStream::isDone()
{
	return !mEmitting && mStartDelayLeft <= 0.0f && mChildren.empty();
}

// 0x493810 (pizzafrenzy)
void SignalParticleStream::clearParticles()
{
	while (!mChildren.empty())
	{
		Node* aParticle = mChildren.front();
		aParticle->clearParent(this);
		mChildren.erase(mChildren.begin());
	}
	for (size_t i = 0; i < mPool.size(); i++)
		mPool[i]->setVisible(false);
}

// 0x494100 (pizzafrenzy): the particles whose life ran out leave; after the start delay, and until the automatic stop,
// the stream emits at its rate (at most its maximum in the air)
void SignalParticleStream::update(UpdateInfo& theInfo)
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	while (anIt != mChildren.end())
	{
		SignalParticle* aParticle = static_cast<SignalParticle*>(*anIt);
		if (0.0f <= aParticle->mLifeLeft)
			++anIt;
		else
		{
			aParticle->clearParent(this);
			anIt = mChildren.erase(anIt);
		}
	}
	if (0.0f < mStartDelay && 0.0f < mStartDelayLeft)
	{
		mStartDelayLeft = mStartDelayLeft - theInfo.mSeconds;
		if (mStartDelayLeft < 0.0f)
			mEmitting = true;
	}
	if (mEmitting)
	{
		// a stop here still lets this frame's particles out
		if (0.0f < mAutoStop)
		{
			mAutoStopLeft = mAutoStopLeft - theInfo.mSeconds;
			if (mAutoStopLeft < 0.0f)
				mEmitting = false;
		}
		// the x87 sum in double, stored as a float
		float aCredit = (float)((double)mEmitRate * theInfo.mSeconds + mEmitCredit);
		// left unset by the original for a group-relative stream, whose emit does not read it
		float x = 0;
		float y = 0;
		if (mGroupRelative == 0)
		{
			x = theInfo.mOrigin.x + mPosition.x;
			y = theInfo.mOrigin.y + mPosition.y;
		}
		while (1.0f <= aCredit)
		{
			if (mMaxParticles != 0 && (int)mChildren.size() >= mMaxParticles)
				break;
			emit(x, y);
			aCredit = aCredit - 1.0f;
		}
		mEmitCredit = aCredit;
	}
	Container::update(theInfo);
}

// 0x493d90 (pizzafrenzy): up to 25 random picks from the pool for a free particle, which then draws its values from
// the stream's ranges in the original's order
void SignalParticleStream::emit(float x, float y)
{
	SignalParticle* aParticle = nullptr;
	int aTries = POOL_PICK_TRIES;
	while (aTries >= 1 && !mPool.empty())
	{
		int anIndex = randomRange(0, (int)mPool.size() - 1);
		SignalParticle* aCandidate = mPool.at(anIndex);
		if (aCandidate->refCount() == 1)
			aParticle = aCandidate;
		aTries--;
		if (aParticle != nullptr)
			break;
	}
	if (aParticle == nullptr)
		return;
	aParticle->mStream = this;
	aParticle->mBlackHole = mBlackHole;
	aParticle->mAcceleration = randomFloat(mAcceleration);
	float aStartX = randomFloat(mPositionX);
	float aStartY = randomFloat(mPositionY);
	if (mGroupRelative == 0)
	{
		aStartX = x + aStartX;
		aStartY = y + aStartY;
	}
	aParticle->mGroupRelative = mGroupRelative;
	aParticle->setPosition(aStartX, aStartY);
	float aScale = randomFloat(mStartScale);
	aParticle->mStartScale = aScale;
	if (!mConstantScale)
		aScale = randomFloat(mEndScale);
	aParticle->mEndScale = aScale;
	float aLife = randomFloat(mLifeTime);
	aParticle->mLife = aLife;
	aParticle->mLifeLeft = aLife;
	float aVelocityX = randomFloat(mVelocityX);
	float aVelocityY = randomFloat(mVelocityY);
	aParticle->mVelocity = Point(aVelocityX, aVelocityY);
	aParticle->mVelocity.rotate(randomFloat(mRotation));
	float anOpacity = randomFloat(mStartOpacity);
	aParticle->mStartOpacity = anOpacity;
	if (!mConstantOpacity)
		anOpacity = randomFloat(mEndOpacity);
	aParticle->mEndOpacity = anOpacity;
	float aSizeX = randomFloat(mJitterSizeX);
	float aSizeY = randomFloat(mJitterSizeY);
	// a particle without jitter keeps a jitter it was given before
	if (aSizeX != 0.0f || aSizeY != 0.0f)
	{
		aParticle->removeAllListeners();
		// the original draws the constructor's arguments right to left
		float aPeriodY = randomFloat(mJitterPeriodY);
		float aPeriodX = randomFloat(mJitterPeriodX);
		aParticle->addListener(new WaveModifier(aPeriodX, aPeriodY, aSizeX, aSizeY, 0.0f));
	}
	aParticle->setVisible(true);
	aParticle->mForce = mForce;
	addChild(aParticle);
}

// 0x479470 (pizzafrenzy)
SignalParticleSystem::SignalParticleSystem() :
	mEmitting(false),
	mRemoveWhenDone(false),
	mAutoStop(NO_TIME),
	mAutoStopLeft(0),
	mWarmup(0)
{
}

// 0x473aa0 (pizzafrenzy)
void SignalParticleSystem::layout()
{
	mBounds = Rect();
	clearFlags(NODE_LAYOUT);
}

// 0x479330 (pizzafrenzy)
void SignalParticleSystem::setUp(ParticleSystemDef* theDef)
{
	removeAllChildren();
	mAutoStop = theDef->mAutoStop;
	mWarmup = theDef->mWarmup;
	for (size_t i = 0; i < theDef->mStreams.size(); i++)
		addStream(theDef->mStreams[i]);
}

// 0x4790a0 (pizzafrenzy)
void SignalParticleSystem::addStream(ParticleStreamDef* theDef)
{
	SignalParticleStream* aStream = new SignalParticleStream();
	addChild(aStream);
	aStream->setUp(theDef);
}

// 0x478fe0 (pizzafrenzy)
void SignalParticleSystem::start()
{
	if (!mEmitting)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
			static_cast<SignalParticleStream*>(mChildren[i])->clearParticles();
	}
	for (size_t i = 0; i < mChildren.size(); i++)
		static_cast<SignalParticleStream*>(mChildren[i])->start();
	if (0.0f < mAutoStop)
		mAutoStopLeft = mAutoStop;
	if (0.0f < mWarmup)
	{
		UpdateInfo anInfo;
		anInfo.mOrigin = Point(0, 0);
		anInfo.mSeconds = mWarmup;
		Container::update(anInfo);
	}
	mEmitting = true;
}

// 0x478ef0 (pizzafrenzy)
void SignalParticleSystem::stop()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		static_cast<SignalParticleStream*>(mChildren[i])->stop();
	mEmitting = false;
}

// 0x478f20 (pizzafrenzy)
bool SignalParticleSystem::isDone()
{
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		if (!static_cast<SignalParticleStream*>(mChildren[i])->isDone())
			return false;
	}
	return true;
}

// 0x478f60 (pizzafrenzy): the system stops after its automatic stop's seconds, and asks to be removed when it is done
void SignalParticleSystem::update(UpdateInfo& theInfo)
{
	if (mEmitting && 0.0f < mAutoStop)
	{
		mAutoStopLeft = mAutoStopLeft - theInfo.mSeconds;
		if (mAutoStopLeft < 0.0f)
			stop();
	}
	Container::update(theInfo);
	if (mRemoveWhenDone && isDone())
		addOwnFlags(NODE_REMOVE);
}
