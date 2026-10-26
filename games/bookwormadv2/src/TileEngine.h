// Sexy::TileEngine (0x1dc bytes, constructor 0x499770, vtable 0x7b1074): the battle's tiles. A LuaLinkedClass whose
// Lua object (gTileEngine, scripts/TileEngine.lua) draws the tiles and decides what happens when they are
// made, change letters, move or are chosen. The C++ part makes, copies and deletes tiles, slides them, picks the
// letters of new tiles (keeping words from the avoided list from showing up in a 4x4 grid) and passes the board's
// events to the Lua object. One exists at a time (gTileEngine); the board makes it with the battle.
#pragma once

#include <list>
#include <set>
#include <string>
#include <vector>

#include "LuaLinkedClass.h"

namespace Sexy {

class Board;
class Graphics;
class Tile;

class TileEngine : public LuaLib::LuaLinkedClass {
public:
	Board* mBoard = nullptr;                      // +0x28: null when there is no battle
	std::set<Tile*> mTiles;                       // +0x2c: tiles out of the board, deleted at its next update
	std::vector<std::string> mAvoidedWords[26];   // +0x38: by first letter 'A' to 'Z' (Init)
	int mMovingCount = 0;                         // +0x1d8: tiles that slide

	TileEngine();           // 0x499770
	~TileEngine() override; // 0x499860: the Lua object's Destructor first, then the Lua globals TileEngine, gTileTable and gTileEngine to nil

	const char* LuaGetRegisteredName() override { return "TileEngine"; }  // 0x498880
	// The original registered the class in the constructor (0x499770), every time
	static void RegisterLua();

	// 0x499c80: fills the avoided words, takes the board and runs scripts/TileEngine.lua
	void Init(Board* theBoard);
	// 0x499bc0: deletes the tiles taken out of the board (the board's Update calls it)
	void DeleteAllTiles();

	// The scripts' methods
	// 0x49a990: a new tile with a letter chosen at random (theRandomLetter; otherwise a space); the script's
	// RegisterTile hears of it when theRegister
	Tile* CreateTile(bool theUnused, bool theRegister, bool theRandomLetter);
	// 0x49abb0: no more of the tile for the board; the script's UnregisterTile hears of it when theUnregister,
	// and theDelete deletes it
	void DeleteTile(Tile* theTile, bool theUnregister, bool theDelete);
	// 0x49aa90: a copy of the tile, registered with the script and kept by the board as a clone
	Tile* CloneTile(Tile* theTile);
	// 0x49b090: the tile slides to a pixel position, theSpeed of the way each update
	void SlideTile(Tile* theTile, int theToX, int theToY, float theSpeed, bool theNotify);

	// The board's
	// 0x4990e0: DeleteTile(theTile, true, false)
	void RemoveTile(Tile* theTile) { DeleteTile(theTile, true, false); }
	// 0x49ac50: the script's StartRound(timed_mode)
	void StartRound(bool theTimedMode);
	// 0x49aca0: draws the tiles of a list that slide (theMoving) or do not, with the script's Draw
	void Draw(std::list<Tile*>& theTiles, Graphics* g, bool theMoving, bool theDrawExtraStuff);
	// 0x49af50 (no caller found in the original): the tiles that do not slide, then the ones that do, unless
	// theStillOnly, which draws only the still ones and without the extras
	void DrawTiles(std::list<Tile*>& theTiles, Graphics* g, bool theStillOnly);
	// 0x49afb0: Update of every tile of a list, then the script's Update when theCallLua
	void Update(std::list<Tile*>& theTiles, bool theCallLua);
	// 0x49b010: every tile of the list gets a new letter (the script's ChangeLetter)
	void ChangeLetters(std::list<Tile*>& theTiles);
	// 0x49b0e0: the tile starts to slide (the script's TileXYChanged and TileStartedMoving)
	void StartMoving(Tile* theTile, bool theNotify);
	// 0x49b170: the tile stops sliding (the script's TileXYChanged and TileStoppedMoving when theNotify)
	void StopMoving(Tile* theTile, bool theNotify);
	// 0x49b1e0 (0x486fa0 in the exe): the script's ValidWordSpelled(is_valid, num_tiles, is_noun, is_adjective, is_verb,
	// tiles...); theFlags are the word's (Lex::GetFlags), 0 when it is not a word
	void ValidWordSpelled(std::list<Tile*>& theTiles, bool theIsValid, int theFlags);

	// 0x49a120: the letter of a new tile. Counts the letters on the board; the candidates are the letters below
	// their maximum count (kMaxLetters) that make no avoided word in a 4x4 grid (IsLetterAllowed) and keep the
	// vowels and consonants from running past 9; each has a weight (the English letter frequencies plus the script's
	// GetExtraLetterFreq, less for letters already on the board twice or three times) and one is chosen at
	// random. theReplacing is for replacing tiles on a board that is full, which avoids a third copy of a letter.
	char GenerateLetter(bool theReplacing);
	// 0x499e30: whether adding theLetter to the board keeps the avoided words out: the patterns are the grid's four
	// rows and four columns with '*' for a cell without a tile
	bool IsLetterAllowed(const std::vector<std::string>& thePatterns, char theLetter, int theVowels, int theConsonants,
	                     int theTotal);
};

extern TileEngine* gTileEngine;  // 0x825994

}  // namespace Sexy
