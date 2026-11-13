#ifndef __HEAVYWEAPON_MENUBUTTON_H__
#define __HEAVYWEAPON_MENUBUTTON_H__

// MenuButton: one of the main menu's six picture buttons. It draws nothing of its own but a glow: its picture is
// added over the menu at a grey level that rises while the mouse is over it. The main menu reads mGlow to fade
// the button's description in. Original: a ButtonWidget of 0x100 bytes, constructor 0x4258a0, vtable 0x52ddc0
// (deleting destructor 0x425ad0, body 0x425980).

#include "ButtonWidget.h"

namespace Sexy
{

class HeavyWeaponApp;
class Sprite;

class MenuButton : public ButtonWidget
{
public:
	enum
	{
		GLOW_MAX = 0x82,		// the glow's top grey level
		GLOW_SPEED = 10			// the glow's change an update
	};

	HeavyWeaponApp*	mApp;			// +0xf0
	Sprite*			mSprite;		// +0xf4 the glow's picture (set by the main menu)
	int				mGlow;			// +0xf8 the grey level the picture is added at, 0..GLOW_MAX
	int				mGlowSpeed;		// +0xfc

public:
	MenuButton(HeavyWeaponApp* theApp, ButtonListener* theListener, int theId);	// 0x4258a0
	virtual ~MenuButton();														// 0x425ad0 (body 0x425980)

	virtual void	Draw(Graphics* g) override;									// 0x425a60 slot 16
	virtual void	Update() override;											// 0x425990 slot 17
	virtual void	MouseEnter() override;										// 0x4259e0 slot 24
	virtual void	MouseLeave() override;										// 0x425a20 slot 25
};

}

#endif
