#ifndef __EGG_H__
#define __EGG_H__

// The eggs (docs/GAME_NOTES.md section 4): the base (vtable 0x4fa3f4, 0x20 bytes; 0x4155a0-0x415770), the egg in
// the launcher (vtable 0x4f73a0, 0x401000) and the egg in flight (vtable 0x4fa3b8, 0x38 bytes; 0x414d10-0x415360).

namespace Sexy
{

class Dynomite;
class FossilizedApp;

class Egg
{
public:
	int					mColor;			// +4 the egg picture (app +0x13c0); 0 sparkles
	int					mKind;			// +8 a picture over it (app +0x1830), 0 none
	float				mSparkle;		// +0xc the sparkle's frame (0 to 3)
	FossilizedApp*		mApp;			// +0x10
	Dynomite*			mBoard;			// +0x14
	float				mX;				// +0x18 its middle
	float				mY;				// +0x1c

	Egg();												// 0x4155a0
	virtual ~Egg();										// 0x4155c0
	virtual bool		Update();						// 0x4155f0
	virtual void		Init(FossilizedApp* theApp, Dynomite* theBoard, int theColor);	// 0x4155d0

	void				Draw();							// 0x415660
};

// 0x401000: the egg in the launcher
class LoadedEgg : public Egg
{
public:
	LoadedEgg();										// 0x401000
	virtual ~LoadedEgg();								// 0x401020
};

// 0x414d10: the egg in flight, until it lands in the grid
class FlyingEgg : public Egg
{
public:
	bool				mField20;		// +0x20 (cleared when it crosses the launcher's middle)
	float				mVX;			// +0x24
	float				mVY;			// +0x28
	bool				mGhost;			// +0x2c
	int					mCol;			// +0x30 the cell it landed in
	int					mRow;			// +0x34

	FlyingEgg();										// 0x414d10
	virtual ~FlyingEgg();								// 0x414d30
	virtual bool		Update() override;				// 0x4150b0: false when it has landed

	void				Land();							// 0x414d60
};

}

#endif
