#include "SoundMgr.h"
#include "App.h"
#include "Layout.h"
#include "Random.h"
#include "Sound.h"
#include "Stream.h"

using namespace engine;

// 0x490e10 (pizzafrenzy)
ManifestSound::ManifestSound(SoundSystem* theSystem, const std::string& theName, int theCount, float theVolume,
	float thePitch, int theCategory)
{
	mSystem = theSystem;
	mName = theName;
	mVolume = theVolume;
	mPitch = thePitch;
	if (0 < theCount)
	{
		int aLeft = theCount;
		do
		{
			Sound* aVoice = mSystem->createSound(mName.c_str(), true, 0);
			aVoice->setCategory(theCategory);
			mVoices.push_back(aVoice);
			aLeft--;
		} while (aLeft != 0);
	}
}

// 0x490b20 (pizzafrenzy)
ManifestSound::~ManifestSound()
{
	for (size_t i = 0; i < mVoices.size(); i++)
		mVoices[i]->close();
	mVoices.clear();
}

// 0x490f00 (pizzafrenzy)
Sound* ManifestSound::getVoice(bool theCreate)
{
	for (std::vector<Sound*>::iterator anIt = mVoices.begin(); anIt != mVoices.end(); ++anIt)
	{
		Sound* aVoice = *anIt;
		if (!aVoice->isPlaying())
		{
			aVoice->setVolume(mVolume);
			aVoice->setPitch(mPitch);
			return aVoice;
		}
	}
	if (theCreate)
	{
		Sound* aVoice = mSystem->createSound(mName.c_str(), true, 0);
		mVoices.push_back(aVoice);
		return aVoice;
	}
	return nullptr;
}

// 0x490ab0 (pizzafrenzy)
void ManifestSound::play(float theVolume, float thePitch)
{
	Sound* aVoice = getVoice(false);
	if (aVoice != nullptr)
	{
		aVoice->setVolume(theVolume * mVolume);
		aVoice->setPitch(thePitch * mPitch);
		aVoice->play();
	}
}

RandomSound::~RandomSound()
{
	for (size_t i = 0; i < mChoices.size(); i++)
		delete mChoices[i].mEntry;
}

// 0x46f2e0 (pizzafrenzy)
void RandomSound::addEntry(RandomSoundEntry* theEntry, float theWeight)
{
	float aCumulative = theWeight + mTotalWeight;
	mTotalWeight = aCumulative;
	Choice aChoice;
	aChoice.mEntry = theEntry;
	aChoice.mCumulativeWeight = aCumulative;
	mChoices.push_back(aChoice);
}

// 0x46df70 (pizzafrenzy)
RandomSoundEntry* RandomSound::pick()
{
	if (mChoices.empty())
		return nullptr;
	size_t aFirst = 0;
	if (mChoices.size() != 1)
	{
		// the original's random float (0x476c20 (pizzafrenzy)) from 0 to the total weight, then the first entry
		// whose cumulative weight is not below it (0x46dd70 (pizzafrenzy), a binary search)
		float aDraw = randomFloat(Point(0, mTotalWeight));
		size_t aCount = mChoices.size();
		while (aCount > 0)
		{
			size_t aHalf = aCount / 2;
			if (mChoices[aFirst + aHalf].mCumulativeWeight < aDraw)
			{
				aFirst = aFirst + aHalf + 1;
				aCount = aCount - aHalf - 1;
			}
			else
			{
				aCount = aHalf;
			}
		}
	}
	return mChoices[aFirst].mEntry;
}

// 0x46f090 (pizzafrenzy): the sounds are deleted; the random sounds the original leaves, the port deletes
SoundMgr::~SoundMgr()
{
	for (std::map<std::string, ManifestSound*>::iterator anIt = mSounds.begin(); anIt != mSounds.end(); ++anIt)
		delete anIt->second;
	mSounds.clear();
	for (std::map<std::string, RandomSound*>::iterator anIt = mRandomSounds.begin(); anIt != mRandomSounds.end(); ++anIt)
		delete anIt->second;
	mRandomSounds.clear();
}

// 0x46ddc0 (pizzafrenzy)
void SoundMgr::load(const std::string& theFile)
{
	SoundMgrDocHandler aHandler;
	aHandler.setManifest(this);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
}

// 0x46f1c0 (pizzafrenzy)
bool SoundMgr::add(const std::string& theName, const std::string& theFile, int theCount, float theVolume,
	float thePitch, int theCategory)
{
	if (mSounds.find(theName) != mSounds.end())
		return true;
	if (!App::instance()->loadSound(theName, theFile))
	{
		// the application's slot 0xa0 (0x4bd010 (pizzafrenzy)), a printf to the log
		std::string aWarning = "Warning: sound file " + theName + "=" + theFile + " missing or invalid\n";
		App::instance()->log(aWarning.c_str());
		return false;
	}
	mSounds[theName] = new ManifestSound(App::getSoundSystem(), theName, theCount, theVolume, thePitch, theCategory);
	return true;
}

// 0x46f2e0 (pizzafrenzy)
void SoundMgr::addRandom(const std::string& theRandomName, const std::string& theName, float theWeight,
	float theVolume, float thePitch)
{
	RandomSound* aRandom;
	std::map<std::string, RandomSound*>::iterator anIt = mRandomSounds.find(theRandomName);
	if (anIt == mRandomSounds.end())
	{
		aRandom = new RandomSound();
		mRandomSounds[theRandomName] = aRandom;
	}
	else
	{
		aRandom = anIt->second;
	}
	RandomSoundEntry* anEntry = new RandomSoundEntry();
	anEntry->mName = theName;
	anEntry->mPitch = thePitch;
	anEntry->mVolume = theVolume;
	aRandom->addEntry(anEntry, theWeight);
}

// 0x46dfe0 (pizzafrenzy)
ManifestSound* SoundMgr::getSound(const std::string& theName)
{
	std::map<std::string, ManifestSound*>::iterator anIt = mSounds.find(theName);
	if (anIt == mSounds.end())
		return nullptr;
	return anIt->second;
}

// 0x46e420 (pizzafrenzy)
bool SoundMgr::play(const std::string& theName, float theVolume, float thePitch)
{
	ManifestSound* aSound = getSound(theName);
	if (aSound != nullptr)
	{
		aSound->play(theVolume, thePitch);
		return true;
	}
	std::map<std::string, RandomSound*>::iterator anIt = mRandomSounds.find(theName);
	if (anIt != mRandomSounds.end())
	{
		RandomSoundEntry* anEntry = anIt->second->pick();
		if (anEntry != nullptr)
			return play(anEntry->mName, anEntry->mVolume, anEntry->mPitch);
	}
	return false;
}

// 0x46e4c0 (pizzafrenzy)
Sound* SoundMgr::getVoice(const std::string& theName)
{
	std::string aName = theName;
	do
	{
		ManifestSound* aSound = getSound(aName);
		if (aSound != nullptr)
			return aSound->getVoice(false);
		std::map<std::string, RandomSound*>::iterator anIt = mRandomSounds.find(aName);
		if (anIt == mRandomSounds.end())
			break;
		RandomSoundEntry* anEntry = anIt->second->pick();
		if (anEntry == nullptr)
			break;
		aName = anEntry->mName;
	} while (true);
	return nullptr;
}

// 0x491170 (pizzafrenzy)
void SoundMgrDocHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	std::string aName = getStringAttribute(theAttributes, "name", "");
	if (theName.compare("randomSound") == 0)
	{
		mRandomName = aName;
	}
	else if (theName.compare("sound") == 0)
	{
		float aVolume = getFloatAttribute(theAttributes, "vol", 1.0f);
		float aPitch = getFloatAttribute(theAttributes, "pitch", 1.0f);
		if (mRandomName.size() == 0)
		{
			std::string aFile = getStringAttribute(theAttributes, "file", "");
			int aCount = (int)getIntAttribute(theAttributes, "count", 1);
			int aCategory = (int)getIntAttribute(theAttributes, "category", 0);
			mManifest->add(aName, aFile, aCount, aVolume, aPitch, aCategory);
		}
		else
		{
			float aWeight = getFloatAttribute(theAttributes, "weight", 1.0f);
			mManifest->addRandom(mRandomName, aName, aWeight, aVolume, aPitch);
		}
	}
}

// 0x491120 (pizzafrenzy)
void SoundMgrDocHandler::endElement(const std::string& theName)
{
	if (theName.compare("randomSound") == 0)
		mRandomName.clear();
}
