#ifndef __HEAVYWEAPON_PROJECTILE_H__
#define __HEAVYWEAPON_PROJECTILE_H__

// Projectile.cpp: the base of the enemies' bigger shots (bombs, missiles, mines, lasers): where it is, its sprite, its
// health against the tank's bullets, and its hit on the tank's shield.
// Original: 0x64 bytes, constructor 0x447760, destructor 0x447810 (body 0x4476a0); vtable 0x5310f8: 0 destructor,
// 1 Draw 0x4476b0, 2 Update 0x447890. The board keeps them in its list at +0x214 (the list's Remove of one node,
// which deletes it: 0x4314f0, the enemies'). Its double at +0x38 is set to 0 and never read, and is left out.

#include "ObjectList.h"

namespace Sexy
{

class Board;
class Graphics;
class Sprite;

class Projectile
{
public:
	int							mPoints;			// +0x08 for shooting it down (10 unless the kind says)
	double						mX;					// +0x10 from the screen's centre
	double						mY;					// +0x18
	double						mVelX;				// +0x20
	double						mVelY;				// +0x28
	double						mFrame;				// +0x30 its sprite's column
	bool						mMirrored;			// +0x40
	bool						mIsDestructible;	// +0x41 bullets and nukes can destroy it
	double						mHealth;			// +0x48 below 0 a bullet has destroyed it
	int							mHitFlash;			// +0x50 255 when a bullet hits it, down 24 an update: drawn red
	bool						mIsShootable;		// +0x54 bullets can hit it now
	bool						mIsInBackground;	// +0x55 far off, drawn behind the far background, out of reach
	Board*						mBoard;				// +0x58
	Sprite*						mSprite;			// +0x5c
	ObjectList<Projectile>::Node* mNode;			// +0x60 its node in the board's list

public:
	// It adds itself to theBoard's list and counts itself
	Projectile(Board* theBoard);					// 0x447760
	virtual ~Projectile();							// 0x447810

	virtual void	Draw(Graphics* g);				// 0x4476b0 slot 1
	// The shield's hit, which the kinds call at the end of their own: it may delete the projectile
	virtual void	Update();						// 0x447890 slot 2

	Sprite*			GetSprite();					// 0x447730
	int				GetCelWidth();					// 0x447740
	int				GetCelHeight();					// 0x447750
	void			Remove();						// 0x447840; deletes it
	void			HitGround();					// 0x447860: a crater where it lands; deletes it
};

}

#endif
