#ifndef __EFFECTS_H__
#define __EFFECTS_H__

// The board's effects (its list at +0xd0): the base (vtable 0x4f73e4, 0x10 bytes; 0x4034b0) and the kinds an egg's
// breaking makes (EggGrid::Break, 0x415a20). Work in progress: the kinds are added as they are translated, and
// their Save/Load slots (3, 4, 8, 9) come with the saved games.

namespace Sexy
{

class FossilizedApp;
class Pic;

class Effect
{
public:
	float				mX;				// +4
	float				mY;				// +8
	int					mType;			// +0xc 2 a yolk, 3 egg white, 7 a baby, 9 confetti

	Effect();								// 0x4034b0
	virtual ~Effect();						// 0x4034c0
	virtual void		Draw();				// 0x4b1930 (empty)
	virtual bool		Update();			// 0x4034d0: false when it is done
};

// vtable 0x4f73f8, 0x24 bytes: a baby dinosaur from a bonus egg, falling off the screen after a moment
class BabyEffect : public Effect
{
public:
	FossilizedApp*		mApp;			// +0x10
	int					mBaby;			// +0x14 0-2
	int					mDelay;			// +0x18
	float				mVY;			// +0x1c
	float				mGravity;		// +0x20

	BabyEffect();							// 0x403540
	virtual ~BabyEffect();					// 0x403560
	virtual void		Draw() override;	// 0x403700
	virtual bool		Update() override;	// 0x403620

	void				Init(int x, int y, int theBaby, FossilizedApp* theApp);	// 0x4035d0
};

// vtable 0x4fa3d8, 0x20 bytes: a star of confetti
class ConfettiEffect : public Effect
{
public:
	FossilizedApp*		mApp;			// +0x10
	int					mColor;			// +0x14 0-4
	float				mVX;			// +0x18
	float				mVY;			// +0x1c

	ConfettiEffect();						// 0x415360
	virtual ~ConfettiEffect();				// 0x415380
	virtual void		Draw() override;	// 0x415510
	virtual bool		Update() override;	// 0x415450

	void				Init(int x, int y, FossilizedApp* theApp);	// 0x415390
};

// vtable 0x4ff754, 0x1c bytes: a drop of egg white flung out of a broken egg
class EggWhiteEffect : public Effect
{
public:
	Pic*				mPic;			// +0x10 app +0x16a8
	float				mVX;			// +0x14
	float				mVY;			// +0x18

	EggWhiteEffect();						// 0x442320
	virtual ~EggWhiteEffect();				// 0x442340
	virtual void		Draw() override;	// 0x442460
	virtual bool		Update() override;	// 0x4423c0

	void				Init(int x, int y, int theAngle, FossilizedApp* theApp);	// 0x442350
};

// vtable 0x4ff76c, 0x24 bytes: a yolk (or a stick of dynamite) dropping off the screen
class YolkEffect : public Effect
{
public:
	FossilizedApp*		mApp;			// +0x10
	bool				mYolk;			// +0x14 a yolk, else a stick of dynamite
	int					mWhich;			// +0x18
	float				mVY;			// +0x1c
	float				mGravity;		// +0x20

	YolkEffect();							// 0x4424b0
	virtual ~YolkEffect();					// 0x4424d0
	virtual void		Draw() override;	// 0x442610
	virtual bool		Update() override;	// 0x442530

	void				Init(int x, int y, bool isYolk, int theWhich, FossilizedApp* theApp);	// 0x4424e0
};

}

#endif
