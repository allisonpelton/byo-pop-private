#ifndef __HEAVYWEAPON_ENEMY_H__
#define __HEAVYWEAPON_ENEMY_H__

// Enemy.cpp: the base of every craft on the field, the enemies, the bosses and the friendly units: where it is, its
// sprite, its health, its place in the board's list, and the board's counts of them.
// Original: 0x6c bytes, constructor 0x431550, destructor 0x431600 (body 0x431250); vtable 0x530194: 0 destructor,
// 1 Draw 0x431260, 2 Update 0x4312e0, 3 UpdateF 0x471be0, 4 Killed 0x431330, 5 Explode 0x431480, 6 SilenceSounds
// and 7 RestoreSounds 0x4011a0 (both empty), 8 ChooseSide 0x431650. The board keeps them in its list at +0x1f4 (the
// list's Remove of one node, which deletes it: 0x4314f0).

#include "ObjectList.h"

namespace Sexy
{

class Board;
class Graphics;
class HeavyWeaponApp;
class Sprite;

class Enemy
{
public:
	double					mX;				// +0x08 from the screen's centre
	double					mY;				// +0x10
	double					mVelX;			// +0x18
	double					mVelY;			// +0x20
	double					mFrame;			// +0x28 its sprite's column
	bool					mMirrored;		// +0x30 it came in from the right, flying left (the art faces right)
	int						mHitFlash;		// +0x34 255 when a bullet hits it, down 24 an update: its sprite is drawn red
	bool					mIsFriendly;	// +0x38 one of the player's units
	double					mMaxHealth;		// +0x40 the bosses' full health, for their bar and a nuke's damage
	bool					mIsAlive;		// +0x48 not yet killed (a boss stays on the list while it dies)
	Board*					mBoard;			// +0x4c
	HeavyWeaponApp*			mApp;			// +0x50
	double					mHealth;		// +0x58 its armour left; below 0 it is killed
	Sprite*					mSprite;		// +0x60
	int						mPoints;		// +0x64 the points for killing it
	ObjectList<Enemy>::Node* mNode;			// +0x68 its node in the board's list

public:
	// It adds itself to theBoard's list and counts itself
	Enemy(Board* theBoard);									// 0x431550
	virtual ~Enemy();										// 0x431600

	virtual void	Draw(Graphics* g);						// 0x431260 slot 1
	virtual void	Update();								// 0x4312e0 slot 2
	virtual void	UpdateF(float theFrac);					// 0x471be0 slot 3, empty
	virtual void	Killed();								// 0x431330 slot 4
	virtual void	Explode();								// 0x431480 slot 5
	virtual void	SilenceSounds();						// 0x4011a0 slot 6, empty: a craft with a looping sound mutes it
	virtual void	RestoreSounds();						// 0x4011a0 slot 7, empty: and turns it back up
	virtual void	ChooseSide();							// 0x431650 slot 8

	Sprite*			GetSprite();							// 0x431300
	int				GetCelWidth();							// 0x431310
	int				GetCelHeight();							// 0x431320
	void			Remove();								// 0x431630; deletes it
};

}

#endif
