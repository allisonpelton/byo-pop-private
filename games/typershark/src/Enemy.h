#ifndef __TYPERSHARK_ENEMY_H__
#define __TYPERSHARK_ENEMY_H__

// Enemy.cpp: a creature with a word on it that the player types to kill it. Enemy derives from GameObject (the second
// virtual base of every creature class in the original); Shark, Piranha, Projectile and the bosses derive from Enemy.
// Original: class Sexy::Enemy (vftable 0x5298fc with 3 virtuals; its standalone layout puts the part below at +0 and
// the GameObject at +0xf0, the fields' comments give offsets inside the Enemy part, see docs/ENTITIES.md).
// Constructor 0x40a3f0, destructor 0x4078b0.

#include "GameObject.h"
#include "Color.h"
#include <string>
#include <vector>

namespace Sexy
{

class Image;
class Font;
class WinWSApp;

class Enemy : public GameObject
{
public:
	// ---- the Enemy part ----
	bool					mFlag08;				// +0x08 ctor 0
	WinWSApp*				mApp;					// +0x0c the game (constructor argument)
	Enemy*					mTarget;				// +0x10 another creature this one is tied to (ReleaseTarget, 0x4022d0); set with 0x401250
	Enemy*					mAttacker;				// +0x14 the back link: its mTarget is this creature (set with 0x401260)
	bool					mFlag18;				// +0x18 ctor 0
	bool					mFlag19;				// +0x19 ctor 0
	bool					mFlag1a;				// +0x1a ctor 0
	bool					mFlag1b;				// +0x1b ctor 1
	bool					mFlag1c;				// +0x1c ctor 1
	bool					mFlag1d;				// +0x1d ctor 1
	int						mField20;				// +0x20 not set by this constructor; the subclass constructors do (Shark 0, Piranha 6)
	int						mField24;				// +0x24 likewise (Shark 0xb, Piranha 0)
	int						mField28;				// +0x28 not set by this constructor
	int						mField2c;				// +0x2c not set by this constructor; Piranha::Update counts it down
	bool					mFlag30;				// +0x30 not set by this constructor; Piranha::Update reads it
	std::string				mWord;					// +0x34 the word to type (starts empty)
	bool					mFlag50;				// +0x50 ctor 0
	int						mField54;				// +0x54 ctor 0
	int						mNumTyped;				// +0x58 letters typed so far; never reaches the length (TypeLetter)
	std::vector<Image*>		mImages;				// +0x5c six pictures by role (0 swim, 1, 2 death, 3 mirrored swim, 4 red; set with 0x403770)
	Image*					mImage;					// +0x6c the picture drawn (the subclass constructors set it)
	Color					mTextColor;				// +0x70 colour of the remaining letters (default Color())
	Color					mTypedColor;			// +0x80 colour of the typed letters
	Font*					mFont;					// +0x90 constructor: the game's font at +0x8c4
	Font*					mTypedFont;				// +0x94 constructor: the game's font at +0x8c8
	std::string				mTyped;					// +0x98 the typed part of the word
	std::string				mRemaining;				// +0xb4 the rest
	bool					mFlagD0;				// +0xd0 not set by this constructor; the subclasses clear it
	std::vector<int>		mEvents;				// +0xd4 event codes for the board (the creature pushes 1 when its word is finished, 2, 5, 6, 9...)

public:
	Enemy(WinWSApp* theApp);														// 0x40a3f0
	virtual ~Enemy();																// 0x4078b0

	virtual int				GetPoints();											// slot 0 0x403130: what the word is worth, 1..4 a letter times 15
	virtual double			GetSpeed();												// slot 1 0x4011f0: mVelX
	virtual void			TypeLetter();											// slot 2 0x40a5b0: one more letter typed

	void					SetTarget(Enemy* theTarget) { mTarget = theTarget; }	// 0x401250
	void					SetAttacker(Enemy* theAttacker) { mAttacker = theAttacker; }	// 0x401260
	void					ReleaseTarget();										// 0x4022d0
};

}

#endif
