#ifndef __HEAVYWEAPON_TANKSHOTS_H__
#define __HEAVYWEAPON_TANKSHOTS_H__

// TankShots.cpp: the tank's special weapons: the static strike, the homing missile, the rocket (all Projectiles, which
// bullets cannot destroy) and the flak burst (a TankShot of its own, in the board's list at +0x234).
// Original: 0x449830..0x44b510, 0x44a220..0x44b120. Vtables: StaticStrike 0x531118, HomingMissile 0x5311e4,
// Rocket 0x531210 (slots: 0 the deleting destructor, 1 Draw, 2 Update); FlakBurst 0x531174 (slots: 0 Draw, 1 Update,
// 2 the deleting destructor).

#include "Projectile.h"

namespace Sexy
{

class Board;
class Enemy;
class Graphics;

// The base of the shots the board's TankShots list holds: a Draw, an Update and the deleting destructor, in that order
// (Board::Draw calls slot 0, UpdateTankShots slot 1)
class TankShot
{
public:
	virtual void	Draw(Graphics* g) = 0;		// slot 0
	virtual void	Update() = 0;				// slot 1
	virtual ~TankShot() {}						// slot 2
};

// Lightning that jumps from the tank to the nearest enemy, from it to the next one, and so on, as many times as it
// has hits. Vtable 0x531118.
class StaticStrike : public Projectile
{
public:
	enum
	{
		TRAIL_LENGTH = 10				// the points of its path
	};

	Enemy*			mTarget;			// +0x68 the enemy it is going for now
	Enemy*			mLastTarget;		// +0x6c the one it hit last, which it does not go for again
	double			mTrailX[TRAIL_LENGTH];	// +0x70 its path: the head first
	double			mTrailY[TRAIL_LENGTH];	// +0xc0
	int				mHitsLeft;			// +0x110 twice the weapon's level, down one a hit

	explicit StaticStrike(Board* theBoard);					// 0x44aa10 (the original also takes the tank's x and y, unused)
	virtual ~StaticStrike();								// 0x447cf0
	virtual void	Draw(Graphics* g) override;				// 0x447d00 slot 1
	virtual void	Update() override;						// 0x44aca0 slot 2

private:
	Enemy*			FindTarget();							// 0x449e60
};

// The homing missile: it flies at 4 pixels an update, turning its frame (its heading) 0.1 an update toward the
// nearest enemy ahead of it. Vtable 0x5311e4.
class HomingMissile : public Projectile
{
public:
	HomingMissile(Board* theBoard, double x, double y);		// 0x449830
	virtual ~HomingMissile();								// 0x449920
	virtual void	Update() override;						// 0x44b510 slot 2

private:
	Enemy*			FindTarget();							// 0x44a520
};

// A rocket of the pods: it flies straight along the gun's heading at 4 pixels an update. Vtable 0x531210.
class Rocket : public Projectile
{
public:
	// x and y are not used: it starts at the tank's gun, theSideOffset pixels to the side
	Rocket(Board* theBoard, double x, double y, int theSideOffset);	// 0x449c80
	virtual ~Rocket();										// 0x449df0
	virtual void	Update() override;						// 0x44a6a0 slot 2
};

// A burst of flak where the mouse was, which goes off at points round it; it hurts what its flash touches.
// Vtable 0x531174.
class FlakBurst : public TankShot
{
public:
	double			mX;				// +0x08 where the mouse was, from the screen's centre
	double			mY;				// +0x10
	int				mBurstX;		// +0x18 the point of the burst now
	int				mBurstY;		// +0x1c
	double			mPhase;			// +0x20 the flash's frame, 0 to 4
	int				mBurstIndex;	// +0x28 bursts so far
	int				mMaxBursts;		// +0x2c 5 for each level of the flak weapon, and 5
	int				mBurstRadius;	// +0x38 how far from (mX, mY) the next bursts go: 8 for each level, and 8
	Board*			mBoard;			// +0x3c
	ObjectList<TankShot>::Node* mNode;	// +0x40 its node in the board's list

	explicit FlakBurst(Board* theBoard);					// 0x44a220
	virtual ~FlakBurst();									// 0x448620 (deleting 0x44a300, slot 2)
	virtual void	Draw(Graphics* g) override;				// 0x448630 slot 0
	virtual void	Update() override;						// 0x44b120 slot 1
};

}

#endif
