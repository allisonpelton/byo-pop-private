#ifndef __HEAVYWEAPON_BRACKETBUTTON_H__
#define __HEAVYWEAPON_BRACKETBUTTON_H__

// BracketButton: the game's wide menu button: the bracket picture, a tinted glow picture over it, the label in the
// game's font, and the pressed glow while it is held. Used by the armory (GO and MAIN) and the dialogs. Original: a
// ButtonWidget of 0x108 bytes, constructor 0x41cec0, vtable 0x52c9c8 (Draw 0x41d010, deleting destructor 0x41cfe0).

#include "ButtonWidget.h"
#include "Color.h"

namespace Sexy
{

class HeavyWeaponApp;

class BracketButton : public ButtonWidget
{
public:
	HeavyWeaponApp*	mApp;		// +0xf0
	Color			mTint;		// +0xf8 the glow picture's colour

public:
	BracketButton(HeavyWeaponApp* theApp, ButtonListener* theListener, int theId);	// 0x41cec0
	void			SetTint(const Color& theTint);									// 0x41cfb0
	virtual void	Draw(Graphics* g) override;										// 0x41d010 slot 16
};

}

#endif
