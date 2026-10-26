#include "BattleEngine.h"

#include "Board.h"
#include "GameApp.h"
#include "GameText.h"
#include "Lex.h"
#include "LuaBind.h"
#include "Tile.h"
#include "TileEngine.h"

using namespace Sexy;
using namespace LuaLib;

BattleEngine* Sexy::gBattleEngine = nullptr;

// The i-th result of a call into Lua as a boolean (0x6c9880 and 0x6c94d0): anything else is reported and false
static bool RetBool(const LuaClassRet& theRet, int i)
{
	if (i >= theRet.count || theRet.v[i].type != LuaClassRet::TBool) {
		gLuaSupport->ReportError("Invalid Lua Result: Expected boolean in LuaClassRet");
		return false;
	}
	return theRet.v[i].b;
}

// The three questions the script answers with (override, value), 0x414430, 0x4144e0 and 0x4145a0: an override
// gives the value; otherwise the player may when the battle is idle and the value says so
static bool AskScript(BattleEngine* theEngine, const char* theMethod)
{
	LuaClassRet aRet = theEngine->CallLinkedMethod(theMethod, 0);
	bool aIdle = theEngine->mState == BattleEngine::BE_IDLE;
	if (aRet.count > 0) {
		if (RetBool(aRet, 0))
			return RetBool(aRet, 1);
		return aIdle && RetBool(aRet, 1);
	}
	return aIdle;
}

BattleEngine::BattleEngine(Board* theBoard, GameApp* theApp) : mApp(theApp), mBoard(theBoard)
{
	RegisterLua();
	gBattleEngine = this;
	mEnums["BE_UNDEFINED"] = BE_UNDEFINED;
	mEnums["BE_INITIALIZED"] = BE_INITIALIZED;
	mEnums["BE_IDLE"] = BE_IDLE;
	mEnums["BE_PRE_ATTACK"] = BE_PRE_ATTACK;
	mEnums["BE_ATTACK"] = BE_ATTACK;
	mEnums["BE_DEFEND"] = BE_DEFEND;
	mEnums["BE_SWAP"] = BE_SWAP;
	mEnums["BE_RESTART_BATTLE"] = BE_RESTART_BATTLE;
	mEnums["BE_DYING"] = BE_DYING;
	mEnums["BE_END_CHAPTER"] = BE_END_CHAPTER;
	mEnums["BE_MISC"] = BE_MISC;
}

BattleEngine::~BattleEngine()
{
	if (IsLinked())
		CallLinkedMethod("Destructor", 0);
	if (gLuaSupport != nullptr && gLuaSupport->L != nullptr) {
		lua_pushnil(gLuaSupport->L);
		lua_setglobal(gLuaSupport->L, "gBattleEngine");
	}
	// Port: the original leaves the C++ global to its owner
	if (gBattleEngine == this)
		gBattleEngine = nullptr;
}

void BattleEngine::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Class<BattleEngine>("BattleEngine", false)
		.Method<&BattleEngine::LuaGetRegisteredName>("LuaGetRegisteredName")
		.Method<&BattleEngine::GetBoard>("GetBoard")
		.Method<&BattleEngine::Update>("Update")
		.Method<&BattleEngine::GetState>("GetState")
		.Method<&BattleEngine::LostTurn>("LostTurn")
		.Method<&BattleEngine::SetState>("SetState");
}

void BattleEngine::Init(Board* theBoard)
{
	mBoard = theBoard;
	lua_pushstring(gLuaSupport->L, "scripts/BattleEngine.lua");
	gLuaSupport->Call("dofile", 1, 0);
	mInitialized = true;
}

void BattleEngine::SetState(int theState)
{
	// Port: the original does not look for a board
	if (mBoard != nullptr) {
		if (theState == BE_END_CHAPTER)
			mBoard->mEndingChapter = true;
		else if (mState == BE_END_CHAPTER)
			mBoard->mEndingChapter = false;
	}
	// Port: the original also asked Lua for the stack info of the caller (lua_getstack and lua_getinfo "Snl") and did
	// nothing with it
	mState = theState;
}

bool BattleEngine::LostTurn()
{
	lua_State* L = gLuaSupport->L;
	lua_getglobal(L, "gBattleEngine");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pushstring(L, "mPlayerPtr");
	lua_gettable(L, -2);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2);
		return false;
	}
	lua_pop(L, 2);
	return !CallLua("gBattleEngine.mPlayerPtr:CanAttack", 0).Bool() && CallLinkedMethod("CanBattle", 0).Bool() &&
	       CanSubmitTiles();
}

bool BattleEngine::CanSubmitTiles()
{
	return AskScript(this, "CanSubmitTiles");
}

bool BattleEngine::CanScrambleTiles()
{
	return AskScript(this, "CanScrambleTiles");
}

bool BattleEngine::CanClickItems()
{
	return AskScript(this, "CanClickItems");
}

bool BattleEngine::SubmitTiles(std::list<Tile*>& theTiles)
{
	if (gTileEngine != nullptr && gTileEngine->mMovingCount > 0)
		return false;
	if (!CallLinkedMethod("CanBattle", 0).Bool())
		return false;
	if (!CanSubmitTiles())
		return false;
	std::string aWord = Board::GetWord(theTiles);
	if (!mBoard->IsValidWord(aWord, true))
		return false;
	if (!mApp->mTimedMode && gTileEngine != nullptr)
		gTileEngine->StartRound(false);

	// gLinkedObjectTable[this].mSubmittedTilePtrs = {tile, tile, ...} (from 0)
	lua_State* L = gLuaSupport->L;
	lua_getglobal(L, "gLinkedObjectTable");
	int aTable = lua_gettop(L);
	PushLinkedObject();
	lua_gettable(L, aTable);
	int anObject = lua_gettop(L);
	lua_pushstring(L, "mSubmittedTilePtrs");
	lua_newtable(L);
	lua_settable(L, anObject);
	lua_pushstring(L, "mSubmittedTilePtrs");
	lua_gettable(L, anObject);
	int aList = lua_gettop(L);
	int anIndex = 0;
	for (Tile* aTile : theTiles) {
		lua_pushint(L, anIndex);
		Tile::PushTile(aTile);
		lua_settable(L, aList);
		anIndex++;
	}
	lua_pop(L, 3);

	// 0x415af0: SubmitTiles(number of tiles, word)
	lua_pushint(L, (int)theTiles.size());
	lua_pushstring(L, aWord.c_str());
	CallLinkedMethod("SubmitTiles", 2);
	return true;
}
