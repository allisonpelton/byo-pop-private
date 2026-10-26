// The engine's check: opens WHArchive.saf from a game folder (assets/wordharmony by default) through the engine's
// resource manager and parses every XML file in it with the engine's parser, counting the elements; then reads each as
// a screen layout, as a particle file, as a sounds file and as a music file; runs the particle systems until they are
// done and the music for a while, over a sound system that plays nothing. It opens no window and plays no sound.
//   wordharmony_enginetest [<game folder>]

#include "App.h"
#include "Exception.h"
#include "Font.h"
#include "Graphics.h"
#include "Host.h"
#include "Music.h"
#include "Node.h"
#include "Particles.h"
#include "ScreenLayout.h"
#include "Sound.h"
#include "SoundBank.h"
#include "Xml.h"
#include <chrono>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

using namespace engine;

// Images of a fixed size: the test reads no pixels
class TestImage : public Image
{
public:
	int getWidth() override { return 16; }
	int getHeight() override { return 16; }
};

class TestHost : public Host
{
public:
	std::string mDir;

	uint32_t timeMs() override
	{
		return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}
	bool readGameFile(const std::string& theName, std::vector<uint8_t>& theData) override
	{
		std::ifstream aFile(mDir + "/" + theName, std::ios::binary);
		if (!aFile)
			return false;
		theData.assign(std::istreambuf_iterator<char>(aFile), std::istreambuf_iterator<char>());
		return true;
	}
	bool gameFileExists(const std::string& theName) override
	{
		return std::ifstream(mDir + "/" + theName).good();
	}
	Image* createImage(const std::vector<uint8_t>&, const std::vector<uint8_t>*) override { return new TestImage(); }
	bool registryReadString(const std::string&, std::string&) override { return false; }
	bool registryWriteString(const std::string&, const std::string&) override { return false; }
	bool registryReadInt(const std::string&, int&) override { return false; }
	bool registryWriteInt(const std::string&, int) override { return false; }
	void setFullScreen(bool) override {}
	bool isFullScreen() override { return false; }
	void quit() override {}
	SoundSystem* createSoundSystem() override { return nullptr; }
	void openURL(const std::string&) override {}
	void log(const char* theText) override { fputs(theText, stdout); }
	void showMessage(const char* theCaption, const char* theText) override { printf("%s: %s\n", theCaption, theText); }
};

class CountingHandler : public XmlHandler
{
public:
	int mStarts = 0;
	int mEnds = 0;
	bool mDocument = false;
	void startElement(const std::string&, const XmlAttributes&) override { mStarts++; }
	void endElement(const std::string&) override { mEnds++; }
	void endDocument() override { mDocument = true; }
};

// The sound files are loose files beside the archive, named in other cases than the files give; the host's sound
// system looks them up ignoring case, and so does this check
static bool fileExistsIgnoringCase(const std::string& theDir, const std::string& thePath)
{
	std::filesystem::path aDir = theDir;
	size_t aStart = 0;
	while (aStart < thePath.size())
	{
		size_t anEnd = thePath.find_first_of("/\\", aStart);
		if (anEnd == std::string::npos)
			anEnd = thePath.size();
		std::string aPart = thePath.substr(aStart, anEnd - aStart);
		aStart = anEnd + 1;
		if (aPart.empty())
			continue;
		bool aFound = false;
		std::error_code anError;
		for (const auto& anEntry : std::filesystem::directory_iterator(aDir, anError))
		{
			std::string aName = anEntry.path().filename().string();
			if (aName.size() != aPart.size())
				continue;
			size_t i = 0;
			while (i < aName.size() && tolower((unsigned char)aName[i]) == tolower((unsigned char)aPart[i]))
				i++;
			if (i == aName.size())
			{
				aDir = anEntry.path();
				aFound = true;
				break;
			}
		}
		if (!aFound)
			return false;
	}
	return std::filesystem::is_regular_file(aDir);
}

// A voice that plays nothing: it "plays" for three seconds of the system's updates, or until stopped
class TestSound : public Sound
{
public:
	std::string mFile;
	int mLeftMs = 0;
	int mPlays = 0;
	bool mLoop = false;
	float mVolume = 1.0f;
	float mPitch = 1.0f;
	int mCategory = 0;
	void play() override
	{
		if (mLeftMs > 0)
			return;
		mLeftMs = 3000;
		mPlays++;
	}
	void halt() override { mLeftMs = 0; }
	void stop() override { mLeftMs = 0; }
	void rewind() override {}
	void close() override {}
	void setPitch(float thePitch) override { mPitch = thePitch; }
	float getPitch() override { return mPitch; }
	void setPan(float) override {}
	float getPan() override { return 0; }
	void setVolume(float theVolume) override { mVolume = theVolume; }
	float getVolume() override { return mVolume; }
	void setLoop(bool theLoop) override { mLoop = theLoop; }
	bool isLoop() override { return mLoop; }
	bool isPlaying() override { return mLeftMs > 0; }
	void setFadeIn(float) override {}
	void setFadeOut(float) override {}
	void setCategory(int theCategory) override { mCategory = theCategory; }
	int getCategory() override { return mCategory; }
	void update(int theMs) override
	{
		if (mLeftMs <= 0)
			return;
		mLeftMs -= theMs;
		if (mLeftMs <= 0 && mLoop)
		{
			mLeftMs = 3000;
			mPlays++;
		}
	}
};

class TestSoundSystem : public SoundSystem
{
public:
	std::string mDir;
	std::map<std::string, std::string> mLoaded;
	std::vector<TestSound*> mSounds;
	int mMissing = 0;
	bool load(const char* theName, const char* theFile) override
	{
		if (mLoaded.count(theName) != 0)
			return true;
		if (!fileExistsIgnoringCase(mDir, theFile))
		{
			printf("FAIL sound %s: no file %s\n", theName, theFile);
			mMissing++;
			return false;
		}
		mLoaded[theName] = theFile;
		return true;
	}
	bool loadStream(const char* theName, const char* theFile) override { return load(theName, theFile); }
	void unloadUnused() override {}
	void unload(const char* theName) override { mLoaded.erase(theName); }
	Sound* createSound(const char* theName, int, int) override
	{
		std::map<std::string, std::string>::iterator anIt = mLoaded.find(theName);
		if (anIt == mLoaded.end())
			return nullptr;
		TestSound* aSound = new TestSound();
		aSound->mFile = anIt->second;
		mSounds.push_back(aSound);
		return aSound;
	}
	void setVolume(float) override {}
	float getVolume() override { return 1; }
	void setCategoryVolume(int, float) override {}
	float getCategoryVolume(int) override { return 1; }
	void update(int theMs) override
	{
		for (TestSound* aSound : mSounds)
			aSound->update(theMs);
	}
};

class TestApp : public App
{
};

int main(int argc, char** argv)
{
	TestHost aHost;
	aHost.mDir = argc > 1 ? argv[1] : "assets/wordharmony";
	setHost(&aHost);
	TestApp* anApp = new TestApp();
	anApp->addRef();
	int aFiles = 0;
	int aFailed = 0;
	int anElements = 0;
	try
	{
		anApp->addArchive("WHArchive.saf");
	}
	catch (const Exception& e)
	{
		printf("FAIL: %s\n", e.what());
		return 1;
	}
	std::vector<std::string> aNames = anApp->getResources()->getArchives()[0]->getNames();
	for (const std::string& aName : aNames)
	{
		if (aName.size() < 4 || aName.compare(aName.size() - 4, 4, ".xml") != 0)
			continue;
		aFiles++;
		try
		{
			Ref<InputStream> aStream = anApp->openStream(aName.c_str());
			CountingHandler aHandler;
			parseXml(aHandler, aStream);
			if (!aHandler.mDocument || aHandler.mStarts != aHandler.mEnds)
			{
				printf("FAIL %s: %d starts, %d ends, document end %d\n", aName.c_str(), aHandler.mStarts,
					aHandler.mEnds, aHandler.mDocument);
				aFailed++;
			}
			anElements += aHandler.mStarts;
		}
		catch (const Exception& e)
		{
			printf("FAIL %s: %s\n", aName.c_str(), e.what());
			aFailed++;
		}
	}
	printf("%d files in the archive, %d XML files parsed, %d elements, %d failed\n", (int)aNames.size(), aFiles,
		anElements, aFailed);

	// Every XML file read as a screen layout (the files that are not layouts give no nodes)
	int aScreens = 0;
	int aNodes = 0;
	for (const std::string& aName : aNames)
	{
		if (aName.size() < 4 || aName.compare(aName.size() - 4, 4, ".xml") != 0)
			continue;
		Ref<ScreenLayout> aScreen = new ScreenLayout();
		try
		{
			aScreen->loadLayout(aName);
			if (aScreen->mNodes.size() != 0)
			{
				aScreens++;
				aNodes += (int)aScreen->mNodes.size();
				printf("layout %s: %d named nodes, %d children\n", aName.c_str(), (int)aScreen->mNodes.size(),
					(int)aScreen->mChildren.size());
			}
		}
		catch (const Exception& e)
		{
			printf("FAIL layout %s: %s\n", aName.c_str(), e.what());
			aFailed++;
		}
	}
	printf("%d layouts, %d named nodes\n", aScreens, aNodes);

	// Every XML file read as a particle file (the files that are not give no streams), emitting for five seconds of
	// 60 frames a second, then run until it is done
	int aSystems = 0;
	int aStreams = 0;
	for (const std::string& aName : aNames)
	{
		if (aName.size() < 4 || aName.compare(aName.size() - 4, 4, ".xml") != 0)
			continue;
		Ref<ParticleSystem> aSystem = new ParticleSystem();
		try
		{
			aSystem->load(aName);
			if (aSystem->mChildren.size() == 0)
				continue;
			aSystems++;
			aStreams += (int)aSystem->mChildren.size();
			int aPool = 0;
			float aLongest = 0;
			for (Node* aChild : aSystem->mChildren)
			{
				ParticleStream* aStream = static_cast<ParticleStream*>(aChild);
				aPool += (int)aStream->mParticles.size();
				if (aStream->mLifeTime.y > aLongest)
					aLongest = aStream->mLifeTime.y;
			}
			if (aPool == 0)
			{
				// smoke.xml names images the archive does not have (and no code names the file): the original's
				// makeParticles would loop forever on it
				printf("particles %s: no images\n", aName.c_str());
				continue;
			}
			aSystem->startEmitting();
			int aMost = 0;
			int aFrames = 0;
			int anAfter = (int)((aLongest + 1) * 60);
			while (aFrames < 300 || (!aSystem->isDone() && aFrames < 300 + anAfter))
			{
				if (aFrames == 300)
					aSystem->stopEmitting();
				UpdateInfo anInfo;
				anInfo.mSeconds = 1.0f / 60;
				aSystem->update(anInfo);
				int aLive = 0;
				for (Node* aChild : aSystem->mChildren)
					aLive += (int)static_cast<ParticleStream*>(aChild)->mChildren.size();
				if (aLive > aMost)
					aMost = aLive;
				aFrames++;
			}
			printf("particles %s: %d streams, %d in the pools, at most %d at once, %s %d frames after the stop\n",
				aName.c_str(), (int)aSystem->mChildren.size(), aPool, aMost, aSystem->isDone() ? "done" : "NOT done",
				aFrames - 300);
			if (!aSystem->isDone())
				aFailed++;
		}
		catch (const std::exception& e)
		{
			printf("FAIL particles %s: %s\n", aName.c_str(), e.what());
			aFailed++;
		}
	}
	printf("%d particle files, %d streams\n", aSystems, aStreams);

	// Every XML file read as a sounds file and as a music file; each music file's layers run for two minutes of 60
	// frames a second (the sound system's update every 100 ms, as the window's timer calls it), stop, and run ten
	// seconds more
	TestSoundSystem aSoundSystem;
	aSoundSystem.mDir = aHost.mDir;
	anApp->setSoundSystem(&aSoundSystem);
	SoundBank aBank;
	int aMusicFiles = 0;
	for (const std::string& aName : aNames)
	{
		if (aName.size() < 4 || aName.compare(aName.size() - 4, 4, ".xml") != 0)
			continue;
		try
		{
			size_t aGroups = aBank.mGroups.size();
			aBank.load(aName);
			if (aBank.mGroups.size() != aGroups)
				printf("sounds %s: %d sounds\n", aName.c_str(), (int)(aBank.mGroups.size() - aGroups));
			size_t aFirst = aSoundSystem.mSounds.size();
			MusicSystem aMusic;
			aMusic.load(aName);
			if (aMusic.mLayers.empty())
				continue;
			aMusicFiles++;
			int aClips = 0;
			for (MusicLayer* aLayer : aMusic.mLayers)
				aClips += (int)aLayer->mClips.size();
			aMusic.start();
			UpdateInfo anInfo;
			anInfo.mSeconds = 1.0f / 60;
			int aMs = 0;
			for (int aFrame = 0; aFrame < 130 * 60; aFrame++)
			{
				if (aFrame == 120 * 60)
					aMusic.stop();
				aMusic.update(anInfo);
				aMs += 1000 / 60;
				while (aMs >= 100)
				{
					aSoundSystem.update(100);
					aMs -= 100;
				}
			}
			int aPlays = 0;
			int aLooping = 0;
			for (size_t i = aFirst; i < aSoundSystem.mSounds.size(); i++)
			{
				aPlays += aSoundSystem.mSounds[i]->mPlays;
				if (aSoundSystem.mSounds[i]->isPlaying() && aSoundSystem.mSounds[i]->mLoop)
					aLooping++;
			}
			printf("music %s: %d layers, %d clips, %d plays in two minutes, %d looping after the stop\n", aName.c_str(),
				(int)aMusic.mLayers.size(), aClips, aPlays, aLooping);
			if (aPlays == 0)
				aFailed++;
		}
		catch (const std::exception& e)
		{
			printf("FAIL sound or music %s: %s\n", aName.c_str(), e.what());
			aFailed++;
		}
	}
	printf("%d sounds, %d music files, %d missing sound files\n", (int)aBank.mGroups.size(), aMusicFiles,
		aSoundSystem.mMissing);
	aFailed += aSoundSystem.mMissing;
	anApp->setSoundSystem(nullptr);
	return aFailed == 0 && aFiles > 0 ? 0 : 1;
}
