#ifndef __LAUNCHER_H__
#define __LAUNCHER_H__

// The board's launcher at +0x5c (vtable 0x4f73ac, only its destructor; 0xab0 bytes; 0x401050-0x4034b0): the
// slingshot that holds the loaded egg and fires it. Work in progress: the functions not translated yet are TODO.

#include "MTRand.h"
#include "Point.h"
#include "PtrList.h"

namespace Sexy
{

class Dynomite;
class Egg;
class FlyingEgg;
class FossilizedApp;
class Graphics;
class LoadedEgg;

// a line of the aim's path: two points of two floats (the original's pair of pointers on the stack)
struct PathLine
{
	float*				mFrom;
	float*				mTo;
};

class Launcher
{
public:
	FossilizedApp*		mApp;				// +4
	Dynomite*			mBoard;				// +8
	LoadedEgg*			mLoaded;			// +0xc
	FlyingEgg*			mFlying;			// +0x10 the last fired
	int					mAngle;				// +0x14 degrees, 0 up
	int					mField18;			// +0x18 (the hand's cell is 8 - this / 8)
	int					mField1c;			// +0x1c
	int					mField20[2];		// +0x20 the hand's middle (saved)
	float				mRecoilX;			// +0x28
	float				mRecoilY;			// +0x2c
	int					mField30[2];		// +0x30 the egg's place mirrored to 0.4 of its distance (saved)
	int					mField38[20];		// +0x38 (0x50 bytes; saved; the first two the band's pull from the egg)
	MTRand				mRand;				// +0x88 seeded with the tick count
	int					mAimX;				// +0xa4c the point aimed at (the board's +0x158 + +0x70)
	int					mAimY;				// +0xa50 (382)
	int					mSlingX;			// +0xa54 the slingshot's middle (as the aim)
	int					mSlingY;			// +0xa58 (432)
	int					mFieldA5c;			// +0xa5c (50; counts down from 24 again and again)
	int					mFieldA60;			// +0xa60 eases a pixel a frame toward +0xa64, never below +0xa68 unless
										// the board's bonus bit 1 is on
	int					mFieldA64;			// +0xa64 (-600 while the board's bonus bit 1 is on, else 0)
	int					mFieldA68;			// +0xa68 how far below the aim the last path ended
	int					mFieldA6c;			// +0xa6c (255) the arrow's red
	int					mFieldA70;			// +0xa70 the arrow's green and blue
	PtrList<Point>		mPathRight;			// +0xa74 the aim path's right side, first to last
	PtrList<Point>		mPathLeft;			// +0xa8c its left side, last to first
	bool				mFrozen;			// +0xaa4 Fire does nothing while it is set
	LoadedEgg*			mHeld;				// +0xaa8 an egg held while frozen, loaded 56 frames on (saved like the loaded one)
	int					mFieldAac;			// +0xaac

	Launcher(FossilizedApp* theApp, Dynomite* theBoard);	// 0x401050
	virtual ~Launcher();									// 0x401160

	void				SetLoaded(LoadedEgg* theEgg);		// 0x4011f0: unless one is loaded
	void				Load(int theColor);					// 0x401210: unless one is loaded
	void				Fire();								// 0x4012a0
	void				Update();							// 0x402c10
	void				Extend(PathLine* theLine, float theDist);	// 0x4013f0: moves its end on
	void				Retract(PathLine* theLine, float theDist);	// 0x401470: moves its start back
	void				Freeze(LoadedEgg* theEgg);			// 0x4014f0
	void				DrawSling(Graphics* g);				// 0x401940
	void				DrawBand();							// 0x401a10
	void				PlaceEgg(float& theX, float& theY);	// 0x401d30
	void				Aim(int theX, int theY);			// 0x401e40
	void				DrawArrow(Graphics* g);				// 0x401ed0
	// 0x4021e0: the line's two points each moved theWidth to either side (x, y, x, y)
	void				Widen(PathLine* theLine, float* theFrom, float* theTo, float theWidth);
	void				AddPath(Graphics* g, PathLine* thePrev, PathLine* theLine, PathLine* theNext);	// 0x4022f0
	void				DrawPath(Graphics* g);				// 0x402d90
	void				Draw(Graphics* g);					// 0x4033a0
};

}

#endif
