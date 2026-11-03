#include "Particles.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include "ImageNode.h"
#include "Layout.h"
#include "Random.h"
#include "Stream.h"
#include <cmath>
#include <cstdio>

using namespace engine;

// 0x435760
Particle::Particle()
{
	addOwnFlags(NODE_LISTENERS);
	mStream = nullptr;
	mAcceleration = 0;
	mStartScale = 0;
	mEndScale = 0;
	mStartOpacity = 0;
	mEndOpacity = 0;
	mBlackHole = false;
	mLife = 0;
	mLifeLeft = 0;
	mAnchor = nullptr;
	mGroupRelative = 0;
}

// 0x435690: the particle's position is on the screen, so the translation its parents made is taken back off (the
// anchor's position is added); no rotation and no colour offset. A group-relative particle is placed like any node.
void Particle::applyTransform(Graphics* g)
{
	if (mGroupRelative != 0)
	{
		Node::applyTransform(g);
		return;
	}
	Point aTranslation(g->state().mTranslation);
	aTranslation.x = mPosition.x - aTranslation.x;
	aTranslation.y = mPosition.y - aTranslation.y;
	if (mAnchor != nullptr)
	{
		aTranslation.x = mAnchor->getX() + aTranslation.x;
		aTranslation.y = mAnchor->getY() + aTranslation.y;
	}
	g->setSnap(mStream->mFilter);
	g->translate(aTranslation.x, aTranslation.y);
	g->scale(mScale.x, mScale.y);
	g->multiplyOpacity(mOpacity);
	if (mFont != nullptr)
		g->setFont(mFont);
}

// 0x4353b0: the velocity grows by the acceleration along it, the force and the black hole's pull; the scale and the
// opacity go from their start to their end over the particle's life (neither below 0)
void Particle::update(UpdateInfo& theInfo)
{
	float aSeconds = theInfo.mSeconds;
	Point aVelocity = mVelocity;
	mLifeLeft = mLifeLeft - aSeconds;
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
		{
			Point aStreamPosition(*mStream->getAbsPosition());
			aPull.add(aStreamPosition);
		}
		aPull.subtract(Point(mPosition));
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
	if (anOpacity <= 0)
		anOpacity = 0;
	if (aScale <= 0)
		aScale = 0;
	setScale(aScale, aScale);
	setOpacity(anOpacity);
	Container::update(theInfo);
}

// 0x431930
JitterListener::JitterListener(float theRateX, float theRateY, float theAmountX, float theAmountY)
{
	mRateX = theRateX;
	mRateY = theRateY;
	mAmountX = theAmountX;
	mAmountY = theAmountY;
	mTime = 0;
}

// 0x431a90
void JitterListener::setX(float theRate, float theAmount)
{
	mRateX = theRate;
	mAmountX = theAmount;
}

// 0x431ab0
void JitterListener::setY(float theRate, float theAmount)
{
	mRateY = theRate;
	mAmountY = theAmount;
}

// 0x431a00
void JitterListener::update(UpdateInfo& theInfo, Node* theNode)
{
	mTime = theInfo.mSeconds + mTime;
	Point aPosition(*theNode->getPosition());
	if (mAmountX != 0)
		aPosition.x = (float)(cos((double)mRateX * mTime) * mAmountX + aPosition.x);
	if (mAmountY != 0)
		aPosition.y = (float)(cos((double)mRateY * mTime) * mAmountY + aPosition.y);
	theNode->setPosition(aPosition);
}

// 0x433390
ParticleStream::ParticleStream()
{
	mEmitRate = 0;
	mMaxParticles = 0;
	mConstantScale = false;
	mConstantOpacity = false;
	mBlackHoleAcceleration = 0;
	mFilter = 0;
	mEmitCredit = 1.0f;
	addOwnFlags(NODE_LISTENERS);
	mEmitting = false;
	mGroupRelative = 0;
	mAutoStop = -1.0f;
	mAutoStopLeft = 0;
	mStartDelay = -1.0f;
	mStartDelayLeft = 0;
	mAnchor = nullptr;
	mBlackHolePosition = Point();
	mBlackHole = false;
}

// 0x432940
ParticleStream::~ParticleStream()
{
	std::vector<Particle*>::iterator anIt = mParticles.begin();
	if (anIt != mParticles.end())
	{
		do
		{
			(*anIt)->release();
			++anIt;
		} while (anIt != mParticles.end());
	}
}

// 0x432880
void ParticleStream::startEmitting()
{
	if (mAutoStop > 0)
		mAutoStopLeft = mAutoStop;
	if (mStartDelay > 0)
	{
		mStartDelayLeft = mStartDelay;
		return;
	}
	mEmitting = true;
}

// 0x432900
bool ParticleStream::isDone()
{
	if (mEmitting)
		return false;
	if (mStartDelayLeft > 0)
		return false;
	return mChildren.size() == 0;
}

// 0x4328e0
void ParticleStream::layout()
{
	mBounds = Rect();
	clearFlags(NODE_LAYOUT);
}

// 0x432ad0
void ParticleStream::setAnchor(std::string theName)
{
	mAnchor = App::instance()->findNode(theName);
}

// 0x4331b0: the particles whose life ran out leave; after the start delay, and until the automatic stop, the stream
// emits at its rate (at most its maximum at a time)
void ParticleStream::update(UpdateInfo& theInfo)
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			Particle* aParticle = static_cast<Particle*>(*anIt);
			if (aParticle->mLifeLeft < 0)
			{
				aParticle->clearParent(this);
				anIt = mChildren.erase(anIt);
			}
			else
				++anIt;
		} while (anIt != mChildren.end());
	}
	if (mStartDelay > 0 && mStartDelayLeft > 0)
	{
		mStartDelayLeft = mStartDelayLeft - theInfo.mSeconds;
		if (mStartDelayLeft < 0)
			mEmitting = true;
	}
	if (mEmitting)
	{
		// a stop here still lets this frame's particles out
		if (mAutoStop > 0)
		{
			mAutoStopLeft = mAutoStopLeft - theInfo.mSeconds;
			if (mAutoStopLeft < 0)
				mEmitting = false;
		}
		// the x87 sum in double, stored as a float (a float product first emits a frame late now and then)
		float aCredit = (float)((double)mEmitRate * theInfo.mSeconds + mEmitCredit);
		// left unset by the original for a group-relative stream, whose emit does not read it
		Point aPosition;
		if (mGroupRelative == 0)
		{
			aPosition.x = theInfo.mOrigin.x + mPosition.x;
			aPosition.y = theInfo.mOrigin.y + mPosition.y;
		}
		while (aCredit > 1.0f)
		{
			if (mMaxParticles != 0 && (int)mChildren.size() >= mMaxParticles)
				break;
			emit(aPosition);
			aCredit = aCredit - 1.0f;
		}
		mEmitCredit = aCredit;
	}
	Container::update(theInfo);
}

// 0x432e10: up to 25 random picks from the pool for a free particle (randomInt never picks the pool's last one), which
// then draws its values from the stream's ranges in the original's order
void ParticleStream::emit(Point thePosition)
{
	Particle* aParticle = nullptr;
	int aTries = 25;
	while (aTries > 0)
	{
		int anIndex = randomInt(0, (int)mParticles.size() - 1);
		Particle* aCandidate = mParticles.at(anIndex);
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
	float x = randomFloat(mPositionX);
	float y = randomFloat(mPositionY);
	if (mGroupRelative == 0)
	{
		if (mAnchor == nullptr)
		{
			x = thePosition.x + x;
			y = thePosition.y + y;
		}
		else
		{
			// against the anchor: the stream's position on the screen, less the anchor's
			Point anOffset;
			anOffset.set(x, y);
			Point aStreamPosition(mAbsPosition);
			anOffset.add(aStreamPosition);
			Point anAnchorPosition(*mAnchor->getAbsPosition());
			x = anOffset.x - anAnchorPosition.x;
			aParticle->mAnchor = mAnchor;
			y = anOffset.y - anAnchorPosition.y;
		}
	}
	aParticle->mGroupRelative = mGroupRelative;
	aParticle->setPosition(x, y);
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
	float aJitterRateX = randomFloat(mJitterRateX);
	float aJitterRateY = randomFloat(mJitterRateY);
	// a particle without jitter keeps a jitter it was given before
	if (aJitterRateX != 0 || aJitterRateY != 0)
	{
		aParticle->removeAllListeners();
		// the original draws the constructor's arguments right to left
		float aJitterAmountY = randomFloat(mJitterAmountY);
		float aJitterAmountX = randomFloat(mJitterAmountX);
		aParticle->addListener(new JitterListener(aJitterRateX, aJitterRateY, aJitterAmountX, aJitterAmountY));
	}
	aParticle->setVisible(true);
	aParticle->mForce = mForce;
	addChild(aParticle);
}

// 0x4335b0: enough particles for the longest life at the emit rate, and half as many again; each round of the loop
// makes one particle for each of the images <path>0, <path>1, ... (a .png, or else a .jpg) up to the first one missing
void ParticleStream::makeParticles()
{
	unsigned int aCount = (unsigned int)(int)((double)mLifeTime.y * mEmitRate * 1.5f + 1.0f);
	while (mParticles.size() < aCount)
	{
		size_t aBefore = mParticles.size();
		int i = 0;
		Image* anImage;
		do
		{
			anImage = nullptr;
			char aName[512];
			snprintf(aName, sizeof(aName), "%s%d%s", mImagePath.c_str(), i, ".png");
			if (!App::instance()->exists(aName))
				snprintf(aName, sizeof(aName), "%s%d%s", mImagePath.c_str(), i, ".jpg");
			if (App::instance()->exists(aName))
			{
				anImage = App::instance()->getImage(aName);
				if (anImage != nullptr)
				{
					anImage->setPivot(1);
					ImageNode* aNode = new ImageNode(anImage);
					Particle* aParticle = new Particle();
					aParticle->addChild(aNode);
					aParticle->addRef();
					mParticles.push_back(aParticle);
				}
			}
			i++;
		} while (anImage != nullptr);
		// the original loops forever when the first image is missing; the port stops
		if (mParticles.size() == aBefore)
			break;
	}
}

// 0x42b920
ParticleSystem::ParticleSystem()
{
	mRunning = false;
	mDuration = -1.0f;
	mTimeLeft = 0;
	setName("ParticleSystem");
}

// 0x42b8c0
void ParticleSystem::update(UpdateInfo& theInfo)
{
	if (mRunning && mDuration > 0)
	{
		mTimeLeft = mTimeLeft - theInfo.mSeconds;
		if (mTimeLeft < 0)
			stopEmitting();
	}
	Container::update(theInfo);
}

// 0x4328e0
void ParticleSystem::layout()
{
	mBounds = Rect();
	clearFlags(NODE_LAYOUT);
}

// 0x42b6b0
void ParticleSystem::warmup(float theSeconds)
{
	UpdateInfo anInfo;
	anInfo.mOrigin = Point(0, 0);
	anInfo.mSeconds = theSeconds;
	Container::update(anInfo);
}

// 0x42b6e0
void ParticleSystem::startEmitting()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			static_cast<ParticleStream*>(*anIt)->startEmitting();
			++anIt;
		} while (anIt != mChildren.end());
	}
	mRunning = true;
	if (mDuration > 0)
		mTimeLeft = mDuration;
}

// 0x42b730
void ParticleSystem::stopEmitting()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			static_cast<ParticleStream*>(*anIt)->stopEmitting();
			++anIt;
		} while (anIt != mChildren.end());
	}
	mRunning = false;
}

// 0x42b760
bool ParticleSystem::isDone()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			if (!static_cast<ParticleStream*>(*anIt)->isDone())
				return false;
			++anIt;
		} while (anIt != mChildren.end());
	}
	return true;
}

// 0x42b7a0
bool ParticleSystem::load(std::string theFile)
{
	ParticleHandler aHandler;
	aHandler.setSystem(this);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
	return true;
}

// 0x42bb20
ParticlePool::ParticlePool(std::string theFile, int theCount)
{
	mFile = theFile;
	if (0 < theCount)
	{
		int aLeft = theCount;
		do
		{
			ParticleSystem* aSystem = new ParticleSystem();
			aSystem->load(mFile);
			Container::addChild(aSystem);
			aLeft--;
		} while (aLeft != 0);
	}
}

// 0x42ba00
bool ParticlePool::isDone()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			if (!static_cast<ParticleSystem*>(*anIt)->isDone())
				return false;
			++anIt;
		} while (anIt != mChildren.end());
	}
	return true;
}

// 0x42ba40
ParticleSystem* ParticlePool::getSystem()
{
	std::vector<Node*>::iterator anIt = mChildren.begin();
	if (anIt != mChildren.end())
	{
		do
		{
			ParticleSystem* aSystem = static_cast<ParticleSystem*>(*anIt);
			if (aSystem->isDone())
				return aSystem;
			++anIt;
		} while (anIt != mChildren.end());
	}
	ParticleSystem* aSystem = new ParticleSystem();
	aSystem->load(mFile);
	addChild(aSystem);
	return aSystem;
}

// 0x433870
ParticleHandler::ParticleHandler()
{
	mStream = nullptr;
	mSystem = nullptr;
}

// 0x433ef0: a settings element outside a stream is passed over
void ParticleHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("particleSystem") == 0)
		startSystem(theAttributes);
	else if (theName.compare("particleStream") == 0)
		startStream(theAttributes);
	else if (theName.compare("velocity") == 0 && mStream != nullptr)
	{
		mStream->mVelocityX = getScaleAttribute(theAttributes, "startX", Point(0, 10.0f));
		mStream->mVelocityY = getScaleAttribute(theAttributes, "startY", Point(0, 10.0f));
	}
	else if (theName.compare("acceleration") == 0 && mStream != nullptr)
		mStream->mAcceleration = getScaleAttribute(theAttributes, "rate", Point(0, 0));
	else if (theName.compare("extForce") == 0 && mStream != nullptr)
		mStream->mForce = getPointAttribute(theAttributes, "force", Point(0, 0));
	else if (theName.compare("position") == 0 && mStream != nullptr)
	{
		mStream->mPositionX = getScaleAttribute(theAttributes, "startX", Point(0, 0));
		mStream->mPositionY = getScaleAttribute(theAttributes, "startY", Point(0, 0));
	}
	else if (theName.compare("rotation") == 0 && mStream != nullptr)
		mStream->mRotation = getScaleAttribute(theAttributes, "degrees", Point(0, 0));
	else if (theName.compare("scale") == 0 && mStream != nullptr)
	{
		mStream->mStartScale = getScaleAttribute(theAttributes, "startScale", Point(1.0f, 1.0f));
		mStream->mEndScale = getScaleAttribute(theAttributes, "endScale", Point(1.0f, 1.0f));
		std::string aConstant = getStringAttribute(theAttributes, "constant", "false");
		mStream->mConstantScale = aConstant.compare("true") == 0;
	}
	else if (theName.compare("particleLife") == 0 && mStream != nullptr)
		mStream->mLifeTime = getScaleAttribute(theAttributes, "lifeTime", Point(1.0f, 2.0f));
	else if (theName.compare("opacity") == 0 && mStream != nullptr)
	{
		mStream->mStartOpacity = getScaleAttribute(theAttributes, "startOp", Point(1.0f, 1.0f));
		mStream->mEndOpacity = getScaleAttribute(theAttributes, "endOp", Point(1.0f, 1.0f));
		std::string aConstant = getStringAttribute(theAttributes, "constant", "false");
		mStream->mConstantOpacity = aConstant.compare("true") == 0;
	}
	else if (theName.compare("resource") == 0 && mStream != nullptr)
	{
		mStream->mImagePath = getStringAttribute(theAttributes, "path", "INVALID_RES_PATH");
		std::string aFilter = getStringAttribute(theAttributes, "filter", "");
		if (aFilter.compare("X") == 0)
			mStream->mFilter = 1;
		else if (aFilter.compare("Y") == 0)
			mStream->mFilter = 2;
		else if (aFilter.compare("XY") == 0)
			mStream->mFilter = 3;
		else
			mStream->mFilter = 0;
	}
	else if (theName.compare("jitter") == 0 && mStream != nullptr)
	{
		std::string anAxis = getStringAttribute(theAttributes, "axis", "");
		if (anAxis.compare("X") == 0)
		{
			mStream->mJitterRateX = getScaleAttribute(theAttributes, "rate", Point(0, 0));
			mStream->mJitterAmountX = getScaleAttribute(theAttributes, "amplitude", Point(0, 0));
		}
		else if (anAxis.compare("Y") == 0)
		{
			mStream->mJitterRateY = getScaleAttribute(theAttributes, "rate", Point(0, 0));
			mStream->mJitterAmountY = getScaleAttribute(theAttributes, "amplitude", Point(0, 0));
		}
	}
	else if (theName.compare("blackHole") == 0 && mStream != nullptr)
	{
		mStream->mBlackHole = true;
		mStream->mBlackHolePosition = getPointAttribute(theAttributes, "pos", Point(0, 0));
		mStream->mBlackHoleAcceleration = getFloatAttribute(theAttributes, "accel", 0);
	}
}

// 0x433940: a stream is complete at its end: its pool is made and it joins the system
void ParticleHandler::endElement(const std::string& theName)
{
	if (theName.compare("particleStream") == 0)
	{
		mStream->makeParticles();
		mSystem->addChild(mStream);
		mStream = nullptr;
	}
}

// 0x433980
void ParticleHandler::startStream(const XmlAttributes& theAttributes)
{
	mStream = new ParticleStream();
	mStream->mEmitRate = getFloatAttribute(theAttributes, "emitRate", 5.0f);
	mStream->mMaxParticles = (int)getIntAttribute(theAttributes, "maxParticles", 0);
	std::string anOffsetType = getStringAttribute(theAttributes, "offsetType", "independent");
	if (anOffsetType.compare("groupRelative") == 0)
		mStream->mGroupRelative = 1;
	else
		mStream->mGroupRelative = 0;
	std::string aParent = getStringAttribute(theAttributes, "parent", "");
	if (aParent.compare("") != 0)
		mStream->setAnchor(aParent);
	mStream->mAutoStop = getFloatAttribute(theAttributes, "autoStop", -1.0f);
	mStream->mStartDelay = getFloatAttribute(theAttributes, "startDelay", -1.0f);
	Point anOffset;
	anOffset = getPointAttribute(theAttributes, "offset", Point(0, 0));
	mStream->setPosition(anOffset.x, anOffset.y);
}

// 0x433e10
void ParticleHandler::startSystem(const XmlAttributes& theAttributes)
{
	mSystem->setDuration(getFloatAttribute(theAttributes, "autoStop", -1.0f));
}
