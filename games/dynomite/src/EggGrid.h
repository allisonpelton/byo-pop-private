#ifndef __EGGGRID_H__
#define __EGGGRID_H__

// The board's grid of eggs at +0x8c (vtable 0x4fa408, 0x46e8 bytes; constructor 0x415770, destructor 0x417420,
// lists at +0x466c, +0x4694 and +0x46b8; its cells are 4 bytes from +0x14, 100 to a row). Work in progress: the
// functions the eggs call are declared, their bodies come next.

namespace Sexy
{

// a cell: the egg's colour in the low byte, flags above
struct GridCell
{
	unsigned int		mBits;
};

class EggGrid
{
public:
	unsigned int		Probe(int x, int y, bool theGhost);				// 0x4169c0: bits of what is at (x, y)
	void				Break(int x, int y, GridCell* theCell);			// 0x415a20
};

}

#endif
