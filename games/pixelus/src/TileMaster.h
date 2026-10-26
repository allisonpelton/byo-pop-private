#ifndef __PIXELUS_TILEMASTER_H__
#define __PIXELUS_TILEMASTER_H__

// TileMaster.cpp: the character who stands at the edge of the puzzle and throws the tiles (Pixelus.exe 0x42bd40
// constructor, vftable 0x50f5d8; setters 0x42aec0-0x42b090, position 0x42b180, walk 0x42bf50). He walks round the
// board on a path a little way outside the grid to the side and place the player aims at, and shows one of the four
// views of the sprite sheets (tilemaster.gif: standing, tilemasterthrow.gif: the two throwing frames).
//
// port: the original steps along the path in sub-steps of 0.3 per update and turns the corners in four moves
// (0x42b180); the port walks a fixed distance per update along the same rectangle. The idle animations (scratching,
// juggling, shrugging: IMAGE_CHARACTER_* and the sprite-sheet animation set "TileMaster") are not ported.

namespace Sexy
{

class Graphics;
class PixelusApp;

class TileMaster
{
public:
	enum
	{
		FRAME_WIDTH = 45,
		FRAME_HEIGHT = 59,
		WALK_SPEED = 9			// pixels along the path per update
	};

	PixelusApp*		mApp;
	int				mGridX;			// the grid's top left corner in the board's coordinates
	int				mGridY;
	int				mCols;
	int				mRows;
	int				mSide;			// +0x6c the side he is aiming at (and, once arrived, stands at)
	int				mCursor;		// +0x40 the position along that edge
	double			mU;				// where he is on the path (pixels clockwise from the lower left corner, up the left edge first)
	double			mTargetU;
	int				mThrowTimer;	// updates left of the throw (or pull) pose
	bool			mPull;			// the pose is the pull
	int				mWalkPhase;		// counts updates while he walks

public:
	TileMaster(PixelusApp* theApp, int theGridX, int theGridY, int theCols, int theRows);

	void			SetTarget(int theSide, int theCursor, bool theJump);	// 0x42aec0
	void			Throw();												// 0x42b010
	void			Pull();
	int				GetSide() const { return mSide; }						// 0x42b090
	bool			IsWalking() const { return mU != mTargetU; }
	void			Update();												// 0x42bf50
	void			Draw(Graphics* g, int theAlpha);

private:
	double			PathLength(int* theLeftLength, int* theTopWidth) const;
	double			ToPath(int theSide, int theCursor) const;
	void			FromPath(double theU, double* theX, double* theY, int* theFacing) const;
};

}

#endif
