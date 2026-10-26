// The engine's check: opens WHArchive.saf from a game folder (assets/wordharmony by default) through the engine's
// resource manager and parses every XML file in it with the engine's parser, counting the elements; then reads each as
// a screen layout and as a particle file, and runs the particle systems until they are done. It opens no window and
// plays no sound.
//   wordharmony_enginetest [<game folder>]

#include "App.h"
#include "Exception.h"
#include "Font.h"
#include "Graphics.h"
#include "Host.h"
#include "Particles.h"
#include "ScreenLayout.h"
#include "Xml.h"
#include <chrono>
#include <cstdio>
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
	return aFailed == 0 && aFiles > 0 ? 0 : 1;
}
