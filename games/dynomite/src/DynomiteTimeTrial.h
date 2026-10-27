#ifndef __DYNOMITETIMETRIAL_H__
#define __DYNOMITETIMETRIAL_H__

// The time trial ("Time Trial" on the side bar; vtable 0x4fa100, 0x254 bytes; constructor 0x411ff0, destructor
// 0x4121a0; mode 4 of StartGame): a fixed stack of rows (60, 30 or 15 by skill) to clear as fast as possible. The
// stack comes down until its lowest row is at y 250; the score is the time, counting down from 0 by one an
// update, so the best score is the highest.

#include "Dynomite.h"

namespace Sexy
{

class DynomiteTimeTrial : public Dynomite
{
public:
	int					mEggsBroken;		// +0x248 AddPoints's points (the eggs a match broke, and the power-ups')
	int					mEggsDropped;		// +0x24c AddPoints's multipliers (the eggs a match dropped)
	int					mRowsCleared;		// +0x250 the most rows to go shown (the rows at the start)

	DynomiteTimeTrial();					// 0x411ff0
	virtual ~DynomiteTimeTrial();			// 0x4121a0

	// 0x412750: at the game's end "cleared" fades in
	virtual void		Draw(Graphics* g) override;
	// 0x412320: the clock; the ceiling comes down a pixel an update to at most 30, and back up a third of the way
	// when the lowest row passes y 250; the game's end when no eggs are left
	virtual void		Update() override;
	// 0x4121b0: a colour from the six rows above the lowest, else (past 50 tries) any colour
	virtual Egg*		NextEgg() override;
	// 0x412840: the colours and the rows by skill, the clock on the side bar
	virtual void		Init(FossilizedApp* theApp, int theDifficulty) override;
	// 0x412680: no points; it counts the eggs broken and dropped
	virtual void		AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain) override;
	virtual void		DrawScenery() override;		// 0x412ad0: and the rows to go
	// 0x412cb0: the board's, with the rows cleared, the eggs broken and dropped and the time
	virtual void		DrawGameOver(Graphics* g) override;
	virtual void		Save() override;				// 0x4126b0 (slot 59): the board's, then the mode's
	virtual void		Load() override;				// 0x4126f0 (slot 60)
};

}

#endif
