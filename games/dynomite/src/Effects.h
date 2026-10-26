#ifndef __EFFECTS_H__
#define __EFFECTS_H__

// The board's effects (its lists at +0xd0 and +0xe8): the base (vtable 0x4f73e4, 0x10 bytes; 0x4034b0) and the
// kinds an egg's breaking makes (EggGrid::Break, 0x415a20), and the power-ups (the board's list at +0x10c). Work in
// progress: the kinds are added as they are translated, and their Save/Load slots (3, 4) come with the saved games.

namespace Sexy
{

class Dynomite;
class FossilizedApp;
class Pic;

class Effect
{
public:
	float				mX;				// +4
	float				mY;				// +8
	int					mType;			// +0xc 1 a falling egg, 2 a yolk, 3 egg white, 4 a boom, 5 a shell, 6 points, 7 a baby, 8 a bonus, 9 confetti

	Effect();								// 0x4034b0
	virtual ~Effect();						// 0x4034c0
	virtual void		Draw();				// 0x4b1930 (empty)
	virtual bool		Update();			// 0x4034d0: false when it is done
};

// vtable 0x4fa3a0, 0x24 bytes: an egg that lost its hold, falling off the screen (colours from 50 up are the
// mode's pieces, drawn by the board's slot 57 and reported by slot 58)
class FallingEggEffect : public Effect
{
public:
	FossilizedApp*		mApp;			// +0x10
	int					mColor;			// +0x14
	int					mKind;			// +0x18
	float				mVY;			// +0x1c
	float				mGravity;		// +0x20

	FallingEggEffect();						// 0x4147c0
	virtual ~FallingEggEffect();			// 0x4147e0
	virtual void		Draw() override;	// 0x414930
	virtual bool		Update() override;	// 0x414840

	void				Init(FossilizedApp* theApp, int x, int y, int theColor, int theKind);	// 0x4147f0
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

// vtable 0x4fcb6c, 0x28 bytes: a quarter of a broken egg's shell
class ShellEffect : public Effect
{
public:
	FossilizedApp*		mApp;			// +0x10
	int					mColor;			// +0x14
	int					mQuarter;		// +0x18 0-3
	float				mVX;			// +0x1c
	float				mVY;			// +0x20
	float				mGravity;		// +0x24

	ShellEffect();							// 0x434b90
	virtual ~ShellEffect();					// 0x434bb0
	virtual void		Draw() override;	// 0x434d60
	virtual bool		Update() override;	// 0x434bc0

	void				Init(int x, int y, int theAngle, int theColor, int theQuarter, FossilizedApp* theApp);	// 0x434cc0
};

// vtable 0x4f7444, 0x20 bytes: a stick of dynamite's explosion, rising slowly through the 15 frames of the boom sheet
class BoomEffect : public Effect
{
public:
	FossilizedApp*		mApp;			// +0x10
	float				mFrame;			// +0x14
	float				mSpeed;			// +0x18 frames a frame
	bool				mStretched;		// +0x1c 150 x 200: a bomb's (EggGrid::Blast)

	BoomEffect();							// 0x403e20
	virtual ~BoomEffect();					// 0x403e40
	virtual void		Draw() override;	// 0x403f60
	virtual bool		Update() override;	// 0x403e90

	void				Init(int x, int y, FossilizedApp* theApp, float theSpeed);	// 0x403e50
};

// vtable 0x4f742c, 0x2c bytes: a bonus egg's prize flying to the power-ups' circle (20 frames), where it becomes
// a power-up
class BonusItemEffect : public Effect
{
public:
	float				mTargetX;		// +0x10
	float				mTargetY;		// +0x14
	float				mVX;			// +0x18
	float				mVY;			// +0x1c
	int					mKind;			// +0x20 1-10
	Dynomite*			mBoard;			// +0x24
	int					mFrames;		// +0x28

	BonusItemEffect();						// 0x403c30
	virtual ~BonusItemEffect();				// 0x403c50
	virtual void		Draw() override;	// 0x403dc0
	virtual bool		Update() override;	// 0x403cc0

	void				Init(int x, int y, int theKind, int theTargetX, int theTargetY, Dynomite* theBoard);	// 0x403c60
};

// vtable 0x4fcb50, 0x2c bytes: points rising from where they were scored (the board's list at +0xe8)
class ScorePopup : public Effect
{
public:
	char*				mText;			// +0x10
	int					mLength;		// +0x14
	bool				mBlink;			// +0x18
	int					mBlinkCount;	// +0x1c
	int					mLife;			// +0x20
	float				mSpeed;			// +0x24
	FossilizedApp*		mApp;			// +0x28

	ScorePopup();							// 0x4346a0
	virtual ~ScorePopup();					// 0x4346d0
	virtual void		Draw() override;	// 0x434900
	virtual bool		Update() override;	// 0x4347d0

	void				Init(int x, int y, const char* theText, FossilizedApp* theApp);	// 0x434700
};

// vtable 0x4f7420 (only its destructor), 0x2c bytes: a power-up circling the board's centre for power-ups; kinds 8
// to 10 pay their points at once
class PowerUp
{
public:
	Dynomite*			mBoard;			// +4
	int					mAngle;			// +8
	int					mSpin;			// +0xc the last Draw's
	int					mKind;			// +0x10 1-10
	int					mTime;			// +0x14
	int					mBlinkTime;		// +0x18 it blinks for its last this many frames
	float				mRadius;		// +0x1c
	bool				mVisible;		// +0x20
	int					mBlinkCount;	// +0x24
	int					mFlag;			// +0x28 1 << (mKind - 1)

	PowerUp(Dynomite* theBoard, int theKind, int theAngle);	// 0x403750
	virtual ~PowerUp();											// 0x4038c0

	bool				Update();								// 0x4038d0: false when it is done
	void				Draw(int theSpin);						// 0x403ba0
};

}

#endif
