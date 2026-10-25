#ifndef __ATOMICA_EFFECTS_H__
#define __ATOMICA_EFFECTS_H__

// Effects.cpp: the short-lived objects that the board keeps in its effects list (Atomica.exe 0x401000-0x401570
// and 0x40c250-0x40c4c0, 0x415d20-0x41a010, 0x429e30-0x42a310). They all derive from the original's GameObj
// (vftable 0x4ae380: 0 destructor, 1 Update, 2 Draw); Update returns false when the effect is over.

#include "Support.h"
#include "Color.h"
#include <string>

namespace Sexy
{

class AtomicaApp;
class Graphics;
class Grid;
class Font;

class Effect
{
public:
	AtomicaApp*	mApp;			// +4
	float		mX;				// +8 (a position on the screen)
	float		mY;				// +0xc
	float		mVX;			// +0x10
	float		mVY;			// +0x14

	Effect(AtomicaApp* theApp) : mApp(theApp), mX(0), mY(0), mVX(0), mVY(0) {}	// 0x401000
	virtual ~Effect() {}
	virtual bool Update() = 0;
	virtual void Draw(Graphics* g) = 0;
};

// 0x418ca0: a picture that drifts and fades
class Particle : public Effect
{
public:
	Sprite*		mSprite;		// +0x18
	int			mAlpha;			// +0x1c
	int			mFade;			// +0x20
	Particle(AtomicaApp* theApp, Sprite* theSprite);
	virtual bool Update();		// 0x418ce0
	virtual void Draw(Graphics* g);	// 0x418d40
};

// 0x416660: a coloured picture that lives for a number of updates, the alpha being what is left of them
class Flash : public Effect
{
public:
	int			mLife;			// +0x18 starts at 300
	Sprite*		mSprite;		// +0x1c
	Color		mColor;			// +0x20 (the alpha is mLife)
	Flash(AtomicaApp* theApp, Sprite* theSprite, int theR, int theG, int theB, int theA);
	virtual bool Update();		// 0x4166f0
	virtual void Draw(Graphics* g);	// 0x416740
};

// 0x417a70: a dot of the electron picture that fades
class Spark : public Effect
{
public:
	int			mAlpha;			// +0x18
	Color		mColor;			// +0x1c
	Spark(AtomicaApp* theApp);
	virtual bool Update();		// 0x417ae0
	virtual void Draw(Graphics* g);	// 0x417b30
};

// 0x40c250: a ring of the big explosion
class AtomPop : public Effect
{
public:
	float		mFrame;			// +0x18
	AtomPop(AtomicaApp* theApp);
	virtual bool Update();		// 0x40c2c0
	virtual void Draw(Graphics* g);	// 0x40c310
};

// 0x418da0: the shock wave of an atomic bomb (mTier 1..3) or of a cleared bonus: a growing circle
class Blast : public Effect
{
public:
	float		mRadius;		// +0x18
	float		mGrow;			// +0x1c
	float		mFrame;			// +0x20 0 to 20 in steps of a quarter
	int			mDelay;			// +0x24
	bool		mKills;			// +0x28 the circle takes the atoms it reaches
	Grid*		mGrid;			// +0x2c
	int			mTier;			// +0x30
	Blast(AtomicaApp* theApp, int theDelay, bool theKills, int theTier);
	virtual bool Update();		// 0x418e60
	virtual void Draw(Graphics* g);	// 0x419060
};

// 0x4170d0: a picture that bursts into pieces of 2 x 2 pixels that fly out and fade
class Shatter : public Effect
{
public:
	struct Piece
	{
		float	mX, mY;			// +0 +4 from the middle of the picture
		float	mAngle;			// +8 degrees
		int		mSrcX, mSrcY;	// +0xc +0x10
		int		mW, mH;			// +0x14 +0x18 (2, 2)
		int		mSpeed;			// +0x1c (1 to 5)
	};
	Sprite*		mSprite;		// +0x18
	PtrList<Piece> mPieces;		// +0x1c
	int			mAlpha;			// +0x34
	Shatter(AtomicaApp* theApp, Sprite* theSprite, int theX, int theY);
	virtual ~Shatter();
	virtual bool Update();		// 0x416f90
	virtual void Draw(Graphics* g);	// 0x417000
};

// 0x429e30: a text that rises and fades
class FloatyText : public Effect
{
public:
	char*		mText;			// +0x18 (malloc'd copy)
	Font*		mFont;			// +0x1c
	int			mLife;			// +0x20 alpha, 2 less each update
	int			mHalfWidth;		// +0x24
	Color		mColor;			// +0x28
	FloatyText(AtomicaApp* theApp, int theX, int theY, const char* theText, Font* theFont);
	virtual ~FloatyText();
	virtual bool Update();		// 0x429dd0
	virtual void Draw(Graphics* g);	// 0x429f40
};

// 0x422860: the white flash of a nuked atom: the picture at the atom's place, fading by 2 an update
class NukeFlash : public Effect
{
public:
	int			mAlpha;			// +0x18 starts at 255
	NukeFlash(AtomicaApp* theApp);	// 0x422860
	virtual bool Update();		// 0x4228a0
	virtual void Draw(Graphics* g);	// 0x4228e0
};

// 0x421e40: the cloud of a burst atom: two pictures that run through their frames, the small one rising
class Explosion : public Effect
{
public:
	float		mFrame1;		// +0x18 the 16 frames of the atomic explosion
	float		mStep1;			// +0x1c 0.15
	float		mFrame2;		// +0x20 the 20 frames of the big explosion
	float		mStep2;			// +0x24 0.2
	float		mRise;			// +0x28 5, +0.5 an update
	Explosion(AtomicaApp* theApp);	// 0x421e40
	virtual bool Update();		// 0x421e90
	virtual void Draw(Graphics* g);	// 0x421f10
};

// 0x421680: the earth element's bolt: it runs through the field a cell at a time (every fourth update), taking the atom
// of each cell it reaches; the port draws sparks along it
class EarthBolt : public Effect
{
public:
	float		mDX, mDY;		// +0x10 +0x14 the direction (-1, 0 or 1)
	int			mDirTimer;		// +0x18 updates to a new direction
	int			mStepTimer;		// +0x1c updates to the next step
	EarthBolt(AtomicaApp* theApp);	// 0x421680
	virtual bool Update();		// 0x421770
	virtual void Draw(Graphics* g);
};

// 0x431300: the air element's whirlwind: it wanders around the middle of the field for up to 2000 updates and takes the
// atom it passes over; 100 particles of the electron picture spiral outward
class Vortex : public Effect
{
public:
	struct Mote
	{
		float	mRadius;		// +0 (grows by 0.85 percent an update)
		float	mAge;			// +4 0 to 50
		float	mAngle;			// +8 degrees
		float	mSpeed;			// +0xc degrees an update
	};
	PtrList<Mote>	mParticles;		// +0x1c
	int			mLife;			// +0x18 updates left
	float		mDistance;		// +0x3c distance from the middle of the field
	float		mDistanceTarget;// +0x40
	float		mAngle;			// +0x44
	float		mAngleSpeed;	// +0x48
	float		mAngleSpeedTarget; // +0x4c
	int			mTimer;			// +0x50 updates to new targets
	int			mCenterX;		// +0x58
	int			mCenterY;		// +0x5c
	Mote*		mFirst;			// +0x34
	bool		mMoving;		// +0x38
	Vortex(AtomicaApp* theApp, int theX, int theY);	// 0x431300
	virtual ~Vortex();
	virtual bool Update();		// 0x431510
	virtual void Draw(Graphics* g);	// 0x431850
};

// 0x417300: a flash of lightning between two cells of the field (the bond of a molecule), drawn as a jagged
// line of dots and lines
class Bolt : public Effect
{
public:
	Grid*		mGrid;			// +0x28
	int			mCol1, mRow1, mCol2, mRow2;	// +0x18 +0x1c +0x20 +0x24
	int			mTwist;			// +0x2c
	Color		mColor;			// +0x30
	Bolt(AtomicaApp* theApp, Grid* theGrid, int theCol1, int theRow1, int theCol2, int theRow2, const Color& theColor, bool thePlaySound);
	virtual bool Update();		// 0x417270
	virtual void Draw(Graphics* g);	// 0x4173f0
	void		DrawPixels(Graphics* g, int theX1, int theY1, int theX2, int theY2);
};


// 0x41c5f0: a spark that runs along the edge of the field while it is revealed, with a trail of 20 points
class Sparkle : public Effect
{
public:
	int			mDir;			// +0x18 which way up or down it runs
	int			mSpeed;			// +0x1c steps an update (2 to 4)
	int			mTimer;			// +0x20 steps to a change of direction
	bool		mDead;			// +0x24
	float		mTrail[20][2];	// +0x28 the last points
	int			mTrailIndex;	// +0xc8
	int			mAlpha;			// +0xcc
	bool		mRevealing;		// +0xd0 it lights the field's first column as it passes
	bool		mFading;		// +0xd1
	int			mFadeStep;		// +0xd4
	Sparkle(AtomicaApp* theApp, int theCol, int theRow, int theCorner);
	void		Reset();		// 0x41c4c0
	virtual bool Update();		// 0x41c710
	virtual void Draw(Graphics* g);	// 0x41c890
};

}

#endif
