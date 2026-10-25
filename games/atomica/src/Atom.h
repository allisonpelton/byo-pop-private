#ifndef __ATOMICA_ATOM_H__
#define __ATOMICA_ATOM_H__

// Atom.cpp: the atoms on the field (Atomica.exe 0x401060-0x403100 and 0x4037f0-0x404390) and the five element
// atoms (0x4043d0-0x405700). Behaviour spec: docs/GAME_NOTES.md section 4.
//
// Original: `Atom` (vftable 0x4ae3bc, 0xf0 bytes, 9 virtual slots: 0 destructor, 1 Update, 2 Draw, 3 DrawAimLine,
// 4 DrawOverlay, 5 nothing, 6 Save, 7 Load, 8 IsElement) and `ElementAtom` (0x4ae478, 0x128 bytes). The
// fields keep the original's offsets in the comments; the ones whose meaning is not clear yet are named
// after their offset (mF84: a float at +0x84).

#include "Support.h"
#include "Color.h"

namespace Sexy
{

class AtomicaApp;
class Grid;
class Graphics;

enum AtomKind
{
	KIND_PLAIN = 0,
	KIND_CHROMIUM = 1,		// matches any atom
	KIND_BIOHAZARD = 2		// spreads until it is destroyed
};

// a ring of electrons: its angle in degrees, its speed in degrees per update, and which ring
struct Electron
{
	float	mAngle;
	float	mSpeed;
	int		mRing;
};

class Atom
{
public:
	bool		mPre;				// +4 still a pre-atom: the spot is reserved and the atom has not formed yet
	bool		mPreStarted;		// +5 the pre-atom has started to form
	bool		mDead;				// +6 the grid removes it from its list at the next update
	AtomicaApp*	mApp;				// +8
	Grid*		mGrid;				// +0xc
	int			mCol;				// +0x10 cell of the field (0 to 8)
	int			mRow;				// +0x14 (0 to 7)
	int			mX;					// +0x18 position on the screen of the atom's centre
	int			mY;					// +0x1c
	float		mDirX;				// +0x20 unit direction of the aim line
	float		mDirY;				// +0x24
	float		mAimDist;			// +0x28 distance to the cursor
	float		mScale;				// +0x2c
	int			mAimAngle;			// +0x30 degrees to the cursor
	bool		mPlaced;			// +0x34 sits in a cell
	bool		mFollowing;			// +0x35 follows the path in mPath
	int*		mPathNode;			// +0x38 the node it came from (col, row)
	int*		mNextNode;			// +0x3c the node it goes to
	int			mSpeed;				// +0x40 steps a path takes per update (1..3)
	Color		mColor;				// +0x44 the colour of the atom's type (alpha at +0x50)
	PtrList<Electron> mElectrons;	// +0x54
	PtrList<int>	mPath;			// +0x6c the cells of the path it follows (pairs of ints)
	int			mWobble;			// +0x84 0 to 0x80
	int			mWobbleStep;		// +0x88 +2 or -2
	int			mType;				// +0x8c the picture and the colour: 0 to 10 plain, 11 chromium, 12 to 26 elements
	float		mSpawn;				// +0x90 the pre-atom's timer
	float		mFlyX;				// +0x94 fly-away position
	float		mFlySize;			// +0x98
	float		mFlyTurn;			// +0x9c
	float		mFlyGrow;			// +0xa0
	int			mAlpha;				// +0xa8
	bool		mFlag_ac;			// +0xac
	bool		mHidden;			// +0xad hidden until the field's reveal wave reaches it
	float		mF_b0;				// +0xb0
	float		mBounce;			// +0xb4 0 to 5
	float		mBounceStep;		// +0xb8
	int			mBounceTick;		// +0xbc
	int			mKind;				// +0xc0 AtomKind
	int			mKindArg;			// +0xc4 the biohazard's spread timer
	float		mSpin;				// +0xc8 degrees
	int			mFade;				// +0xcc
	float		mGlow;				// +0xd0 64 to 255
	float		mGlowStep;			// +0xd4
	bool		mNear;				// +0xd8
	int			mNearAlpha;			// +0xdc
	bool		mFlag_e0;			// +0xe0 (element atoms)
	int			mE4, mE8, mEC;		// +0xe4 +0xe8 +0xec

public:
	// 0x4015d0: the grid's atom of a type; theNoSpecial: not a chromium or biohazard atom by chance
	Atom(Grid* theGrid, int theType, bool theNoSpecial);
	virtual ~Atom();									// 0x401060

	void			RemoveFromCell();					// 0x4011f0
	void			SetPath(PtrList<int>* thePath);		// 0x401230
	virtual void	DrawAimLine(Graphics* g);			// 0x401c60
	bool			PlaceAt(int theCol, int theRow, bool theAddToList); // 0x401dc0
	bool			PlaceRandom();						// 0x401e60
	virtual bool	Update();							// 0x401f90
	void			FlyAway(Atom* theTarget);			// 0x4027a0
	virtual void	DrawOverlay(Graphics* g, int theFrame, int theX, int theY); // 0x4028c0
	void			Infect();							// 0x402c60
	virtual void	Draw(Graphics* g);					// 0x4037f0
	virtual bool	IsElement() { return false; }		// 0x450070
	virtual void	OnPlaced() {}						// 0x405b00
	virtual void	Save(FILE* f);						// 0x4012c0
	virtual void	Load(FILE* f);						// 0x403100
};

// The bonus atoms of the five elements (fire, water, earth, air, ether; three levels each): types 12 to 26
class ElementAtom : public Atom
{
public:
	float		mF_f0;				// +0xf0
	float		mFlicker[5];		// +0xf4 0 to 32
	float		mFlickerStep[5];	// +0x108
	int			mElement;			// +0x11c 0 fire, 1 water, 2 earth, 3 air, 4 ether
	int			mLevel;				// +0x120 0 to 2
	int			mF124;				// +0x124

	ElementAtom(Grid* theGrid, int theType);			// 0x4043d0
	virtual ~ElementAtom();								// 0x404630
	virtual bool	Update();							// 0x404640
	virtual void	Draw(Graphics* g);					// 0x404b00
	virtual bool	IsElement() { return true; }
	virtual void	OnPlaced();							// 0x4055b0: the element's effect
	virtual void	Save(FILE* f);						// 0x4049d0
	virtual void	Load(FILE* f);						// 0x404a50
	void			ShatterIn();						// 0x4047e0
	void			Fire(int theLevel);					// 0x404e30
	void			Water(int theLevel);				// 0x404fd0
	void			Earth(int theLevel);				// 0x405100
	void			Air(int theLevel);					// 0x4052a0
	void			Ether(int theLevel);				// 0x405480
};

// 0x421b00: the compass angle in whole degrees from the first point to the second (0 up, 90 right)
int			CompassAngle(int theX1, int theY1, int theX2, int theY2);
// the table of the original (app+0x3260 and +0x3800): sine and minus cosine of a whole degree
float		SinDeg(int theAngle);
float		NegCosDeg(int theAngle);

}

#endif
