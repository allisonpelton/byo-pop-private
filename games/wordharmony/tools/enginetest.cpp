// The engine's check: opens WHArchive.saf from a game folder (assets/wordharmony by default) through the engine's
// resource manager and parses every XML file in it with the engine's parser, counting the elements. It opens no window
// and plays no sound.
//   wordharmony_enginetest [<game folder>]

#include "App.h"
#include "Exception.h"
#include "Host.h"
#include "Xml.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>

using namespace engine;

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
	Image* createImage(const std::vector<uint8_t>&, const std::vector<uint8_t>*) override { return nullptr; }
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
	return aFailed == 0 && aFiles > 0 ? 0 : 1;
}
