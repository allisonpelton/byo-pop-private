#ifndef __HEAVYWEAPON_BOSSES_H__
#define __HEAVYWEAPON_BOSSES_H__

// Bosses*.cpp: the ten bosses, one per file (the helicopter, battleship, rainer, wrecker, ape, eye, head, worm, robot and
// final boss), each with its parts (turrets, launchers, arms, hands, a laser). The board makes one when a boss level
// comes: switch (mLevel % 9): 0 the final boss on the last level and the helicopter before it, 1 battleship, 2 rainer,
// 3 wrecker, 4 ape, 5 eye, 6 head, 7 worm, 8 robot. Each is an Enemy that the board also keeps in mBoss.
// Original: 0x437de0..0x447210, their data read by each constructor's own loader.

#include "Enemy.h"
#include "Projectile.h"

#include <map>
#include <string>

namespace Sexy
{

class Board;
class Graphics;
class HeavyWeaponApp;
class Sprite;

// How many levels it takes to meet every boss once: the first nine meet each for the first time, the next nine for the
// second, and the data (data/bosses.xml) has a block for each ("Level1", "Level2")
const int BOSS_ENCOUNTER_LEVELS = 9;

// A boss's block of data/bosses.xml for this encounter (BossesCommon.cpp): the block's own attributes under the element
// name "", and each element inside it under its name. A missing one reads as an empty string, 0 and 0.0.
class BossData
{
public:
	BossData(const char* theBoss, const char* theBlock);

	const std::string&	Attribute(const char* theElement, const char* theName);
	int					Int(const char* theElement, const char* theName);		// atol
	double				Double(const char* theElement, const char* theName);	// atof

private:
	std::map<std::string, std::map<std::string, std::string> > mElements;
};

// Writes "Level1" or "Level2" for the encounter the app's level is
void GetBossBlockName(HeavyWeaponApp* theApp, char* theName);

// The ape boss (level 5 and 14): it stands at the right edge, throws bursting rockets at the tank three times, then
// jumps across the field, and stomps the tank it lands on. Original: 0xa8 bytes, constructor 0x438be0, destructor
// 0x4383d0 (body 0x437de0); vtable 0x530650: 1 Draw 0x4382a0, 2 Update 0x437e50, 4 Killed 0x438340.
class ApeBoss : public Enemy
{
public:
	Sprite*			mShadow;		// +0x70 images\Ape\shadow
	int				mWaitTime;		// +0x74 updates it stands still before it chooses what to do next (100 at first)
	int				mJumpCount;		// +0x78 the throws since its last jump; at 3 it jumps, and it counts 3 while the tank is dead
	int				mLandTime;		// +0x7c updates of crouching left before a jump (uninitialised in the original,
									// which sets it before it reads it; 0 here)
	bool			mIsThrowing;	// +0x80 it throws rockets rather than jumps
	int				mShake;			// +0x84 how hard the screen shakes (0x80 when it lands, down 2 an update)
	double			mJumpVelX;		// +0x88 its sideways speed in a jump (the data's xspeed)
	double			mJumpVelY;		// +0x90 its upward speed at the start of a jump, and its falling speed's limit
	double			mGravity;		// +0x98 added to its fall each update
	double			mThrowSpeed;	// +0xa0 the frames its throw advances an update (the data's throw)

	ApeBoss(Board* theBoard);								// 0x438be0
	virtual ~ApeBoss();										// 0x4383d0 (body 0x437de0)
	virtual void	Draw(Graphics* g) override;				// 0x4382a0 slot 1: its shadow, then the ape
	virtual void	Update() override;						// 0x437e50 slot 2
	virtual void	Killed() override;						// 0x438340 slot 4
};

class BattleshipTurret;

// The battleship boss (level 2 and 11): a ship that sails in from the right with four gun turrets and a launcher
// of hellfire missiles. Original: 0xc8 bytes, constructor 0x439e60, destructor 0x4390f0 (body 0x438d80); vtable
// 0x5306f0: 1 Draw 0x439000, 2 Update 0x438df0, 4 Killed 0x439060.
class BattleshipBoss : public Enemy
{
public:
	enum
	{
		NUM_TURRETS = 4					// 0 and 1 on the left, 2 and 3 on the right
	};

	Sprite*			mHull;				// +0x70 images\battleship\hull, drawn under the ship (the ship's sprite is images\battleship\main)
	Sprite*			mGunSprite;			// +0x74 images\battleship\gun, the turrets' (21 frames)
	BattleshipTurret* mTurrets[NUM_TURRETS]; // +0x78 each, NULL once shot down
	int				mLauncherDelay;		// +0x88 updates until the launcher fires a hellfire missile
	int				mTurretFire[NUM_TURRETS]; // +0x8c the data's fire: updates between a turret's shots
	double			mTurretSpeed[NUM_TURRETS]; // +0xa0 the data's speed: the frames a turret's gun turns an update
	int				mTurretArmor;		// +0xc0 each turret's armour
	int				mLauncherFire;		// +0xc4 the data's fire: updates between the launcher's missiles

	BattleshipBoss(Board* theBoard);						// 0x439e60
	virtual ~BattleshipBoss();								// 0x4390f0 (body 0x438d80)
	virtual void	Draw(Graphics* g) override;				// 0x439000 slot 1: its hull, then the ship
	virtual void	Update() override;						// 0x438df0 slot 2
	virtual void	Killed() override;						// 0x439060 slot 4
};

// One of the battleship's four gun turrets: it rides on the ship, sweeps its gun and fires down at the tank. It
// takes the ship's gun sprite and counts neither as spawned nor as destroyed. Original: 0xa8 bytes, constructor
// 0x43a1c0, destructor 0x43a2d0 (body 0x43a250); vtable 0x5307a8: 2 Update 0x43a300, 4 Killed 0x43a260.
class BattleshipTurret : public Enemy
{
public:
	BattleshipBoss*	mShip;				// +0x70
	double			mOffsetX;			// +0x78 where it sits from the ship's centre (the ship sets it)
	double			mOffsetY;			// +0x80
	double			mGunStep;			// +0x88 the frames its gun turns an update, negative on the way back
	int				mFireDelay;			// +0x90 updates until it fires
	int				mIndex;				// +0x94 which of the ship's turrets it is

	BattleshipTurret(Board* theBoard, BattleshipBoss* theShip, int theIndex);	// 0x43a1c0
	virtual ~BattleshipTurret();							// 0x43a2d0 (body 0x43a250)
	virtual void	Update() override;						// 0x43a300 slot 2
	virtual void	Killed() override;						// 0x43a260 slot 4
};

class EyeHand;

// The eye boss (level 6 and 15): a floating eye with six jointed arms that end in hands. Open, it fires energy shots;
// shut, one of its hands charges a bolt of lightning that strikes the ground below it. Original: 0x4b8 bytes,
// constructor 0x43ba90, destructor 0x43aa70 (body 0x43a650); vtable 0x530820: 1 Draw 0x43a700, 2 Update 0x43aaa0,
// 4 Killed 0x43a9e0.
class EyeBoss : public Enemy
{
public:
	enum
	{
		NUM_ARMS = 6,					// three on each side
		NUM_JOINTS = 8					// each arm's joints: 0 at the shoulder, the last at the hand
	};

	// A joint of an arm
	struct ArmJoint
	{
		double		mX;					// +0x00
		double		mY;					// +0x08
	};

	// Where an arm's hand is going and the hand itself
	struct HandSlot
	{
		double		mOffsetX;			// +0x00 its place from the arm's shoulder, picked every 50 updates
		double		mOffsetY;			// +0x08
		double		mVelX;				// +0x10 its speed toward it
		double		mVelY;				// +0x18
		EyeHand*	mHand;				// +0x20 NULL once shot down
	};

	Sprite*			mBody;				// +0x70 images\eye\body (the boss's own sprite, images\eye\eye, is its open eye, 6 frames)
	Sprite*			mArm;				// +0x74 images\eye\arm, one for each joint
	Sprite*			mGlowSprite;		// +0x78 images\eye\glow, over the open eye
	Sprite*			mHandSprite;		// +0x7c images\eye\hand
	Sprite*			mBoltSprite;		// +0x80 images\eye\bolt
	Sprite*			mHandGlowSprite;	// +0x84 images\eye\handglow, over a hand charging a bolt
	double			mTargetX;			// +0x88 the place it drifts to, picked every 50 updates
	double			mTargetY;			// +0x90 (from 180 to 199)
	int				mGlow;				// +0x98 how bright the glow over its open eye is
	int				mShotDelay;			// +0x9c updates until the open eye fires again
	int				mFire;				// +0xa0 the data's fire: updates between its shots
	int				mHandArmor;			// +0xa4 the data's Hand armor
	int				mBoltHand;			// +0xa8 the arm charging or striking a bolt, -1 for none
	int				mBoltDelay;			// +0xac updates until it chooses a hand to charge a bolt
	int				mHandFire;			// +0xb0 the data's Hand fire: updates between its bolts
	int				mBoltTime;			// +0xb4 updates left of a bolt's strike, 0 when it is not striking
	double			mBoltCharge;		// +0xb8 the chosen hand's charge, 0 to 255
	int				mHandsLeft;			// +0xc0 hands not shot down; with none the eye stays open
	int				mCycle;				// +0xc4 updates it has spent low enough to act: the eye is open for the second half of each 1000
	ArmJoint		mArms[NUM_ARMS][NUM_JOINTS]; // +0xc8
	HandSlot		mHands[NUM_ARMS];	// +0x3c8

	EyeBoss(Board* theBoard);								// 0x43ba90
	virtual ~EyeBoss();										// 0x43aa70 (body 0x43a650)
	virtual void	Draw(Graphics* g) override;				// 0x43a700 slot 1
	virtual void	Update() override;						// 0x43aaa0 slot 2
	virtual void	Killed() override;						// 0x43a9e0 slot 4

private:
	static void		PullJoint(ArmJoint* theJoint, const ArmJoint* theNeighbour);	// port: part of 0x43aaa0
	void			ChooseBoltHand();										// port: part of 0x43aaa0
};

// One of the eye's hands: it sits at the end of its arm, takes the bullets' hits, and carries the bolt. It takes the
// eye's hand sprite (which the eye draws, so it has no Draw) and counts neither as spawned nor as destroyed.
// Original: 0x78 bytes, constructor 0x43c000, destructor 0x43c170 (body 0x43c070); vtable 0x5308e8: 2 Update
// 0x43c080, 4 Killed 0x43c0d0.
class EyeHand : public Enemy
{
public:
	EyeBoss*		mEye;				// +0x70
	int				mIndex;				// +0x74 which of the eye's arms it is on

	EyeHand(Board* theBoard, EyeBoss* theEye, int theIndex);	// 0x43c000
	virtual ~EyeHand();										// 0x43c170 (body 0x43c070)
	virtual void	Draw(Graphics* g) override {}			// 0x471be0 slot 1, empty: the eye draws it
	virtual void	Update() override;						// 0x43c080 slot 2
	virtual void	Killed() override;						// 0x43c0d0 slot 4
};

// The final boss (the last level): a flying fortress that fights in three phases as its health falls. Phase 0 holds
// near the middle and charges two lasers that drop bomb fragments; phase 1 (below two thirds of its health) turns a
// turret of shots and drops fat boys; phase 2 (below a third) fires hellfire and head missiles. Original: 0xe0
// bytes, constructor 0x43de30, destructor 0x43c5f0 (body 0x43c1a0); vtable 0x53090c: 1 Draw 0x43c240, 2 Update
// 0x43c620, 4 Killed 0x43c480, 5 Explode 0x43c510.
class FinalBoss : public Enemy
{
public:
	Sprite*			mBody1;				// +0x70 images\finalboss\body1 (also its sprite, +0x60, while in phase 0)
	Sprite*			mBody2;				// +0x74 images\finalboss\body2 (phase 1)
	Sprite*			mBody3;				// +0x78 images\finalboss\body3 (phase 2)
	Sprite*			mTurretSprite;		// +0x7c images\finalboss\turret (16 frames, the angle)
	Sprite*			mLaserSprite;		// +0x80 images\finalboss\laser (5 frames)
	Sprite*			mLaserGlowSprite;	// +0x84 images\finalboss\laserglow (5 frames)
	int				mPhase;				// +0x88 0, 1 or 2
	int				mAttackDelay;		// +0x8c phase 0: updates until it moves on (its laser charges below 80); phase 1: until the turret fires
	double			mTargetX;			// +0x90 the place it goes to
	double			mTargetY;			// +0x98
	int				mSmallLauncherDelay;// +0xa0 phase 2: updates until the pair of hellfire missiles
	int				mBigLauncherDelay;	// +0xa4 phase 2: updates until the pair of head missiles
	int				mBombDelay;			// +0xa8 phase 1: updates until the fat boy
	int				mLaserFire;			// +0xac the data's Laser fire (nothing reads it)
	int				mTurretFire;		// +0xb0 the data's Turret fire
	double			mLaserCharge;		// +0xb8 phase 0's laser charge, 0 to pi
	int				mSmallLauncherFire;	// +0xc0 the data's SmallLauncher fire
	int				mBigLauncherFire;	// +0xc4 the data's BigLauncher fire
	int				mBombFire;			// +0xc8 the data's Bomb fire
	int				mLastLaserKind;		// +0xcc the kind of bomb fragment its last laser dropped (it never drops the same twice running)
	double			mTurretFrame;		// +0xd0 phase 0: the lasers' frame; phase 1: the turret's, 0 to 16
	double			mLastHealth;		// +0xd8 its health at the last update, to see a phase's threshold crossed

	FinalBoss(Board* theBoard);								// 0x43de30
	virtual ~FinalBoss();									// 0x43c5f0 (body 0x43c1a0)
	virtual void	Draw(Graphics* g) override;				// 0x43c240 slot 1
	virtual void	Update() override;						// 0x43c620 slot 2
	virtual void	Killed() override;						// 0x43c480 slot 4
	virtual void	Explode() override;						// 0x43c510 slot 5: a phase's end

private:
	void			EmitSmoke(double theOffsetX, double theOffsetY, double theVelX);	// port: part of 0x43c620
};

// The head boss (level 7 and 16): a giant head that floats across the top of the field firing homing missiles from two
// launchers, and now and then charges across the whole field dropping iron bombs. Original: 0xbc bytes, constructor
// 0x43f310, destructor 0x43e7a0 (body 0x43e260); vtable 0x530ad8: 1 Draw 0x43e6e0, 2 Update 0x43e2c0, 4 Killed 0x43e710.
class HeadBoss : public Enemy
{
public:
	int				mLauncherDelay[2];	// +0x70 updates until each launcher fires (counts down only while a tank lives)
	int				mLauncherFire[2];	// +0x78 their delays between missiles (the data's Launcher1 and Launcher2 fire)
	int				mBombCounter;		// +0x84 counts down the "on" and "off" stretches of its bombing
	int				mBombOn;			// +0x88 updates of each bombing stretch it may drop bombs (the data's Bomb on)
	int				mBombOff;			// +0x8c updates between them (off)
	int				mBombFreq;			// +0x90 it drops one on each update of the app's count that divides by this
	double			mTargetX;			// +0x98 where it eases to
	double			mTargetY;			// +0xa0
	int				mChargeWait;		// +0xa8 updates it floats before its next charge (500 after each)
	bool			mIsCharging;		// +0xac it charges across the field
	double			mMaxSpeed;			// +0xb0 its speed limit (the data's speed)
	int				mChargeDelay;		// +0xb8 the data's Charge delay, which nothing reads

	HeadBoss(Board* theBoard);								// 0x43f310
	virtual ~HeadBoss();									// 0x43e7a0 (body 0x43e260)
	virtual void	Draw(Graphics* g) override;				// 0x43e6e0 slot 1
	virtual void	Update() override;						// 0x43e2c0 slot 2
	virtual void	Killed() override;						// 0x43e710 slot 4
};

// The helicopter boss (level 1 and 10): a huge helicopter that floats down into the field and wanders over it, firing
// hellfire missiles from two launchers and, from the second time it comes, bullets from two turrets. Original: 0xc4
// bytes, constructor 0x440300, destructor 0x43f8e0 (body 0x43f440); vtable 0x530b6c: 1 Draw 0x43f6f0, 2 Update 0x43f4d0,
// 4 Killed 0x43f800, 6 SilenceSounds 0x43f890, 7 RestoreSounds 0x43f8b0.
class HelicopterLauncher;
class HelicopterTurret;
class SoundInstance;

class HelicopterBoss : public Enemy
{
public:
	Sprite*				mBlades;			// +0x70 images\hugecopter\copterblades, 5 frames
	Sprite*				mTurretSprite;		// +0x74 its turrets' sprite, 21 frames; NULL on the first level
	Sprite*				mLauncherSprite;	// +0x78 its launchers' sprite
	HelicopterLauncher*	mLaunchers[2];		// +0x7c +0x80 left, right
	HelicopterTurret*	mTurrets[2];		// +0x84 +0x88 left, right; unset in the original on the first level
	double				mTurretArmor;		// +0x90 the data's Turret armor
	double				mLauncherArmor;		// +0x98 the data's Launcher armor
	int					mTurretFire;		// +0xa0 updates between a turret's bullets
	int					mLauncherFire;		// +0xa4 updates between a launcher's missiles
	bool				mTurretsMove;		// +0xa8 the data's Turret stationary is not "yes"; nothing reads it
	double				mLauncherScale;		// +0xb0 the data's Launcher speed, the missiles' scale
	double				mRotorFrame;		// +0xb8 the blades' frame, 0 to 5
	SoundInstance*		mSound;				// +0xc0 its rotor, looping, or NULL

	HelicopterBoss(Board* theBoard);						// 0x440300
	virtual ~HelicopterBoss();								// 0x43f8e0 (body 0x43f440)
	virtual void	Draw(Graphics* g) override;				// 0x43f6f0 slot 1
	virtual void	Update() override;						// 0x43f4d0 slot 2
	virtual void	Killed() override;						// 0x43f800 slot 4
	virtual void	SilenceSounds() override;				// 0x43f890 slot 6
	virtual void	RestoreSounds() override;				// 0x43f8b0 slot 7
};

// One of the helicopter's two launchers, hanging from it. Original: 0x98 bytes, constructor 0x440780, vtable 0x530c30:
// 2 Update 0x440820, 4 Killed 0x4409b0.
class HelicopterLauncher : public Enemy
{
public:
	HelicopterBoss*	mHelicopter;	// +0x70
	double			mOffsetX;		// +0x78 where it hangs from the helicopter's centre
	double			mOffsetY;		// +0x80
	int				mFireDelay;		// +0x88 updates until its next missile
	double			mScale;			// +0x90 the helicopter's missile scale

	HelicopterLauncher(Board* theBoard, HelicopterBoss* theHelicopter);	// 0x440780
	virtual ~HelicopterLauncher();							// 0x4409f0 (body 0x440810)
	virtual void	Update() override;						// 0x440820 slot 2
	virtual void	Killed() override;						// 0x4409b0 slot 4
};

// One of the helicopter's two turrets, hanging from it and sweeping back and forth. Original: 0x98 bytes, constructor
// 0x440a20, vtable 0x530c54: 2 Update 0x440ab0, 4 Killed 0x4409b0.
class HelicopterTurret : public Enemy
{
public:
	HelicopterBoss*	mHelicopter;	// +0x70
	double			mOffsetX;		// +0x78
	double			mOffsetY;		// +0x80
	double			mFrameSpeed;	// +0x88 frames it turns an update, 0.1 one way or the other
	int				mFireDelay;		// +0x90 updates until its next bullet

	HelicopterTurret(Board* theBoard, HelicopterBoss* theHelicopter);	// 0x440a20
	virtual ~HelicopterTurret();							// 0x440c60 (body 0x440aa0)
	virtual void	Update() override;						// 0x440ab0 slot 2
	virtual void	Killed() override;						// 0x4409b0 slot 4
};

// The rainer boss (level 3 and 12): a flying saucer with a propeller to each side and a dish below it that raises a
// tractor beam, drops air mines and calls down meteors. Original: 0xac bytes, constructor 0x441c30, destructor 0x441210
// (body 0x440c90); vtable 0x530c78: 1 Draw 0x441240, 2 Update 0x440d20, 4 Killed 0x441130, 6 SilenceSounds 0x4411c0,
// 7 RestoreSounds 0x4411e0.
class RainerBoss : public Enemy
{
public:
	Sprite*			mDish;			// +0x70 images\rainer\dish
	Sprite*			mProp;			// +0x74 its propellers, 8 frames
	Sprite*			mBeam;			// +0x78 the tractor beam's picture
	int				mMineTimer;		// +0x7c updates until its next air mine
	double			mPropFrame;		// +0x80 the propellers' frame, 0 to 8
	double			mDishAngle;		// +0x88 how far the dish has turned up, 0 to a quarter turn
	bool			mIsDishUp;		// +0x90 the dish is up, or turning up (the beam is on)
	int				mDishTimer;		// +0x94 updates left of the dish's down or up stretch
	int				mDishUpTime;	// +0x98 the data's Dish up
	int				mDishDownTime;	// +0x9c the data's Dish down
	int				mMineFire;		// +0xa0 updates between its air mines (the data's fire)
	int				mMeteors;		// +0xa4 meteors it calls down when the dish comes down (the data's Dish meteors)
	SoundInstance*	mSound;			// +0xa8 the beam's hum

	RainerBoss(Board* theBoard);							// 0x441c30
	virtual ~RainerBoss();									// 0x441210 (body 0x440c90)
	virtual void	Draw(Graphics* g) override;				// 0x441240 slot 1
	virtual void	Update() override;						// 0x440d20 slot 2
	virtual void	Killed() override;						// 0x441130 slot 4
	virtual void	SilenceSounds() override;				// 0x4411c0 slot 6
	virtual void	RestoreSounds() override;				// 0x4411e0 slot 7
};

// The robot boss (level 9 and 18): a giant robot that hops across the field after the tank,
// stamping the ground with its foot. Its two arms must be shot off first; its launchers fire hellfire, and it fires
// pairs of lasers. Original: 0xf4 bytes, constructor 0x443210, destructor 0x442690 (body 0x441f10); vtable 0x530d68:
// 1 Draw 0x442490, 2 Update 0x441fb0, 4 Killed 0x442600.
class RobotArm;
class RobotLauncher;

class RobotBoss : public Enemy
{
public:
	Sprite*			mWaist;				// +0x70 images\robot\waist
	Sprite*			mFoot;				// +0x74 images\robot\foot, also the sprite its foot hits the tank with
	Sprite*			mArmSprite;			// +0x78 the arms' sprite, 2 frames (left, right)
	Sprite*			mLauncherSprite;	// +0x7c the launchers' sprite, 2 frames
	Sprite*			mPanel;				// +0x80 its chest panel, 4 frames
	Sprite*			mEyeGlow;			// +0x84 its eye's glow
	RobotArm*		mArms[2];			// +0x88 +0x8c left, right; NULL once shot off
	RobotLauncher*	mLaunchers[2];		// +0x90 +0x94 left, right
	double			mFootY;				// +0x98 where its foot is, held to 410 at the ground
	double			mPrevFootY;			// +0xa0 where it was an update ago
	double			mPanelFrame;		// +0xa8 its chest panel's frame, 0 to 4
	int				mFireTimer;			// +0xb0 updates until its lasers fire (counts only with both arms gone and a tank)
	int				mFire;				// +0xb4 updates between its lasers (the data's fire)
	double			mLauncherArmor;		// +0xb8 the data's Launcher armor
	int				mLauncherFire;		// +0xc0 updates between a launcher's missiles
	double			mArmArmor;			// +0xc8 the data's Arm armor
	double			mDownAccel;			// +0xd0 its fall's acceleration while it is above 210 (the data's Jump)
	double			mDownMax;			// +0xd8 and its fastest fall
	double			mUpAccel;			// +0xe0 its push up from the ground's acceleration
	double			mUpMax;				// +0xe8 and its fastest rise
	int				mShake;				// +0xf0 how hard the screen shakes (0x80 when it lands, down 2 an update)

	RobotBoss(Board* theBoard);								// 0x443210
	virtual ~RobotBoss();									// 0x442690 (body 0x441f10)
	virtual void	Draw(Graphics* g) override;				// 0x442490 slot 1
	virtual void	Update() override;						// 0x441fb0 slot 2
	virtual void	Killed() override;						// 0x442600 slot 4
};

// One of the robot's two arms, hanging from it. Original: 0x70 bytes before the robot's pointer, constructor
// 0x443720, vtable 0x530e98: 2 Update 0x4437d0, 4 Killed 0x443850. Its frame (+0x28) is its side: 0 left, 1 right.
class RobotArm : public Enemy
{
public:
	RobotBoss*		mRobot;			// +0x70

	RobotArm(Board* theBoard, RobotBoss* theRobot, int theSide);	// 0x443720
	virtual ~RobotArm();									// 0x4438c0 (body 0x4437c0)
	virtual void	Update() override;						// 0x4437d0 slot 2
	virtual void	Killed() override;						// 0x443850 slot 4
};

// One of the robot's two shoulder launchers. Original: constructor 0x443d00, vtable 0x530ed0: 2 Update 0x443db0, 4 Killed
// 0x4409b0. Its frame (+0x28) is its side.
class RobotLauncher : public Enemy
{
public:
	RobotBoss*		mRobot;			// +0x70
	int				mFireTimer;		// +0x74 updates until its next missile

	RobotLauncher(Board* theBoard, RobotBoss* theRobot, int theSide);	// 0x443d00
	virtual ~RobotLauncher();								// 0x443ed0 (body 0x443da0)
	virtual void	Update() override;						// 0x443db0 slot 2
	virtual void	Killed() override;						// 0x4409b0 slot 4
};

// The robot's laser: a bolt fired at the tank's place when it leaves. Original: 0xa4 bytes, constructor 0x4438f0,
// destructor 0x443b70 (body 0x4439f0); vtable 0x530ebc: 1 Draw 0x443ba0, 2 Update 0x443a00.
class RobotLaser : public Projectile
{
public:
	double			mTailX;			// +0x78 where its tail is (its head is mX, mY)
	double			mTailY;			// +0x80
	double			mDirX;			// +0x88 its head's speed
	double			mDirY;			// +0x90
	double			mAngle;			// +0x98 the tank's bearing from where it left, plus a quarter turn
	bool			mHasLanded;		// +0xa0 its head has reached the ground

	RobotLaser(Board* theBoard, double x, double y);		// 0x4438f0
	virtual ~RobotLaser();									// 0x443b70 (body 0x4439f0)
	virtual void	Draw(Graphics* g) override;				// 0x443ba0 slot 1
	virtual void	Update() override;						// 0x443a00 slot 2
};

// The worm boss (level 8 and 17): a giant sand worm that leaps out of the ground in arcs, a chain of 30 ribs behind its
// head, with three turrets on its back that drop iron bombs. With the turrets shot off it leaps faster and fires
// hellfire; it throws boulders where it dives back into the sand. Original: 0x2e8 bytes, constructor 0x445800,
// destructor 0x444280 (body 0x443f00); vtable 0x530ef4: 1 Draw 0x443f80, 2 Update 0x4442b0, 4 Killed 0x444120.
class WormTurret;

class WormBoss : public Enemy
{
public:
	enum
	{
		NUM_SEGMENTS = 30,			// the head and its ribs
		NUM_TURRETS = 3
	};

	Sprite*			mRib;						// +0x70 images\worm\rib, 4 frames
	Sprite*			mTurretSprite;				// +0x74 images\worm\turret
	Sprite*			mSand;						// +0x78 the sand it sprays, 4 frames
	WormTurret*		mTurrets[NUM_TURRETS];		// +0x7c until each is shot off
	double			mSegX[NUM_SEGMENTS];		// +0x88 where the head (0) and each rib is
	double			mSegY[NUM_SEGMENTS];		// +0x178
	int				mTurretFire;				// +0x268 updates between a turret's bombs
	int				mTurretArmor;				// +0x26c
	int				mJumpDepth[2];				// +0x270 how deep it digs before a jump: slow (turrets alive), fast
	double			mJumpSpeed[2];				// +0x278 its fastest fall (the data's jump), slow and fast
	double			mJumpGravity[2];			// +0x288 its fall's acceleration
	double			mJumpLateral[2];			// +0x298 its fastest sideways speed while it turns toward the tank
	int				mFire;						// +0x2a8 updates between its hellfire once the turrets are gone
	int				mBoulders;					// +0x2ac boulders it throws where it dives in
	int				mDepth;						// +0x2b0 the depth its ribs start at: the slow or the fast one
	double			mCurrentJumpSpeed;			// +0x2b8
	double			mCurrentGravity;			// +0x2c0
	int				mFireTimer;					// +0x2c8 updates until its next hellfire
	double			mCurrentLateral;			// +0x2d0
	int				mSandX;						// +0x2d8 where the sand sprays
	int				mSandAlpha;					// +0x2dc its opacity, 0 to 255
	int				mSandFade;					// +0x2e0 how fast it changes (8 while it sprays, then -2)
	bool			mIsFast;					// +0x2e4 the turrets are gone

	WormBoss(Board* theBoard);								// 0x445800
	virtual ~WormBoss();									// 0x444280 (body 0x443f00)
	virtual void	Draw(Graphics* g) override;				// 0x443f80 slot 1
	virtual void	Update() override;						// 0x4442b0 slot 2
	virtual void	Killed() override;						// 0x444120 slot 4

	void			NewJump(bool isFirst);					// 0x4441b0
};

// One of the worm's three turrets, riding its ribs 9, 19 and 29. Original: constructor 0x445b90, vtable 0x530fc4:
// 1 Draw 0x471be0 (empty: the worm draws them), 2 Update 0x445c30, 4 Killed 0x445e20.
class WormTurret : public Enemy
{
public:
	WormBoss*		mWorm;			// +0x70
	int				mIndex;			// +0x74 which turret, 0 to 2
	double			mFrameSpeed;	// +0x78 0.05, which nothing reads
	int				mFireTimer;		// +0x80 updates until its next bomb

	WormTurret(Board* theBoard, WormBoss* theWorm, int theIndex);	// 0x445b90
	virtual ~WormTurret();									// 0x445e60 (body 0x445c20)
	virtual void	Draw(Graphics* g) override {}			// 0x471be0 slot 1, empty
	virtual void	Update() override;						// 0x445c30 slot 2
	virtual void	Killed() override;						// 0x445e20 slot 4
};

// The wrecker boss (level 4 and 13): a crane on four wheels that drives in from the right and swings a wrecking ball on a
// chain over the field. Its chain pays out while the tank lives. Original: 0x110 bytes, constructor 0x447210,
// destructor 0x446c00 (body 0x445e90); vtable 0x530ff0: 1 Draw 0x4466f0, 2 Update 0x445f60, 4 Killed 0x446a30,
// 6 SilenceSounds 0x446b50, 7 RestoreSounds 0x446b80.
class SoundInstance;

class WreckerBoss : public Enemy
{
public:
	// The ball once the boss is dead: it falls on its own (0x20 bytes)
	struct FallingBall
	{
		double		mX;
		double		mY;
		double		mVelX;
		double		mVelY;
	};

	Sprite*			mBody;				// +0x70 images\wrecker\body
	Sprite*			mWheel;				// +0x74 its wheels, 12 frames
	Sprite*			mLink;				// +0x78 a link of the chain
	Sprite*			mBall;				// +0x7c the wrecking ball
	Sprite*			mShadow;			// +0x80 the ball's shadow on the ground
	double			mWheelFrame;		// +0x88 its wheels' frame, 0 to 12
	double			mApproachX;			// +0x90 where the crane is (640 at first, driving in to 0)
	double			mApproachVel;		// +0x98 how fast it drives
	double			mSwingA;			// +0xa0 the slow swing of the hook (radians)
	double			mSwingB;			// +0xa8 and the quick one
	double			mBallX;				// +0xb0 the ball
	double			mBallY;				// +0xb8
	double			mBallVelX;			// +0xc0
	double			mBallVelY;			// +0xc8
	double			mChainLength;		// +0xf0 how much chain is out, 200 to mLongChain
	bool			mHasSlammed;		// +0xf8 the ball has already hit the ground this time
	int				mShake;				// +0xfc how hard the screen shakes (0x80 when the ball slams, down 2 an update)
	SoundInstance*	mEngineSound;		// +0x100 its engine, always running
	SoundInstance*	mChainSound;		// +0x104 the chain's clatter while it pays out or draws in, or NULL
	FallingBall*	mFallingBall;		// +0x108 the ball after the boss has died, or NULL
	int				mLongChain;			// +0x10c its longest chain: 0x170 (the data's longchain is "yes") or 0x160

	WreckerBoss(Board* theBoard);							// 0x447210
	virtual ~WreckerBoss();									// 0x446c00 (body 0x445e90)
	virtual void	Draw(Graphics* g) override;				// 0x4466f0 slot 1
	virtual void	Update() override;						// 0x445f60 slot 2
	virtual void	Killed() override;						// 0x446a30 slot 4
	virtual void	SilenceSounds() override;				// 0x446b50 slot 6
	virtual void	RestoreSounds() override;				// 0x446b80 slot 7

	void			ReleaseChainSound();					// port: Update's 0x4466c6
};

}

#endif
