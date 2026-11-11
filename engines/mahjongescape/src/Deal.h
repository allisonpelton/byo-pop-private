#ifndef __MAHJONG_DEAL_H__
#define __MAHJONG_DEAL_H__

// The deal (0x412dc0..0x413ea0 (mahjongchina), 0x413340..0x414420 (mahjongjapan), vtable 0x51c6ec (mahjongchina),
// 0x51d804 (mahjongjapan)): which kinds the tiles of a layout get. It searches for an order in which the board can be
// cleared, taking free pairs as a player would (0x413810 (mahjongchina), 0x413d90 (mahjongjapan), for at most 2
// seconds), and gives the pairs of that order their kinds, so a dealt game can be won; without one in time the kinds go
// at random (0x4136b0 (mahjongchina), 0x413c30 (mahjongjapan)). The layout itself (0x413f60 (mahjongchina),
// 0x4144e0 (mahjongjapan)) is in Layout.h.

#include "Layout.h"

namespace mahjong
{

// 0x412dc0 (mahjongchina), 0x413340 (mahjongjapan)
bool TilesMatch(int theKind, int theOther);
// 0x412e00 (mahjongchina), 0x413380 (mahjongjapan)
bool KindsMatch(int theKind, int theOther);
// 0x412e40 (mahjongchina), 0x4133c0 (mahjongjapan)
int KindPoints(int theKind);
// 0x413480 (mahjongchina), 0x413a00 (mahjongjapan)
void AddKind(int* theCounts, int theSize, int* theEmpty, int* theFull);

// 0x412f40 (mahjongchina), 0x4134c0 (mahjongjapan)
class Deal
{
public:
	Layout* mLayout = nullptr;  // +0x04
	int mQuadPercent = 0;       // +0x08 the share of fours, and how often the search groups pairs into fours
	// +0x0c the search's move (it starts from 0x412f80 (mahjongchina), 0x413500 (mahjongjapan)'s third argument)
	int mMove = 0;

	// 0x412f50 (mahjongchina), 0x4134d0 (mahjongjapan), 0x412f70 (mahjongchina), 0x4134f0 (mahjongjapan)
	virtual ~Deal() {}

	// 0x412f80 (mahjongchina), 0x413500 (mahjongjapan): deal theLayout, or deal again the kinds still on the board
	// (theRedeal)
	void Start(Layout* theLayout, int theQuadPercent, int theMove, bool theRedeal);
	// 0x413120 (mahjongchina), 0x4136a0 (mahjongjapan)
	void Distribute(int theTiles);
	// 0x413520 (mahjongchina), 0x413aa0 (mahjongjapan): the kinds along the order found
	void DealOrder();
	// 0x4136b0 (mahjongchina), 0x413c30 (mahjongjapan): the kinds at random
	void DealAny();
	// 0x413810 (mahjongchina), 0x413d90 (mahjongjapan): an order from the theCount free tiles at depth theDepth;
	// theGroup is a pair waiting to be joined into a four
	bool Solve(int theDepth, int theCount, int theGroup);
	// 0x413a70 (mahjongchina), 0x413ff0 (mahjongjapan): take theTile and theOther (null for a joker or a magnet) and
	// search on
	bool Take(Tile* theTile, Tile* theOther, int theCount, int theDepth, int theGroup);
	// 0x413d90 (mahjongchina), 0x414310 (mahjongjapan): the grouping of a pair taken; which tiles it grouped (1
	// theTile, 2 theOther, 4 theThird)
	int Group(Tile* theTile, Tile* theOther, Tile* theThird, int* theGroup);
	// 0x413ea0 (mahjongchina), 0x414420 (mahjongjapan): the pair into the order (a four's other pair with it); the
	// original takes a third argument it does not use
	void Record(Tile* theTile, Tile* theOther);
};

// 0x4135f0 (mahjongchina), 0x413b70 (mahjongjapan): theSize tiles of one kind, weighted by how many times theSize each
// kind still has
void DealKind(Tile** theTiles, int theSize, int* theLeft);
// 0x413700 (mahjongchina), 0x413c80 (mahjongjapan): one tile's kind, weighted by the counts
void DealOne(Tile* theTile, int* theLeft);
// 0x4137a0 (mahjongchina), 0x413d20 (mahjongjapan): which season (theBase KIND_FIRST_SEASON) or flower
// (KIND_FIRST_FLOWER)
void DealBonus(Tile* theTile, int theRange, int theBase);
// 0x413a20 (mahjongchina), 0x413fa0 (mahjongjapan): the search's time is not up; the clock is read every 1001 steps
bool SolveInTime();

extern int gDealCount;  // 0x532988 (mahjongchina), 0x533630 (mahjongjapan) the tiles the search must take

} // namespace mahjong

#endif
