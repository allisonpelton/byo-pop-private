#ifndef __DYNOMITEPANIC_H__
#define __DYNOMITEPANIC_H__

// The endless mode ("Endless" on the side bar; vtable 0x4f9ce0, 0x274 bytes; constructor 0x40ca40, destructor 0x40cbf0;
// mode 2 of StartGame): the ceiling comes down by itself, CeilingSpeed sets how fast, and each time Kroney's sand
// runs out a whirley rises with an egg of a new colour. The game ends when the eggs reach the board's line.

#include "Dynomite.h"

namespace Sexy
{

class CeilingSpeed;
class Kroney;
class Whirley;

class DynomitePanic : public Dynomite
{
public:
	Kroney*				mKroney;			// +0x248
	PtrList<Whirley>	mWhirleys;			// +0x24c
	int					mWhirleyLevel;		// +0x264 the whirleys knocked out in a row (at least 1 in Update)
	CeilingSpeed*		mCeilingSpeed;		// +0x268
	float				mKick;				// +0x26c the ceiling's push up after a blast (7), less a twentieth a frame
	int					mWhirleysHit;		// +0x270

	DynomitePanic();						// 0x40ca40
	virtual ~DynomitePanic();				// 0x40cbf0

	// 0x40d2a0: the screen shakes while a power-up moves the walls or the ceiling; at the game's end "game over" fades in
	virtual void		Draw(Graphics* g) override;
	// 0x40d7b0: the ceiling's speed and kick, the walls' and the ceiling's power-ups, the whirleys, Kroney, and the game's end
	virtual void		Update() override;
	virtual void		GetPowerUpCenter(int* x, int* y) override;	// 0x40d050: by Kroney
	// 0x40cce0: from the colours in the lowest row once the eggs are low enough (two times in three), else any
	// colour; a baby that left the board (mNextEggKind 20) makes a power-up egg
	virtual Egg*		NextEgg() override;
	// 0x40d420: the colours by skill, six rows, Kroney's minute, the ceiling's speed
	virtual void		Init(FossilizedApp* theApp, int theDifficulty) override;
	virtual void		DrawScenery() override;		// 0x40d0a0: and Kroney
	virtual void		DrawMode() override;		// 0x40cca0: the whirleys
	virtual void		Blasted() override;			// 0x40d080: the kick
	// 0x40dd50: the board's, with the best combo, the babies hatched, the whirleys whacked and the final score
	virtual void		DrawGameOver(Graphics* g) override;
	virtual void		Save() override;				// 0x40d0c0 (slot 59): the board's, then the mode's
	virtual void		Load() override;				// 0x40d160 (slot 60)
};

}

#endif
