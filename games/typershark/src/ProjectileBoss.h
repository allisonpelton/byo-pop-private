#ifndef __TYPERSHARK_PROJECTILEBOSS_H__
#define __TYPERSHARK_PROJECTILEBOSS_H__

// ProjectileBoss.cpp: the base of the bosses that fire shots (TorpedoBoss and through it GhostShip, MechaShark and
// MechaSquid). It floats in from the right (state 0), bobs up and down between two heights, waits for a second
// (state 6), then fires (state 7) and waits for the player's cannon (state 8) to shoot again. It owns the shots in
// flight (mShots), updates them each tick and asks the boss (HandleShot) what each shot has told it; a boss with no
// hit points left falls out of the sea (state 3). It never reads a word of its own; the shots carry the words.
// Original: class Sexy::ProjectileBoss : Enemy, GameObject (virtual bases), the Enemy part at +0 (vftable 0x52996c, 5
// virtuals: the three of Enemy, then Move 0x4019b0 and HandleShot 0x4083b0), vbptr at +4, own fields from +0xe8, the
// GameObject part at +0x178 (vftable 0x529938, 0xf0 bytes); the constructor is 0x40b030 (its last argument
// says whether it builds the virtual bases itself), the destructor of the part 0x4082a0, the deleting destructor
// thunk 0x4083c0. The field comments give the offsets in the original's object (the Enemy fields end at +0xe4).
//
// States (GameObject::mState): 0 floating in from the right (mX -= mVelX until it reaches mMinX), 6 bobbing and
// waiting a second, 7 firing (bobbing, and sideways too), 8 bobbing until the player's cannon is ready (state 9) again,
// 3 falling up out of the picture (dying). Events (pushed on Enemy::mEvents, the board reads them): 10 at creation,
// 0xb on entering state 7, 0xc when the hit points have run out, 5 when the dying boss has left the picture.
//
// port: the single chain GameObject, Enemy, ProjectileBoss; nothing derives from it through a second path, so no
// virtual inheritance. The vtordisp thunks (GameObject slots 0, 1, 2, 3, 4, 7: 0x4083c0, 0x40e080, 0x4083d0, 0x4083e0,
// 0x4083f0, 0x40e090) only adjust the pointer; Draw (0x401a70) is GameObject::Draw and the two sizes are the Enemy
// level 0x402180 and 0x4021a0, so they are not overridden here.

#include "Enemy.h"
#include <vector>

namespace Sexy
{

class Projectile;
class WSTimer;
class LessonManager;

class ProjectileBoss : public Enemy
{
public:
	std::vector<Projectile*> mFinishedShots;	// port (leak fix, docs/LEAKS.md): the shots the boss has let go (HandleShot said true); the original drops them without deleting. Update leaves them here for the board, which takes them (Board::Update) so that it can free them with the board; the destructor frees any it still holds
	std::vector<Projectile*> mShots;			// +0xe8 (the allocator at +0xe8, the pointers at +0xec, +0xf0, +0xf4) the shots in flight, owned: the destructor deletes each
	WSTimer*				mTimer;					// +0xf8 new in the constructor, paused (SetDisabled(true)), added to the widget manager; times the pauses in states 6 and 7
	double					mFireInterval;			// +0x100 milliseconds between two volleys (the boss record's 0x40, 1000.0 in the abyss mode); not read by this class
	int						mHitPoints;				// +0x108 hit points left (100 in the constructor, then the record's 0x48, 200 in the abyss mode); SetState(8) with none left kills the boss
	int						mMaxHitPoints;				// +0x10c the hit points the boss started with (the same value as mHitPoints from the constructor on; the subclass constructors set both): the boss's health bar in BoardDraw divides mHitPoints by it
	double					mMinY;					// +0x110 the bobbing turns round below this (not set here; the subclass constructor sets it)
	double					mMaxY;					// +0x118 and above this
	double					mMinX;					// +0x120 the left end of the boss's way (floating in stops here; state 7 turns round)
	double					mMaxX;					// +0x128 the right end of the sideways way in state 7
	int						mMinShots;				// +0x130 not set here; TorpedoBoss's constructor sets it (the record's MinMissiles at 0x4c, or 2), GhostShip and MechaSquid set 2: the fewest shots in a volley, the counterpart of mMaxShots; never read
	int						mMaxShots;				// +0x134 not set here; TorpedoBoss's constructor sets it (the record's 0x50, or 3): how many shots at most are in flight
	double					mBobVelY;				// +0x138 the bobbing speed (the record's 0x58, 0.2 in the abyss mode); turns sign at the ends of the way
	double					mBobVelX;				// +0x140 the sideways speed of state 7, a copy of mBobVelY from the constructor
	double					mShotSpeedMin;			// +0x148 not set here; TorpedoBoss's constructor sets it: the slowest a shot flies
	double					mShotSpeedMax;			// +0x150 and the fastest
	LessonManager*			mConfig;				// +0x158 constructor argument 3: the lesson manager, whose records give the numbers of the boss (and whose abyss mode flag says whether it is the abyss)
	int						mAlpha;					// +0x15c constructor 255: the alpha of the colour the dying boss is drawn with (TorpedoBoss::Draw); nothing ever changes it
	GameObject*				mPlayer;				// +0x160 constructor argument 2: the player's cannon, ready for a shot when its state is 9

public:
	ProjectileBoss(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig);	// 0x40b030
	virtual ~ProjectileBoss();																// destructor 0x4082a0, deleting thunk 0x4083c0

	virtual void			Move();															// Enemy part slot 3, 0x4019b0
	virtual bool			HandleShot(Projectile* theShot);								// Enemy part slot 4, 0x4083b0 (here: false)
	virtual void			Update() override;												// GameObject slot 1, 0x40b360
	virtual void			SetState(int theState) override;								// slot 7, 0x40b240
};

}

#endif
