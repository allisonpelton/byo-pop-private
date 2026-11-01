#ifndef __MAHJONGJAPAN_DEAL_H__
#define __MAHJONGJAPAN_DEAL_H__

// The deal (0x413500..0x413aa0): which kinds the tiles of a layout get. The layout itself (0x4144e0) is in Layout.h.

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

// The object 0x413500 is a method of: the layout it deals and the share of fours
class Deal
{
public:
	Layout* mLayout = nullptr;  // +0x04
	int mQuadPercent = 0;       // +0x08
	int mArg = 0;               // +0x0c (0x413500's third argument)

	// 0x4136a0
	void Distribute(int theTiles);
};

extern int gDealCount;  // 0x533630

} // namespace mahjongjapan

#endif
