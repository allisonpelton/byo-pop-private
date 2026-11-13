#ifndef __HAMMERHEADS_IMAGEBLAST_H__
#define __HAMMERHEADS_IMAGEBLAST_H__

// A picture that breaks into square pieces and flies apart (0x43fb10-0x440730): what is left of a creature when it is
// smashed. A cubby owns it, steps it (Update) and draws it in two passes. Not an Object.

#include <vector>

#include "Object.h"
#include "Rect.h"

namespace Sexy
{

class Graphics;
class Image;
class Cubby;

// One piece: the part of the picture it shows (a Rect, drawn from by pointer) and its flight
struct BlastPiece
{
	Rect			mSrcRect;		// +0x00
	double			mX;				// +0x10
	double			mY;				// +0x18
	double			mVelocityX;		// +0x20
	double			mVelocityY;		// +0x28: gravity 0.13 a step
	double			mAngle;			// +0x30
	double			mSpin;			// +0x38
	double			mLife;			// +0x40: counts down from about 1 by 0.01 a step; below 0.5 it fades
	double			mLifeStart;		// +0x48: kept, never read
	double			mFloorY;		// +0x50: the piece bounces when it comes down past this
	bool			mBounced;		// +0x58: it has been above the floor, so that a bounce may happen
	bool			mFront;			// +0x59: drawn in the pass with this value (see ImageBlast::Draw)
};

class ImageBlast
{
public:
	int				mTick;			// +0x08
	double			mX;				// +0x10
	double			mY;				// +0x18
	Rect			mSrcRect;		// +0x20: the part of the image that breaks up
	int				mStyle;			// +0x30: 2 steps only every fourth tick
	double			mRadius;		// +0x38: half a piece's diagonal; a piece is drawn only when it is this far inside the screen
	int				mRows;			// +0x40
	int				mCols;			// +0x44
	Image*			mImage;			// +0x48
	bool			mShatter;		// +0x4c: faster and wilder
	std::vector<BlastPiece> mPieces;	// +0x50
	ObjectRef		mCubby;			// +0x60: the cubby it came from (its cover is drawn over the pieces)
	bool			mFlag;			// +0x80
	bool			mFinished;		// +0x81: every piece has faded

public:
	ImageBlast(int theStyle, double theX, double theY, Image* theImage, const Rect& theSrcRect, int theRows, int theCols,
		double theGroundY, Cubby* theCubby, bool theFlag, bool theShatter);	// 0x440030
	~ImageBlast();										// 0x43fb10

	void			Update();							// 0x43fb70: one 0.01 step
	void			Draw(Graphics* g, bool theFront);	// 0x43fca0: the pieces of one pass; the front pass also draws the cubby's cover
};

// 0x440730
ImageBlast* MakeImageBlast(int theStyle, double theX, double theY, Image* theImage, const Rect* theSrcRect, int theRows,
	int theCols, double theGroundY, Cubby* theCubby, bool theFlag, bool theShatter);

}

#endif
