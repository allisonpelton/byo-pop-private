#ifndef __ALCHEMY_PIECE_H__
#define __ALCHEMY_PIECE_H__

// A piece: one of 22 symbols in one of 7 colours, the blank block (the "skull" of the four-way piece, it matches
// anything) or the bomb. The one to place is a widget that follows the mouse; placed ones are only data in the
// board's grid (the board draws them). Class 0x4e4798, ctor 0x40e433, size 0xc8.

#include "Widget.h"

namespace Sexy
{

class WinAlchApp;

extern const int gPieceColors[9][3];   // rdata 0x4de390: r, g, b of the colours 0..6, white (blank block, bomb) and gray

class Piece : public Widget
{
public:
	WinAlchApp*		mApp;           // +0x58
	Image*			mImage;         // +0x5c a picture to show instead (the discarded piece melting into the forge)
	int				mCol;           // +0x60 the cell it hovers over or sits in
	int				mRow;           // +0x64
	int				mShape;         // +0x68 symbol 0..21 (a bomb: 0..1, the frame pair of its fuse in the board's update)
	int				mColor;         // +0x6c 0..6; 7 for the blank block and the bomb
	int				mDelay;         // +0x70 updates before the shrink animation starts
	int				mShrink;        // +0x74 frame of the shrink animation: 0 whole .. 19 gone
	int				mShrinkDir;     // +0x78 +1 shrinking, -1 growing, 0 still
	bool			mFollowMouse;   // +0x7c
	double			mFlyX;          // +0x80 discarded: where it is, where it goes
	double			mFlyY;          // +0x88
	double			mFlyVX;         // +0x90
	double			mFlyVY;         // +0x98
	double			mSpeed;         // +0xa0 smoothed mouse speed (a discarded piece: the length of its path)
	double			mSparkleAcc;    // +0xa8 sparkles to give off
	double			mPulse;         // +0xb0 0..1.5 the red ring's alpha
	int				mPulseDir;      // +0xb8
	bool			mStuck;         // +0xbc no place to put it: the red ring pulses (beginner level)
	bool			mBomb;          // +0xbd
	int				mBombFrame;     // +0xc0 0..31
	bool			mSkull;         // +0xc4 the blank block

	Piece(WinAlchApp* theApp);      // 0x40e433

	virtual void	Update() override;                  // 0x40e4c2
	virtual void	Draw(Graphics* g) override;         // 0x40e773
};

}

#endif
