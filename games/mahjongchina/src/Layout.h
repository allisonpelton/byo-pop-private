#ifndef __MAHJONGCHINA_LAYOUT_H__
#define __MAHJONGCHINA_LAYOUT_H__

// A layout (0x4144e0, vtable 0x51d808) and its tiles (0x413260, vtable 0x51d7f8, 0x48 bytes). A layout comes from a
// LAYOUTS resource of Resource.dll (0x415110): "LM", version 1, its name and its tiles as x, y, z and kind (0 for a
// kind the deal picks). x and y count half tiles, so neighbours in a row are 2 apart; the list is in drawing order,
// lowest z first, which the tests for free tiles rely on.

#include "JBBase.h"
#include "Stream.h"

#include <string>

namespace mahjongchina
{

// 0x413260, 0x4132d0
class Tile : public jb::Streamable
{
public:
	int mX = 0;            // +0x04
	int mY = 0;            // +0x08
	int mZ = 0;            // +0x0c
	int mKind = 0;         // +0x10 1-based; 0 until the deal gives it one
	bool mPreset = false;  // +0x14 the layout gave the kind
	int mTaken = 0;        // +0x18 0 on the board, else the move it went at
	Tile* mWildLink = nullptr;  // +0x1c the tile that turns wild (0x2f) when this one has gone (0x414fd0)
	jb::Rect mRect = {-1, -1, -1, -1};  // +0x20 where it is drawn (a shuffle moves left and top off the board)
	int mFromX = 0;        // +0x30 where it flies in from (0x40b780); the original leaves it unset until then
	int mFromY = 0;        // +0x34
	bool mFree = false;    // +0x38 it can be taken (0x414c20); the original leaves it unset until the first test
	bool mBobsAsHint = false;  // +0x39 the lift is a hint's bob, not an undo's settle
	int mLiftStart = 0;    // +0x3c when its lift began (a hint's bob, an undo's settle); 0 for none
	int mGroup = 0;        // +0x40 the deal's pairing: the tiles of a pair or a four share it, -1 for a kind given
	bool mQuad = false;    // +0x44 the deal's pair is half of a four

	// 0x413260
	Tile() {}
	// 0x4132d0
	Tile(int theX, int theY, int theZ, int theKind) : mX(theX), mY(theY), mZ(theZ), mKind(theKind) {}
	// 0x4132b0, 0x413330
	virtual ~Tile() {}

	// 0x413430: a saved game's tile (and the move it went at)
	void Serialize(jb::Stream& theStream) override;
	// 0x413480: a layout's tile
	virtual void SerializeLayout(jb::Stream& theStream);
};

// 0x4144e0
class Layout : public jb::Streamable
{
public:
	jb::ListNode* mHead = nullptr;    // +0x04
	jb::ListNode* mTail = nullptr;    // +0x08
	jb::ListNode* mCursor = nullptr;  // +0x0c
	int mCount = 0;                   // +0x10
	bool mHasCounts = false;          // +0x14 some tile has its kind from the layout
	int mMagic = 0;                   // +0x18 "LM" (0x4d4c)
	int mVersion = 0;                 // +0x1c 1
	std::string mName;                // +0x20

	// 0x4144e0
	Layout();
	// 0x414510, 0x414530
	virtual ~Layout();

	// 0x415490: a saved game's kinds, moves and wild links
	void Serialize(jb::Stream& theStream) override;
	// 0x415670: the layout itself
	virtual void SerializeLayout(jb::Stream& theStream);

	// 0x414590, 0x4145b0
	Tile* First();
	Tile* Next();
	// 0x4145d0: every tile back on the board, and without the deal's kinds when theKinds
	void Reset(bool theKinds);
	// 0x414610, 0x4146a0
	void AddTile(int theX, int theY, int theZ, int theKind);
	void Append(Tile* theTile);
	// 0x414700
	void Clear(bool theDelete);
	// 0x414750: the topmost tile on the board at the point, on level *theZ unless that is -1
	Tile* HitTest(int theX, int theY, int* theZ, int* theTileX, int* theTileY);
	// 0x4147e0, 0x414820
	void UpdateFree();
	void UpdateFree(Tile* theTile);
	// 0x414840: the free tiles on the board; how many (the array holds at most theMax)
	int GetFree(Tile** theTiles, int theMax);
	// 0x414880, 0x4148a0: the tiles on the board that match a kind
	int GetHalves(Tile** theTiles, int theMax);
	int GetMatching(int theKind, Tile** theTiles, int theMax);
	// 0x414900: the tiles on the board with a kind
	int CountKinded();
	// 0x414930: the free tiles of kinds 0x2c, 0x2e and 0x2f, in a layout with kinds of its own
	int GetFreeSpecials(Tile** theTiles, int theMax);
	// 0x414990: the tiles without a kind, gone or not
	int CountBlank();
	// 0x4149b0, 0x414be0: the tiles next to or under theTiles whose freedom becomes theFree (the tiles themselves
	// gone); they are marked so and listed
	int UpdateNeighbours(Tile** theTiles, int theNum, Tile** theOut, int theMax, bool theFree);
	int UpdateNeighbours(Tile* theTile, Tile* theOther, Tile** theOut, int theMax, bool theFree);
	// 0x414c20: nothing on it and a side open
	bool IsFree(int theX, int theY, int theZ);
	// 0x414d10: the pairs among the free tiles, the tiles on the board, and the kinds with a free pair
	void CountMoves(int* thePairs, int* theLeft, int* theKinds);
	// 0x414df0
	int CountLeft();
	// 0x414e10: the free tiles of kind 0x2b
	int CountFree2b();
	// 0x414e40: two free flowers
	bool HasFreeFlowers();
	// 0x414e80: the tiles taken at a move
	int GetTaken(int theMove, Tile** theTiles, int theMax);
	// 0x414ec0: the free tiles of the theIndex-th kind that has a free pair
	int GetPairsOf(int theIndex, Tile** theTiles, int theMax);
	// 0x414fd0
	void ApplyWildLinks();
	// 0x415010
	void ClearGroups();
	// 0x415030: the tiles of a deal's group
	int FindGroup(int theGroup, Tile** theTiles, int theMax);
	// 0x415070: the dealt kinds still on the board, counted as gKindCounts counts them and taken off the tiles
	int CollectKinds(int* theCounts);
	// 0x4150c0: the 42 counts of the kinds the layout gives; how many tiles they fix
	int GetCounts(int* theCounts);
	// 0x415110, 0x415210
	bool Load(int theId);
	bool Load(jb::Stream& theStream);
	// 0x415260: a tile of the lowest kind on the board and the last other one of it
	void GetLowest(Tile** theTile, Tile** theOther);
	// 0x4152d0, 0x415300: the tiles of kinds below 0x2b, or the others, taken (move 1); how many
	int TakeKinds();
	int TakeSpecials();
	// 0x415330: some of it shows from under the tiles drawn after it
	bool IsVisible(Tile* theTile);
	// 0x415770, 0x415790
	int IndexOf(Tile* theTile);
	Tile* At(int theIndex);
};

} // namespace mahjongchina

#endif
