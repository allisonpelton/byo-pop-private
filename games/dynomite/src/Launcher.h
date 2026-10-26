#ifndef __LAUNCHER_H__
#define __LAUNCHER_H__

// The board's launcher at +0x5c (vtable 0x4f73ac, only its destructor; 0xab0 bytes; 0x401050-0x4034b0): the
// slingshot that holds the loaded egg and fires it. Work in progress: the functions not translated yet are TODO.

#include "MTRand.h"
#include "PtrList.h"

namespace Sexy
{

class Dynomite;
class Egg;
class FlyingEgg;
class FossilizedApp;
class Graphics;
class LoadedEgg;

class Launcher
{
public:
	FossilizedApp*		mApp;				// +4
	Dynomite*			mBoard;				// +8
	LoadedEgg*			mLoaded;			// +0xc
	FlyingEgg*			mFlying;			// +0x10 the last fired
	int					mAngle;				// +0x14 degrees, 0 up
	int					mField18;			// +0x18
	int					mField1c;			// +0x1c
	int					mField20[2];		// +0x20 (a point; saved)
	float				mRecoilX;			// +0x28
	float				mRecoilY;			// +0x2c
	int					mField30[2];		// +0x30 (a point; saved)
	int					mField38[20];		// +0x38 (0x50 bytes; saved)
	MTRand				mRand;				// +0x88 seeded with the tick count
	int					mFieldA4c;			// +0xa4c (the board's +0x158 + +0x70)
	int					mFieldA50;			// +0xa50 (382)
	int					mFieldA54;			// +0xa54 the slingshot's middle (as +0xa4c)
	int					mFieldA58;			// +0xa58 (432)
	int					mFieldA5c;			// +0xa5c (50)
	int					mFieldA60;			// +0xa60
	int					mFieldA64;			// +0xa64
	int					mFieldA68;			// +0xa68
	int					mFieldA6c;			// +0xa6c (255)
	int					mFieldA70;			// +0xa70
	PtrList<void>		mListA74;			// +0xa74
	PtrList<void>		mListA8c;			// +0xa8c
	bool				mFrozen;			// +0xaa4 Fire does nothing while it is set
	Egg*				mHeld;				// +0xaa8 an egg held while frozen (saved like the loaded one)
	int					mFieldAac;			// +0xaac

	Launcher(FossilizedApp* theApp, Dynomite* theBoard);	// 0x401050
	virtual ~Launcher();									// 0x401160

	void				SetLoaded(LoadedEgg* theEgg);		// 0x4011f0: unless one is loaded
	void				Load(int theColor);					// 0x401210: unless one is loaded
	void				Fire();								// 0x4012a0
	void				Freeze(Egg* theEgg);				// 0x4014f0
	void				DrawSling(Graphics* g);				// 0x401940
};

}

#endif
