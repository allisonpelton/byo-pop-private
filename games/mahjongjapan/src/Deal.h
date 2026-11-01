#ifndef __MAHJONGJAPAN_DEAL_H__
#define __MAHJONGJAPAN_DEAL_H__

// The deal (0x413340..0x414420, vtable 0x51d804): which kinds the tiles of a layout get. It searches for an order in
// which the board can be cleared, taking free pairs as a player would (0x413d90, for at most 2 seconds), and gives
// the pairs of that order their kinds, so a dealt game can be won; without one in time the kinds go at random
// (0x413c30). The layout itself (0x4144e0) is in Layout.h.

#include "Layout.h"

namespace mahjongjapan
{

// 0x413340
bool TilesMatch(int theKind, int theOther);
// 0x413380
bool KindsMatch(int theKind, int theOther);
// 0x4133c0
int KindPoints(int theKind);
// 0x413a00
void AddKind(int* theCounts, int theSize, int* theEmpty, int* theFull);

// 0x4134c0
class Deal
{
public:
	Layout* mLayout = nullptr;  // +0x04
	int mQuadPercent = 0;       // +0x08 the share of fours, and how often the search groups pairs into fours
	int mMove = 0;              // +0x0c the search's move (it starts from 0x413500's third argument)

	// 0x4134d0, 0x4134f0
	virtual ~Deal() {}

	// 0x413500: deal theLayout, or deal again the kinds still on the board (theRedeal)
	void Start(Layout* theLayout, int theQuadPercent, int theMove, bool theRedeal);
	// 0x4136a0
	void Distribute(int theTiles);
	// 0x413aa0: the kinds along the order found
	void DealOrder();
	// 0x413c30: the kinds at random
	void DealAny();
	// 0x413d90: an order from the theCount free tiles at depth theDepth; theGroup is a pair waiting to be joined
	// into a four
	bool Solve(int theDepth, int theCount, int theGroup);
	// 0x413ff0: take theTile and theOther (null for a wild tile) and search on
	bool Take(Tile* theTile, Tile* theOther, int theCount, int theDepth, int theGroup);
	// 0x414310: the grouping of a pair taken; which tiles it grouped (1 theTile, 2 theOther, 4 theThird)
	int Group(Tile* theTile, Tile* theOther, Tile* theThird, int* theGroup);
	// 0x414420: the pair into the order (a four's other pair with it); the original takes a third argument it
	// does not use
	void Record(Tile* theTile, Tile* theOther);
};

// 0x413b70: theSize tiles of one kind, weighted by how many times theSize each kind still has
void DealKind(Tile** theTiles, int theSize, int* theLeft);
// 0x413c80: one tile's kind, weighted by the counts
void DealOne(Tile* theTile, int* theLeft);
// 0x413d20: which season (theBase 0x23) or flower (0x27)
void DealBonus(Tile* theTile, int theRange, int theBase);
// 0x413fa0: the search's time is not up; the clock is read every 1001 steps
bool SolveInTime();

extern int gDealCount;  // 0x533630 the tiles the search must take

} // namespace mahjongjapan

#endif
