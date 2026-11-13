#ifndef __HEAVYWEAPON_ARMORYARROWBUTTON_H__
#define __HEAVYWEAPON_ARMORYARROWBUTTON_H__

// ArmoryArrowButton: the small plus or minus button beside a weapon in the armory. It draws one cel of the upgrade-button
// strip, added to the picture, and glows while the mouse is over it. Original: a ButtonWidget of 0x108 bytes, constructor
// 0x4300c0, vtable 0x52ff50 (deleting destructor 0x4302e0).

#include "ButtonWidget.h"

namespace Sexy
{

class HeavyWeaponApp;

class ArmoryArrowButton : public ButtonWidget
{
public:
	HeavyWeaponApp*	mApp;			// +0xf0
	int				mGlow;			// +0xf4 the glow's grey level, 0..255
	int				mGlowSpeed;		// +0xf8 how much it changes each update (12 over the button, -12 off it)
	int				mColumn;		// +0xfc the strip's column (the weapon's column in the armory)
	int				mRow;			// +0x100 the strip's row: the button's id, 0 plus and 1 minus

public:
	ArmoryArrowButton(HeavyWeaponApp* theApp, ButtonListener* theListener, int theId, int theColumn);	// 0x4300c0
	virtual void	Draw(Graphics* g) override;			// 0x430260 slot 16
	virtual void	Update() override;					// 0x4301c0 slot 17
	virtual void	MouseEnter() override;				// 0x430220 slot 24
	virtual void	MouseLeave() override;				// 0x430240 slot 25
};

}

#endif
