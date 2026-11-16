#include "ParticleSystemDocHandler.h"

#include "Engine.h"

#include <cstdio>

namespace engine
{

ParticleStreamDef::ParticleStreamDef()
	: mEmitRate(0), mMaxParticles(0), mSequence(PARTICLE_SEQUENCE_RANDOM), mAnimationRate(0), mJitterPeriodX(1, 1),
	  mJitterPeriodY(1, 1), mConstantScale(false), mConstantOpacity(true), mBlackHole(false),
	  mBlackHoleAcceleration(0), mBlackHolePosition(0, 0), mLifeTime(1, 1), mBounds(0, 0, 0, 0), mExternalForce(0, 0),
	  mFilterMode(0), mBlendMode(BLEND_ALPHA), mPivot(PIVOT_CENTER), mAbsolutePivot(false), mPivotPoint(0, 0),
	  mOffsetType(PARTICLE_OFFSET_INDEPENDENT), mAutoStop(-1.0f), mStartDelay(0)
{
}

bool ParticleStreamDef::loadImages()
{
	Engine* anEngine = getEngine();
	mImages.clear();
	int aNumber = 0;
	for (;;)
	{
		char aBuffer[512];
		snprintf(aBuffer, sizeof(aBuffer), "%s%d%s", mPath.c_str(), aNumber, ".png");
		std::string aFile = aBuffer;
		if (!anEngine->fileExists(aFile))
		{
			snprintf(aBuffer, sizeof(aBuffer), "%s%d%s", mPath.c_str(), aNumber, ".jpg");
			aFile = aBuffer;
			if (!anEngine->fileExists(aFile))
				break;
		}
		Surface* aSurface = anEngine->getImage(aFile);
		if (mAbsolutePivot)
			aSurface->setAnchor(mPivotPoint);
		else
			aSurface->setPivot(mPivot);
		aSurface->setBlendMode(mBlendMode, 0);
		mImages.push_back(aSurface);
		aNumber++;
	}
	return aNumber > 0;
}

// 0x45b6b0 (feedingfrenzy2): a string compared with a literal
static bool equals(const std::string& theString, const char* theLiteral)
{
	return theString == theLiteral;
}

void ParticleSystemDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "particleSystem")
	{
		readSystem(theAttributes);
		return;
	}
	if (theName == "particleStream")
	{
		readStream(theAttributes);
		return;
	}
	// Every element below is read into the stream being read, and ignored outside one
	if (theName == "opacityKey")
	{
		readOpacityKey(theAttributes);
		return;
	}
	if (mStream == nullptr)
		return;
	if (theName == "velocity")
	{
		mStream->mVelocityX = theAttributes.getRange("startX", FloatRange(0, 10.0f));
		mStream->mVelocityY = theAttributes.getRange("startY", FloatRange(0, 10.0f));
	}
	else if (theName == "acceleration")
		mStream->mAcceleration = theAttributes.getRange("rate", FloatRange(0, 0));
	else if (theName == "extForce")
		mStream->mExternalForce = theAttributes.getFloatPair("force", Vector2f(0, 0));
	else if (theName == "position")
	{
		mStream->mPositionX = theAttributes.getRange("startX", FloatRange(0, 0));
		mStream->mPositionY = theAttributes.getRange("startY", FloatRange(0, 0));
	}
	else if (theName == "rotation")
		mStream->mRotation = theAttributes.getRange("degrees", FloatRange(0, 0));
	else if (theName == "scale")
	{
		mStream->mStartScale = theAttributes.getRange("startScale", FloatRange(1.0f, 1.0f));
		mStream->mEndScale = theAttributes.getRange("endScale", FloatRange(1.0f, 1.0f));
		mStream->mConstantScale = equals(theAttributes.getString("constant", "false"), "true");
	}
	else if (theName == "particleLife")
	{
		mStream->mLifeTime = theAttributes.getRange("lifeTime", FloatRange(1.0f, 2.0f));
		mStream->mBounds = theAttributes.getRect("bounds", RectI(0, 0, 0, 0));
	}
	else if (theName == "opacity")
	{
		mStream->mStartOpacity = theAttributes.getRange("startOp", FloatRange(1.0f, 1.0f));
		mStream->mEndOpacity = theAttributes.getRange("endOp", FloatRange(1.0f, 1.0f));
		mStream->mConstantOpacity = equals(theAttributes.getString("constant", "false"), "true");
	}
	else if (theName == "resource")
		readResource(theAttributes);
	else if (theName == "jitter")
	{
		// The element's "rate" is not read
		std::string anAxis = theAttributes.getString("axis", "");
		if (equals(anAxis, "X"))
		{
			mStream->mJitterPeriodX = theAttributes.getRange("period", FloatRange(0, 0));
			mStream->mJitterAmplitudeX = theAttributes.getRange("amplitude", FloatRange(0, 0));
		}
		else if (equals(anAxis, "Y"))
		{
			mStream->mJitterPeriodY = theAttributes.getRange("period", FloatRange(0, 0));
			mStream->mJitterAmplitudeY = theAttributes.getRange("amplitude", FloatRange(0, 0));
		}
	}
	else if (theName == "blackHole")
	{
		mStream->mBlackHole = true;
		mStream->mBlackHolePosition = theAttributes.getFloatPair("pos", Vector2f(0, 0));
		mStream->mBlackHoleAcceleration = theAttributes.getFloat("accel", 0.0f);
	}
}

void ParticleSystemDocHandler::endElement(const std::string& theName)
{
	if (theName == "particleStream")
		mStream = nullptr;
}

void ParticleSystemDocHandler::readSystem(Params& theAttributes)
{
	mSystem = new ParticleSystemDef();
	mSystem->mAutoStop = theAttributes.getFloat("autoStop", -1.0f);
	mSystem->mWarmup = theAttributes.getFloat("warmup", 0.0f);
}

void ParticleSystemDocHandler::readStream(Params& theAttributes)
{
	mStream = new ParticleStreamDef();
	// (a stream outside a <particleSystem> crashes the original)
	if (mSystem != nullptr)
		mSystem->mStreams.push_back(mStream);
	mStream->mEmitRate = theAttributes.getFloat("emitRate", 5.0f);
	mStream->mMaxParticles = theAttributes.getInt("maxParticles", 0);
	std::string aSequence = theAttributes.getString("sequence", "random");
	if (equals(aSequence, "anim"))
		mStream->mSequence = PARTICLE_SEQUENCE_ANIMATION;
	else if (equals(aSequence, "sequential"))
		mStream->mSequence = PARTICLE_SEQUENCE_SEQUENTIAL;
	else
		mStream->mSequence = PARTICLE_SEQUENCE_RANDOM;
	mStream->mAnimationRate = theAttributes.getFloat("animRate", 15.0f);
	std::string anOffsetType = theAttributes.getString("offsetType", "independent");
	mStream->mOffsetType =
		equals(anOffsetType, "groupRelative") ? PARTICLE_OFFSET_GROUP_RELATIVE : PARTICLE_OFFSET_INDEPENDENT;
	std::string aParent = theAttributes.getString("parent", "");
	if (!aParent.empty())
		mStream->mParentName = aParent;
	mStream->mAutoStop = theAttributes.getFloat("autoStop", -1.0f);
	mStream->mStartDelay = theAttributes.getFloat("startDelay", -1.0f);
	// The "offset" pair is read into +0x0c and +0x10, which nothing reads
}

void ParticleSystemDocHandler::readOpacityKey(Params& theAttributes)
{
	float anAlpha = theAttributes.getFloat("alpha", 0.0f);
	float aValue = theAttributes.getFloat("value", 1.0f);
	if (mStream == nullptr)
		return;
	FloatKey aKey;
	aKey.mTime = anAlpha;
	aKey.mValue = aValue;
	mStream->mOpacityKeys.push_back(aKey);
}

void ParticleSystemDocHandler::readResource(Params& theAttributes)
{
	// The element's "type" is not read
	mStream->mPath = theAttributes.getString("path", "INVALID_RES_PATH");
	std::string aFilter = theAttributes.getString("filter", "");
	mStream->mFilterMode = (equals(aFilter, "X") || equals(aFilter, "Y") || equals(aFilter, "XY")) ? 1 : 0;
	std::string anAlphaType = theAttributes.getString("alphaType", "alphaBlend");
	if (equals(anAlphaType, "alphaBlend"))
		mStream->mBlendMode = BLEND_ALPHA;
	else if (equals(anAlphaType, "alphaAdd"))
		mStream->mBlendMode = BLEND_ADD;
	else if (equals(anAlphaType, "alphaNone"))
		mStream->mBlendMode = BLEND_OPAQUE;
	mStream->mPivot = theAttributes.getPivot("pivot");
	mStream->mAbsolutePivot = theAttributes.has("abspivot");
	mStream->mPivotPoint = theAttributes.getFloatPair("abspivot", Vector2f(0, 0));
}

} // namespace engine
