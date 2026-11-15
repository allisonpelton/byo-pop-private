#ifndef __PIXELUS_TILEMASTER_H__
#define __PIXELUS_TILEMASTER_H__

#include <vector>

// TileMaster.cpp: the character who stands beside the puzzle and throws the tiles (Pixelus.exe 0x42bd40 constructor, vftable
// 0x50f5d8; update 0x42c460, draw 0x42b6a0). Two things in one object:
//  - the trail: a glowing dot that runs round the outside of the grid to the place the player aims at, leaving puffs of
//    light that fade in 8 updates (the walker at +0xc: update 0x42bf50, place 0x42b180, direction 0x42ad90);
//  - the Tile Master himself, who does not walk: one update after the aim changes he is at the place beside the grid
//    (0x42b8b0), facing it, and he plays a sprite-sheet animation of the NcAnimation set "TileMaster" (built at 0x437b90):
//    the stand, the throw, and the idle actions he starts by himself (scratching, juggling) after 400 to 599 updates.

namespace Sexy
{

class Graphics;
class PixelusApp;
class Image;

class TileMaster
{
public:
	enum
	{
		FRAME_WIDTH = 45,
		FRAME_HEIGHT = 59,
		PIVOT_X = 21,			// frames are drawn this far left of and (PIVOT_Y) above the place he stands at
		PIVOT_Y = 55,
		PIVOT_Y_CELEBRATE = 74,
		PUFF_LIFE = 8
	};

	// the actions of the set, in the order 0x437b90 lists them
	enum
	{
		ACTION_STAND = 0,		// four rows of one frame, one per side
		ACTION_THROW = 1,		// four rows of two frames
		ACTION_SCRATCH = 2,
		ACTION_JUGGLE = 3,
		ACTION_JUGGLE2 = 4,
		ACTION_SHRUG = 5,
		ACTION_CELEBRATE = 6,
		ACTION_COUNT = 7
	};

	struct Puff
	{
		int			mLife;		// +0 updates left
		int			mX;			// +4 the middle of the light
		int			mY;			// +8
	};

	PixelusApp*		mApp;
	int				mGridX;			// +0x38 the grid's top left corner in the board's coordinates
	int				mGridY;
	int				mCols;
	int				mRows;

	// the trail (0x42bf50): the place on the way round, the place aimed at, how it goes and how far into the step it is
	std::vector<Puff> mPuffs;			// +0xc
	int				mTrailSide;			// +0x1c
	int				mTrailPos;			// +0x20
	int				mTrailTargetSide;	// +0x24
	int				mTrailTargetPos;	// +0x28
	int				mTrailMode;			// +0x30 0 stopped, 1 round the way the sides count up, 2 back
	float			mTrailPhase;		// +0x34 in cells (a corner takes 1 to 4 of them)

	// the Tile Master
	int				mSide;				// +0x40 the side aimed at
	int				mStandSide;			// +0x48 the side he stands at (0x42b090 gives this one)
	int				mCursor;			// +0x6c the position along that side
	int				mSettleDelay;		// +0x70 updates until he goes to the place aimed at
	int				mStandX;			// +0x54 where he stands (the middle of the cell beside the grid)
	int				mStandY;			// +0x58
	bool			mIdling;			// +0x74 an idle action is running
	int				mIdleAction;		// +0x78
	int				mIdleRepeats;		// +0x7c
	int				mIdleDelay;			// +0x80 updates until the next idle action
	bool			mFrozen;			// +0x88 the puzzle is done: he stays where he is

	// the animation player (0x436dc0): action, row of the sheet (the side he faces), frame and how long it has been shown
	int				mAction;			// +4 of the player
	int				mRow;				// +0xc
	int				mFrame;				// +0x10
	int				mTick;				// +0x14
	bool			mBackwards;			// +0x18

public:
	TileMaster(PixelusApp* theApp, int theGridX, int theGridY, int theCols, int theRows);

	void			SetTarget(int theSide, int theCursor, bool theJump);	// 0x42aec0 (theJump: the original does not)
	void			Throw();												// 0x42bb20, for a throw and for a pull
	void			Celebrate();											// 0x42b010
	void			Shrug();												// 0x42b030
	void			Freeze();												// 0x42afc0
	void			IdleSoon() { mIdleDelay = 1; }							// 0x42af70
	int				GetSide() const { return mStandSide; }					// 0x42b090
	void			Update();												// 0x42c460
	void			Draw(Graphics* g, int theAlpha);						// 0x42b6a0

private:
	void			SetAction(int theAction);								// 0x436f30
	void			SetRow(int theRow);										// 0x436f10
	bool			AdvanceAnimation();										// 0x436dc0
	void			Settle();												// 0x42b8b0
	void			UpdateIdle();											// 0x42b7a0
	void			EndIdle();												// 0x42af80
	void			CancelIdle();
	void			SetTrailTarget(int theSide, int theCursor);				// 0x42ad90
	void			UpdateTrail();											// 0x42bf50
	void			TrailPlace(int* theX, int* theY) const;					// 0x42b180
	void			CellCenter(int theRow, int theCol, int* theX, int* theY) const;	// 0x423b90 with the middle
};

}

#endif
