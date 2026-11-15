#ifndef __HEAVYWEAPON_ARMORY_H__
#define __HEAVYWEAPON_ARMORY_H__

// Armory: the screen between levels where the player spends upgrade points on the six special weapons. One point is
// earned for each level passed (at most 0x12 in all); the tile of each weapon (ArmoryWeaponPanel) has plus and minus
// buttons, the weapon under the mouse is described, and GO starts the next level and MAIN leaves for the main menu (both
// save the progress; GO refuses while points are left). Original: a Widget with a ButtonListener at +0x58, constructor
// 0x40f190, vtable 0x52b840 (listener 0x52b820), destructor body 0x40f0a0.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class BracketButton;
class ArmoryWeaponPanel;
class FlickerLight;
class HeavyWeaponApp;

class Armory : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_WEAPONS = 6,
		BUTTON_GO = 1000,		// start the next level
		BUTTON_MAIN = 0x3e9		// back to the main menu
	};

public:
	HeavyWeaponApp*		mApp;						// +0x5c
	BracketButton*		mGoButton;					// +0x60 id 1000
	BracketButton*		mMainButton;				// +0x64 id 0x3e9
	ArmoryWeaponPanel*	mPanels[NUM_WEAPONS];		// +0x68 one for each weapon the player has the level for
	int					mGlow[NUM_WEAPONS];			// +0x80 the weapon's flash after a change, fading 2 an update
	FlickerLight*		mFlickerLight;				// +0x98 the lamp over the picture
	int					mPointsSpent;				// +0x9c the points in the weapons now
	int					mPointsTotal;				// +0xa0 the points earned: the level, at most 0x12
	int					mSelectedWeapon;			// +0xa4 the weapon described, -1 for none
	int					mNumPanels;					// +0xa8 the weapons on offer: the level plus 2, at most 6

public:
	Armory(HeavyWeaponApp* theApp);														// 0x40f190
	virtual ~Armory();																	// 0x40f160 (body 0x40f0a0)

	virtual void	AddedToManager(WidgetManager* theManager) override;				// 0x40efd0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;			// 0x40f030 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x40ef20 slot 13
	virtual void	Draw(Graphics* g) override;											// 0x40f440 slot 16
	virtual void	Update() override;													// 0x40ee90 slot 17
	virtual void	MouseMove(int x, int y) override;									// 0x40f090 slot 26

	virtual void	ButtonPress(int theId) override;									// 0x410670 listener slot 4
	virtual void	ButtonDepress(int theId) override;									// 0x40fa90 listener slot 5

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
