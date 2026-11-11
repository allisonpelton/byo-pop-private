// A check of the layouts and the deal (no window, no sound): every LAYOUTS resource of the user's Resource.dll is
// loaded and dealt, and the kinds dealt are checked to pair up. Usage: mahjongchina_dealtest <folder with
// Resource.dll> [quad percent] [seed], or <folder> --solve <layout id> [seed quad percent],
// or <folder> --stage <escape stage 1-52>
// (below)

#include "Bot.h"
#include "Deal.h"
#include "Globals.h"
#include "Host.h"
#include "Layout.h"
#include "PeResources.h"
#include "TextResource.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

using namespace mahjongchina;

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
		const PeResources& aFile = theModule == jb::Module::DLL ? mDll : mExe;
		if (theTypeName != nullptr)
			return aFile.Find(PeResources::Key(theTypeName), PeResources::Key((uint32_t)theId), theSize);
		return aFile.Find(PeResources::Key((uint32_t)theTypeId), PeResources::Key((uint32_t)theId), theSize);
	}
	bool LoadString(jb::Module theModule, int theId, std::string& theText) override
	{
		return (theModule == jb::Module::DLL ? mDll : mExe).LoadString((uint32_t)theId, theText);
	}
	bool RegReadInt(const char*, const char*, bool, int&) override { return false; }
	bool RegWriteInt(const char*, const char*, int) override { return false; }
	std::string CommandLine() override { return ""; }
	std::string FolderPath(int) override { return ""; }
	std::string ExeFolder() override { return ""; }
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

// --solve <layout id> [seed quad percent]: the clicks that win an escape layout (its kinds are its own) by taking
// its key tiles (0x2b), which ends the stage (0x40d8b0), for scripted test runs: one "x1 y1 x2 y2" line per move, in
// screen coordinates (the board at 0,0x42, placed as Board::Measure, 0x40b690, does). A move is a pair of free
// tiles, or a free special tile and a tile it can take (Board::CanSelect 0x40c350, CanPair 0x40c3b0, Take 0x40c560):
// a key (0x2c) takes a lock (0x2d) that can be seen; the wild tile 0x2e takes every tile of the kind of an ordinary
// tile that can be seen (TakeWild 0x40cc10); 0x2f takes such a tile and becomes its kind (TakeLinked 0x40cd80).
// The search tries the moves nearest the key tiles first.
struct Move
{
	Tile* mFirst;
	Tile* mSecond;
	std::vector<Tile*> mTaken;  // every tile the move takes
	Tile* mChanged = nullptr;   // 0x2f, which took the second's kind
};

static std::vector<Tile*> gTiles;
static std::set<std::string> gDead;
static std::vector<Move> gMoves;
// The search gives up after gLimit positions and starts again with equally ranked moves in another order
static long gNodes = 0;
static long gLimit = 400000;
static int gTries = 60;  // MJ_SOLVE_LIMIT and MJ_SOLVE_TRIES set both (for a stage that needs many short restarts)
static bool gGaveUp = false;
static std::mt19937 gOrder;
static bool gShuffle = false;
static bool gClassic = false;  // MJ_CLASSIC: a classic game is won when no tile is left
static bool gDeadEnd = false;  // MJ_DEADEND (with MJ_CLASSIC): the first play that leaves tiles and no pair, for the no-moves box

static void Apply(Layout& theLayout, Move& theMove)
{
	theMove.mTaken.clear();
	theMove.mChanged = nullptr;
	int aKind = theMove.mFirst->mKind;
	if (aKind == 0x2e)
	{
		theMove.mTaken.push_back(theMove.mFirst);
		for (Tile* aTile : gTiles)
			if (aTile->mTaken == 0 && KindsMatch(aTile->mKind, theMove.mSecond->mKind))
				theMove.mTaken.push_back(aTile);
	}
	else if (aKind == 0x2f)
	{
		theMove.mTaken.push_back(theMove.mSecond);
		theMove.mChanged = theMove.mFirst;
		theMove.mFirst->mKind = theMove.mSecond->mKind;
	}
	else
	{
		theMove.mTaken.push_back(theMove.mFirst);
		theMove.mTaken.push_back(theMove.mSecond);
	}
	for (Tile* aTile : theMove.mTaken)
		aTile->mTaken = 1;
	theLayout.UpdateFree();
}

static void Undo(Layout& theLayout, Move& theMove)
{
	for (Tile* aTile : theMove.mTaken)
		aTile->mTaken = 0;
	if (theMove.mChanged != nullptr)
		theMove.mChanged->mKind = 0x2f;
	theLayout.UpdateFree();
}

// A point of theTile that the hit test (Layout::HitTest, 0x414750) gives to it, in board coordinates; a tile can be
// visible (IsVisible) by a sliver the hit test gives to a tile drawn later
static bool ClickPoint(Layout& theLayout, Tile* theTile, int& theX, int& theY)
{
	for (int y = theTile->mRect.top + 26; y < theTile->mRect.bottom; y += 3)
		for (int x = theTile->mRect.left + 24; x < theTile->mRect.right; x += 3)
		{
			int z = -1, tx, ty;
			if (theLayout.HitTest(x, y, &z, &tx, &ty) == theTile)
			{
				theX = x;
				theY = y;
				return true;
			}
		}
	for (int y = theTile->mRect.top; y < theTile->mRect.bottom; y++)
		for (int x = theTile->mRect.left; x < theTile->mRect.right; x++)
		{
			int z = -1, tx, ty;
			if (theLayout.HitTest(x, y, &z, &tx, &ty) == theTile)
			{
				theX = x;
				theY = y;
				return true;
			}
		}
	return false;
}

// A free tile can lie under a tile two levels up with nothing between them (Layout::IsFree, 0x414c20, looks one
// level up), and then no click reaches it
static bool CanClick(Layout& theLayout, Tile* theTile)
{
	bool aHigher = false;
	for (Tile* aTile : gTiles)
		if (aTile->mTaken == 0 && theTile->mZ + 1 < aTile->mZ && aTile->mRect.left < theTile->mRect.right &&
			theTile->mRect.left < aTile->mRect.right && aTile->mRect.top < theTile->mRect.bottom &&
			theTile->mRect.top < aTile->mRect.bottom)
			aHigher = true;
	int x, y;
	return !aHigher || ClickPoint(theLayout, theTile, x, y);
}

static bool AllTaken()
{
	for (Tile* aTile : gTiles)
		if (aTile->mTaken == 0)
			return false;
	return true;
}

static bool SolveFrom(Layout& theLayout)
{
	if (gDeadEnd && AllTaken())
		return false;
	if (gClassic ? AllTaken() : (!gMoves.empty() && gMoves.back().mFirst->mKind == 0x2b))
		return true;
	std::string aKey;
	for (Tile* aTile : gTiles)
		aKey += aTile->mTaken ? '\x7f' : (char)aTile->mKind;
	if (gDead.count(aKey))
		return false;
	if (gLimit < ++gNodes)
	{
		gGaveUp = true;
		return false;
	}
	std::vector<Tile*> aKeys;
	std::vector<Tile*> aFree;
	for (Tile* aTile : gTiles)
	{
		if (aTile->mTaken != 0)
			continue;
		if (aTile->mKind == 0x2b)
			aKeys.push_back(aTile);
		if (aTile->mFree && CanClick(theLayout, aTile))
			aFree.push_back(aTile);
	}
	std::vector<std::pair<int, Move>> aMoves;
	auto aNear = [&](Tile* theTile) {
		int aBest = 1000;
		for (Tile* aKeyTile : aKeys)
			aBest = std::min(aBest, abs(aKeyTile->mX - theTile->mX) + abs(aKeyTile->mY - theTile->mY) +
				abs(aKeyTile->mZ - theTile->mZ));
		return aBest;
	};
	auto anAdd = [&](Tile* theFirst, Tile* theSecond) {
		Move aMove;
		aMove.mFirst = theFirst;
		aMove.mSecond = theSecond;
		int aRank = std::min(aNear(theFirst), aNear(theSecond));
		if (theFirst->mKind == 0x2b)
			aRank = -1;
		aMoves.push_back({aRank, aMove});
	};
	for (size_t i = 0; i < aFree.size(); i++)
		for (size_t j = i + 1; j < aFree.size(); j++)
		{
			Tile* a = aFree[i];
			Tile* b = aFree[j];
			if (0x2b < a->mKind || 0x2b < b->mKind || !TilesMatch(a->mKind, b->mKind))
				continue;
			anAdd(a, b);
		}
	for (Tile* a : aFree)
	{
		if (a->mKind < 0x2c || a->mKind == 0x2d)
			continue;
		for (Tile* b : gTiles)
		{
			int bx, by;
			if (b->mTaken != 0 || b == a || !theLayout.IsVisible(b) || !ClickPoint(theLayout, b, bx, by))
				continue;
			if (a->mKind == 0x2c ? b->mKind == 0x2d : b->mKind < 0x2b)
				anAdd(a, b);
		}
	}
	if (gDeadEnd && aMoves.empty())
		return true;
	if (gShuffle)
		std::shuffle(aMoves.begin(), aMoves.end(), gOrder);
	std::stable_sort(aMoves.begin(), aMoves.end(),
		[](const std::pair<int, Move>& x, const std::pair<int, Move>& y) { return x.first < y.first; });
	for (auto& aRanked : aMoves)
	{
		gMoves.push_back(aRanked.second);
		Apply(theLayout, gMoves.back());
		if (SolveFrom(theLayout))
			return true;
		Undo(theLayout, gMoves.back());
		gMoves.pop_back();
		if (gGaveUp)
			return false;
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
	}
	int aBlank = 0;
	for (Tile* aTile : gTiles)
		if (aTile->mKind == 0)
			aBlank++;
	fprintf(stderr, "%d tiles, %d without a kind\n", (int)gTiles.size(), aBlank);
	if (getenv("MJ_SOLVE_KINDS"))
	{
		fprintf(stderr, "kinds set by the layout:");
		for (Tile* aTile : gTiles)
			if (aTile->mKind != 0)
				fprintf(stderr, " %#x", aTile->mKind);
		fprintf(stderr, "\n");
	}
	int anOffX = ((aLeft - aRight) + 800) / 2 - aLeft;
	int anOffY = ((aTop - aBottom) + 0x1c4) / 2 - aTop;   // Board::Measure's, China's without the -5
	aLayout.UpdateFree();
	if (const char* aKindsFile = getenv("MJ_KINDS"))
	{
		// MJ_KINDS=<file>: the kinds of a deal that the original made (read from its saved game), one tile a line as
		// "x y z kind", for solving a classic game whose deal came from the clock; with MJ_CLASSIC=1
		FILE* aFile = fopen(aKindsFile, "r");
		size_t aCount = 0;
		int aX = 0, aY = 0, aZ = 0, aKind = 0;
		while (aFile != nullptr && fscanf(aFile, "%d %d %d %d", &aX, &aY, &aZ, &aKind) == 4)
			for (Tile* aTile : gTiles)
				if (aTile->mX == aX && aTile->mY == aY && aTile->mZ == aZ)
				{
					aTile->mKind = aKind;
					aCount++;
				}
		if (aFile != nullptr)
			fclose(aFile);
		fprintf(stderr, "kinds read for %d of %d tiles\n", (int)aCount, (int)gTiles.size());
		aLayout.UpdateFree();
	}
	gClassic = getenv("MJ_CLASSIC") != nullptr;
	gDeadEnd = gClassic && getenv("MJ_DEADEND") != nullptr;
	if (theDeal)
	{
		// As Board::Start (0x40ae60) deals a stage's blank tiles: with the stage's seed, from the player's file
		jb::CrtSrand(theSeed);
		Deal aDeal;
		aDeal.Start(&aLayout, thePercent, 0, false);
		aLayout.UpdateFree();
	}
	if (getenv("BOTTEST") != nullptr)
	{
		// the port's bot on the dealt board: its moves, positions tried and seconds
		for (Tile* aTile : gTiles)
			if (aTile->mKind > 0x2a)
				fprintf(stderr, "special kind 0x%x at %d,%d,%d free %d%s", aTile->mKind, aTile->mX, aTile->mY, aTile->mZ,
					aTile->mFree, "; ");
		if (getenv("BOTTEST")[0] == 'v')
			for (Tile* aTile : gTiles)
				fprintf(stderr, "T %d %d %d %d%s", aTile->mZ, aTile->mY, aTile->mX, aTile->mKind, "\n");
		auto aStart = std::chrono::steady_clock::now();
		Bot aBot;
		long aNodes = 0;
		int aMoves = aBot.SolveLayout(aLayout, &aNodes);
		printf("bot: %d moves, %ld positions, %.2f s\n", aMoves, aNodes,
			std::chrono::duration<double>(std::chrono::steady_clock::now() - aStart).count());
		return aMoves < 0 ? 1 : 0;
	}
	std::vector<int> aKinds;
	for (Tile* aTile : gTiles)
		aKinds.push_back(aTile->mKind);
	bool aSolved = SolveFrom(aLayout);
	for (int aTry = 1; !aSolved && gGaveUp && aTry < gTries; aTry++)
	{
		gDead.clear();
		gNodes = 0;
		gGaveUp = false;
		gShuffle = true;
		gOrder.seed(aTry);
		aSolved = SolveFrom(aLayout);
	}
	if (!aSolved)
	{
		printf("unsolved\n");
		return 1;
	}
	for (size_t i = 0; i < gTiles.size(); i++)
	{
		gTiles[i]->mTaken = 0;
		gTiles[i]->mKind = aKinds[i];
	}
	// A point of each tile that the hit test gives to it, with the tiles taken before it gone
	int aMoveCount = 0;
	auto aPoint = [&](Tile* theTile, int& theX, int& theY) {
		if (ClickPoint(aLayout, theTile, theX, theY))
		{
			theX = theX + anOffX;
			theY = theY + anOffY + 0x42;
		}
		else
		{
			theX = theY = -1;
			fprintf(stderr, "unclickable %d: tile %d,%d,%d kind 0x%x free %d taken %d\n", aMoveCount, theTile->mX,
				theTile->mY, theTile->mZ, theTile->mKind, theTile->mFree, theTile->mTaken);
		}
	};
	// The tips that come after a move (Board::UpdateMoves, 0x40d560, and CheckStuck, 0x40dfd0): both key tiles
	// free (1), a flower or a season free (7), no pair free (5); "tip <id> <moves before it>" on stderr, for the
	// script to click the tip away
	bool aKeysTipped = false;
	bool aFlowersTipped = false;
	bool aStuckTipped = false;
	auto aTips = [&]() {
		if (!aKeysTipped && aLayout.CountFree2b() == 2)
		{
			aKeysTipped = true;
			fprintf(stderr, "tip 1 %d\n", aMoveCount);
		}
		if (!aFlowersTipped && aLayout.HasFreeFlowers())
		{
			aFlowersTipped = true;
			fprintf(stderr, "tip 7 %d\n", aMoveCount);
		}
		// No free pair left but a special move (Board::CheckStuck, 0x40dfd0): tip 5, once a level
		int aPairs, aLeft, aKinds;
		aLayout.CountMoves(&aPairs, &aLeft, &aKinds);
		if (!aStuckTipped && aPairs < 1 && aLeft != 0 && 0 < aMoveCount)
		{
			aStuckTipped = true;
			fprintf(stderr, "tip 5 %d\n", aMoveCount);
		}
	};
	aLayout.UpdateFree();
	aTips();
	for (auto& aMove : gMoves)
	{
		int x1, y1, x2, y2;
		aPoint(aMove.mFirst, x1, y1);
		aPoint(aMove.mSecond, x2, y2);
		printf("%d %d %d %d\n", x1, y1, x2, y2);
		fprintf(stderr, "pair %d: tile %d,%d,%d kind 0x%x with tile %d,%d,%d kind 0x%x\n", aMoveCount, aMove.mFirst->mX,
			aMove.mFirst->mY, aMove.mFirst->mZ, aMove.mFirst->mKind, aMove.mSecond->mX, aMove.mSecond->mY,
			aMove.mSecond->mZ, aMove.mSecond->mKind);
		// A special tile's flights take longer, and the board ignores clicks while they last (Board::OnLButtonDown,
		// 0x40c030): "special <moves before it> <kind>" on stderr, for the script to wait
		if (0x2b < aMove.mFirst->mKind)
			fprintf(stderr, "special %d 0x%x\n", aMoveCount, aMove.mFirst->mKind);
		Apply(aLayout, aMove);
		aMoveCount++;
		aTips();
	}
	return 0;
}

// The framework links SDL2main, whose WinMain calls this
extern "C" int SDL_main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: mahjongchina_dealtest <folder> [quad percent] [seed]\n");
		return 2;
	}
	std::string aFolder = argv[1];
	bool aSolve = argc > 3 && std::string(argv[2]) == "--solve";
	int aPercent = argc > 2 ? atoi(argv[2]) : 50;
	unsigned int aSeed = argc > 3 ? (unsigned int)atoi(argv[3]) : 1;
	if (const char* aLimit = getenv("MJ_SOLVE_LIMIT"))
		gLimit = atol(aLimit);
	if (const char* aTries = getenv("MJ_SOLVE_TRIES"))
		gTries = atoi(aTries);
	TestHost aHost;
	if (!aHost.mDll.Open(aFolder + "/Resource.dll"))
	{
		fprintf(stderr, "no Resource.dll in %s\n", aFolder.c_str());
		return 2;
	}
	aHost.mExe.Open(aFolder + "/MahjongEscapeAC.exe");
	jb::SetHost(&aHost);
	if (argc > 3 && std::string(argv[2]) == "--stage")
	{
		// the escape's layouts (MapLayer.cpp's LEVEL_BACKGROUNDS, China's 0x51c280)
		static const int kStageLayouts[52] = {
			385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397,
			398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410,
			411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423,
			424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436};
		int aStage = atoi(argv[3]);
		jb::TextResource aText;
		if (aStage < 1 || 52 < aStage || !aText.Load(0x17f, 0, jb::Module::DLL))
			return 2;
		std::string aLine;
		int aPos = 0;
		int aCount = 0;
		while (aText.ReadLine(aPos, aLine))
		{
			if (aLine.empty())
				continue;
			if (++aCount < aStage)
				continue;
			int aTab = 0;
			int aPercent = (int)strtol(jb::Tokenize(aLine, 9, aTab).c_str(), nullptr, 10);
			unsigned int aStageSeed = (unsigned int)strtoul(jb::Tokenize(aLine, 9, aTab).c_str(), nullptr, 10);
			fprintf(stderr, "stage %d: layout %d, seed %u, fours %d%%\n", aStage, kStageLayouts[aStage - 1], aStageSeed,
				aPercent);
			return Solve(kStageLayouts[aStage - 1], true, aStageSeed, aPercent);
		}
		return 2;
	}
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
		if (getenv("MJ_FLOATING") != nullptr)
		{
			// a tile with nothing under it on the level below (Layout::IsFree looks one level up only, so a free tile can
			// lie under a tile two levels up that no click then reaches the first one through)
			std::vector<Tile*> anAll;
			for (Tile* aTile = aLayout.First(); aTile != nullptr; aTile = aLayout.Next())
				anAll.push_back(aTile);
			int aFloating = 0;
			for (Tile* aUpper : anAll)
			{
				if (aUpper->mZ < 2)
					continue;
				bool aSupported = false;
				for (Tile* aLower : anAll)
					if (aLower->mZ == aUpper->mZ - 1 && abs(aLower->mX - aUpper->mX) <= 1 && abs(aLower->mY - aUpper->mY) <= 1)
						aSupported = true;
				if (!aSupported)
					aFloating++;
			}
			Bot aBot;
			aLayout.UpdateFree();
			int anUnclickable = aBot.CountUnclickable(aLayout);
			if (aFloating != 0 || anUnclickable != 0)
				printf("layout %u: %d of %d tiles float, %d free at the start but out of reach\n", aKey.mId, aFloating,
					(int)anAll.size(), anUnclickable);
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
