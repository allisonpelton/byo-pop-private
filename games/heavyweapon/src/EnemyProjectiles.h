#ifndef __HEAVYWEAPON_ENEMYPROJECTILES_H__
#define __HEAVYWEAPON_ENEMYPROJECTILES_H__

// EnemyProjectiles.cpp: the enemies' bombs, missiles, mines and rockets: every Projectile kind but the tank's own
// (TankShots.h). A bomb or missile kills the tank it touches, bursts on the shield, or lands in a crater; the board's
// AddDumbBomb, AddIronBomb and AddHellfire make the first three, the craft and the bosses the rest.
// Original: 0x447a30..0x44bcc0, and the big missile at 0x431d20. Each class's vtable (0x5310f8 and up, 3 slots: 0
// destructor, 1 Draw, 2 Update) is listed with it. The projectiles' fields start at +0x64.

#include "Projectile.h"

#include <vector>

namespace Sexy
{

class Enemy;

// The air mine (the original's constructor at 0x447a30 has no name of its own): it falls through the air and sways
// toward the tank. Vtable 0x531104.
class AirMine : public Projectile
{
public:
	AirMine(Board* theBoard, double x, double y, double theVelX, double theVelY);	// 0x447a30
	virtual ~AirMine();											// 0x447ab0
	virtual void	Update() override;							// 0x447ac0 slot 2
};

// A boulder thrown by a boss; a dark one is drawn grey. Vtable 0x531138.
class Boulder : public Projectile
{
public:
	bool			mIsDark;		// +0x68 drawn grey

	Boulder(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isDark);	// 0x447e00
	virtual ~Boulder();											// 0x447ec0
	virtual void	Draw(Graphics* g) override;					// 0x447ed0 slot 1
	virtual void	Update() override;							// 0x447fa0 slot 2
};

// A rocket that flies sideways and bursts into bullets when it is over the tank. Vtable 0x531144.
class BurstRocket : public Projectile
{
public:
	int				mBulletCount;	// +0x68 the bullets it bursts into, evenly round a circle

	BurstRocket(Board* theBoard, double x, double y, double theVelX, bool isMirrored, int theBulletCount);	// 0x448130
	virtual ~BurstRocket();										// 0x4481b0
	virtual void	Update() override;							// 0x44b010 slot 2

private:
	void			Burst();									// 0x449fa0
};

// Vtable 0x531150.
class DumbBomb : public Projectile
{
public:
	DumbBomb(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored);	// 0x4481c0
	virtual ~DumbBomb();										// 0x448280
	virtual void	Update() override;							// 0x449010 slot 2
};

// The fat boy: a bomb that kills the tank through its shield, with a flare as it goes off. Vtable 0x531168.
class FatBoy : public Projectile
{
public:
	bool			mIsDetonating;	// +0x68 it has hit the ground or the tank, and the flare grows
	double			mFlareTime;		// +0x70 the flare's size, 0.2 up 0.2 an update; at 6 the tank dies

	FatBoy(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored, bool isArmed);	// 0x448290
	virtual ~FatBoy();											// 0x448380
	virtual void	Draw(Graphics* g) override;					// 0x44a0e0 slot 1
	virtual void	Update() override;							// 0x448390 slot 2
};

// A bomb that throws 4 fragments up when it hits the ground. Vtable 0x53117c.
class FragBomb : public Projectile
{
public:
	FragBomb(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored);	// 0x448680
	virtual ~FragBomb();										// 0x448740
	virtual void	Update() override;							// 0x44b350 slot 2

private:
	void			ThrowFragments();							// 0x44a330
};

// A missile a boss fires, which turns toward the tank. Vtable 0x531188.
class HeadMissile : public Projectile
{
public:
	bool			mIsLocked;		// +0x68 it has turned as far as the tank's bearing and turns no more
	double			mTurnSpeed;		// +0x70 the frames it turns an update, 0.05 to 0.199

	// isLowFrame: it starts at frame 3 (turned one way) rather than 17 (the other)
	HeadMissile(Board* theBoard, double x, double y, bool isLowFrame);	// 0x448750
	virtual ~HeadMissile();										// 0x448880
	virtual void	Update() override;							// 0x448890 slot 2
};

// The missile of AddHellfire: it turns toward the tank until its heading points at it. Vtable 0x5311a0.
class Hellfire : public Projectile
{
public:
	bool			mIsLocked;		// +0x68 it has turned as far as the tank's bearing and turns no more
	double			mTurnSpeed;		// +0x70 the frames it turns an update, 0.1 to 0.249 times mScale
	double			mScale;			// +0x78 its speed in pixels an update is 4 times this, and its turning is scaled by it

	Hellfire(Board* theBoard, double x, double y, double theVelX, double theVelY, double theScale);	// 0x448b20
	virtual ~Hellfire();										// 0x448c80
	virtual void	Update() override;							// 0x448c90 slot 2
};

// Vtable 0x5311ac.
class IronBomb : public Projectile
{
public:
	IronBomb(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored);	// 0x448f30
	virtual ~IronBomb();										// 0x449000
	virtual void	Update() override;							// 0x449010 slot 2
};

// The laser-guided bomb: it swings on its way down toward the tank's side. Vtable 0x5311b8.
class LGB : public Projectile
{
public:
	double			mAngle;			// +0x68 its heading in degrees: 90 or -90 at first, then steered within -45..45; its
									// frame is (mAngle + 90) / 9

	LGB(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored);	// 0x4491c0
	virtual ~LGB();												// 0x4492a0
	virtual void	Update() override;							// 0x4492b0 slot 2
};

// A meteor falling from above the field's top at the ground. Vtable 0x5311d8.
class Meteor : public Projectile
{
public:
	explicit Meteor(Board* theBoard);							// 0x449510
	virtual ~Meteor();											// 0x449620
	virtual void	Update() override;							// 0x449630 slot 2
};

// The truck's rocket. Vtable 0x5311f0.
class Rpg : public Projectile
{
public:
	double			mFrameStep;		// +0x68 the frames it turns an update (0.05 to 0.149); its frame is its heading

	Rpg(Board* theBoard, double x, double y, bool isMirrored, double theFrameStep);	// 0x449930
	virtual ~Rpg();												// 0x449a30
	virtual void	Update() override;							// 0x449a40 slot 2
};

// A fragment that falls leaving a trail of its last 40 points, each of which kills the tank it touches; it makes a
// flash on the ground where it lands. Vtable 0x531260.
class BombFragment : public Projectile
{
public:
	enum
	{
		KIND_STRAIGHT = 0,				// falls 3.5 a frame
		KIND_DRIFT = 1,					// falls 3 a frame and drifts sideways 3 pixels for each of mDrift
		KIND_SWAY = 2					// falls 2 a frame and sways sideways (speed up to 5 either way)
	};

	enum
	{
		TRAIL_LENGTH = 40,				// the points of its path that kill the tank, newest last
		GROUND_FLASH_ALPHA = 300
	};

	int				mDrift;			// +0x68 how far it drifts (KIND_DRIFT), or its sway's acceleration in tenths
	double			mSwayAccel;		// +0x70 mDrift * 0.1; reverses at the sway's ends
	int				mKind;			// +0x78 how it falls (KIND_)
	bool			mHasLanded;		// +0x7c it has made its mark on the ground
	std::vector<int> mTrailX;		// +0x84 the points of its path, oldest first
	std::vector<int> mTrailY;		// +0x94

	BombFragment(Board* theBoard, double x, double y, int theKind, int theDrift);	// 0x44b950
	virtual ~BombFragment();									// 0x44b870 (deleting 0x44b920)
	virtual void	Draw(Graphics* g) override;					// 0x44a990 slot 1
	virtual void	Update() override;							// 0x44ba10 slot 2
};

// The big missile that rises from the ground to the top of the screen, then comes back down on the tank. Vtable
// 0x530224.
class BigMissile : public Projectile
{
public:
	enum
	{
		MAX_HEALTH = 40,
		HEALTH_PER_LEVEL = 7
	};

	double			mRising;		// +0x38 1 while it climbs (out of reach in the background), 0 once it falls; also
									// its sprite's row
	bool			mTankDied;		// +0x68 the tank died while it climbed: it never comes back

	BigMissile(Board* theBoard, double x);						// 0x431d20
	virtual ~BigMissile();										// 0x431e20
	virtual void	Draw(Graphics* g) override;					// 0x432030 slot 1
	virtual void	Update() override;							// 0x431e30 slot 2
};

}

#endif
