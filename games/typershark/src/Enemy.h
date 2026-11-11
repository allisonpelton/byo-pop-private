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
	bool					mMovedToFront;				// +0x08 constructor false; set once when a dead shark or piranha (state 3) has been moved to the front of the board's list, so that it is not moved again
	WinWSApp*				mApp;					// +0x0c the game (constructor argument)
	Enemy*					mTarget;				// +0x10 another creature this one is tied to (ReleaseTarget, 0x4022d0); set with 0x401250
	Enemy*					mAttacker;				// +0x14 the back link: its mTarget is this creature (set with 0x401260)
	bool					mDeathFinished;				// +0x18 constructor false; the death animation has reached its last frame (set once with event 6 pushed by Shark, Piranha, Torpedo and BonusCreature, and by ProjectileBoss without one): IsDeathFinished reads it
	bool					mStealthy;				// +0x19 constructor false; the stealth creatures set it in their constructors and clear it when they turn solid (SetStealth(false), Update at x <= 400): while it is set they are drawn faint with their own alpha and move by their base speed; BoardKeys takes no letter for a bonus creature while it is set
	bool					mPlayerDiesSoundPlayed;				// +0x1a constructor false; the shark has played the sound of the diver dying as it came up to him (Shark::Update, state 1); the typing tutor sets it so that its shark stays silent (only Shark reads it)
	bool					mAlwaysTrueA;				// +0x1b constructor true; set to true again by the Projectile constructor and the typing tutor; never read
	bool					mAlwaysTrueB;				// +0x1c constructor true; set to true again by the Projectile constructor and the typing tutor; never read
	bool					mAlwaysTrueC;				// +0x1d constructor true; set to true again by the Projectile constructor and the typing tutor; never read
	int						mTextOffsetY;				// +0x20 y offset of the word from the picture; the subclass constructors set it (Shark 0, Piranha 6)
	int						mTextOffsetX;				// +0x24 x offset of the word (Shark 0xb, Piranha 0)
	int						mShockPeriod;				// +0x28 not set by this constructor; SetState(5) sets it to 7: the ticks between two flips of mShockShowing in the shocked state (Shark::Update and Piranha::Update test mTick against it)
	int						mShockTogglesLeft;				// +0x2c not set by this constructor; SetState(5) sets it to 6: the flips of mShockShowing still to come (Shark::Update and Piranha::Update count it down; at 0 the creature goes on to state 3, dying); other states leave it at 0
	bool					mShockShowing;				// +0x30 not set by this constructor; in the shocked state (5) the shock picture is showing: SetState(5) sets it, Update flips it every mShockPeriod ticks, any other state clears it; Shark, Piranha, GhostShark and ToxicShark read it to pick the picture
	std::string				mWord;					// +0x34 the word to type (starts empty)
	bool					mKilledByEmp;				// +0x50 constructor false; the board's EMP wave sets it when it kills the creature (points already scored, SetState(5) follows): SetState then skips the dying sound, a black shark or white piranha with a second word dies at once instead of taking it on, and their IsSwimming no longer counts it once it is out of the swimming states
	int						mWordPoints;				// +0x54 ctor 0; the points of the word as it stands (SetWord and the TypeLetter of the multi-word creatures store GetPoints()); the board adds it to the score when the word is finished or the EMP wave kills the creature
	int						mNumTyped;				// +0x58 letters typed so far; never reaches the length (TypeLetter)
	std::vector<Image*>		mImages;				// +0x5c six pictures by role (0 swim, 1, 2 death, 3 mirrored swim, 4 red; set with 0x403770)
	Image*					mImage;					// +0x6c the picture drawn (the subclass constructors set it)
	Color					mTextColor;				// +0x70 colour of the remaining letters (default Color())
	Color					mTypedColor;			// +0x80 colour of the typed letters
	Font*					mFont;					// +0x90 constructor: the game's font at +0x8c4
	Font*					mTypedFont;				// +0x94 constructor: the game's font at +0x8c8
	std::string				mTyped;					// +0x98 the typed part of the word
	std::string				mRemaining;				// +0xb4 the rest
	bool					mBiteEventSent;				// +0xd0 not set by this constructor; the subclasses clear it; Shark and Piranha set it when they push event 2 (the diver is bitten), and the typing tutor sets it so that its creatures never bite
	std::vector<int>		mEvents;				// +0xd4 event codes for the board (the creature pushes 1 when its word is finished, 2, 5, 6, 9...)
	int						mEnemyType;				// +0xe4 not set by this constructor; the EnemyType (LessonManager.h) of the spawn record the board made the creature from, or the boss kind in the abyss (0xc torpedo boss, 0xe ghost ship, 0xf mecha shark, 0x10 mecha squid); the board reads it to space the words of neighbouring creatures, to count a boss kill in the profile and to tell which bosses leave the letters to the creatures around them

public:
	Enemy(WinWSApp* theApp);														// 0x40a3f0
	virtual ~Enemy();																// 0x4078b0

	virtual int				GetPoints();											// slot 0 0x403130: what the word is worth, 1..4 a letter times 15
	virtual double			GetSpeed();												// slot 1 0x4011f0: mVelX
	virtual void			TypeLetter();											// slot 2 0x40a5b0: one more letter typed
	virtual void			SetState(int theState) = 0;								// GameObject slot 7: every creature class overrides it

	void					SetTarget(Enemy* theTarget) { mTarget = theTarget; }	// 0x401250
	void					SetAttacker(Enemy* theAttacker) { mAttacker = theAttacker; }	// 0x401260
	void					ReleaseTarget();										// 0x4022d0

	// ---- added for E2 (the bosses use them) ----
	int						PopEvent();												// 0x407070
	bool					HasEvent(int theEvent);									// 0x403790
	void					SetWord(const std::string& theWord);					// 0x407a30

	// ---- added for E3 ----
	bool					IsDeathFinished();										// 0x401230 (the export puts it in Board): state 3 with mDeathFinished set
	void					RemoveEvent(int theEvent);								// 0x407100 (the export puts it in Board): every entry equal to the event leaves the list
};

}

#endif
