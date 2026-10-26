#include "SoundBank.h"
#include "App.h"
#include "Layout.h"
#include "Object.h"
#include "Sound.h"
#include "Stream.h"

using namespace engine;

// 0x435a10
SoundGroup::SoundGroup(SoundSystem* theSystem, const char* theName, int theCount, int theCategory)
{
	mName = theName;
	mSystem = theSystem;
	if (0 < theCount)
	{
		int aLeft = theCount;
		do
		{
			Sound* aSound = mSystem->createSound(mName.c_str(), 1, 0);
			aSound->setCategory(theCategory);
			mSounds.push_back(aSound);
			aLeft--;
		} while (aLeft != 0);
	}
}

// 0x435970
SoundGroup::~SoundGroup()
{
	for (size_t i = 0; i < mSounds.size(); i++)
		mSounds[i]->close();
	mSounds.clear();
}

// 0x4358e0
void SoundGroup::play(float theVolume, float thePitch)
{
	std::vector<Sound*>::iterator anIt = mSounds.begin();
	if (anIt == mSounds.end())
		return;
	Sound* aSound;
	while (true)
	{
		aSound = *anIt;
		if (!aSound->isPlaying())
			break;
		++anIt;
		if (anIt == mSounds.end())
			return;
	}
	aSound->setVolume(theVolume);
	aSound->setPitch(thePitch);
	aSound->play();
}

// 0x435940
int SoundGroup::getPlayingCount()
{
	int aCount = 0;
	std::vector<Sound*>::iterator anIt = mSounds.begin();
	if (anIt != mSounds.end())
	{
		do
		{
			if ((*anIt)->isPlaying())
				aCount++;
			++anIt;
		} while (anIt != mSounds.end());
	}
	return aCount;
}

// 0x435af0
Sound* SoundGroup::getFreeSound(bool theMake)
{
	std::vector<Sound*>::iterator anIt = mSounds.begin();
	if (anIt != mSounds.end())
	{
		do
		{
			if (!(*anIt)->isPlaying())
				return *anIt;
			++anIt;
		} while (anIt != mSounds.end());
	}
	if (theMake)
	{
		Sound* aSound = mSystem->createSound(mName.c_str(), 1, 0);
		mSounds.push_back(aSound);
		return aSound;
	}
	return nullptr;
}

// 0x426ac0
SoundBank::SoundBank()
{
	mVolume = 1.0f;
	mPitch = 1.0f;
}

// 0x426760: no sound system, or a file that does not load, gives false
bool SoundBank::add(std::string theName, std::string theFile, int theCount, float theVolume, float thePitch,
	int theCategory)
{
	if (getGroup(theName) != nullptr)
		return true;
	if (App::getSoundSystem() == nullptr)
		return false;
	if (!App::getSoundSystem()->load(theName.c_str(), theFile.c_str()))
		return false;
	SoundGroup* aGroup = new SoundGroup(App::getSoundSystem(), theName.c_str(), theCount, theCategory);
	mGroups.insert(std::pair<const std::string, SoundGroup*>(theName, aGroup));
	getGroup(theName);
	mVolume = theVolume;
	mPitch = thePitch;
	return true;
}

// 0x425e80
SoundGroup* SoundBank::getGroup(std::string theName)
{
	std::map<std::string, SoundGroup*>::iterator anIt = mGroups.find(theName);
	if (anIt != mGroups.end())
		return anIt->second;
	return nullptr;
}

// 0x425f10
void SoundBank::play(std::string theName, float theVolume, float thePitch)
{
	SoundGroup* aGroup = getGroup(theName);
	if (aGroup != nullptr)
		aGroup->play(theVolume * mVolume, thePitch * mPitch);
}

// 0x425cf0
bool SoundBank::load(std::string theFile)
{
	SoundsHandler aHandler;
	aHandler.setBank(this);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(aHandler, aStream);
	return true;
}

// 0x42beb0
void SoundsHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("sound") != 0)
		return;
	std::string aName = getStringAttribute(theAttributes, "name", "");
	std::string aFile = getStringAttribute(theAttributes, "file", "");
	float aVolume = getFloatAttribute(theAttributes, "vol", 1.0f);
	float aPitch = getFloatAttribute(theAttributes, "pitch", 1.0f);
	int aCount = (int)getIntAttribute(theAttributes, "count", 1);
	int aCategory = (int)getIntAttribute(theAttributes, "category", 0);
	mBank->add(aName, aFile, aCount, aVolume, aPitch, aCategory);
}
