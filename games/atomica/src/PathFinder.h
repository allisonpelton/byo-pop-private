#ifndef __ATOMICA_PATHFINDER_H__
#define __ATOMICA_PATHFINDER_H__

// PathFinder.cpp: the route an atom takes to the cell the player clicked (Atomica.exe 0x424880-0x425110): a search
// along straight lines through free cells, with as few turns as the cost allows, and the arrows that draw it.

#include "Support.h"

namespace Sexy
{

class Grid;
class Graphics;

class PathFinder
{
public:
	PtrList<int>	mPath;				// +4 the cells of the route: (col, row, shape of the turn, end direction)
	PtrList<int>	mPoints;			// +0x1c the route as pixel points (col * 32 + 9, row * 32 + 25 of the original)
	int				mWayCol[50];		// +0x44 the cells where the route turns (and the end)
	int				mWayRow[50];		// +0x48 (the original interleaves them: col at +0x44 + 8 i, row at +0x48 + 8 i)
	int				mCost[9][8]; // +0x1d4 the cheapest cost so far to reach each cell
	Grid*			mGrid;				// +0x2f4
	int				mBestWays;			// +0x2f8 the number of turns of the best route (-1: none)
	int				mBestCost;			// +0x2fc
	int				mSearches;			// +0x300
	int				mLength;			// +0x304 pixel length of the route

	PathFinder(Grid* theGrid);						// 0x424900
	~PathFinder();									// 0x424970
	void	Reset();								// 0x424880
	bool	Contains(int theCol, int theRow);		// 0x4248b0
	bool	Find(int theCol1, int theRow1, int theCol2, int theRow2);	// 0x424e80
	void	DrawNode(int* theNode, int theX, int theY, Graphics* g);	// 0x424b40

private:
	bool	Search(int theCol1, int theRow1, int theCol2, int theRow2, int theWantRoute, int theMaxWays, int theBuf[]); // 0x4249d0
	bool	Recurse(int theCol, int theRow, int theTargetCol, int theTargetRow, int theWays, int theMaxWays, int theCost, int* theBuffer); // 0x4246a0
};

}

#endif
