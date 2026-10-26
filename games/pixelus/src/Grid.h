#ifndef __PIXELUS_GRID_H__
#define __PIXELUS_GRID_H__

// Grid.cpp: the rules of Pixelus' puzzle ("PuzzPix::Grid", Pixelus.exe 0x421a30-0x426910; vftable 0x50f0f4, ctor
// 0x425d40). The original's Grid also draws itself and animates its tiles; the port keeps the rules here, free of the
// framework (so that tools/ and the tests can replay the shipped solutions on it), and the picture and the flying
// tiles in Board.cpp.
//
// A puzzle is a grid of at most 32x32 cells (16x16 in the shipped ones). The Tile Master stands on one of the four
// sides (0 below, throwing up; 1 left, throwing right; 2 above, throwing down; 3 right, throwing left, which the
// move strings call U, R, D, L) at a cursor position along that edge and throws a tile along the row or column: it
// slides until the next cell holds something, and stays at the cell before it. With nothing in its way it flies
// out of the other side (and is lost); the cell next to the thrower being taken, it cannot be thrown at all.
// Pulling takes the first tile of that line back, unless it is a fixed block or a start cell's tile. Cells: wanted
// (the picture), fixed ('X' blocks), bouncy ('B' blocks that throw a tile back) and start ('S').

#include <string>
#include <vector>

namespace Sexy
{

// 0x42eee0: a tile (or a fixed block) with the cell it rests in. The fields from mX on are the Board's animation of it
// (0x42f740): plain numbers, so the rules stay free of the framework. Pixel positions are relative to the grid's top
// left corner, a cell is 20 pixels wide.
struct GridObject
{
	int		mRow;
	int		mCol;
	bool	mFixed;

	int		mX;				// +0x8 position in pixels
	int		mY;				// +0xc
	int		mDir;			// +0x18 the side the tile was thrown from (0 up, 1 right, 2 down, 3 left)
	int		mVariant;		// +0x1c which of the five pictures of its colour
	bool	mDead;			// +0x24 gone: not drawn, not updated
	bool	mBounce;		// +0x3c a thrown tile that hits a bouncy block flies back
	int		mTrailLen;		// +0x40 how much of the trail picture shows
	int		mState;			// +0x44 0 at rest, 1 returning, 2 flying to its cell
	int		mIdle;			// +0x48 ticks at rest; the shine plays below 100
	int		mTrail;			// +0x4c the trail's opacity, 0-200
	int		mFlash;			// +0x20 ticks until the picture changes at random (0x42f740 counts it down; the constructor
							// leaves it alone and nothing sets it, so it stays 0)

	// the push a tile gets when another lands in front of it (0x42ec40, 0x42f740): out along the direction by a pixel an
	// update up to mNudgeMax, a wait of mNudgeDelay updates, and back
	bool	mNudge;			// +0x25
	bool	mNudgeOut;		// +0x26 still going out
	int		mNudgeDir;		// +0x28
	int		mNudgeAmount;	// +0x2c pixels it is pushed now
	int		mNudgeMax;		// +0x30
	int		mNudgeStep;		// +0x34
	int		mNudgeDelay;	// +0x38
};

class Grid
{
public:
	enum { MAX_SIZE = 32, CELL_SIZE = 20 };
	enum { SIDE_UP = 0, SIDE_RIGHT = 1, SIDE_DOWN = 2, SIDE_LEFT = 3 };

	// the cell at 0x40 + (row * 32 + col) * 0x2c: +0x10 wanted, +0x11 fixed, +0x14 object, +0x18 bouncy, +0x19 start
	struct Cell
	{
		bool		mWanted;
		bool		mFixed;
		bool		mBouncy;
		bool		mStart;
		GridObject*	mObject;
	};

	// one entry of the move history (+0xb114): {side, cursor, -1} for a thrown tile, {side, cursor, row or column of
	// the pulled tile} for a pulled one; the strings are the kind's letter (A, R, B or F), the side letter and two
	// two-digit numbers
	struct Move
	{
		int		mSide;
		int		mCursor;
		int		mPosition;
		bool	mRemove;		// a pull (kind 1)
		int		mKind;			// +0xc the letter: 0 A a throw that stayed, 1 R a pull, 2 B a throw that bounced back, 3 F a
								// tile that flew out of the grid
	};

public:
	int			mRows;				// +0x30
	int			mCols;				// +0x34
	Cell		mCells[MAX_SIZE][MAX_SIZE];
	std::vector<GridObject*> mObjects;	// +0xb040 every tile ever made (the cells point into it)

	int			mSupply;			// +0xb050 tiles on the workbench
	int			mCursor;			// +0xb054 position along the edge
	int			mSide;				// +0xb058
	bool		mDone;				// +0xb05c
	int			mMoveCount;			// +0xb060
	int			mTime;				// +0xb064 milliseconds spent on the puzzle (10 per update until it is done and still)

	// the finish (0x422590, 0x424b30, 0x421e10): a delay of 84 updates, a sweep of the picture over the board and a
	// shimmer across the tiles; the puzzle-completed countdown waits for them
	bool		mTileMoving;		// +0xb0a8 a tile was in flight at the last update
	double		mGlow;				// +0xb108 the delay: 1.0 at the last throw, down by 0.03 an update to -1.5
	int			mReveal;			// +0xb100 cells still to show as the picture: rows * cols, down by 3 an update
	bool		mGlowDone;			// +0xb110 the sweep is over
	bool		mWaveOn;			// +0xf the shimmer runs
	double		mWave;				// +0x10 where the shimmer is, in diagonals: -25 up to 60 by 0.6 an update

	bool		mCanPlace;			// +0xb069 a thrown tile would stay
	bool		mSlidesOut;			// +0xb068 a thrown tile would leave the grid
	int			mLandCol;			// +0xb06c where it would stay (or leave the grid)
	int			mLandRow;			// +0xb070
	int			mStartCol;			// +0xb074 the edge cell it enters
	int			mStartRow;			// +0xb078
	bool		mCanPull;			// +0xb07c the tile it would hit can be pulled back
	int			mStuckCol;			// +0xb080 the first cell on the line that is taken (or the last cell of the line)
	int			mStuckRow;			// +0xb084

	// the finished picture (0x426910 reads images/levels/<name>.gif; the Board and the thumbnails do that, this class
	// is free of the framework): the colour of each cell, ARGB, already brightened
	bool		mHasPicture;
	unsigned int mPicture[MAX_SIZE][MAX_SIZE];

	int			mEmptyWanted;		// +0xb164 wanted cells still without a tile
	int			mStrayTiles;		// +0xb168 tiles on cells nobody wants
	int			mWantedCells;		// +0xb16c

	std::string	mTitle;				// +0xb0b0
	std::string	mName;				// +0xb0cc
	std::string	mUuid;				// +0xb0ec (text; the original keeps the 16 bytes)
	int			mDifficulty;		// +0xb0e8
	bool		mHasStart;			// +0xb18c a start cell exists
	std::vector<Move> mHistory;		// +0xb114

public:
	Grid(int theRows = 16, int theCols = 16);	// 0x425d40
	~Grid();									// 0x4256b0

	void			Clear();									// 0x4262c0
	bool			LoadFromFile(const std::string& thePath, std::string* theError = NULL);	// 0x426910
	bool			LoadFromText(const std::string& theText, std::string* theError = NULL);

	// 0x4220a0: the cell of the edge the tile enters at
	void			EdgeCell(int theSide, int theCursor, int* theCol, int* theRow) const;
	bool			AtFarEdge(int theCol, int theRow, int theSide) const;
	void			Step(int* theCol, int* theRow, int theSide, int theCount) const;	// 0x4030a0
	bool			InGrid(int theRow, int theCol) const;
	bool			HasObject(int theRow, int theCol) const { return mCells[theRow][theCol].mObject != NULL; }

	void			SetCursor(int theSide, int theCursor);		// 0x424590
	void			ComputeLanding();							// 0x423ee0
	void			Recount();									// 0x422320
	GridObject*		NewObject(int theRow, int theCol);			// 0x424390
	GridObject*		PutTile(int theRow, int theCol);			// 0x4245c0: a tile on a free cell (taking back a pull)
	bool			IsBouncyAhead() const;						// the cell behind the landing cell is a bouncy block

	void			Nudge(int theRow, int theCol, int theDir);	// 0x423d30: push the tiles in line behind a tile that landed
	void			UpdateFinish();								// 0x424b30 after the tiles: the delay, the sweep and the shimmer
	bool			IsFinished() const;							// 0x421e10: the countdown to the puzzle-completed screen may run

	bool			PlaceTile(bool theRecord);					// 0x424750: throw a tile
	bool			PullTile(bool theRecord, bool theForce);	// 0x424990: pull the tile in the way back

	// 0x425b40 and 0x4257e0: the move strings
	static bool		ParseMove(const std::string& theString, Move* theMove);
	static std::string MoveToString(const Move& theMove);
	bool			ReplayMove(const Move& theMove);

	// the mouse: the side the Tile Master throws from to land a tile on the free cell (0x4253f0), or to pull
	// the tile on the cell back (0x425220); the preferred side if it works, else the first; -1 if none
	int				FindThrowSide(int theRow, int theCol, int thePreferredSide, int* theCursor) const;
	int				FindPullSide(int theRow, int theCol, int thePreferredSide, int* theCursor) const;

private:
	void			Complete();									// 0x422590
};

}

#endif
