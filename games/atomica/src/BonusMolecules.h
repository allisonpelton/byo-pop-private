#ifndef __ATOMICA_BONUSMOLECULES_H__
#define __ATOMICA_BONUSMOLECULES_H__

// BonusMolecules.cpp: the bonus molecules (Atomica.exe 0x414830-0x415d20 and the effect 0x415d20-0x416480): after a few
// molecules a sign shows a shape of atoms of given colours (a 5 x 5 pattern); when the player makes it on the field the
// atoms spiral together and turn into an element atom (fire, water, earth, air, ether). The patterns come in three lists
// of ten, one for each level of the element: the upgrade shapes need the element atom of the level below.

#include "Support.h"
#include <cstdio>
#include <vector>

namespace Sexy
{

class AtomicaApp;
class Grid;
class Graphics;

struct BonusPattern
{
	int		mCells[25];			// rows of five: a colour plus one, an element atom's type plus one, or 0 for any cell
	int		mElement;			// the type of the element atom it makes, plus one
};

class BonusMolecules
{
public:
	AtomicaApp*	mApp;			// +4
	int			mX;				// +8 the sign's place (it slides in from the left)
	int			mRound;			// +0xc molecules made since the last bonus
	int			mTarget;		// +0x10 the molecules a bonus takes (3; 1000 when there are none)
	int			mCount;			// +0x14 bonus molecules made
	int			mCurrent[25];	// +0x18 the shape being asked for (a column of the shape after the other)
	const BonusPattern*	mPattern;	// +0xc4
	int			mTimer;			// +0xc8
	int			mElement;		// +0xcc the index of mPattern in the table of patterns (a saved game holds the index)
	bool		mDone;			// +0xd0
	bool		mBlinkOn;		// +0xd1
	int			mBlink;			// +0xd4
	std::vector<const BonusPattern*>	mLists[3];	// +0x7c, +0x94, +0xac the shapes for the three levels of the elements

	BonusMolecules(AtomicaApp* theApp);		// 0x414830
	void	Check(Grid* theGrid);			// 0x415540
	void	Upgrade();						// 0x4153f0
	void	Revalidate();					// 0x4146a0
	void	PickPattern();					// 0x415420
	bool	Feasible(const BonusPattern* thePattern);	// 0x415be0
	void	Update();						// 0x415760
	void	Draw(Graphics* g);				// 0x415830
	void	Save(FILE* theFile);			// 0x4146d0
	void	Load(FILE* theFile);			// 0x414750
};

}

#endif
