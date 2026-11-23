#include "MusicSystem.h"
#include "Draws.h"
#include "Engine.h"
#include "SimpleSound.h"

#include <algorithm>

using namespace engine;
using simplesound::SoundHandle;

namespace
{

const char* const DEFAULT_CLIP_PATH = "INVALID_CLIP_PATH";

// 0x414270 (feedingfrenzy2): a handle of the file's stream, registered under the name, in the category; null, with a
// line logged, when the file cannot be registered or its stream already has a handle
SoundHandle* makeClip(const std::string& theName, const std::string& thePath, int theCategory)
{
	Engine* anEngine = getEngine();
	if (!anEngine->registerSoundStream(theName, thePath))
	{
		anEngine->log("ERROR: could not register sound stream %s. Missing file or appropriate CODEC?\n",
		    theName.c_str());
		return nullptr;
	}
	SoundHandle* aClip = anEngine->getSoundHandle(theName, 0, theCategory);
	if (aClip == nullptr)
		anEngine->log("ERROR: could not get sound handle %s. Same music file used twice?\n", theName.c_str());
	return aClip;
}

} // namespace

MusicLayer::MusicLayer()
    : mSequence(MUSIC_SEQUENCE_RANDOM), mFadeInTime(0.0f), mFadeOutTime(0.0f), mVolume(0.0f), mStartClip(nullptr),
      mFinalClip(nullptr), mBridgeClip(nullptr), mStopMode(MUSIC_STOP_FADE_OUT), mDelayLeft(0.0f),
      mClip(nullptr), mClipIndex(0), mStarted(false), mRequestedIndex(-1)
{
}

MusicLayer::~MusicLayer()
{
	for (SoundHandle* aClip : mClips)
		delete aClip;
	// port: the original lets go of the list's clips only; a start, final or bridge clip that is not on the list
	// ("startOnly", "finalOnly", "bridge") is never let go of. The port lets go of them too
	std::vector<SoundHandle*> aDeleted = mClips;
	for (SoundHandle* aClip : {mStartClip, mFinalClip, mBridgeClip})
	{
		if (aClip != nullptr && std::find(aDeleted.begin(), aDeleted.end(), aClip) == aDeleted.end())
		{
			aDeleted.push_back(aClip);
			delete aClip;
		}
	}
}

SoundHandle* MusicLayer::addClip(const std::string& theName, const std::string& thePath, int theCategory)
{
	SoundHandle* aClip = makeClip(theName, thePath, theCategory);
	if (aClip == nullptr)
		return nullptr;
	aClip->setLooping(false);
	aClip->setFadeInTime(0.0f);
	aClip->setFadeOutTime(0.0f);
	aClip->setVolume(mVolume);
	mClips.push_back(aClip);
	return aClip;
}

void MusicLayer::start()
{
	if (mStarted)
		return;
	float aDelay = drawIn(mInitialDelay);
	if (aDelay > 0.0f)
	{
		mDelayLeft = aDelay;
		return;
	}
	if (mStartClip != nullptr)
		mClip = mStartClip;
	else
		chooseClip();
	if (mClip == nullptr)
		return;
	mClip->setFadeInTime(mFadeInTime);
	playClip();
	mStarted = true;
}

void MusicLayer::stop()
{
	if (!mStarted)
		return;
	mDelayLeft = 0.0f;
	mStarted = false;
	switch (mStopMode)
	{
	case MUSIC_STOP_AT_ONCE:
		if (mClip != nullptr && mClip->isPlaying())
		{
			mClip->setFadeOutTime(0.0f);
			mClip->stop();
		}
		break;
	case MUSIC_STOP_FADE_OUT:
		if (mClip != nullptr && mClip->isPlaying())
		{
			mClip->setFadeOutTime(mFadeOutTime);
			mClip->stop();
		}
		break;
	case MUSIC_STOP_PLAY_FINAL:
		if (mClip != nullptr && mClip->isPlaying())
			mClip->stop();
		if (mFinalClip != nullptr)
		{
			mClip = mFinalClip;
			playClip();
			mClip->setLooping(false);
		}
		break;
	}
}

void MusicLayer::update(UpdateContext* theContext)
{
	if (!mStarted)
	{
		if (mDelayLeft != 0.0f)
		{
			mDelayLeft = mDelayLeft - theContext->mElapsed;
			if (!(mDelayLeft > 0.0f))
			{
				mDelayLeft = 0.0f;
				if (mStartClip != nullptr)
					mClip = mStartClip;
				else
					chooseClip();
				playClip();
				mStarted = true;
			}
		}
		return;
	}
	if (mDelayLeft > 0.0f)
	{
		mDelayLeft = mDelayLeft - theContext->mElapsed;
		if (mDelayLeft > 0.0f)
			return;
		mDelayLeft = 0.0f;
	}
	else if (mClip == nullptr || mClip->isPlaying())
	{
		// A loop layer goes on to the clip asked for
		if (mRequestedIndex < 0)
			return;
	}
	else
	{
		mClip = nullptr;
		float aDelay = drawIn(mDelay);
		if (aDelay > 0.0f)
		{
			mDelayLeft = aDelay;
			return;
		}
	}
	chooseClip();
	playClip();
}

void MusicLayer::setVolume(float theVolume)
{
	mVolume = theVolume;
	if (mClip != nullptr)
		mClip->setVolume(theVolume);
}

SoundHandle* MusicLayer::chooseClip()
{
	if (mClips.empty())
	{
		mClip = nullptr;
		return nullptr;
	}
	int aCount = (int)mClips.size();
	int aLastIndex = mClipIndex;
	switch (mSequence)
	{
	case MUSIC_SEQUENCE_RANDOM:
		mClipIndex = drawWhole(0, aCount - 1);
		break;
	case MUSIC_SEQUENCE_RANDOM_NOT_LAST:
	{
		// One of all but the last drawn, the last taking the place of the one played before
		int anIndex = drawWhole(0, aCount - 2);
		if (anIndex == mClipIndex)
			anIndex = aCount - 1;
		// port: with one clip the draw is of 0 to -1, so 0 or 1, and the original takes the clip past the list's end
		// when it is 1; the port keeps to the list's last
		if (anIndex >= aCount)
			anIndex = aCount - 1;
		mClipIndex = anIndex;
		break;
	}
	case MUSIC_SEQUENCE_SEQUENTIAL:
		mClipIndex = aLastIndex + 1;
		if (mClipIndex == aCount)
			mClipIndex = 0;
		break;
	case MUSIC_SEQUENCE_LOOP:
		if (mRequestedIndex >= 0 && mRequestedIndex != aLastIndex)
		{
			mClipIndex = mRequestedIndex;
			if (mClipIndex >= aCount)
				mClipIndex = 0;
		}
		break;
	}
	mRequestedIndex = -1;
	if (mBridgeClip != nullptr && mClipIndex != aLastIndex)
	{
		mBridgeClip->setLooping(false);
		mBridgeClip->play();
	}
	mClip = mClips[mClipIndex];
	return mClip;
}

void MusicLayer::playClip()
{
	if (mClip == nullptr)
		return;
	if (mClip->isPlaying())
		mClip->stop();
	if (mSequence == MUSIC_SEQUENCE_LOOP)
		mClip->setLooping(true);
	mClip->play();
	for (int i = 0; i < (int)mClips.size(); i++)
	{
		if (mClips[i] == mClip)
		{
			mClipIndex = i;
			return;
		}
	}
}

void MusicSystem::load(const std::string& thePath)
{
	MusicSystemDocHandler aHandler(this);
	clear();
	getEngine()->parseXml(thePath, &aHandler);
}

void MusicSystem::addLayer(MusicLayer* theLayer)
{
	mLayers.push_back(theLayer);
}

void MusicSystem::start()
{
	for (Ref<MusicLayer>& aLayer : mLayers)
		aLayer->start();
}

void MusicSystem::stop()
{
	for (Ref<MusicLayer>& aLayer : mLayers)
		aLayer->stop();
}

void MusicSystem::update(UpdateContext* theContext)
{
	for (Ref<MusicLayer>& aLayer : mLayers)
		aLayer->update(theContext);
}

void MusicSystem::setVolume(float theVolume)
{
	for (Ref<MusicLayer>& aLayer : mLayers)
		aLayer->setVolume(theVolume);
}

void MusicSystem::clear()
{
	mLayers.clear();
}

MusicSystemDocHandler::MusicSystemDocHandler(MusicSystem* theSystem) : mSystem(theSystem), mCategory(0)
{
}

void MusicSystemDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "musicSystem")
	{
		mCategory = theAttributes.getInt("category", 1);
		return;
	}
	if (theName == "musicLayer")
	{
		beginLayer(theAttributes);
		return;
	}
	if (theName != "clip" || !mLayer)
		return;
	std::string aPath = theAttributes.getString("path", DEFAULT_CLIP_PATH);
	std::string aSpecial = theAttributes.getString("special", "");
	std::string aName = theAttributes.getString("name", aPath);
	if (aSpecial.empty())
	{
		mLayer->addClip(aName, aPath, mCategory);
		return;
	}
	// "startOnly" and "finalOnly" are left off the list, "start" and "final" put on it too; a "bridge" never is
	if (aSpecial.find("startOnly") != std::string::npos)
		mLayer->mStartClip = makeClip(aName, aPath, mCategory);
	else if (aSpecial.find("start") != std::string::npos)
		mLayer->mStartClip = mLayer->addClip(aName, aPath, mCategory);
	if (aSpecial.find("finalOnly") != std::string::npos)
		mLayer->mFinalClip = makeClip(aName, aPath, mCategory);
	else if (aSpecial.find("final") != std::string::npos)
		mLayer->mFinalClip = mLayer->addClip(aName, aPath, mCategory);
	if (aSpecial.find("bridge") != std::string::npos)
		mLayer->mBridgeClip = makeClip(aName, aPath, mCategory);
}

void MusicSystemDocHandler::endElement(const std::string& theName)
{
	if (theName == "musicLayer")
	{
		mSystem->addLayer(mLayer.get());
		mLayer = nullptr;
	}
}

void MusicSystemDocHandler::beginLayer(Params& theAttributes)
{
	mLayer = new MusicLayer();
	std::string aSequence = theAttributes.getString("sequence", "random-not-last");
	if (aSequence == "random")
		mLayer->mSequence = MUSIC_SEQUENCE_RANDOM;
	else if (aSequence == "random-not-last")
		mLayer->mSequence = MUSIC_SEQUENCE_RANDOM_NOT_LAST;
	else if (aSequence == "sequential")
		mLayer->mSequence = MUSIC_SEQUENCE_SEQUENTIAL;
	else if (aSequence == "loop")
		mLayer->mSequence = MUSIC_SEQUENCE_LOOP;
	mLayer->mInitialDelay = theAttributes.getRange("initialDelay", FloatRange(0.0f, 0.0f));
	mLayer->mVolume = theAttributes.getFloat("volume", 1.0f);
	mLayer->mFadeInTime = theAttributes.getFloat("fadeInTime", 0.0f);
	mLayer->mFadeOutTime = theAttributes.getFloat("fadeOutTime", 0.0f);
	std::string aStopMode = theAttributes.getString("immediate", "stop");
	if (aStopMode == "stop")
		mLayer->mStopMode = MUSIC_STOP_AT_ONCE;
	else if (aStopMode == "fadeOut")
		mLayer->mStopMode = MUSIC_STOP_FADE_OUT;
	else if (aStopMode == "playFinal")
		mLayer->mStopMode = MUSIC_STOP_PLAY_FINAL;
	mLayer->mDelay = theAttributes.getRange("delay", FloatRange(0.0f, 0.0f));
	mLayer->mName = theAttributes.getString("name", "Layer");
}
