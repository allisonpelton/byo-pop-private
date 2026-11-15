#ifndef __MAHJONG_LAYOUT_H__
#define __MAHJONG_LAYOUT_H__

// A layout (0x413f60 (mahjongchina), 0x4144e0 (mahjongjapan), vtable 0x51c6f0 (mahjongchina), 0x51d808 (mahjongjapan))
// and its tiles (0x412ce0 (mahjongchina), 0x413260 (mahjongjapan), vtable 0x51c6e0 (mahjongchina),
// 0x51d7f8 (mahjongjapan), 0x48 bytes). A layout comes from a LAYOUTS resource of Resource.dll
// (0x414b90 (mahjongchina), 0x415110 (mahjongjapan)): "LM", version 1, its name and its tiles as x, y, z and kind (0
// for a kind the deal picks). x and y count half tiles, so neighbours in a row are 2 apart; the list is in drawing
// order, lowest z first, which the tests for free tiles rely on.

#include "JBBase.h"
#include "Stream.h"

#include <string>

namespace mahjong
{

// A tile's kind (Tile::mKind). 1 to 0x22 are the plain tiles, which match their own kind; any season matches any
// season and any flower any flower; the last five are the adventure's magic tiles.
enum
{
	KIND_LAST_PLAIN = 0x22,
	KIND_FIRST_SEASON = 0x23,
	KIND_LAST_SEASON = 0x26,
	KIND_FIRST_FLOWER = 0x27,
	KIND_LAST_FLOWER = 0x2a,
	KIND_GOLD = 0x2b,    // the Magic Gold tile: its pair ends an escape stage
	KIND_KEY = 0x2c,     // takes a lock that can be seen
	KIND_LOCK = 0x2d,
	KIND_JOKER = 0x2e,   // takes every free tile of the kind of a tile that can be seen
	KIND_MAGNET = 0x2f,  // takes a tile that can be seen and becomes its kind
	NUM_COUNTED_KINDS = 0x2c,  // the kinds a layout counts free pairs of: up to the gold tile
};

// 0x412ce0 (mahjongchina), 0x413260 (mahjongjapan), 0x412d50 (mahjongchina), 0x4132d0 (mahjongjapan)
class Tile : public jb::Streamable
{
public:
	int mX = 0;            // +0x04
	int mY = 0;            // +0x08
	int mZ = 0;            // +0x0c
	int mKind = 0;         // +0x10 1-based; 0 until the deal gives it one
	bool mPreset = false;  // +0x14 the layout gave the kind
	int mTaken = 0;        // +0x18 0 on the board, else the move it went at
	// +0x1c the tile that turns into a magnet when this one has gone (0x414a50 (mahjongchina), 0x414fd0 (mahjongjapan))
	Tile* mMagnetLink = nullptr;
	jb::Rect mRect = {-1, -1, -1, -1};  // +0x20 where it is drawn (a shuffle moves left and top off the board)
	// +0x30 where it flies in from (0x40b740 (mahjongchina), 0x40b780 (mahjongjapan)); the original leaves it unset
	// until then
	int mFromX = 0;
	int mFromY = 0;        // +0x34
	// +0x38 it can be taken (0x4146a0 (mahjongchina), 0x414c20 (mahjongjapan)); the original leaves it unset until the
	// first test
	bool mFree = false;
	bool mBobsAsHint = false;  // +0x39 the lift is a hint's bob, not an undo's settle
	int mLiftStart = 0;    // +0x3c when its lift began (a hint's bob, an undo's settle); 0 for none
	int mGroup = 0;        // +0x40 the deal's pairing: the tiles of a pair or a four share it, -1 for a kind given
	bool mQuad = false;    // +0x44 the deal's pair is half of a four

	// 0x412ce0 (mahjongchina), 0x413260 (mahjongjapan)
	Tile() {}
	// 0x412d50 (mahjongchina), 0x4132d0 (mahjongjapan)
	Tile(int theX, int theY, int theZ, int theKind) : mX(theX), mY(theY), mZ(theZ), mKind(theKind) {}
	// 0x412d30 (mahjongchina), 0x4132b0 (mahjongjapan), 0x412db0 (mahjongchina), 0x413330 (mahjongjapan)
	virtual ~Tile() {}

	// 0x412eb0 (mahjongchina), 0x413430 (mahjongjapan): a saved game's tile (and the move it went at)
	void Serialize(jb::Stream& theStream) override;
	// 0x412f00 (mahjongchina), 0x413480 (mahjongjapan): a layout's tile
	virtual void SerializeLayout(jb::Stream& theStream);
};

// 0x413f60 (mahjongchina), 0x4144e0 (mahjongjapan)
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

	// 0x413f60 (mahjongchina), 0x4144e0 (mahjongjapan)
	Layout();
	// 0x413f90 (mahjongchina), 0x414510 (mahjongjapan), 0x413fb0 (mahjongchina), 0x414530 (mahjongjapan)
	virtual ~Layout();

	// 0x414f10 (mahjongchina), 0x415490 (mahjongjapan): a saved game's kinds, moves and magnet links
	void Serialize(jb::Stream& theStream) override;
	// 0x4150f0 (mahjongchina), 0x415670 (mahjongjapan): the layout itself
	virtual void SerializeLayout(jb::Stream& theStream);

	// 0x414010 (mahjongchina), 0x414590 (mahjongjapan), 0x414030 (mahjongchina), 0x4145b0 (mahjongjapan)
	Tile* First();
	Tile* Next();
	// 0x414050 (mahjongchina), 0x4145d0 (mahjongjapan): every tile back on the board, and without the deal's kinds when
	// theKinds
	void Reset(bool theKinds);
	// 0x414090 (mahjongchina), 0x414610 (mahjongjapan), 0x414120 (mahjongchina), 0x4146a0 (mahjongjapan)
	void AddTile(int theX, int theY, int theZ, int theKind);
	void Append(Tile* theTile);
	// 0x414180 (mahjongchina), 0x414700 (mahjongjapan)
	void Clear(bool theDelete);
	// 0x4141d0 (mahjongchina), 0x414750 (mahjongjapan): the topmost tile on the board at the point, on level *theZ
	// unless that is -1
	Tile* HitTest(int theX, int theY, int* theZ, int* theTileX, int* theTileY);
	// 0x414260 (mahjongchina), 0x4147e0 (mahjongjapan), 0x4142a0 (mahjongchina), 0x414820 (mahjongjapan)
	void UpdateFree();
	void UpdateFree(Tile* theTile);
	// 0x4142c0 (mahjongchina), 0x414840 (mahjongjapan): the free tiles on the board; how many (the array holds at most
	// theMax)
	int GetFree(Tile** theTiles, int theMax);
	// 0x414300 (mahjongchina), 0x414880 (mahjongjapan), 0x414320 (mahjongchina), 0x4148a0 (mahjongjapan): the tiles on
	// the board that match a kind
	int GetLocks(Tile** theTiles, int theMax);
	int GetMatching(int theKind, Tile** theTiles, int theMax);
	// 0x414380 (mahjongchina), 0x414900 (mahjongjapan): the tiles on the board with a kind
	int CountKinded();
	// 0x4143b0 (mahjongchina), 0x414930 (mahjongjapan): the free keys, jokers and magnets, in a layout with kinds of
	// its own
	int GetFreeSpecials(Tile** theTiles, int theMax);
	// 0x414410 (mahjongchina), 0x414990 (mahjongjapan): the tiles without a kind, gone or not
	int CountBlank();
	// 0x414430 (mahjongchina), 0x4149b0 (mahjongjapan), 0x414660 (mahjongchina), 0x414be0 (mahjongjapan): the tiles
	// next to or under theTiles whose freedom becomes theFree (the tiles themselves gone); they are marked so and
	// listed
	int UpdateNeighbours(Tile** theTiles, int theNum, Tile** theOut, int theMax, bool theFree);
	int UpdateNeighbours(Tile* theTile, Tile* theOther, Tile** theOut, int theMax, bool theFree);
	// 0x4146a0 (mahjongchina), 0x414c20 (mahjongjapan): nothing on it and a side open
	bool IsFree(int theX, int theY, int theZ);
	// 0x414790 (mahjongchina), 0x414d10 (mahjongjapan): the pairs among the free tiles, the tiles on the board, and the
	// kinds with a free pair
	void CountMoves(int* thePairs, int* theLeft, int* theKinds);
	// 0x414870 (mahjongchina), 0x414df0 (mahjongjapan)
	int CountLeft();
	// 0x414890 (mahjongchina), 0x414e10 (mahjongjapan): the free gold tiles
	int CountFree2b();
	// 0x4148c0 (mahjongchina), 0x414e40 (mahjongjapan): two free flowers
	bool HasFreeFlowers();
	// 0x414900 (mahjongchina), 0x414e80 (mahjongjapan): the tiles taken at a move
	int GetTaken(int theMove, Tile** theTiles, int theMax);
	// 0x414940 (mahjongchina), 0x414ec0 (mahjongjapan): the free tiles of the theIndex-th kind that has a free pair
	int GetPairsOf(int theIndex, Tile** theTiles, int theMax);
	// 0x414a50 (mahjongchina), 0x414fd0 (mahjongjapan)
	void ApplyMagnetLinks();
	// 0x414a90 (mahjongchina), 0x415010 (mahjongjapan)
	void ClearGroups();
	// 0x414ab0 (mahjongchina), 0x415030 (mahjongjapan): the tiles of a deal's group
	int FindGroup(int theGroup, Tile** theTiles, int theMax);
	// 0x414af0 (mahjongchina), 0x415070 (mahjongjapan): the dealt kinds still on the board, counted as gKindCounts
	// counts them and taken off the tiles
	int CollectKinds(int* theCounts);
	// 0x414b40 (mahjongchina), 0x4150c0 (mahjongjapan): the 42 counts of the kinds the layout gives; how many tiles
	// they fix
	int GetCounts(int* theCounts);
	// 0x414b90 (mahjongchina), 0x415110 (mahjongjapan), 0x414c90 (mahjongchina), 0x415210 (mahjongjapan)
	bool Load(int theId);
	bool Load(jb::Stream& theStream);
	// 0x414ce0 (mahjongchina), 0x415260 (mahjongjapan): a tile of the lowest kind on the board and the last other one
	// of it
	void GetLowest(Tile** theTile, Tile** theOther);
	// 0x414d50 (mahjongchina), 0x4152d0 (mahjongjapan), 0x414d80 (mahjongchina), 0x415300 (mahjongjapan): the plain
	// tiles, seasons and flowers, or the magic tiles, taken (move 1); how many
	int TakeKinds();
	int TakeSpecials();
	// 0x414db0 (mahjongchina), 0x415330 (mahjongjapan): some of it shows from under the tiles drawn after it
	bool IsVisible(Tile* theTile);
	// 0x4151f0 (mahjongchina), 0x415770 (mahjongjapan), 0x415210 (mahjongchina), 0x415790 (mahjongjapan)
	int IndexOf(Tile* theTile);
	Tile* At(int theIndex);
};

} // namespace mahjong

#endif
