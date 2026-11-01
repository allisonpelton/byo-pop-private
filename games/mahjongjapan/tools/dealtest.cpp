// A check of the layouts and the deal (no window, no sound): every LAYOUTS resource of the user's Resource.dll is
// loaded and dealt, and the kinds dealt are checked to pair up. Usage: mahjongjapan_dealtest <folder with
// Resource.dll> [quad percent] [seed], or <folder> --solve <layout id> [seed quad percent]
// (below)

#include "Deal.h"
#include "Globals.h"
#include "Host.h"
#include "Layout.h"
#include "PeResources.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>
#include <algorithm>

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

// --solve <layout id> [seed quad percent]: the clicks that win an escape layout (its kinds are its own) by taking its key tiles (0x2b),
// which ends the stage (0x40d8b0), for scripted test runs: one
// "x1 y1 x2 y2" line per pair, in screen coordinates (the board at 0,0x42, placed as Board::Measure, 0x40b690, does).
// Wild and key tiles are taken only as plain pairs.
static std::vector<Tile*> gTiles;
static std::set<std::string> gDead;
static std::vector<std::pair<Tile*, Tile*>> gMoves;

static bool SolveFrom(Layout& theLayout, int theLeft)
{
	if (!gMoves.empty() && gMoves.back().first->mKind == 0x2b)
		return true;
	std::string aKey;
	for (Tile* aTile : gTiles)
		aKey += aTile->mTaken ? '1' : '0';
	if (gDead.count(aKey))
		return false;
	std::vector<Tile*> aFree;
	for (Tile* aTile : gTiles)
		if (aTile->mTaken == 0 && aTile->mFree)
			aFree.push_back(aTile);
	for (size_t i = 0; i < aFree.size(); i++)
		for (size_t j = i + 1; j < aFree.size(); j++)
		{
			Tile* a = aFree[i];
			Tile* b = aFree[j];
			if (!TilesMatch(a->mKind, b->mKind))
			{
				if (!TilesMatch(b->mKind, a->mKind))
					continue;
				std::swap(a, b);
			}
			a->mTaken = b->mTaken = 1;
			theLayout.UpdateFree();
			gMoves.push_back({a, b});
			if (SolveFrom(theLayout, theLeft - 2))
				return true;
			gMoves.pop_back();
			a->mTaken = b->mTaken = 0;
			theLayout.UpdateFree();
		}
	gDead.insert(aKey);
	return false;
}

static int Solve(int theId, bool theDeal, unsigned int theSeed, int thePercent)
{
	Layout aLayout;
	jb::Stream aSeeding;
	if (!aLayout.Load(theId))
	{
		fprintf(stderr, "layout %d does not load\n", theId);
		return 2;
	}
	int aLeft = 800, aRight = 0, aTop = 0x1c4, aBottom = 0;
	for (Tile* aTile = aLayout.First(); aTile != nullptr; aTile = aLayout.Next())
	{
		int x = aTile->mX * 0x15 + -6 + aTile->mZ * 6;
		int y = aTile->mY * 0x1a + aTile->mZ * -6;
		aTile->mRect.left = x;
		aTile->mRect.right = x + 0x30;
		aTile->mRect.top = y + 6;
		aTile->mRect.bottom = y + 0x40;
		aLeft = std::min(aLeft, x);
		aRight = std::max(aRight, x + 0x30);
		aTop = std::min(aTop, y + 6);
		aBottom = std::max(aBottom, y + 0x40);
		gTiles.push_back(aTile);
		if (aTile->mKind > 0x2b)
			fprintf(stderr, "tile kind 0x%x: taken as a plain pair\n", aTile->mKind);
	}
	int aBlank = 0;
	for (Tile* aTile : gTiles)
		if (aTile->mKind == 0)
			aBlank++;
	fprintf(stderr, "%d tiles, %d without a kind\n", (int)gTiles.size(), aBlank);
	int anOffX = ((aLeft - aRight) + 800) / 2 - aLeft;
	int anOffY = (((aTop - aBottom) + 0x1c4) / 2 - aTop) + -5;
	aLayout.UpdateFree();
	if (theDeal)
	{
		// As Board::Start (0x40ae60) deals a stage's blank tiles: with the stage's seed, from the player's file
		jb::CrtSrand(theSeed);
		Deal aDeal;
		aDeal.Start(&aLayout, thePercent, 0, false);
		aLayout.UpdateFree();
	}
	if (!SolveFrom(aLayout, (int)gTiles.size()))
	{
		printf("unsolved\n");
		return 1;
	}
	for (Tile* aTile : gTiles)
		aTile->mTaken = 0;
	// A point of each tile that the hit test gives to it, with the tiles taken before it gone
	auto aPoint = [&](Tile* theTile, int& theX, int& theY) {
		for (int y = theTile->mRect.top + 26; y < theTile->mRect.bottom; y += 3)
			for (int x = theTile->mRect.left + 24; x < theTile->mRect.right; x += 3)
			{
				int z = -1, tx, ty;
				if (aLayout.HitTest(x, y, &z, &tx, &ty) == theTile)
				{
					theX = x + anOffX;
					theY = y + anOffY + 0x42;
					return;
				}
			}
		for (int y = theTile->mRect.top; y < theTile->mRect.bottom; y += 2)
			for (int x = theTile->mRect.left; x < theTile->mRect.right; x += 2)
			{
				int z = -1, tx, ty;
				if (aLayout.HitTest(x, y, &z, &tx, &ty) == theTile)
				{
					theX = x + anOffX;
					theY = y + anOffY + 0x42;
					return;
				}
			}
		theX = theY = -1;
	};
	for (auto& aMove : gMoves)
	{
		int x1, y1, x2, y2;
		aPoint(aMove.first, x1, y1);
		aPoint(aMove.second, x2, y2);
		printf("%d %d %d %d\n", x1, y1, x2, y2);
		aMove.first->mTaken = aMove.second->mTaken = 1;
	}
	return 0;
}

// The framework links SDL2main, whose WinMain calls this
extern "C" int SDL_main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: mahjongjapan_dealtest <folder> [quad percent] [seed]\n");
		return 2;
	}
	std::string aFolder = argv[1];
	bool aSolve = argc > 3 && std::string(argv[2]) == "--solve";
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
	if (aSolve)
		return Solve(atoi(argv[3]), argc > 5, argc > 4 ? (unsigned int)strtoul(argv[4], nullptr, 10) : 0,
			argc > 5 ? atoi(argv[5]) : 0);

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
