#ifndef __TYPERSHARK_TORPEDOBOSS_H__
#define __TYPERSHARK_TORPEDOBOSS_H__

// TorpedoBoss.cpp: the boss submarine that fires torpedoes (Torpedo, each with a word to type) from up to three places
// in its hull, one shot at each. GhostShip, MechaShark and MechaSquid derive from it and change what it fires
// (CreateProjectile) and what word the shots carry (GetWord).
// Original: class Sexy::TorpedoBoss : ProjectileBoss, Enemy, GameObject (all virtual bases), the Enemy part at +0
// (vftable 0x529a70, 7 virtuals: the five of ProjectileBoss, then GetWord 0x4055c0 and CreateProjectile 0x40d5b0; its
// HandleShot is 0x40cba0), vbptr at +4, the GameObject part at +0x1b8 (vftable 0x529a3c, 0xf0 bytes); the constructor
// is 0x40c630 (its last argument says whether it builds the virtual bases itself), the destructor of the part 0x408d00
// (it calls ProjectileBoss's 0x4082a0 first and again at its end; the second call finds all freed). The field comments
// give the offsets in the original's object; ProjectileBoss's fields end at +0x160.
//
// The shots' places: the boss has three, 1 to 3, with the offset of the shot's y from the boss's y at +0x178, +0x17c and
// +0x180 (0x41, 0x65, 0x87), and a flag for each that says the place is taken (+0x184, +0x185 and, out of line, +0x198).
// A shot is put in the first free place where it overlaps none of the shots flying right (ShotOverlaps) and carries the
// place in Projectile::mSlot; the place is freed again when the shot is gone (event 5) or its word is typed (event 1).
//
// Events the boss pushes on Enemy::mEvents for the board, besides ProjectileBoss's: 0xd for each shot it fires, 1 when
// a shot's word was typed (the shot then turns back), 0x13 when a shot hit the player's cannon, 0x15 when a shot has
// left the screen, 6 when a shot flying left sends 6 (HandleShot passes the shots' events on)
//
// port: the single chain GameObject, Enemy, ProjectileBoss, TorpedoBoss; nothing derives from it through a second path,
// so no virtual inheritance. The vtordisp thunks (GameObject slots 0, 1, 2, 3, 4 and 7: 0x408df0, 0x40e0a0, 0x408e00,
// 0x408dd0, 0x408de0, 0x40e0f0) only adjust the pointer.

#include "ProjectileBoss.h"
#include <string>
#include <vector>

namespace Sexy
{

class Graphics;
class Image;
class WSTimer;

class TorpedoBoss : public ProjectileBoss
{
public:
	// one explosion of the dying boss: a frame (0 to 7) of the 8-frame picture of IMAGE_TORPEDO_EXPLODE at a place; 24 bytes
	struct ExplosionRecord
	{
		int					mFrame;					// +0x00
		double				mX;						// +0x08
		double				mY;						// +0x10
	};

public:
	int						mMinWordLength;			// +0x164 the shortest word of a shot in the abyss mode (3, then by the score 5 or 7)
	int						mMaxWordLength;			// +0x168 the longest (4, then 6 or 7); outside the abyss mode the words come from the last record of the lesson manager (not set, 0 here)
	int						mRotorFrame;			// +0x170 the frame of the rotor at the stern: counts up every mFrameDelay ticks, not while the boss dies
	int						mRotorFrames;			// +0x174 the rotor picture's width divided by 22: how many frames it has
	int						mSlotOffsetY[3];		// +0x178 the shot's y from the boss's y at the places 1, 2 and 3: 0x41, 0x65 and 0x87
	bool					mSlot1Taken;			// +0x184 ctor 0
	bool					mSlot2Taken;			// +0x185 ctor 0
	Image*					mRotorImage;			// +0x188 IMAGE_TORPEDO_BOSS_ROTOR (63)
	int						mRotorOffsetX;			// +0x18c 0xe3: where the rotor is drawn from the boss's x
	int						mRotorOffsetY;			// +0x190 0x36 and from its y
	int						mRotorWidth;			// +0x194 0x16: the width of one frame of the rotor
	bool					mSlot3Taken;			// +0x198 ctor 0
	WSTimer*				mSonarTimer;			// +0x19c new in the constructor, added to the widget manager, reset: times the sonar ping (every 2 seconds while the boss lives)
	std::vector<ExplosionRecord> mExplosions;		// +0x1a0 (the allocator at +0x1a0, the pointers at +0x1a4, +0x1a8, +0x1ac) the explosions of the dying boss (state 3)

public:
	TorpedoBoss(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig);	// 0x40c630
	virtual ~TorpedoBoss();															// destructor 0x408d00, deleting thunk 0x408df0

	virtual bool			HandleShot(Projectile* theShot) override;					// Enemy part slot 4, 0x40cba0
	virtual std::string		GetWord(int theMinLength, int theMaxLength);				// Enemy part slot 5, 0x4055c0
	virtual Projectile*		CreateProjectile();										// Enemy part slot 6, 0x40d5b0
	virtual void			Update() override;											// GameObject slot 1, 0x40cdc0
	virtual void			Draw(Graphics* g) override;									// slot 2, 0x405270
	virtual void			SetState(int theState) override;							// slot 7, 0x40ca50

	bool					ShotOverlaps(Projectile* theNewShot);						// 0x4054b0
};

}

#endif
