#ifndef __TYPERSHARK_PROJECTILE_H__
#define __TYPERSHARK_PROJECTILE_H__

// Projectile.cpp: a shot that flies across the board: Torpedo and CannonBall (and through Torpedo the MechaTorpedo and
// SquidCannonBall) derive from it. It is a creature without a word of its own that the board moves by its velocity;
// each tick it looks whether it has hit the player's cannon (a shot flying left) or its target (a shot flying right) or
// has left the screen, and tells the board by pushing an event.
// Original: class Sexy::Projectile : Enemy, GameObject (virtual bases), vftable 0x529458 (own part at +0, 2 virtuals
// 0x401820 and 0x4018c0), 0x529424 (GameObject part at +0x20), 0x529414 (Enemy part at +0x118), sizeof 0x208; the
// constructor is 0x40acc0 (its last argument says whether it builds the virtual bases itself: the most derived class
// does, as in C++), the deleting destructor 0x4098e0 (the export names it GameObject::ctor_or_dtor_4098e0). The field
// comments give the offsets in the original's own part (vptr at +0, vbptr at +4).
//
// Events (pushed on Enemy::mEvents, the board reads them): 0x13 the shot hit the player, 0x14 it hit its target, 0x15
// and then 5 it left the screen (see Update).
//
// port: the single chain GameObject, Enemy, Projectile; nothing derives from Projectile through a second path, so it
// needs no virtual inheritance. The original keeps the target as a pointer to a part with a vbptr at +4 and reaches its
// GameObject through the vbtable; the port stores that GameObject. The player is a pointer to the GameObject itself.

#include "Enemy.h"

namespace Sexy
{

class Graphics;
class ImageFont;

class Projectile : public Enemy
{
public:
	ImageFont*				mProjFont;				// +0x08 constructor: the app's Courier font (app +0x8d4); not used by this class
	GameObject*				mHitTarget;				// +0x0c constructor argument 2: the creature a shot flying right can hit
	GameObject*				mPlayer;				// +0x10 constructor argument 3: the player's cannon, hit when its state is 9

public:
	Projectile(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer);	// 0x40acc0
	virtual ~Projectile();															// deleting destructor 0x4098e0

	virtual bool			HitsPlayer();											// own slot 0, 0x401820
	virtual bool			HitsTarget();											// own slot 1, 0x4018c0

	virtual void			Update() override;										// GameObject slot 1, 0x40adf0
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x4017f0
	virtual void			SetState(int theState) override;						// slot 7, 0x4017a0
};

}

#endif
