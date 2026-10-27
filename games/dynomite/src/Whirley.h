#ifndef __WHIRLEY_H__
#define __WHIRLEY_H__

// DynomitePanic's two helpers (docs/GAME_NOTES.md): the ceiling's speed (the board's +0x268) and the whirleys
// (the list at +0x24c) that Kroney's hourglass sends.

#include "EggGrid.h"

namespace Sexy
{

class BinFile;
class DynomitePanic;
class FossilizedApp;

// vtable 0x4fcc78 (only its destructor), 0x24 bytes; 0x435cd0-0x435eb0: the ceiling comes down mSpeed a frame,
// halved when the lowest row is below mHigh on the screen and raised by a quarter when it is above mLow, at most
// every mWaitReset updates; each whirley hit raises the slowest speed and mHigh
class CeilingSpeed
{
public:
	FossilizedApp*		mApp;				// +4
	DynomitePanic*		mBoard;				// +8
	int					mHigh;				// +0xc the screen y past which it slows down (200; 250 at most)
	int					mLow;				// +0x10 the screen y above which it speeds up (100)
	int					mWait;				// +0x14 updates until it changes again
	int					mWaitReset;			// +0x18 (100)
	float				mSpeed;				// +0x1c pixels an update (0.04; 0.5 at most)
	float				mMinSpeed;			// +0x20 (0.04; 0.6 at most)

	CeilingSpeed(FossilizedApp* theApp, DynomitePanic* theBoard, int theSkill);	// 0x435cd0 (theSkill unused)
	virtual ~CeilingSpeed();									// 0x435d10

	int					Update();			// 0x435d20: 2 when it slowed down, 1 when it sped up, else 0
	void				ResetSpeed();		// 0x435dc0
	void				LevelUp();			// 0x435e70: a whirley was hit
	void				Reset();			// 0x435eb0: a whirley got away
	void				SaveState(BinFile* theFile);	// 0x435dd0
	void				LoadState(BinFile* theFile);	// 0x435e20
};

// vtable 0x4ff740, 0x50 bytes; 0x441c90-0x442320: a whirley rises from below the board with an egg, slows, then
// crosses from side to side up the board; an egg that hits it (the grid's target) knocks it out and places its
// egg, and one that gets away above the board adds its egg's colour to the game's colours
class Whirley
{
public:
	FossilizedApp*		mApp;				// +4
	DynomitePanic*		mBoard;				// +8
	EggGrid*			mGrid;				// +0xc
	float				mX;					// +0x10
	float				mY;					// +0x14 (570, below the board)
	float				mVX;				// +0x18
	float				mVY;				// +0x1c (-20)
	float				mSpeed;				// +0x20 its speed across (1.2 times more each level, either way)
	float				mClimb;				// +0x24 its speed up (0.35 more each level, 12 at most)
	int					mLevel;				// +0x28
	int					mColor;				// +0x2c its egg's
	int					mState;				// +0x30 0 rising, 1 crossing, 2 knocked out
	float				mFrame;				// +0x34 0 to 12
	GridTarget			mTarget;			// +0x38 (bits 8), in the grid's targets

	Whirley(FossilizedApp* theApp, DynomitePanic* theBoard, EggGrid* theGrid, int theLevel, int theColor);	// 0x441c90
	virtual ~Whirley();						// 0x441de0

	bool				Update();			// 0x441e00: false when it has left the board
	void				Draw();				// 0x4421e0
	void				SaveState(BinFile* theFile);	// 0x4420e0
	void				LoadState(BinFile* theFile);	// 0x442150
};

}

#endif
