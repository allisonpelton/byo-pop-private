#ifndef __HEAVYWEAPON_CRAFT_H__
#define __HEAVYWEAPON_CRAFT_H__

// Craft.h: the enemy craft, every subclass of Enemy except the supply copter (SupplyCopter.h) and the bosses. The
// aircraft are in Aircraft.cpp, the ground units (the truck, the tank and the dozer) in GroundUnits.cpp.
// Each one's constructor takes its armour, points and sprite from the app's craft table (HeavyWeaponApp::mCraft, the
// entry of its craft index) and adds that entry's mIndex to the board's mLevelEndBlocks, which its destructor takes
// out. A class lists only the virtuals it has of its own: the rest are Enemy's. Original vtables (9 slots, from
// 0x5301c8 on): 0 destructor, 1 Draw, 2 Update, 3 UpdateF (0x4376f0 or 0x435350, MoveEnemyX and MoveEnemyXY here),
// 4 Killed, 5 Explode (0x431480) and 8 ChooseSide (0x431650) are shared with Enemy, 6 and 7 are empty.
// Killed counts the kill, explodes, and adds the points to the score at the craft's centre.

#include "Enemy.h"

namespace Sexy
{

class Board;
class Graphics;

// The craft's indices in craft.xml's table (HeavyWeaponApp::mCraft; the picture of craft n is image 135 + n)
enum CraftIndex
{
	CRAFT_PROPFIGHTER = 1,
	CRAFT_SMALLJET = 2,
	CRAFT_BOMBER = 3,
	CRAFT_JETFIGHTER = 4,
	CRAFT_TRUCK = 5,
	CRAFT_BIGBOMBER = 6,
	CRAFT_SMALLCOPTER = 7,
	CRAFT_MEDCOPTER = 8,
	CRAFT_BIGCOPTER = 9,
	CRAFT_DELTABOMBER = 10,
	CRAFT_DELTAJET = 11,
	CRAFT_BIGMISSILE = 12,		// a Projectile (EnemyProjectiles.h)
	CRAFT_SUPERBOMBER = 13,
	CRAFT_FATBOMBER = 14,
	CRAFT_BLIMP = 15,
	CRAFT_SATELLITE = 16,
	CRAFT_STRAFER = 17,
	CRAFT_ENEMYTANK = 18,
	CRAFT_DOZER = 19,
	CRAFT_DEFLECTOR = 20,
	CRAFT_CRUISEMISSILE = 21
};

// The field is 640 wide, x from -320 to 320 from its centre: a craft comes in just outside one edge and goes when it
// has left past the other
const int CRAFT_EDGE_X = 320;
// A craft fires only while its x is inside this
const double CRAFT_FIRE_RANGE = 300.0;
// The sky an aircraft flies in: its centre is at least CRAFT_SKY_TOP + half its height, and CRAFT_SKY_HEIGHT less
// its own height below that at most
const int CRAFT_SKY_TOP = 50;
const int CRAFT_SKY_HEIGHT = 230;
// The y of the ground, where the ground units' shadows are drawn and shells land
const int CRAFT_GROUND_Y = 460;
// The ground units leave in the last 700 updates of a level (survival has none)
const int CRAFT_LEAVING_TIME = 700;

// 0x4376f0, slot 3 of most craft: moves it sideways for the part of an update a frame shows
void MoveEnemyX(Enemy* theEnemy, float theFrac);
// 0x435350, slot 3 of the craft that fly up and down: moves it both ways
void MoveEnemyXY(Enemy* theEnemy, float theFrac);

// 1: only flies, its propeller turning
class PropFighter : public Enemy
{
public:
	PropFighter(Board* theBoard);							// 0x434430
	virtual ~PropFighter();									// 0x434590
	virtual void	Update() override;						// 0x4345c0 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x434650 slot 4
};

// 2: drops a dumb bomb now and then
class SmallJet : public Enemy
{
public:
	enum
	{
		BOMB_ODDS = 80,					// one update in this many may drop a bomb
		BOMB_DELAY = 200				// updates before the next
	};

	int				mBombDelay;			// +0x70 updates until it may drop another bomb

	SmallJet(Board* theBoard);								// 0x434690
	virtual ~SmallJet();									// 0x4347e0
	virtual void	Update() override;						// 0x434810 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x434910 slot 4
};

// 3: drops dumb bombs
class Bomber : public Enemy
{
public:
	enum
	{
		BOMB_ODDS = 40,
		BOMB_DELAY = 100
	};

	int				mBombDelay;			// +0x70

	Bomber(Board* theBoard);								// 0x4324f0
	virtual ~Bomber();										// 0x432640
	virtual void	Update() override;						// 0x432670 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x432770 slot 4
};

// 4: drops laser-guided bombs (LGB)
class JetFighter : public Enemy
{
public:
	enum
	{
		BOMB_ODDS = 40,
		BOMB_DELAY = 150
	};

	int				mBombDelay;			// +0x70

	JetFighter(Board* theBoard);							// 0x433e90
	virtual ~JetFighter();									// 0x433fe0
	virtual void	Update() override;						// 0x434010 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x434160 slot 4
};

// 5: a ground unit that comes in at the side the tank is on, slows down over the field and throws rockets (Rpg)
class Truck : public Enemy
{
public:
	enum
	{
		ROCKET_ODDS = 20,
		ROCKET_DELAY = 100,
		SHADOW_Y = 0x1d1				// the shadow's y
	};

	int				mRocketDelay;		// +0x70 updates until it may throw another rocket

	Truck(Board* theBoard);									// 0x435650
	virtual ~Truck();										// 0x435760
	virtual void	Draw(Graphics* g) override;				// 0x4359e0 slot 1: its shadow, then the truck
	virtual void	Update() override;						// 0x435790 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x435a20 slot 4: the side it held is free again
};

// 6: drops a run of 3 or 4 dumb bombs, 20 updates apart
class BigBomber : public Enemy
{
public:
	enum
	{
		RUN_ODDS = 20,					// one update in this many may start a run
		RUN_DELAY = 200,				// updates from a run's start until the next may start
		BOMB_SPACING = 20,				// updates between the bombs of a run
		MIN_RUN_BOMBS = 3				// a run drops this many or one more
	};

	int				mRunDelay;			// +0x70 updates until it may start another run
	int				mBombsLeft;			// +0x74 the bombs of the run in progress still to drop
	int				mBombTimer;			// +0x78 updates until the next bomb of the run (the original never sets it:
										// it is uninitialised memory there; 0 here)

	BigBomber(Board* theBoard);								// 0x431730
	virtual ~BigBomber();									// 0x431890
	virtual void	Update() override;						// 0x4318c0 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x431a30 slot 4
};

// 7: a small helicopter, which fires bullets at the tank
class SmallCopter : public Enemy
{
public:
	enum
	{
		SHOT_ODDS = 20,
		SHOT_DELAY = 150,				// updates from one shot until the next
		MUZZLE_FLASH_TIME = 5,			// updates its muzzle flash shows
		FRAMES = 5						// its cels
	};

	int				mReloadDelay;		// +0x70 updates until it may fire again
	int				mShotDelay;			// +0x74 updates its muzzle flash still shows (5 when it fires, down 1 an update;
										// the help screen sets it too)

	SmallCopter(Board* theBoard);							// 0x434c80
	virtual ~SmallCopter();									// 0x434de0
	virtual void	Draw(Graphics* g) override;				// 0x434f70 slot 1: the muzzle flash, added
	virtual void	Update() override;						// 0x434e10 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x434fe0 slot 4
};

// 8: a medium helicopter, which goes after the player's bullets and fires hellfire missiles
class MedCopter : public Enemy
{
public:
	enum
	{
		MISSILE_ODDS = 30,
		MISSILE_DELAY = 125
	};

	int				mMissileDelay;		// +0x70 updates until it may fire again
	double			mRotorFrame;		// +0x78 its rotor's column

	MedCopter(Board* theBoard);								// 0x4341a0
	virtual ~MedCopter();									// 0x4342d0
	virtual void	Draw(Graphics* g) override;				// 0x434330 slot 1
	virtual void	Update() override;						// 0x4367d0 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x434300 slot 3: its frame is its sideways speed
	virtual void	Killed() override;						// 0x4343f0 slot 4
};

// 9: a big helicopter, which goes after the player's bullets and fires hellfire missiles
class BigCopter : public Enemy
{
public:
	enum
	{
		MISSILE_ODDS = 30,
		MISSILE_DELAY = 75
	};

	int				mMissileDelay;		// +0x70 updates until it may fire again
	double			mRotorFrame;		// +0x78 its rotor's column

	BigCopter(Board* theBoard);								// 0x431a70
	virtual ~BigCopter();									// 0x431ba0
	virtual void	Draw(Graphics* g) override;				// 0x431c00 slot 1
	virtual void	Update() override;						// 0x435ad0 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x431bd0 slot 3: its frame is its sideways speed
	virtual void	Killed() override;						// 0x431ce0 slot 4
};

// 10: drops frag bombs (FragBomb)
class DeltaBomber : public Enemy
{
public:
	enum
	{
		BOMB_ODDS = 40,
		BOMB_DELAY = 100
	};

	int				mBombDelay;			// +0x70

	DeltaBomber(Board* theBoard);							// 0x432f50
	virtual ~DeltaBomber();									// 0x4330a0
	virtual void	Update() override;						// 0x4330d0 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x433220 slot 4
};

// 11: drops iron bombs
class DeltaJet : public Enemy
{
public:
	enum
	{
		BOMB_ODDS = 40,
		BOMB_DELAY = 75
	};

	int				mBombDelay;			// +0x70

	DeltaJet(Board* theBoard);								// 0x433260
	virtual ~DeltaJet();									// 0x4333b0
	virtual void	Update() override;						// 0x4333e0 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x4334e0 slot 4
};

// 13: drops an iron bomb every 50 updates while over the field
class SuperBomber : public Enemy
{
public:
	enum
	{
		BOMB_DELAY = 50
	};

	int				mBombDelay;			// +0x70

	SuperBomber(Board* theBoard);							// 0x4353b0
	virtual ~SuperBomber();									// 0x435500
	virtual void	Update() override;						// 0x435530 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x435610 slot 4
};

// 14: drops a fat boy (FatBoy) every 150 updates while over the field
class FatBomber : public Enemy
{
public:
	enum
	{
		BOMB_DELAY = 150
	};

	int				mBombDelay;			// +0x70

	FatBomber(Board* theBoard);								// 0x433b90
	virtual ~FatBomber();									// 0x433cf0
	virtual void	Update() override;						// 0x433d20 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x433e50 slot 4
};

// 15: a blimp, with two propellers, a dumb bomb from each end, and a burst of shrapnel when it is shot down
class Blimp : public Enemy
{
public:
	enum
	{
		BOMB_ODDS = 40,
		BOMB_DELAY = 50,
		NUM_SHRAPNEL = 10,
		SHRAPNEL_ANGLE_RANGE = 3141		// a shot's angle is Rand() % this thousandths of a radian
	};

	int				mFrontBombDelay;	// +0x70 updates until the bomb at mX + 60 may drop
	int				mBackBombDelay;		// +0x74 the same for the bomb at mX - 60
	double			mPropFrame;			// +0x78 the propellers' column, 0..4

	Blimp(Board* theBoard);									// 0x4320c0
	virtual ~Blimp();										// 0x432220
	virtual void	Draw(Graphics* g) override;				// 0x4323e0 slot 1
	virtual void	Update() override;						// 0x432250 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x435f50 slot 4: shrapnel
};

// 16: a satellite: a laser beam down to the ground, which kills a tank under it
class Satellite : public Enemy
{
public:
	enum
	{
		BEAM_DELAY = 400,				// updates from a beam's start until the next may start
		BEAM_TIME = 200,				// updates a beam takes, its last 99 burning
		BEAM_BURN_START = 100,
		BEAM_ODDS = 40,
		TANK_KILL_RANGE = 32			// the beam kills a tank this near its x (it takes a shield 50 away)
	};

	int				mBeamDelay;			// +0x70 updates until it may fire a beam (200 on arrival)
	int				mBeamTime;			// +0x74 updates the beam in progress has left, 0 for none
	double			mBeamFrame;			// +0x78 the beam source's column, 0..10
	int				mSmokeCounter;		// +0x80 updates since its last sparks (they come on every 4th)
	int				mKeyDelay;			// +0x84 updates until its next key-tapping sound
	int				mChatterDelay;		// +0x88 updates until its next radio sound

	Satellite(Board* theBoard);								// 0x434950
	virtual ~Satellite();									// 0x434a90
	virtual void	Draw(Graphics* g) override;				// 0x434ac0 slot 1: the beam
	virtual void	Update() override;						// 0x436c80 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x435350 slot 3
	virtual void	Killed() override;						// 0x434c40 slot 4
};

// 17: a strafer, which dives on an arc from one side, firing, and leaves by the other
class Strafer : public Enemy
{
public:
	enum
	{
		SHOT_DELAY = 75
	};

	int				mShotDelay;			// +0x70 updates until it may fire again
	double			mSpeed;				// +0x78 its speed along the arc

	Strafer(Board* theBoard);								// 0x435020
	virtual ~Strafer();										// 0x435160
	virtual void	Update() override;						// 0x435190 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x435350 slot 3
	virtual void	Killed() override;						// 0x435370 slot 4
};

// 21: a cruise missile (counted as neither spawned nor destroyed): flies across, dives on the tank
class CruiseMissile : public Enemy
{
public:
	CruiseMissile(Board* theBoard);							// 0x4327b0
	virtual ~CruiseMissile();								// 0x432900
	virtual void	Update() override;						// 0x432930 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x435350 slot 3
	virtual void	Killed() override;						// 0x432c20 slot 4
};

// 18: the enemy tank: a ground unit that comes in at the side the tank is on, slows down and fires shells
class EnemyTank : public Enemy
{
public:
	enum
	{
		SHELL_DELAY = 120,
		SHADOW_Y = 0x1d1,				// the shadow's y
		GUN_Y = 0x1a7					// the gun's y
	};

	int				mShellDelay;		// +0x70 updates until it may fire again (120 on arrival)
	double			mGunAngle;			// +0x78 the gun's cel, 0..8 (7 on arrival)
	double			mGunStep;			// +0x80 the cels it moves an update (0.03 on arrival, then +-0.04)

	EnemyTank(Board* theBoard);								// 0x433960
	virtual ~EnemyTank();									// 0x433aa0
	virtual void	Draw(Graphics* g) override;				// 0x433ad0 slot 1: its shadow, its gun, then the tank
	virtual void	Update() override;						// 0x436410 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x433b40 slot 4: the sides it held are free again
};

// 19: the dozer: rams the tank
class Dozer : public Enemy
{
public:
	// The original also sets an int at +0x70 to 120 and never reads it
	Dozer(Board* theBoard);									// 0x433520
	virtual ~Dozer();										// 0x433630
	virtual void	Draw(Graphics* g) override;				// 0x4338e0 slot 1: its shadow, then the dozer
	virtual void	Update() override;						// 0x433660 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x433920 slot 4
};

// 20: the deflector: a shield that turns the tank's bullets back on it while it lasts
class Deflector : public Enemy
{
public:
	enum
	{
		MISSILE_ODDS = 40,
		MISSILE_DELAY = 75,
		SHIELD_ENERGY = 60,				// what its shield starts with
		SHIELD_RADIUS = 60,				// the shield's reach, in pixels
		GLOW_FADE = 8,
		GLOW_MAX = 255
	};

	int				mMissileDelay;		// +0x70 updates until it may fire a hellfire missile
	int				mGlow;				// +0x74 the shield's flash: 255 when it turns a bullet, down 8 an update
	int				mShieldEnergy;		// +0x78 down by a bullet's damage for each it turns; the shield is down at 0

	Deflector(Board* theBoard);								// 0x432c80
	virtual ~Deflector();									// 0x432df0
	virtual void	Draw(Graphics* g) override;				// 0x432e20 slot 1
	virtual void	Update() override;						// 0x436110 slot 2
	virtual void	UpdateF(float theFrac) override;		// 0x4376f0 slot 3
	virtual void	Killed() override;						// 0x432f10 slot 4
};

}

#endif
