#include "Music.h"
#include "App.h"
#include "Layout.h"
#include "Node.h"
#include "Random.h"
#include "Sound.h"
#include "Stream.h"
#include <cstdio>
#include <stdexcept>

using namespace engine;

// 0x424160
Sound* engine::registerClip(const std::string& thePath, const std::string& theName)
{
	char aText[512];
	if (App::getSoundSystem() != nullptr &&
		App::getSoundSystem()->loadStream(theName.c_str(), thePath.c_str()))
	{
		Sound* aSound = App::getSoundSystem()->createSound(theName.c_str(), 0, 1);
		if (aSound == nullptr)
		{
			snprintf(aText, sizeof(aText), "ERROR: could not get sound handle %s. Same music file used twice?\n",
				theName.c_str());
			App::instance()->log(aText);
		}
		return aSound;
	}
	snprintf(aText, sizeof(aText),
		"ERROR: could not register sound stream %s. Missing file or appropriate CODEC?\n", thePath.c_str());
	App::instance()->log(aText);
	return nullptr;
}

// 0x424810
MusicLayer::MusicLayer()
{
	mSequence = MUSIC_RANDOM;
	mDelay = Point(0, 0);
	mInitialDelay = Point();
	mFadeIn = 0;
	mFadeOut = 0;
	mVolume = 1.0f;
	mTimeLeft = 0;
	mCurrent = nullptr;
	mStartClip = nullptr;
	mFinalClip = nullptr;
	mBridgeClip = nullptr;
	mRunning = false;
	mIndex = 0;
	mStopMode = MUSIC_STOP_FADE_OUT;
	mChosen = -1;
}

// 0x424220
MusicLayer::~MusicLayer()
{
	std::vector<Sound*>::iterator anIt = mClips.begin();
	if (anIt != mClips.end())
	{
		do
		{
			(*anIt)->close();
			++anIt;
		} while (anIt != mClips.end());
	}
}

// 0x4245f0
Sound* MusicLayer::pickClip()
{
	int anOld = mIndex;
	if (mClips.size() == 0)
	{
		mCurrent = nullptr;
		return nullptr;
	}
	switch (mSequence)
	{
	case MUSIC_RANDOM:
		mIndex = randomIndex(0, (int)mClips.size() - 1);
		break;
	case MUSIC_RANDOM_NOT_LAST:
	{
		int anIndex = randomIndex(0, (int)mClips.size() - 2);
		if (anIndex == mIndex)
			anIndex = (int)mClips.size() - 1;
		mIndex = anIndex;
		break;
	}
	case MUSIC_SEQUENTIAL:
		mIndex = anOld + 1;
		if (anOld + 1 == (int)mClips.size())
			mIndex = 0;
		break;
	case MUSIC_LOOP:
		if (mChosen < 0 || mChosen == anOld)
			break;
		mIndex = mChosen;
		if ((unsigned int)mChosen >= mClips.size())
			mIndex = 0;
		break;
	}
	mChosen = -1;
	if (mBridgeClip != nullptr && mIndex != anOld)
	{
		mBridgeClip->setLoop(false);
		mBridgeClip->play();
	}
	if ((unsigned int)mIndex >= mClips.size())
		throw std::out_of_range("invalid vector<T> subscript");
	mCurrent = mClips[mIndex];
	return mCurrent;
}

// 0x424740
void MusicLayer::playCurrent()
{
	if (mCurrent == nullptr)
		return;
	if (mCurrent->isPlaying())
		mCurrent->stop();
	if (mSequence == MUSIC_LOOP)
		mCurrent->setLoop(true);
	mCurrent->play();
	unsigned int i = 0;
	while (true)
	{
		if (mClips.size() <= i)
			return;
		if (mClips[i] == mCurrent)
			break;
		i++;
	}
	mIndex = (int)i;
}

// 0x4247c0
void MusicLayer::select(int theIndex, bool theNow)
{
	if (theIndex == mIndex)
		return;
	mChosen = theIndex;
	if (!mRunning)
		return;
	if (mCurrent != nullptr)
		mCurrent->setLoop(false);
	if (theNow)
	{
		if (mCurrent != nullptr)
			mCurrent->stop();
		pickClip();
		playCurrent();
	}
}

// 0x424910
void MusicLayer::start()
{
	if (mRunning)
		return;
	float aDelay = randomFloat(mInitialDelay);
	if (0.0f < aDelay)
	{
		mTimeLeft = aDelay;
		return;
	}
	if (mStartClip == nullptr)
		pickClip();
	else
		mCurrent = mStartClip;
	if (mCurrent != nullptr)
	{
		mCurrent->setFadeIn(mFadeIn);
		playCurrent();
		mRunning = true;
	}
}

// 0x424970
void MusicLayer::stop()
{
	if (!mRunning)
		return;
	int aMode = mStopMode;
	mRunning = false;
	mTimeLeft = 0;
	if (aMode == MUSIC_STOP_IMMEDIATE)
	{
		if (mCurrent != nullptr && mCurrent->isPlaying())
		{
			mCurrent->setFadeOut(0);
			mCurrent->stop();
		}
	}
	else if (aMode == MUSIC_STOP_FADE_OUT)
	{
		if (mCurrent != nullptr && mCurrent->isPlaying())
		{
			mCurrent->setFadeOut(mFadeOut);
			mCurrent->stop();
		}
	}
	else if (aMode == MUSIC_STOP_PLAY_FINAL)
	{
		if (mCurrent != nullptr && mCurrent->isPlaying())
			mCurrent->stop();
		if (mFinalClip != nullptr)
		{
			mCurrent = mFinalClip;
			playCurrent();
			mCurrent->setLoop(false);
		}
	}
}

// 0x424a10: stop() does nothing to a layer in its initial delay (it is not running yet), so it still starts when the
// delay runs out
void MusicLayer::update(UpdateInfo& theInfo)
{
	if (!mRunning)
	{
		if (mTimeLeft != 0.0f)
		{
			mTimeLeft = mTimeLeft - theInfo.mSeconds;
			if (mTimeLeft <= 0.0f)
			{
				mTimeLeft = 0;
				if (mStartClip == nullptr)
				{
					pickClip();
					playCurrent();
					mRunning = true;
					return;
				}
				mCurrent = mStartClip;
				playCurrent();
				mRunning = true;
				return;
			}
		}
	}
	else if (mTimeLeft <= 0.0f)
	{
		if (mCurrent != nullptr && !mCurrent->isPlaying())
		{
			mCurrent = nullptr;
			float aDelay = randomFloat(mDelay);
			if (aDelay <= 0.0f)
			{
				pickClip();
				playCurrent();
				return;
			}
			mTimeLeft = aDelay;
			return;
		}
		if (-1 < mChosen)
		{
			pickClip();
			playCurrent();
		}
	}
	else
	{
		mTimeLeft = mTimeLeft - theInfo.mSeconds;
		if (mTimeLeft <= 0.0f)
		{
			mTimeLeft = 0;
			pickClip();
			playCurrent();
			return;
		}
	}
}

// 0x424b80
Sound* MusicLayer::addClip(const std::string& thePath, const std::string& theName)
{
	Sound* aSound = registerClip(thePath, theName);
	if (aSound == nullptr)
		return nullptr;
	aSound->setLoop(false);
	aSound->setFadeIn(0);
	aSound->setFadeOut(0);
	aSound->setVolume(mVolume);
	mClips.push_back(aSound);
	return aSound;
}

// 0x424db0
MusicSystem::~MusicSystem()
{
	std::vector<MusicLayer*>::iterator anIt = mLayers.begin();
	if (anIt != mLayers.end())
	{
		do
		{
			if (*anIt != nullptr)
				delete *anIt;
			++anIt;
		} while (anIt != mLayers.end());
	}
}

// 0x424be0
void MusicSystem::update(UpdateInfo& theInfo)
{
	std::vector<MusicLayer*>::iterator anIt = mLayers.begin();
	if (anIt != mLayers.end())
	{
		do
		{
			(*anIt)->update(theInfo);
			++anIt;
		} while (anIt != mLayers.end());
	}
}

// 0x424c10
void MusicSystem::start()
{
	std::vector<MusicLayer*>::iterator anIt = mLayers.begin();
	if (anIt != mLayers.end())
	{
		do
		{
			(*anIt)->start();
			++anIt;
		} while (anIt != mLayers.end());
	}
}

// 0x424c40
void MusicSystem::stop()
{
	std::vector<MusicLayer*>::iterator anIt = mLayers.begin();
	if (anIt != mLayers.end())
	{
		do
		{
			(*anIt)->stop();
			++anIt;
		} while (anIt != mLayers.end());
	}
}

// 0x424c70
void MusicSystem::select(int theIndex, bool theNow)
{
	std::vector<MusicLayer*>::iterator anIt = mLayers.begin();
	if (anIt != mLayers.end())
	{
		do
		{
			(*anIt)->select(theIndex, theNow);
			++anIt;
		} while (anIt != mLayers.end());
	}
}

// 0x424ca0
bool MusicSystem::load(std::string theFile)
{
	MusicHandler aHandler;
	aHandler.setSystem(this);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
	return true;
}

// 0x4251e0
void MusicSystem::addLayer(MusicLayer* theLayer)
{
	mLayers.push_back(theLayer);
}

// 0x424e90
void MusicSystem::clearLayers()
{
	std::vector<MusicLayer*>::iterator anIt = mLayers.begin();
	if (anIt != mLayers.end())
	{
		do
		{
			if (*anIt != nullptr)
				delete *anIt;
			++anIt;
		} while (anIt != mLayers.end());
	}
	mLayers.clear();
}

// 0x425230
void MusicHandler::setSystem(MusicSystem* theSystem)
{
	mSystem = theSystem;
	theSystem->clearLayers();
}

// 0x4259a0: "special" is searched, so one clip can be the start and the ending clip, and a "start"/"final" clip is
// also in the list
void MusicHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("musicSystem") == 0)
		return;
	if (theName.compare("musicLayer") == 0)
	{
		startLayer(theAttributes);
		return;
	}
	if (theName.compare("clip") != 0)
		return;
	if (mLayer == nullptr)
		return;
	std::string aPath = getStringAttribute(theAttributes, "path", "INVALID_CLIP_PATH");
	std::string aSpecial = getStringAttribute(theAttributes, "special", "");
	std::string aName = getStringAttribute(theAttributes, "name", aPath.c_str());
	if (aSpecial.size() == 0)
	{
		mLayer->addClip(aPath, aName);
		return;
	}
	if (aSpecial.find("startOnly") != std::string::npos)
		mLayer->mStartClip = registerClip(aPath, aName);
	else if (aSpecial.find("start") != std::string::npos)
		mLayer->mStartClip = mLayer->addClip(aPath, aName);
	if (aSpecial.find("finalOnly") != std::string::npos)
		mLayer->mFinalClip = registerClip(aPath, aName);
	else if (aSpecial.find("final") != std::string::npos)
		mLayer->mFinalClip = mLayer->addClip(aPath, aName);
	if (aSpecial.find("bridge") != std::string::npos)
		mLayer->mBridgeClip = registerClip(aPath, aName);
}

// 0x4253c0
void MusicHandler::endElement(const std::string& theName)
{
	if (theName.compare("musicLayer") == 0)
	{
		mSystem->addLayer(mLayer);
		mLayer = nullptr;
	}
}

// 0x425400: an unknown sequence keeps random (the constructor's 0)
void MusicHandler::startLayer(const XmlAttributes& theAttributes)
{
	mLayer = new MusicLayer();
	std::string aSequence = getStringAttribute(theAttributes, "sequence", "random-not-last");
	if (aSequence.compare("random") == 0)
		mLayer->mSequence = MUSIC_RANDOM;
	else if (aSequence.compare("random-not-last") == 0)
		mLayer->mSequence = MUSIC_RANDOM_NOT_LAST;
	else if (aSequence.compare("sequential") == 0)
		mLayer->mSequence = MUSIC_SEQUENTIAL;
	else if (aSequence.compare("loop") == 0)
		mLayer->mSequence = MUSIC_LOOP;
	mLayer->mInitialDelay = getScaleAttribute(theAttributes, "initialDelay", Point(0, 0));
	mLayer->mVolume = getFloatAttribute(theAttributes, "volume", 1.0f);
	mLayer->mFadeIn = getFloatAttribute(theAttributes, "fadeInTime", 0.0f);
	mLayer->mFadeOut = getFloatAttribute(theAttributes, "fadeOutTime", 0.0f);
	std::string aStop = getStringAttribute(theAttributes, "stop", "immediate");
	if (aStop.compare("immediate") == 0)
		mLayer->mStopMode = MUSIC_STOP_IMMEDIATE;
	else if (aStop.compare("fadeOut") == 0)
		mLayer->mStopMode = MUSIC_STOP_FADE_OUT;
	else if (aStop.compare("playFinal") == 0)
		mLayer->mStopMode = MUSIC_STOP_PLAY_FINAL;
	mLayer->mDelay = getScaleAttribute(theAttributes, "delay", Point(0, 0));
	mLayer->mName = getStringAttribute(theAttributes, "name", "Layer");
}
