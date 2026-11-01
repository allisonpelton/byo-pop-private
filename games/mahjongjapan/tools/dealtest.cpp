// A check of the layouts and the deal (no window, no sound): every LAYOUTS resource of the user's Resource.dll is
// loaded and dealt, and the kinds dealt are checked to pair up. Usage: mahjongjapan_dealtest <folder with
// Resource.dll> [quad percent] [seed]

#include "Deal.h"
#include "Globals.h"
#include "Host.h"
#include "Layout.h"
#include "PeResources.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>

using namespace mahjongjapan;

namespace
{

class TestHost : public jb::Host
{
public:
	PeResources mExe, mDll;

	uint32_t TickCount() override
	{
		using namespace std::chrono;
		return (uint32_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
	}
	const uint8_t* FindResource(jb::Module theModule, const char* theTypeName, int theTypeId, int theId,
		uint32_t* theSize) override
	{
		const PeResources& aFile = theModule == jb::Module::Dll ? mDll : mExe;
		if (theTypeName != nullptr)
			return aFile.Find(PeResources::Key(theTypeName), PeResources::Key((uint32_t)theId), theSize);
		return aFile.Find(PeResources::Key((uint32_t)theTypeId), PeResources::Key((uint32_t)theId), theSize);
	}
	bool LoadString(jb::Module theModule, int theId, std::string& theText) override
	{
		return (theModule == jb::Module::Dll ? mDll : mExe).LoadString((uint32_t)theId, theText);
	}
	bool RegReadInt(const char*, const char*, bool, int&) override { return false; }
	bool RegReadString(const char*, const char*, bool, std::string&) override { return false; }
	bool RegWriteInt(const char*, const char*, int) override { return false; }
	bool RegWriteString(const char*, const char*, const char*) override { return false; }
	bool RegDeleteValue(const char*, const char*) override { return false; }
	bool RegDeleteKey(const char*) override { return false; }
	std::string CommandLine() override { return ""; }
	std::string FolderPath(int) override { return ""; }
	std::string ExeFolder() override { return ""; }
	void OpenUrl(const char*) override {}
	int MessageBox(const char*, const char*, unsigned int) override { return 1; }
	void Log(const char* theText) override { fprintf(stderr, "%s\n", theText); }
	int InputFlags() override { return 0; }
	void GetMousePos(int& theX, int& theY, bool& theOutside) override { theX = theY = 0; theOutside = true; }
	void SetCursor(int) override {}
	void CaptureMouse(bool) override {}
	bool GetClipboard(std::string&) override { return false; }
	bool SetClipboard(const char*) override { return false; }
	void Sleep(uint32_t) override {}
	void SetFullScreen(bool) override {}
	void Present(const uint8_t*, int) override {}
};

} // namespace

// The framework links SDL2main, whose WinMain calls this
extern "C" int SDL_main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: mahjongjapan_dealtest <folder> [quad percent] [seed]\n");
		return 2;
	}
	std::string aFolder = argv[1];
	int aPercent = argc > 2 ? atoi(argv[2]) : 50;
	unsigned int aSeed = argc > 3 ? (unsigned int)atoi(argv[3]) : 1;
	TestHost aHost;
	if (!aHost.mDll.Open(aFolder + "/Resource.dll"))
	{
		fprintf(stderr, "no Resource.dll in %s\n", aFolder.c_str());
		return 2;
	}
	aHost.mExe.Open(aFolder + "/MahjongEscapeAJ.exe");
	jb::SetHost(&aHost);

	int aLayouts = 0;
	int aFailed = 0;
	uint32_t aSlowest = 0;
	for (const PeResources::Key& aKey : aHost.mDll.Names(PeResources::Key("LAYOUTS")))
	{
		if (aKey.mIsName)
			continue;
		Layout aLayout;
		jb::Stream aSeeding;  // the stream seeds rand() with the clock; the test seeds it after
		if (!aLayout.Load((int)aKey.mId))
		{
			printf("layout %u: does not load\n", aKey.mId);
			aFailed++;
			continue;
		}
		jb::CrtSrand(aSeed);
		aLayout.UpdateFree();
		Deal aDeal;
		uint32_t aStart = aHost.TickCount();
		aDeal.Start(&aLayout, aPercent, 0, false);
		uint32_t aTime = aHost.TickCount() - aStart;
		if (aSlowest < aTime)
			aSlowest = aTime;
		// Every tile has a kind, and each kind (the seasons together, the flowers together) is dealt an even number
		int aCounts[0x30] = {};
		int aBlank = 0;
		int aTiles = 0;
		for (Tile* aTile = aLayout.First(); aTile != nullptr; aTile = aLayout.Next())
		{
			aTiles++;
			int aKind = aTile->mKind;
			if (aKind == 0 || aKind >= 0x30)
			{
				aBlank++;
				continue;
			}
			if (0x22 < aKind && aKind < 0x27)
				aKind = 0x23;
			else if (0x26 < aKind && aKind < 0x2b)
				aKind = 0x27;
			aCounts[aKind]++;
		}
		int anOdd = 0;
		for (int k = 1; k < 0x2b; k++)
			if (aCounts[k] % 2 != 0)
				anOdd++;
		bool aBad = aBlank != 0 || anOdd != 0;
		if (aBad)
			aFailed++;
		printf("layout %u: %d tiles%s, %u ms%s%s\n", aKey.mId, aTiles, aLayout.mHasCounts ? " (kinds of its own)" : "",
			aTime, aBlank ? " BLANK" : "", anOdd ? " ODD" : "");
		aLayouts++;
	}
	printf("%d layouts, %d failed, slowest %u ms\n", aLayouts, aFailed, aSlowest);
	return aFailed == 0 ? 0 : 1;
}
