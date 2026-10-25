#ifndef __ATOMICA_BONUSMOLECULES_H__
#define __ATOMICA_BONUSMOLECULES_H__

// BonusMolecules.cpp: the bonus molecules (Atomica.exe 0x414830-0x415d20): a shape of atoms of given colours
// that the player may make for a bonus. port: the shapes' detection and the sign are not ported yet; the class
// holds the state the saved game keeps.

#include "Support.h"
#include <cstdio>

namespace Sexy
{

class AtomicaApp;
class Grid;
class Graphics;

class BonusMolecules
{
public:
	AtomicaApp*	mApp;			// +4
	int			mI8;			// +8
	int			mRound;			// +0xc
	int			mTarget;		// +0x10 the molecules needed for the next one
	int			mCount;			// +0x14
	int			mI18[25];		// +0x18 the shape being made (100 bytes)
	int			mAlpha;			// +0xc8 (shown by the sign)
	int			mI0cc;			// +0xcc

	BonusMolecules(AtomicaApp* theApp);		// 0x414830
	void	Check(Grid* theGrid);			// 0x415540
	void	Upgrade();						// 0x4153f0
	void	Update();						// vtable +8
	void	Draw(Graphics* g);				// vtable +4
	void	Save(FILE* f);					// 0x4146d0
	void	Load(FILE* f);					// 0x414750
};

}

#endif
