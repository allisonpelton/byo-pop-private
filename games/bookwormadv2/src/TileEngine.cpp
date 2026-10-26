#include "TileEngine.h"

#include <cstring>

#include "Board.h"
#include "Common.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "LuaBind.h"
#include "LuaWidgetTpl.h"
#include "Profile.h"
#include "Tile.h"

using namespace Sexy;
using namespace LuaLib;

TileEngine* Sexy::gTileEngine = nullptr;

// The words a 4x4 grid must not spell (0x818978-0x818df4, made by 0x795240 in this order): game data, read from the
// exe by address
static const unsigned int kAvoidedWordAddrs[] = {
    GTA(0x7b0df0), GTA(0x7b0de8), GTA(0x7b0de0), GTA(0x7b0ddc), GTA(0x7b0dd4), GTA(0x7b0dcc), GTA(0x7b0dc4),
    GTA(0x7b0dbc), GTA(0x7b0db4), GTA(0x7b0dac), GTA(0x7b0da4), GTA(0x7b0da0), GTA(0x7b0d98), GTA(0x7b0d90),
    GTA(0x7b0d8c), GTA(0x7b0d84), GTA(0x7b0d7c), GTA(0x7b0d74), GTA(0x7b0d6c), GTA(0x7b0d68), GTA(0x7b0d60),
    GTA(0x7b0d58), GTA(0x7b0d50), GTA(0x7b0d48), GTA(0x7b0d44), GTA(0x7b0d3c), GTA(0x7b0d34), GTA(0x7b0d2c),
    GTA(0x7b0d28), GTA(0x7b0d20), GTA(0x7b0d1c), GTA(0x7b0d14), GTA(0x7b0d10), GTA(0x7b0d08), GTA(0x7b0d00),
    GTA(0x7b0cfc), GTA(0x7b0cf4), GTA(0x7b0cec), GTA(0x7b0ce4), GTA(0x7b0cdc), GTA(0x7b0cd4)};

// 0x7a8b98: the most copies of each letter 'A' to 'Z' that the board may hold
static const int kMaxLetters[26] = {4, 3, 3, 4, 4, 3, 4, 3, 4, 2, 2, 4, 3, 4, 3, 4, 2, 4, 4, 4, 4, 3, 3, 2, 3, 2};

// 0x7a8b28: the weight of each letter 'A' to 'Z' when a letter is chosen (English letter frequencies)
static const int kLetterWeights[26] = {9420, 2200, 2000, 4200, 12700, 2000, 2500, 2000, 7330, 700, 700, 4000, 2300,
                                       6000, 7330, 2300, 600,  6900,  5600, 6000, 4400, 1600, 1500, 500, 2100, 500};

static bool IsVowelLetter(char c)
{
	return c == 'a' || c == 'A' || c == 'e' || c == 'E' || c == 'i' || c == 'I' || c == 'o' || c == 'O' || c == 'u' ||
	       c == 'U' || c == 'y' || c == 'Y';
}

// 0x49d460: a letter as a string of one character (a script's letters are strings)
static void PushLetter(char theLetter)
{
	char aString[2] = {theLetter, 0};
	lua_pushstring(gLuaSupport->L, aString);
}

// 0x49c5f0: gTileEngine:TileXYChanged(tile, x, y), the tile engine's own call
static void TellXYChanged(TileEngine* theEngine, Tile* theTile)
{
	lua_State* L = gLuaSupport->L;
	Tile::PushTile(theTile);
	lua_pushint(L, theTile->mX);
	lua_pushint(L, theTile->mY);
	theEngine->CallLinkedMethod("TileXYChanged", 3);
}

TileEngine::TileEngine()
{
	RegisterLua();
	gTileEngine = this;
}

TileEngine::~TileEngine()
{
	if (IsLinked())
		CallLinkedMethod("Destructor", 0);
	if (gLuaSupport != nullptr && gLuaSupport->L != nullptr) {
		lua_State* L = gLuaSupport->L;
		static const char* const kGlobals[] = {"TileEngine", "gTileTable", "gTileEngine"};
		for (const char* aName : kGlobals) {
			lua_pushnil(L);
			lua_setglobal(L, aName);
		}
	}
	DeleteAllTiles();
	// Port: the original leaves the C++ global to its owner
	if (gTileEngine == this)
		gTileEngine = nullptr;
}

void TileEngine::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Tile::RegisterLua();  // (its methods take and give tiles)
	Class<TileEngine>("TileEngine", false)
		.Method<&TileEngine::LuaGetRegisteredName>("LuaGetRegisteredName")
		.Method<&TileEngine::CreateTile>("CreateTile")
		.Method<&TileEngine::DeleteTile>("DeleteTile")
		.Method<&TileEngine::CloneTile>("CloneTile")
		.Method<&TileEngine::SlideTile>("SlideTile");
}

void TileEngine::Init(Board* theBoard)
{
	for (unsigned int aVA : kAvoidedWordAddrs) {
		std::string aWord = GT(aVA);
		// Port: the original takes the letter as it is; a text that did not load is left out
		if (aWord.empty() || aWord[0] < 'A' || aWord[0] > 'Z')
			continue;
		mAvoidedWords[aWord[0] - 'A'].push_back(aWord);
	}
	mBoard = theBoard;
	lua_pushstring(gLuaSupport->L, "scripts/TileEngine.lua");
	gLuaSupport->Call("dofile", 1, 0);
}

void TileEngine::DeleteAllTiles()
{
	for (Tile* aTile : mTiles)
		delete aTile;
	mTiles.clear();
}

Tile* TileEngine::CreateTile(bool theUnused, bool theRegister, bool theRandomLetter)
{
	Tile* aTile = new Tile();
	char aLetter = ' ';
	if (theRandomLetter) {
		if (mBoard != nullptr)
			aLetter = GenerateLetter(false);
		else
			aLetter = (char)(Rand() % 26 + 'A');
	}
	aTile->CreateLinkedObject(aTile, theUnused);
	if (theRegister) {
		Tile::PushTile(aTile);
		PushLetter(aLetter);
		CallLinkedMethod("RegisterTile", 2);
	}
	return aTile;
}

Tile* TileEngine::CloneTile(Tile* theTile)
{
	Tile* aTile = new Tile();
	aTile->CreateLinkedObject(aTile, true);
	aTile->CopyState(theTile);
	char aLetter = theTile->GetLetter();
	Tile::PushTile(aTile);
	PushLetter(aLetter);
	CallLinkedMethod("RegisterTile", 2);
	aTile->mX = theTile->mX;
	aTile->mY = theTile->mY;
	// the board keeps the clone (Port: the original does not look for a board)
	if (mBoard != nullptr)
		mBoard->AddClone(aTile);
	return aTile;
}

void TileEngine::DeleteTile(Tile* theTile, bool theUnregister, bool theDelete)
{
	if (theTile == nullptr)
		return;
	if (theTile->mMoving) {
		mMovingCount--;
		theTile->mSlideSpeed = 0.0f;
		theTile->mMoving = false;
		if (mBoard != nullptr)
			mBoard->RehupMouse();
	}
	if (theUnregister) {
		Tile::PushTile(theTile);
		CallLinkedMethod("UnregisterTile", 1);
	}
	if (mBoard != nullptr)
		mBoard->TileDeleted(theTile);
	if (theDelete)
		delete theTile;
	else
		mTiles.insert(theTile);  // deleted at the board's next update (DeleteAllTiles)
}

void TileEngine::SlideTile(Tile* theTile, int theToX, int theToY, float theSpeed, bool theNotify)
{
	CurvedValue* aCurve = theTile->mSlideCurve;
	theTile->mSlideToX = theToX;
	theTile->mSlideToY = theToY;
	theTile->mSlideFromX = theTile->mX;
	theTile->mSlideFromY = theTile->mY;
	theTile->mSlideSpeed = theSpeed;
	aCurve->mT = 0.0;
	aCurve->Step();
	StartMoving(theTile, theNotify);
}

void TileEngine::StartRound(bool theTimedMode)
{
	lua_pushboolean(gLuaSupport->L, theTimedMode);
	CallLinkedMethod("StartRound", 1);
}

void TileEngine::Draw(std::list<Tile*>& theTiles, Graphics* g, bool theMoving, bool theDrawExtraStuff)
{
	lua_State* L = gLuaSupport->L;
	// gTileEngine:Draw(g, extra, tile, x, y, tile, x, y, ...) for the tiles that slide (or not)
	PushGraphics(g);
	if (!gApp->Is3DAccelerated())
		theDrawExtraStuff = false;
	lua_pushboolean(L, theDrawExtraStuff);
	int aCount = 0;
	for (Tile* aTile : theTiles) {
		if (aTile->mMoving == theMoving) {
			aCount++;
			Tile::PushTile(aTile);
			lua_pushnumber(L, aTile->mX);
			lua_pushnumber(L, aTile->mY);
		}
	}
	gLuaSupport->Call("gTileEngine:Draw", aCount * 3 + 2, 0);
	gLuaSupport->ObjectDeleted(g);
}

void TileEngine::DrawTiles(std::list<Tile*>& theTiles, Graphics* g, bool theStillOnly)
{
	if (mBoard == nullptr)
		return;
	if (theStillOnly) {
		Draw(theTiles, g, false, false);
		return;
	}
	bool aAnyMoving = mMovingCount > 0;
	Draw(theTiles, g, false, !aAnyMoving);
	if (aAnyMoving)
		Draw(theTiles, g, true, true);
}

void TileEngine::Update(std::list<Tile*>& theTiles, bool theCallLua)
{
	for (auto anIt = theTiles.begin(); anIt != theTiles.end(); ++anIt)
		(*anIt)->Update();
	if (theCallLua)
		CallLinkedMethod("Update", 0);
}

void TileEngine::ChangeLetters(std::list<Tile*>& theTiles)
{
	for (Tile* aTile : theTiles) {
		char aLetter = GenerateLetter(true);
		Tile::PushTile(aTile);
		PushLetter(aLetter);
		CallLinkedMethod("ChangeLetter", 2);
	}
}

void TileEngine::StartMoving(Tile* theTile, bool theNotify)
{
	if (theTile->mMoving)
		return;
	mMovingCount++;
	theTile->mMoving = true;
	TellXYChanged(this, theTile);
	Tile::PushTile(theTile);
	lua_pushboolean(gLuaSupport->L, theNotify);
	CallLinkedMethod("TileStartedMoving", 2);
}

void TileEngine::StopMoving(Tile* theTile, bool theNotify)
{
	if (!theTile->mMoving)
		return;
	mMovingCount--;
	theTile->mSlideSpeed = 0.0f;
	theTile->mMoving = false;
	if (theNotify) {
		TellXYChanged(this, theTile);
		// (0x49ce90) the second argument is whether it was the last tile that slid
		Tile::PushTile(theTile);
		lua_pushboolean(gLuaSupport->L, mMovingCount <= 0);
		CallLinkedMethod("TileStoppedMoving", 2);
	}
	// Port: the original does not look for a board
	if (mBoard != nullptr)
		mBoard->RehupMouse();
}

void TileEngine::ValidWordSpelled(std::list<Tile*>& theTiles, bool theIsValid)
{
	lua_State* L = gLuaSupport->L;
	lua_pushboolean(L, theIsValid);
	lua_pushint(L, (int)theTiles.size());
	for (Tile* aTile : theTiles)
		Tile::PushTile(aTile);
	gLuaSupport->Call("gTileEngine:ValidWordSpelled", (int)theTiles.size() + 2, 0);
}

bool TileEngine::IsLetterAllowed(const std::vector<std::string>& thePatterns, char theLetter, int theVowels,
                                 int theConsonants, int theTotal)
{
	for (const std::string& aPattern : thePatterns) {
		std::string aCopy = aPattern;
		for (size_t i = 0; i < aCopy.size(); i++) {
			if (aCopy[i] != '*')
				continue;
			aCopy[i] = theLetter;
			// the avoided words that start like the pattern (Port: the original does not check the letter's range)
			int aIdx = aCopy[0] - 'A';
			if (aIdx < 0 || aIdx >= 26)
				continue;
			for (const std::string& aWord : mAvoidedWords[aIdx])
				if (aWord == aCopy)
					return false;
		}
	}
	if (theTotal >= 9) {
		if (theVowels >= 9 && IsVowelLetter(theLetter))
			return false;
		if (theConsonants >= 9 && !IsVowelLetter(theLetter))
			return false;
	}
	return true;
}

char TileEngine::GenerateLetter(bool theReplacing)
{
	Board* aBoard = mBoard;
	int aCounts[26] = {};
	int aTotal = 0;
	int aVowels = 0;
	int aConsonants = 0;
	bool aIs4x4 = aBoard->mGridHeight == 4 && aBoard->mGridWidth == 4;
	char aGrid[16];  // [row][col] of a 4x4 grid: the letter, or '*' for no tile

	for (int aRow = 0; aRow < aBoard->mGridHeight; aRow++) {
		for (int aCol = 0; aCol < aBoard->mGridWidth; aCol++) {
			Tile* aTile = aBoard->GridGetTile(aCol, aRow);
			if (aTile != nullptr) {
				aTotal++;
				char aLetter = aTile->GetLetter();
				// Port: the original counts any character (and would write outside its table for a lowercase one)
				if (aLetter >= 'A' && aLetter <= 'Z')
					aCounts[aLetter - 'A']++;
				if (IsVowelLetter(aLetter))
					aVowels++;
				else
					aConsonants++;
				if (aIs4x4)
					aGrid[aRow * 4 + aCol] = aLetter;
			} else if (aIs4x4)
				aGrid[aRow * 4 + aCol] = '*';
		}
	}

	// the four rows, then the four columns, as strings
	std::vector<std::string> aPatterns;
	if (aIs4x4) {
		auto anAppend = [](std::string& theString, char c) {
			if (c != 0)
				theString += c;
		};
		for (int aRow = 0; aRow < 4; aRow++) {
			std::string aPattern;
			for (int aCol = 0; aCol < 4; aCol++)
				anAppend(aPattern, aGrid[aRow * 4 + aCol]);
			aPatterns.push_back(aPattern);
		}
		for (int aCol = 0; aCol < 4; aCol++) {
			std::string aPattern;
			for (int aRow = 0; aRow < 4; aRow++)
				anAppend(aPattern, aGrid[aRow * 4 + aCol]);
			aPatterns.push_back(aPattern);
		}
	}

	// the letters that may be chosen: below their maximum, and no avoided word in the grid with them
	std::vector<char> aCandidates;
	for (int i = 0; i < 26; i++) {
		char aLetter = (char)('A' + i);
		if (aCounts[i] < kMaxLetters[i] && IsLetterAllowed(aPatterns, aLetter, aVowels, aConsonants, aTotal))
			aCandidates.push_back(aLetter);
	}
	if (aCandidates.empty())
		aCandidates.push_back('A');

	int aWeights[26];
	memcpy(aWeights, kLetterWeights, sizeof(aWeights));
	for (char aCandidate : aCandidates) {
		int aIdx = aCandidate - 'A';
		PushLetter(aCandidate);
		aWeights[aIdx] += CallLinkedMethod("GetExtraLetterFreq", 1).Int();
		int aWeight = aWeights[aIdx];
		if (theReplacing && kMaxLetters[aIdx] != 4 && aCounts[aIdx] == 2)
			aWeights[aIdx] = 0;
		else if (kMaxLetters[aIdx] == 3 && aCounts[aIdx] == 2)
			aWeights[aIdx] = aWeight - aWeight / 3;
		else if (kMaxLetters[aIdx] == 4) {
			if (aCounts[aIdx] == 2)
				aWeights[aIdx] = aWeight - aWeight / 6;
			else if (aCounts[aIdx] == 3)
				aWeights[aIdx] = aWeight - aWeight / 4;
		}
	}

	// a random pick by weight; as in the original, a roll past the last candidate rolls again
	size_t aChosen = 0;
	for (;;) {
		int aSum = 0;
		for (char aCandidate : aCandidates)
			aSum += aWeights[aCandidate - 'A'];
		if (aSum <= 0)
			break;  // Port: the original divides by zero here (or never ends)
		int aRoll = Rand() % aSum;
		int aRunning = 0;
		size_t i = 0;
		for (; i < aCandidates.size(); i++) {
			int aEnd = aWeights[aCandidates[i] - 'A'] + aRunning;
			if (aRoll < aEnd)
				break;
			aRunning = aEnd;
		}
		if (i < aCandidates.size()) {
			aChosen = i;
			break;
		}
	}
	char aLetter = aCandidates[aChosen];

	// the profile counts the letters chosen (in the battles, not the mini-games)
	Profile* aProfile = gApp->mProfile;
	if (aProfile != nullptr && !aBoard->mIsMiniGame) {
		std::string aKey = StrFormat(GT(0x7b0180).c_str(), aLetter);
		int aCount = aProfile->KeyExists(aKey) ? aProfile->GetInt(aKey) : 0;
		aProfile->SetInt(aKey, aCount + 1);
	}
	return aLetter;
}
