#ifndef __ATOMICA_SWIRL_H__
#define __ATOMICA_SWIRL_H__

// Swirl.cpp: the background of the game (Atomica.exe 0x4283a0-0x429230, with the constructor at 0x428100, the update at 0x427c70
// and the destructor at 0x427ba0). A swirl is one ring of the tunnel behind the field: a polygon of twenty points around a
// centre that wanders, which shrinks toward the middle; the board makes a new one every 95 updates, draws each as the band
// between its polygon and the polygon of the one before (the first as a disc), in a colour that follows its size, and lets
// it go when it has shrunk to its minimum. An atom that leaves the field is handed to a copy of the newest swirl (a
// 'clone'): it circles the middle of the screen, drawn smaller and smaller, until the clone has gone.

#include "Support.h"

#include <vector>

namespace Sexy
{

class AtomicaApp;
class Board;
class Atom;
class Graphics;
struct Span;

class Swirl
{
public:
	enum
	{
		NUM_POINTS = 0x17c		// the arrays: 20 points of the ring, 18 entries apart (the rest is room for more)
	};

	Board*			mBoard;			// +4
	AtomicaApp*		mApp;			// +8
	float			mX;				// +0xc the centre
	float			mY;				// +0x10
	float			mOffsetX;		// +0x14 where the ring is round it
	float			mOffsetY;		// +0x18
	float			mTargetX;		// +0x1c where the centre goes
	float			mTargetY;		// +0x20
	float			mRadius;		// +0x24 starts at 650
	float			mSpeed;			// +0x28 how fast it shrinks
	float			mMinRadius;		// +0x2c it goes when it has shrunk to this
	int				mPoints[NUM_POINTS][2];		// +0x30 the ring's points (screen position)
	int				mJitter[NUM_POINTS][2];		// +0xc10 what each is moved by (-7 to 7)
	int				m17f0;			// +0x17f0 (a clone copies it; the original never sets it)
	PtrList<Atom>	mAtoms;			// +0x17f4 the atoms that circle in it
	bool			mClone;			// +0x180c a clone of a swirl for flown atoms: it draws no ring
	float			mShrink;		// +0x1810 how fast a clone's shrinking speeds up

	// the constants that go on between swirls (the original keeps them in its data, set once for the program)
	static float	sRadiusA;		// 0x4cbe08
	static float	sRadiusAStep;	// 0x4cbe0c
	static float	sRadiusB;		// 0x4cbe10
	static float	sRadiusBStep;	// 0x4cbe14
	static int		sAngle;			// 0x4d40d8

	Swirl(Board* theBoard);					// 0x428100
	virtual ~Swirl();						// 0x428370 with 0x427ba0
	bool	Update();						// 0x427c70: false when the swirl has gone
	void	Aim(int theX, int theY, float* theAngle, float* theRatio);	// 0x427ed0
	Swirl*	Clone();						// 0x428870
	void	Draw(Graphics* g, Swirl* thePrevious);	// 0x428940
	void	DrawTriangle(std::vector<Span>& theSpans, const int* theA, const int* theB, const int* theC, int theStride);	// 0x4283a0 with 0x427f50
};

}

#endif
