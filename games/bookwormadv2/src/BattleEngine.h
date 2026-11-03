// Sexy::BattleEngine (0x3c bytes, constructor V1:0x413ec0, vtable 0x6f7ca8): the rules of a battle. A LuaLinkedClass
// whose Lua object (gBattleEngine, scripts/BattleEngine.lua) is the battle's turn order and its states; the C++ part
// keeps the state, asks the script what the player may do (CanSubmitTiles, CanScrambleTiles, CanClickItems) and
// hands it the word the player spells (SubmitTiles). One exists at a time (gBattleEngine); the board makes it with
// the battle, and a mini-game does not have one.
#pragma once

#include <list>
#include <string>

#include "LuaLinkedClass.h"

namespace Sexy {

class Board;
class GameApp;
class Tile;

class BattleEngine : public LuaLib::LuaLinkedClass {
public:
	// The states (the scripts' BE_*, set on the Lua object and class when it is linked)
	enum {
		BE_UNDEFINED = -1,
		BE_INITIALIZED = 0,
		BE_IDLE = 1,
		BE_PRE_ATTACK = 2,
		BE_ATTACK = 3,
		BE_DEFEND = 4,
		BE_SWAP = 5,
		BE_RESTART_BATTLE = 6,
		BE_DYING = 7,
		BE_END_CHAPTER = 8,
		BE_MISC = 9,
		BE_COMPANION = 10,  // the companion acts (Volume 2)
	};

	GameApp* mApp;                        // +0x28
	Board* mBoard;                        // +0x2c
	int mState = BE_UNDEFINED;            // +0x30
	int m34 = 0;                          // +0x34: set to 0 by the constructor; no use found
	bool mInitialized = false;            // +0x38: Init has run

	BattleEngine(Board* theBoard, GameApp* theApp);  // V1:0x413ec0
	~BattleEngine() override;                        // 0x40dec0: the Lua object's Destructor first, then the Lua global gBattleEngine to nil

	const char* LuaGetRegisteredName() override { return "BattleEngine"; }  // 0x40da30
	// The original registered the class in the constructor (V1:0x413ec0), every time
	static void RegisterLua();

	// V1:0x414660: takes the board and runs scripts/BattleEngine.lua
	void Init(Board* theBoard);

	// The scripts' methods
	Board* GetBoard() { return mBoard; }  // V1:0x413e60
	void Update() {}                      // V1:0x413e50: nothing
	int GetState() { return mState; }     // V1:0x413e70
	// V1:0x414770: the new state (BE_END_CHAPTER sets the board's +0xe3 flag and leaving it clears it)
	void SetState(int theState);
	// 0x40e800: the player cannot attack and the script and the state would let the tiles be submitted
	bool LostTurn();

	// The board's
	bool CanSubmitTiles();    // V1:0x414430: the script's CanSubmitTiles, or the state being BE_IDLE
	bool CanScrambleTiles();  // 0x40e0e0
	bool CanClickItems();     // 0x40e1a0
	// V1:0x414810: the player submits the selected tiles as a word: when no tiles slide and the script lets a battle
	// go on, the word is a word and the state is right, the script gets the tiles (mSubmittedTilePtrs of its
	// object, from 0) and SubmitTiles(number of tiles, word). False when it was not taken.
	bool SubmitTiles(std::list<Tile*>& theTiles);
};

extern BattleEngine* gBattleEngine;  // 0x825930

}  // namespace Sexy
