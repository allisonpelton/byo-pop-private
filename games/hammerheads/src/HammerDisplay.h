#ifndef __HAMMERHEADS_HAMMERDISPLAY_H__
#define __HAMMERHEADS_HAMMERDISPLAY_H__

// The hammer on the shop screen's shelf (0x4491a0-0x449590) and the rules of the hammer: its level (0 to 3) and whether it
// is on fire are in the stats, and buying an upgrade starts a second in which the shelf still shows the old hammer.

#include "Object.h"

namespace Sexy
{

class Image;

enum HammerUpgradeState
{
	HAMMER_UPGRADE_LEVEL = 0,		// a hammer level was bought
	HAMMER_UPGRADE_FIRE = 1,		// the fire was bought
	HAMMER_UPGRADE_NONE = 2
};

class HammerDisplay : public Object
{
public:
	double			mUpgradeTimer;		// +0x80: a second after an upgrade, running down
	int				mUpgradeState;		// +0x88
	int				mLevelsBought;		// +0x8c: how many levels the upgrade added
	bool			mWasOnFire;			// +0x90: whether the old hammer was

public:
	HammerDisplay();												// 0x4496d0 (made by MakeHammerDisplay)
	virtual ~HammerDisplay();										// 0x4491a0

	OBJECT_TYPE(HammerDisplay)

	virtual void	Update(float theFrac) override;					// 0x449420
	virtual void	Draw(Graphics* g) override;						// 0x449590

	int				GetLevel();										// 0x4491b0: the level to show (the old one while the upgrade runs)
	bool			IsOnFire();										// 0x4491f0
	void			UpgradeToLevel(int theLevel);					// 0x449270 (1), 0x4492f0 (2), 0x449370 (3): the hammer is at least that level
	void			SetOnFire();									// 0x4493f0
};

// 0x449240: the display on screen
HammerDisplay* GetHammerDisplay();
// 0x4496d0: makes the display and remembers it
HammerDisplay* MakeHammerDisplay();

// 0x449470: how much damage a hammer of the level does (a gnome has a few points of health), twenty more while the
// double-hit power-up is on
int GetDamageOfLevel(int theLevel);
// 0x4494d0: the same for the hammer the player has
int GetHammerDamage();

}

#endif
